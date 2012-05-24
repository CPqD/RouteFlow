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
import logging
import array
import struct
import types
from socket import htons, htonl
import nox.lib.openflow as openflow

from nox.coreapps.pyrt.pycomponent import * 
from util import *
from nox.lib.netinet.netinet import Packet_expr 
from nox.lib.netinet.netinet import create_eaddr 
from nox.lib.packet import *

lg = logging.getLogger('core')

IN_PORT    = "in_port"
AP_SRC     = "ap_src"
AP_DST     = "ap_dst"
DL_SRC     = "dl_src"
DL_DST     = "dl_dst"
DL_VLAN    = "dl_vlan"
DL_VLAN_PCP = "dl_vlan_pcp"
DL_TYPE    = "dl_type"
NW_SRC     = "nw_src"
NW_SRC_N_WILD = "nw_src_n_wild"
NW_DST     = "nw_dst"
NW_DST_N_WILD = "nw_dst_n_wild"
NW_PROTO   = "nw_proto"
NW_TOS     = "nw_tos"
TP_SRC     = "tp_src"
TP_DST     = "tp_dst"
GROUP_SRC  = "group_src"
GROUP_DST  = "group_dst"

N_TABLES    = 'n_tables'
N_BUFFERS   = 'n_bufs'
CAPABILITES = 'caps'
ACTIONS     = 'actions'
PORTS       = 'ports'

PORT_NO     = 'port_no'
SPEED       = 'speed'
CONFIG      = 'config'
STATE       = 'state'
CURR        = 'curr'
ADVERTISED  = 'advertised'
SUPPORTED   = 'supported'
PEER        = 'peer'
HW_ADDR     = 'hw_addr'

################################################################################
# API NOTES:
#
# Automatically returns CONTINUE for handlers that do not 
# return a value (handlers are supposed to return a Disposition)
#
# All values should be passed in host byte order.  The API changes
# values to network byte order based on knowledge of field.  (NW_SRC
# --> 32 bit val, TP_SRC --> 16 bit value, etc.).  Other than
# DL_SRC/DST and NW_SRC/DST fields, packet header fields should be
# passed as integers.  DL_SRC, DL_DST fields should be passed in as
# either vigil.netinet.ethernetaddr objects.  They can however also be
# passed in any other type that an ethernetaddr constructor has be
# defined for.  NW_SRC/NW_DST fields meanwhile can be passed either
# ints, ip strings, or vigil.netinet.ipaddr objects.
###########################################################################

