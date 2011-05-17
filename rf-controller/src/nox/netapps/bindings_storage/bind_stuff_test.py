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
from nox.netapps.bindings_storage.pybindings_storage import pybindings_storage,Name
from nox.netapps.tests import unittest
from twisted.internet.defer import Deferred
import time
import sys


class BindStuffTest(Component):
    """ Simple component that creates some bindings to 
        to test web-services """

    def configure(self,configuration):
      pass

    def install(self):
      self.bs = self.resolve(pybindings_storage) 
      self.bs.store_binding_state(datapathid.from_host(0),0,
                ethernetaddr(0),0,"javelina", Name.HOST, False,0) 
      self.bs.store_binding_state(datapathid.from_host(0),0,
                ethernetaddr(0),0,"dan", Name.USER, False,0) 
      
      self.bs.store_binding_state(datapathid.from_host(1),1,
                ethernetaddr(1),1,"miwok", Name.HOST, False,0) 
      self.bs.store_binding_state(datapathid.from_host(1),1,
                ethernetaddr(1),1,"natasha", Name.USER, False,0) 

      
      self.bs.add_name_for_location(datapathid.from_host(0),0,
                                        "dan's desk", Name.LOCATION);
      self.bs.add_name_for_location(datapathid.from_host(1),1,
                                        "natasha's desk", Name.LOCATION);
      self.bs.add_name_for_location(datapathid.from_host(0),0,
                                        "switch #0", Name.SWITCH);
      self.bs.add_name_for_location(datapathid.from_host(1),1,
                                        "switch #1", Name.SWITCH);


    def getInterface(self):
        return str(BindStuffTest)
      
  
def getFactory():
        class Factory():
            def instance(self, context):
                return BindStuffTest(context)

        return Factory()
