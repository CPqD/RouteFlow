# Copyright 2008, 2009 (C) Nicira, Inc.
# 
# This file is part of NOX.
# 
# NOX is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# NOX is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with NOX.  If not, see <http://www.gnu.org/licenses/>.

#import redis
import pymongo
import time
from datetime import date
import logging
from nox.lib.core import *

from collections import defaultdict

import nox.lib.openflow as openflow
from nox.lib.packet.packet_utils  import mac_to_str

from nox.lib.netinet.netinet import datapathid,create_ipaddr,c_htonl
from nox.netapps.switchstats.pycswitchstats import pycswitchstats
from twisted.python import log

from nox.lib.directory import Directory
from nox.lib.directory import LocationInfo 

# Default values for the periodicity of polling for each class of
# statistic

DEFAULT_POLL_TABLE_PERIOD     = 3
DEFAULT_POLL_PORT_PERIOD      = 3
DEFAULT_POLL_AGGREGATE_PERIOD = 3
DEFAULT_POLL_FLOW_PERIOD      = 3
DEFAULT_POLL_FILE_PERIOD      = 5

#pool = redis.ConnectionPool(host='localhost', port=6379, db=0)
#r = redis.Redis(connection_pool=pool)
#r.flushall()
lg = logging.getLogger('switchstats')
dp_list = []

from pymongo import Connection
connection = Connection('localhost', 27017)
db = connection.test_database
mongo_stats = {}

## \ingroup noxcomponents
# Collects and maintains switch and port stats for the network.  
#
# Monitors switch and port stats by sending out port_stats requests
# periodically to all connected switches.  
#
# The primary method of accessing the ports stats is through the
# webserver (see switchstatsws.py)  however, components can also
# register port listeners which are called each time stats are
# received for a particular port.
#

