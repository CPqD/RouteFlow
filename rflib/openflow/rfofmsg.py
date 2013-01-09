import binascii
import logging
from socket import inet_aton

from pox.openflow.libopenflow_01 import *
from pox.lib.addresses import *
from pox.core import core

from rflib.defs import *
from rflib.types.Match import *
from rflib.types.Action import *
from rflib.types.Option import *

log = core.getLogger("rfproxy")

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

def create_flow_mod(routemod):
    ofm = ofp_flow_mod()

    mod = routemod.get_mod()

    if mod == RMT_ADD:
        ofm.command = OFPFC_ADD
    elif mod == RMT_DELETE:
        ofm.command = OFPFC_DELETE_STRICT
    else:
        log.error("Unrecognised RouteMod Type (type: %s)" % (mod))
        return None

    for match in routemod.get_matches():
        match = Match.from_dict(match)
        if match._type == RFMT_IPV4:
            ofm_match_dl(ofm, OFPFW_DL_TYPE, ETHERTYPE_IP)
            # OpenFlow 1.0 doesn't support arbitrary netmasks, so
            # TODO calculate netmask CIDR prefix better than below...
            nmprefix = bin(int(binascii.b2a_hex(inet_aton(match.get_value()[1])), 16))[2:].find('0')
            nmprefix = 32 if nmprefix == -1 else nmprefix
            ofm_match_nw(ofm, OFPFW_NW_DST_MASK, 0, 0, 0, str(match.get_value()[0]))
            ofm.match.wildcards &= ~parseCIDR(str(match.get_value()[0]) + "/" + str(nmprefix))[1]
            ofm.match.set_nw_dst(match.get_value()[0])
        elif match._type == RFMT_ETHERNET:
            ofm_match_dl(ofm, OFPFW_DL_DST, match.get_value())
        elif match._type == RFMT_ETHERTYPE:
            ofm_match_dl(ofm, OFPFW_DL_TYPE, match.get_value())
        elif match._type == RFMT_NW_PROTO:
            ofm_match_nw(ofm, OFPFW_NW_PROTO, match.get_value(), 0, 0, 0)
        elif match._type == RFMT_TP_SRC:
            ofm_match_tp(ofm, OFPFW_TP_SRC, match.get_value(), 0)
        elif match._type == RFMT_TP_DST:
            ofm_match_tp(ofm, OFPFW_TP_DST, 0, match.get_value())
        elif match.optional():
            log.debug("Dropping unsupported Match (type: %s)" % option._type)
        else:
            log.warning("Failed to serialise Match (type: %s)" % option._type)
            return None


    for action in routemod.get_actions():
        action = Action.from_dict(action)
        if action._type == RFAT_OUTPUT:
            ofm.actions.append(ofp_action_output(port = action.get_value()))
        elif action._type == RFAT_SET_ETH_SRC:
            ofm.actions.append(ofp_action_dl_addr(type=OFPAT_SET_DL_SRC, dl_addr=EthAddr(action.get_value())))
        elif action._type == RFAT_SET_ETH_DST:
            ofm.actions.append(ofp_action_dl_addr(type=OFPAT_SET_DL_DST, dl_addr=EthAddr(action.get_value())))
        elif action.optional():
            log.debug("Dropping unsupported Action (type: %s)" % option._type)
        else:
            log.warning("Failed to serialise Action (type: %s)" % option._type)
            return None

    for option in routemod.get_options():
        option = Option.from_dict(option)
        if option._type == RFOT_PRIORITY:
            ofm.priority = option.get_value()
        elif option._type == RFOT_IDLE_TIMEOUT:
            ofm.idle_timeout = option.get_value()
        elif option._type == RFOT_HARD_TIMEOUT:
            ofm.hard_timeout = option.get_value()
        elif option.optional():
            log.debug("Dropping unsupported Option (type: %s)" % option._type)
        else:
            log.warning("Failed to serialise Option (type: %s)" % option._type)
            return None


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
