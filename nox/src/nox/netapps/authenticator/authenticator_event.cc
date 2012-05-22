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

#include "authenticator.hh"

#include "assert.hh"
#include "datapath-join.hh"
#include "datapath-leave.hh"
#include "netinet++/ethernet.hh"
#include "port-status.hh"
#include "vlog.hh"
#include "kernel.hh"

#if AUTH_WITH_ROUTING
#include "discovery/link-event.hh"
#endif

namespace vigil {
namespace applications {

static Vlog_module lg("authenticator");

Disposition
Authenticator::handle_bootstrap(const Event& e)
{
    routing = ctxt->get_kernel()->get("routing", INSTALLED) != NULL;
    auto_auth = ctxt->get_kernel()->get("sepl_enforcer", INSTALLED) == NULL;
    timeval exp = { expire_timer, 0 };
    post(boost::bind(&Authenticator::expire_entities, this), exp);
    return CONTINUE;
}

Disposition
Authenticator::handle_datapath_join(const Event& e)
{
    const Datapath_join_event& dj = assert_cast<const Datapath_join_event&>(e);
    VLOG_DBG(lg, "Received dp %"PRIx64" join.", dj.datapath_id.as_host());
    activate_switch(dj.datapath_id);
    for (std::vector<Port>::const_iterator iter = dj.ports.begin();
         iter != dj.ports.end(); ++iter)
    {
        activate_location(dj.datapath_id, iter->port_no, iter->name);
    }
    return CONTINUE;
}

Disposition
Authenticator::handle_datapath_leave(const Event& e)
{
    const Datapath_leave_event& dl = assert_cast<const Datapath_leave_event&>(e);
    deactivate_switch(dl.datapath_id, false);
    return CONTINUE;
}

// XXX can modify port name?!
Disposition
Authenticator::handle_port_status(const Event& e)
{
    const Port_status_event& ps = assert_cast<const Port_status_event&>(e);
    if (ps.reason == OFPPR_DELETE) {
        deactivate_location(ps.datapath_id, ps.port.port_no, true);
    } else if (ps.reason == OFPPR_ADD) {
        activate_location(ps.datapath_id, ps.port.port_no, ps.port.name);
    }
    return CONTINUE;
}

Disposition
Authenticator::handle_link_change(const Event& e)
{
#if AUTH_WITH_ROUTING
    const Link_event& le = assert_cast<const Link_event&>(e);
    if (le.action == Link_event::ADD) {
        remove_location_hosts(le.dpdst, le.dport, false,
                              Host_event::INTERNAL_LOCATION, false);
    }
#endif
    return CONTINUE;
}

Disposition
Authenticator::handle_host_auth(const Event& e)
{
    VLOG_DBG(lg, "Host auth event received.");
    const Host_auth_event& ha = assert_cast<const Host_auth_event&>(e);
    if (ha.action == Host_auth_event::AUTHENTICATE) {
        bool success = add_host(ha);
        if (ha.to_post && (success
                           || (ha.reason != Host_event::NWADDR_AUTO_ADD
                               && ha.reason != Host_event::AUTO_AUTHENTICATION)))
        {
            post(ha.to_post);
            const_cast<Host_auth_event&>(ha).to_post = NULL;
        }
    } else if (ha.action == Host_auth_event::DEAUTHENTICATE) {
        remove_host_by_event(ha);
        if (ha.to_post) {
            post(ha.to_post);
            const_cast<Host_auth_event&>(ha).to_post = NULL;
        }
    }
    return CONTINUE;
}

Disposition
Authenticator::handle_user_auth(const Event& e)
{
    VLOG_DBG(lg, "User auth event received.");
    const User_auth_event& ua = assert_cast<const User_auth_event&>(e);
    if (ua.action == User_auth_event::AUTHENTICATE) {
        add_user(ua);
        if (ua.to_post) {
            post(ua.to_post);
            const_cast<User_auth_event&>(ua).to_post = NULL;
        }
    } else if (ua.action == User_auth_event::DEAUTHENTICATE) {
        remove_user(ua.username, ua.hostname, ua.reason);
        if (ua.to_post) {
            post(ua.to_post);
            const_cast<User_auth_event&>(ua).to_post = NULL;
        }
    }
    return CONTINUE;
}

Disposition
Authenticator::handle_packet_in(const Event& e)
{
    const Packet_in_event& pi = assert_cast<const Packet_in_event&>(e);

    Flow flow(htons(pi.in_port), *(pi.buf));
    if (flow.dl_type == ethernet::LLDP) {
        return CONTINUE;
    }

    timeval curtime = { 0, 0 };
    gettimeofday(&curtime, NULL);
    Flow_in_event *fi = new Flow_in_event(curtime, pi, flow);
// enable if want all flows printed
//     if (lg.is_dbg_enabled()) {
//         std::ostringstream os;
//         os << fi->flow;
//         VLOG_DBG(lg, "%s on dp %"PRIx64".", os.str().c_str(),
//                  fi->datapath_id.as_host());
//     }

    if (set_flow_in(*fi, true)) {
        post(fi);
    } else {
        delete fi;
    }
    return CONTINUE;
}

bool
Authenticator::set_flow_in(Flow_in_event &fi, bool packet_in)
{
    if (!set_flow_src(fi, packet_in)) {
        return false;
    }

    set_flow_dst(fi);

    return true;
}

bool
Authenticator::set_flow_src(Flow_in_event& fi, bool packet_in)
{
    bool set_location = true;
    uint32_t nwaddr = ntohl(fi.flow.nw_src);
    uint16_t inport = ntohs(fi.flow.in_port);
    time_t nwaddr_expire = fi.received.tv_sec + NWADDR_TIMEOUT;

    DLEntry *dlentry = get_dladdr_entry(fi.flow.dl_src, nwaddr_expire);
    NWEntry *nwentry = NULL; // delay as much as possible
    if (dlentry != NULL) {
        fi.src_dladdr_groups = dlentry->groups;
        std::list<AuthedLocation>::iterator location;
        if (get_location(fi.datapath_id, inport, dlentry->locations, location)) {
            if (location->location->sw->dp == fi.datapath_id) {
                make_primary(fi.received.tv_sec, dlentry, location);
            }
            DLNWEntry *dlnwentry = get_dlnw_entry(dlentry, nwaddr,
                                                  nwaddr_expire, false);
            if (dlnwentry != NULL && dlnwentry->authed) {
                fi.src_location = *location;
                fi.src_host_netid = dlnwentry->host_netid;
                fi.src_host_netid->host->last_active = fi.received.tv_sec;
                fi.src_nwaddr_groups = dlnwentry->nwaddr_groups;
                return true;
            }
            nwentry = get_nwaddr_entry(nwaddr, nwaddr_expire, true);
            fi.src_nwaddr_groups = nwentry->groups;
            if (dlentry->zero->host_netid->is_router) {
                set_location = false;
                fi.route_source = location->location;
                if (set_source_route_host(nwentry, fi)) {
                    return true;
                }
            } else if (packet_in) {
                VLOG_DBG(lg, "Automatically adding %s on %s.",
                         ipaddr(nwaddr).string().c_str(),
                         dlentry->dladdr.string().c_str());
                Host_auth_event ha(
                    location->location->sw->dp,
                    location->location->port,
                    dlentry->dladdr, nwaddr,
                    dlentry->zero->host_netid->host->name,
                    dlentry->zero->host_netid->name,
                    location->idle_timeout,
                    dlentry->zero->host_netid->host->hard_timeout,
                    Host_event::NWADDR_AUTO_ADD);
                const HostNetEntry *hnentry
                    = get_names_from_bindings(dlentry, nwentry);
                if (hnentry != NULL) {
                    ha.hostname = hnentry->entry->host->name;
                    ha.host_netid = hnentry->entry->name;
                }
                if (add_host(ha)) {
                    return set_flow_src(fi, packet_in);
                }
                fi.src_location = *location;
            } else {
                fi.src_location = *location;
            }
        }
    } else {
        dlentry = new_dladdr(fi.flow.dl_src, nwaddr_expire);
        fi.src_dladdr_groups = dlentry->groups;
    }

    if (auto_auth && packet_in) {
        VLOG_DBG(lg, "Automatically authing %s %s.",
                 fi.flow.dl_src.string().c_str(),
                 ipaddr(nwaddr).string().c_str());
        auth_flow_host(fi, true);
        return false;
    }

    if (fi.src_location.location == NULL) {
        LocEntry *lentry;
        if (set_location) {
            lentry = get_active_location(fi.datapath_id, inport);
            if (lentry == NULL) {
                VLOG_ERR(lg, "Received packet in from inactive "
                         "location %"PRIx64":%"PRIu16", dropping",
                         fi.datapath_id.as_host(), inport);
                return false;
            }
        } else {
            lentry = get_unauth_location();
        }
        fi.src_location.location = lentry->entry;
        fi.src_location.last_active = fi.src_location.idle_timeout = 0;
    }

    if (fi.src_host_netid == NULL) {
        fi.src_host_netid = get_unauth_netid()->entry;
    }

    if (nwentry == NULL) {
        nwentry = get_nwaddr_entry(nwaddr, nwaddr_expire, true);
        fi.src_nwaddr_groups = nwentry->groups;
    }

    return true;
}

void
Authenticator::set_flow_dst(Flow_in_event &fi)
{
   fi.dst_authed = false;
   uint32_t nwaddr = ntohl(fi.flow.nw_dst);
   time_t nwaddr_expire = fi.received.tv_sec + NWADDR_TIMEOUT;

   DLEntry *dlentry = get_dladdr_entry(fi.flow.dl_dst, NWADDR_TIMEOUT);
   NWEntry *nwentry = NULL;  // push off if possible
   DLNWEntry *dlnwentry = NULL;
   if (dlentry != NULL) {
       fi.dst_dladdr_groups = dlentry->groups;
       dlnwentry = get_dlnw_entry(dlentry, nwaddr, nwaddr_expire, false);
       bool cached = dlnwentry != NULL && dlnwentry->host_netid != NULL;
       if (cached && dlnwentry->authed) {
           fi.dst_authed = true;
           fi.dst_host_netid = dlnwentry->host_netid;
           fi.dst_nwaddr_groups = dlnwentry->nwaddr_groups;
           set_destinations(dlentry->locations, fi);
           return;
       }

       bool zero = dlentry->zero != NULL && dlentry->zero->host_netid != NULL;
       if (!zero) {
           cache_host(dlentry, get_nwaddr_entry(0), 0, nwaddr_expire);
           if (nwaddr == 0) {
               dlnwentry = dlentry->zero;
               cached = true;
           }
       }

       if (dlentry->zero->host_netid->is_router && nwaddr != 0) {
           set_route_destinations(dlentry->locations, fi);
           nwentry = get_nwaddr_entry(nwaddr, nwaddr_expire, true);
           fi.dst_nwaddr_groups = nwentry->groups;
           if (set_dest_route_host(nwentry, fi)) {
               return;
           }
           set_empty_destinations(fi);
       } else {
           if (cached) {
               fi.dst_nwaddr_groups = dlnwentry->nwaddr_groups;
           }
           set_destinations(dlentry->locations, fi);
       }

       if (cached) {
           fi.dst_authed = dlnwentry->authed;
           fi.dst_host_netid = dlnwentry->host_netid;
           return;
       }
   } else {
       dlentry = new_dladdr(fi.flow.dl_dst, nwaddr_expire);
       fi.dst_dladdr_groups = dlentry->groups;
       set_empty_destinations(fi);
   }

   if (nwentry == NULL) {
       nwentry = get_nwaddr_entry(nwaddr, nwaddr_expire, true);
       fi.dst_nwaddr_groups = nwentry->groups;
   }
   dlnwentry = cache_host(dlentry, nwentry, nwaddr, nwaddr_expire);
   fi.dst_authed = false;
   fi.dst_host_netid = dlnwentry->host_netid;
}

Authenticator::DLNWEntry*
Authenticator::cache_host(DLEntry *dlentry, const NWEntry *nwentry,
                          uint32_t nwaddr, const time_t& exp_time)
{
    DLNWEntry *dlnwentry = get_dlnw_entry(dlentry, nwaddr, exp_time, true);
    const HostNetEntry *hnnentry
        = get_names_from_bindings(dlentry, nwentry);
    if (hnnentry == NULL) {
        const DLNWEntry *zero = dlentry->zero;
        if (zero != NULL && zero->host_netid != NULL
            && !zero->host_netid->is_router)
        {
            hnnentry = &host_netids[zero->host_netid->name];
        } else {
            hnnentry = get_unauth_netid();
        }
    }
    dlnwentry->host_netid = hnnentry->entry;
    const_cast<HostNetEntry*>(hnnentry)->dlnwentries.push_back(dlnwentry);
    return dlnwentry;
}

void
Authenticator::make_primary(const time_t& curtime, DLEntry *dlentry,
                            std::list<AuthedLocation>::iterator& location)
{
    location->last_active = curtime;
    std::list<AuthedLocation>::iterator begin = dlentry->locations.begin();
    if (location != begin) {
#if AUTH_WITH_ROUTING
        poison_location(location->location->sw->dp, dlentry->dladdr, 0, true);
#endif
        dlentry->locations.splice(begin, dlentry->locations, location);
    }
}

bool
Authenticator::set_source_route_host(NWEntry *nwentry, Flow_in_event& fi)
{
    for (std::list<DLNWEntry*>::iterator iter = nwentry->dlnwentries.begin();
         iter != nwentry->dlnwentries.end(); ++iter)
    {
        if ((*iter)->authed) {
            fi.src_location = (*iter)->dlentry->locations.front();
            fi.src_host_netid = (*iter)->host_netid;
            fi.src_host_netid->host->last_active = fi.received.tv_sec;
            return true;
        }
    }
    return false;
}

bool
Authenticator::set_dest_route_host(NWEntry *nwentry, Flow_in_event& fi)
{
    for (std::list<DLNWEntry*>::iterator iter = nwentry->dlnwentries.begin();
         iter != nwentry->dlnwentries.end(); ++iter)
    {
        if ((*iter)->authed) {
            fi.dst_authed = true;
            fi.dst_host_netid = (*iter)->host_netid;
            set_destinations((*iter)->dlentry->locations, fi);
            return true;
        }
    }
    return false;
}

void
Authenticator::set_destinations(const std::list<AuthedLocation>& als,
                                Flow_in_event& fi)
{
    Flow_in_event::DestinationList non_switch_dst;
    bool found_switch = false;
    for (std::list<AuthedLocation>::const_iterator al = als.begin();
         al != als.end(); ++al)
    {
        // important to use switch locations if !routing to include dst acls in
        // sepl
        if (routing || fi.datapath_id == al->location->sw->dp) {
            Flow_in_event::DestinationInfo dest_info = { *al,
                                                         true,
                                                         std::vector<int64_t>(),
                                                         hash_set<uint32_t>() };
            fi.dst_locations.push_back(dest_info);
            found_switch = true;
        } else if (!found_switch) {
            Flow_in_event::DestinationInfo dest_info = { *al,
                                                         true,
                                                         std::vector<int64_t>(),
                                                         hash_set<uint32_t>() };
            non_switch_dst.push_back(dest_info);
        }
    }
    if (fi.dst_locations.empty()) {
        fi.dst_locations.swap(non_switch_dst);
        if (fi.dst_locations.empty()) {
            set_empty_destinations(fi);
        }
    }
}

void
Authenticator::set_empty_destinations(Flow_in_event& fi)
{
    AuthedLocation al
        = { 0, 0, get_unauth_location()->entry };
    Flow_in_event::DestinationInfo info = { al,
                                            true,
                                            std::vector<int64_t>(),
                                            hash_set<uint32_t>() };
    fi.dst_locations.push_back(info);
}

void
Authenticator::set_route_destinations(const std::list<AuthedLocation>& als,
                                      Flow_in_event& fi)
{
    for (std::list<AuthedLocation>::const_iterator al = als.begin();
         al != als.end(); ++al)
    {
        fi.route_destinations.push_back(al->location);
    }

    if (fi.route_destinations.empty()) {
        fi.route_destinations.push_back(get_unauth_location()->entry);
    }
}

}
}
