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
#ifndef ROUTING_HH
#define ROUTING_HH 1

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <list>
#include <queue>
#include <sstream>
#include <vector>

#include "component.hh"
#include "event.hh"
#include "flow.hh"
#include "hash_map.hh"
#include "hash_set.hh"
#include "discovery/link-event.hh"
#include "nat_enforcer.hh"
#include "netinet++/datapathid.hh"
#include "netinet++/ethernetaddr.hh"
#include "openflow/openflow.h"
#include "topology/topology.hh"


namespace vigil {
namespace applications {

/** \ingroup noxcomponents
 *
 * Routing_module is a utility component that can be called to retrieve a
 * shortest path route and/or to set up flow entries in the network with
 * Openflow actions to route a flow.
 *
 * NOX instances will usually have an additional routing component listening
 * for Flow_in_events and making calls to this API to route the flows through
 * the network.  Such components should list 'routing_module' in their meta.xml
 * dependencies.  See 'sprouting.hh/.cc' for an example.
 *
 * Uses algorithm described in "A New Approach to Dynamic All Pairs Shortest
 * Paths" by C. Demetrescu to perform incremental updates when links are
 * added/removed from the network instead of recomputing all shortest paths.
 *
 * All integer values are stored in host byte order and should be passed in as
 * such as well.
 *
 */

class Routing_module
    : public container::Component {

public:
    struct Link {
        datapathid dst;    // destination dp of link starting at current dp
        uint16_t outport;  // outport of current dp link connected to
        uint16_t inport;   // inport of destination dp link connected to
    };

    struct RouteId {
        datapathid src;    // Source dp of a route
        datapathid dst;    // Destination dp of a route
    };

    struct Route {
        RouteId id;            // Start/End datapath
        std::list<Link> path;  // links connecting datapaths
    };

    typedef boost::shared_ptr<Route> RoutePtr;
    typedef std::list<Nonowning_buffer> ActionList;

    Routing_module(const container::Context*,
                   const json_object*);
    // for python
    Routing_module();
    ~Routing_module() { }

    static void getInstance(const container::Context*, Routing_module*&);

    void configure(const container::Configuration*);
    void install();

    // Given a RouteId 'id', sets 'route' to the route between the two
    // datapaths.  Returns 'true' if a route between the two exists, else
    // 'false'.  In most cases, the route should later be passed on to
    // check_route() with the inport and outport of the start and end dps to
    // verify that a packet will not get route out the same port it came in
    // on, (which should only happen in very specific cases, e.g. wireless APs).

    // Should avoid calling this method unnecessarily when id.src == id.dst -
    // as module will allocate a new empty Route() to populate 'route'.  Caller
    // will ideally check for this case.

    // Routes retrieved through this method SHOULD NOT BE MODIFIED.

    bool get_route(const RouteId& id, RoutePtr& route) const;

    // Given a route and an access point inport and outport, verifies that a
    // flow will not get route out the same port it came in on at the
    // endpoints.  Returns 'true' if this will not happen, else 'false'.

    bool check_route(const Route& route, uint16_t inport, uint16_t outport) const;

    bool is_on_path_location(const RouteId& id, uint16_t src_port,
                             uint16_t dst_port);

    // Sets up the switch entries needed to route Flow 'flow' according to
    // 'route' and the source access point 'inport' and destination access
    // point 'outport'.  Entries will time out after 'flow_timeout' seconds of
    // inactivity.  If 'actions' is non-empty, it should specify the set of
    // Openflow actions to input at each datapath en route, in addition to the
    // default OFPAT_OUTPUT action used to route the flow.  If non-empty,
    // 'actions' should be of length 'route.path.size() + 1'.  'actions[i]'
    // should be the set of actions to place in the ith dp en route to the
    // destination, packed in a buffer of size equal to the action data's len.
    // If a flow's headers will be overwritten by the actions at a particular
    // datapath, module takes care of putting the correct modified version of
    // 'flow' in later switch flow entries en route.  Buffers in 'actions'
    // should be Openflow message-ready - actions should be packed together
    // with correct padding and fields should be in network byte order.

    bool setup_route(const Flow& flow, const Route& route, uint16_t inport,
                     uint16_t outport, uint16_t flow_timeout,
                     const ActionList& actions, bool check_nat,
                     const GroupList *sdladdr_groups,
                     const GroupList *snwaddr_groups,
                     const GroupList *ddladdr_groups,
                     const GroupList *dnwaddr_groups);

    bool setup_flow(const Flow& flow, const datapathid& dp,
                    uint16_t outport, uint32_t bid, const Buffer& buf,
                    uint16_t flow_timeout, const Buffer& actions,
                    bool check_nat,
                    const GroupList *sdladdr_groups,
                    const GroupList *snwaddr_groups,
                    const GroupList *ddladdr_groups,
                    const GroupList *dnwaddr_groups);

    bool send_packet(const datapathid& dp, uint16_t inport, uint16_t outport,
                     uint32_t bid, const Buffer& buf,
                     const Buffer& actions, bool check_nat,
                     const Flow& flow,
                     const GroupList *sdladdr_groups,
                     const GroupList *snwaddr_groups,
                     const GroupList *ddladdr_groups,
                     const GroupList *dnwaddr_groups);

private:
    struct ridhash {
        std::size_t operator()(const RouteId& rid) const;
    };

    struct rideq {
        bool operator()(const RouteId& a, const RouteId& b) const;
    };

    struct routehash {
        std::size_t operator()(const RoutePtr& rte) const;
    };

    struct routeq {
        bool operator()(const RoutePtr& a, const RoutePtr& b) const;
    };

    struct ruleptrcmp {
        bool operator()(const RoutePtr& a, const RoutePtr& b) const;
    };

    typedef std::list<RoutePtr> RouteList;
    typedef hash_set<RoutePtr, routehash, routeq> RouteSet;
    typedef hash_map<RouteId, RoutePtr, ridhash, rideq> RouteMap;
    typedef hash_map<RouteId, RouteList, ridhash, rideq> RoutesMap;
    typedef hash_map<RoutePtr, RouteList, routehash, routeq> ExtensionMap;
    typedef std::priority_queue<RoutePtr, std::vector<RoutePtr>, ruleptrcmp> RouteQueue;

    // Data structures needed by All-Pairs Shortest Path Algorithm

    Topology *topology;
    NAT_enforcer *nat;
    RouteMap shortest;
    RoutesMap local_routes;
    ExtensionMap left_local;
    ExtensionMap left_shortest;
    ExtensionMap right_local;
    ExtensionMap right_shortest;

    std::vector<const std::vector<uint64_t>*> nat_flow;

    uint16_t max_output_action_len;
    uint16_t len_flow_actions;
    uint32_t num_actions;
    boost::shared_array<uint8_t> raw_of;
    ofp_flow_mod *ofm;

    std::ostringstream os;

    Disposition handle_link_change(const Event&);

    // All-pairs shortest path fns

    void cleanup(RoutePtr, bool);
    void clean_subpath(RoutePtr&, const RouteList&, RouteSet&, bool);
    void clean_route(const RoutePtr&, RoutePtr&, bool);

    void fixup(RouteQueue&, bool);
    void add_local_routes(const RoutePtr&, const RoutePtr&,
                          const RoutePtr&, RouteQueue&);

    void set_subpath(RoutePtr&, bool);
    void get_cached_path(RoutePtr&);

    bool remove(RouteMap&, const RoutePtr&);
    bool remove(RoutesMap&, const RoutePtr&);
    bool remove(ExtensionMap&, const RoutePtr&, const RoutePtr&);
    void add(RouteMap&, const RoutePtr&);
    void add(RoutesMap&, const RoutePtr&);
    void add(ExtensionMap&, const RoutePtr&, const RoutePtr&);

    // Flow-in handler that sets up path

    void init_openflow(uint16_t);
    void check_openflow(uint16_t);
    void set_openflow(const Flow&, uint32_t, uint16_t);
    bool set_openflow_actions(const Buffer&, const datapathid&, uint16_t, bool, bool);
    void modify_match(const Buffer&);
    bool set_action(uint8_t*, const datapathid&, uint16_t, uint16_t&, bool check_nat,
                    bool allow_overwrite, uint64_t overwrite_mac, bool& nat_overwritten);
};

}
}

#endif