class switchstats(Component):
    """Track switch statistics during runtime"""
    
    def add_port_listener(self, dpid, port, listener):
        self.port_listeners[dpid][port].append(listener)

    def remove_port_listener(self, dpid, port, listener):
        try:
            self.port_listeners[dpid][port].remove(listener)
        except Exception, e: 
            lg.warn('Failed to remove port %d from dpid %d' %(port, dpid))
            pass

    def fire_port_listeners(self, dpid, portno, port):        
        for listener in self.port_listeners[dpid][portno]:
            if not listener(port):
                self.remove_port_listener(dpid, portno, listener)

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)

        # {dpid : {port : [listeners]}}
        self.port_listeners = defaultdict(lambda: defaultdict(list)) 

        self.dp_stats = {} 

        self.dp_poll_period = {}
        self.dp_table_stats = {}
        self.dp_desc_stats = {}
        self.dp_port_stats  = {}
	self.dp_aggr_stats ={}
	self.dp_flow_stats = {}


    def port_timer(self, dp):
        if dp in self.dp_stats:
            #self.ctxt.send_port_stats_request(dp, openflow.OFPP_NONE)
            self.post_callback(self.dp_poll_period[dp]['port'] + 1, lambda :  self.port_timer(dp))

    def table_timer(self, dp):
        if dp in self.dp_stats:
            self.ctxt.send_table_stats_request(dp)
            self.post_callback(self.dp_poll_period[dp]['table'], lambda : self.table_timer(dp))

    def aggr_timer(self, dp):
        if dp in self.dp_stats:
            match = openflow.ofp_match()
            match.wildcards = 0xffffffff
            self.ctxt.send_aggregate_stats_request(dp, match,  0xff)
            self.post_callback(self.dp_poll_period[dp]['aggr'], lambda : self.aggr_timer(dp))

    def flow_timer(self, dp):
        if dp in self.dp_stats:
            match = openflow.ofp_match()
            match.wildcards = 0xffffffff
            self.ctxt.send_flow_stats_request(dp, match,  0xff)
	    #lg.debug("Mandei o pedido de flowstats para o dpid " + mac_to_str(dp))
            self.post_callback(self.dp_poll_period[dp]['flow'], lambda : self.flow_timer(dp))

    def file_timer(self):
	self.print_stats()
	self.post_callback(DEFAULT_POLL_FILE_PERIOD, lambda : self.file_timer())
       
    def dp_join(self, dp, stats):
	#lg.warn('############### dp_join ###############\n')
	global dp_num
	if dp in dp_list:
		dp_list.remove(dp)
	#lg.warn(dp)
	dp_list.append(dp)
	dp_list.sort()
        dpid_obj = datapathid.from_host(dp)
        stats['dpid']     = dp 
        self.dp_stats[dp] = stats
       
        # convert all port hw_addrs to ASCII
        # and register all port names with bindings storage
   
        port_list = self.dp_stats[dp]['ports']
        for i in range(0,len(port_list)):
          new_mac = mac_to_str(port_list[i]['hw_addr']).replace(':','-')
          port_list[i]['hw_addr'] = new_mac 

        # polling intervals for switch statistics
        self.dp_poll_period[dp] = {} 
        self.dp_poll_period[dp]['table'] = DEFAULT_POLL_TABLE_PERIOD
        self.dp_poll_period[dp]['port']  = DEFAULT_POLL_PORT_PERIOD
        self.dp_poll_period[dp]['aggr']  = DEFAULT_POLL_AGGREGATE_PERIOD
	self.dp_poll_period[dp]['flow']  = DEFAULT_POLL_FLOW_PERIOD

        # Switch descriptions do not change while connected, so just send once
        #self.ctxt.send_desc_stats_request(dp)
           
        # stagger timers by one second
        self.post_callback(self.dp_poll_period[dp]['table'], 
                              lambda : self.table_timer(dp))
        self.post_callback(self.dp_poll_period[dp]['port'] + 0.5, 
                              lambda : self.port_timer(dp))
	self.post_callback(self.dp_poll_period[dp]['aggr'] + 1,
                              lambda : self.aggr_timer(dp))
	self.post_callback(self.dp_poll_period[dp]['flow'] + 1.5,
                              lambda : self.flow_timer(dp))
	self.post_callback(DEFAULT_POLL_FILE_PERIOD,
                              lambda : self.file_timer())


	mongo_stats[dp] = {}
	mongo_stats[dp]["table"] = db.mongo_stats[dp]["table"]
	mongo_stats[dp]["aggr"] = db.mongo_stats[dp]["aggr"]
	mongo_stats[dp]["desc"] = db.mongo_stats[dp]["desc"]
	mongo_stats[dp]["flow"] = db.mongo_stats[dp]["flow"]

        return CONTINUE 
                    
    def dp_leave(self, dp): 
        dpid_obj = datapathid.from_host(dp)

        if self.dp_stats.has_key(dp):
            del self.dp_stats[dp]  
        else:    
            log.err('Unknown datapath leave', system='switchstats')

        if self.dp_poll_period.has_key(dp):
            del self.dp_poll_period[dp]  
        if self.dp_table_stats.has_key(dp):
            del self.dp_table_stats[dp]  
        if self.dp_desc_stats.has_key(dp):
            del self.dp_desc_stats[dp]  
        if self.dp_port_stats.has_key(dp):
            del self.dp_port_stats[dp]  
	if self.dp_aggr_stats.has_key(dp):
            del self.dp_aggr_stats[dp]
	if self.dp_flow_stats.has_key(dp):
            del self.dp_flow_stats[dp]
        if dp in self.port_listeners:    
            del self.port_listeners[dp]

        return CONTINUE


    def map_name_to_portno(self, dpid, name):
        for port in self.dp_stats[dpid]['ports']:
            if port['name'] == name:
                return port['port_no']
        return None        
            
    def table_stats_in_handler(self, dpid, tables):
	#lg.warn('############### table_stats_in_handler ###############\n')
        self.dp_table_stats[dpid] = tables
	for i in range(len(tables)):
		#print "table[",i,"]"

		doc =	{"_id": self.dp_table_stats[dpid][i]['table_id'],
                        "name": self.dp_table_stats[dpid][i]['name'],
                        "active_count": self.dp_table_stats[dpid][i]['active_count'],
                        "lookup_count": self.dp_table_stats[dpid][i]['lookup_count']}

		mongo_stats[dpid]["table"].save(doc)

