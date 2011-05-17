from nox.lib import core, openflow, packet
from nox.lib.core import Component
from nox.lib.packet import ethernet, ipv4, dns
from nox.coreapps.pyrt.pycomponent import CONTINUE

from twisted.python import log
from collections import defaultdict
import curses

class DnsSpy(Component):
    def __init__(self, ctxt):
        Component.__init__(self, ctxt)
        self.ip_records = defaultdict(list) 

    def install(self):
        self.register_for_datapath_join(self.dp_join)
        match_src = { core.DL_TYPE: ethernet.ethernet.IP_TYPE,
                      core.NW_PROTO : ipv4.ipv4.UDP_PROTOCOL,
                      core.TP_SRC : 53}
        self.register_for_packet_match(self.handle_dns, 0xffff, match_src)

    def dp_join(self, dpid, stats): 
        # Make sure we get the full DNS packet at the Controller
        self.install_datapath_flow(dpid, 
                                   { core.DL_TYPE : ethernet.ethernet.IP_TYPE,
                                     core.NW_PROTO : ipv4.ipv4.UDP_PROTOCOL,
                                     core.TP_SRC : 53 },
                                   openflow.OFP_FLOW_PERMANENT, openflow.OFP_FLOW_PERMANENT,
                                   [[openflow.OFPAT_OUTPUT, [0, openflow.OFPP_CONTROLLER]]])
        return CONTINUE

    def handle_dns(self, dpid, inport, ofp_reason, total_frame_len, buffer_id, packet):
        dnsh = packet.find('dns')
        if not dnsh:
            log.err('received invalid DNS packet',system='dnsspy')
            return CONTINUE

        log.msg(str(dnsh),system='dnsspy')

        for answer in dnsh.answers:
            if answer.qtype == dns.dns.rr.A_TYPE:
                val = self.ip_records[answer.rddata]
                if answer.name not in val:
                    val.insert(0, answer.name)
                    log.msg("add dns entry: %s %s" % (answer.rddata, answer.name), system='dnsspy')
        for addition in dnsh.additional:
# WHAT IS THIS?! XXX            
#            for char in addition.name:
#                # some debugging magic in case we have a bad parse in DNS
#                if not curses.ascii.isascii(char):
#                    for byte in dnsh.get_layer():
#                        print '%x' % byte,
#                    print ''    
#                    continue
            if addition.qtype == dns.dns.rr.A_TYPE: 
                val = self.ip_records[addition.rddata]
                if addition.name not in val:
                    val.insert(0, addition.name)
                    log.msg("additional dns entry: %s %s" % (addition.rddata, addition.name), system='dnsspy')

        return CONTINUE

    def getInterface(self):
        return str(DnsSpy)

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return DnsSpy(ctxt)

    return Factory()
