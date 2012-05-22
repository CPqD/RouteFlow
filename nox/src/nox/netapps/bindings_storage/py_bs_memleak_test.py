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
from nox.netapps.bindings_storage.pybindings_storage import pybindings_storage, Name
from nox.netapps.tests import unittest
from twisted.internet.defer import Deferred
from nox.netapps.storage import Storage
import time
import sys


class BindingsStorageMemCheck(Component):
    """ Mem-leak test for Bindings Storage"""

    ROUNDS = 1000

    def configure(self,configuration):
      pass

    def install(self):
      self.bs = self.resolve(pybindings_storage) 
      self.store = self.resolve(Storage) 
        
      self.num_triggers_inserted = 0 
      d = self.store.put_table_trigger("bindings_id", True, 
                                            self.trigger_callback)
      d.addCallback(self.trigger_inserted_callback)  
      d = self.store.put_table_trigger("bindings_name", True, 
                                            self.trigger_callback)
      d.addCallback(self.trigger_inserted_callback)  

    def getInterface(self):
        return str(BindingsStorageMemCheck)
      
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
      if(self.num_triggers_inserted == 2): 
        print "starting..." 
        self.do_clear() 

    def trigger_callback(self, trigger_id, row, reason):
      self.check_wait() 
    
    def names_cb(self, names):
      #  print "got names: " 
      #  for n in names: 
      #    print str(n) 
        self.check_wait() 
      
    def clear_cb(self):
      print "BS cleared.  sleeping" 
      time.sleep(5.0)
      print "awake again" 
      self.next_fn = self.do_write
      self.i = 0
      self.do_write() 

    def do_clear(self):
      print "sleeping before clear"
      time.sleep(5.0)
      print "clearing" 
      self.bs.clear(self.clear_cb)

    def do_write(self):
        i = self.i
        self.bs.store_binding_state(datapathid.from_host(i),i,
                ethernetaddr(i),i,"user" + str(i), Name.USER, False,0) 
        self.wait_count = 2
        self.i = self.i + 1 
        if self.i == self.ROUNDS: 
          self.next_fn = self.do_read
          self.i = 0

    def do_read(self): 
        i = self.i
        self.bs.get_names_by_ap(datapathid.from_host(i),i,self.names_cb)
        self.wait_count = 1
        self.i = self.i + 1 
        if self.i == self.ROUNDS:
          self.next_fn = self.do_clear
          self.i = 0

  
def getFactory():
        class Factory():
            def instance(self, context):
                return BindingsStorageMemCheck(context)

        return Factory()
