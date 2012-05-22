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
from nox.lib.packet.tcp import *
from nox.coreapps.testharness.testdefs import *


large_tcp = \
"""\x00\x07\xe9\x4c\xa9\xeb\x00\xd0\x05\x5d\x24\x00\x08\x00\x45\x00\
\x01\xdd\xc0\xe4\x40\x00\x35\x06\x10\xb2\x45\x6b\x37\xa8\xab\x40\
\x4a\x31\xc0\x1f\x00\x50\x4e\xe2\xa8\x87\x16\xa6\xc2\x13\x80\x18\
\x01\x6d\xbc\xff\x00\x00\x01\x01\x08\x0a\x03\xe0\x59\x9b\xf3\xb5\
\xb3\xce\x47\x45\x54\x20\x2f\x20\x48\x54\x54\x50\x2f\x31\x2e\x31\
\x0d\x0a\x48\x6f\x73\x74\x3a\x20\x6e\x69\x74\x79\x2e\x73\x74\x61\
\x6e\x66\x6f\x72\x64\x2e\x65\x64\x75\x0d\x0a\x55\x73\x65\x72\x2d\
\x41\x67\x65\x6e\x74\x3a\x20\x4d\x6f\x7a\x69\x6c\x6c\x61\x2f\x35\
\x2e\x30\x20\x28\x58\x31\x31\x3b\x20\x55\x3b\x20\x4c\x69\x6e\x75\
\x78\x20\x69\x36\x38\x36\x3b\x20\x65\x6e\x2d\x55\x53\x3b\x20\x72\
\x76\x3a\x31\x2e\x38\x2e\x31\x2e\x31\x31\x29\x20\x47\x65\x63\x6b\
\x6f\x2f\x32\x30\x30\x37\x31\x31\x32\x38\x20\x49\x63\x65\x77\x65\
\x61\x73\x65\x6c\x2f\x32\x2e\x30\x2e\x30\x2e\x31\x31\x20\x28\x44\
\x65\x62\x69\x61\x6e\x2d\x32\x2e\x30\x2e\x30\x2e\x31\x31\x2d\x31\
\x29\x0d\x0a\x41\x63\x63\x65\x70\x74\x3a\x20\x74\x65\x78\x74\x2f\
\x78\x6d\x6c\x2c\x61\x70\x70\x6c\x69\x63\x61\x74\x69\x6f\x6e\x2f\
\x78\x6d\x6c\x2c\x61\x70\x70\x6c\x69\x63\x61\x74\x69\x6f\x6e\x2f\
\x78\x68\x74\x6d\x6c\x2b\x78\x6d\x6c\x2c\x74\x65\x78\x74\x2f\x68\
\x74\x6d\x6c\x3b\x71\x3d\x30\x2e\x39\x2c\x74\x65\x78\x74\x2f\x70\
\x6c\x61\x69\x6e\x3b\x71\x3d\x30\x2e\x38\x2c\x69\x6d\x61\x67\x65\
\x2f\x70\x6e\x67\x2c\x2a\x2f\x2a\x3b\x71\x3d\x30\x2e\x35\x0d\x0a\
\x41\x63\x63\x65\x70\x74\x2d\x4c\x61\x6e\x67\x75\x61\x67\x65\x3a\
\x20\x65\x6e\x2d\x75\x73\x2c\x65\x6e\x3b\x71\x3d\x30\x2e\x35\x0d\
\x0a\x41\x63\x63\x65\x70\x74\x2d\x45\x6e\x63\x6f\x64\x69\x6e\x67\
\x3a\x20\x67\x7a\x69\x70\x2c\x64\x65\x66\x6c\x61\x74\x65\x0d\x0a\
\x41\x63\x63\x65\x70\x74\x2d\x43\x68\x61\x72\x73\x65\x74\x3a\x20\
\x49\x53\x4f\x2d\x38\x38\x35\x39\x2d\x31\x2c\x75\x74\x66\x2d\x38\
\x3b\x71\x3d\x30\x2e\x37\x2c\x2a\x3b\x71\x3d\x30\x2e\x37\x0d\x0a\
\x4b\x65\x65\x70\x2d\x41\x6c\x69\x76\x65\x3a\x20\x33\x30\x30\x0d\
\x0a\x43\x6f\x6e\x6e\x65\x63\x74\x69\x6f\x6e\x3a\x20\x6b\x65\x65\
\x70\x2d\x61\x6c\x69\x76\x65\x0d\x0a\x0d\x0a"""


