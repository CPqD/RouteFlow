/* Copyright 2008, 2009 (C) Nicira, Inc.
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
#include "pyrouting.hh"

#include "swigpyrun.h"
#include "pyrt/pycontext.hh"

namespace vigil {
namespace applications {

PyRouting_module::PyRouting_module(PyObject* ctxt)
    : routing(0)
{
    if (!SWIG_Python_GetSwigThis(ctxt) || !SWIG_Python_GetSwigThis(ctxt)->ptr) {
        throw std::runtime_error("Unable to access Python context.");
    }

    c = ((PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr)->c;
}

void
PyRouting_module::configure(PyObject* configuration)
{
    c->resolve(routing);
}

bool
PyRouting_module::get_route(Routing_module::Route& route) const
{
    Routing_module::RoutePtr rte;

    if (route.id.src == route.id.dst) {
        route.path.clear();
        return true;
    }

    if (!routing->get_route(route.id, rte)) {
        return false;
    }

    route.path = rte->path;
    return true;
}

bool
PyRouting_module::check_route(const Routing_module::Route& route,
                              uint16_t inport, uint16_t outport) const
{
    return routing->check_route(route, inport, outport);
}

bool
PyRouting_module::setup_route(const Flow& flow,
                              const Routing_module::Route& route,
                              uint16_t inport, uint16_t outport,
                              uint16_t flow_timeout,
                              const std::list<Nonowning_buffer>& bufs,
                              bool check_nat)
{
    return routing->setup_route(flow, route, inport, outport,
                                flow_timeout, bufs, check_nat,
                                NULL, NULL, NULL, NULL);
}

bool
PyRouting_module::setup_flow(const Flow& flow, const datapathid& dp,
                             uint16_t outport, uint32_t bid,
                             const Nonowning_buffer& buf,
                             uint16_t flow_timeout,
                             const Nonowning_buffer& actions, bool check_nat)
{
    return routing->setup_flow(flow, dp, outport, bid, buf, flow_timeout,
                               actions, check_nat, NULL, NULL, NULL, NULL);
}

bool
PyRouting_module::send_packet(const datapathid& dp, uint16_t inport,
                              uint16_t outport, uint32_t bid,
                              const Nonowning_buffer& buf,
                              const Nonowning_buffer& actions,
                              bool check_nat, const Flow& flow)
{
    return routing->send_packet(dp, inport, outport, bid, buf, actions,
                                check_nat, flow, NULL, NULL, NULL, NULL);
}

}
}

