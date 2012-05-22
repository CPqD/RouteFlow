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
#include "component.hh"
#include "link-event.hh"

#include "vlog.hh"

using namespace std;
using namespace vigil;
using namespace vigil::container;

namespace vigil {
    Link_event::Link_event(datapathid dpsrc_, datapathid dpdst_,
               uint16_t sport_, uint16_t dport_,
               Action action_)
        : Event(static_get_name()), dpsrc(dpsrc_), dpdst(dpdst_),
          sport(sport_), dport(dport_),
          action(action_) { }
    
    // -- only for use within python
    Link_event::Link_event() : Event(static_get_name()) { }

} // namespace vigil

namespace {

static Vlog_module lg("link-event");

class LinkEvent_component
    : public Component {
public:
    LinkEvent_component(const Context* c,
                     const json_object*) 
        : Component(c) {
    }

    void configure(const Configuration*) {
    }

    void install() {
    }

private:
    
};

REGISTER_COMPONENT(container::Simple_component_factory<LinkEvent_component>, 
                   LinkEvent_component);

} // unnamed namespace
