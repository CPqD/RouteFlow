/* Copyright 2008 (C) Nicira, Inc.
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
%module "nox.netapps.user_event_log"


%{
#include "user_event_log_proxy.hh"
#include "pyrt/pycontext.hh"
#include "bindings_storage/bindings_storage.hh" 
using namespace vigil;
using namespace vigil::applications;
%}

%import "netinet/netinet.i"

%include "common-defs.i"
%include "std_string.i"
%include "user_event_log_proxy.hh"
%include "../bindings_storage/bs_datastructures.i" 

struct LogEntry { 

    enum Level { INVALID = 0, CRITICAL, ALERT, INFO }; 
    enum Direction { SRC = 0, DST } ; 

    LogEntry(const string &_app, Level _level, const string &_msg);  
    void setName(int64_t uid, int64_t type,Direction dir); 
    //void setNameByLocation(const datapathid &dpid, uint16_t port, Direction dir); 
    void addLocationKey(const datapathid &dpid, uint16_t port,Direction dir);  
    void addMacKey(const ethernetaddr &dladdr, Direction dir); 
    void addIPKey(uint32_t nwaddr, Direction dir);  
}; 


%pythoncode
%{
  from nox.lib.core import Component
  from nox.netapps.bindings_storage.pybindings_storage import Name
  from nox.netapps.data.datatypes_impl import Datatypes
  from nox.coreapps.pyrt.pycomponent import CONTINUE


  # TODO: remove these, and use swigged values instead
  # from LogEntry class
  class LogLevel: 
    INVALID = 0    
    CRITICAL = 1
    ALERT = 2 
    INFO = 3 

  class pyuser_event_log(Component):
      """
      Python interface for the User_Event_Log 
      """  
      def __init__(self, ctxt):
          Component.__init__(self,ctxt)
          self.proxy = user_event_log_proxy(ctxt)
          self.principal_format_map = None

      def configure(self, configuration):
          self.proxy.configure(configuration)

      def install(self): 
          self.datatypes = self.resolve(Datatypes) 
          self.register_for_bootstrap_complete(self.bootstrap_complete_cb)

      def get_fmt_str(self,type,dir):
        for k,v in self.principal_format_map.items(): 
          if v[0] == type and v[1] == dir: 
            return k
        raise Exception("Invalid type and direction for format string")

      def bootstrap_complete_cb(self,ign): 
          self.principal_format_map = {  
            "su" : (self.datatypes.USER,LogEntry.SRC),  
            "du" : (self.datatypes.USER,LogEntry.DST),  
            "sh" : (self.datatypes.HOST,LogEntry.SRC),  
            "dh" : (self.datatypes.HOST,LogEntry.DST),  
            "sn" : (self.datatypes.HOST_NETID,LogEntry.SRC),  
            "dn" : (self.datatypes.HOST_NETID,LogEntry.DST),  
            "sl" : (self.datatypes.LOCATION,LogEntry.SRC), 
            "dl" : (self.datatypes.LOCATION,LogEntry.DST),  
            "ss" : (self.datatypes.SWITCH,LogEntry.SRC),  
            "ds" : (self.datatypes.SWITCH,LogEntry.DST),  
            "sg" : (self.datatypes.GROUP,LogEntry.SRC),   
            "dg" : (self.datatypes.GROUP,LogEntry.DST),  
          }
          return CONTINUE

      def getInterface(self):
          return str(pyuser_event_log)

      def log_simple(self, app_name, level,msg):  
          self.proxy.log_simple(app_name,int(level),msg)
      
      def log_entry(self, entry):  
          self.proxy.log(entry)
      
      # convenience function for logging in one line
      # when using keys.  
      # With respect to add*Key(), this function expects one or
      # two dict arguments indicating a source key and a
      # destination key.  Variable names are:
      # src_location, dst_location, src_mac, dst_mac, src_ip, dst_ip:
      # *_location must be a tuple containing a datapathid and a port
      # *_mac must be an etheraddr
      # *_ip must be an int 
      # if you specify more than one src_* paramter, or more than
      # one dst_* parameter, the result of the function is undefined.
      # With respect to setName() functionality, this can take any of
      # the following parameters (su,du,sh,dh,sl,dl).  Each should be
      # a list of strings.  For example, to associated two source users
      # with this log message, have su = ("bob","sarah")
      # This method also special-cases the scenario when there is only
      # one name, allowing you to pass in just the string instead of
      # a tuple with one item
      # Note: Group names can only be set explicitly.  No bindings 
      # storage query returns a group name

      def log(self, app_name, level, msg, **dict):  
         
          e = LogEntry(app_name,level,msg)
          if "src_location" in dict and len(dict["src_location"]) == 2: 
            t = dict["src_location"] 
            e.addLocationKey(t[0],t[1],LogEntry.SRC)
          if "dst_location" in dict and len(dict["dst_location"]) == 2: 
            t = dict["dst_location"] 
            e.addLocationKey(t[0],t[1],LogEntry.DST)
          if "src_mac" in dict : 
            e.addMacKey(dict["src_mac"],LogEntry.SRC)
          if "dst_mac" in dict : 
            e.addMacKey(dict["dst_mac"],LogEntry.DST)
          if "src_ip" in dict : 
            e.addIPKey(dict["src_ip"],LogEntry.SRC)
          if "dst_ip" in dict : 
            e.addIPKey(dict["dst_ip"],LogEntry.DST)
        
          # for each key-value pair in the keyword args dict,
          # test if it is a format string and, and if so
          # add the corresponding names to the log entry
          for fmt,values in dict.iteritems(): 
            if fmt in self.principal_format_map :
              principal_type,dir = self.principal_format_map[fmt]
              if type(values) == type(0) or type(values) == type(0L): 
                values = (values,) 
              for uid in values: 
                e.setName(uid,principal_type,dir)
    
          #if "set_src_loc" in dict:
          #  t = dict["set_src_loc"] # expects (dpid,port) tuple
          #  e.setNameByLocation(t[0],t[1],LogEntry.SRC)
          
          #if "set_dst_loc" in dict:
          #  t = dict["set_dst_loc"] # expects (dpid,port) tuple
          #  e.setNameByLocation(t[0],t[1],LogEntry.DST) 

          self.proxy.log(e)

      def get_log_entry(self,logid, cb):
          self.proxy.get_log_entry(logid,cb) 

      def get_max_logid(self):
          return self.proxy.get_max_logid()
      
      def get_min_logid(self):
          return self.proxy.get_min_logid()
    
      def set_max_num_entries(self,num): 
          self.proxy.set_max_num_entries(num)

      def get_logids_for_name(self,name,name_type,cb): 
          self.proxy.get_logids_for_name(name,name_type,cb) 

      def clear(self,cb):
          self.proxy.clear(cb)

      def remove(self,max_logid,cb):
          self.proxy.remove(max_logid,cb)

  def getFactory():
        class Factory():
            def instance(self, context):
                return pyuser_event_log(context)

        return Factory()
%}
