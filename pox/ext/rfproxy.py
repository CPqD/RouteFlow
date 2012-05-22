from pox.core import core
from pox.openflow.libopenflow_01 import *
from pox.lib.addresses import *
import IPC
import MongoIPC
import struct
import pymongo as mongo
from socket import htonl
from defs import *

ipc = MongoIPC.MongoIPCMessageService(MONGO_ADDRESS, MONGO_DB_NAME, RFPROXY_ID)

conn = mongo.Connection()
rftable = conn[MONGO_DB_NAME][RF_TABLE_NAME]
        
        
class DatapathJoin(MongoIPC.MongoIPCMessage):
    def __init__(self, **kwargs):
        MongoIPC.MongoIPCMessage.__init__(self, DATAPATH_JOIN, **kwargs)

    def str(self):
        string = "DatapathJoin\n"
        string += "  " + MongoIPC.MongoIPCMessage.str(self).replace("\n", "\n  ")
        return string

class DatapathConfig(MongoIPC.MongoIPCMessage):
    def __init__(self, **kwargs):
        MongoIPC.MongoIPCMessage.__init__(self, DATAPATH_CONFIG, **kwargs)

    def str(self):
        string = "DatapathConfig\n"
        string += "  " + MongoIPC.MongoIPCMessage.str(self).replace("\n", "\n  ")
        return string

class FlowMod(MongoIPC.MongoIPCMessage):
    def __init__(self, **kwargs):
        MongoIPC.MongoIPCMessage.__init__(self, FLOW_MOD, **kwargs)

    def str(self):
        string = "FlowMod\n"
        string += "  " + MongoIPC.MongoIPCMessage.str(self).replace("\n", "\n  ")
        return string

class VMMap(MongoIPC.MongoIPCMessage):
    def __init__(self, **kwargs):
        MongoIPC.MongoIPCMessage.__init__(self, VM_MAP, **kwargs)

    def str(self):
        string = "VMMap\n"
        string += "  " + MongoIPC.MongoIPCMessage.str(self).replace("\n", "\n  ")
        return string

class RFProcessor:
    def process(self, from_, to, channel, msg):
        topology = core.components['topology']
        type_ = msg.get_type()
        if type_ == DATAPATH_CONFIG:
            dp_id = int(msg["dp_id"])
            operation_id = int(msg["operation_id"])

            ofmsg = create_config_msg(operation_id)
            switch = topology.getEntityByID(dp_id)
            switch.send(ofmsg)
        elif type_ == FLOW_MOD:
            print "Installing flow mod to datapath %s" % msg["dp_id"]
            dp_id = int(msg["dp_id"])
            
            netmask = str(msg["netmask"])
            # TODO: fix this. It was just to make it work...
            netmask = netmask.count("255")*8
            
            address = str(msg["address"]) + "/" + str(netmask)
            
            src_hwaddress = str(msg["src_hwaddress"])
            dst_hwaddress = str(msg["dst_hwaddress"])
            dst_port = int(msg["dst_port"])
            
            ofmsg = create_flow_install_msg(address, netmask, src_hwaddress, dst_hwaddress, dst_port)
            
            switch = topology.getEntityByID(dp_id)
            switch.send(ofmsg)
            
class RFFactory(IPC.IPCMessageFactory):
    def build_for_type(self, type_):
        if type_ == DATAPATH_CONFIG:
            return DatapathConfig()
        if type_ == FLOW_MOD:
            return FlowMod()
        
def ofm_match_dl(ofm, match, value):
    ofm.match.wildcards &= ~match;

    # Ethernet frame type
    if match & OFPFW_DL_TYPE:
        ofm.match.dl_type = value
    # Ethernet source address
    if match & OFPFW_DL_SRC:
        ofm.match.dl_src = EthAddr(value)
    # Ethernet destination address
    if match & OFPFW_DL_DST:
        ofm.match.dl_dst = EthAddr(value)

