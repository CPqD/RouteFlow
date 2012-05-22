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

#ifndef TABLE_STATS_IN_HH__
#define TABLE_STATS_IN_HH__

#include <string>

#include "event.hh"
#include "netinet++/datapathid.hh"
#include "ofp-msg-event.hh"

#include "openflow/openflow.h"

namespace vigil {


struct Table_stats
{
    Table_stats(int tid, const std::string& n, uint32_t me,
            uint32_t ac, uint64_t lc, uint64_t mc):
        table_id(tid), name(n),
        max_entries(me), active_count(ac),
        lookup_count(lc), matched_count(mc){ ; }

    int table_id;
    std::string name;
    uint32_t max_entries;
    uint32_t active_count;
    uint64_t lookup_count;
    uint64_t matched_count;
};

struct Table_stats_in_event
    : public Event,
      public Ofp_msg_event
{
    Table_stats_in_event(const datapathid& dpid, const ofp_stats_reply *osf,
                         std::auto_ptr<Buffer> buf);

    // -- only for use within python
    Table_stats_in_event() : Event(static_get_name()) { }

    static const Event_name static_get_name() {
        return "Table_stats_in_event";
    }

    datapathid datapath_id;
    std::vector<Table_stats> tables;

    Table_stats_in_event(const Table_stats_in_event&);
    Table_stats_in_event& operator=(const Table_stats_in_event&);

    void add_table(int tid, const std::string& n, uint32_t
        me, uint32_t ac, uint64_t lc, uint64_t mc);

}; // Table_stats_in_event

inline
Table_stats_in_event::Table_stats_in_event(const datapathid& dpid,
                                           const ofp_stats_reply *osr,
                                           std::auto_ptr<Buffer> buf)
    : Event(static_get_name()), Ofp_msg_event(&osr->header, buf)
{
    datapath_id  = dpid;
}

inline
void
Table_stats_in_event::add_table(int tid, const std::string& n, uint32_t
        me, uint32_t ac, uint64_t lc, uint64_t mc)
{
    tables.push_back(Table_stats(tid, n, me, ac, lc, mc));
}

} // namespace vigil

#endif // TABLE_STATS_IN_HH__
