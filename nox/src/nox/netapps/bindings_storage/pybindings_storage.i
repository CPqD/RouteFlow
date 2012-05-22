/* Copyright 2008, 2009 (C) Nicira, Inc.
 *
 * This file is part of NOX.
 *
 * NOX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NOX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NOX.  If not, see <http://www.gnu.org/licenses/>.
 */
%module "nox.netapps.bindings_storage"

%{
#include "bindings_storage_proxy.hh"
#include "pyrt/pycontext.hh"
using namespace vigil;
using namespace vigil::applications;
%}

%import "netinet/netinet.i"

%include "common-defs.i"
%include "utf8_string.i"

%typemap(in) const storage::Query& (storage::Query temp) {

      PyObject *val = PyDict_GetItemString($input, "dladdr");
      if (val != NULL) {
          temp["dladdr"] = (int64_t)(PyLong_AsUnsignedLongLongMask(val));
      }
      val = PyDict_GetItemString($input, "nwaddr");
      if (val != NULL) {
          temp["nwaddr"] = (int64_t)(PyLong_AsUnsignedLongMask(val));
      }
      val = PyDict_GetItemString($input, "dpid");
      if (val != NULL) {
          temp["dpid"] = (int64_t)(PyLong_AsUnsignedLongLongMask(val));
      }
      val = PyDict_GetItemString($input, "port");
      if (val != NULL) {
          temp["port"] = (int64_t)(PyLong_AsUnsignedLongMask(val));
      }
      $1 = &temp;
}

%include "bs_datastructures.i" 
%include "bindings_storage_proxy.hh"

%pythoncode
%{
  from nox.lib.core import Component
  from twisted.internet import defer 
  from twisted.python.failure import Failure
  from nox.lib.netinet.netinet import *

  import logging

  lg = logging.getLogger('nox.netapps.bindings_storage')
 
  class pybindings_storage(Component):
      """
      Python interface for the Bindings_Storage class 
      """  
      def __init__(self, ctxt):
          self.proxy = Bindings_Storage_Proxy(ctxt)

      def configure(self, configuration):
          self.proxy.configure(configuration)

      def getInterface(self):
          return str(pybindings_storage)

      # callback gets a list of (name,name_type) tuples
      def get_names(self, dpid, port, mac, ip, cb):
          self.proxy.get_names(dpid, port, mac, ip, cb) 

      def get_names_query(self, q, loc_tuples, cb):
          self.proxy.get_names(q, loc_tuples, cb)

      def get_names_by_ap(self, dpid, port, cb):
          self.proxy.get_names_by_ap(dpid, port, cb) 

      def get_names_by_mac(self, mac, cb):
          self.proxy.get_names_by_mac(mac, cb)
      
      def get_names_by_ip(self, ip, cb):
          self.proxy.get_names_by_ip(ip, cb)
      
      def get_all_names(self, name_type, cb):
          self.proxy.get_all_names(name_type, cb) 

      def get_host_users(self, hostname, cb):
          self.proxy.get_host_users(hostname, cb) 

      def get_user_hosts(self, username, cb):
          self.proxy.get_user_hosts(username, cb) 

      # callback gets a list of (dpid,port,mac,ip) tuples
      def get_entities_by_name(self, name, name_type, cb):
          self.proxy.get_entities_by_name(name, name_type, cb)

      def add_link(self, dpid1, port1, dpid2, port2):
        self.proxy.add_link(dpid1, port1, dpid2, port2)
      
      def remove_link(self, dpid1, port1, dpid2, port2):
        self.proxy.remove_link(dpid1, port1, dpid2, port2)

      def get_all_links(self, cb): 
        self.proxy.get_all_links(cb) 
    
      def get_switch_links(self, dpid, cb):
        self.proxy.get_links(dpid, cb)
      
      def get_links(self, dpid, port, cb):
        self.proxy.get_links(dpid, port, cb)
    
      def clear_links(self):
          self.proxy.clear_links()

      def get_names_for_location(self, dpid, port, name_type, cb):
          self.proxy.get_names_for_location(dpid, port, name_type, cb)

      # note: calling this with a port name (e.g., 'eth0') 
      # will get you all locations that have that name
      # You should use get_location_by_switchport_name() instead
      def get_location_by_name(self, name, name_type, cb):
          self.proxy.get_location_by_name(name, name_type, cb) 

      def get_location_by_switchport_name(self, switchname, portname, cb): 

        def cb1(loc_list1):
          if len(loc_list1) == 0: 
            cb([]) 
            return 
          elif len(loc_list1) > 1: 
            print("Lookup for switch '%s' got multiple dpids" % switchname)
          dpid = loc_list1[0][0] 
          
          def cb2(loc_list2): 
            for loc in loc_list2: 
              if loc[0] == dpid: 
                cb([loc]) 
                return 
            cb([])
          
          self.proxy.get_location_by_name(portname, Name.PORT, cb2)

        self.proxy.get_location_by_name(switchname, Name.SWITCH, cb1) 
 
      # note: if multiple hostnames are bound to an IP address, 
      # this method aribtrarily chooses one of them  
      def get_netinfos_by_ip(self, ip):
        d = defer.Deferred()
        def filter_netinfos(netinfos): 
            matching = [] 
            for ni in netinfos:
              if ni[3] == ip:
                matching.append(ni) 
            d.callback(matching) 

        def process_names(names): 
            for name, type, id in names: 
              if type == Name.HOST: 
                self.get_entities_by_name(id, type, filter_netinfos)
                return
            ip_obj = ipaddr(ip)
            d.errback(Failure("no hostname in binding storage for IP : %s" \
               % str(ip_obj)))

        self.get_names_by_ip(ip, process_names)
        return d

  def getFactory():
        class Factory():
            def instance(self, context):
                return pybindings_storage(context)

        return Factory()
%}
