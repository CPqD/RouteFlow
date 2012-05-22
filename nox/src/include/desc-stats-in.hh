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

#ifndef DESC_STATS_IN_HH__
#define DESC_STATS_IN_HH__

#include <string>

#include "event.hh"
#include "netinet++/datapathid.hh"
#include "ofp-msg-event.hh"

#include "openflow/openflow.h"

namespace vigil {


struct Desc_stats_in_event
    : public Event,
      public Ofp_msg_event
{
    Desc_stats_in_event(const datapathid& dpid, const ofp_stats_reply *osf,
                        std::auto_ptr<Buffer> buf);

    // -- only for use within python
    Desc_stats_in_event() : Event(static_get_name()) { }

    static const Event_name static_get_name() {
        return "Desc_stats_in_event";
    }

    datapathid datapath_id;

    std::string mfr_desc;
    std::string hw_desc;
    std::string sw_desc;
    std::string dp_desc;
    std::string serial_num;

    Desc_stats_in_event(const Desc_stats_in_event&);
    Desc_stats_in_event& operator=(const Desc_stats_in_event&);

}; // Desc_stats_in_event

inline
Desc_stats_in_event::Desc_stats_in_event(const datapathid& dpid,
                                         const ofp_stats_reply *osr,
                                         std::auto_ptr<Buffer> buf)
    : Event(static_get_name()), Ofp_msg_event(&osr->header, buf)
{
    datapath_id  = dpid;

    ofp_desc_stats* ods = (ofp_desc_stats*)(osr->body);
    mfr_desc   = ods->mfr_desc;
    hw_desc    = ods->hw_desc;
    sw_desc    = ods->sw_desc;
    dp_desc    = ods->dp_desc;
    serial_num = ods->serial_num;
}

} // namespace vigil

#endif // DESC_STATS_IN_HH__
