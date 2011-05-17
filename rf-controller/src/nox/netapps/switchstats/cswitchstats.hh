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
#ifndef CSWITCHSTATS_HH
#define CSWITCHSTATS_HH 1

#include "component.hh"
#include "hash_map.hh"
#include "event-tracker.hh"

#include <vector>

namespace vigil {
namespace applications {

class CSwitchStats
    : public container::Component 
{
    public:
       
        const static int TIMESLICE = 5000; // milliseconds

    protected:

        hash_map<uint64_t, EventTracker>              tracker_map; 
        hash_map<uint64_t, hash_map<uint64_t, bool> > switch_port_map; 

    public:

        CSwitchStats(const container::Context*, const json_object*);

        static void getInstance(const container::Context*, CSwitchStats*&);

        void configure(const container::Configuration*);
        void install();

        float get_global_conn_p_s(void);
        float get_switch_conn_p_s(uint64_t);
        float get_loc_conn_p_s   (uint64_t);

        Disposition handle_datapath_join(const Event&);
        Disposition handle_data_leave   (const Event&);
        Disposition handle_packet_in    (const Event& e);
        Disposition handle_port_status  (const Event& e);

        void test_print_averages();
};


} // namespace applications
} // namespace vigil


#endif  //  CSWITCHSTATS_HH