#		print "*** Estatisticas com o MongoDB ***"
#		for x in mongo_stats[dpid]["table"].find({"_id":self.dp_table_stats[dpid][i]['table_id']}):
#			print "mongo_table[",dpid,"].table_id = ", x["_id"]
#			print "mongo_table[",dpid,"].name = ", x["name"]
#			print "mongo_table[",dpid,"].active_count = ", x["active_count"]
#			print "mongo_table[",dpid,"].lookup_count = ", x["lookup_count"]
#	print "*****"


    def desc_stats_in_handler(self, dpid, desc):
	#lg.warn('############### desc_stats_in_handler ###############\n')
        self.dp_desc_stats[dpid] = desc
        ip = self.ctxt.get_switch_ip(dpid)
        self.dp_desc_stats[dpid]["ip"] = str(create_ipaddr(c_htonl(ip)))

	mongo_stats[dpid]["desc"].drop()

	doc =   {"mfr_desc": self.dp_desc_stats[dpid]['mfr_desc'],
		"hw_desc": self.dp_desc_stats[dpid]['hw_desc'],
                "sw_desc": self.dp_desc_stats[dpid]['sw_desc'],
                "serial_num": self.dp_desc_stats[dpid]['serial_num'],
		"dp_desc": self.dp_desc_stats[dpid]['dp_desc']}

        mongo_stats[dpid]["desc"].save(doc)

#	print "***** mongodb desc *****"

#	for x in mongo_stats[dpid]["desc"].find():
#                print "mongo_desc[",dpid,"].table_id = ", x["mfr_desc"]
#                print "mongo_desc[",dpid,"].name = ", x["hw_desc"]
#                print "mongo_desc[",dpid,"].active_count = ", x["sw_desc"]
#                print "mongo_desc[",dpid,"].lookup_count = ", x["serial_num"]
#		print "mongo_desc[",dpid,"].lookup_count = ", x["dp_desc"]
#        print "*****"


    def aggr_stats_in_handler(self, dpid, aggr):
	#lg.warn('############### aggr_stats_in_handler ###############\n')
        self.dp_aggr_stats[dpid] = aggr

	#r.set('dp_aggr_stats:' + str(dpid) + ':packet_count', self.dp_aggr_stats[dpid]['packet_count'])
	#r.set('dp_aggr_stats:' + str(dpid) + ':byte_count', self.dp_aggr_stats[dpid]['byte_count'])
	#r.set('dp_aggr_stats:' + str(dpid) + ':flow_count', self.dp_aggr_stats[dpid]['flow_count'])

	mongo_stats[dpid]["aggr"].drop()

        doc =   {"packet_count": self.dp_aggr_stats[dpid]['packet_count'],
                "byte_count": self.dp_aggr_stats[dpid]['byte_count'],
                "flow_count": self.dp_aggr_stats[dpid]['flow_count']}

        mongo_stats[dpid]["aggr"].save(doc)


