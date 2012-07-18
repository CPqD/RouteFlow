import struct

from pox.core import core
from pox.openflow.libopenflow_01 import *
import pymongo as mongo

import rflib.ipc.IPC as IPC
import rflib.ipc.MongoIPC as MongoIPC
from rflib.ipc.RFProtocol import *
from rflib.openflow.rfofmsg import *
from rflib.rftable.rftable import *
from rflib.ipc.rfprotocolfactory import RFProtocolFactory
from rflib.defs import *

netmask_prefix = lambda a: sum([bin(int(x)).count("1") for x in a.split(".", 4)])
ipc = MongoIPC.MongoIPCMessageService(MONGO_ADDRESS, MONGO_DB_NAME, RFPROXY_ID)
rftable = RFTable()
log = core.getLogger()

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
            dp_id = msg.get_dp_id()
            operation_id = msg.get_operation_id()
            ofmsg = create_config_msg(operation_id)
            send_ofmsg(dp_id, ofmsg,
                       "Flow modification (config) sent to datapath %i" % dp_id,
                       "Switch is disconnected, cannot send flow modification (config)")
        elif type_ == FLOW_MOD:
            dp_id = msg.get_dp_id()
            netmask = msg.get_netmask()
            netmask = netmask_prefix(netmask)
            address = msg.get_address() + "/" + str(netmask)
            src_hwaddress = msg.get_src_hwaddress()
            dst_hwaddress = msg.get_dst_hwaddress()
            dst_port = msg.get_dst_port()

            if (not msg.get_is_removal()):
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
        return True
    
def dpid_to_switchname(dpid):
    dpid = hex(dpid)[2:][:-1]
    switchname = ""
    for char in [dpid[i:i+2] for i in xrange(0, len(dpid), 2)]:
        switchname += chr(int(char, 16))
    return switchname

def handle_connectionup(event):
    log.info("Datapath id=%s is up, installing config flows...", event.dpid)
    topology = core.components['topology']
    ports = topology.getEntityByID(event.dpid).ports
    for port in ports:
        # Discard the local OpenFlow port
        if port == OFPP_LOCAL:
            continue
        msg = DatapathPortRegister(dp_id=event.dpid, dp_port=port)
        ipc.send(RFSERVER_RFPROXY_CHANNEL, RFSERVER_ID, msg)

def handle_connectiondown(event):
    log.info("Datapath id=%s is down, disabling it...", event.dpid)
    msg = DatapathDown(dp_id=event.dpid)
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
    # If we have a mapping packet, inform RFServer through a PortMap message
    if packet.type == 0x0A0A:
        vm_id, vm_port = struct.unpack("QB", packet.raw[14:])
        msg = PortMap(vm_id=vm_id, vm_port=vm_port,
                      vs_id=event.dpid, vs_port=event.port)
        ipc.send(RFSERVER_RFPROXY_CHANNEL, RFSERVER_ID, msg)
        return

    # If the packet came from RFVS, redirect it to the right switch port
    if event.dpid == RFVS_DPID:
        entry = rftable.get_entry_by_vs_port(event.dpid, event.port)
        if entry is not None and entry.get_status() == RFENTRY_ACTIVE:
            packet_out(event.data, int(entry.dp_id), int(entry.dp_port))
        else:
            log.debug("Unmapped RFVS port")
    # If the packet came from a switch, redirect it to the right RFVS port
    else:
        entry = rftable.get_entry_by_dp_port(event.dpid, event.port)
        if entry is not None and entry.get_status() == RFENTRY_ACTIVE:
            packet_out(event.data, int(entry.vs_id), int(entry.vs_port))
        else:
            log.debug("Datapath port not associated with a VM port")

def launch ():
    core.openflow.addListenerByName("ConnectionUp", handle_connectionup)
    core.openflow.addListenerByName("ConnectionDown", handle_connectiondown)
    core.openflow.addListenerByName("PacketIn", handle_packetin)
    ipc.listen(RFSERVER_RFPROXY_CHANNEL, RFProtocolFactory(), RFProcessor(), False)
    log.info("RFProxy running.")