class Component: 
    """Abstract class to inherited by all Python components.
    \ingroup noxapi
    """
    def __init__(self, ctxt):
        self.ctxt = ctxt
        self.component_names = None 

    def configure(self, config):
        """Configure the component.
        Once configured, the component has parsed its configuration and
        resolve any references to other components it may have.
        """
        pass

    def install(self):
        """Install the component. 
        Once installed, the component runs and is usable by other
        components.
        """
        pass

    def getInterface(self):
        """Return the interface (class) component provides.  The default
        implementation returns the class itself."""
        return self.__class__

    def resolve(self, interface):
        return self.ctxt.resolve(str(interface))    

    # Interface to allow components to check at runtime without having
    # to import them (which will cause linking errors)
    def is_component_loaded(self, name):    
        if not self.component_names:
            self.component_names = []
            for component in self.ctxt.get_kernel().get_all():
                self.component_names.append(component.get_name())
        return name in self.component_names        
                
    def register_event(self, event_name):
        return self.ctxt.register_event(event_name)

    def register_python_event(self, event_name):
        return self.ctxt.register_python_event(event_name)

    def register_handler(self, event_name, handler):
        return self.ctxt.register_handler(event_name, handler)

    def post_timer(self, event):
        return self.ctxt.post_timer(event)

    def post(self, event):
        # if event is a swigobject, make sure that it doesn't try
        # to handle memory deletion
        if hasattr(event, 'thisown'):
            event.thisown = 0 # just in case
        return self.ctxt.post(event)

    def make_action_array(self, actions):
        action_str = ""
        
        for action in actions:
            if action[0] == openflow.OFPAT_OUTPUT \
                    and isinstance(action[1],list) \
                    and len(action[1]) == 2:
                a = struct.pack("!HHHH", action[0], 8,
                                action[1][1], action[1][0])
            elif action[0] == openflow.OFPAT_SET_VLAN_VID:
                a = struct.pack("!HHHH", action[0], 8, action[1], 0)
            elif action[0] == openflow.OFPAT_SET_VLAN_PCP:
                a = struct.pack("!HHBBH", action[0], 8, action[1], 0, 0)
            elif action[0] == openflow.OFPAT_STRIP_VLAN:
                a = struct.pack("!HHI", action[0], 8, 0)
            elif action[0] == openflow.OFPAT_SET_DL_SRC \
                    or action[0] == openflow.OFPAT_SET_DL_DST:
                eaddr = convert_to_eaddr(action[1])
                if eaddr == None:
                    print 'invalid ethernet addr'
                    return None
                a = struct.pack("!HH6sHI", action[0], 16, eaddr.binary_str(), 0, 0)
            elif action[0] == openflow.OFPAT_SET_NW_SRC \
                    or action[0] == openflow.OFPAT_SET_NW_DST:
                iaddr = convert_to_ipaddr(action[1])
                if iaddr == None:
                    print 'invalid ip addr'
                    return None
                a = struct.pack("HHI", htons(action[0]), htons(8), htonl(ipaddr(iaddr).addr))
            elif action[0] == openflow.OFPAT_SET_TP_SRC \
                    or action[0] == openflow.OFPAT_SET_TP_DST:
                a = struct.pack("!HHHH", action[0], 8, action[1], 0)
            else:
                print 'invalid action type', action[0]
                return None

            action_str = action_str + a

        return action_str

    def send_port_mod(self, dpid, portno, hwaddr, mask, config):    
        try:
            addr = create_eaddr(str(hwaddr)) 
            return self.ctxt.send_port_mod(dpid, portno, addr, mask, config)
        except Exception, e:    
            print e
            #lg.error("unable to send port mod"+str(e))

    def send_switch_command(self, dpid, command, arg_list):
        return self.ctxt.send_switch_command(dpid, command, ",".join(arg_list))

    def switch_reset(self, dpid):
        return self.ctxt.switch_reset(dpid)

    def switch_update(self, dpid):
        return self.ctxt.switch_update(dpid)
            

    def send_openflow_packet(self, dp_id, packet, actions, 
                             inport=openflow.OFPP_CONTROLLER):
        """
        sends an openflow packet to a datapath

        dp_id - datapath to send packet to
        packet - data to put in openflow packet
        actions - list of actions or dp port to send out of
        inport - dp port to mark as source (defaults to Controller port)
        """
        if type(packet) == type(array.array('B')):
            packet = packet.tostring()

        if type(actions) == types.IntType:
            self.ctxt.send_openflow_packet_port(dp_id, packet, actions, inport)
        elif type(actions) == types.ListType:
            oactions = self.make_action_array(actions)
            if oactions == None:
                raise Exception('Bad action')
            self.ctxt.send_openflow_packet_acts(dp_id, packet, oactions, inport)
        else:
            raise Exception('Bad argument')

    def send_openflow_buffer(self, dp_id, buffer_id, actions, 
                             inport=openflow.OFPP_CONTROLLER):
        """
        Tells a datapath to send out a buffer
        
        dp_id - datapath to send packet to
        buffer_id - id of buffer to send out
        actions - list of actions or dp port to send out of
        inport - dp port to mark as source (defaults to Controller port)
        """
        if type(actions) == types.IntType:
            self.ctxt.send_openflow_buffer_port(dp_id, buffer_id, actions,
                                                inport)
        elif type(actions) == types.ListType:
            oactions = self.make_action_array(actions)
            if oactions == None:
                raise Exception('Bad action')
            self.ctxt.send_openflow_buffer_acts(dp_id, buffer_id, oactions,
                                                inport)
        else:
            raise Exception('Bad argument')

    def post_callback(self, t, function):
        from twisted.internet import reactor
        reactor.callLater(t, function)

    def send_flow_command(self, dp_id, command, attrs, 
                          priority=openflow.OFP_DEFAULT_PRIORITY,
                          add_args=None,
                          hard_timeout=openflow.OFP_FLOW_PERMANENT):
        m = set_match(attrs)
        if m == None:
            return False

        if command == openflow.OFPFC_ADD:
            (idle_timeout, actions, buffer_id) = add_args
            oactions = self.make_action_array(actions)
            if oactions == None:
                return False
        else:
            idle_timeout = 0
            oactions = ""
            buffer_id = UINT32_MAX
        
        self.ctxt.send_flow_command(dp_id, command, m, idle_timeout,
                                    hard_timeout, oactions, buffer_id, priority)

        return True

    # Former PyAPI methods

    def send_openflow(self, dp_id, buffer_id, packet, actions,
                      inport=openflow.OFPP_CONTROLLER):
        """
        Sends an openflow packet to a datapath.

        This function is a convenient wrapper for send_openflow_packet
        and send_openflow_buffer for situations where it is unknown in
        advance whether the packet to be sent is buffered.  If
        'buffer_id' is -1, it sends 'packet'; otherwise, it sends the
        buffer represented by 'buffer_id'.

        dp_id - datapath to send packet to
        buffer_id - id of buffer to send out
        packet - data to put in openflow packet
        actions - list of actions or dp port to send out of
        inport - dp port to mark as source (defaults to Controller
                 port)
        """
        if buffer_id != None:
            self.send_openflow_buffer(dp_id, buffer_id, actions, inport)
        else:
            self.send_openflow_packet(dp_id, packet, actions, inport)

    def delete_datapath_flow(self, dp_id, attrs):
        """
        Delete all flow entries matching the passed in (potentially
        wildcarded) flow

        dp_id - datapath to delete the entries from
        attrs - the flow as a dictionary (described above)
        """
        return self.send_flow_command(dp_id, openflow.OFPFC_DELETE, attrs)

    def delete_strict_datapath_flow(self, dp_id, attrs, 
                        priority=openflow.OFP_DEFAULT_PRIORITY):
        """
        Strictly delete the flow entry matching the passed in (potentially
        wildcarded) flow.  i.e. matched flow have exactly the same
        wildcarded fields.
        
        dp_id - datapath to delete the entries from
        attrs - the flow as a dictionary (described above)
        priority - the priority of the entry to be deleted (only meaningful 
                   for entries with wildcards)
        """
        return self.send_flow_command(dp_id, openflow.OFPFC_DELETE_STRICT, 
                                      attrs, priority)

    ###########################################################################
    # The following methods manipulate a flow entry in a datapath.
    # A flow is defined by a dictionary containing 0 or more of the
    # following keys (commented keys have already been defined above):
    # 
    
    # DL_SRC     = "dl_src"
    # DL_DST     = "dl_dst"
    # DL_VLAN    = "dl_vlan"
    # DL_VLAN_PCP = "dl_vlan_pcp"
    # DL_TYPE    = "dl_type"
    # NW_SRC     = "nw_src"
    # NW_DST     = "nw_dst"
    # NW_PROTO   = "nw_proto"
    # TP_SRC     = "tp_src"
    # TP_DST     = "tp_dst"
    #
    # Absent keys are interpretted as wildcards
    ###########################################################################

    def install_datapath_flow(self, dp_id, attrs, idle_timeout, hard_timeout,
                              actions, buffer_id=None, 
                              priority=openflow.OFP_DEFAULT_PRIORITY,
                              inport=None, packet=None):
        """
        Add a flow entry to datapath

        dp_id - datapath to add the entry to

        attrs - the flow as a dictionary (described above)

        idle_timeout - # idle seconds before flow is removed from dp

        hard_timeout - # of seconds before flow is removed from dp

        actions - a list where each entry is a two-element list representing
        an action.  Elem 0 of an action list should be an ofp_action_type
        and elem 1 should be the action argument (if needed). For
        OFPAT_OUTPUT, this should be another two-element list with max_len
        as the first elem, and port_no as the second

        buffer_id - the ID of the buffer to apply the action(s) to as well.
        Defaults to None if the actions should not be applied to a buffer

        priority - when wildcards are present, this value determines the
        order in which rules are matched in the switch (higher values
        take precedence over lower ones)

        packet - If buffer_id is None, then a data packet to which the
        actions should be applied, or None if none.

        inport - When packet is sent, the port on which packet came in as input,
        so that it can be omitted from any OFPP_FLOOD outputs.
        """
        if buffer_id == None:
            buffer_id = UINT32_MAX

        self.send_flow_command(dp_id, openflow.OFPFC_ADD, attrs, priority,
                          (idle_timeout, actions, buffer_id), hard_timeout)
        
        if buffer_id == UINT32_MAX and packet != None:
            for action in actions:
                if action[0] == openflow.OFPAT_OUTPUT:
                    self.send_openflow_packet(dp_id, packet, action[1][1], inport)
                else:
                    raise NotImplementedError

    def register_for_packet_in(self, handler):
        """
        register a handler to be called on every packet_in event
        handler will be called with the following args:
        
        handler(dp_id, inport, ofp_reason, total_frame_len, buffer_id,
        captured_data)
        
        'buffer_id' == None if the datapath does not have a buffer for
        the frame
        """
        self.register_handler(Packet_in_event.static_get_name(),
                              gen_packet_in_cb(handler))

    def register_for_flow_removed(self, handler):
        self.register_handler(Flow_removed_event.static_get_name(),
                              handler)

    def register_for_flow_mod(self, handler):
        self.register_handler(Flow_mod_event.static_get_name(),
                              handler)

    def register_for_bootstrap_complete(self, handler):
        self.register_handler(Bootstrap_complete_event.static_get_name(),
                              handler)

    ################################################################################
    # register a handler to be called on a every switch_features event
    # handler will be called with the following args:
    #
    # handler(dp_id, attrs)
    #
    # attrs is a dictionary with the following keys:


    # the PORTS value is a list of port dictionaries where each dictionary
    # has the keys listed in the register_for_port_status message
    ################################################################################

    def register_for_datapath_join(self, handler):
        self.register_handler(Datapath_join_event.static_get_name(),
                              gen_dp_join_cb(handler))

    ################################################################################
    # register a handler to be called whenever table statistics are
    # returned by a switch.
    #
    # handler will be called with the following args:
    #
    # handler(dp_id, stats)
    #
    # Stats is a dictionary of table stats with the following keys:
    #
    #   "table_id"
    #   "name"
    #   "max_entries"
    #   "active_count"
    #   "lookup_count"
    #   "matched_count"
    #
    # XXX
    #
    # We should get away from using strings here eventually.
    #
    ################################################################################

    def register_for_table_stats_in(self, handler):
        self.register_handler(Table_stats_in_event.static_get_name(),
                              gen_ts_in_cb(handler))

    ################################################################################
    # register a handler to be called whenever port statistics are
    # returned by a switch.
    #
    # handler will be called with the following args:
    #
    # handler(dp_id, stats)
    #
    # Stats is a dictionary of port stats with the following keys:
    #
    #   "port_no"
    #   "rx_packets"
    #   "tx_packets"
    #   "rx_bytes"
    #   "tx_bytes"
    #   "rx_dropped"
    #   "tx_dropped"
    #   "rx_errors"
    #   "tx_errors"
    #   "rx_frame_err"
    #   "rx_over_err"
    #   "rx_crc_err"
    #   "collisions"
    #
    ################################################################################

    def register_for_port_stats_in(self, handler):
        self.register_handler(Port_stats_in_event.static_get_name(),
                              gen_ps_in_cb(handler))


    ################################################################################
    # register a handler to be called whenever table aggregate
    # statistics are returned by a switch.
    #
    # handler will be called with the following args:
    #
    # handler(dp_id, stats)
    #
    # Stats is a dictionary of aggregate stats with the following keys:
    #
    #   "packet_count"
    #   "byte_count"
    #   "flow_count"
    #
    ################################################################################

    def register_for_aggregate_stats_in(self, handler):
        self.register_handler(Aggregate_stats_in_event.static_get_name(),
                              gen_as_in_cb(handler))


    ################################################################################
    # register a handler to be called whenever description
    # statistics are returned by a switch.
    #
    # handler will be called with the following args:
    #
    # handler(dp_id, stats)
    #
    # Stats is a dictionary of descriptions with the following keys:
    #
    #   "mfr_desc"
    #   "hw_desc"
    #   "sw_desc"
    #   "serial_num"
    #
    ################################################################################

    def register_for_desc_stats_in(self, handler):
        self.register_handler(Desc_stats_in_event.static_get_name(),
                              gen_ds_in_cb(handler))

    def register_for_datapath_leave(self, handler):
        """
        register a handler to be called on a every datapath_leave
        event handler will be called with the following args:
        
        handler(dp_id)
        """
        self.register_handler(Datapath_leave_event.static_get_name(),
                              gen_dp_leave_cb(handler))

    ##########################################################################
    # register a handler to be called on a every port_status event
    # handler will be called with the following args:
    #
    # handler(dp_id, ofp_port_reason, attrs)
    #
    # attrs is a dictionary with the following keys:
        

    ###########################################################################
    def register_for_port_status(self, handler):
        self.register_handler(Port_status_event.static_get_name(),
                              gen_port_status_cb(handler))

    ###########################################################################
    # register a handler to be called on every packet_in event matching
    # the passed in expression.
    #
    # priority - the priority the installed classifier rule should have
    # expr - a dictionary containing 0 or more of the following keys.
        

    # Absent keys will be interpretted as wildcards (i.e. any value is
    # accepted for those attributes when checking for a potential match)
    #
    # handler will be called with the following args:
    #
    # handler(dp_id, inport, ofp_reason, total_frame_len, buffer_id,
    # captured_data)
    #
    # 'buffer_id' == None if the datapath does not have a buffer for
    # the frame
    ###########################################################################

    def register_for_packet_match(self, handler, priority, expr):
        e = Packet_expr()
        for key, val in expr.items():
            if key == AP_SRC:
                field = Packet_expr.AP_SRC
                val = htons(val)
            elif key == AP_DST:
                field = Packet_expr.AP_DST
                val = htons(val)
            elif key == DL_VLAN:
                field = Packet_expr.DL_VLAN
                val = htons(val)
            elif key == DL_VLAN_PCP:
                field = Packet_expr.DL_VLAN_PCP
                val = val
            elif key == DL_TYPE:
                field = Packet_expr.DL_TYPE
                val = htons(val)
            elif key == DL_SRC:
                field = Packet_expr.DL_SRC
                val = convert_to_eaddr(val)
                if val == None:
                    print 'invalid ethernet addr'
                    return False
            elif key == DL_DST:
                field = Packet_expr.DL_DST
                val = convert_to_eaddr(val)
                if val == None:
                    print 'invalid ethernet addr'
                    return False
            elif key == NW_SRC:
                field = Packet_expr.NW_SRC
                val = convert_to_ipaddr(val)
                if val == None:
                    print 'invalid ip addr'
                    return False
            elif key == NW_DST:
                field = Packet_expr.NW_DST
                val = convert_to_ipaddr(val)
                if val == None:
                    print 'invalid ip addr'
                    return False
            elif key == NW_PROTO:
                field = Packet_expr.NW_PROTO
            elif key == TP_SRC:
                field = Packet_expr.TP_SRC
                val = htons(val)
            elif key == TP_DST:
                field = Packet_expr.TP_DST
                val = htons(val)
            elif key == GROUP_SRC:
                field = Packet_expr.GROUP_SRC
                val = htonl(val)
            elif key == GROUP_DST:
                field = Packet_expr.GROUP_DST
                val = htonl(val)
            else:
                print 'invalid key', key
                return False
        
            if isinstance(val, ethernetaddr):
                e.set_eth_field(field, val)
            else:
                # check for max?
                if val > UINT32_MAX:
                    print 'value %u exceeds accepted range', val
                    return False
                e.set_uint32_field(field, val)

        return self.ctxt.register_handler_on_match(gen_packet_in_cb(handler), priority, e)

    def register_for_switch_mgr_join(self, handler):
        """
        register a handler to be called on every switch_mgr_join
        event handler will be called with the following args:
        
        handler(mgmt_id)
        """
        self.register_handler(Switch_mgr_join_event.static_get_name(),
                              gen_switch_mgr_join_cb(handler))

    def register_for_switch_mgr_leave(self, handler):
        """
        register a handler to be called on every switch_mgr_leave
        event handler will be called with the following args:
        
        handler(mgmt_id)
        """
        self.register_handler(Switch_mgr_leave_event.static_get_name(),
                              gen_switch_mgr_leave_cb(handler))

    def unregister_handler(self, rule_id):
        """
        Unregister a handler for match.
        """
        return self.ctxt.register_handler(event_type, event_name, handler)