def ofm_match_nw(ofm, match, proto, tos, src, dst):
    ofm.match.wildcards &= ~match

    if match & OFPFW_NW_PROTO: # IP protocol.
        ofm.match.nw_proto = proto
    if match & OFPFW_NW_TOS: # IP ToS (DSCP field, 6 bits).
        ofm.match.nw_tos = tos
    if (match & OFPFW_NW_SRC_MASK) > 0: # IP source address.
        ofm.match.nw_src = IPAddr(src)
    if (match & OFPFW_NW_DST_MASK) > 0: # IP destination address.
        ofm.match.nw_dst = IPAddr(dst)

def ofm_match_tp(ofm, match, src, dst):
    ofm.match.wildcards &= ~match

    if match & OFPFW_TP_SRC: # TCP/UDP source port.
        ofm.match.tp_src = src
    if match & OFPFW_TP_DST: # TCP/UDP destination port.
        ofm.match.tp_dst = dst

def create_config_msg(operation):
    ofm = ofp_flow_mod()

    if operation == DATAPATH_CONFIG_OPERATION.DC_RIPV2:
        ofm_match_dl(ofm, OFPFW_DL_TYPE, 0x0800)
        ofm_match_nw(ofm, (OFPFW_NW_PROTO | OFPFW_NW_DST_MASK), 0x11, 0, 0, IPAddr("224.0.0.9"))
    elif operation == DATAPATH_CONFIG_OPERATION.DC_OSPF:
        ofm_match_dl(ofm, OFPFW_DL_TYPE, 0x0800)
        ofm_match_nw(ofm, OFPFW_NW_PROTO, 0x59, 0, 0, 0)
    elif operation == DATAPATH_CONFIG_OPERATION.DC_ARP:
        ofm_match_dl(ofm, OFPFW_DL_TYPE, 0x0806)
    elif operation == DATAPATH_CONFIG_OPERATION.DC_ICMP:
        ofm_match_dl(ofm, OFPFW_DL_TYPE, 0x0800)
        ofm_match_nw(ofm, OFPFW_NW_PROTO, 0x01, 0, 0, 0)
    elif operation == DATAPATH_CONFIG_OPERATION.DC_BGP:
        ofm_match_dl(ofm, OFPFW_DL_TYPE, 0x0800)
        ofm_match_nw(ofm, OFPFW_NW_PROTO, 0x06, 0, 0, 0)
        ofm_match_tp(ofm, OFPFW_TP_DST, 0, 0x00B3)
    elif operation == DATAPATH_CONFIG_OPERATION.DC_VM_INFO:
        ofm_match_dl(ofm, OFPFW_DL_TYPE, 0x0A0A)
    elif operation == DATAPATH_CONFIG_OPERATION.DC_DROP_ALL:
        ofm.priority = 1;

    if operation == DATAPATH_CONFIG_OPERATION.DC_CLEAR_FLOW_TABLE:
        ofm.command = OFPFC_DELETE
        ofm.priority = 0
    else:
        ofm.command = OFPFC_ADD
        ofm.idle_timeout = OFP_FLOW_PERMANENT
        ofm.hard_timeout = OFP_FLOW_PERMANENT
        ofm.out_port = OFPP_NONE
        ofm.actions.append(ofp_action_output(port = OFPP_CONTROLLER))

    return ofm

def create_flow_install_msg(ip, mask, srcMac, dstMac, dstPort):
    ofm = ofp_flow_mod()

    ofm_match_dl(ofm, OFPFW_DL_TYPE, 0x0800)
    if (MATCH_L2):
	    ofm_match_dl(ofm, OFPFW_DL_DST, srcMac)
	    
    ofm.match.set_nw_dst(ip)
    
    ofm.priority = OFP_DEFAULT_PRIORITY + mask
    ofm.command = OFPFC_ADD
    if (mask == 32):
        ofm.idle_timeout = 300
    else:
        ofm.idle_timeout = OFP_FLOW_PERMANENT
    ofm.hard_timeout = OFP_FLOW_PERMANENT
    ofm.out_port = OFPP_NONE

    ofm.actions.append(ofp_action_dl_addr(type=OFPAT_SET_DL_SRC, dl_addr=EthAddr(srcMac)))
    ofm.actions.append(ofp_action_dl_addr(type=OFPAT_SET_DL_DST, dl_addr=EthAddr(dstMac)))
    ofm.actions.append(ofp_action_output(port=dstPort))

    return ofm


