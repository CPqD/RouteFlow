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

class ModTestCase(unittest.NoxTestCase):

    def getInterface(self):
        return str(ModTestCase)    

    def setUp(self):
        pass 
 
    def tearDown(self):
        pass
    
    def testImport(self):
        """attempt to load nox.lib.core module"""
        import nox.lib.core

    def testAttributes(self):
        """verify that core has the appropriate set of attributes"""
        import nox.lib.core
        attributes = ['Component','IN_PORT']
        for attr in attributes:                
            if not hasattr(nox.lib.core, attr):
                self.fail("nox.lib.core module module " + 
                          "missing an attribute " + attr)

def suite(ctxt):
    suite = pyunit.TestSuite()
    suite.addTest(ModTestCase("testImport", ctxt))
    suite.addTest(ModTestCase("testAttributes", ctxt))
    return suite
