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

import core
import array
from socket import htons

import nox.lib.openflow as openflow

from   nox.coreapps.pyrt.pycomponent import UINT32_MAX
from   nox.coreapps.pyrt.pycomponent import CONTINUE 

from   nox.lib.packet.packet_exceptions import IncompletePacket

from nox.lib.netinet.netinet import ipaddr, create_ipaddr, ethernetaddr, create_bin_eaddr, create_eaddr, c_ntohl, c_htonl

from   nox.lib.packet.ethernet import ethernet 
from   nox.lib.packet.vlan import vlan 
from   nox.lib.packet.ipv4 import ipv4 
from   nox.lib.packet.udp import udp 
from   nox.lib.packet.tcp import tcp 
from   nox.lib.packet.icmp import icmp 

from twisted.python import log

###############################################################################
# HELPER FUNCTIONS (NOT REQUIRING ACCESS TO THE CORE)
###############################################################################

def __dladdr_check__(addr):
    if isinstance(addr, basestring) and not (':' in addr):
        if len(addr) != ethernetaddr.LEN:
            return None
        addr = create_bin_eaddr(addr)
    elif isinstance(addr, basestring) or ((isinstance(addr, long) or isinstance(addr, int)) and addr >= 0):
        addr = create_eaddr(addr)
    elif not isinstance(addr, ethernetaddr):
        return None

    if addr == None:
        return None

    return addr.hb_long()

def __nwaddr_check__(addr):
    # should check if defined constant here

    if ((isinstance(addr, int) or isinstance(addr, long)) and addr >= 0 and addr <= 0xffffffff) \
            or isinstance(addr, basestring):
        addr = create_ipaddr(addr)
    elif not  isinstance(addr, ipaddr):
        return None

    if addr == None:
        return None

    return c_ntohl(addr.addr)

def convert_to_eaddr(val):
    if isinstance(val, ethernetaddr):
        return val
    if isinstance(val, array.array):
        val = val.tostring()
    if isinstance(val, str) and not (':' in val):
        if len(val) < ethernetaddr.LEN:
            return None
        return create_bin_eaddr(val)
    elif isinstance(val, str) or isinstance(val, int) or isinstance(val, long):
        return create_eaddr(val)
    return None

def convert_to_ipaddr(val):
    if isinstance(val, ipaddr):
        return val.addr
    if isinstance(val, str) or isinstance(val, int) or isinstance(val, long):
        a = create_ipaddr(val)
        if a != None:
            return a.addr
    return None

def gen_packet_in_cb(handler):
    def f(event):
        if event.reason == openflow.OFPR_NO_MATCH:
            reason = openflow.OFPR_NO_MATCH
        elif event.reason == openflow.OFPR_ACTION:
            reason = openflow.OFPR_ACTION
        else:
            print 'packet_in reason type %u unknown...just passing along' % event.reason
            reason = event.reason

        if event.buffer_id == UINT32_MAX:
            buffer_id = None
        else:
            buffer_id = event.buffer_id

        try:
            packet = ethernet(array.array('B', event.buf))
        except IncompletePacket, e:
            log.err('Incomplete Ethernet header',system='pyapi')
        
        ret = handler(event.datapath_id, event.in_port, reason,
                      event.total_len, buffer_id, packet)
        if ret == None:
            return CONTINUE
        return ret
    return f

def gen_dp_join_cb(handler):
    def f(event):
        attrs = {}
        attrs[core.N_BUFFERS] = event.n_buffers
        attrs[core.N_TABLES] = event.n_tables
        attrs[core.CAPABILITES] = event.capabilities
        attrs[core.ACTIONS] = event.actions
        attrs[core.PORTS] = event.ports
        for i in range(0, len(attrs[core.PORTS])):
            port = attrs[core.PORTS][i]
            config = port['config']
            state = port['state']
            attrs[core.PORTS][i]['link']    = (state & openflow.OFPPS_LINK_DOWN) == 0
            attrs[core.PORTS][i]['enabled'] = (config & openflow.OFPPC_PORT_DOWN) == 0
            attrs[core.PORTS][i]['flood']   = (config & openflow.OFPPC_NO_FLOOD)  == 0

        ret = f.cb(event.datapath_id, attrs)
        if ret == None:
            return CONTINUE
        return ret
    f.cb = handler
    return f

