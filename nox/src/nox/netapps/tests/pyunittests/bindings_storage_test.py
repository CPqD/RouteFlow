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

pyunit = __import__('unittest')

class BindingsStorageTestCase(unittest.NoxTestCase):
    """ Simple Bindings_Storage python tests.  Main tests are in C++"""

    def configure(self,configuration):
      pass

    def install(self):
      self.bs = self.resolve(pybindings_storage) 
      self.store = self.resolve(Storage)
      self.debug = False
      #self.debug = True

    def getInterface(self):
        return str(BindingsStorageTestCase)
      
    def check_wait(self): 
      self.wait_count = self.wait_count - 1
      if self.wait_count == 0: 
        self.next_fn() 

    def names_cb(self, names):
      if(self.debug): 
        print "expected names: " 
        for n in self.expected_names: 
          print str(n) 
        print "got names: " 
        for n in names: 
          print str(n) 
      
        assert(len(names) == len(self.expected_names))

      
      for n in names: 
        found = False
        for e in self.expected_names:
          if e == n: 
            found = True
            break
        assert(found)

      self.check_wait() 


    def clear_cb(self):
        self.num_triggers_inserted = 0 
        d = self.store.put_table_trigger("bindings_id", True, 
                                            self.trigger_callback)
        d.addCallback(self.trigger_inserted_callback)  
        d = self.store.put_table_trigger("bindings_name", True, 
                                            self.trigger_callback)
        d.addCallback(self.trigger_inserted_callback)  
      
    def trigger_inserted_callback(self, res): 
      result , tid = res 

      if (result[0] != 0): 
        print "put trigger failed: " + str(result[1]) 
        self.exit_test(False) 
        return 
      self.num_triggers_inserted = self.num_triggers_inserted + 1
      if(self.num_triggers_inserted == 1):
        self.trigger1 = tid 
      if(self.num_triggers_inserted == 2): 
        self.trigger2 = tid
        self.run_test1() 

    def trigger_callback(self, trigger_id, row, reason):
      self.check_wait() 

    def run_test1(self):
      
      self.dpid = datapathid.from_host(1)
      self.port = 1
      mac = ethernetaddr(1)
      ip = 1

      self.bs.store_binding_state(datapathid.from_host(1),1,ethernetaddr(1),1,
                                                    "", Name.NONE, False,0) 
      self.bs.store_binding_state(datapathid.from_host(1),1,ethernetaddr(1),1,
                                                    "dan",Name.USER,False,0) 
      self.bs.store_binding_state(datapathid.from_host(1),1,ethernetaddr(1),2,
                                                  "dan-2",Name.USER,True,ip) 
      self.next_fn = self.run_test2
      self.wait_count = 4 

    def run_test2(self): 
      if(self.debug): 
        print "getting names based on net-ids " 
      self.expected_names = [ ("dan",Name.USER), ("dan-2",Name.USER),
                                ("none;000000000001:1", Name.LOCATION) ] 
      self.bs.get_names_by_ap(datapathid.from_host(1), 1, self.names_cb) 
      self.bs.get_names_by_mac(ethernetaddr(1), self.names_cb) 
      self.bs.get_names_by_ip(1, self.names_cb) 
      self.bs.get_names(datapathid.from_host(1),1,ethernetaddr(1),1,self.names_cb) 
      
      self.next_fn = self.run_test3
      self.wait_count = 4 

    def run_test3(self):
      if(self.debug): 
        print "removing binding state for user = dan" 
      self.bs.remove_binding_state(datapathid.from_host(1),1,ethernetaddr(1),1,
                                                          "dan",Name.USER) 
      self.next_fn = self.run_test4
      self.wait_count = 1 

    def run_test4(self):
      self.expected_names = [ ("dan-2",Name.USER), ("none;000000000001:1",Name.LOCATION) ] 
      self.bs.get_names(datapathid.from_host(1),1,ethernetaddr(1),1,self.names_cb) 
      
      self.next_fn = self.run_test5
      self.wait_count = 1 

    def run_test5(self): 
      if(self.debug): 
        print "removing machine" 
      self.bs.remove_machine(datapathid.from_host(1),1,ethernetaddr(1),1,True) 

      self.next_fn = self.run_test6
      self.wait_count = 3 

    def run_test6(self): 
      self.expected_names = [ ] 
      self.bs.get_names(datapathid.from_host(1),1,ethernetaddr(1),1,self.names_cb) 
 
      self.next_fn = self.run_test7
      self.wait_count = 1 

    def run_test7(self): 
      d = self.store.remove_trigger(self.trigger1)
      d.addCallback(self.remove_trigger_callback)
      d = self.store.remove_trigger(self.trigger2)
      d.addCallback(self.remove_trigger_callback)
    
    def start(self): 
      self.bs.clear(self.clear_cb) 

      self.deferred = Deferred()
      return self.deferred

    def exit_test(self, success) : 
      if(not success):
        assert(False)
      self.deferred.callback("done") # tell test-suite that we are done
    
    def remove_trigger_callback(self, result): 
      if (result[0] != 0): 
        print "remove trigger failed: " + str(result[1]) 
        self.exit_test(False) 
        return 
      
      self.num_triggers_inserted = self.num_triggers_inserted - 1 
      if self.num_triggers_inserted == 0: 
        self.exit_test(True) # success 


def suite(ctxt):
    suite = pyunit.TestSuite()
    suite.addTest(BindingsStorageTestCase("start", ctxt))
    return suite