def create_flow_remove_msg(ip, mask, srcMac):
    ofm = ofp_flow_mod()

    ofm_match_dl(ofm, OFPFW_DL_TYPE , 0x0800)
    if (MATCH_L2):
	    ofm_match_dl(ofm, OFPFW_DL_DST, srcMac)

    ofm_match_nw(ofm, ((31 + mask) << OFPFW_NW_DST_SHIFT), 0, 0, 0, ip)
    ofm.priority = OFP_DEFAULT_PRIORITY + mask
    ofm.command = OFPFC_DELETE_STRICT
    return ofm


def create_temporary_flow_msg(ip, mask, srcMac):
    ofm = ofp_flow_mod()

    ofm_match_dl(ofm, OFPFW_DL_TYPE , 0x0800)
    if (MATCH_L2):
	    ofm_match_dl(ofm, OFPFW_DL_DST, srcMac)

    ofm_match_nw(ofm, ((31 + mask) << OFPFW_NW_DST_SHIFT), 0, 0, 0, ip)

    ofm.priority = OFP_DEFAULT_PRIORITY + mask

    ofm.command = OFPFC_ADD
    ofm.idle_timeout = 60
    ofm.out_port = OFPP_NONE

    ofm.actions.append(ofp_action_output(port=0))
    return ofm

log = core.getLogger()

def dpid_to_switchname(dpid):
    dpid = hex(dpid)[2:][:-1]
    switchname = ""
    for char in [dpid[i:i+2] for i in xrange(0, len(dpid), 2)]:
        switchname += chr(int(char, 16))
    return switchname

def handle_connectionup(event):
    print "DP is up, installing config flows...", dpid_to_switchname(event.dpid)
    topology = core.components['topology']
    msg = DatapathJoin(dp_id=str(event.dpid), 
                       n_ports=str((len(topology.getEntityByID(event.dpid).ports) - 1)), 
                       is_rfvs=(event.dpid == RFVS_DPID))
    ipc.send(RFSERVER_RFPROXY_CHANNEL, RFSERVER_ID, msg)

def packet_out(data, dp_id, outport):
    try:
        msg = ofp_packet_out()
        msg.actions.append(ofp_action_output(port=outport))
        msg.data = data
        msg.in_port = OFPP_NONE
        topology = core.components['topology']
        switch = topology.getEntityByID(dp_id)
        switch.send(msg)
    except Exception as e:
        print e
    
def handle_packetin(event):
    try:
        packet = event.parsed
        
        if packet.type == ethernet.LLDP_TYPE:
            return

        if packet.type == 0x0A0A:
            vm_id, vm_port = struct.unpack("QB", packet.raw[14:])
            dp_id = event.dpid
            in_port = event.port
            msg = VMMap(vm_id=str(vm_id), 
                        vm_port=str(vm_port), 
                        vs_id=str(dp_id), 
                        vs_port=str(in_port))
            ipc.send(RFSERVER_RFPROXY_CHANNEL, RFSERVER_ID, msg)
            return
        
        results = rftable.find({VS_ID: str(event.dpid)})
        if results.count() == 0 or results[0][VM_ID] == "":
            results = rftable.find({VS_ID: {"$ne": ""}, DP_ID: str(event.dpid), DP_PORT: str(event.port)})
            if results.count() == 0:
                # log.info("Datapath not associated with a VM")
                return
            else:
                vs_id = int(results[0][VS_ID])
                vs_port = int(results[0][VS_PORT])
                packet_out(event.data, vs_id, vs_port)
        else:
            results = rftable.find({VS_PORT: str(event.port)})
            if results.count() == 0 or results[0][DP_ID] == "":
                # log.info("Datapath not associated with a VM")
                return
            else:
                dp_id = int(results[0][DP_ID])
                dp_port = int(results[0][DP_PORT])
                packet_out(event.data, dp_id, dp_port)
    except Exception as e:
        print e
                
def launch ():
    core.openflow.addListenerByName("ConnectionUp", handle_connectionup)
    core.openflow.addListenerByName("PacketIn", handle_packetin)
    ipc.listen(RFSERVER_RFPROXY_CHANNEL, RFFactory(), RFProcessor(), False)
    log.info("RFProxy running.")