def gen_dp_leave_cb(handler):
    def f(event):
        ret = f.cb(event.datapath_id)
        if ret == None:
            return CONTINUE
        return ret
    f.cb = handler
    return f

def gen_ds_in_cb(handler):
    def f(event):
        stats = {}
        stats['mfr_desc'] = event.mfr_desc
        stats['hw_desc'] = event.hw_desc
        stats['sw_desc'] = event.sw_desc
        stats['dp_desc'] = event.dp_desc
        stats['serial_num'] = event.serial_num
        ret = f.cb(event.datapath_id, stats)
        if ret == None:
            return CONTINUE
        return ret
    f.cb = handler
    return f

def gen_as_in_cb(handler):
    def f(event):
        stats = {}
        stats['packet_count'] = event.packet_count
        stats['byte_count'] = event.byte_count
        stats['flow_count'] = event.flow_count
        ret = f.cb(event.datapath_id, stats)
        if ret == None:
            return CONTINUE
        return ret
    f.cb = handler
    return f

def gen_ps_in_cb(handler):
    def f(event):
        ports = event.ports 
        ret = f.cb(event.datapath_id, ports)
        if ret == None:
            return CONTINUE
        return ret
    f.cb = handler
    return f

def gen_ts_in_cb(handler):
    def f(event):
        tables = event.tables 
        ret = f.cb(event.datapath_id, tables)
        if ret == None:
            return CONTINUE
        return ret
    f.cb = handler
    return f

def gen_port_status_cb(handler):
    def f(event):
        if event.reason == openflow.OFPPR_ADD:
            reason = openflow.OFPPR_ADD
        elif event.reason == openflow.OFPPR_DELETE:
            reason = openflow.OFPPR_DELETE
        elif event.reason == openflow.OFPPR_MODIFY:
            reason = openflow.OFPPR_MODIFY
        else:
            print 'port_status reason type %u unknown...just passing along' % event.reason
            reason = event.reason

        config = event.port['config']
        state = event.port['state']
        event.port['link']    = (state & openflow.OFPPS_LINK_DOWN) == 0
        event.port['enabled'] = (config & openflow.OFPPC_PORT_DOWN) == 0
        event.port['flood']   = (config & openflow.OFPPC_NO_FLOOD)  == 0

        ret = f.cb(event.datapath_id, reason, event.port)
        if ret == None:
            return CONTINUE
        return ret
    f.cb = handler
    return f

def gen_switch_mgr_join_cb(handler):
    def f(event):
        ret = f.cb(event.mgmt_id)
        if ret == None:
            return CONTINUE
        return ret
    f.cb = handler
    return f

def gen_switch_mgr_leave_cb(handler):
    def f(event):
        ret = f.cb(event.mgmt_id)
        if ret == None:
            return CONTINUE
        return ret
    f.cb = handler
    return f

