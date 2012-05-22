/* Copyright 2008 (C) Nicira, Inc.
 *
 * This file is part of NOX.
 *
 * NOX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NOX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NOX.  If not, see <http://www.gnu.org/licenses/>.
 */
%module "nox.netapps.routing.pyrouting"

%{
#include "pyrouting.hh"
#include "routing.hh"
using namespace vigil;
using namespace vigil::applications;
%}

%import "netinet/netinet.i"
%import "openflow.i"

%include "common-defs.i"
%include "std_list.i"
%include "buffer.i"
%include "pyrouting.hh"

namespace Routing_module {
    struct Link {
        datapathid dst;    // destination dp of link starting at current dp
        uint16_t outport;  // outport of current dp link connected to
        uint16_t inport;   // inport of destination dp link connected to
    };
};

%template(linklist) std::list<Routing_module::Link>;

namespace Routing_module {
    struct RouteId {
        datapathid src;
        datapathid dst;
    };

    struct Route {
        RouteId id;            // start/endpoint
        std::list<Link> path;  // links connecting start/end
    };
};

%{

bool
dp_on_route(const datapathid& dp, const Routing_module::Route& route)
{
    for (std::list<Routing_module::Link>::const_iterator iter = route.path.begin();
         iter != route.path.end(); ++iter)
    {
        if (dp == iter->dst) {
            return true;
        }
    }
    return false;
}

%}

bool dp_on_route(const datapathid& dp, const Routing_module::Route& route);

%pythoncode
%{
    from nox.lib.core import Component

    class PyRouting(Component):
        def __init__(self, ctxt):
            Component.__init__(self, ctxt)
            self.routing = PyRouting_module(ctxt)

        def configure(self, configuration):
            self.routing.configure(configuration)         

        def getInterface(self):
            return str(PyRouting)

        def get_route(self, route):
            return self.routing.get_route(route)

        def check_route(self, route, inport, outport):
            return self.routing.check_route(route, inport, outport)

        # eventually add actions
        def setup_route(self, flow, route, inport, outport,
                        flow_timeout, bufs, check_nat):
            return self.routing.setup_route(flow, route, inport, outport,
                                            flow_timeout, bufs, check_nat)

        def setup_flow(self, flow, dp, outport, bid, buf, timeout,
                       actions, check_nat):
            return self.routing.setup_flow(flow, dp, outport, bid,
                                           buf, timeout, actions, check_nat)

        def send_packet(self, dp, inport, outport, bid, buf,
                        actions, check_nat, flow):
            return self.routing.send_packet(dp, inport, outport, bid,
                                            buf, actions, check_nat, flow)

    def getFactory():
        class Factory():
            def instance(self, context):
                return PyRouting(context)

        return Factory()
%}
