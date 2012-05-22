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
# Prints all incoming packets in text to stdout 

import traceback
from nox.lib.packet import *
from nox.lib.core import *

from twisted.python import log

def print_packet(dp, inport, reason, len, bid, packet):
    # packet is already parsed (using code in nox/lib/packet) so just
    # print it and let __str__ do all the work
    print packet

class packetdump(Component):

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)

    def install(self):
        self.register_for_packet_in(print_packet)

    def getInterface(self):
        return str(packetdump)


def getFactory():
    class Factory:
        def instance(self, ctxt):
            return packetdump(ctxt)

    return Factory()
