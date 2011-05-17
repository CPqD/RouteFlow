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
#ifndef NORMAL_ROUTING_HH
#define NORMAL_ROUTING_HH 1

#include <boost/shared_array.hpp>

#include "authenticator/flow_in.hh"
#include "component.hh"
#include "openflow/openflow.h"


namespace vigil {
namespace applications {

class Normal_routing
    : public container::Component {

public:
    Normal_routing(const container::Context*,
                   const json_object*);

    ~Normal_routing() { }

    static void getInstance(const container::Context*, Normal_routing*&);

    void configure(const container::Configuration*);
    void install();

private:
    boost::shared_array<uint8_t> raw_of;
    ofp_flow_mod *ofm;

    Disposition handle_flow_in(const Event& e);
};

}
}

#endif
