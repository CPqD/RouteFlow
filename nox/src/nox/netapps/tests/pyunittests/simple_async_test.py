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
from nox.netapps.tests import unittest
from twisted.internet.defer import Deferred

pyunit = __import__('unittest')

class SimpleAsyncTestCase(unittest.NoxTestCase):
    """This is a demonstration class to show you how to 
       do python unit testing with asynchronous operations
       NOTE: the test is currently disabled, as we don't actually
       want it to length the time to run all tests"""


    def configure(self,configuration):
      pass

    def install(self):
      pass

    def getInterface(self):
        return str(SimpleAsyncTestCase)

    def run_test(self): 

      # create a deferred object that we will use
      # to tell the testing framework when we are done
      self.deferred = Deferred()
      
      # do something which spawns a callback 
      # at a later point
      self.post_callback(5, self.timer_callback) 
      print "Hey, this test is just going to stall for a while"

      # return a deferred object.  the testing 
      # framework will block on this until 
      return self.deferred

    def timer_callback(self):

      # wooohooo, i have made our unit tests
      # five seconds longer.  now i am done, so
      # use the deferral to let the testing 
      # framework know that

      self.deferred.callback("all done!")
      print "Ok, done stalling!"

def suite(ctxt):
    suite = pyunit.TestSuite()
    # by default, this test is disabled b/c it just wastes time!
    #suite.addTest(SimpleAsyncTestCase("run_test", ctxt, name))
    return suite
