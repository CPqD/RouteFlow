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

# Component used by test framework to indicate NOX has finished bootstrap.

import logging

from nox.coreapps.pyrt.pycomponent import *
from nox.lib.core import *

lg = logging.getLogger('initindicator')

class initindicator(Component):
    def __init__(self, ctxt):
        Component.__init__(self, ctxt)
        self.file=None

    def bootstrap_complete(self, *args):
        lg.info("NOX has finished bootstrap.")
        if self.file != None:
            f = open(self.file, "w")
            f.write("Bootstrap has completed.\n")
            f.close()

    def configure(self, conf):
        for a in conf['arguments']:
            k, v = a.split("=")
            if k == "file":
                self.file = v
            else:
                lg.error("initindicator only supports a 'file' argument.")

    def install(self):
        self.register_for_bootstrap_complete(self.bootstrap_complete)

    def getInterface(self):
        return str(initindicator)

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return initindicator(ctxt)

    return Factory()
