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

#include "cswitchstats.hh"

#include <boost/bind.hpp>
#include <iostream>

#include "openflow.hh"
#include "packet-in.hh"
#include "port-status.hh"
#include "datapath-join.hh"
#include "datapath-leave.hh"
#include "assert.hh"
#include "vlog.hh"

using namespace std;
using namespace vigil;
using namespace vigil::applications;
using namespace vigil::container;

static Vlog_module lg("cswitchstats");

CSwitchStats::CSwitchStats(const Context* c,
                   const json_object*)
    : Component(c) 
{
}

void
CSwitchStats::install() {

}

void 
CSwitchStats::test_print_averages()
{
    cout << " TOTAL! " << get_global_conn_p_s() << endl;
    timeval tv = { 1, 0 };
    post(boost::bind(&CSwitchStats::test_print_averages, this), tv);
}

void
CSwitchStats::configure(const Configuration*)
{
    register_handler<Datapath_join_event>
        (boost::bind(&CSwitchStats::handle_datapath_join, this, _1));
    register_handler<Datapath_leave_event>
        (boost::bind(&CSwitchStats::handle_data_leave, this, _1));
    register_handler<Packet_in_event>
        (boost::bind(&CSwitchStats::handle_packet_in, this, _1));
    register_handler<Port_status_event>
        (boost::bind(&CSwitchStats::handle_port_status, this, _1));

    // For testing
    // -- timeval tv = { 1, 0 };
    // -- post(boost::bind(&CSwitchStats::test_print_averages, this), tv);
}

Disposition
CSwitchStats::handle_datapath_join(const Event& e)
{
    const Datapath_join_event& dj = assert_cast<const Datapath_join_event&>(e);
    uint64_t dpint = dj.datapath_id.as_host();

    if(switch_port_map.find(dpint) != switch_port_map.end()){
        VLOG_ERR(lg, "DP join of existing switch %"PRIu64, dpint);
        for(hash_map<uint64_t, hash_map<uint64_t, bool>
                >::const_iterator iter = switch_port_map.begin();
                iter != switch_port_map.end(); ++iter){
            tracker_map.erase(iter->first);
        }
        switch_port_map.erase(dpint);
    }

    for (std::vector<Port>::const_iterator iter = dj.ports.begin();
         iter != dj.ports.end(); ++iter) {
        uint64_t loc = dpint + (((uint64_t) iter->port_no) << 48);
        switch_port_map[dpint][loc] = true;
        tracker_map[loc].reset(TIMESLICE); // 5 second timeslices
    }

    return CONTINUE;
}

Disposition
CSwitchStats::handle_data_leave(const Event& e)
{
    const Datapath_leave_event& dl = assert_cast<const Datapath_leave_event&>(e);
    uint64_t dpint = dl.datapath_id.as_host();

    if(switch_port_map.find(dpint) == switch_port_map.end()){
        VLOG_ERR(lg, "DP leave of non-existent switch %"PRIu64"", dpint);
        return CONTINUE;
    }

    for(hash_map<uint64_t, bool>::const_iterator iter = switch_port_map[dpint].begin();
             iter != switch_port_map[dpint].end(); ++iter){
        tracker_map.erase(iter->first);
    }

    switch_port_map.erase(dpint);

    return CONTINUE;
}

Disposition
CSwitchStats::handle_packet_in(const Event& e)
{
    const Packet_in_event& pi = assert_cast<const Packet_in_event&>(e);
    uint64_t loc = pi.datapath_id.as_host() + (((uint64_t) pi.in_port) << 48);
    if(tracker_map.find(loc) == tracker_map.end()){
        VLOG_ERR(lg, "packet-in from non-existent location %"PRIx64"", loc);
        return CONTINUE;
    }
    
    tracker_map[loc].add_event(1);

    return CONTINUE;
}

Disposition
CSwitchStats::handle_port_status(const Event& e)
{
    const Port_status_event& ps = assert_cast<const Port_status_event&>(e);
    uint64_t dpid = ps.datapath_id.as_host();

    if (switch_port_map.find(dpid) == switch_port_map.end()){
        VLOG_ERR(lg, "(handle_port_stats) received port status from untracked %"PRIu64, dpid);
        return CONTINUE;
    }

    if (ps.reason == OFPPR_ADD){
        uint64_t loc = dpid + (((uint64_t) ps.port.port_no) << 48);
        VLOG_DBG(lg, "(handle_port_stats) adding loc %"PRIu64, loc);
        tracker_map[loc].reset(TIMESLICE); // 5 second timeslices
        switch_port_map[dpid][loc] = true;
    }else if (ps.reason == OFPPR_DELETE){
        uint64_t loc = dpid + (((uint64_t) ps.port.port_no) << 48);
        VLOG_DBG(lg, "(handle_port_stats) deleting loc %"PRIu64, loc);
        tracker_map.erase(loc);
        switch_port_map[dpid].erase(loc);
    }

    return CONTINUE;
}

float 
CSwitchStats::get_global_conn_p_s(void)
{
    float total = 0.;

    for(hash_map<uint64_t, hash_map<uint64_t, bool>
            >::const_iterator iter = switch_port_map.begin();
            iter != switch_port_map.end(); ++iter){
        for (hash_map<uint64_t, bool>::const_iterator inneriter =
                iter->second.begin(); inneriter != iter->second.end();
                ++inneriter) {
            if (tracker_map.find(inneriter->first) == tracker_map.end() ) {
                VLOG_ERR(lg, "mapped location %"PRIu64" doesn't exist", inneriter->first);
                continue;
            }
            if (tracker_map[inneriter->first].get_history_q_ref().size() > 0){
                total += tracker_map[inneriter->first].get_history_q_ref().front();
            }
        }
    }

    return total;
}

float 
CSwitchStats::get_switch_conn_p_s(uint64_t dpid)
{
    float total = 0.;

    if (switch_port_map.find(dpid) == switch_port_map.end()){
        VLOG_ERR(lg, "(get_switch_conn_p_s) no ports associated with dpid %"PRIu64, dpid);
        return total;
    }

    for (hash_map<uint64_t, bool>::const_iterator iter = switch_port_map[dpid].begin(); 
            iter != switch_port_map[dpid].end();
            ++iter) {

        if (tracker_map.find(iter->first) == tracker_map.end() ) {
            VLOG_ERR(lg, "mapped location %"PRIu64" doesn't exist", iter->first); 
            continue;
        }
        if (tracker_map[iter->first].get_history_q_ref().size() > 0){
            total += tracker_map[iter->first].get_history_q_ref().front();
        }
    }

    return total;
}

float 
CSwitchStats::get_loc_conn_p_s   (uint64_t loc)
{
    if (tracker_map.find(loc) == tracker_map.end()){
        VLOG_ERR(lg, "(get_loc_conn_p_s) loc %"PRIu64" doesn't exist", loc);
        return 0.;
    }

    if (tracker_map[loc].get_history_q_ref().size() > 0){
        return tracker_map[loc].get_history_q_ref().front();
    }
    return 0.;
}

void 
CSwitchStats::getInstance(const container::Context* ctxt, CSwitchStats*& scpa) {
    scpa = dynamic_cast<CSwitchStats*>
        (ctxt->get_by_interface(container::Interface_description
                                (typeid(CSwitchStats).name())));
}

REGISTER_COMPONENT(vigil::container::Simple_component_factory<CSwitchStats>,
                   CSwitchStats);
