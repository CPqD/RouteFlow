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

from nox.lib.core     import Component
from nox.lib.netinet.netinet import *
from nox.netapps.user_event_log.pyuser_event_log import pyuser_event_log
from nox.netapps.bindings_storage.pybindings_storage import pybindings_storage,Name
from twisted.internet.defer import Deferred
from nox.netapps.storage import Storage
import time
import sys


class UserEventLogMemCheck(Component):
    """ Mem-leak test for User Event Log"""

    ROUNDS = 1000

    def configure(self,configuration):
      pass

    def install(self):
      self.total_count = 0
      self.uel = self.resolve(pyuser_event_log) 
      self.bs = self.resolve(pybindings_storage) 
      self.store = self.resolve(Storage) 
        
      self.num_triggers_inserted = 0 
      d = self.store.put_table_trigger("user_event_log", True, 
                                            self.trigger_callback)
      d.addCallback(self.trigger_inserted_callback)  
      d = self.store.put_table_trigger("user_event_log_names", True, 
                                            self.trigger_callback)
      d.addCallback(self.trigger_inserted_callback)  
      d = self.store.put_table_trigger("bindings_name", True, 
                                            self.trigger_callback)
      d.addCallback(self.trigger_inserted_callback)  

    def getInterface(self):
        return str(UserEventLogMemCheck)
      
    def check_wait(self): 
      self.wait_count = self.wait_count - 1
      if self.wait_count == 0: 
        self.next_fn() 
    
    def trigger_inserted_callback(self, res): 
      result , tid = res 

      if (result[0] != 0): 
        print "put trigger failed: " + str(result[1]) 
        sys.exit(1) 
        return 
      self.num_triggers_inserted = self.num_triggers_inserted + 1
      if(self.num_triggers_inserted == 3): 
        self.setup_bindings() 

    def setup_bindings(self): 
        print "setup bindings"
        self.bs.store_binding_state(datapathid.from_host(1),1,
                ethernetaddr(1),1,"dan", Name.USER, False,0) 
        self.bs.store_binding_state(datapathid.from_host(1),1,
                ethernetaddr(1),1,"javelina", Name.HOST, False,0) 
        
        self.next_fn = self.do_write
        self.i = 0
        self.wait_count = 2 # trigger only on bindings_name
        print "starting..." 

    def trigger_callback(self, trigger_id, row, reason):
      #print "trigger fired: " + str(trigger_id)
      self.check_wait() 
    
    def clear_cb(self):
      print "UEL cleared.  sleeping" 
      self.post_callback(5,self.prepare_write)

    def prepare_write(self): 
      print "awake for writing: total_count = " + str(self.total_count) 
      self.next_fn = self.do_write
      self.i = 0  # reset rounds counter
      self.do_write() 
    
    def do_sleep1(self):
      print "sleeping before clear ..."
      self.post_callback(5,self.do_clear)

    def do_clear(self):
      print "clearing" 
      self.uel.clear(self.clear_cb)

    def do_write(self):
        #print "do_write" 
        self.total_count = self.total_count + 1
        i = self.i
        self.uel.log("memtest",1,"msg" + str(i), src_mac = ethernetaddr(1), 
                                                 dst_mac = ethernetaddr(2)) 
        self.wait_count = 3
        self.i = self.i + 1 
        if self.i == self.ROUNDS:
          print "starting read"
          self.next_fn = self.do_read
          self.i = 0

    def do_read(self): 
        #print "do_read" 
        i = self.i
        self.uel.get_log_entry(i,self.get_entry_cb)
        self.wait_count = 1
        self.i = self.i + 1 
        if self.i == self.ROUNDS:
          self.next_fn = self.do_sleep1
          self.i = 0
    
    def get_entry_cb(self, logid, ts, app_name,level,msg,src_names,dst_names):
      #print "callback with logid = " + str(logid) + " and msg = '" + msg + "'"
      self.check_wait() 
      

  
def getFactory():
        class Factory():
            def instance(self, context):
                return UserEventLogMemCheck(context)

        return Factory()
