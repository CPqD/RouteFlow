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

#ifndef FLOW_STATS_IN_HH__
#define FLOW_STATS_IN_HH__

#include <stddef.h>
#include <string>

#include "event.hh"
#include "netinet++/datapathid.hh"
#include "ofp-msg-event.hh"

#include "openflow/openflow.h"

namespace vigil {

struct Flow_stats
    : public ofp_flow_stats
{
    std::vector<ofp_action_header> v_actions;

    Flow_stats(const ofp_flow_stats*);
};

struct Flow_stats_in_event
    : public Event,
      public Ofp_msg_event
{
    Flow_stats_in_event(const datapathid& dpid, const ofp_stats_reply *osr,
                        std::auto_ptr<Buffer> buf);

    // -- only for use within python
    Flow_stats_in_event() : Event(static_get_name()) { }

    static const Event_name static_get_name() {
        return "Flow_stats_in_event";
    }

    datapathid datapath_id;
    bool more;
    std::vector<Flow_stats> flows;

    Flow_stats_in_event(const Flow_stats_in_event&);
    Flow_stats_in_event& operator=(const Flow_stats_in_event&);
}; // Flow_stats_in_event

} // namespace vigil

#endif // FLOW_STATS_IN_HH__
