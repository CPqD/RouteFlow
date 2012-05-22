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
import array
from nox.lib.packet import *
from nox.lib.packet.ethernet import *
from nox.lib.packet.packet_utils import *
from nox.netapps.tests import unittest

pyunit = __import__('unittest')

class UtilTestCase(unittest.NoxTestCase):

    def getInterface(self):
        return str(UtilTestCase)    

    def setUp(self):
        pass 
 
    def tearDown(self):
        pass
    
    def testUtils(self):

        # ethernet constants
        eth_any = array_to_octstr(array.array('B', ETHER_ANY))
        assert(eth_any == '00:00:00:00:00:00')
        eth_any = array_to_octstr(array.array('B', ETHER_BROADCAST))
        assert(eth_any == 'ff:ff:ff:ff:ff:ff')
        eth_any = array_to_octstr(array.array('B', BRIDGE_GROUP_ADDRESS))
        assert(eth_any == '01:80:c2:00:00:00')

        # ethernet address conversion methods
        assert(octstr_to_array(array_to_octstr(array.array('B',
        ETHER_ANY))).tostring() == ETHER_ANY)
        assert(octstr_to_array(array_to_octstr(array.array('B',
        ETHER_BROADCAST))).tostring() == ETHER_BROADCAST)
        assert(longlong_to_octstr(0xcafedeadbeef)[6:] == "ca:fe:de:ad:be:ef")
        assert(mac_to_int('\xca\xfe\xde\xad\xbe\xef') == 0xcafedeadbeef)

        # IP address handling methods
        assert(ip_to_str(0xffffffff) == "255.255.255.255")
        ip_bcast = array.array('B', '\xff\xff\xff\xff')
        assert(array_to_ipstr(ip_bcast) == "255.255.255.255")
        assert(ipstr_to_int(ip_to_str(0xdeadbeef)) == 0xdeadbeef)

def suite(ctxt):
    suite = pyunit.TestSuite()
    suite.addTest(UtilTestCase("testUtils", ctxt))
    return suite

