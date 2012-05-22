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
from twisted.internet import reactor, defer
from twisted.python import failure
from nox.netapps.tests import unittest

pyunit = __import__('unittest')

class ControllerTestCase(unittest.NoxTestCase):
    d1 = defer.Deferred()

    def getInterface(self):
        return str(ControllerTestCase)

    def configure(self, configuration):
        pass

    def setUp(self):
        pass 
 
    def tearDown(self):
        pass
    
    def testPost_cb(self):
        """test post_cb"""
        d = defer.Deferred()
        def foo():
            d.callback(None)
            
        reactor.callLater(0, foo)
        return d

    def testPost_cb_tv(self):
        """test post_cb with delay"""

        d = defer.Deferred()
        def foo():
            d.callback(None)

        reactor.callLater(0.01, foo)
        return d
        
def suite(ctxt):
    suite = pyunit.TestSuite()
    suite.addTest(ControllerTestCase("testPost_cb", ctxt))
    suite.addTest(ControllerTestCase("testPost_cb_tv", ctxt))
    return suite
