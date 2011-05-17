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
# ----------------------------------------------------------------------
#
# This app just drops to the python interpreter.  Useful for debugging
#
from nox.lib.core import *
from nox.lib.netinet.netinet import datapathid
from nox.netapps.topology.pytopology import pytopology 

class pytopology_test(Component):

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)

    def timer(self):
        val = datapathid.from_host(0)
        print self.pytop.is_internal(val, 0)
        self.post_callback(1, self.timer)

    def install(self):
        self.pytop = self.resolve(pytopology)
        self.post_callback(1, self.timer)

    def getInterface(self):
        return str(pytopology_test)


def getFactory():
    class Factory:
        def instance(self, ctxt):
            return pytopology_test(ctxt)

    return Factory()
