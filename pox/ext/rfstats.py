from pox.core import core
from pox.lib.revent import *
from pox.lib.recoco import Timer
from pox.openflow.libopenflow_01 import *
from pox.openflow import *
from pox.openflow.discovery import *
from pox.lib.addresses import *
from pox.topology.topology import *
import time
import json
import pymongo

UPDATE_INTERVAL = 5
topology_timer = None

log = core.getLogger("rfproxy")

def rf_id(id):
    return "{:#016x}".format(id)

class StatsDB:
    def __init__(self):
        self.db = {}
        self.connection = pymongo.Connection()
        self.collection = self.connection.db.rfstats

    def update(self, id_, type_, links=None, **data):
        if id_ not in self.db:
            if links is None:
                links = []
            self.db[id_] = {
                "type": type_,
                "links": links,
                "data": {},}
        else:
            self.db[id_]["type"] = type_
            if links is not None:
                self.db[id_]["links"] = links

        for k, v in data.items():
            self.db[id_]["data"][k] = v

        self.db[id_]["_id"] = id_
        self.collection.update({"_id": id_}, self.db[id_], upsert=True)

    def delete(self, id_):
        try:
            del self.db[id_]
            self.collection.remove(id_)
            return True
        except:
            return False
            
    @staticmethod
    def create_desc_stats_dict(desc):
        return {
        "mfr_desc": desc.mfr_desc,
        "hw_desc": desc.hw_desc,
        "sw_desc": desc.sw_desc,
        "serial_num": desc.serial_num,
        "dp_desc": desc.dp_desc,
        }

    @staticmethod
    def create_match_dict(match):
        # TODO: do this properly and not depending on POX behavior
        match = match.show().strip("\n").split("\n")            
        result = dict([attr.split(": ") for attr in match])
        try:
            w = result["wildcards"]
            p = w[w.find("nw_dst"):]
            s = p.find("(") + 2 # Strip (/
            e = p.find(")")
            netmask = int(p[s:e])
        except:
            netmask = 0
        if netmask != 0 and "nw_dst" in result:
            result["nw_dst"] = result["nw_dst"] + "/" + str(netmask)
        return result
        
    @staticmethod
    def create_actions_list(actions):
        # TODO: do this properly and not depending on POX behavior
        actionlist = []
        for action in actions:
            string = action.__class__.__name__ + "["
            string += action.show().strip("\n").replace("\n", ", ") + "]"
            actionlist.append(string)
        return actionlist

    @staticmethod
    def create_flow_stats_list(flows):
        flowlist = []
        for flow in flows:
            flowlist.append({
            "length": len(flow),
            "table_id": flow.table_id,
            "match": StatsDB.create_match_dict(flow.match),
            "duration_sec": flow.duration_sec,
            "duration_nsec": flow.duration_nsec,
            "priority": flow.priority,
            "idle_timeout": flow.idle_timeout,
            "hard_timeout": flow.hard_timeout,
            "cookie": flow.cookie,
            "packet_count": flow.packet_count,
            "byte_count": flow.byte_count,
            "actions": StatsDB.create_actions_list(flow.actions),
            })
        return flowlist

    @staticmethod
    def create_aggregate_stats_dict(aggregate):
        return {
        "packet_count": aggregate.packet_count,
        "byte_count": aggregate.byte_count,
        "flow_count": aggregate.flow_count,
        }

db = StatsDB()

def timer_func():
    topology = core.components['topology']
    for switch in topology.getEntitiesOfType(Switch):
        if switch.connected:
            try:
                # OFPST_DESC
                req = ofp_stats_request(type=OFPST_DESC)
                switch.send(req)
                # OFPST_FLOW
                req = ofp_stats_request(body=ofp_flow_stats_request())
                switch.send(req)
                # OFPST_AGGREGATE
                req = ofp_stats_request(body=ofp_aggregate_stats_request())
                switch.send(req)
            except:
                log.info("Failed to send stats request to switch")

def handle_switch_desc(event):
    dp_id = rf_id(event.connection.dpid)
    db.update(dp_id, "switch", desc=StatsDB.create_desc_stats_dict(event.stats))

def handle_flow_stats(event):
    dp_id = rf_id(event.connection.dpid)
    db.update(dp_id, "switch", flows=StatsDB.create_flow_stats_list(event.stats))

def handle_aggregate_flow_stats(event):
    dp_id = rf_id(event.connection.dpid)
    db.update(dp_id, "switch", aggregate=StatsDB.create_aggregate_stats_dict(event.stats))

def update_topology():
    topology = core.components['topology']
    rfproxy_links = ["rfserver"]
    for switch in topology.getEntitiesOfType(Switch):
        links = []
        
        # If we are not connected, delete
        # TODO: We could be using SwitchLeave, but since it isn't working...
        if not switch.connected:
            db.delete(rf_id(switch.dpid))
            continue
            
        # Keep the rfproxy link if we are connected
        links.append("rfproxy")
        rfproxy_links.append(rf_id(switch.dpid))
        
        # Connection to other switches
        for port in switch.ports:
            for entity in switch.ports[port].entities:
                links.append(rf_id(entity.dpid))
        db.update(rf_id(switch.dpid), "switch", links=links)
        
    db.update("rfproxy", "rfproxy", links=rfproxy_links)

def handle_link_event(event):
    # Every time the topology links change, update all the nodes
    # We use a timer to update only in the last event of a series
    global topology_timer
    if topology_timer is not None:
        topology_timer.cancel()
    topology_timer = Timer(UPDATE_INTERVAL, update_topology, recurring=False)

def handle_connection_up(event):
    db.update(rf_id(event.dpid), "switch", links=["rfproxy"])

def launch():
    core.openflow.addListenerByName("ConnectionUp", handle_connection_up)
    core.openflow.addListenerByName("SwitchDescReceived", handle_switch_desc)
    core.openflow.addListenerByName("FlowStatsReceived", handle_flow_stats)
    core.openflow.addListenerByName("AggregateFlowStatsReceived", handle_aggregate_flow_stats)
    core.openflow_discovery.addListenerByName("LinkEvent", handle_link_event)
    
    Timer(UPDATE_INTERVAL, timer_func, recurring=True)

    db.update("rfserver", "rfserver", links=["rfproxy"])
    db.update("rfproxy", "rfproxy", links=["rfserver"])
