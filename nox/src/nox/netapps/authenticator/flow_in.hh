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

#ifndef FLOW_IN_HH
#define FLOW_IN_HH 1

#include "data/datatypes.hh"
#include "event.hh"
#include "flow.hh"
#include "hash_set.hh"
#include "packet-in.hh"

namespace vigil {

struct Flow_in_event
    : public Event
{
    Flow_in_event(const timeval& received_,
                  const Packet_in_event& pi,
                  const Flow& flow_);

    Flow_in_event();

    ~Flow_in_event() { }

    static const Event_name static_get_name() {
        return "Flow_in_event";
    }

    static const uint32_t NOT_ROUTED = UINT32_MAX - 1;
    static const uint32_t BROADCASTED = UINT32_MAX;

    struct DestinationInfo {
        AuthedLocation              authed_location;
        bool                        allowed;
        std::vector<int64_t>        waypoints;
        hash_set<uint32_t>          rules;
    };

    typedef std::vector<DestinationInfo> DestinationList;

    // 'active' == true if flow can still be "acted" upon else it has been
    // consumed by some part of the system.

    bool                                      active;
    bool                                      fn_applied; //if a function consumed the flow
    std::string                               fn_name;
    timeval                                   received;
    datapathid                                datapath_id;
    Flow                                      flow;
    boost::shared_ptr<Buffer>                 buf;
    size_t                                    total_len;
    uint32_t                                  buffer_id;
    uint8_t                                   reason;

    boost::shared_ptr<HostNetid>              src_host_netid;
    AuthedLocation                            src_location;
    boost::shared_ptr<GroupList>              src_dladdr_groups;
    boost::shared_ptr<GroupList>              src_nwaddr_groups;

    bool                                      dst_authed;
    boost::shared_ptr<HostNetid>              dst_host_netid;
    DestinationList                           dst_locations;
    boost::shared_ptr<GroupList>              dst_dladdr_groups;
    boost::shared_ptr<GroupList>              dst_nwaddr_groups;

    boost::shared_ptr<Location>               route_source;
    std::vector<boost::shared_ptr<Location> > route_destinations;
    uint32_t                                  routed_to;

}; // class Flow_in_event

#ifdef TWISTED_ENABLED

template <>
PyObject*
to_python(const Flow_in_event::DestinationInfo& dinfo);

template <>
PyObject*
to_python(const Flow_in_event::DestinationList& dlist);

PyObject*
route_source_to_python(const boost::shared_ptr<Location>& location);

PyObject*
route_destinations_to_python(const std::vector<boost::shared_ptr<Location> >& dlist);

#endif

} // namespace vigil

#endif // FLOW_IN_HH