#	print "***** mongodb Aggr *****"
#        for x in mongo_stats[dpid]["aggr"].find():
#                print "mongo_aggr[",dpid,"].packet_count = ", x["packet_count"]
#                print "mongo_aggr[",dpid,"].byte_count = ", x["byte_count"]
#                print "mongo_aggr[",dpid,"].flow_count = ", x["flow_count"]
#        print "*****"

	#print "dp_aggr_stats[",dpid,"].packet_count = ", r.get('dp_aggr_stats:' + str(dpid) + ':packet_count')
	#print "dp_aggr_stats[",dpid,"].byte_count = ", r.get('dp_aggr_stats:' + str(dpid) + ':byte_count')
	#print "dp_aggr_stats[",dpid,"].flow_count = ", r.get('dp_aggr_stats:' + str(dpid) + ':flow_count')

    def flow_stats_in_handler(self, dpid, flows):
        #lg.warn('############### flow_stats_in_handler ###############\n')
	self.dp_flow_stats[dpid] = flows
	mongo_stats[dpid]["flow"].drop()
	for i in range(len(flows)):
        #print "flow[",i,"]"

		#r.hset('dp_flow_stats:' + str(dpid) + ':packet_count', i, self.dp_flow_stats[dpid][i]['packet_count'])
		#r.hset('dp_flow_stats:' + str(dpid) + ':byte_count', i,  self.dp_flow_stats[dpid][i]['byte_count'])
		
		match = ""
		k = self.dp_flow_stats[dpid][i]['match']
		if k.has_key('in_port'):
			match = "in_port: " + str(k['in_port'])
		if k.has_key('dl_src'):
                        match = "dl_src: " + str(ethernetaddr(k['dl_src']))

                elif k.has_key('dl_dst'):
			if k['dl_type'] == 2048:
                		match = "ip"
			elif k['dl_type'] == 2054:
				match = "ethernet"

			match = match + ", dl_dst: " + str(ethernetaddr(k['dl_dst'])) + "; nw_dst: " + str(ipaddr(k['nw_dst']))
			if k.has_key('nw_dst_n_wild'):
				if k['nw_dst_n_wild'] > 0:
					match = match + "/" + str(32 - k['nw_dst_n_wild'])

                elif k.has_key('nw_dst'):
			if k['nw_proto'] == 17:
	                	match = "udp"
			match = match + "; nw_dst: " + str(ipaddr(k['nw_dst']))

                elif k.has_key('nw_dst_mask'):
        	        match = "nw_dst_mask: " + str(ipaddr(k['nw_dst_mask']))

		elif k.has_key('nw_proto'):
                        match = "" 
			if k['nw_proto'] == 89:
				match = match + "ospf"
				match = match + "; tp_src: " + str(k['tp_src']) + "; tp_dst: " + str(k['tp_dst'])
			if k['nw_proto'] == 6:
                                match = match + "tcp"
				match = match + "; tp_dst: " + str(k['tp_dst'])
			if k['nw_proto'] == 1:
                                match = match + "icmp"
		elif k.has_key('dl_type'):
			if k['dl_type'] == 2054:
				match = "arp"
		#match = str(k) + match

		cont = 0
		actions = []
		actionStr = ""
		for j in self.dp_flow_stats[dpid][i]['actions']:
			del j['len']
			if j['type'] == 0:
				j['type'] = "OUTPUT"
				actions.insert(cont, str(j['type'] + ': ' + 'port ' + str(j['port'])))
			elif j['type'] == 1:
				j['type'] = "SET_VLAN_VID"
			elif j['type'] == 2:
				j['type'] = "SET_VLAN_PCP"
			elif j['type'] == 3:
				j['type'] = "STRIP_VLAN"
			elif j['type'] == 4:
				j['type'] = "SET_DL_SRC"
				actions.insert(cont, str(j['type'] + ': ' + str(ethernetaddr(j['dl_src']))))
			elif j['type'] == 5:
                                j['type'] = "SET_DL_DST"
				actions.insert(cont, str(j['type'] + ': ' + str(ethernetaddr(j['dl_dst']))))
			elif j['type'] == 6:
                                j['type'] = "SET_NW_SRC"
                        elif j['type'] == 7:
                                j['type'] = "OFPAT_SET_NW_DST"
                        elif j['type'] == 8:
                                j['type'] = "OFPAT_SET_NW_TOS"
                        elif j['type'] == 9:
                                j['type'] = "OFPAT_SET_TP_SRC"
			elif j['type'] == 10:
                                j['type'] = "OFPAT_SET_TP_DST"
			else:
			    pass
				# print "ACTION TYPE: ",j['type']

			cont = cont + 1

		for j in range(len(self.dp_flow_stats[dpid][i]['actions'])):
			actionStr = actionStr + actions[j]
			if j != len(self.dp_flow_stats[dpid][i]['actions']):
				actionStr = actionStr + "; "

		#r.hset('dp_flow_stats:' + str(dpid) + ':packet_count', i, self.dp_flow_stats[dpid][i]['packet_count'])
                #r.hset('dp_flow_stats:' + str(dpid) + ':byte_count', i,  self.dp_flow_stats[dpid][i]['byte_count'])
		#r.hset('dp_flow_stats:' + str(dpid) + ':match', i, match)
		#r.hset('dp_flow_stats:' + str(dpid) + ':actions', i, actionStr)

		doc =   {"_id": i,
			"packet_count": self.dp_flow_stats[dpid][i]["packet_count"],
                        "byte_count": self.dp_flow_stats[dpid][i]["byte_count"],
                        "match": match,
                        "actions": actionStr}

		#mongo_stats[dpid]["flow"].drop()
                mongo_stats[dpid]["flow"].save(doc)

		#print "REDIS FLOW_STATS"
		#print "dp_flow_stats[",dpid,"].packet_count = ", r.hget('dp_flow_stats:' + str(dpid) + ':packet_count', i)
		#print "dp_flow_stats[",dpid,"].byte_count = ", r.hget('dp_flow_stats:' + str(dpid) + ':byte_count', i)
		#print "dp_flow_stats[",dpid,"].match = ", r.hget('dp_flow_stats:' + str(dpid) + ':match', i)
		#print "dp_flow_stats[",dpid,"].actions = ", r.hget('dp_flow_stats:' + str(dpid) + ':actions', i)

