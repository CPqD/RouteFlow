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

class BSLinkTestCase(unittest.NoxTestCase):
    """ Bindings_Storage python tests for Link functionality"""

    def configure(self,configuration):
      pass

    def install(self):
      self.bs = self.resolve(pybindings_storage) 
      self.store = self.resolve(Storage)
      #self.debug = True
      self.debug = False

    def getInterface(self):
        return str(BSLinkTestCase)
      
    def check_wait(self): 
      self.wait_count = self.wait_count - 1
      if self.wait_count == 0: 
        self.next_fn() 

    def links_cb(self, links):
      if(self.debug): 
        print "expected links: " 
        for n in self.expected_links: 
          print "(%s,%s,%s,%s)" % (n[0].as_host(),n[1],n[2].as_host(),n[3]) 
        print "got links: " 
        for n in links: 
          print str(n) 
      
      assert(len(links) == len(self.expected_links))

      for n in links: 
        found = False
        for e in self.expected_links:
          # links are now uni-directional, so order matters
          match1 = e[0].as_host() == n[0] and e[1] == n[1] and e[2].as_host() == n[2] and e[3] == n[3] 
          if match1: 
            found = True
            break
        assert(found)

      self.check_wait() 


    def clear_cb(self):
        self.num_triggers_inserted = 0 
        d = self.store.put_table_trigger("bindings_link", True, 
                                            self.trigger_callback)
        d.addCallback(self.trigger_inserted_callback)  
      
    def trigger_inserted_callback(self, res): 
      result , tid = res 

      if (result[0] != 0): 
        print "put trigger failed: " + str(result[1]) 
        self.exit_test(False) 
        return 
      self.tid = tid  
      self.run_test1() 

    def trigger_callback(self, trigger_id, row, reason):
      self.check_wait() 

    def run_test1(self):
      self.dpid1 = datapathid.from_host(1 << 33) 
      self.dpid2 = datapathid.from_host(2 << 33) 
      self.dpid3 = datapathid.from_host(3 << 33) 
      self.dpid4 = datapathid.from_host(4 << 33)

      if(self.debug): print "\nadding link (1,10,2,10) " 
      self.bs.add_link(self.dpid1,10,self.dpid2,10) 
      
      if(self.debug): print "adding link (1,1,3,0) " 
      self.bs.add_link(self.dpid1,1,self.dpid3,0) 
      
      if(self.debug): print "adding link (4,10,1,1) " 
      self.bs.add_link(self.dpid4,10,self.dpid1,1) 
      
      if(self.debug): print "adding link (2,15,4,9) " 
      self.bs.add_link(self.dpid2,15,self.dpid4,9) 
      
      self.wait_count = 4
      self.next_fn = self.run_test2
    
    def run_test2(self):

      if(self.debug): print "getting all links"
      self.expected_links = [ ]
      self.expected_links.append((self.dpid1,10,self.dpid2,10))
      self.expected_links.append((self.dpid1,1,self.dpid3,0))
      self.expected_links.append((self.dpid4,10,self.dpid1,1))
      self.expected_links.append((self.dpid2,15,self.dpid4,9))

      self.bs.get_all_links(self.links_cb) 

      self.next_fn = self.run_test3
      self.wait_count = 1
    
    def run_test3(self):

      if(self.debug): print "getting links for dpid = %s" % (1 << 33)
      self.expected_links = [ ]
      self.expected_links.append((self.dpid1,10,self.dpid2,10))
      self.expected_links.append((self.dpid1,1,self.dpid3,0))
      self.expected_links.append((self.dpid4,10,self.dpid1,1))

      self.bs.get_switch_links(self.dpid1,self.links_cb) 

      self.next_fn = self.run_test4
      self.wait_count = 1
    
    def run_test4(self):

      if(self.debug): print "getting links for dpid = %s port = %d" % (4 << 33,10)
      self.expected_links = [ ]
      self.expected_links.append((self.dpid4,10,self.dpid1,1))

      self.bs.get_links(self.dpid4, 10, self.links_cb) 

      self.next_fn = self.run_test5
      self.wait_count = 1
    
    def run_test5(self):

      if(self.debug): print "getting links for non-existent dpid"
      dpid = datapathid.from_host(99)
      self.expected_links = [ ]
      self.bs.get_switch_links(dpid, self.links_cb) 

      self.next_fn = self.run_test6
      self.wait_count = 1
    
    def run_test6(self):

      if(self.debug): print "removing link (1,1,3,0) " 
      self.expected_links = [ ]
      self.bs.remove_link(self.dpid1,1,self.dpid3,0)

      self.next_fn = self.run_test7
      self.wait_count = 1
    
    def run_test7(self):

      if(self.debug): print "getting links for (3,0). " \
                            "should be empty after remove"
      self.expected_links = [ ]
      self.bs.get_links(self.dpid3, 0, self.links_cb) 

      self.next_fn = self.tear_down
      self.wait_count = 1

    def tear_down(self): 
      d = self.store.remove_trigger(self.tid)
      d.addCallback(self.remove_trigger_callback)
    
    def start(self): 
      self.bs.clear_links() 
      self.deferred = Deferred()
      self.clear_cb()
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
      
      self.exit_test(True) # success 


def suite(ctxt):
    suite = pyunit.TestSuite()
    suite.addTest(BSLinkTestCase("start", ctxt))
    return suite
