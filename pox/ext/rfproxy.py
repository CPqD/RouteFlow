import struct
import logging
import threading
import time

from pox.core import core
from pox.openflow.libopenflow_01 import *
import pymongo as mongo

import rflib.ipc.IPC as IPC
import rflib.ipc.MongoIPC as MongoIPC
from rflib.ipc.RFProtocol import *
from rflib.ipc.RFProtocolFactory import RFProtocolFactory
from rflib.defs import *
from rfofmsg import *

FAILURE = 0
SUCCESS = 1

# Association table
class Table:
    def __init__(self):
        self.dp_to_vs = {}
        self.vs_to_dp = {}

    def update_dp_port(self, dp_id, dp_port, vs_id, vs_port):
        # If there was a mapping for this DP port, reset it
        if (dp_id, dp_port) in self.dp_to_vs:
            old_vs_port = self.dp_to_vs[(dp_id, dp_port)]
            del self.vs_to_dp[old_vs_port]
        self.dp_to_vs[(dp_id, dp_port)] = (vs_id, vs_port)
        self.vs_to_dp[(vs_id, vs_port)] = (dp_id, dp_port)

    def dp_port_to_vs_port(self, dp_id, dp_port):
        try:
            return self.dp_to_vs[(dp_id, dp_port)]
        except KeyError:
            return None

    def vs_port_to_dp_port(self, vs_id, vs_port):
        try:
            return self.vs_to_dp[(vs_id, vs_port)]
        except KeyError:
            return None

    def delete_dp(self, dp_id):
        for (id_, port) in self.dp_to_vs.keys():
            if id_ == dp_id:
                del self.dp_to_vs[(id_, port)]

        for key in self.vs_to_dp.keys():
            id_, port = self.vs_to_dp[key]
            if id_ == dp_id:
                del self.vs_to_dp[key]

    # We're not considering the case of this table becoming invalid when a
    # datapath goes down. When the datapath comes back, the server recreates
    # the association, forcing new map messages to be generated, overriding the
    # previous mapping.
    # If a packet comes and matches the invalid mapping, it can be redirected
    # to the wrong places. We have to fix this.

netmask_prefix = lambda a: sum([bin(int(x)).count("1") for x in a.split(".", 4)])

# TODO: add proper support for ID
ID = 0
ipc = MongoIPC.MongoIPCMessageService(MONGO_ADDRESS, MONGO_DB_NAME, str(ID),
                                      threading.Thread, time.sleep)
table = Table()

# Logging
log = core.getLogger("rfproxy")

# Base methods
def send_of_msg(dp_id, ofmsg):
    topology = core.components['topology']
    switch = topology.getEntityByID(dp_id)
    if switch is not None and switch.connected:
        try:
            switch.send(ofmsg)
        except:
            return FAILURE
        return SUCCESS
    else:
        return FAILURE

def send_packet_out(dp_id, port, data):
    msg = ofp_packet_out()
    msg.actions.append(ofp_action_output(port=port))
    msg.data = data
    msg.in_port = OFPP_NONE
    topology = core.components['topology']
    switch = topology.getEntityByID(dp_id)
    if switch is not None and switch.connected:
        try:
            switch.send(msg)
        except:
            return FAILURE
        return SUCCESS
    else:
        return FAILURE

# Event handlers
def on_datapath_up(event):
    topology = core.components['topology']
    dp_id = event.dpid

    ports = topology.getEntityByID(dp_id).ports
    for port in ports:
        if port <= OFPP_MAX:
            msg = DatapathPortRegister(ct_id=ID, dp_id=dp_id, dp_port=port)
            ipc.send(RFSERVER_RFPROXY_CHANNEL, RFSERVER_ID, msg)
            log.info("Registering datapath port (dp_id=%s, dp_port=%d)",
                     format_id(dp_id), port)

def on_datapath_down(event):
    dp_id = event.dpid

    log.info("Datapath is down (dp_id=%s)", format_id(dp_id))

    table.delete_dp(dp_id)

    msg = DatapathDown(ct_id=ID, dp_id=dp_id)
    ipc.send(RFSERVER_RFPROXY_CHANNEL, RFSERVER_ID, msg)

def on_packet_in(event):
    packet = event.parsed
    dp_id = event.dpid
    in_port = event.port

    # Drop all LLDP packets
    if packet.type == ethernet.LLDP_TYPE:
        return

    # If we have a mapping packet, inform RFServer through a Map message
    if packet.type == RF_ETH_PROTO:
        vm_id, vm_port = struct.unpack("QB", packet.raw[14:])

        log.info("Received mapping packet (vm_id=%s, vm_port=%d, vs_id=%s, vs_port=%d)",
                 format_id(vm_id), vm_port, event.dpid, event.port)

        msg = VirtualPlaneMap(vm_id=vm_id, vm_port=vm_port,
                              vs_id=event.dpid, vs_port=event.port)
        ipc.send(RFSERVER_RFPROXY_CHANNEL, RFSERVER_ID, msg)
        return

    # If the packet came from RFVS, redirect it to the right switch port
    if is_rfvs(event.dpid):
        dp_port = table.vs_port_to_dp_port(dp_id, in_port)
        if dp_port is not None:
            dp_id, dp_port = dp_port
            send_packet_out(dp_id, dp_port, event.data)
        else:
            log.debug("Unmapped RFVS port (vs_id=%s, vs_port=%d)",
                      format_id(dp_id), in_port)
    # If the packet came from a switch, redirect it to the right RFVS port
    else:
        vs_port = table.dp_port_to_vs_port(dp_id, in_port)
        if vs_port is not None:
            vs_id, vs_port = vs_port
            send_packet_out(vs_id, vs_port, event.data)
        else:
            log.debug("Unmapped datapath port (dp_id=%s, dp_port=%d)",
                      format_id(dp_id), in_port)

# IPC message Processing
class RFProcessor(IPC.IPCMessageProcessor):
    def process(self, from_, to, channel, msg):
        topology = core.components['topology']
        type_ = msg.get_type()
        if type_ == ROUTE_MOD:
            try:
                ofmsg = create_flow_mod(msg)
            except Warning as e:
                log.info("Error creating FlowMod: {}" % str(e))
            if send_of_msg(msg.get_id(), ofmsg) == SUCCESS:
                log.info("routemod sent to datapath (dp_id=%s)",
                         format_id(msg.get_id()))
            else:
                log.info("Error sending routemod to datapath (dp_id=%s)",
                         format_id(msg.get_id()))
        if type_ == DATA_PLANE_MAP:
            table.update_dp_port(msg.get_dp_id(), msg.get_dp_port(),
                                 msg.get_vs_id(), msg.get_vs_port())

        return True

# Initialization
def launch ():
    core.openflow.addListenerByName("ConnectionUp", on_datapath_up)
    core.openflow.addListenerByName("ConnectionDown", on_datapath_down)
    core.openflow.addListenerByName("PacketIn", on_packet_in)
    ipc.listen(RFSERVER_RFPROXY_CHANNEL, RFProtocolFactory(), RFProcessor(), False)
    log.info("RFProxy running.")
