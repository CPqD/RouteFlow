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
#include "sprouting.hh"

#include <boost/bind.hpp>
#include <sstream>

#include "assert.hh"
#include "authenticator/flow_util.hh"
#include "bootstrap-complete.hh"
#include "netinet++/ethernet.hh"
#include "vlog.hh"

#define FLOW_TIMEOUT         5
#define BROADCAST_TIMEOUT   60

#define SEPL_PROP_SECTION   "sepl"
#define ENFORCE_POLICY_VAR  "enforce_policy"

namespace vigil {
namespace applications {

static Vlog_module lg("sp_routing");

SPRouting::SPRouting(const container::Context *c, const json_object* d)
    : container::Component(c), routing(NULL), 
      empty(new Routing_module::Route()), passive(false)
{}

void
SPRouting::getInstance(const container::Context * ctxt,
                       SPRouting*& s)
{
    s = dynamic_cast<SPRouting*>
        (ctxt->get_by_interface(container::Interface_description
                                (typeid(SPRouting).name())));
}


void
SPRouting::configure(const container::Configuration*)
{
    resolve(routing);
    register_handler<Bootstrap_complete_event>
        (boost::bind(&SPRouting::handle_bootstrap, this, _1));
    register_handler<Flow_in_event>
        (boost::bind(&SPRouting::handle_flow_in, this, _1));
}

bool
validate_args(const std::vector<std::string>& args,
              bool& is_source, ethernetaddr& eth, ipaddr& ip,
              uint16_t& tport, bool& check_nat)
{
    if (args.size() != 5) {
        VLOG_ERR(lg, "Incorrect number of arguments %zu", args.size());
        return false;
    }

    bool at_least_one = false;

    if (args[0] == "SRC") {
        is_source = true;
    } else if (args[0] == "DST") {
        is_source = false;
    } else {
        VLOG_ERR(lg, "Neither source nor destination specified by %s.",
                 args[0].c_str());
        return false;
    }

    try {
        if (args[1] == "") {
            eth = ethernetaddr((uint64_t)0);
        } else {
            eth = ethernetaddr(args[1]);
            at_least_one = true;
        }
    } catch (...) {
        VLOG_ERR(lg, "Invalid ethernet address %s.", args[1].c_str());
        return false;
    }

    try {
        if (args[2] == "") {
            ip = ipaddr((uint32_t)0);
        } else {
            ip = ipaddr(args[2]);
            at_least_one = true;
        }
    } catch (...) {
        VLOG_ERR(lg, "Invalid IP address %s.", args[2].c_str());
        return false;
    }

    if (args[3] == "") {
        tport = 0;
    } else {
        const char *tstr = args[3].c_str();
        while (*tstr != '\0') {
            if (!isdigit(*(tstr++))) {
                VLOG_ERR(lg, "Invalid transport port string %s.",
                         args[3].c_str());
                return false;
            }
        }

        uint32_t val = atoi(args[3].c_str());
        if (val > 0xffff) {
            VLOG_ERR(lg, "Transport port %"PRIu32" is too large.", val);
            return false;
        }

        tport = (uint16_t) val;
        at_least_one = true;
    }

    if (args[4] == "CHECK_NAT") {
        check_nat = true;
    } else if (args[4] == "NO_NAT") {
        check_nat = false;
    } else {
        VLOG_ERR(lg, "Neither CHECK_NAT or NO_NAT specified by %s.",
                 args[4].c_str());
        return false;
    }

    if (!at_least_one) {
        VLOG_ERR(lg, "Need to specify at least one overwrite argument.");
        return false;
    }

    return true;
}

bool
validate_redirect_args(const std::vector<std::string>& args)
{
    bool is_source;
    ethernetaddr eth;
    ipaddr ip;
    uint16_t tport;
    bool check_nat;

    std::vector<std::string> copy(args);
    copy.insert(copy.begin(), "SRC");
    copy.push_back("NO_NAT");

    return validate_args(copy, is_source, eth, ip, tport, check_nat);
}

void
SPRouting::install()
{
    Flow_util *flow_util;
    resolve(flow_util);
    bool is_source = false;
    ethernetaddr eth;
    ipaddr ip;
    uint16_t tport = 0;
    bool check_nat = false;
    flow_util->fns.register_function("rewrite_headers",
                                     boost::bind(&SPRouting::generate_rewrite,
                                                 this, _1, ""),
                                     boost::bind(validate_args, _1,
                                                 is_source, eth, ip, tport, check_nat));
    flow_util->fns.register_function("http_proxy_redirect",
                                     boost::bind(&SPRouting::generate_rewrite,
                                                 this, _1, "DST"),
                                     boost::bind(validate_redirect_args, _1));
    flow_util->fns.register_function("http_proxy_undo_redirect",
                                     boost::bind(&SPRouting::generate_rewrite,
                                                 this, _1, "SRC"),
                                     boost::bind(validate_redirect_args, _1));
    flow_util->fns.register_function("allow_no_nat",
                                     boost::bind(&SPRouting::route_no_nat,
                                                 this, _1));
}

Disposition
SPRouting::handle_bootstrap(const Event& e)
{
    update_passive(); // Currently does nothing.  
                      // In the future can set whether
                      // running in passive mode
    return CONTINUE;
}


void
SPRouting::update_passive()
{
  // To set in passive mode, set passive bool to true
    //     passive = true;
}

Disposition
SPRouting::handle_flow_in(const Event& e)
{
    const Flow_in_event& flow_in = assert_cast<const Flow_in_event&>(e);
    Flow_in_event& fi = const_cast<Flow_in_event&>(flow_in);

    if (!fi.active) {
        return CONTINUE;
    }

    return route_flow(fi, Nonowning_buffer(), true);
}

Disposition
SPRouting::route_flow(Flow_in_event& fi, const Buffer& actions,
                      bool check_nat)
{
    Routing_module::RoutePtr route;
    uint16_t inport, outport;
    check_nat = check_nat && fi.flow.dl_type == ethernet::IP;
    if (!set_route(fi, route, inport, outport, actions, check_nat)) {
        //Cannot ignore packets, so flood.
        routing->send_packet(fi.datapath_id, ntohs(fi.flow.in_port),
			     OFPP_FLOOD, fi.buffer_id, *(fi.buf),
                             Nonowning_buffer(), check_nat,
                             fi.flow, NULL, NULL, NULL, NULL);
        return CONTINUE;
    }

    Routing_module::ActionList alist;
    if (actions.size() > 0) {
        alist.resize(route->path.size());
        alist.push_front(actions);
    }

    routing->setup_route(fi.flow, *route, inport, outport, FLOW_TIMEOUT,
                         alist, check_nat,
                         fi.src_dladdr_groups.get(),
                         fi.src_nwaddr_groups.get(),
                         fi.dst_dladdr_groups.get(),
                         fi.dst_nwaddr_groups.get());

    bool on_route = (fi.datapath_id == route->id.src)
        || (fi.datapath_id == route->id.dst);

    if (!on_route) {
        for (std::list<Routing_module::Link>::const_iterator link = route->path.begin();
             link != route->path.end(); ++link)
        {
            if (fi.datapath_id == link->dst) {
                on_route = true;
                break;
            }
        }
    }

    if (on_route) {
        routing->send_packet(fi.datapath_id, ntohs(fi.flow.in_port),
                             OFPP_TABLE, fi.buffer_id, *(fi.buf),
                             Nonowning_buffer(), false,
                             fi.flow, NULL, NULL, NULL, NULL);
    } else {
        if (lg.is_dbg_enabled()) {
            std::ostringstream os;
            os << fi.flow;
            VLOG_DBG(lg, "Packet not on route - flooding %s", os.str().c_str());
        }
        routing->send_packet(fi.datapath_id, ntohs(fi.flow.in_port),
                             OFPP_FLOOD, fi.buffer_id, *(fi.buf),
                             Nonowning_buffer(), check_nat,
                             fi.flow, NULL, NULL, NULL, NULL);
    }

    return CONTINUE;
}

bool
SPRouting::set_route(Flow_in_event& fi,
                     Routing_module::RoutePtr& route,
                     uint16_t& inport, uint16_t& outport,
                     const Buffer& actions, bool check_nat)
{
    if (fi.route_source == NULL) {
        empty->id.src = fi.src_location.location->sw->dp;
        inport = fi.src_location.location->port;
    } else {
        empty->id.src = fi.route_source->sw->dp;
        inport = fi.route_source->port;
    }

    bool use_dst;
    if (fi.route_destinations.empty()) {
        use_dst = true;
    } else {
        use_dst = false;
    }

    fi.routed_to = 0;
    bool checked = false;
    while (true) {
        if (use_dst) {
            if (fi.routed_to >= fi.dst_locations.size()) {
                break;
            }
            const Flow_in_event::DestinationInfo& dst = fi.dst_locations[fi.routed_to];
            if (dst.allowed || passive) {
                empty->id.dst = dst.authed_location.location->sw->dp;
                outport = dst.authed_location.location->port;
            } else {
                empty->id.dst = datapathid::from_host(0);
            }
        } else if (fi.routed_to >= fi.route_destinations.size()) {
            break;
        } else {
            empty->id.dst = fi.route_destinations[fi.routed_to]->sw->dp;
            outport = fi.route_destinations[fi.routed_to]->port;
        }

        if (empty->id.dst.as_host() != 0) {
            bool check = false;
            if (empty->id.src == empty->id.dst) {
                route = empty;
                check = true;
            } else {
                check = routing->get_route(empty->id, route);
            }

            if (check) {
                checked = true;
                if (routing->check_route(*route, inport, outport)) {
                    return true;
                }
                VLOG_DBG(lg, "Invalid route between aps %"PRIx64":%"PRIu16" and %"PRIx64":%"PRIu16".",
                          empty->id.src.as_host(), inport, empty->id.dst.as_host(), outport);
            } else {
                VLOG_DBG(lg, "No route found between dps %"PRIx64" and %"PRIx64".",
                          empty->id.src.as_host(), empty->id.dst.as_host());
            }
        }

        ++fi.routed_to;
    }

//   If failed on check_route(), don't route...correct?
    if (checked) {
        fi.routed_to = Flow_in_event::NOT_ROUTED;
        VLOG_WARN(lg, "Could not find valid route to any destination.");
        std::ostringstream os;
        os << fi.flow;
        VLOG_WARN(lg, "Dropping %s", os.str().c_str());
        return false;
    }

    if (lg.is_dbg_enabled()) {
        std::ostringstream os;
        os << fi.flow;
        VLOG_DBG(lg, "Broadcasting %"PRIx64" %s",
                 fi.datapath_id.as_host(), os.str().c_str());
    }

    fi.routed_to = Flow_in_event::BROADCASTED;
    if (fi.flow.dl_dst.is_broadcast()) {
        routing->setup_flow(fi.flow, fi.datapath_id, OFPP_FLOOD,
                            fi.buffer_id, *(fi.buf), BROADCAST_TIMEOUT,
                            actions, check_nat,
                            fi.src_dladdr_groups.get(),
                            fi.src_nwaddr_groups.get(),
                            fi.dst_dladdr_groups.get(),
                            fi.dst_nwaddr_groups.get());
    } else {
        routing->send_packet(fi.datapath_id, ntohs(fi.flow.in_port), OFPP_FLOOD,
                             fi.buffer_id, *(fi.buf), actions, check_nat, fi.flow,
                             fi.src_dladdr_groups.get(),
                             fi.src_nwaddr_groups.get(),
                             fi.dst_dladdr_groups.get(),
                             fi.dst_nwaddr_groups.get());
    }

    return false;
}


Flow_fn_map::Flow_fn
SPRouting::generate_rewrite(const std::vector<std::string>& args,
                            const std::string& redir_side)
{
    Flow_fn_map::Flow_fn empty_fn;
    bool is_source;
    ethernetaddr eth;
    ipaddr ip;
    uint16_t tport;
    bool check_nat;

    std::vector<std::string> copy(args);

    if (redir_side != "") {
        copy.insert(copy.begin(), redir_side);
        copy.push_back("NO_NAT");
    }

    if (!validate_args(copy, is_source, eth, ip, tport, check_nat)) {
        return empty_fn;
    }

    uint64_t eth_hb = eth.hb_long();
    uint32_t buf_size = (eth_hb == 0 ? 0 : sizeof(ofp_action_dl_addr))
        + (ip.addr == 0 ? 0 : sizeof(ofp_action_nw_addr))
        + (tport == 0 ? 0 : sizeof(ofp_action_tp_port));

    boost::shared_array<uint8_t> buf(new uint8_t[buf_size]);
    uint32_t ofs = 0;

    if (eth_hb != 0) {
        ofp_action_dl_addr *dl = (ofp_action_dl_addr*)(buf.get() + ofs);
        if (is_source) {
            dl->type = htons(OFPAT_SET_DL_SRC);
        } else {
            dl->type = htons(OFPAT_SET_DL_DST);
        }
        dl->len = htons(sizeof(ofp_action_dl_addr));
        memcpy(dl->dl_addr, eth.octet, ethernetaddr::LEN);
        ofs += sizeof(ofp_action_dl_addr);
    }

    if (ip.addr != 0) {
        ofp_action_nw_addr *nw = (ofp_action_nw_addr*)(buf.get() + ofs);
        if (is_source) {
            nw->type = htons(OFPAT_SET_NW_SRC);
        } else {
            nw->type = htons(OFPAT_SET_NW_DST);
        }
        nw->len = htons(sizeof(ofp_action_nw_addr));
        nw->nw_addr = ip.addr;
        ofs += sizeof(ofp_action_nw_addr);
    }

    if (tport != 0) {
        ofp_action_tp_port *tp = (ofp_action_tp_port*)(buf.get() + ofs);
        if (is_source) {
            tp->type = htons(OFPAT_SET_TP_SRC);
        } else {
            tp->type = htons(OFPAT_SET_TP_DST);
        }
        tp->len = htons(sizeof(ofp_action_tp_port));
        tp->tp_port = htons(tport);
    }

    return boost::bind(&SPRouting::rewrite_and_route, this,
                       _1, Nonowning_buffer(buf.get(), buf_size),
                       buf, check_nat);
}

void
SPRouting::rewrite_and_route(const Flow_in_event& ev,
                             const Buffer& actions,
                             const boost::shared_array<uint8_t>& buf,
                             bool check_nat)
{
    Flow_in_event& flow_in =
        const_cast<Flow_in_event&>(ev);

    Flow_in_event::DestinationList::reverse_iterator iter = flow_in.dst_locations.rbegin();
    for (; iter != flow_in.dst_locations.rend(); ++iter) {
        if (!iter->allowed) {
            iter->allowed = true;
            break;
        }
    }
    route_flow(flow_in, actions, check_nat);
    if (iter != flow_in.dst_locations.rend()) {
        iter->allowed = false;
    }
}

void
SPRouting::route_no_nat(const Flow_in_event& ev)
{
    boost::shared_array<uint8_t> empty;
    rewrite_and_route(ev, Nonowning_buffer(), empty, false);
}

}
}

REGISTER_COMPONENT(vigil::container::Simple_component_factory
                   <vigil::applications::SPRouting>,
                   vigil::applications::SPRouting);
