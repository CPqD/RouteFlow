# Copyright 2008 (C) Nicira, Inc.
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
# Python L2 learning switch 
# ----------------------------------------------------------------------
#
# This app just drops to the python interpreter.  Useful for debugging
#

from nox.lib.core     import *
from nox.coreapps.pyrt.pycomponent import Table_stats_in_event, Aggregate_stats_in_event
from nox.lib.openflow import OFPST_TABLE,  OFPST_PORT, ofp_match, OFPP_NONE
from nox.lib.packet.packet_utils import longlong_to_octstr

MONITOR_TABLE_PERIOD     = 3
MONITOR_PORT_PERIOD      = 3
MONITOR_AGGREGATE_PERIOD = 3

class Monitor(Component):

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)

    def aggregate_timer(self, dpid):
        flow = ofp_match() 
        flow.wildcards = 0xffff
        self.ctxt.send_aggregate_stats_request(dpid, flow,  0xff)
        self.post_callback(MONITOR_TABLE_PERIOD, lambda : self.aggregate_timer(dpid))

    def table_timer(self, dpid):
        self.ctxt.send_table_stats_request(dpid)
        self.post_callback(MONITOR_TABLE_PERIOD, lambda : self.table_timer(dpid))

    def port_timer(self, dpid):
        self.ctxt.send_port_stats_request(dpid, OFPP_NONE)
        self.post_callback(MONITOR_PORT_PERIOD, lambda : self.port_timer(dpid))

    # For each new datapath that joins, create a timer loop that monitors
    # the statistics for that switch
    def datapath_join_callback(self, dpid, stats):
        self.post_callback(MONITOR_TABLE_PERIOD, lambda : self.table_timer(dpid))
        self.post_callback(MONITOR_PORT_PERIOD + 1, lambda :  self.port_timer(dpid))
        self.post_callback(MONITOR_AGGREGATE_PERIOD + 2, lambda :  self.aggregate_timer(dpid))

    def aggregate_stats_in_handler(self, dpid, stats):
        print "Aggregate stats in from datapath", longlong_to_octstr(dpid)[6:]
        print '\t',stats

    def table_stats_in_handler(self, dpid, tables):
        print "Table stats in from datapath", longlong_to_octstr(dpid)[6:]
        for item in tables:
            print '\t',item['name'],':',item['active_count']

    def port_stats_in_handler(self, dpid, ports):
        print "Port stats in from datapath", longlong_to_octstr(dpid)[6:]
        for item in ports:
            print '\t',item['port_no'],':',item['tx_packets']

    def install(self):
        self.register_for_datapath_join(lambda dpid, stats : self.datapath_join_callback(dpid,stats))
        self.register_for_table_stats_in(self.table_stats_in_handler)
        self.register_for_port_stats_in(self.port_stats_in_handler)
        self.register_for_aggregate_stats_in(self.aggregate_stats_in_handler)

    def getInterface(self):
        return str(Monitor)


def getFactory():
    class Factory:
        def instance(self, ctxt):
            return Monitor(ctxt)

    return Factory()
