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
from nox.netapps.tests import unittest

pyunit = __import__('unittest')

class ModTestCase(unittest.TestCase):     

    def getInterface(self):
        return str(ModTestCase)    

    def setUp(self):
        pass 
 
    def tearDown(self):
        pass
    
    def testSpeeds(self):
        """test event speeds"""
        from nox.lib.core import Event_type
        fast   = Event_type.allocate(Event_type.FAST); 
        medium = Event_type.allocate(Event_type.MEDIUM); 
        slow   = Event_type.allocate(Event_type.SLOW); 
        self.failUnlessEqual(fast.get_speed(), Event_type.FAST, 'Fast not equal');
        self.failUnlessEqual(medium.get_speed(), Event_type.MEDIUM, 'Medium not equal');
        self.failUnlessEqual(slow.get_speed(), Event_type.SLOW, 'Slow not equal');

    def testToInt(self):
        """test uniqueness of to_int"""    
        from nox.lib.core import Event_type
        allocated = []
        for i in range(0,256):
            new_type = Event_type.allocate(Event_type.FAST).to_int(); 
            if new_type in allocated:
                self.fail('Allocation failure')
            allocated.append(new_type)

def suite(ctxt):
    suite = pyunit.TestSuite()
    #suite.addTest(ModTestCase("testSpeeds", ctxt, name))
    #suite.addTest(ModTestCase("testToInt", ctxt, name))
    return suite
