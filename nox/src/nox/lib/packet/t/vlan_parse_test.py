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
from nox.coreapps.testharness.testdefs import *


vlan_str = \
"""\
\xff\xff\xff\xff\xff\xff\x00\x19\xe1\xe2\x03\x53\x81\x00\xc2\x3a\
\x08\x06\x00\x01\x08\x00\x06\x04\x00\x01\x00\x19\xe1\xe2\x03\x53\
\xac\x1c\x40\x7e\x00\x19\xe1\xe2\x03\x53\xac\x1c\x40\x01\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
"""

vlan_hdr = """\xc2\x3a\x08\x06"""

no_vlan_str = \
"""\
\xff\xff\xff\xff\xff\xff\x00\x19\xe1\xe2\x03\x53\x08\x06\
\x00\x01\x08\x00\x06\x04\x00\x01\x00\x19\xe1\xe2\x03\x53\
\xac\x1c\x40\x7e\x00\x19\xe1\xe2\x03\x53\xac\x1c\x40\x01\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
"""

def testVlan():
    eth = ethernet(vlan_str)
    vlanh = eth.find('vlan') 
    nox_test_assert(vlanh, 'vlan parse')
    nox_test_assert(eth.type       == ethernet.VLAN_TYPE)
    nox_test_assert(vlanh.eth_type == ethernet.ARP_TYPE)
    nox_test_assert(vlanh.pcp      == 6)
    nox_test_assert(vlanh.cfi      == 0)
    nox_test_assert(vlanh.id       == 570)

def testVlanARP():
    eth = ethernet(vlan_str)
    vlanh = eth.find('vlan') 
    arph  = eth.find('arp') 
    nox_test_assert(vlanh)
    nox_test_assert(arph)
    nox_test_assert(arph.hwtype    == arp.HW_TYPE_ETHERNET)
    nox_test_assert(arph.prototype == arp.PROTO_TYPE_IP)
    nox_test_assert(arph.hwlen     == 6)
    nox_test_assert(arph.protolen  == 4)
    nox_test_assert(array_to_octstr(arph.hwsrc) == "00:19:e1:e2:03:53")
    nox_test_assert(array_to_octstr(arph.hwdst) == "00:19:e1:e2:03:53")
    nox_test_assert(arph.opcode == 1)
    nox_test_assert(ip_to_str(arph.protosrc) == "172.28.64.126")
    nox_test_assert(ip_to_str(arph.protodst) == "172.28.64.1")

def testVlanRemove():
    eth = ethernet(vlan_str)
    vlanh = eth.find('vlan') 
    eth.type = vlanh.eth_type
    eth.set_payload(vlanh.next)
    nox_test_assert(len(eth.tostring()) == (len(vlan_str) - 4))
    nox_test_assert(eth.tostring() == no_vlan_str)

def testVlanAdd():
    eth = ethernet(vlan_str)
    vlanh = eth.find('vlan') 
    arph  = eth.find('arp') 
    nox_test_assert(arph)
    eth.type = vlanh.eth_type
    eth.set_payload(vlanh.next)
    nox_test_assert(len(eth.tostring()) == (len(vlan_str) - 4))
    nox_test_assert(eth.tostring() == no_vlan_str)
    vlanh = vlan(vlan_hdr)
    vlanh.set_payload(arph)
    eth.set_payload(vlanh)
    eth.type = ethernet.VLAN_TYPE
    nox_test_assert(eth.tostring() == vlan_str)
