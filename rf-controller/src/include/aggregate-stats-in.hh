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

#ifndef AGGREGATE_STATS_IN_HH__
#define AGGREGATE_STATS_IN_HH__

#include <string>

#include "event.hh"
#include "netinet++/datapathid.hh"
#include "ofp-msg-event.hh"

#include "openflow/openflow.h"

namespace vigil {


struct Aggregate_stats_in_event
    : public Event,
      public Ofp_msg_event
{
    Aggregate_stats_in_event(const datapathid& dpid,
                             const ofp_stats_reply *osf,
                             std::auto_ptr<Buffer> buf);

    // -- only for use within python
    Aggregate_stats_in_event() : Event(static_get_name()) { }

    static const Event_name static_get_name() {
        return "Aggregate_stats_in_event";
    }

    datapathid datapath_id;

    uint64_t packet_count;
    uint64_t byte_count;
    uint32_t flow_count;

    Aggregate_stats_in_event(const Aggregate_stats_in_event&);
    Aggregate_stats_in_event& operator=(const Aggregate_stats_in_event&);

}; // Aggregate_stats_in_event

inline
Aggregate_stats_in_event::Aggregate_stats_in_event(const datapathid& dpid,
                                                   const ofp_stats_reply *osr,
                                                   std::auto_ptr<Buffer> buf)
    : Event(static_get_name()), Ofp_msg_event(&osr->header, buf)
{
    datapath_id  = dpid;

    ofp_aggregate_stats_reply* oasr = (ofp_aggregate_stats_reply*)(osr->body);
    packet_count = ntohll(oasr->packet_count);
    byte_count   = ntohll(oasr->byte_count);
    flow_count   = ntohl(oasr->flow_count);
}


} // namespace vigil

#endif // AGGREGATE_STATS_IN_HH__