#		print "MONGO FLOW_STATS"
#		for x in mongo_stats[dpid]["flow"].find({"_id": i}):
#	                print "mongo_flow[",dpid,"].packet_count = ", x["packet_count"]
#        	        print "mongo_flow[",dpid,"].byte_count = ", x["byte_count"]
#                	print "mongo_flow[",dpid,"].match = ", x["match"]
#	                print "mongo_flow[",dpid,"].actions = ", x["actions"]
#        print "*****"


    """def port_stats_in_handler(self, dpid, ports):
        if dpid not in self.dp_port_stats:
            new_ports = {}
            for port in ports:
                port['delta_bytes'] = 0 
                new_ports[port['port_no']] = port
            self.dp_port_stats[dpid] = new_ports 
            return
        new_ports = {}
        for port in ports:    
            if port['port_no'] in self.dp_port_stats[dpid]:
                port['delta_bytes'] = port['tx_bytes'] - \
                            self.dp_port_stats[dpid][port['port_no']]['tx_bytes']
                new_ports[port['port_no']] = port
            else:    
                port['delta_bytes'] = 0 
                new_ports[port['port_no']] = port
            # XXX Fire listeners for port stats    
            self.fire_port_listeners(dpid, port['port_no'], port)
        self.dp_port_stats[dpid] = new_ports 
   """

    def port_status_handler(self, dpid, reason, port):
	#lg.warn('############### port_status_in_handler ###############\n')
        intdp = int(dpid)
        if intdp not in self.dp_stats:
            log.err('port status from unknown datapath', system='switchstats')
            return
        # copy over existing port status
        for i in range(0, len(self.dp_stats[intdp]['ports'])):
            oldport  = self.dp_stats[intdp]['ports'][i]
            if oldport['name'] == port['name']:
                port['hw_addr'] = mac_to_str(port['hw_addr']).replace(':','-')
                self.dp_stats[intdp]['ports'][i] = port

    def get_switch_conn_p_s_heavy_hitters(self):
        hitters = []
        for dp in self.dp_stats:
            hitters.append((dp, self.cswitchstats.get_switch_conn_p_s(dp)))
        return hitters

    def get_switch_port_error_heavy_hitters(self): 
        error_list = []
        for dpid in self.dp_port_stats:
            ports = self.dp_port_stats[dpid].values()
            for port in ports:
                error_list.append((dpid, port['port_no'], port['rx_errors'] + port['tx_errors']))
        return error_list    

    def get_switch_port_bandwidth_hitters(self): 
        error_list = []
        for dpid in self.dp_port_stats:
            ports = self.dp_port_stats[dpid].values()
            for port in ports:
                error_list.append((dpid, port['port_no'], 
                  (port['delta_bytes']) / DEFAULT_POLL_PORT_PERIOD))
        return error_list    
            
    def get_global_conn_p_s(self):
        return self.cswitchstats.get_global_conn_p_s()

    def get_switch_conn_p_s(self, dpid):
        return self.cswitchstats.get_switch_conn_p_s(datapathid.from_host(dpid))

    def install(self):
        self.cswitchstats     = self.resolve(pycswitchstats)

        self.register_for_datapath_join (self.dp_join)
        self.register_for_datapath_leave(self.dp_leave)

        self.register_for_table_stats_in(self.table_stats_in_handler)
	self.register_for_flow_stats_in(self.flow_stats_in_handler)
        self.register_for_desc_stats_in(self.desc_stats_in_handler)
	self.register_for_aggregate_stats_in(self.aggr_stats_in_handler)
        #self.register_for_port_stats_in(self.port_stats_in_handler)
        self.register_for_port_status(self.port_status_handler)

    def getInterface(self):
        return str(switchstats)

    def print_stats(self):
	pass
	#if dpid == 5: 
	f = open("../../../rfweb/data/switchstats.json", "w")
	f.write('{ "nodes": [\n')
	for i in range(len(dp_list)):
		f.write('\t{\n')
		value = '\t\t"id": "' + str(dp_list[i]) + '",\n'
		f.write(value)

		value = '\t\t"name": "switch' + str(dp_list[i])  + '",\n'
                f.write(value)

                value = '\t\t"data": {\n'
		f.write(value)

		dp_id = "dp" + str(i+1)
                value = '\t\t\t"$dp_id": "' + str(dp_id) + '",\n'
                f.write(value)
	
		# IMPRIMINDO FLOW_STATS
		f.write('\t\t\t"$flows": [\n')
		if self.dp_flow_stats:



			#for j in range(len(self.dp_flow_stats[dp_list[i]])):
			#	f.write('\t\t\t{\n')
