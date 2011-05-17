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
from nox.coreapps.testharness.testdefs import *


icmp_echo_str = \
"""\
\x00\x1a\xa0\x8f\x9a\x95\x00\xa0\xcc\x28\xa9\x94\x08\x00\x45\x00\
\x00\x54\x00\x00\x40\x00\x40\x01\xb7\x39\xc0\xa8\x01\x0c\xc0\xa8\
\x01\x13\x08\x00\x2b\xde\x6d\x73\x00\x01\x72\xf9\xb4\x47\x41\x69\
\x0b\x00\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\
\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\x20\x21\x22\x23\x24\x25\
\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34\x35\
\x36\x37\
"""

icmp_unreach_str = \
"""\
\x00\xa0\xcc\x28\xa9\x94\x00\x1a\x92\x40\xac\x05\x08\x00\x45\xc0\
\x00\x38\x13\x77\x00\x00\x40\x01\xe3\x23\xc0\xa8\x01\x0e\xc0\xa8\
\x01\x0c\x03\x03\x80\x81\x00\x00\x00\x00\x45\x00\x00\x1c\x8e\x60\
\x00\x00\x40\x11\x69\x06\xc0\xa8\x01\x0c\xc0\xa8\x01\x0e\x07\xd6\
\x14\xca\x00\x08\x5f\xd3\
"""

def testICMPEcho():
    eth = ethernet(icmp_echo_str)
    icmph = eth.find('icmp')
    nox_test_assert(icmph, 'icmp parse')
    nox_test_assert(icmph.type == 8)
    nox_test_assert(icmph.csum == 0x2bde)
    echoh = eth.find('echo')
    nox_test_assert(echoh)
    nox_test_assert(echoh.id  == 0x6d73)
    nox_test_assert(echoh.seq == 1)

def testICMPEchoConstruct():
    eth = ethernet(icmp_echo_str)
    nox_test_assert(eth.tostring() == icmp_echo_str)

def testICMPUnreach():
    eth = ethernet(icmp_unreach_str)
    icmph = eth.find('icmp')
    nox_test_assert(icmph)
    unreach = eth.find('unreach')
    nox_test_assert(unreach)
    iph = unreach.find('ipv4')
    nox_test_assert(iph)
    udph = unreach.find('udp')
    nox_test_assert(udph)

    nox_test_assert(icmph.type == 3)
    nox_test_assert(icmph.code == 3)
    nox_test_assert(icmph.csum == 0x8081)

    nox_test_assert(ip_to_str(iph.srcip) == "192.168.1.12")
    nox_test_assert(ip_to_str(iph.dstip) == "192.168.1.14")

    nox_test_assert(udph.srcport == 2006)
    nox_test_assert(udph.dstport == 5322)

def testICMPUnreachConstruct():
    eth = ethernet(icmp_unreach_str)
    nox_test_assert(eth.tostring() == icmp_unreach_str)