def testTCP():
    eth = ethernet()
    eth.dst  = '\xca\xfe\xde\xad\xbe\xef'
    eth.src  = '\xca\xfe\xde\xad\xd0\xd0'
    eth.type = ethernet.IP_TYPE 
    ipstr = '\x45\x00\x00\x34\x1c\x1b\x40\x00\x40\x06\x34\xdf\xab\x40\x4a\x31\xac\x18\x48\x40'
    iparr = array('B',ipstr)
    tcpstr = '\x03\x20\x08\x01\xd2\x9d\x61\x67\x57\x37\xc3\x0f\x80\x10\x7e\x82\xf2\x6e\x00\x00\x01\x01\x08\x0a\xf3\xb5\x9c\x36\x18\xa2\x1a\x06'
    tcparr = array('B',tcpstr)
    tcph = ethernet(array('B', eth.tostring()) + iparr + tcparr).find('tcp')
    nox_test_assert(tcph, 'tcp parse')
    nox_test_assert(tcph.parsed)
    nox_test_assert(tcph.srcport == 800)
    nox_test_assert(tcph.dstport == 2049)
    nox_test_assert(tcph.seq     == 0xd29d6167)
    nox_test_assert(tcph.ack     == 0x5737c30f)
    nox_test_assert(tcph.off     == 8)
    nox_test_assert(tcph.res     == 0)
    nox_test_assert(not tcph.flags & tcp.CWR)
    nox_test_assert(not tcph.flags & tcp.ECN)
    nox_test_assert(not tcph.flags & tcp.URG)
    nox_test_assert(tcph.flags & tcp.ACK)
    nox_test_assert(not tcph.flags & tcp.PUSH)
    nox_test_assert(not tcph.flags & tcp.RST)
    nox_test_assert(not tcph.flags & tcp.SYN)
    nox_test_assert(not tcph.flags & tcp.FIN)
    nox_test_assert(tcph.win == 32386)
    nox_test_assert(tcph.csum == 0xf26e)
    nox_test_assert(tcph.checksum() == tcph.csum)
    nox_test_assert(len(tcph.options) == 3)                            
    nox_test_assert(tcph.options[0].type == tcp_opt.NOP)
    nox_test_assert(tcph.options[1].type == tcp_opt.NOP)
    nox_test_assert(tcph.options[2].type == tcp_opt.TSOPT)
    nox_test_assert(tcph.options[2].val[0] == 4088765494)

def checkFullTcpHeader(tcph):    
    nox_test_assert(tcph.srcport == 49183)
    nox_test_assert(tcph.dstport == 80)
    nox_test_assert(tcph.off     == 8)
    nox_test_assert(tcph.res     == 0)
    nox_test_assert(not tcph.flags & tcp.CWR)
    nox_test_assert(not tcph.flags & tcp.ECN)
    nox_test_assert(not tcph.flags & tcp.URG)
    nox_test_assert(tcph.flags & tcp.ACK)
    nox_test_assert(tcph.flags & tcp.PUSH)
    nox_test_assert(not tcph.flags & tcp.RST)
    nox_test_assert(not tcph.flags & tcp.SYN)
    nox_test_assert(not tcph.flags & tcp.FIN)
    nox_test_assert(tcph.win == 0x16d)
    nox_test_assert(tcph.csum == 0xbcff)
    nox_test_assert(len(tcph.options) == 3)
    nox_test_assert(tcph.next[:14] == "GET / HTTP/1.1")
    nox_test_assert(tcph.next.find("User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.11) Gecko/20071128 Iceweasel/2.0.0.11 (Debian-2.0.0.11-1)") != -1)

def fullTCPPacket():
    eth = ethernet(array('B',large_tcp))   
    tcph = eth.find('tcp')
    iph  = eth.find('ipv4')
    nox_test_assert(tcph)
    nox_test_assert(iph)
    checkFullTcpHeader(tcph)
    tcp2 = tcp(arr=tcph.tostring(),prev=None)
    nox_test_assert(tcph.checksum() == tcph.csum)
    checkFullTcpHeader(tcp2)

    # check out some edge conditions
    tcp2 = tcp(arr=tcph.tostring()[:12])
    nox_test_assert(not tcp2.parsed)
    tcph.off = 0
    tcp2 = tcp(arr=tcph.tostring())
    nox_test_assert(not tcp2.parsed)
    tcph.off = 0xff 
    tcp2 = tcp(arr=tcph.tostring())
    tcp2 = tcp(arr=tcph.tostring()[:21])
    nox_test_assert(not tcp2.parsed)
    tcph.off = 0x06 
    tcp2 = tcp(arr=tcph.tostring()[:25])
    nox_test_assert(not tcp2.parsed)
