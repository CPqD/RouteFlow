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
#ifndef SPROUTING_HH
#define SPROUTING_HH 1

#include "authenticator/flow_in.hh"
#include "component.hh"
#include "openflow/openflow.h"
#include "routing.hh"

/*
 * Default routing component.  Listens for Flow_in_events and sets up shortest
 * path route from source to destination.  If no route betweent the access
 * point datapaths exists, broadcasts packet.  If a route does exist but would
 * cause the packet to be routed out the same port in on at either the source
 * or destination datapath, flow is dropped.
 *
 */

namespace vigil {
namespace applications {

class SPRouting
    : public container::Component {

public:
    SPRouting(const container::Context*,
              const json_object*);

    ~SPRouting() { }

    static void getInstance(const container::Context*, SPRouting*&);

    void configure(const container::Configuration*);
    void install();

private:
    Routing_module *routing;
    Routing_module::RoutePtr empty;
    bool passive;

    Disposition handle_bootstrap(const Event& e);
    void update_passive();
    Disposition handle_flow_in(const Event& e);
    Disposition route_flow(Flow_in_event&, const Buffer&, bool);
    bool set_route(Flow_in_event&, Routing_module::RoutePtr&,
                   uint16_t& inport, uint16_t& outport, const Buffer&,
                   bool);
    Flow_fn_map::Flow_fn generate_rewrite(const std::vector<std::string>&,
                                          const std::string&);
    void rewrite_and_route(const Flow_in_event&, const Buffer&,
                           const boost::shared_array<uint8_t>&, bool);
    void route_no_nat(const Flow_in_event& ev);
};

}
}

#endif
