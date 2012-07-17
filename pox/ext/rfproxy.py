import struct

from pox.core import core
from pox.openflow.libopenflow_01 import *
import pymongo as mongo

import rflib.ipc.IPC as IPC
import rflib.ipc.MongoIPC as MongoIPC
from rflib.ipc.RFProtocol import *
from rflib.openflow.rfofmsg import *
from rflib.defs import *

ipc = MongoIPC.MongoIPCMessageService(MONGO_ADDRESS, MONGO_DB_NAME, RFPROXY_ID)

conn = mongo.Connection()
rftable = conn[MONGO_DB_NAME][RF_TABLE_NAME]

def send_ofmsg(dp_id, ofmsg, success, fail):
    topology = core.components['topology']
    switch = topology.getEntityByID(dp_id)
    if switch is not None and switch.connected:
        switch.send(ofmsg)
        log.info(success)
    else:
        log.debug(fail)

class RFProcessor(IPC.IPCMessageProcessor):
    def process(self, from_, to, channel, msg):
        topology = core.components['topology']
        type_ = msg.get_type()
        if type_ == DATAPATH_CONFIG:
            dp_id = int(msg["dp_id"])
            operation_id = int(msg["operation_id"])

            ofmsg = create_config_msg(operation_id)
            send_ofmsg(dp_id, ofmsg,
                       "Flow modification (config) sent to datapath %i" % dp_id,
                       "Switch is disconnected, cannot send flow modification (config)")
        elif type_ == FLOW_MOD:
            dp_id = int(msg["dp_id"])

            netmask = str(msg["netmask"])
            # TODO: fix this. It was just to make it work...
            netmask = netmask.count("255")*8

            address = str(msg["address"]) + "/" + str(netmask)

            src_hwaddress = str(msg["src_hwaddress"])
            dst_hwaddress = str(msg["dst_hwaddress"])
            dst_port = int(msg["dst_port"])

            if (not msg["is_removal"]):
                ofmsg = create_flow_install_msg(address, netmask, src_hwaddress, dst_hwaddress, dst_port)
                send_ofmsg(dp_id, ofmsg,
                           "Flow modification (install) sent to datapath %i" % dp_id,
                           "Switch is disconnected, cannot send flow modification (install)")
            else:
                ofmsg = create_flow_remove_msg(address, netmask, src_hwaddress)
                send_ofmsg(dp_id, ofmsg,
                           "Flow modification (removal) sent to datapath %i" % dp_id,
                           "Switch is disconnected, cannot send flow modification (removal)")
                ofmsg = create_temporary_flow_msg(address, netmask, src_hwaddress)
                send_ofmsg(dp_id, ofmsg,
                           "Flow modification (temporary) sent to datapath %i" % dp_id,
                           "Switch is disconnected, cannot send flow modification (temporary)")

class RFFactory(IPC.IPCMessageFactory):
    def build_for_type(self, type_):
        if type_ == DATAPATH_CONFIG:
            return DatapathConfig()
        if type_ == FLOW_MOD:
            return FlowMod()

log = core.getLogger()

def dpid_to_switchname(dpid):
    dpid = hex(dpid)[2:][:-1]
    switchname = ""
    for char in [dpid[i:i+2] for i in xrange(0, len(dpid), 2)]:
        switchname += chr(int(char, 16))
    return switchname

def handle_connectionup(event):
    log.info("Datapath id=%s is up, installing config flows...", dpid_to_switchname(event.dpid))
    topology = core.components['topology']
    ports = topology.getEntityByID(event.dpid).ports
    for port in ports:
        msg = DatapathPortRegister(dp_id=str(event.dpid), dp_port=str(port.number))
        ipc.send(RFSERVER_RFPROXY_CHANNEL, RFSERVER_ID, msg)

def packet_out(data, dp_id, outport):
    msg = ofp_packet_out()
    msg.actions.append(ofp_action_output(port=outport))
    msg.data = data
    msg.in_port = OFPP_NONE
    topology = core.components['topology']
    switch = topology.getEntityByID(dp_id)
    if switch.connected:
        switch.send(msg)
    else:
        log.debug("Switch is disconnected, can't send packet out")

def handle_packetin(event):
    packet = event.parsed

    if packet.type == ethernet.LLDP_TYPE:
        return

    if packet.type == 0x0A0A:
        vm_id, vm_port = struct.unpack("QB", packet.raw[14:])
        dp_id = event.dpid
        in_port = event.port
        msg = PortMap(vm_id=str(vm_id),
                      vm_port=str(vm_port),
                      vs_id=str(dp_id),
                      vs_port=str(in_port))
        ipc.send(RFSERVER_RFPROXY_CHANNEL, RFSERVER_ID, msg)
        return

    results = rftable.find({VS_ID: str(event.dpid)})
    if results.count() == 0 or results[0][VM_ID] == "":
        results = rftable.find({VS_ID: {"$ne": ""}, DP_ID: str(event.dpid), DP_PORT: str(event.port)})
        if results.count() == 0:
            log.debug("Datapath not associated with a VM")
            return
        else:
            vs_id = int(results[0][VS_ID])
            vs_port = int(results[0][VS_PORT])
            packet_out(event.data, vs_id, vs_port)
    else:
        results = rftable.find({VS_PORT: str(event.port)})
        if results.count() == 0 or results[0][DP_ID] == "":
            log.debug("Datapath not associated with a VM")
            return
        else:
            dp_id = int(results[0][DP_ID])
            dp_port = int(results[0][DP_PORT])
            packet_out(event.data, dp_id, dp_port)

def launch ():
    core.openflow.addListenerByName("ConnectionUp", handle_connectionup)
    core.openflow.addListenerByName("PacketIn", handle_packetin)
    ipc.listen(RFSERVER_RFPROXY_CHANNEL, RFFactory(), RFProcessor(), False)
    log.info("RFProxy running.")
