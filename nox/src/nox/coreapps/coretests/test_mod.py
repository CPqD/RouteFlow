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

import os, sys

from nox.lib.core import *
from nox.coreapps.testharness.testdefs import *

class ModTestCase(Component):

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)

    def configure(self, configuration):
        pass

    def getInterface(self):
        return str(ModTestCase)    

    def install(self):
        self.register_for_bootstrap_complete(self.bootstrap_complete_callback)

    def bootstrap_complete_callback(self, *args):
        self.testImport()
        self.testAttributes()

        sys.stdout.flush() # XXX handle in component::exit 
        os._exit(0)  # XXX push to component 


    
    def testImport(self):
        """attempt to load nox.lib.core module"""
        try:
            import nox.lib.core
        except Exception, e:    
            nox_test_assert(0, "importing nox core") 
            

    def testAttributes(self):
        """verify that core has the appropriate set of attributes"""
        import nox.lib.core
        attributes = ['Component','IN_PORT']
        for attr in attributes:                
            nox_test_assert(hasattr(nox.lib.core, attr),
                "nox.lib.core module module ")

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return ModTestCase(ctxt)

    return Factory()
