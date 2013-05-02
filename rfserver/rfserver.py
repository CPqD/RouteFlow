#!/usr/bin/env python
#-*- coding:utf-8 -*-

import sys
import logging
import binascii
import threading
import time
import argparse

from bson.binary import Binary

import rflib.ipc.IPC as IPC
import rflib.ipc.MongoIPC as MongoIPC
from rflib.ipc.RFProtocol import *
from rflib.ipc.RFProtocolFactory import RFProtocolFactory
from rflib.defs import *
from rflib.types.Match import *
from rflib.types.Action import *
from rflib.types.Option import *

from rftable import *

# Register actions
REGISTER_IDLE = 0
REGISTER_ASSOCIATED = 1
REGISTER_ISL = 2

class RFServer(RFProtocolFactory, IPC.IPCMessageProcessor):
    def __init__(self, configfile, islconffile):
        self.rftable = RFTable()
        self.isltable = RFISLTable()
        self.config = RFConfig(configfile)
        self.islconf = RFISLConf(islconffile)
        self.configured_rfvs = []
        # Logging
        self.log = logging.getLogger("rfserver")
        self.log.setLevel(logging.INFO)
        ch = logging.StreamHandler()
        ch.setLevel(logging.INFO)
        ch.setFormatter(logging.Formatter(logging.BASIC_FORMAT))
        self.log.addHandler(ch)

        self.ipc = MongoIPC.MongoIPCMessageService(MONGO_ADDRESS,
                                                   MONGO_DB_NAME,
                                                   RFSERVER_ID,
                                                   threading.Thread,
                                                   time.sleep)
        self.ipc.listen(RFCLIENT_RFSERVER_CHANNEL, self, self, False)
        self.ipc.listen(RFSERVER_RFPROXY_CHANNEL, self, self, True)

    def process(self, from_, to, channel, msg):
        type_ = msg.get_type()
        if type_ == PORT_REGISTER:
            self.register_vm_port(msg.get_vm_id(), msg.get_vm_port(),
                                  msg.get_hwaddress())
        elif type_ == ROUTE_MOD:
            self.register_route_mod(msg)
        elif type_ == DATAPATH_PORT_REGISTER:
            self.register_dp_port(msg.get_ct_id(),
                                  msg.get_dp_id(),
                                  msg.get_dp_port())
        elif type_ == DATAPATH_DOWN:
            self.set_dp_down(msg.get_ct_id(), msg.get_dp_id())
        elif type_ == VIRTUAL_PLANE_MAP:
            self.map_port(msg.get_vm_id(), msg.get_vm_port(),
                          msg.get_vs_id(), msg.get_vs_port())
        else:
            return False
        return True

    # Port register methods
    def register_vm_port(self, vm_id, vm_port, eth_addr):
        action = None
        config_entry = self.config.get_config_for_vm_port(vm_id, vm_port)
        if config_entry is None:
            # Register idle VM awaiting for configuration
            action = REGISTER_IDLE
        else:
            entry = self.rftable.get_entry_by_dp_port(config_entry.ct_id,
                                                      config_entry.dp_id,
                                                      config_entry.dp_port)
            # If there's no entry, we have no DP, register VM as idle
            if entry is None:
                action = REGISTER_IDLE
            # If there's an idle DP entry matching configuration, associate
            elif entry.get_status() == RFENTRY_IDLE_DP_PORT:
                action = REGISTER_ASSOCIATED

        # Apply action
        if action == REGISTER_IDLE:
            self.rftable.set_entry(RFEntry(vm_id=vm_id, vm_port=vm_port,
                                           eth_addr=eth_addr))
            self.log.info("Registering client port as idle (vm_id=%s, "
                          "vm_port=%i, eth_addr=%s)" % (format_id(vm_id),
                                                        vm_port, eth_addr))
        elif action == REGISTER_ASSOCIATED:
            entry.associate(vm_id, vm_port, eth_addr=eth_addr)
            self.rftable.set_entry(entry)
            self.config_vm_port(vm_id, vm_port)
            self.log.info("Registering client port and associating to "
                          "datapath port (vm_id=%s, vm_port=%i, "
                          "eth_addr = %s, dp_id=%s, dp_port=%s)"
                          % (format_id(vm_id), vm_port, eth_addr,
                             format_id(entry.dp_id), entry.dp_port))

    def config_vm_port(self, vm_id, vm_port):
        self.ipc.send(RFCLIENT_RFSERVER_CHANNEL, str(vm_id),
                      PortConfig(vm_id=vm_id, vm_port=vm_port, operation_id=0))
        self.log.info("Asking client for mapping message for port "
                      "(vm_id=%s, vm_port=%i)" % (format_id(vm_id), vm_port))

    # Handle RouteMod messages (type ROUTE_MOD)
    #
    # Takes a RouteMod, replaces its VM id,port with the associated DP id,port
    # and sends to the corresponding controller
    def register_route_mod(self, rm):
        vm_id = rm.get_id()

        # Find the output action
        for i, action in enumerate(rm.actions):
            if action['type'] is RFAT_OUTPUT:
                # Put the action in an action object for easy modification
                action_output = Action.from_dict(action)
                vm_port = action_output.get_value()

                # Find the (vmid, vm_port), (dpid, dpport) pair
                entry = self.rftable.get_entry_by_vm_port(vm_id, vm_port)

                # If we can't find an associated datapath for this RouteMod,
                # drop it.
                if entry is None or entry.get_status() == RFENTRY_IDLE_VM_PORT:
                    self.log.info("Received RouteMod destined for unknown "
                                  "datapath - Dropping (vm_id=%s)" %
                                  (format_id(vm_id)))
                    return

                # Replace the VM id,port with the Datapath id.port
                rm.set_id(int(entry.dp_id))

                if rm.get_mod() is RMT_DELETE:
                    # When deleting a route, we don't need an output action.
                    rm.actions.remove(action)
                else:
                    # Replace the VM port with the datapath port
                    action_output.set_value(entry.dp_port)
                    rm.actions[i] = action_output.to_dict()

                entries = self.rftable.get_entries(dp_id=entry.dp_id,
                                                   ct_id=entry.ct_id)
                entries.extend(self.isltable.get_entries(dp_id=entry.dp_id,
                                                         ct_id=entry.ct_id))
                rm.add_option(Option.CT_ID(entry.ct_id))

                self._send_rm_with_matches(rm, entry.dp_port, entries)

                remote_dps = self.isltable.get_entries(rem_ct=entry.ct_id,
                                                       rem_id=entry.dp_id)
                for r in remote_dps:
                    if r.get_status() == RFISL_ACTIVE:
                        rm.set_options(rm.get_options()[:-1])
                        rm.add_option(Option.CT_ID(r.ct_id))
                        rm.set_id(int(r.dp_id))
                        rm.set_actions(None)
                        rm.add_action(Action.SET_ETH_SRC(r.eth_addr))
                        rm.add_action(Action.SET_ETH_DST(r.rem_eth_addr))
                        rm.add_action(Action.OUTPUT(r.dp_port))
                        entries = self.rftable.get_entries(dp_id=r.dp_id,
                                                           ct_id=r.ct_id)
                        self._send_rm_with_matches(rm, r.dp_port, entries)

                return

        # If no output action is found, don't forward the routemod.
        self.log.info("Received RouteMod with no Output Port - Dropping "
                      "(vm_id=%s)" % (format_id(vm_id)))

    def _send_rm_with_matches(self, rm, out_port, entries):
        #send entries matching external ports
        for entry in entries:
            if out_port != entry.dp_port:
                if entry.get_status() == RFENTRY_ACTIVE or \
                   entry.get_status() == RFISL_ACTIVE:
                    rm.add_match(Match.ETHERNET(entry.eth_addr))
                    rm.add_match(Match.IN_PORT(entry.dp_port))
                    self.ipc.send(RFSERVER_RFPROXY_CHANNEL,
                                  str(entry.ct_id), rm)
                    rm.set_matches(rm.get_matches()[:-2])

    # DatapathPortRegister methods
    def register_dp_port(self, ct_id, dp_id, dp_port):
        stop = self.config_dp(ct_id, dp_id)
        if stop:
            return

        # The logic down here is pretty much the same as register_vm_port
        action = None
        config_entry = self.config.get_config_for_dp_port(ct_id, dp_id,
                                                          dp_port)
        if config_entry is None:
            islconfs = self.islconf.get_entries_by_port(ct_id, dp_id, dp_port)
            if islconfs:
                action = REGISTER_ISL
            else:
                # Register idle DP awaiting for configuration
                action = REGISTER_IDLE
        else:
            entry = self.rftable.get_entry_by_vm_port(config_entry.vm_id,
                                                      config_entry.vm_port)
            # If there's no entry, we have no VM, register DP as idle
            if entry is None:
                action = REGISTER_IDLE
            # If there's an idle VM entry matching configuration, associate
            elif entry.get_status() == RFENTRY_IDLE_VM_PORT:
                action = REGISTER_ASSOCIATED

        # Apply action
        if action == REGISTER_IDLE:
            self.rftable.set_entry(RFEntry(ct_id=ct_id, dp_id=dp_id,
                                           dp_port=dp_port))
            self.log.info("Registering datapath port as idle (dp_id=%s, "
                          "dp_port=%i)" % (format_id(dp_id), dp_port))
        elif action == REGISTER_ASSOCIATED:
            entry.associate(dp_id, dp_port, ct_id)
            self.rftable.set_entry(entry)
            self.config_vm_port(entry.vm_id, entry.vm_port)
            self.log.info("Registering datapath port and associating to "
                          "client port (dp_id=%s, dp_port=%i, vm_id=%s, "
                          "vm_port=%s)" % (format_id(dp_id), dp_port,
                                           format_id(entry.vm_id),
                                           entry.vm_port))
        elif action == REGISTER_ISL:
            self._register_islconf(islconfs, ct_id, dp_id, dp_port)

    def _register_islconf(self, c_entries, ct_id, dp_id, dp_port):
        for conf in c_entries:
            entry = None
            eth_addr = None
            if conf.rem_id != dp_id or conf.rem_ct != ct_id:
                entry = self.isltable.get_entry_by_addr(conf.rem_ct,
                                                        conf.rem_id,
                                                        conf.rem_port,
                                                        conf.rem_eth_addr)
                eth_addr = conf.eth_addr
            else:
                entry = self.isltable.get_entry_by_addr(conf.ct_id,
                                                        conf.dp_id,
                                                        conf.dp_port,
                                                        conf.eth_addr)
                eth_addr = conf.rem_eth_addr

            if entry is None:
                n_entry = RFISLEntry(vm_id=conf.vm_id, ct_id=ct_id,
                                     dp_id=dp_id, dp_port=dp_port,
                                     eth_addr=eth_addr)
                self.isltable.set_entry(n_entry)
                self.log.info("Registering ISL port as idle "
                              "(dp_id=%s, dp_port=%i, eth_addr=%s)" %
                              (format_id(dp_id), dp_port, eth_addr))
            elif entry.get_status() == RFISL_IDLE_DP_PORT:
                entry.associate(ct_id, dp_id, dp_port, eth_addr)
                self.isltable.set_entry(entry)
                n_entry = self.isltable.get_entry_by_remote(entry.ct_id,
                                                            entry.dp_id,
                                                            entry.dp_port,
                                                            entry.eth_addr)
                if n_entry is None:
                    n_entry = RFISLEntry(vm_id=entry.vm_id, ct_id=ct_id,
                                         dp_id=dp_id, dp_port=dp_port,
                                         eth_addr=entry.rem_eth_addr,
                                         rem_ct=entry.ct_id,
                                         rem_id=entry.dp_id,
                                         rem_port=entry.dp_port,
                                         rem_eth_addr=entry.eth_addr)
                    self.isltable.set_entry(n_entry)
                else:
                    n_entry.associate(ct_id, dp_id, dp_port, eth_addr)
                    self.isltable.set_entry(n_entry)
                self.log.info("Registering ISL port and associating to "
                              "remote ISL port (ct_id=%s, dp_id=%s, "
                              "dp_port=%s, rem_ct=%s, rem_id=%s, "
                              "rem_port=%s)" % (ct_id, format_id(dp_id),
                                                dp_port, entry.ct_id,
                                                format_id(entry.dp_id),
                                                entry.dp_port))

    def send_datapath_config_message(self, ct_id, dp_id, operation_id):
        rm = RouteMod(RMT_ADD, dp_id)

        if operation_id == DC_CLEAR_FLOW_TABLE:
            rm.set_mod(RMT_DELETE)
            rm.add_option(Option.PRIORITY(PRIORITY_LOWEST))
        elif operation_id == DC_DROP_ALL:
            rm.add_option(Option.PRIORITY(PRIORITY_LOWEST + PRIORITY_BAND))
            # No action specifies discard
            pass
        else:
            rm.add_option(Option.PRIORITY(PRIORITY_HIGH))
            if operation_id == DC_RIPV2:
                rm.add_match(Match.ETHERTYPE(ETHERTYPE_IP))
                rm.add_match(Match.NW_PROTO(IPPROTO_UDP))
                rm.add_match(Match.IPV4(IPADDR_RIPv2, IPV4_MASK_EXACT))
            elif operation_id == DC_OSPF:
                rm.add_match(Match.ETHERTYPE(ETHERTYPE_IP))
                rm.add_match(Match.NW_PROTO(IPPROTO_OSPF))
            elif operation_id == DC_ARP:
                rm.add_match(Match.ETHERTYPE(ETHERTYPE_ARP))
            elif operation_id == DC_ICMP:
                rm.add_match(Match.ETHERTYPE(ETHERTYPE_IP))
                rm.add_match(Match.NW_PROTO(IPPROTO_ICMP))
            elif operation_id == DC_ICMPV6:
                rm.add_match(Match.ETHERTYPE(ETHERTYPE_IPV6))
                rm.add_match(Match.NW_PROTO(IPPROTO_ICMPV6))
            elif operation_id == DC_BGP_PASSIVE:
                rm.add_match(Match.ETHERTYPE(ETHERTYPE_IP))
                rm.add_match(Match.NW_PROTO(IPPROTO_TCP))
                rm.add_match(Match.TP_DST(TPORT_BGP))
            elif operation_id == DC_BGP_ACTIVE:
                rm.add_match(Match.ETHERTYPE(ETHERTYPE_IP))
                rm.add_match(Match.NW_PROTO(IPPROTO_TCP))
                rm.add_match(Match.TP_SRC(TPORT_BGP))
            elif operation_id == DC_LDP_PASSIVE:
                rm.add_match(Match.ETHERTYPE(ETHERTYPE_IP))
                rm.add_match(Match.NW_PROTO(IPPROTO_TCP))
                rm.add_match(Match.TP_DST(TPORT_LDP))
            elif operation_id == DC_LDP_ACTIVE:
                rm.add_match(Match.ETHERTYPE(ETHERTYPE_IP))
                rm.add_match(Match.NW_PROTO(IPPROTO_TCP))
                rm.add_match(Match.TP_SRC(TPORT_LDP))
            elif operation_id == DC_VM_INFO:
                rm.add_match(Match.ETHERTYPE(RF_ETH_PROTO))
            rm.add_action(Action.CONTROLLER())

        rm.add_option(Option.CT_ID(ct_id))
        self.ipc.send(RFSERVER_RFPROXY_CHANNEL, str(ct_id), rm)

    def config_dp(self, ct_id, dp_id):
        if is_rfvs(dp_id) and dp_id not in self.configured_rfvs:
            # If rfvs is coming up and we haven't configured it yet, do it
            # TODO: support more than one OVS
            self.configured_rfvs.append(dp_id)
            self.send_datapath_config_message(ct_id, dp_id, DC_ALL)
            self.log.info("Configuring RFVS (dp_id=%s)" % format_id(dp_id))
        elif self.rftable.is_dp_registered(ct_id, dp_id) or \
             self.isltable.is_dp_registered(ct_id, dp_id):
            # Configure a normal switch. Clear the tables and install default
            # flows.
            self.send_datapath_config_message(ct_id, dp_id,
                                              DC_CLEAR_FLOW_TABLE)
            # TODO: enforce order: clear should always be executed first
            self.send_datapath_config_message(ct_id, dp_id, DC_DROP_ALL)
            self.send_datapath_config_message(ct_id, dp_id, DC_OSPF)
            self.send_datapath_config_message(ct_id, dp_id, DC_BGP_PASSIVE)
            self.send_datapath_config_message(ct_id, dp_id, DC_BGP_ACTIVE)
            self.send_datapath_config_message(ct_id, dp_id, DC_RIPV2)
            self.send_datapath_config_message(ct_id, dp_id, DC_ARP)
            self.send_datapath_config_message(ct_id, dp_id, DC_ICMP)
            self.send_datapath_config_message(ct_id, dp_id, DC_ICMPV6)
            self.send_datapath_config_message(ct_id, dp_id, DC_LDP_PASSIVE)
            self.send_datapath_config_message(ct_id, dp_id, DC_LDP_ACTIVE)
            self.log.info("Configuring datapath (dp_id=%s)" % format_id(dp_id))
        return is_rfvs(dp_id)

    # DatapathDown methods
    def set_dp_down(self, ct_id, dp_id):
        for entry in self.rftable.get_dp_entries(ct_id, dp_id):
            # For every port registered in that datapath, put it down
            self.set_dp_port_down(entry.ct_id, entry.dp_id, entry.dp_port)
        for entry in self.isltable.get_dp_entries(ct_id, dp_id):
            entry.make_idle(RFISL_IDLE_REMOTE)
            self.isltable.set_entry(entry)
        for entry in self.isltable.get_entries(rem_ct=ct_id, rem_id=dp_id):
            entry.make_idle(RFISL_IDLE_DP_PORT)
            self.isltable.set_entry(entry)
        self.log.info("Datapath down (dp_id=%s)" % format_id(dp_id))

    def set_dp_port_down(self, ct_id, dp_id, dp_port):
        entry = self.rftable.get_entry_by_dp_port(ct_id, dp_id, dp_port)
        if entry is not None:
            # If the DP port is registered, delete it and leave only the
            # associated VM port. Reset this VM port so it can be reused.
            vm_id, vm_port = entry.vm_id, entry.vm_port
            entry.make_idle(RFENTRY_IDLE_VM_PORT)
            self.rftable.set_entry(entry)
            if vm_id is not None:
                self.reset_vm_port(vm_id, vm_port)
            self.log.debug("Datapath port down (dp_id=%s, dp_port=%i)" %
                           (format_id(dp_id), dp_port))

    def reset_vm_port(self, vm_id, vm_port):
        if vm_id is None:
            return
        self.ipc.send(RFCLIENT_RFSERVER_CHANNEL, str(vm_id),
                      PortConfig(vm_id=vm_id, vm_port=vm_port, operation_id=1))
        self.log.info("Resetting client port (vm_id=%s, vm_port=%i)" %
                      (format_id(vm_id), vm_port))

    # PortMap methods
    def map_port(self, vm_id, vm_port, vs_id, vs_port):
        entry = self.rftable.get_entry_by_vm_port(vm_id, vm_port)
        if entry is not None and entry.get_status() == RFENTRY_ASSOCIATED:
            # If the association is valid, activate it
            entry.activate(vs_id, vs_port)
            self.rftable.set_entry(entry)
            msg = DataPlaneMap(ct_id=entry.ct_id,
                               dp_id=entry.dp_id, dp_port=entry.dp_port,
                               vs_id=vs_id, vs_port=vs_port)
            self.ipc.send(RFSERVER_RFPROXY_CHANNEL, str(entry.ct_id), msg)
            self.log.info("Mapping client-datapath association "
                          "(vm_id=%s, vm_port=%i, dp_id=%s, "
                          "dp_port=%i, vs_id=%s, vs_port=%i)" %
                          (format_id(entry.vm_id), entry.vm_port,
                           format_id(entry.dp_id), entry.dp_port,
                           format_id(entry.vs_id), entry.vs_port))

if __name__ == "__main__":
    description='RFServer co-ordinates RFClient and RFProxy instances, ' \
                'listens for route updates, and configures flow tables'
    epilog='Report bugs to: https://github.com/CPqD/RouteFlow/issues'

    parser = argparse.ArgumentParser(description=description, epilog=epilog)
    parser.add_argument('configfile',
                        help='VM-VS-DP mapping configuration file')
    parser.add_argument('-i', '--islconfig',
                        help='ISL mapping configuration file')

    args = parser.parse_args()
    try:
        RFServer(args.configfile, args.islconfig)
    except IOError:
        sys.exit("Error opening file: {}".format(args.configfile))
