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


ether_str = \
"""\
\x00\x00\x0c\x07\xac\x4a\x00\x07\xe9\x4c\xa9\xeb\x08\x00\x45\x00\
\x00\x34\x1c\x1b\x40\x00\x40\x06\x34\xdf\xab\x40\x4a\x31\xac\x18\
\x48\x40\x03\x20\x08\x01\xd2\x9d\x61\x67\x57\x37\xc3\x0f\x80\x10\
\x7e\x82\xf2\x6e\x00\x00\x01\x01\x08\x0a\xf3\xb5\x9c\x36\x18\xa2\
\x1a\x06"""

ip_str = \
"""\
\x45\x00\x00\x34\x1c\x1b\x40\x00\x40\x06\x34\xdf\xab\x40\x4a\x31\
\xac\x18\x48\x40\x03\x20\x08\x01\xd2\x9d\x61\x67\x57\x37\xc3\x0f\
\x80\x10\x7e\x82\xf2\x6e\x00\x00\x01\x01\x08\x0a\xf3\xb5\x9c\x36\
\x18\xa2\x1a\x06"""


def testEthernet():
    eth = ethernet(array('B','\x00'))
    nox_test_assert(not eth.parsed, 'ethernet parse')
    pstr = '\xca\xfe\xde\xad\xbe\xef\xca\xfe\xde\xad\xd0\xd0\xff\xff'
    parr = array('B', pstr)
    eth = ethernet(parr)
    nox_test_assert(eth.parsed)
    nox_test_assert(eth.tostring() == pstr)
    parr = array('B', pstr + '\x00')
    eth = ethernet(parr)
    nox_test_assert(eth.parsed)
    for i in xrange(0,255):
        nstr = '%s%c' % (pstr,i)
        parr = array('B',nstr)
        eth = ethernet(parr)
        nox_test_assert(eth.parse)
    eth = ethernet()
    eth.dst  = '\xca\xfe\xde\xad\xbe\xef'
    eth.src  = '\xca\xfe\xde\xad\xd0\xd0'
    eth.type = 0xffff 
    nox_test_assert(eth.tostring() == pstr, 'to string')

def testEthernetConstruct():
    eth = ethernet(ether_str)
    nox_test_assert(eth)
    nox_test_assert(array_to_octstr(eth.dst) == '00:00:0c:07:ac:4a')
    nox_test_assert(array_to_octstr(eth.src) == '00:07:e9:4c:a9:eb')
    nox_test_assert(eth.type == ethernet.IP_TYPE)
    nox_test_assert(eth.find('ipv4'))
    nox_test_assert(eth.tostring() == ether_str)

    eth = ethernet()
    eth.dst = octstr_to_array('00:00:0c:07:ac:4a')
    eth.src = octstr_to_array('00:07:e9:4c:a9:eb')
    eth.type = ethernet.IP_TYPE
    eth.set_payload(ip_str)
    nox_test_assert(eth.tostring() == ether_str)

    iph = ipv4(arr=ip_str)
    eth.set_payload(iph)
    nox_test_assert(eth.next.tostring() == iph.tostring())
    nox_test_assert(eth.tostring() == ether_str)
