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
from nox.netapps.tests import unittest
from nox.lib.netinet import netinet
from nox.netapps.discovery.pylinkevent import Link_event
from nox.netapps.routing import pyrouting
from nox.coreapps.pyrt.pycomponent import CONTINUE

from twisted.internet import reactor, defer
from twisted.python import log

pyunit = __import__('unittest')

class RoutingTestCase(unittest.NoxTestCase):
    def getInterface(self):
        return str(RoutingTestCase)

    def configure(self, configuration):
        #self.register_event(Link_event.static_get_name())
        Link_event.register_event_converter(self.ctxt)

    def install(self):
        self.routing = self.resolve(pyrouting.PyRouting)
        self.n_received = 0;
        self.register_handler(Link_event.static_get_name(),
                              self.handle_link_event)

    def post_events(self):
        e = Link_event(netinet.datapathid.from_host(2),
                       netinet.datapathid.from_host(3),
                       0, 0, Link_event.ADD)
        self.post(e)
        e = Link_event(netinet.datapathid.from_host(1),
                       netinet.datapathid.from_host(2),
                       0, 1, Link_event.ADD)
        self.post(e)
        self.d = defer.Deferred()
        return self.d

    def handle_link_event(self, event):
        if self.n_received == 0:
            self.n_received = 1
            return CONTINUE

        route = pyrouting.Route()
        route.id.src = src = netinet.datapathid.from_host(1)
        route.id.dst = dst = netinet.datapathid.from_host(3)

        if not self.routing.get_route(route):
            log.err('Route between dps 1 and 3 not found.')
            self.fail()
        else:
            if route.id.src != src:
                log.err('route.src got reset!')
                self.fail()
            if route.id.dst != dst:
                log.err('route.dst got reset!')
                self.fail()
            if route.path.size() != 2:
                log.err('route length %u != 2.' % route.path.size())
                self.fail()
            it = route.path.begin()
            dst = netinet.datapathid.from_host(2)
            oport = 0
            inport = 1
            while it != route.path.end():
                lk = it.value()
                if lk.dst != dst:
                    log.err('Link dst has wrong dp')
                    self.fail()
                if lk.outport != oport:
                    log.err('src dp link outport %u not equal to %u' % (lk.outport, oport))
                    self.fail()
                if lk.inport != inport:
                    log.err('dst dp link inport %u not equal to %u' % (lk.inport, inport))
                    self.fail()
                it.incr()
                dst = netinet.datapathid.from_host(3)
                inport = 0

        reactor.callLater(0, self.d.callback, None)
        return CONTINUE

def suite(ctxt):
    suite = pyunit.TestSuite()
    suite.addTest(RoutingTestCase("post_events", ctxt))
    return suite

