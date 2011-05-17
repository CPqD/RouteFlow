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
#include "nox.hh"
#include "switch-mgr.hh"
#include "switch_event.hh"

#define HOST_TIMEOUT           300

namespace vigil {
namespace applications {

static Vlog_module lg("authenticator");
static const std::string app_name("authenticator");

// ----------------------------------------------------------------------------
// Perform host authentication.
// ----------------------------------------------------------------------------

bool
Authenticator::add_host(const Host_auth_event& ha)
{
    LocEntry *location = get_active_location(ha.datapath_id, ha.port);
    if (location == NULL) {
        VLOG_ERR(lg, "Cannot authenticate host %s, location %"PRIx64""
                 ":%"PRIu16" not active.",
                 get_netid_name(ha.host_netid).c_str(),
                 ha.datapath_id.as_host(), ha.port);
        return false;
    }

    HostNetMap::iterator hniter = host_netids.find(ha.host_netid);
    if (hniter == host_netids.end()) {
        VLOG_ERR(lg, "Cannot authenticate netid %"PRId64", does not exist.",
                 ha.host_netid);
        return false;
    }
    HostNetEntry *netid = &hniter->second;

    HostMap::iterator hiter = hosts.find(netid->entry->host->name);
    if (hiter == hosts.end() || hiter->first != ha.hostname) {
        VLOG_ERR(lg, "Netid %"PRId64" and hostname %"PRId64" inconsistent.",
                 ha.host_netid, ha.hostname);
    }
    HostEntry *host = &hiter->second;

    timeval curtime = { 0, 0 };
    gettimeofday(&curtime, NULL);

    DLNWEntry *dlnwentry = get_dlnw_entry(ha.dladdr, ha.nwaddr,
                                          curtime.tv_sec + NWADDR_TIMEOUT,
                                          true);

    return add_host2(ha, location, netid, host, dlnwentry, curtime);
}

bool
Authenticator::add_host2(const Host_auth_event& ha, LocEntry *location,
                         HostNetEntry *netid, HostEntry *host,
                         DLNWEntry *dlnwentry, const timeval& curtime)
{
    // Remove any host currently authenticated at addresses

    if (dlnwentry->host_netid != NULL
        && (!dlnwentry->authed || dlnwentry->host_netid != netid->entry))
    {
        remove_dlnw_host(dlnwentry, Host_event::AUTHENTICATION_EVENT,
                         dlnwentry->host_netid != netid->entry);
    }

    // Automatically authenticate nwaddr == 0 if not already.

    if (ha.nwaddr != 0 && !auth_zero(ha, location, netid,
                                     host, dlnwentry, curtime))
    {
        return false;
    }

    std::string dlstr = ha.dladdr.string();

    // add host to nw entry
    if (dlnwentry->host_netid == NULL) {
        dlnwentry->authed = true;
        dlnwentry->host_netid = netid->entry;

        // host join if no interfaces authed
        bool host_join = true;
        std::list<HostNetEntry*>::const_iterator niter
            = host->netids.begin();
        for (; niter != host->netids.end(); ++niter) {
            if (!(*niter)->dlnwentries.empty()
                && (*niter)->dlnwentries.front()->authed)
            {
                host_join = false;
                break;
            }
        }

        const std::string& hostname = get_host_name(ha.hostname);
        std::string nwstr = ipaddr(ha.nwaddr).string();

        if (host_join) {
            host->entry->hard_timeout = ha.hard_timeout;
            host->entry->auth_time = host->entry->last_active = curtime.tv_sec;
            post(new Host_join_event(Host_join_event::JOIN,
                                     ha.hostname, ha.reason));
            snprintf(buf, 1024, "{sh} joined the network.");
            LogEntry to_log(app_name, LogEntry::INFO, buf);
            to_log.setName(ha.hostname, datatypes->host_type(), LogEntry::SRC);
            user_log->log(to_log);
        }

        netid->dlnwentries.push_front(dlnwentry);
        if (ha.nwaddr != 0) {
            VLOG_DBG(lg, "Added %s %s to host %s.",
                     dlstr.c_str(), nwstr.c_str(), hostname.c_str());
            snprintf(buf, 1024, "{sh} authenticated%s %s.",
                     ha.nwaddr == 0 ? "" : (" " + nwstr + " on").c_str(),
                     dlstr.c_str());
            LogEntry to_log(app_name, LogEntry::INFO, buf);
            to_log.setName(ha.hostname, datatypes->host_type(), LogEntry::SRC);
            user_log->log(to_log);
            // trigger attr changes
            std::list<AuthedLocation>::iterator al
                = dlnwentry->dlentry->locations.begin();
            for (; al != dlnwentry->dlentry->locations.end(); ++al) {
                call_updated_fns(al->location, dlnwentry, true);
            }
        }
        bindings->store_host_binding(ha.hostname, ha.dladdr, ha.nwaddr);
        post(new Host_bind_event(Host_bind_event::ADD, ha.dladdr,
                                 ha.nwaddr, ha.hostname,
                                 ha.host_netid, ha.reason));
    }
    add_host_location(ha, location, host, dlnwentry->dlentry, curtime);
    return true;
}

bool
Authenticator::auth_zero(const Host_auth_event& ha, LocEntry *location,
                         HostNetEntry *netid, HostEntry *host,
                         DLNWEntry *dlnwentry, const timeval& curtime)
{
    DLEntry *dlentry = dlnwentry->dlentry;
    bool is_router;
    bool auth_zero;
    DLBinding *binding = NULL;
    if (dlentry->zero != NULL && dlentry->zero->authed) {
        is_router = dlentry->zero->host_netid->is_router;
        auth_zero = !is_router
            && dlentry->zero->host_netid != netid->entry;
    } else {
        auth_zero = true;
        if (!dlentry->bindings.empty()) {
            binding = &dlentry->bindings.front();
            is_router = binding->netid->entry->is_router;
        } else {
            is_router = false;
        }
    }

    if (auth_zero) {
        bool success;
        Host_auth_event& ha2 = const_cast<Host_auth_event&>(ha);
        ha2.nwaddr = 0;
        if (is_router) {
            ha2.host_netid = binding->netid->entry->name;
            ha2.hostname = binding->netid->entry->host->name;
            success = add_host(ha2);
            ha2.host_netid = netid->entry->name;
            ha2.hostname = host->entry->name;
        } else {
            if (dlentry->zero == NULL) {
                new_dlnw_entry(dlentry, 0, curtime.tv_sec + NWADDR_TIMEOUT);
            }
            success = add_host2(ha2, location, netid, host,
                                dlentry->zero, curtime);
        }
        ha2.nwaddr = dlnwentry->nwaddr;
        if (!success) {
            return false;
        }
    }
    return true;
}

void
Authenticator::add_host_location(const Host_auth_event& ha, LocEntry *location,
                                 HostEntry *host, DLEntry *dlentry,
                                 const timeval& curtime)
{
    // Set host's hard_timeout to most relaxed

    if (host->entry->hard_timeout != 0
        && (ha.hard_timeout > host->entry->hard_timeout
            || ha.hard_timeout == 0))
    {
        host->entry->hard_timeout = ha.hard_timeout;
    }

    // Add location if not already present

    std::string dlstr = ha.dladdr.string();
    std::list<AuthedLocation>::iterator authed;
    std::vector<std::pair<datapathid, uint16_t> > to_remove;
#if AUTH_WITH_ROUTING
    Routing_module::RouteId rid, rid2;
    rid.src = rid2.dst = ha.datapath_id;
#endif
    for (authed = dlentry->locations.begin();
         authed != dlentry->locations.end(); ++authed)
    {
        // if present, update timeout and return
        if (authed->location == location->entry) {
            if (authed->idle_timeout != 0
                && (ha.idle_timeout > authed->idle_timeout
                    || ha.idle_timeout == 0))
            {
                authed->idle_timeout = ha.idle_timeout;
            }
            return;
        // if virtual, auto delete all other locations
        } else if (location->entry->is_virtual) {
            if (!authed->location->is_virtual) {
                to_remove.push_back(std::make_pair(authed->location->sw->dp,
                                                   authed->location->port));
            }
        // if other location is virtual, ignore this one
        } else if (authed->location->is_virtual) {
            VLOG_DBG(lg, "%s not adding location %s because virtual exists.",
                     dlstr.c_str(),
                     get_location_name(location->entry->name).c_str());
            return;
        // if upstream, remove other.  if downstream, dont add and return
#if AUTH_WITH_ROUTING
        } else { // want if routing?
            rid.dst = rid2.src = authed->location->sw->dp;
            if (routing_mod->is_on_path_location(rid, ha.port,
                                                 authed->location->port))
            {
                to_remove.push_back(std::make_pair(authed->location->sw->dp,
                                                   authed->location->port));
            } else if (routing_mod->is_on_path_location(rid2,
                                                        authed->location->port,
                                                        ha.port))
            {
                return;
            }
#endif
        }
    }

    AuthedLocation authed_loc = { curtime.tv_sec,
                                  ha.idle_timeout,
                                  location->entry };
    dlentry->locations.push_back(authed_loc);
    location->dlentries.push_back(dlentry);

    // Post events

    const std::string& hostname
        = get_host_name(dlentry->zero->host_netid->host->name);
    const std::string& locname = get_location_name(location->entry->name);
    VLOG_DBG(lg, "%s added location %s to %s.",
             hostname.c_str(), locname.c_str(), dlstr.c_str());
    bindings->store_location_binding(ha.dladdr, location->entry->name);
    post(new Host_bind_event(Host_bind_event::ADD, ha.datapath_id,
                             ha.port, location->entry->sw->name,
                             location->entry->name, ha.dladdr,
                             dlentry->zero->host_netid->host->name,
                             dlentry->zero->host_netid->name, ha.reason));
    snprintf(buf, 1024, "{sh} authenticated %s on {sl}.", dlstr.c_str());
    LogEntry to_log(app_name, LogEntry::INFO, buf);
    to_log.setName(ha.hostname, datatypes->host_type(), LogEntry::SRC);
    to_log.setName(location->entry->name, datatypes->location_type(),
                   LogEntry::SRC);
    user_log->log(to_log);

    // trigger attr changes
    call_updated_fns(location->entry, dlentry, true);

    // Remove any internal locations on path of new location
    std::vector<std::pair<datapathid, uint16_t> >::const_iterator iter
        = to_remove.begin();
    for (; iter != to_remove.end(); ++iter) {
        remove_dl_location(dlentry, iter->first, iter->second, false,
                           Host_event::INTERNAL_LOCATION,
                           location->entry->is_virtual);
    }
}

// ----------------------------------------------------------------------------
// Deauthenticates a location on a dladdr, poisoning the location for the
// address if 'poison' == true.  'reason' should signal why location is being
// removed.  If dlentry's location list is empty after the removal, all hosts
// on dladdr are deauthenticated.
// ----------------------------------------------------------------------------

void
Authenticator::remove_dl_location(DLEntry *dlentry,
                                  std::list<AuthedLocation>::iterator& authed,
                                  Host_event::Reason reason, bool poison)
{
    datapathid dp = authed->location->sw->dp;
    uint16_t port = authed->location->port;

    int64_t swid = authed->location->sw->name;
    int64_t locid = authed->location->name;
    int64_t hostid = dlentry->zero->host_netid->host->name;
    const std::string& locname = get_location_name(locid);
    const std::string& hostname = get_host_name(hostid);
    std::string dlstr = dlentry->dladdr.string();
    const char *reason_str = Host_event::get_reason_string(reason);

    bool not_found = true;
    LocMap::iterator liter = locations.find(authed->location->name);
    LocEntry *lentry = get_active_location(authed->location->sw->dp,
                                           authed->location->port);
    if (lentry != NULL) {
        std::list<DLEntry*>::iterator diter = lentry->dlentries.begin();
        for (; diter != lentry->dlentries.end(); ++diter) {
            if (*diter == dlentry) {
                lentry->dlentries.erase(diter);
                not_found = false;
                break;
            }
        }
    }

    if (not_found) {
        VLOG_ERR(lg, "DLEntry %s not found in remove location %s entry.",
                 dlstr.c_str(), locname.c_str());
    }

    // Post events

    VLOG_DBG(lg, "Removed location %s from %s on host %s. (%s)",
             locname.c_str(), dlstr.c_str(), hostname.c_str(), reason_str);
    bindings->remove_location_binding(dlentry->dladdr, authed->location->name);
    post(new Host_bind_event(Host_bind_event::REMOVE, dp, port,
                             swid, locid, dlentry->dladdr,
                             dlentry->zero->host_netid->host->name,
                             dlentry->zero->host_netid->name, reason));
    snprintf(buf, 1024, "{sh} deauthenticated %s from {sl}. (%s)",
             dlstr.c_str(), reason_str);
    LogEntry to_log(app_name, LogEntry::INFO, buf);
    to_log.setName(hostid, datatypes->host_type(), LogEntry::SRC);
    to_log.setName(locid, datatypes->location_type(), LogEntry::SRC);
    user_log->log(to_log);

    authed = dlentry->locations.erase(authed);

    if (poison) {
        poison_location(dp, dlentry->dladdr, 0, true);
    }

    // deauthenticate dladdr
    if (dlentry->locations.empty()) {
        remove_dlnw_host(dlentry->zero, reason, poison);
    }
}

// ----------------------------------------------------------------------------
// Deauthenticates location 'loc' on a dladdr, deauthenticating all of dladdr's
// locations if 'loc' == 0.
// ----------------------------------------------------------------------------

void
Authenticator::remove_dl_location(DLEntry *dlentry, const datapathid& dp,
                                  uint16_t port, bool ignore_port,
                                  Host_event::Reason reason, bool poison)
{
    bool remove_all = dp.as_host() == 0;
    std::list<AuthedLocation>::iterator authed;
    for (authed = dlentry->locations.begin();
         authed != dlentry->locations.end();)
    {
        if (remove_all
            || (authed->location->sw->dp == dp
                && (ignore_port || authed->location->port == port)))
        {
            remove_dl_location(dlentry, authed, reason, poison);
            if (!(ignore_port || remove_all)) {
                break;
            }
        } else {
            ++authed;
        }
    }
}

// ----------------------------------------------------------------------------
// Deauthenticates host on dladdr/nwaddr pair, poisoning the authenticated
// locations if 'poison' == true.  If nwaddr == 0, all nwaddrs and locations on
// the dladdr are deauthenticated.  'reason' should signal why the host is
// being deauthenticated.
// ----------------------------------------------------------------------------
void
Authenticator::remove_dlnw_host(DLNWEntry *dlnwentry,
                                Host_event::Reason reason, bool poison)
{
    if (dlnwentry->host_netid == NULL) {
        return;
    }

    if (dlnwentry->nwaddr == 0) {
        if (!dlnwentry->dlentry->locations.empty()) {
            remove_dl_location(dlnwentry->dlentry, datapathid::from_host(0),
                               0, true, reason, poison);
            return;
        }
        DLNWMap::iterator nwiter = dlnwentry->dlentry->dlnwentries.begin();
        for (; nwiter != dlnwentry->dlentry->dlnwentries.end(); ++nwiter) {
            if (nwiter->first != 0) {
                remove_dlnw_host(&nwiter->second, reason, poison);
            }
        }
    }

    HostNetEntry *netid = &host_netids[dlnwentry->host_netid->name];
    bool not_found = true;
    for (std::list<DLNWEntry*>::iterator niter = netid->dlnwentries.begin();
         niter != netid->dlnwentries.end(); ++niter)
    {
        if (*niter == dlnwentry) {
            netid->dlnwentries.erase(niter);
            not_found = false;
            break;
        }
    }
    if (not_found) {
        VLOG_ERR(lg, "DLNWEntry %s %s not found in remove netid entry %s.",
                 dlnwentry->dlentry->dladdr.string().c_str(),
                 ipaddr(dlnwentry->nwaddr).string().c_str(),
                 get_netid_name(dlnwentry->host_netid->name).c_str());
    }

    if (!dlnwentry->authed) {
        dlnwentry->host_netid.reset();
        return;
    }

    dlnwentry->authed = false;
    bindings->remove_host_binding(dlnwentry->host_netid->host->name,
                                  dlnwentry->dlentry->dladdr,
                                  dlnwentry->nwaddr);
    post(new Host_bind_event(Host_bind_event::REMOVE,
                             dlnwentry->dlentry->dladdr, dlnwentry->nwaddr,
                             dlnwentry->host_netid->host->name,
                             dlnwentry->host_netid->name, reason));
    int64_t hostid = dlnwentry->host_netid->host->name;
    const std::string& hostname = get_host_name(hostid);
    const char *reason_str = Host_event::get_reason_string(reason);
    if (dlnwentry->nwaddr != 0) {
        std::string dlstr = dlnwentry->dlentry->dladdr.string();
        std::string nwstr = ipaddr(dlnwentry->nwaddr).string();
        VLOG_DBG(lg, "Removed nwaddr %s from %s on %s. (%s)",
                 nwstr.c_str(), hostname.c_str(), dlstr.c_str(),
                 reason_str);
        snprintf(buf, 1024, "{sh} deauthenticated %s from %s. (%s)",
                 nwstr.c_str(), dlstr.c_str(), reason_str);
        LogEntry to_log(app_name, LogEntry::INFO, buf);
        to_log.setName(hostid, datatypes->host_type(), LogEntry::SRC);
        user_log->log(to_log);
    }

    bool host_leave = true;
    HostEntry *host = &hosts[netid->entry->host->name];
    std::list<HostNetEntry*>::const_iterator niter
        = host->netids.begin();
    for (; niter != host->netids.end(); ++niter) {
        if (!(*niter)->dlnwentries.empty()
            && (*niter)->dlnwentries.front()->authed)
        {
            host_leave = false;
            break;
        }
    }

    if (host_leave) {
        post(new Host_join_event(Host_join_event::LEAVE,
                                 dlnwentry->host_netid->host->name,
                                 reason));
        snprintf(buf, 1024, "{sh} left the network. (%s)",
                 reason_str);
        LogEntry to_log(app_name, LogEntry::INFO, buf);
        to_log.setName(hostid, datatypes->host_type(), LogEntry::SRC);
        user_log->log(to_log);
        host->entry->auth_time = host->entry->last_active
            = host->entry->hard_timeout = 0;
    }

    if (poison) {
        std::list<AuthedLocation>::iterator authed;
        for (authed = dlnwentry->dlentry->locations.begin();
             authed != dlnwentry->dlentry->locations.end(); ++authed)
        {
            poison_location(authed->location->sw->dp,
                            dlnwentry->dlentry->dladdr, dlnwentry->nwaddr,
                            dlnwentry->nwaddr == 0);
        }
    }

    dlnwentry->host_netid.reset();
}

// ----------------------------------------------------------------------------
// Returns true if anything was removed.
// ----------------------------------------------------------------------------

bool
Authenticator::remove_dlnw_location(DLNWEntry *dlnwentry, const datapathid& dp,
                                    uint16_t port, bool ignore_port,
                                    Host_event::Reason reason)
{
    if (dp.as_host() == 0) {
        remove_dlnw_host(dlnwentry, reason, true);
    } else if (dlnwentry->authed
               && dlnwentry->host_netid
               == dlnwentry->dlentry->zero->host_netid)
    {
        remove_dl_location(dlnwentry->dlentry, dp, port,
                           ignore_port, reason, true);
    } else {
        std::list<AuthedLocation>::iterator authed;
        for (authed = dlnwentry->dlentry->locations.begin();
             authed != dlnwentry->dlentry->locations.end(); ++authed)
        {
            if (authed->location->sw->dp == dp
                && (ignore_port || port == authed->location->port))
            {
                remove_dlnw_host(dlnwentry, reason, true);
                return true;
            }
        }
        return false;
    }
    return true;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void
Authenticator::remove_host(HostEntry *hentry, const datapathid& dp,
                           uint16_t port, bool ignore_port,
                           Host_event::Reason reason)
{
    std::list<HostNetEntry*>::const_iterator niter = hentry->netids.begin();
    for (; niter != hentry->netids.end(); ++niter) {
        remove_host_netid(*niter, dp, port, ignore_port, reason);
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void
Authenticator::remove_host_netid(HostNetEntry *netid, const datapathid& dp,
                                 uint16_t port, bool ignore_port,
                                 Host_event::Reason reason)
{
    std::list<DLNWEntry*> dlist = netid->dlnwentries;
    for (std::list<DLNWEntry*>::iterator diter = dlist.begin();
         diter != dlist.end(); ++diter)
    {
        remove_dlnw_location(*diter, dp, port, ignore_port, reason);
    }
}


// ----------------------------------------------------------------------------
// Deauthenticates any hosts on location 'loc'.
// ----------------------------------------------------------------------------

void
Authenticator::remove_location_hosts(LocEntry *lentry,
                                     Host_event::Reason reason,
                                     bool poison)
{
    for (std::list<DLEntry*>::iterator diter = lentry->dlentries.begin();
         diter != lentry->dlentries.end(); )
    {
        DLEntry *dlentry = *diter;
        ++diter;
        remove_dl_location(dlentry, lentry->entry->sw->dp, lentry->entry->port,
                           false, reason, poison);
    }

    if (!lentry->dlentries.empty()) {
        VLOG_ERR(lg, "Expect any dlentries to still exist in location "
                 "%"PRIx64":%"PRIu16"?", lentry->entry->sw->dp.as_host(),
                 lentry->entry->port);
    }
}

void
Authenticator::remove_location_hosts(const datapathid& dp,
                                     uint16_t port,
                                     bool ignore_port,
                                     Host_event::Reason reason,
                                     bool poison)
{
    uint64_t dp_host = dp.as_host();
    if (dp_host == 0) {
        for (LocMap::iterator liter = locations.begin();
             liter != locations.end(); ++liter)
        {
            remove_location_hosts(&liter->second, reason, poison);
        }
        return;
    }

    DPMap::iterator siter = switches_by_dp.find(dp_host);
    if (siter == switches_by_dp.end() || siter->second.active_switch == NULL) {
        return;
    }

    SwitchEntry *active_switch = siter->second.active_switch;
    std::list<PortEntry>::iterator piter = active_switch->locations.begin();
    for (; piter != active_switch->locations.end(); ++piter) {
        if (ignore_port || piter->port == port) {
            if (piter->active_location != NULL) {
                remove_location_hosts(piter->active_location, reason, poison);
            }
            if (!ignore_port) {
                break;
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Removes any authentications matching the passed in criteria.  Certain values
// can be wildcarded (see Host_auth_event).
// ----------------------------------------------------------------------------

void
Authenticator::remove_host_by_event(const Host_auth_event& ha)
{
    if ((ha.enabled_fields & Host_auth_event::EF_DLADDR) != 0) {
        DLMap::iterator dliter = hosts_by_dladdr.find(ha.dladdr.hb_long());
        if (dliter == hosts_by_dladdr.end()) {
            return;
        }
        if ((ha.enabled_fields & Host_auth_event::EF_NWADDR) != 0) {
            DLNWMap::iterator dlnw
                = dliter->second.dlnwentries.find(ha.nwaddr);
            if (dlnw != dliter->second.dlnwentries.end()) {
                if (dliter->second.zero == NULL
                    || dlnw->second.host_netid == NULL
                    || dlnw->second.host_netid
                    != dliter->second.zero->host_netid)
                {
                    remove_host_by_event(ha, &dlnw->second);
                } else {
                    remove_host_by_event(ha, dliter->second.zero);
                }
            }
            return;
        } else if (dliter->second.zero != NULL) {
            // if removed anything, will have already removed from other nws
            if (remove_host_by_event(ha, dliter->second.zero)) {
                return;
            }
        }
        for (DLNWMap::iterator dlnw = dliter->second.dlnwentries.begin();
             dlnw != dliter->second.dlnwentries.end(); ++dlnw)
        {
            if (dlnw->first != 0) {
                remove_host_by_event(ha, &dlnw->second);
            }
        }
    } else if ((ha.enabled_fields & Host_auth_event::EF_NWADDR) != 0) {
        NWMap::iterator nwiter = hosts_by_nwaddr.find(ha.nwaddr);
        if (nwiter == hosts_by_nwaddr.end()) {
            return;
        }
        std::list<DLNWEntry*>::iterator dlnw
            = nwiter->second.dlnwentries.begin();
        for (; dlnw != nwiter->second.dlnwentries.end(); ++dlnw) {
            DLEntry *dlentry = (*dlnw)->dlentry;
            if (dlentry->zero == NULL
                || (*dlnw)->host_netid == NULL
                || (*dlnw)->host_netid
                != dlentry->zero->host_netid)
            {
                remove_host_by_event(ha, *dlnw);
            } else {
                remove_host_by_event(ha, dlentry->zero);
            }
        }
    } else {
        datapathid dp;
        uint16_t port = 0;
        bool ignore_port = true;
        if ((ha.enabled_fields & Host_auth_event::EF_LOCATION) != 0) {
            dp = ha.datapath_id;
            port = ha.port;
            ignore_port = false;
        } else if ((ha.enabled_fields & Host_auth_event::EF_SWITCH) != 0) {
            dp = ha.datapath_id;
        } else {
            dp = datapathid::from_host(0);
        }

        if ((ha.enabled_fields & Host_auth_event::EF_HOST_NETID) != 0) {
            HostNetMap::iterator host_netid
                = host_netids.find(ha.host_netid);
            if (host_netid == host_netids.end()
                || ((ha.enabled_fields & Host_auth_event::EF_HOSTNAME) != 0
                    && host_netid->second.entry->host->name != ha.hostname))
            {
                return;
            }

            remove_host_netid(&host_netid->second, dp, port,
                              ignore_port, ha.reason);
        } else if ((ha.enabled_fields & Host_auth_event::EF_HOSTNAME) != 0) {
            HostMap::iterator hiter = hosts.find(ha.hostname);
            if (hiter == hosts.end()) {
                return;
            }

            remove_host(&hiter->second, dp, port, ignore_port, ha.reason);
        } else {
            remove_location_hosts(dp, port, ignore_port, ha.reason, true);
        }
    }
}

// ----------------------------------------------------------------------------
// Apply wildcards on single dlnwentry.
// ----------------------------------------------------------------------------

bool
Authenticator::remove_host_by_event(const Host_auth_event& ha,
                                    DLNWEntry *dlnwentry)
{
    if ((ha.enabled_fields & Host_auth_event::EF_HOSTNAME) != 0) {
        if (dlnwentry->host_netid == NULL
            || dlnwentry->host_netid->host->name != ha.hostname)
        {
            return false;
        }
    }

    if ((ha.enabled_fields & Host_auth_event::EF_HOST_NETID) != 0) {
        if (dlnwentry->host_netid == NULL
            || dlnwentry->host_netid->name != ha.host_netid)
        {
            return false;
        }
    }

    datapathid dp;
    uint16_t port = 0;
    bool ignore_port = true;
    if ((ha.enabled_fields & Host_auth_event::EF_LOCATION) != 0) {
        dp = ha.datapath_id;
        port = ha.port;
        ignore_port = false;
    } else if ((ha.enabled_fields & Host_auth_event::EF_SWITCH) != 0) {
        dp = ha.datapath_id;
    } else {
        remove_dlnw_host(dlnwentry, ha.reason, true);
        return true;
    }

    return remove_dlnw_location(dlnwentry, dp, port, ignore_port, ha.reason);
}

// ----------------------------------------------------------------------------
// Authenticates user on host.
// ----------------------------------------------------------------------------

bool
Authenticator::add_user(const User_auth_event& ua)
{
    UserMap::iterator uiter = users.find(ua.username);
    if (uiter == users.end()) {
        VLOG_ERR(lg, "Cannot add user %"PRId64", does not exist.",
                 ua.username);
        return false;
    }
    UserEntry *user = &uiter->second;

    HostMap::iterator hiter = hosts.find(ua.hostname);
    if (hiter == hosts.end()) {
        VLOG_ERR(lg, "Cannot add host %"PRId64", does not exist.",
                 ua.hostname);
        return false;
    }
    HostEntry *host = &hiter->second;

    timeval curtime = { 0, 0 };
    gettimeofday(&curtime, NULL);

    std::list<AuthedUser>::iterator authed;
    for (authed = host->entry->users.begin();
         authed != host->entry->users.end(); ++authed)
    {
        if (authed->user == user->entry) {
            if (authed->idle_timeout != 0
                && (ua.idle_timeout > authed->idle_timeout
                    || ua.idle_timeout == 0))
            {
                authed->idle_timeout = ua.idle_timeout;
            }
            if (authed->hard_timeout != 0
                && (ua.hard_timeout > authed->hard_timeout
                    || ua.hard_timeout == 0))
            {
                authed->hard_timeout = ua.hard_timeout;
            }
            authed->auth_time = curtime.tv_sec;
            return true;
        }
    }

    if (host->entry->users.size() == 1
        && host->entry->users.front().user->name
        == data_cache->get_unauthenticated_id(datatypes->user_type()))
    {
        remove_unauth_user(host);
    }

    AuthedUser au = { curtime.tv_sec, ua.idle_timeout,
                      ua.hard_timeout, user->entry };
    host->entry->users.push_back(au);
    user->hostentries.push_back(host);
    Endpoints endpoints;
    add_updated_entry(host, endpoints);
    endpoints_updated(endpoints, true);
    const std::string& username = get_user_name(ua.username);
    const std::string& hostname = get_host_name(ua.hostname);
    VLOG_DBG(lg, "Added user %s to host %s.",
             username.c_str(), hostname.c_str());
    bindings->store_user_binding(ua.username, ua.hostname);
    post(new User_join_event(User_join_event::JOIN, ua.username,
                             ua.hostname, ua.reason));
    snprintf(buf, 1024, "{su} join on {sh}.");
    LogEntry to_log(app_name, LogEntry::INFO, buf);
    to_log.setName(ua.username, datatypes->user_type(), LogEntry::SRC);
    to_log.setName(ua.hostname, datatypes->host_type(), LogEntry::SRC);
    user_log->log(to_log);

    return true;
}

// ----------------------------------------------------------------------------
// Remove unauthenticated user from host.
// ----------------------------------------------------------------------------
void
Authenticator::remove_unauth_user(HostEntry *host)
{
    UserEntry *unauth_entry
        = &users[data_cache->get_unauthenticated_id(datatypes->user_type())];
    std::list<HostEntry*>::iterator uhost;
    bool not_found = true;
    for (uhost = unauth_entry->hostentries.begin();
         uhost != unauth_entry->hostentries.end(); ++uhost)
    {
        if (*uhost == host) {
            unauth_entry->hostentries.erase(uhost);
            not_found = false;
            break;
        }
    }
    if (not_found) {
        VLOG_ERR(lg, "Host %s not found in unauthenticated user entry "
                 "for auth.", get_host_name(host->entry->name).c_str());
    }
    host->entry->users.pop_front();
}

// ----------------------------------------------------------------------------
// Remove user from host, adding unauthenticated user to the host if its user
// list becomes empty and add_unauth == true.  If user == NULL, all users are
// removed from host.  Poisons locations host is on if any users are removed.
// ----------------------------------------------------------------------------

void
Authenticator::remove_user(UserEntry *user, HostEntry *host,
                           User_event::Reason reason, bool add_unauth)
{
    UserEntry *uentry = user;
    bool not_found = true;
    bool poison = false;
    for (std::list<AuthedUser>::iterator authed = host->entry->users.begin();
         authed != host->entry->users.end(); )
    {
        if (user == NULL || authed->user == user->entry) {
            if (user == NULL) {
                uentry = &users[authed->user->name];
            }
            authed = host->entry->users.erase(authed);
            std::list<HostEntry*>::iterator hiter
                = uentry->hostentries.begin();
            for (; hiter != uentry->hostentries.end(); ++hiter) {
                if (*hiter == host) {
                    uentry->hostentries.erase(hiter);
                    not_found = false;
                    break;
                }
            }
            if (not_found) {
                VLOG_ERR(lg, "Could not find host %s in user %s entry.",
                         get_host_name(host->entry->name).c_str(),
                         get_user_name(uentry->entry->name).c_str());
                not_found = false;
            }
            if (uentry->entry->name !=
                data_cache->get_unauthenticated_id(datatypes->user_type())
                && uentry->entry->name != 0)
            {
 	        poison = poison_allowed;
                int64_t userid = uentry->entry->name;
                int64_t hostid = host->entry->name;
                const std::string& username = get_user_name(userid);
                const std::string& hostname = get_host_name(hostid);
                const char *reason_str = User_event::get_reason_string(reason);
                VLOG_DBG(lg, "Removed user %s from host %s. (%s)",
                         username.c_str(), hostname.c_str(), reason_str);
                bindings->remove_user_binding(uentry->entry->name,
                                              host->entry->name);
                post(new User_join_event(User_join_event::LEAVE,
                                         uentry->entry->name,
                                         host->entry->name, reason));
                snprintf(buf, 1024, "{su} leave on {sh}. (%s)", reason_str);
                LogEntry to_log(app_name, LogEntry::INFO, buf);
                to_log.setName(userid, datatypes->user_type(), LogEntry::SRC);
                to_log.setName(hostid, datatypes->host_type(), LogEntry::SRC);
                user_log->log(to_log);
            }
            if (user != NULL) {
                break;
            }
            not_found = true;
        } else {
            ++authed;
        }
    }

    if (user != NULL && not_found) {
        VLOG_DBG(lg, "User %s not found on host %s.",
                 get_user_name(user->entry->name).c_str(),
                 get_host_name(host->entry->name).c_str());
    }

    if (add_unauth && host->entry->users.empty()) {
        timeval curtime = { 0, 0 };
        gettimeofday(&curtime, 0);
        UserEntry *unauth
            = update_user(data_cache->get_unauthenticated_id
                          (datatypes->user_type()));
        AuthedUser au = { curtime.tv_sec, 0, 0, unauth->entry };
        host->entry->users.push_back(au);
        unauth->hostentries.push_back(host);
    }

    if (poison) {
        Endpoints endpoints;
        add_updated_entry(host, endpoints);
        endpoints_updated(endpoints, true);
    }
}

// ----------------------------------------------------------------------------
// Remove user from host.  Either user or host can be wildcarded by setting to
// UNKNOWN_ID.
// ----------------------------------------------------------------------------

void
Authenticator::remove_user(int64_t username, int64_t hostname,
                           User_event::Reason reason)
{
    UserEntry *user = NULL;
    if (username != data_cache->get_unknown_id(datatypes->user_type())) {
        UserMap::iterator uiter = users.find(username);
        if (uiter == users.end()) {
            VLOG_DBG(lg, "User %"PRId64" does not exist.", username);
            return;
        }
        user = &uiter->second;
    }

    if (hostname == data_cache->get_unknown_id(datatypes->host_type())) {
        if (user == NULL) {
            VLOG_ERR(lg, "Must specify either user or host name when "
                     "removing user(s)");
        }
        for (std::list<HostEntry*>::iterator hiter = user->hostentries.begin();
             hiter != user->hostentries.end(); )
        {
            HostEntry *host = *hiter;
            ++hiter;
            remove_user(user, host, reason, true);
        }
    } else {
        HostMap::iterator hiter = hosts.find(hostname);
        if (hiter == hosts.end()) {
            VLOG_DBG(lg, "Host %"PRId64" does not exist.", hostname);
        } else {
            remove_user(user, &hiter->second, reason, true);
        }
    }
}

// ----------------------------------------------------------------------------
// Creates a new dladdr entry.
// ----------------------------------------------------------------------------

Authenticator::DLEntry*
Authenticator::new_dladdr(const ethernetaddr& dladdr, const time_t& exp_time)
{
    DLEntry& dentry = hosts_by_dladdr[dladdr.hb_long()] = DLEntry();
    dentry.dladdr = dladdr;
    dentry.groups.reset(new GroupList());
    data_cache->get_groups(dladdr, *dentry.groups);
    dentry.zero = NULL;

    dentry.exp_time = exp_time;

    return &dentry;
}

Authenticator::NWEntry*
Authenticator::new_nwaddr(uint32_t nwaddr, const time_t& exp_time)
{
    NWEntry& nwentry = hosts_by_nwaddr[nwaddr] = NWEntry();
    nwentry.groups.reset(new GroupList());
    data_cache->get_groups(ipaddr(nwaddr), *nwentry.groups);

    nwentry.exp_time = exp_time;

    return &nwentry;
}

// ----------------------------------------------------------------------------
// Creates a new dlnwentry.
// ----------------------------------------------------------------------------

Authenticator::DLNWEntry*
Authenticator::new_dlnw_entry(DLEntry *dlentry, uint32_t nwaddr,
                              const time_t& exp_time)
{
    DLNWEntry& dentry = dlentry->dlnwentries[nwaddr] = DLNWEntry();
    dentry.nwaddr = nwaddr;
    dentry.authed = false;
    dentry.dlentry = dlentry;
    if (nwaddr == 0) {
        dentry.dlentry->zero = &dentry;
    }

    NWMap::iterator niter = hosts_by_nwaddr.find(nwaddr);
    NWEntry *nwentry;
    if (niter == hosts_by_nwaddr.end()) {
        nwentry = new_nwaddr(nwaddr, exp_time);
    } else {
        nwentry = &niter->second;
    }
    dentry.nwaddr_groups = nwentry->groups;
    nwentry->dlnwentries.push_back(&dentry);

    dentry.exp_time = exp_time;

    return &dentry;
}

void
Authenticator::delete_principal(const Principal& principal)
{
    if (principal.type == datatypes->switch_type()) {
        delete_switch(principal.id);
    } else if (principal.type == datatypes->location_type()) {
        delete_location(principal.id);
    } else if (principal.type == datatypes->host_type()) {
        delete_host(principal.id);
    } else if (principal.type == datatypes->host_netid_type()) {
        delete_host_netid(principal.id);
    } else if (principal.type == datatypes->user_type()) {
        delete_user(principal.id);
    }
}

// ----------------------------------------------------------------------------
// Creates a new host entry.
// ----------------------------------------------------------------------------
Authenticator::HostEntry*
Authenticator::update_host(int64_t hostname)
{
    HostMap::iterator iter = hosts.find(hostname);
    if (iter != hosts.end()) {
        return &iter->second;
    }

    HostEntry& host = hosts[hostname] = HostEntry();
    host.entry.reset(new Host());
    host.entry->name = hostname;
    Principal principal = { datatypes->host_type(), hostname };
    data_cache->get_groups(principal, host.entry->groups);
    UserEntry *uentry
        = update_user(data_cache->get_unauthenticated_id(
                          datatypes->user_type()));
    AuthedUser au = { 0, 0, 0, uentry->entry };
    host.entry->users.push_back(au);
    uentry->hostentries.push_back(&host);
    host.entry->auth_time = host.entry->last_active
        = host.entry->hard_timeout = 0;
    return &host;
}

// ----------------------------------------------------------------------------
// Deletes entry for hostname, deleting all of its netids.
// ----------------------------------------------------------------------------

void
Authenticator::delete_host(int64_t hostname)
{
    HostMap::iterator hiter = hosts.find(hostname);
    if (hiter == hosts.end()) {
        return;
    }
    std::list<HostNetEntry*>::iterator niter = hiter->second.netids.begin();
    for (; niter != hiter->second.netids.end(); ) {
        HostNetEntry *netid = *niter;
        ++niter;
        delete_host_netid(netid);
    }
    remove_user(NULL, &hiter->second, User_event::HOST_DELETE, false);
    VLOG_DBG(lg, "Deleting host entry %s. (%s)",
             get_host_name(hostname).c_str(),
             Host_event::get_reason_string(Host_event::HOST_DELETE));
    hosts.erase(hiter);
}

// ----------------------------------------------------------------------------
// Creates a new host netid entry, calling 'success' on successful creation,
// else 'fail'.
// ----------------------------------------------------------------------------

Authenticator::HostNetEntry*
Authenticator::update_host_netid(int64_t host_netid, int64_t host_id,
                                 bool is_router, bool is_gateway)
{
    HostNetMap::iterator iter = host_netids.find(host_netid);
    HostNetEntry *netid;
    if (iter == host_netids.end()) {
        HostNetEntry& hnet = host_netids[host_netid] = HostNetEntry();
        hnet.entry.reset(new HostNetid());
        hnet.entry->name = host_netid;
        Principal principal = { datatypes->host_netid_type(), host_netid };
        data_cache->get_groups(principal, hnet.entry->groups);
        netid = &hnet;
    } else {
        netid = &iter->second;
        if (netid->entry->host->name != host_id
            || netid->entry->is_router != is_router
            || netid->entry->is_gateway != is_gateway)
        {
            delete_netid_host_binding(netid, Host_event::BINDING_CHANGE);
        } else {
            return netid;
        }
    }

    netid->entry->is_router = is_router;
    netid->entry->is_gateway = is_gateway;

    if (!add_netid_binding(netid, host_id)) {
        host_netids.erase(host_netid);
        return NULL;
    }

    return netid;
}

bool
Authenticator::add_netid_binding(HostNetEntry *netid, int64_t host_id)
{
    HostMap::iterator hiter = hosts.find(host_id);
    if (hiter == hosts.end()) {
        VLOG_ERR(lg, "Host %"PRId64" doesn't exist to add netid.",
                 host_id);
        return false;
    }

    hiter->second.netids.push_back(netid);
    netid->entry->host = hiter->second.entry;
    return true;
}

void
Authenticator::update_netid_binding(int64_t addr_id, int64_t host_netid,
                                    int64_t priority,
                                    const std::string& addr,
                                    AddressType addr_type, bool deleted)
{
    HostNetMap::iterator iter = host_netids.find(host_netid);
    if (iter == host_netids.end()) {
        VLOG_ERR(lg, "Host netid %"PRIu64" doesn't exist to "
                 "%s address binding.",
                 host_netid, deleted ? "remove" : "add");
        return;
    }

    VLOG_DBG(lg, "Updating netid binding.");

    if (addr_type == datatypes->ipv4_cidr_type()) {
        if (deleted) {
            delete_nwaddr_binding(&iter->second, addr_id, ipaddr(addr),
                                  Host_event::BINDING_CHANGE);
        } else {
            add_nwaddr_binding(&iter->second, addr_id, priority, ipaddr(addr));
        }
    } else if (addr_type == datatypes->mac_type()) {
        if (deleted) {
            delete_dladdr_binding(&iter->second, addr_id, ethernetaddr(addr),
                                  Host_event::BINDING_CHANGE);
        } else {
            add_dladdr_binding(&iter->second, addr_id, priority,
                               ethernetaddr(addr));
        }
    } else {
        VLOG_ERR(lg, "Cannot add binding for unknown address type %"PRIu32".",
                 (uint32_t) addr_type);
    }
}


void
Authenticator::add_dladdr_binding(HostNetEntry *netid, int64_t addr_id,
                                  int64_t priority, const ethernetaddr& dladdr)
{
    DLMap::iterator diter = hosts_by_dladdr.find(dladdr.hb_long());
    if (diter == hosts_by_dladdr.end()) {
        timeval curtime = { 0, 0 };
        gettimeofday(&curtime, NULL);
        DLEntry *dentry = new_dladdr(dladdr, curtime.tv_sec + NWADDR_TIMEOUT);
        DLBinding binding = { addr_id, priority, netid };
        dentry->bindings.push_back(binding);
        return;
    }

    bool inserted = false;
    bool isFirst = true;
    bool begin = true;
    DLEntry *dlentry = &diter->second;
    for (std::list<DLBinding>::iterator biter = dlentry->bindings.begin();
         biter != dlentry->bindings.end(); ++biter)
    {
        if (biter->netid == netid && biter->id == addr_id) {
            if (inserted) {
                dlentry->bindings.erase(biter);
                break;
            } else {
                biter->priority = priority;
                return;
            }
        } else if (biter->priority > priority && !inserted) {
            inserted = true;
            isFirst = begin;
            DLBinding binding = { addr_id, priority, netid };
            dlentry->bindings.insert(biter, binding);
        }
        begin = false;
    }

    if (!inserted) {
        isFirst = begin;
        DLBinding binding = { addr_id, priority, netid };
        dlentry->bindings.push_back(binding);
    }

    if (isFirst) {
        Host_auth_event ha(datapathid::from_host(0), 0,
                           dladdr, 0, 0, 0, Host_auth_event::EF_DLADDR,
                           Host_event::BINDING_CHANGE);
        remove_host_by_event(ha);
    }
}

void
Authenticator::add_nwaddr_binding(HostNetEntry *netid, int64_t addr_id,
                                  int64_t priority, const ipaddr& ip)
{
    uint32_t nwaddr = ntohl(ip.addr);
    NWMap::iterator niter = hosts_by_nwaddr.find(nwaddr);
    if (niter == hosts_by_nwaddr.end()) {
        timeval curtime = { 0, 0 };
        gettimeofday(&curtime, NULL);
        NWEntry *nwentry = new_nwaddr(nwaddr, curtime.tv_sec + NWADDR_TIMEOUT);
        NWBinding binding = { addr_id, priority, netid };
        nwentry->bindings.push_back(binding);
        return;
    }

    bool inserted = false;
    bool isFirst = true;
    bool begin = true;
    NWEntry *nwentry = &niter->second;
    for (std::list<NWBinding>::iterator biter = nwentry->bindings.begin();
         biter != nwentry->bindings.end(); ++biter)
    {
        if (biter->netid == netid && biter->id == addr_id) {
            if (inserted) {
                niter->second.bindings.erase(biter);
                break;
            } else {
                biter->priority = priority;
                return;
            }
        } else if (biter->priority > priority && !inserted) {
            inserted = true;
            isFirst = begin;
            NWBinding binding = { addr_id, priority, netid };
            nwentry->bindings.insert(biter, binding);
        }
        begin = false;
    }

    if (!inserted) {
        isFirst = begin;
        NWBinding binding = { addr_id, priority, netid };
        nwentry->bindings.push_back(binding);
    }

    if (isFirst) {
        Host_auth_event ha(datapathid::from_host(0), 0,
                           ethernetaddr((uint64_t)0),
                           nwaddr, 0, 0, Host_auth_event::EF_NWADDR,
                           Host_event::BINDING_CHANGE);
        remove_host_by_event(ha);
    }
}

void
Authenticator::delete_dladdr_binding(HostNetEntry *netid, int64_t addr_id,
                                     const ethernetaddr& dladdr,
                                     Host_event::Reason reason)
{
    DLMap::iterator diter = hosts_by_dladdr.find(dladdr.hb_long());
    if (diter != hosts_by_dladdr.end() && !diter->second.bindings.empty()) {
        std::list<DLBinding>::iterator biter
            = diter->second.bindings.begin();
        bool begin = true;
        for (; biter != diter->second.bindings.end(); ++biter) {
            if (biter->netid == netid && biter->id == addr_id) {
                diter->second.bindings.erase(biter);
                break;
            }
            begin = false;
        }
        if (begin) {
            Host_auth_event ha(datapathid::from_host(0), 0, dladdr,
                               0, 0, netid->entry->name,
                               Host_auth_event::EF_DLADDR
                               | Host_auth_event::EF_HOST_NETID,
                               reason);
            remove_host_by_event(ha);
        }
    }
}

void
Authenticator::delete_nwaddr_binding(HostNetEntry *netid, int64_t addr_id,
                                     const ipaddr& ip,
                                     Host_event::Reason reason)
{
    uint32_t nwaddr = ntohl(ip.addr);
    NWMap::iterator niter = hosts_by_nwaddr.find(nwaddr);
    if (niter != hosts_by_nwaddr.end() && !niter->second.bindings.empty()) {
        std::list<NWBinding>::iterator biter
            = niter->second.bindings.begin();
        bool begin = true;
        for (; biter != niter->second.bindings.end(); ++biter) {
            if (biter->netid == netid && biter->id == addr_id) {
                niter->second.bindings.erase(biter);
                break;
            }
            begin = false;
        }
        if (begin) {
            Host_auth_event ha(datapathid::from_host(0), 0, uint64_t(0),
                               nwaddr, 0, netid->entry->name,
                               Host_auth_event::EF_NWADDR
                               | Host_auth_event::EF_HOST_NETID,
                               reason);
            remove_host_by_event(ha);
        }
    }

}

void
Authenticator::delete_netid_host_binding(HostNetEntry *netid,
                                         Host_event::Reason reason)
{
    HostMap::iterator hiter = hosts.find(netid->entry->host->name);
    if (hiter != hosts.end()) {
        std::list<HostNetEntry*>::iterator hniter
            = hiter->second.netids.begin();
        for (; hniter != hiter->second.netids.end(); ++hniter) {
            if (*hniter == netid) {
                remove_host_netid(netid, datapathid::from_host(0), 0,
                                  true, reason);
                hiter->second.netids.erase(hniter);
                break;
            }
        }
    }
}

void
Authenticator::delete_host_netid(int64_t host_netid)
{
    HostNetMap::iterator hniter = host_netids.find(host_netid);
    if (hniter != host_netids.end()) {
        delete_host_netid(&hniter->second);
    }
}

void
Authenticator::delete_host_netid(HostNetEntry *netid)
{
    VLOG_DBG(lg, "Deleting netid entry %s. (%s)",
             get_netid_name(netid->entry->name).c_str(),
             Host_event::get_reason_string(Host_event::HOST_DELETE));
    delete_netid_host_binding(netid, Host_event::HOST_DELETE);
    host_netids.erase(netid->entry->name);
}

// ----------------------------------------------------------------------------
// Creates a new user entry.
// ----------------------------------------------------------------------------

Authenticator::UserEntry*
Authenticator::update_user(int64_t username)
{
    UserMap::iterator iter = users.find(username);
    if (iter != users.end()) {
        return &iter->second;
    }

    UserEntry& user = users[username] = UserEntry();
    user.entry.reset(new User());
    user.entry->name = username;
    Principal principal = { datatypes->user_type(), username };
    data_cache->get_groups(principal, user.entry->groups);
    return &user;
}

// ----------------------------------------------------------------------------
// Delete user.
// ----------------------------------------------------------------------------

void
Authenticator::delete_user(int64_t username)
{
    UserMap::iterator user = users.find(username);
    if (user == users.end()) {
        return;
    }
    remove_user(username, data_cache->get_unknown_id(datatypes->host_type()),
                User_event::USER_DELETE);
    VLOG_DBG(lg, "Deleting user entry %s. (%s)",
             get_user_name(username).c_str(),
             User_event::get_reason_string(User_event::USER_DELETE));
    users.erase(user);
}

Authenticator::LocEntry*
Authenticator::update_location(int64_t locname, int64_t priority,
                               int64_t switch_id, uint16_t port,
                               const std::string& portname)
{
    LocMap::iterator iter = locations.find(locname);
    LocEntry *lentry;
    if (iter == locations.end()) {
        LocEntry& location = locations[locname] = LocEntry();
        location.entry.reset(new Location());
        location.entry->name = locname;
        location.entry->portname = portname;
        location.entry->is_virtual = false;
        Principal principal = { datatypes->location_type(), locname };
        data_cache->get_groups(principal, location.entry->groups);
        location.active = false;
        lentry = &location;
    } else {
        lentry = &iter->second;
        if (lentry->entry->sw->name != switch_id
            || lentry->entry->port != port)
        {
            delete_location_binding(lentry);
        }
        if (!lentry->active && portname != "") {
            lentry->entry->portname = portname;
        }
    }

    if (!add_location_binding(lentry, priority, switch_id, port)) {
        lentry = NULL;
        locations.erase(locname);
    }

    return lentry;
}

bool
Authenticator::add_location_binding(LocEntry *location, int64_t priority,
                                    int64_t switch_id, uint16_t port)
{
    SwitchMap::iterator siter = switches.find(switch_id);
    if (siter == switches.end()) {
        VLOG_ERR(lg, "Switch %"PRId64" doesn't exist to add location.",
                 switch_id);
        return false;
    }

    location->entry->port = port;
    location->entry->sw = siter->second.entry;

    std::list<PortEntry>::iterator piter = siter->second.locations.begin();
    for (; piter != siter->second.locations.end(); ++piter)
    {
        if (piter->port == port) {
            bool inserted = false;
            bool isFirst = true;
            bool begin = true;
            std::list<LocBinding>::iterator biter = piter->bindings.begin();
            for (; biter != piter->bindings.end(); ++biter) {
                if (biter->location == location) {
                    if (inserted) {
                        piter->bindings.erase(biter);
                        break;
                    } else {
                        biter->priority = priority;
                        return true;
                    }
                } else if (biter->priority > priority && !inserted) {
                    inserted = true;
                    isFirst = begin;
                    LocBinding binding = { priority, location };
                    piter->bindings.insert(biter, binding);
                }
                begin = false;
            }
            if (!inserted) {
                isFirst = begin;
                LocBinding binding = { priority, location };
                piter->bindings.push_back(binding);
            }

            if (isFirst && piter->active_location != NULL) {
                if (piter->active_location != location) {
                    deactivate_location(siter->second.entry->dp, port,
                                        *piter, true);
                }
            }
            return true;
        }
    }

    siter->second.locations.push_back(PortEntry());
    PortEntry& pentry = siter->second.locations.back();
    pentry.port = port;
    pentry.active_location = NULL;
    LocBinding binding = { priority, location };
    pentry.bindings.push_back(binding);
    return true;
}

void
Authenticator::delete_location_binding(LocEntry *location)
{
    SwitchMap::iterator siter = switches.find(location->entry->sw->name);
    if (siter == switches.end()) {
        return;
    }

    std::list<PortEntry>::iterator piter = siter->second.locations.begin();
    for (; piter != siter->second.locations.end(); ++piter) {
        if (piter->port == location->entry->port) {
            std::list<LocBinding>::iterator biter = piter->bindings.begin();
            for (; biter != piter->bindings.end(); ++biter) {
                if (biter->location == location) {
                    piter->bindings.erase(biter);
                    break;
                }
            }
            if (piter->active_location == location) {
                deactivate_location(siter->second.entry->dp,
                                    piter->port, *piter, true);
            }

            if (piter->bindings.size() == 0) {
                siter->second.locations.erase(piter);
            }
            break;
        }
    }
}

Authenticator::LocEntry *
Authenticator::new_dynamic_location(SwitchEntry *sentry, uint16_t port)
{
    uint64_t loc = sentry->entry->dp.as_host() | (((uint64_t) port) << 48);
    LocEntry &lentry = dynamic_locations[loc] = LocEntry();
    lentry.entry.reset(new Location());
    lentry.entry->port = port;
    lentry.entry->name
        = data_cache->get_authenticated_id(datatypes->location_type());
    lentry.entry->is_virtual = false;
    lentry.entry->sw = sentry->entry;
    sentry->locations.push_back(PortEntry());
    PortEntry& pentry = sentry->locations.back();
    pentry.port = port;
    pentry.active_location = &lentry;
    return &lentry;
}

void
Authenticator::activate_location(const datapathid& dp, uint16_t port,
                                 const std::string& portname)
{
    DPMap::iterator siter = switches_by_dp.find(dp.as_host());
    if (siter == switches_by_dp.end() || siter->second.active_switch == NULL) {
        VLOG_ERR(lg, "Cannot activate location %"PRIx64":%"PRIu16""
                 ", switch isn't active.", dp.as_host(), port);
        return;
    }

    SwitchEntry *sentry = siter->second.active_switch;
    LocEntry *location = NULL;
    for (std::list<PortEntry>::iterator piter = sentry->locations.begin();
         piter != sentry->locations.end(); ++piter)
    {
        if (piter->port == port) {
            if (piter->active_location != NULL) {
                VLOG_WARN(lg, "Received port add for active location "
                          "%"PRIx64":%"PRIu16".", dp.as_host(), port);
                return;
            }
            location = piter->active_location
                = piter->bindings.front().location;
            break;
        }
    }

    if (location == NULL) {
        VLOG_DBG(lg, "Location %"PRIx64":%"PRIu16" isn't registered",
                 dp.as_host(), port);
        location = new_dynamic_location(sentry, port);
    }

    location->active = true;
    location->entry->portname = portname;
    location->entry->is_virtual = portname.find("vif") == 0
        || portname.find("tap") == 0;
    if (dp.as_host() != 0) {
        bindings->add_name_for_location(dp, port,
                                        location->entry->name, Name::LOCATION);
    }
    new_location_config(dp, port, portname, false);
}

void
Authenticator::deactivate_location(const datapathid& dp, uint16_t port,
                                   bool poison)
{
    DPMap::iterator siter = switches_by_dp.find(dp.as_host());
    if (siter == switches_by_dp.end() || siter->second.active_switch == NULL) {
        return;
    }

    SwitchEntry *sentry = siter->second.active_switch;
    for (std::list<PortEntry>::iterator piter = sentry->locations.begin();
         piter != sentry->locations.end(); ++piter)
    {
        if (piter->port == port) {
            deactivate_location(dp, port, *piter, poison);
            if (piter->bindings.empty()) {
                sentry->locations.erase(piter);
            }
            break;
        }
    }
}

void
Authenticator::deactivate_location(const datapathid& dp, uint16_t port,
                                   PortEntry& pentry, bool poison)
{
    if (pentry.active_location == NULL) {
        return;
    }
    LocEntry *lentry = pentry.active_location;
    remove_location_hosts(lentry, Host_event::LOCATION_LEAVE, poison);
    pentry.active_location = NULL;
    bindings->remove_name_for_location(dp, port, 0, Name::LOCATION);
    lentry->active = false;

    if (lentry->entry->name
        == data_cache->get_authenticated_id(datatypes->location_type()))
    {
        dynamic_locations.erase(dp.as_host() | (((uint64_t) port) << 48));
    }

    if (poison) {
        poison_port(dp, port);
    }
}

void
Authenticator::delete_location(int64_t locname, bool remove_binding)
{
    LocMap::iterator liter = locations.find(locname);
    if (liter == locations.end()) {
        return;
    }

    LocEntry *location = &liter->second;

    if (remove_binding) {
        delete_location_binding(location);
    }

    locations.erase(liter);
}

Authenticator::SwitchEntry*
Authenticator::update_switch(int64_t switchname, int64_t priority,
                             const datapathid& dp)
{
    SwitchMap::iterator iter = switches.find(switchname);
    SwitchEntry *sentry;
    if (iter == switches.end()) {
        SwitchEntry& sw = switches[switchname] = SwitchEntry();
        sw.entry.reset(new Switch());
        sw.entry->name = switchname;
        Principal principal = {datatypes->switch_type(), switchname };
        data_cache->get_groups(principal, sw.entry->groups);
        sw.active = false;
        sentry = &sw;
    } else {
        sentry = &iter->second;
        if (sentry->entry->dp != dp) {
            delete_switch_binding(sentry);
        }
    }

    add_switch_binding(sentry, priority, dp);

    return sentry;
}

void
Authenticator::add_switch_binding(SwitchEntry *sentry, int64_t priority,
                                  const datapathid& dp)
{
    sentry->entry->dp = dp;
    uint64_t dp_host = dp.as_host();
    if (dp_host == 0) {
        return;
    }
    DPMap::iterator diter = switches_by_dp.find(dp_host);
    if (diter == switches_by_dp.end()) {
        DPEntry& dentry = switches_by_dp[dp_host] = DPEntry();
        dentry.active_switch = NULL;
        SwitchBinding binding = { priority, sentry };
        dentry.bindings.push_back(binding);
        return;
    }

    bool inserted = false;
    bool isFirst = true;
    bool begin = true;
    std::list<SwitchBinding>::iterator biter = diter->second.bindings.begin();
    for (; biter != diter->second.bindings.end(); ++biter) {
        if (biter->sentry == sentry) {
            if (inserted) {
                diter->second.bindings.erase(biter);
                break;
            } else {
                biter->priority = priority;
                return;
            }
        } else if (biter->priority > priority && !inserted) {
            inserted = true;
            isFirst = begin;
            SwitchBinding binding = { priority, sentry };
            diter->second.bindings.insert(biter, binding);
        }
        begin = false;
    }

    if (!inserted) {
        isFirst = begin;
        SwitchBinding binding = { priority, sentry };
        diter->second.bindings.push_back(binding);
    }

    if (isFirst && diter->second.active_switch != NULL) {
        if (diter->second.active_switch != sentry) {
            VLOG_DBG(lg, "Deactivating old dp %"PRIx64" binding.",
                     dp_host);
            deactivate_switch(diter->second, true);
        }
    }
}

void
Authenticator::delete_switch_binding(SwitchEntry *sentry)
{
    DPMap::iterator diter = switches_by_dp.find(sentry->entry->dp.as_host());
    if (diter == switches_by_dp.end()) {
        return;
    }

    std::list<SwitchBinding>::iterator biter = diter->second.bindings.begin();
    for (; biter != diter->second.bindings.end(); ++biter) {
        if (biter->sentry == sentry) {
            diter->second.bindings.erase(biter);
            break;
        }
    }

    if (sentry == diter->second.active_switch) {
        VLOG_DBG(lg, "Deactivating deleted dp %"PRIx64" binding.",
                 diter->first);
        deactivate_switch(diter->second, true);
    }

    if (diter->second.bindings.empty()) {
        switches_by_dp.erase(diter);
    }
}

Authenticator::SwitchEntry *
Authenticator::new_dynamic_switch(const datapathid& dp)
{
    uint64_t dp_host = dp.as_host();
    SwitchEntry& sentry = dynamic_switches[dp_host] = SwitchEntry();
    sentry.entry.reset(new Switch());
    sentry.entry->dp = dp;
    sentry.entry->name
        = data_cache->get_authenticated_id(datatypes->switch_type());
    sentry.active = false;
    DPEntry& dpentry = switches_by_dp[dp_host] = DPEntry();
    dpentry.active_switch = &sentry;
    return &sentry;
}

void
Authenticator::activate_switch(const datapathid& dp)
{
    DPMap::iterator diter = switches_by_dp.find(dp.as_host());
    SwitchEntry *sentry;
    if (diter == switches_by_dp.end()) {
        VLOG_DBG(lg, "Switch %"PRIx64" isn't registered",
                 dp.as_host());
        sentry = new_dynamic_switch(dp);
    } else {
        DPEntry *dentry = &diter->second;
        if (dentry->active_switch != NULL) {
            VLOG_WARN(lg, "Received dp join for active switch "
                      "%"PRIx64".", dp.as_host());
            return;
        }
        sentry = dentry->active_switch = dentry->bindings.front().sentry;
    }
    sentry->active = true;
    if (dp.as_host() != 0) {
        bindings->add_name_for_location(dp, 0, sentry->entry->name,
                                        Name::SWITCH);
        LogEntry to_log = LogEntry(app_name, LogEntry::ALERT,
                                   "{ss} joined the network.");
        to_log.setName(sentry->entry->name,
                       datatypes->switch_type(), LogEntry::SRC);
        user_log->log(to_log);
        post(new Switch_bind_event(Switch_bind_event::JOIN,
                                   dp, sentry->entry->name));
    }
}

void
Authenticator::deactivate_switch(const datapathid& dp, bool poison)
{
    DPMap::iterator siter = switches_by_dp.find(dp.as_host());
    if (siter == switches_by_dp.end() || siter->second.active_switch == NULL) {
        return;
    }
    deactivate_switch(siter->second, poison);
    if (siter->second.bindings.empty()) {
        switches_by_dp.erase(siter);
    }
}

void
Authenticator::deactivate_switch(DPEntry& dentry, bool poison)
{
    if (dentry.active_switch == NULL) {
        return;
    }

    SwitchEntry *sentry = dentry.active_switch;
    for (std::list<PortEntry>::iterator piter = sentry->locations.begin();
         piter != sentry->locations.end(); )
    {
        deactivate_location(sentry->entry->dp, piter->port, *piter, poison);
        if (piter->bindings.empty()) {
            piter = sentry->locations.erase(piter);
        } else {
            ++piter;
        }
    }

    dentry.active_switch = NULL;
    sentry->active = false;
    bindings->remove_name_for_location(sentry->entry->dp, 0, 0, Name::SWITCH);
    LogEntry to_log = LogEntry(app_name, LogEntry::ALERT,
                               "{ss} left the network.");
    to_log.setName(sentry->entry->name,
                   datatypes->switch_type(), LogEntry::SRC);
    user_log->log(to_log);

    post(new Switch_bind_event(Switch_bind_event::LEAVE,
                               sentry->entry->dp,
                               sentry->entry->name));

    if (poison) {
        close_openflow_connection(sentry->entry->dp);
    }

    if (sentry->entry->name
        == data_cache->get_authenticated_id(datatypes->switch_type()))
    {
        dynamic_switches.erase(sentry->entry->dp.as_host());
    }
}

void
Authenticator::delete_switch(int64_t switchname)
{
    SwitchMap::iterator siter = switches.find(switchname);
    if (siter == switches.end()) {
        return;
    }

    SwitchEntry *sentry = &siter->second;

    delete_switch_binding(sentry);

    for (std::list<PortEntry>::iterator piter = sentry->locations.begin();
         piter != sentry->locations.end(); ++piter)
    {
        for (std::list<LocBinding>::iterator biter = piter->bindings.begin();
             biter != piter->bindings.end(); ++biter)
        {
            delete_location(biter->location->entry->name, false);
        }
    }

    switches.erase(siter);
}

void
Authenticator::new_location_config(const datapathid& dp, uint16_t port,
                                   const std::string& portname, bool is_redo)
{
    if (port == OFPP_LOCAL || dp.as_host() == 0) {
        return;
    }

    if (!is_redo) {
        remove_queued_loc(dp, port);
    }

    LocEntry *lentry = get_active_location(dp, port);
    if (lentry == NULL) {
        return;
    }

    datapathid mgmtid = nox::dpid_to_mgmtid(dp);
    boost::shared_ptr<Switch_mgr> swm;
    if (mgmtid.as_host() == 0) {
        return;
    }

    swm = nox::mgmtid_to_swm(mgmtid);
    if (swm == NULL) {
        VLOG_DBG(lg, "Queuing new loc config dp:%"PRIx64" for "
                 "management conn.", dp.as_host());
	queue_new_location(dp, port, portname);
        return;
    } else {
        if (swm->port_is_virtual(portname)) {
            if (!lentry->entry->is_virtual) {
                mark_as_virtual(lentry);
            }
        } else if (lentry->entry->is_virtual) {
            remove_location_hosts(lentry, Host_event::INTERNAL_LOCATION, true);
            lentry->entry->is_virtual = false;
        }
    }
}

void
Authenticator::queue_new_location(const datapathid& dp, uint16_t port,
				  const std::string& portname)
{
    Queued_loc loc = { dp, port, portname };
    queued_locs.push_back(loc);
    timeval tv = { 5, 0 };
    post(boost::bind(&Authenticator::process_queue, this), tv);
}

void
Authenticator::process_queue()
{
    if (queued_locs.empty()) {
        return;
    }
    Queued_loc loc = queued_locs.front();
    queued_locs.pop_front();
    new_location_config(loc.dp, loc.port, loc.portname, true);
}

void
Authenticator::remove_queued_loc(const datapathid& dp, uint16_t port)
{
    for (std::list<Queued_loc>::iterator entry = queued_locs.begin();
	 entry != queued_locs.end(); ++entry)
    {
        if (entry->dp == dp && entry->port == port) {
  	    queued_locs.erase(entry);
            return;
        }
    }
}

void
Authenticator::mark_as_virtual(LocEntry *location)
{
    location->entry->is_virtual = true;
    VLOG_DBG(lg, "Marking location %s as virtual.",
             get_location_name(location->entry->name).c_str());
    // remove hosts from physical locations because virtual
    // location is known.  don't really expect multiple virtual
    // locations but support for now
    std::list<DLEntry*>::const_iterator dliter = location->dlentries.begin();
    for (; dliter != location->dlentries.end(); ++dliter) {
        std::list<AuthedLocation>::iterator liter
            = (*dliter)->locations.begin();
        for (; liter != (*dliter)->locations.end(); ) {
            if (liter->location->is_virtual) {
                ++liter;
            } else {
                remove_dl_location(*dliter, liter,
                                   Host_event::INTERNAL_LOCATION, false);
            }
        }
    }
}

Authenticator::LocEntry*
Authenticator::get_active_location(const datapathid& dp, uint16_t port) const
{
    DPMap::const_iterator diter = switches_by_dp.find(dp.as_host());
    if (diter == switches_by_dp.end()
        || diter->second.active_switch == NULL)
    {
        return NULL;
    }

    std::list<PortEntry>::const_iterator piter
        = diter->second.active_switch->locations.begin();
    for (; piter != diter->second.active_switch->locations.end(); ++piter) {
        if (piter->port == port) {
            return piter->active_location;
        }
    }
    return NULL;
}

}
}
