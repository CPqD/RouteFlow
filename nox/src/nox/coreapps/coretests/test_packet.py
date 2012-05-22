
from nox.lib.core import *
from nox.coreapps.testharness.testdefs import *

from nox.lib.packet.t.dhcp_parse_test import test_fullDHCPPacket
from nox.lib.packet.t.dns_parse_test  import test_dns_1 
from nox.lib.packet.t.dns_parse_test  import test_dns_2 
from nox.lib.packet.t.eap_parse_test  import testEAPOL 

from nox.lib.packet.t.ethernet_parse_test import testEthernet 
from nox.lib.packet.t.ethernet_parse_test import testEthernetConstruct 

from nox.lib.packet.t.icmp_parse_test import testICMPEcho
from nox.lib.packet.t.icmp_parse_test import testICMPEchoConstruct
from nox.lib.packet.t.icmp_parse_test import testICMPUnreach
from nox.lib.packet.t.icmp_parse_test import testICMPUnreachConstruct

from nox.lib.packet.t.ipv4_parse_test import testIPv4Edge
from nox.lib.packet.t.ipv4_parse_test import testIPv4Construct
from nox.lib.packet.t.ipv4_parse_test import testIPv4

from nox.lib.packet.t.lldp_parse_test import testLLDP
from nox.lib.packet.t.lldp_parse_test import testLLDPConstruct
from nox.lib.packet.t.lldp_parse_test import testLLDPConstruct2

from nox.lib.packet.t.tcp_parse_test  import testTCP 
from nox.lib.packet.t.tcp_parse_test  import fullTCPPacket 

from nox.lib.packet.t.udp_parse_test  import fullUDPPacket

from nox.lib.packet.t.vlan_parse_test  import testVlan 
from nox.lib.packet.t.vlan_parse_test  import testVlanAdd
from nox.lib.packet.t.vlan_parse_test  import testVlanARP
from nox.lib.packet.t.vlan_parse_test  import testVlanRemove

import logging

import os, sys

lg = logging.getLogger('test_packet')

class test_packet(Component):

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)

    def configure(self, configuration):
        pass

    def getInterface(self):
        return str(test_packet)

    def bootstrap_complete_callback(self, *args):
        test_fullDHCPPacket()
        test_dns_1()
        test_dns_2()
        testEAPOL ()
        testEthernet()
        testEthernetConstruct()
        testICMPUnreachConstruct()
        testICMPUnreach()
        testICMPEchoConstruct()
        testICMPEcho()
        testIPv4()
        testIPv4Construct()
        testIPv4Edge()
        testLLDP()
        testLLDPConstruct()
        testLLDPConstruct2()
        testTCP()
        fullTCPPacket()
        fullUDPPacket()
        testVlan()
        testVlanARP()
        testVlanAdd()
        testVlanRemove()
        sys.stdout.flush() # XXX handle in component::exit 
        os._exit(0)  # XXX push to component 

    def install(self):
        self.register_for_bootstrap_complete(self.bootstrap_complete_callback)

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return test_packet(ctxt)

    return Factory()
