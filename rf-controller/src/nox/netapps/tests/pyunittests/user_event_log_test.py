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

from nox.lib.netinet.netinet import *
from nox.lib.core     import Component
from nox.netapps.storage import Storage
from nox.netapps.user_event_log.pyuser_event_log import pyuser_event_log,LogEntry
from nox.netapps.bindings_storage.pybindings_storage import pybindings_storage,Name
from nox.netapps.tests import unittest
from twisted.internet.defer import Deferred
import time

pyunit = __import__('unittest')

class UserEventLogTestCase(unittest.NoxTestCase):
    """Python test component to verify that python bindings
       work for the User_Event_Log component"""

    def configure(self,configuration):
      pass

    def install(self):
      self.counter = 0
      self.uel = self.resolve(pyuser_event_log)
      # disable auto-clear of UEL
      self.uel.set_max_num_entries(-1) 
      self.store = self.resolve(Storage)
      self.bs = self.resolve(pybindings_storage) 
      self.trigger_count = 0
      self.debug = False
#      self.debug = True

    def getInterface(self):
        return str(UserEventLogTestCase)

    def run_test(self):
      if(self.debug) : print "starting User_Event_Log python test"
      # clear the user-event-log tables from past tests
      self.uel.clear(self.clear_cb)
      
      # create a deferred object that we will use
      # to tell the testing framework when we are done
      self.deferred = Deferred()
      return self.deferred

    def clear_cb(self): 

      
      d = self.store.put_table_trigger("user_event_log", True, 
                                        self.uel_trigger_callback)
      d.addCallback(self.set_trigger_callback)  
      d = self.store.put_table_trigger("user_event_log_names", True, 
                                        self.uel_trigger_callback)
      d.addCallback(self.set_trigger_callback)  
      d = self.store.put_table_trigger("bindings_location", True, 
                                        self.other_trigger_callback)
      d.addCallback(self.set_trigger_callback)  
      d = self.store.put_table_trigger("bindings_name", True, 
                                        self.other_trigger_callback)
      d.addCallback(self.set_trigger_callback)  

    def get_message(self): 
      return "log msg # " + str(self.counter) 

    def exit_test(self, success) :
      if(not success):
        assert(False)
      self.deferred.callback("done") # tell test-suite that we are done

    def set_trigger_callback(self, res): 
      result , tid = res 

      if (result[0] != 0): 
        print "put trigger failed: " + str(result[1]) 
        self.exit_test(False) 
        return 
      self.trigger_count += 1
      if self.trigger_count == 1:
        self.tid1 = tid 
      if self.trigger_count == 2: 
        self.tid2 = tid
      if self.trigger_count == 3: 
        self.tid3 = tid
      if self.trigger_count == 4: 
        self.tid4 = tid
        # insert a host binding so we can test get_logids_for_name()
        # control flow continues within trigger_callback
        self.bs.store_binding_state(datapathid.from_host(0),0,
                        ethernetaddr(0),0,"javelina", Name.HOST, False,0) 
    
    def other_trigger_callback(self, trigger_id, row, reason):
      if trigger_id[1] == "bindings_name":
        # user/host binding has been added, let's add a location binding
        self.bs.add_name_for_location(datapathid.from_host(0),0,
                              "of6k-5", Name.LOCATION)
      elif trigger_id[1] == "bindings_location":
        # location binding has been added, let's start logging
        self.log_once()
   
    # triggers for 'user_event_log' and 'user_event_log_names' tables
    # used to trigger the call to get_log_entry() once all rows have been
    # written as indicated by the wait_count value
    def uel_trigger_callback(self, trigger_id, row, reason):
      if trigger_id[1] == "user_event_log":
        assert(row['msg'] == self.get_message())
     
      if(self.debug): print "changed row: " + str(row) 
      self.wait_count -= 1
      if(self.wait_count == 0): 
        if(self.debug): print "doing get_log_entry id = " + str(row['logid']) 
        self.uel.get_log_entry(int(row['logid']),self.get_entry_callback)
    
    def log_once(self):
      if(self.debug): print "starting log_once"
      self.counter = self.counter + 1
      msg = self.get_message()
      if self.counter == 1:
        self.wait_count = 1
        self.uel.log_simple("UEL Test Python",LogEntry.INFO,msg) 
      elif  self.counter == 2: 
        self.wait_count = 5
        dpid = datapathid.from_host(0)
        self.uel.log("UEL Test Python",LogEntry.INFO,msg,
                src_location=(dpid,0), dst_location=(dpid,0))
      elif self.counter == 3: 
        mac = ethernetaddr(0) 
        self.wait_count = 5
        self.uel.log("UEL Test Python",LogEntry.INFO,msg, 
                                    src_mac = mac, dst_mac = mac) 
      elif self.counter == 4: 
        self.wait_count = 5
        self.uel.log("UEL Test Python",LogEntry.INFO,msg, src_ip=0, dst_ip=0) 
      elif self.counter == 5:
        self.wait_count = 2
        self.uel.log("UEL Test Python",LogEntry.INFO,msg, sh="javelina" )
      elif self.counter == 6: 
        self.wait_count = 2
        dpid = datapathid.from_host(0)
        self.uel.log("UEL Test Python",LogEntry.INFO,msg, set_dst_loc=(dpid,0))
      else:
        # done inserting.  now test get_logids_for_name() 
        self.uel.get_logids_for_name("javelina",Name.HOST,
                                            self.get_logids_cb)
    def get_logids_cb(self, logids):
        assert(len(logids) == 4) 

        # all done with tests.  remove triggers
        d = self.store.remove_trigger(self.tid1)
        d.addCallback(self.remove_trigger_callback)
        d = self.store.remove_trigger(self.tid2)
        d.addCallback(self.remove_trigger_callback)
        d = self.store.remove_trigger(self.tid3)
        d.addCallback(self.remove_trigger_callback)
        d = self.store.remove_trigger(self.tid4)
        d.addCallback(self.remove_trigger_callback)
        
    def get_entry_callback(self,logid,ts,app,level,msg,src_names,dst_names):
      if self.debug : 
        print "src_names:"
        for n in src_names:
          print n
        print "dst_names:"
        for n in dst_names:
          print n

      assert(msg == self.get_message())
      if(self.counter == 1):  
        assert(len(src_names) == 0)
        assert(len(dst_names) == 0)
      elif (self.counter == 5): # log with no key, just a sh=javelina 
        assert(len(src_names) == 1)
        assert(len(dst_names) == 0)
      elif (self.counter == 6): # log with no key, just a dloc=(0,0)
        assert(len(src_names) == 0)
        assert(len(dst_names) == 1)
      else: 
        assert(len(src_names) == 2)
        assert(len(dst_names) == 2)

      # let's do it again!
      self.log_once() 

    def remove_trigger_callback(self, result): 
      self.trigger_count -= 1
      if (result[0] != 0): 
        print "remove trigger failed: " + str(result[1]) 
        self.exit_test(False) 
        return 
      elif self.trigger_count == 0:
        self.exit_test(True) # success 

def suite(ctxt):
    suite = pyunit.TestSuite()
    suite.addTest(UserEventLogTestCase("run_test", ctxt))
    return suite
