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

def ofm_match_port(ofm, match, value):
    ofm.match.wildcards &= ~match;

    if match & OFPFW_IN_PORT:
        ofm.match.in_port = value

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
        elif match._type == RFMT_IN_PORT:
            ofm_match_port(ofm, OFPFW_IN_PORT, match.get_value())
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