def set_match(attrs):
    m = openflow.ofp_match()
    wildcards = 0
    num_entries = 0

    if attrs.has_key(core.IN_PORT):
        m.in_port = htons(attrs[core.IN_PORT])
        num_entries += 1
    else:
        wildcards = wildcards | openflow.OFPFW_IN_PORT

    if attrs.has_key(core.DL_VLAN):
        m.dl_vlan = htons(attrs[core.DL_VLAN])
        num_entries += 1
    else:
        wildcards = wildcards | openflow.OFPFW_DL_VLAN

    if attrs.has_key(core.DL_VLAN_PCP):
        m.dl_vlan_pcp = attrs[core.DL_VLAN_PCP]
        num_entries += 1
    else:
        wildcards = wildcards | openflow.OFPFW_DL_VLAN_PCP

    if attrs.has_key(core.DL_SRC):
        v = convert_to_eaddr(attrs[core.DL_SRC])
        if v == None:
            print 'invalid ethernet addr'
            return None
        m.set_dl_src(v.octet)
        num_entries += 1
    else:
        wildcards = wildcards | openflow.OFPFW_DL_SRC

    if attrs.has_key(core.DL_DST):
        v = convert_to_eaddr(attrs[core.DL_DST])
        if v == None:
            print 'invalid ethernet addr'
            return None
        m.set_dl_dst(v.octet)
        num_entries += 1
    else:
        wildcards = wildcards | openflow.OFPFW_DL_DST

    if attrs.has_key(core.DL_TYPE):
        m.dl_type = htons(attrs[core.DL_TYPE])
        num_entries += 1
    else:
        wildcards = wildcards | openflow.OFPFW_DL_TYPE

    if attrs.has_key(core.NW_SRC):
        v = convert_to_ipaddr(attrs[core.NW_SRC])
        if v == None:
            print 'invalid ip addr'
            return None
        m.nw_src = v
        num_entries += 1

        if attrs.has_key(core.NW_SRC_N_WILD):
            n_wild = attrs[core.NW_SRC_N_WILD]
            if n_wild > 31:
                wildcards |= openflow.OFPFW_NW_SRC_MASK
            elif n_wild >= 0:
                wildcards |= n_wild << openflow.OFPFW_NW_SRC_SHIFT
            else:
                print 'invalid nw_src wildcard bit count', n_wild
                return None
            num_entries += 1
    else:
        wildcards = wildcards | openflow.OFPFW_NW_SRC_MASK

    if attrs.has_key(core.NW_DST):
        v = convert_to_ipaddr(attrs[core.NW_DST])
        if v == None:
            print 'invalid ip addr'
            return None            
        m.nw_dst = v
        num_entries += 1

        if attrs.has_key(core.NW_DST_N_WILD):
            n_wild = attrs[core.NW_DST_N_WILD]
            if n_wild > 31:
                wildcards |= openflow.OFPFW_NW_DST_MASK
            elif n_wild >= 0:
                wildcards |= n_wild << openflow.OFPFW_NW_DST_SHIFT
            else:
                print 'invalid nw_dst wildcard bit count', n_wild
                return None
            num_entries += 1
    else:
        wildcards = wildcards | openflow.OFPFW_NW_DST_MASK

    if attrs.has_key(core.NW_PROTO):
        m.nw_proto = attrs[core.NW_PROTO]
        num_entries += 1
    else:
        wildcards = wildcards | openflow.OFPFW_NW_PROTO

    if attrs.has_key(core.NW_TOS):
        m.nw_tos = attrs[core.NW_TOS]
        num_entries += 1
    else:
        wildcards = wildcards | openflow.OFPFW_NW_TOS

    if attrs.has_key(core.TP_SRC):
        m.tp_src = htons(attrs[core.TP_SRC])
        num_entries += 1
    else:
        wildcards = wildcards | openflow.OFPFW_TP_SRC

    if attrs.has_key(core.TP_DST):
        m.tp_dst = htons(attrs[core.TP_DST])
        num_entries += 1
    else:
        wildcards = wildcards | openflow.OFPFW_TP_DST

    if num_entries != len(attrs.keys()):
        print 'undefined flow attribute type in attrs', attrs
        return None

    m.wildcards = c_htonl(wildcards)
    return m

def extract_flow(ethernet):
    """
    Extracts and returns flow attributes from the given 'ethernet' packet.
    The caller is responsible for setting IN_PORT itself.
    """
    attrs = {}
    attrs[core.DL_SRC] = ethernet.src
    attrs[core.DL_DST] = ethernet.dst
    attrs[core.DL_TYPE] = ethernet.type
    p = ethernet.next

    if isinstance(p, vlan):
        attrs[core.DL_VLAN] = p.id
        attrs[core.DL_VLAN_PCP] = p.pcp
        p = p.next
    else:
        attrs[core.DL_VLAN] = 0xffff # XXX should be written OFP_VLAN_NONE
        attrs[core.DL_VLAN_PCP] = 0

    if isinstance(p, ipv4):
        attrs[core.NW_SRC] = p.srcip
        attrs[core.NW_DST] = p.dstip
        attrs[core.NW_PROTO] = p.protocol
        p = p.next

        if isinstance(p, udp) or isinstance(p, tcp):
            attrs[core.TP_SRC] = p.srcport
            attrs[core.TP_DST] = p.dstport
        else:
            if isinstance(p, icmp):
                attrs[core.TP_SRC] = p.type
                attrs[core.TP_DST] = p.code
            else:
                attrs[core.TP_SRC] = 0
                attrs[core.TP_DST] = 0
    else:
        attrs[core.NW_SRC] = 0
        attrs[core.NW_DST] = 0
        attrs[core.NW_PROTO] = 0
        attrs[core.TP_SRC] = 0
        attrs[core.TP_DST] = 0
    return attrs
