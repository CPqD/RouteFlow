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
from nox.lib.packet.ethernet import *
from nox.lib.packet.ipv4 import *
from nox.coreapps.testharness.testdefs import *


def testIPv4():
    eth = ethernet()
    eth.dst  = '\xca\xfe\xde\xad\xbe\xef'
    eth.src  = '\xca\xfe\xde\xad\xd0\xd0'
    eth.type = ethernet.IP_TYPE 
    ipstr = '\x45\x00\x00\x34\x1c\x1b\x40\x00\x40\x06\x34\xdf\xab\x40\x4a\x31\xac\x18\x48\x40'
    iparr = array('B',ipstr)
    iph = ethernet(array('B', eth.tostring()) + iparr).find('ipv4')
    nox_test_assert(iph, 'ipv4 parse')
    nox_test_assert(iph.parsed  )
    nox_test_assert(iph.v     == 4)
    nox_test_assert(iph.hl    == 5)
    nox_test_assert(iph.tos   == 0)
    nox_test_assert(iph.iplen == 52)
    nox_test_assert(iph.id    == 0x1c1b)
    nox_test_assert(iph.flags == 0x02)
    nox_test_assert(iph.flags == ipv4.DF_FLAG)
    nox_test_assert(iph.frag  == 0)
    nox_test_assert(iph.ttl   == 64)
    nox_test_assert(iph.protocol == 0x06)
    nox_test_assert(iph.csum  == 0x34df)
    nox_test_assert(iph.checksum()  == iph.csum)
    nox_test_assert(ip_to_str(iph.srcip) == "171.64.74.49")
    nox_test_assert(ip_to_str(iph.dstip) == "172.24.72.64")

def testIPv4Construct():
    ipstr = '\x45\x00\x00\x34\x1c\x1b\x40\x00\x40\x06\x34\xdf\xab\x40\x4a\x31\xac\x18\x48\x40'
    iph = ipv4(ipstr) 
    nox_test_assert(iph.tostring() == ipstr)

def testIPv4Edge():
    eth = ethernet()
    eth.dst  = '\xca\xfe\xde\xad\xbe\xef'
    eth.src  = '\xca\xfe\xde\xad\xd0\xd0'
    eth.type = ethernet.IP_TYPE 

    ipstr = '\x46\x00\x00\x34\x1c\x1b\x40\x00\x40\x06\x34\xdf\xab\x40\x4a\x31\xac\x18\x48\x40'
    iparr = array('B',ipstr)
    iph = ethernet(array('B', eth.tostring()) + iparr).find('ipv4')
    nox_test_assert(not iph)

    ipstr = '\x45\x00\x00\x34\x1c\x1b\x40\x00\x40\x06\x34\xdf\xab\x40\x4a\x310'
    iparr = array('B',ipstr)
    iph = ethernet(array('B', eth.tostring()) + iparr).find('ipv4')
    nox_test_assert(not iph)
