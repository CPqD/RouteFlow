from pox.openflow.libopenflow_01 import *
from pox.lib.addresses import *

from rflib.defs import *

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

    if operation == DC_RIPV2:
        ofm_match_dl(ofm, OFPFW_DL_TYPE, ETHERTYPE_IP)
        ofm_match_nw(ofm, (OFPFW_NW_PROTO | OFPFW_NW_DST_MASK), IPPROTO_UDP, 0, 0, IPAddr("224.0.0.9"))
    elif operation == DC_OSPF:
        ofm_match_dl(ofm, OFPFW_DL_TYPE, ETHERTYPE_IP)
        ofm_match_nw(ofm, OFPFW_NW_PROTO, IPPROTO_OSPF, 0, 0, 0)
    elif operation == DC_ARP:
        ofm_match_dl(ofm, OFPFW_DL_TYPE, ETHERTYPE_ARP)
    elif operation == DC_ICMP:
        ofm_match_dl(ofm, OFPFW_DL_TYPE, ETHERTYPE_IP)
        ofm_match_nw(ofm, OFPFW_NW_PROTO, IPPROTO_ICMP, 0, 0, 0)
    elif operation == DC_BGP_PASSIVE:
        ofm_match_dl(ofm, OFPFW_DL_TYPE, ETHERTYPE_IP)
        ofm_match_nw(ofm, OFPFW_NW_PROTO, IPPROTO_TCP, 0, 0, 0)
        ofm_match_tp(ofm, OFPFW_TP_DST, 0, TPORT_BGP)
    elif operation == DC_BGP_ACTIVE:
        ofm_match_dl(ofm, OFPFW_DL_TYPE, ETHERTYPE_IP)
        ofm_match_nw(ofm, OFPFW_NW_PROTO, IPPROTO_TCP, 0, 0, 0)
        ofm_match_tp(ofm, OFPFW_TP_SRC, TPORT_BGP, 0)
    elif operation == DC_LDP_PASSIVE:
        ofm_match_dl(ofm, OFPFW_DL_TYPE, ETHERTYPE_IP);
        ofm_match_nw(ofm, OFPFW_NW_PROTO, IPPROTO_TCP, 0, 0, 0);
        ofm_match_tp(ofm, OFPFW_TP_DST, 0, TPORT_LDP);
    elif operation == DC_LDP_ACTIVE:
        ofm_match_dl(ofm, OFPFW_DL_TYPE, ETHERTYPE_IP);
        ofm_match_nw(ofm, OFPFW_NW_PROTO, IPPROTO_TCP, 0, 0, 0);
        ofm_match_tp(ofm, OFPFW_TP_SRC, TPORT_LDP, 0);
    elif operation == DC_VM_INFO:
        ofm_match_dl(ofm, OFPFW_DL_TYPE, RF_ETH_PROTO)
    elif operation == DC_DROP_ALL:
        ofm.priority = 1;

    if operation == DC_CLEAR_FLOW_TABLE:
        ofm.command = OFPFC_DELETE
        ofm.priority = 0
    else:
        ofm.command = OFPFC_ADD
        ofm.idle_timeout = OFP_FLOW_PERMANENT
        ofm.hard_timeout = OFP_FLOW_PERMANENT
        ofm.out_port = OFPP_NONE
        ofm.actions.append(ofp_action_output(port = OFPP_CONTROLLER, max_len = RF_MAX_PACKET_SIZE))

    return ofm

def create_flow_install_msg(ip, mask, srcMac, dstMac, dstPort):
    ofm = ofp_flow_mod()

    ofm_match_dl(ofm, OFPFW_DL_TYPE, ETHERTYPE_IP)
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

    ofm_match_dl(ofm, OFPFW_DL_TYPE, ETHERTYPE_IP)
    if (MATCH_L2):
	    ofm_match_dl(ofm, OFPFW_DL_DST, srcMac)

    ofm.match.set_nw_dst(ip)
    ofm.priority = OFP_DEFAULT_PRIORITY + mask
    ofm.command = OFPFC_DELETE_STRICT
    
    return ofm


def create_temporary_flow_msg(ip, mask, srcMac):
    ofm = ofp_flow_mod()

    ofm_match_dl(ofm, OFPFW_DL_TYPE, ETHERTYPE_IP)
    if (MATCH_L2):
	    ofm_match_dl(ofm, OFPFW_DL_DST, srcMac)

    ofm.match.set_nw_dst(ip)
    ofm.priority = OFP_DEFAULT_PRIORITY + mask

    ofm.command = OFPFC_ADD
    ofm.idle_timeout = 60
    ofm.out_port = OFPP_NONE
    ofm.actions.append(ofp_action_output(port=0))
    
    return ofm