#
#				value = '\t\t\t\t"flow": "' + str(j) + '",\n'
 #                               f.write(value)
  #                              value = '\t\t\t\t"ofp_match": "' + r.hget('dp_flow_stats:' + str(dp_list[i]) + ':match', j) + '",\n'
   #                             f.write(value)

    #                            value = '\t\t\t\t"ofp_actions": "' + r.hget('dp_flow_stats:' + str(dp_list[i]) + ':actions', j) + '",\n'
 
#				f.write(value)
 #                               value = '\t\t\t\t"packet_count": "' + r.hget('dp_flow_stats:' + str(dp_list[i]) + ':packet_count', j) + '",\n'
  #                              f.write(value)
   #                             value = '\t\t\t\t"byte_count": "' + r.hget('dp_flow_stats:' + str(dp_list[i]) + ':byte_count', j) + '"\n'
    #                            f.write(value)
#				if j < (len(self.dp_flow_stats[dp_list[i]])-1):
#					f.write('\t\t\t},\n')
#				else:
#					f.write('\t\t\t}\n')


			l = mongo_stats[dp_list[i]]["flow"].find().explain()["nscanned"]
                	j = 0
			for x in mongo_stats[dp_list[i]]["flow"].find():
				f.write('\t\t\t{\n')

				value = '\t\t\t\t"flow": "' + str(j) + '",\n'
				f.write(value) 
				value = '\t\t\t\t"ofp_match": "' + x["match"] + '",\n'
				f.write(value)
				value = '\t\t\t\t"ofp_actions": "' + x["actions"] + '",\n'
				f.write(value)
				value = '\t\t\t\t"packet_count": "' + str(x["packet_count"]) + '",\n'
				f.write(value)
				value = '\t\t\t\t"byte_count": "' + str(x["byte_count"]) + '"\n'
				f.write(value)
				if j < (l-1):
                                        f.write('\t\t\t},\n')
                                else:
                                        f.write('\t\t\t}\n')
				j = j + 1

                f.write('\t\t\t],\n')

		# IMPRIMINDO DESC_STATS
		x = mongo_stats[dp_list[i]]["desc"].find_one()
		value = '\t\t\t"$ofp_desc_stats":["' + str(x["mfr_desc"]) + '", '
		value = value + '"' + str(x["hw_desc"]) + '", '
		value = value + '"' + str(x["sw_desc"]) + '", '
		value = value + '"' + str(x["serial_num"]) + '", '
		value = value + '"' + str(x["dp_desc"]) + '"],\n'

		f.write(value)

		# IMPRIMINDO AGGREGATE STATS
		x = mongo_stats[dp_list[i]]["aggr"].find_one()
		if (x is not None):		
                    value = '\t\t\t"$ofp_aggr_stats":["' + str(x["packet_count"]) + '", '
                    value = value + '"' + str(x["byte_count"]) + '", '
                    value = value + '"' + str(x["flow_count"]) + '"],\n'

		#value = '\t\t\t"$ofp_aggr_stats":["' + r.get('dp_aggr_stats:' + str(dp_list[i]) + ':packet_count') + '", '
                #value = value + '"' + r.get('dp_aggr_stats:' + str(dp_list[i]) + ':byte_count') + '", '
                #value = value + '"' + r.get('dp_aggr_stats:' + str(dp_list[i]) + ':flow_count') + '"],\n'

		f.write(value)

		# IMPRIMINDO TABLE_STATS
		value = '\t\t\t"$ofp_table_stats":['
		l = mongo_stats[dp_list[i]]["table"].find().explain()["nscanned"]
		j = 0
		for x in mongo_stats[dp_list[i]]["table"].find():
			value = value + '['
                        value = value + '"' + str(x["_id"]) + '", '
	                value = value + '"' + str(x["name"]) + '", '
        	        value = value + '"' + str(x["active_count"]) + '", '
                	value = value + '"' + str(x["lookup_count"])

			if j < l-1:
				value = value + '"],'
			else:
				value = value + '"]]\n'
			j = j + 1

		f.write(value)
	
		f.write('\t\t}\n')
		if i < (len(dp_list)-1):
			f.write('\t},\n')
		else:
			f.write('\t}\n')

        f.write(']}')
        f.close()

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return switchstats(ctxt)

    return Factory()
