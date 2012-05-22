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
#ifndef CONTROLLER_PYROUTEGLUE_HH
#define CONTROLLER_PYROUTEGLUE_HH 1

#include <Python.h>

#include "routing.hh"
#include "component.hh"

/*
 * Proxy routing "component" used to obtain routes from python.
 */

namespace vigil {
namespace applications {

class PyRouting_module {
public:
    PyRouting_module(PyObject*);

    void configure(PyObject*);

    bool get_route(Routing_module::Route& route) const;
    bool check_route(const Routing_module::Route& route,
                     uint16_t inport, uint16_t outport) const;
    bool setup_route(const Flow& flow, const Routing_module::Route& route,
                     uint16_t inport, uint16_t outport, uint16_t flow_timeout,
                     const std::list<Nonowning_buffer>& bufs, bool check_nat);

    bool setup_flow(const Flow& flow, const datapathid& dp,
                    uint16_t outport, uint32_t bid, const Nonowning_buffer& buf,
                    uint16_t flow_timeout, const Nonowning_buffer& actions,
                    bool check_nat);

    bool send_packet(const datapathid& dp, uint16_t inport, uint16_t outport,
                     uint32_t bid, const Nonowning_buffer& buf,
                     const Nonowning_buffer& actions, bool check_nat,
                     const Flow& flow);

private:
    Routing_module *routing;
    container::Component* c;
};

}
}

#endif
