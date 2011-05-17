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

#include "bootstrap-complete.hh"
#include "datapath-join.hh"
#include "datapath-leave.hh"
#include "netinet++/ethernet.hh"
#include "port-status.hh"
#include "switch_event.hh"
#include "openflow-default.hh"
#include "openflow-pack.hh"

#if AUTH_WITH_ROUTING
#include "discovery/link-event.hh"
#endif

#define TIMER_INTERVAL         30
#define DEFAULT_IDLE_TIMEOUT   300     // 5 min idle timeout
#define DEFAULT_HARD_TIMEOUT   18000   // 5 hr hard timeout

namespace vigil {
namespace applications {

static Vlog_module lg("authenticator");

const uint32_t Authenticator::NWADDR_TIMEOUT;

std::size_t
Authenticator::ptrhash::operator()(const void *ptr) const
{
    return HASH_NAMESPACE::hash<const void*>()(ptr);
}

bool
Authenticator::ptreq::operator()(const void *a, const void *b) const
{
    return a == b;
}

std::size_t
Authenticator::
sptrhash::operator()(const boost::shared_ptr<Location>& ptr) const
{
    return HASH_NAMESPACE::hash<const void*>()(ptr.get());
}

bool
Authenticator::sptreq::operator()(const boost::shared_ptr<Location>& a,
                                  const boost::shared_ptr<Location>& b) const
{
    return a == b;
}

Authenticator::Authenticator(const container::Context* c,
                             const json_object*)
    : Component(c), routing(false), auto_auth(true),
      expire_timer(TIMER_INTERVAL),
      datatypes(0), data_cache(0), bindings(0),
      user_log(0), default_hard_timeout(DEFAULT_HARD_TIMEOUT),
      default_idle_timeout(DEFAULT_IDLE_TIMEOUT)
{
    raw_of.reset(new uint8_t[sizeof *ofm]);
    ofm = (ofp_flow_mod*) raw_of.get();
    ofm->header.version = OFP_VERSION;
    ofm->header.type = OFPT_FLOW_MOD;
    ofm->header.length = htons(sizeof *ofm);
    ofm->header.xid = openflow_pack::get_xid();
    ofm->cookie = 0;
    ofm->command = htons(OFPFC_DELETE);
    ofm->idle_timeout = htons(OFP_FLOW_PERMANENT);
    ofm->hard_timeout = htons(OFP_FLOW_PERMANENT);
    ofm->out_port = htons(OFPP_NONE);
    ofm->buffer_id = htonl(UINT32_MAX);
    ofm->priority = htons(OFP_DEFAULT_PRIORITY);
    ofm->flags = htons(ofd_flow_mod_flags());
}

void
Authenticator::getInstance(const container::Context* ctxt,
                           Authenticator*& h)
{
    h = dynamic_cast<Authenticator*>
        (ctxt->get_by_interface(container::Interface_description
                                (typeid(Authenticator).name())));
}

void
Authenticator::configure(const container::Configuration*)
{
    register_event(Host_auth_event::static_get_name());
    register_event(Host_bind_event::static_get_name());
    register_event(Host_join_event::static_get_name());
    register_event(User_auth_event::static_get_name());
    register_event(User_join_event::static_get_name());
    register_event(Switch_bind_event::static_get_name());

    poison_allowed = true;

    resolve(datatypes);
    resolve(data_cache);
    resolve(bindings);
    resolve(user_log);
#if AUTH_WITH_ROUTING
    resolve(routing_mod);
#endif

    data_cache->add_switch_updated(
        boost::bind(&Authenticator::update_switch, this, _1, _2, _3));
    data_cache->add_location_updated(
        boost::bind(&Authenticator::update_location, this, _1, _2, _3, _4, _5));
    data_cache->add_host_updated(
        boost::bind(&Authenticator::update_host, this, _1));
    data_cache->add_netid_updated(
        boost::bind(&Authenticator::update_host_netid, this,
                    _1, _2, _3, _4));
    data_cache->add_netid_binding_updated(
        boost::bind(&Authenticator::update_netid_binding, this,
                    _1, _2, _3, _4, _5, _6));
    data_cache->add_user_updated(
        boost::bind(&Authenticator::update_user, this, _1));
    data_cache->add_principal_deleted(
        boost::bind(&Authenticator::delete_principal, this, _1));
    data_cache->add_groups_updated(
        boost::bind(&Authenticator::principals_updated, this, _1, true, true));
    data_cache->add_addr_groups_updated(
        boost::bind(&Authenticator::addr_updated, this, _1, _2, true, true));

    register_handler<Bootstrap_complete_event>
        (boost::bind(&Authenticator::handle_bootstrap, this, _1));
    register_handler<Datapath_join_event>
        (boost::bind(&Authenticator::handle_datapath_join, this, _1));
    register_handler<Datapath_leave_event>
        (boost::bind(&Authenticator::handle_datapath_leave, this, _1));
    register_handler<Port_status_event>
        (boost::bind(&Authenticator::handle_port_status, this, _1));
    register_handler<Host_auth_event>
        (boost::bind(&Authenticator::handle_host_auth, this, _1));
    register_handler<User_auth_event>
        (boost::bind(&Authenticator::handle_user_auth, this, _1));
    register_handler<Packet_in_event>
        (boost::bind(&Authenticator::handle_packet_in, this, _1));
#if AUTH_WITH_ROUTING
    register_handler<Link_event>
        (boost::bind(&Authenticator::handle_link_change, this, _1));
#endif
}

void
Authenticator::install()
{
    Flow_util *flow_util;
    resolve(flow_util);
    Flow_fn_map::Flow_fn fn = boost::bind(&Authenticator::auth_flow_host,
                                          this, _1, true);
    flow_util->fns.register_function("authenticate_host", fn);
}

const std::string&
Authenticator::get_switch_name(int64_t id)
{
    Principal principal = { datatypes->switch_type(), id };
    return data_cache->get_name_ref(principal);
}

const std::string&
Authenticator::get_location_name(int64_t id)
{
    Principal principal = { datatypes->location_type(), id };
    return data_cache->get_name_ref(principal);
}

const std::string&
Authenticator::get_host_name(int64_t id)
{
    Principal principal = { datatypes->host_type(), id };
    return data_cache->get_name_ref(principal);
}

const std::string&
Authenticator::get_netid_name(int64_t id)
{
    Principal principal = { datatypes->host_netid_type(), id };
    return data_cache->get_name_ref(principal);
}

const std::string&
Authenticator::get_user_name(int64_t id)
{
    Principal principal = { datatypes->user_type(), id };
    return data_cache->get_name_ref(principal);
}

const std::string&
Authenticator::get_group_name(int64_t id)
{
    Principal principal = { datatypes->group_type(), id };
    return data_cache->get_name_ref(principal);
}

int64_t
Authenticator::get_port_number(const datapathid& dp,
                               const std::string& port_name) const
{
    const SwitchEntry *dpentry
        = get_switch_entry(dp);

    if (dpentry != NULL) {
        for (std::list<PortEntry>::const_iterator port
                 = dpentry->locations.begin();
             port != dpentry->locations.end(); ++port)
        {
            if (port->active_location != NULL) {
                if (port->active_location->entry->portname == port_name) {
                    return port->port;
                }
            }
        }
    } else {
        VLOG_DBG(lg, "Datapath %"PRIx64" not active to retrieve port name.",
                 dp.as_host());
    }
    return -1;
}

const Authenticator::DLEntry *
Authenticator::get_dladdr_entry(const ethernetaddr& dladdr) const
{
    DLMap::const_iterator iter = hosts_by_dladdr.find(dladdr.hb_long());
    if (iter == hosts_by_dladdr.end()) {
        return NULL;
    }
    return &iter->second;
}

const Authenticator::NWEntry *
Authenticator::get_nwaddr_entry(uint32_t nwaddr) const
{
    NWMap::const_iterator iter = hosts_by_nwaddr.find(nwaddr);
    if (iter == hosts_by_nwaddr.end()) {
        return NULL;
    }
    return &iter->second;
}

const Authenticator::DLNWEntry *
Authenticator::get_dlnw_entry(const ethernetaddr& dladdr,
                              uint32_t nwaddr) const
{
    const DLEntry *dlentry = get_dladdr_entry(dladdr);
    if (dlentry == NULL) {
        return NULL;
    }
    DLNWMap::const_iterator iter = dlentry->dlnwentries.find(nwaddr);
    if (iter == dlentry->dlnwentries.end()) {
        if (!is_internal_ip(htonl(nwaddr))) {
            return dlentry->zero;
        }
        return NULL;
    }
    return &iter->second;
}

const Authenticator::SwitchEntry *
Authenticator::get_switch_entry(const datapathid& dp) const
{
    DPMap::const_iterator diter = switches_by_dp.find(dp.as_host());
    if (diter == switches_by_dp.end()) {
        return NULL;
    }
    return diter->second.active_switch;
}

const Authenticator::LocEntry *
Authenticator::get_location_entry(const datapathid& dp,
                                  uint16_t port) const
{
    return get_active_location(dp, port);
}

const Authenticator::SwitchEntry *
Authenticator::get_switch_entry(int64_t id) const
{
    SwitchMap::const_iterator iter = switches.find(id);
    if (iter == switches.end()) {
        return NULL;
    }
    return &iter->second;
}

const Authenticator::LocEntry *
Authenticator::get_location_entry(int64_t id) const
{
    LocMap::const_iterator iter = locations.find(id);
    if (iter == locations.end()) {
        return NULL;
    }
    return &iter->second;
}

const Authenticator::HostEntry *
Authenticator::get_host_entry(int64_t id) const
{
    HostMap::const_iterator iter = hosts.find(id);
    if (iter == hosts.end()) {
        return NULL;
    }
    return &iter->second;
}

const Authenticator::HostNetEntry *
Authenticator::get_host_netid_entry(int64_t id) const
{
    HostNetMap::const_iterator iter = host_netids.find(id);
    if (iter == host_netids.end()) {
        return NULL;
    }
    return &iter->second;
}

const Authenticator::UserEntry *
Authenticator::get_user_entry(int64_t id) const
{
    UserMap::const_iterator iter = users.find(id);
    if (iter == users.end()) {
        return NULL;
    }
    return &iter->second;
}

Authenticator::DLEntry *
Authenticator::get_dladdr_entry(const ethernetaddr& dladdr,
                                const time_t& exp_time,
                                bool create)
{
    DLMap::iterator iter = hosts_by_dladdr.find(dladdr.hb_long());
    if (iter == hosts_by_dladdr.end()) {
        if (create) {
            return new_dladdr(dladdr, exp_time);
        }
        return NULL;
    }
    iter->second.exp_time = exp_time;
    return &iter->second;
}

Authenticator::NWEntry *
Authenticator::get_nwaddr_entry(uint32_t nwaddr, const time_t& exp_time,
                                bool create)
{
    NWMap::iterator iter = hosts_by_nwaddr.find(nwaddr);
    if (iter != hosts_by_nwaddr.end()) {
        iter->second.exp_time = exp_time;
        return &iter->second;
    } else if (!create) {
        return NULL;
    }
    return new_nwaddr(nwaddr, exp_time);
}

Authenticator::DLNWEntry *
Authenticator::get_dlnw_entry(const ethernetaddr& dladdr, uint32_t nwaddr,
                              const time_t& exp_time, bool create)
{
    DLEntry *dlentry = get_dladdr_entry(dladdr, exp_time, create);
    if (dlentry == NULL) {
        return NULL;
    }
    return get_dlnw_entry(dlentry, nwaddr, exp_time, create);
}

Authenticator::DLNWEntry *
Authenticator::get_dlnw_entry(DLEntry *dlentry, uint32_t nwaddr,
                              const time_t& exp_time, bool create)
{
    DLNWMap::iterator iter = dlentry->dlnwentries.find(nwaddr);
    if (iter != dlentry->dlnwentries.end()) {
        iter->second.exp_time = exp_time;
        return &iter->second;
    } else if (!create) {
        return NULL;
    }
    return new_dlnw_entry(dlentry, nwaddr, exp_time);
}


void
Authenticator::reset_last_active(const datapathid& dp, uint16_t port,
                                 const ethernetaddr& dladdr, uint32_t nwaddr)
{
    timeval curtime = { 0, 0 };
    gettimeofday(&curtime, NULL);

    DLNWEntry *dlnwentry = get_dlnw_entry(dladdr, nwaddr,
                                          curtime.tv_sec + NWADDR_TIMEOUT,
                                          false);
    if (dlnwentry == NULL) {
        return;
    }

    std::list<AuthedLocation>::iterator al
        = dlnwentry->dlentry->locations.begin();
    for (; al != dlnwentry->dlentry->locations.end(); ++al) {
        if (al->location->sw->dp == dp && al->location->port == port) {
            break;
        }
    }

    if (al != dlnwentry->dlentry->locations.end()) {
        al->last_active = curtime.tv_sec;
        dlnwentry->dlentry->zero->host_netid->host->last_active
            = curtime.tv_sec;
        if (nwaddr == 0) {
            return;
        }
    }

    if (dlnwentry->authed) {
        dlnwentry->host_netid->host->last_active = curtime.tv_sec;
    }
}

void
Authenticator::principal_updated(const Principal& principal, bool poison,
                                 bool refetch_groups)
{
    VLOG_DBG(lg, "principal_updated called.");
    Endpoints endpoints;
    add_updated_principal(principal, endpoints, refetch_groups);
    endpoints_updated(endpoints, poison);
}

void
Authenticator::principals_updated(const PrincipalSet& principals, bool poison,
                                  bool refetch_groups)
{
    VLOG_DBG(lg, "principals_updated called.");
    Endpoints endpoints;
    for (PrincipalSet::const_iterator iter = principals.begin();
         iter != principals.end(); ++iter)
    {
        add_updated_principal(*iter, endpoints, refetch_groups);
    }
    endpoints_updated(endpoints, poison);
}

void
Authenticator::groups_updated(const std::list<int64_t>& groups, bool poison)
{
    VLOG_DBG(lg, "groups_updated called.");
    PrincipalSet principals;
    data_cache->get_all_group_members(groups, principals);
    principals_updated(principals, poison, false);
}

void
Authenticator::group_updated(int64_t group, Endpoints& endpoints)
{
    PrincipalSet principals;
    data_cache->get_all_group_members(std::list<int64_t>(1, group), principals);
    for (PrincipalSet::const_iterator iter = principals.begin();
         iter != principals.end(); ++iter)
    {
        add_updated_principal(*iter, endpoints, false);
    }
}

void
Authenticator::addr_updated(AddressType type, const std::string& addr,
                            bool poison, bool refetch_groups)
{
    VLOG_DBG(lg, "addr_updated called.");
    Endpoints endpoints;
    if (type == datatypes->mac_type()) {
        ethernetaddr dladdr(addr);
        const DLEntry *dentry = get_dladdr_entry(dladdr);
        if (dentry != NULL) {
            add_updated_entry(dentry, endpoints);
            if (refetch_groups) {
                data_cache->get_groups(dladdr, *dentry->groups);
            }
        }
    } else if (type == datatypes->ipv4_cidr_type()) {
        cidr_ipaddr cidr(addr);
        if (cidr.get_prefix_len() == 32) {
            const NWEntry *nentry = get_nwaddr_entry(
                ntohl(cidr.addr.addr));
            if (nentry != NULL) {
                add_updated_entry(nentry, endpoints);
                if (refetch_groups) {
                    data_cache->get_groups(cidr.addr, *nentry->groups);
                }
            }
        } else {
            add_updated_cidr(cidr, endpoints, refetch_groups);
        }
    }
    endpoints_updated(endpoints, poison);
}

void
Authenticator::all_updated(bool poison) const
{
    VLOG_DBG(lg, "all_updated called.");
    Endpoints endpoints;
    for (DLMap::const_iterator diter = hosts_by_dladdr.begin();
         diter != hosts_by_dladdr.end(); ++diter)
    {
        add_updated_entry(&diter->second, endpoints);
    }
    endpoints_updated(endpoints, poison);
}

void
Authenticator::add_updated_principal(const Principal& principal,
                                     Endpoints& endpoints, bool refetch_groups)
{
    GroupList *groups = NULL;

    if (principal.type == datatypes->switch_type()) {
        const SwitchEntry *sentry = get_switch_entry(principal.id);
        if (sentry != NULL) {
            groups = &sentry->entry->groups;
            add_updated_entry(sentry, endpoints);
        }
    } else if (principal.type == datatypes->location_type()) {
        const LocEntry *lentry = get_location_entry(principal.id);
        if (lentry != NULL) {
            groups = &lentry->entry->groups;
            add_updated_entry(lentry, endpoints);
        }
    } else if (principal.type == datatypes->host_type()) {
        const HostEntry *hentry = get_host_entry(principal.id);
        if (hentry != NULL) {
            groups = &hentry->entry->groups;
            add_updated_entry(hentry, endpoints);
        }
    } else if (principal.type == datatypes->user_type()) {
        const UserEntry *uentry = get_user_entry(principal.id);
        if (uentry != NULL) {
            groups = &uentry->entry->groups;
            add_updated_entry(uentry, endpoints);
        }
    } else if (principal.type == datatypes->host_netid_type()) {
        const HostNetEntry *hnentry = get_host_netid_entry(principal.id);
        if (hnentry != NULL) {
            groups = &hnentry->entry->groups;
            add_updated_entry(hnentry, endpoints);
        }
    } else if (principal.type == datatypes->address_type()) {
        std::string name;
        AddressType address_type = 0;
        if (data_cache->get_address(principal.id, address_type, name)) {
            if (address_type == datatypes->mac_type()) {
                ethernetaddr dladdr(name);
                const DLEntry *dentry = get_dladdr_entry(dladdr);
                if (dentry != NULL) {
                    add_updated_entry(dentry, endpoints);
                    if (refetch_groups) {
                        data_cache->get_groups(dladdr, *dentry->groups);
                    }
                }
            } else if (address_type == datatypes->ipv4_cidr_type()) {
                cidr_ipaddr cidr(name);
                if (cidr.get_prefix_len() == 32) {
                    const NWEntry *nentry = get_nwaddr_entry(
                        ntohl(cidr.addr.addr));
                    if (nentry != NULL) {
                        add_updated_entry(nentry, endpoints);
                        if (refetch_groups) {
                            data_cache->get_groups(cidr.addr, *nentry->groups);
                        }
                    }
                } else {
                    add_updated_cidr(cidr, endpoints, refetch_groups);
                }
            }
        }
    } else if (principal.type == datatypes->group_type()) {
        group_updated(principal.id, endpoints);
    } else {
        VLOG_ERR(lg, "Updated principal type %"PRId64" not supported.",
                 principal.type);
        return;
    }

    if (refetch_groups && groups != NULL) {
        data_cache->get_groups(principal, *groups);
    }
}

void
Authenticator::add_updated_cidr(const cidr_ipaddr& cidr,
                                Endpoints& endpoints,
                                bool refetch_groups)
{
    for (NWMap::iterator iter = hosts_by_nwaddr.begin();
         iter != hosts_by_nwaddr.end(); ++iter)
    {
        ipaddr ip(iter->first);
        if (cidr.matches(ip)) {
            add_updated_entry(&iter->second, endpoints);
            if (refetch_groups) {
                data_cache->get_groups(ip, *iter->second.groups);
            }
        }
    }
}

void
Authenticator::add_updated_entry(const SwitchEntry *entry,
                                 Endpoints& endpoints) const
{
    if (!entry->active) {
        return;
    }

    for (std::list<PortEntry>::const_iterator piter = entry->locations.begin();
         piter != entry->locations.end(); ++piter)
    {
        if (piter->active_location != NULL) {
            add_updated_entry(piter->active_location, endpoints);
        }
    }
}

void
Authenticator::add_updated_entry(const LocEntry *entry,
                                 Endpoints& endpoints) const
{
    if (!entry->active) {
        return;
    }

    for (std::list<DLEntry*>::const_iterator iter = entry->dlentries.begin();
         iter != entry->dlentries.end(); ++iter)
    {
        Endpoints::iterator diter = endpoints.find(*iter);
        if (diter == endpoints.end()) {
            DLNWHash entries;
            entries.insert((*iter)->zero);
            EndpointValue value;
            value[entry->entry] = entries;
            endpoints[*iter] = value;
        } else {
            EndpointValue::iterator viter = diter->second.find(entry->entry);
            if (viter == diter->second.end()) {
                DLNWHash entries;
                entries.insert((*iter)->zero);
                diter->second[entry->entry] = entries;
            } else {
                viter->second.insert((*iter)->zero);
            }
        }
    }
}

void
Authenticator::add_updated_entry(const HostEntry *entry,
                                 Endpoints& endpoints) const
{
    for (std::list<HostNetEntry*>::const_iterator iter = entry->netids.begin();
         iter != entry->netids.end(); ++iter)
    {
        add_updated_entry(*iter, endpoints);
    }
}

void
Authenticator::add_updated_entry(const HostNetEntry *entry,
                                 Endpoints& endpoints) const
{
    std::list<DLNWEntry*>::const_iterator iter = entry->dlnwentries.begin();
    for (; iter != entry->dlnwentries.end(); ++iter) {
        add_updated_entry(*iter, endpoints);
    }
}

void
Authenticator::add_updated_entry(const UserEntry *entry,
                                 Endpoints& endpoints) const
{
    std::list<HostEntry*>::const_iterator iter = entry->hostentries.begin();
    for (; iter != entry->hostentries.end(); ++iter) {
        add_updated_entry(*iter, endpoints);
    }
}

void
Authenticator::add_updated_entry(const DLEntry *entry,
                                 Endpoints& endpoints) const
{
    if (entry->zero != NULL) {
        add_updated_entry(entry->zero, endpoints);
    }
}

void
Authenticator::add_updated_entry(const NWEntry *entry,
                                 Endpoints& endpoints) const
{
    std::list<DLNWEntry*>::const_iterator iter = entry->dlnwentries.begin();
    for (; iter != entry->dlnwentries.end(); ++iter) {
        add_updated_entry(*iter, endpoints);
    }
}

void
Authenticator::add_updated_entry(const DLNWEntry *entry,
                                 Endpoints& endpoints) const
{
    if (!entry->authed) {
        return;
    }
    Endpoints::iterator diter = endpoints.find(entry->dlentry);
    if (diter == endpoints.end()) {
        DLNWHash entries;
        entries.insert(entry);
        EndpointValue value;
        std::list<AuthedLocation>::const_iterator liter
            = entry->dlentry->locations.begin();
        for (; liter != entry->dlentry->locations.end(); ++liter) {
            value[liter->location] = entries;
        }
        endpoints[entry->dlentry] = value;
    } else {
        std::list<AuthedLocation>::const_iterator liter
            = entry->dlentry->locations.begin();
        for ( ;  liter != entry->dlentry->locations.end(); ++liter) {
            EndpointValue::iterator viter
                = diter->second.find(liter->location);
            if (viter == diter->second.end()) {
                DLNWHash entries;
                entries.insert(entry);
                diter->second[liter->location] = entries;
            } else {
                viter->second.insert(entry);
            }
        }
    }
}

void
Authenticator::get_names(const datapathid& dp, uint16_t inport,
                         const ethernetaddr& dlsrc, uint32_t nwsrc,
                         const ethernetaddr& dldst, uint32_t nwdst,
                         PyObject *callable)
{
#ifdef TWISTED_ENABLED
    Flow_in_event fi;
    fi.flow.in_port = htons(inport);
    fi.flow.dl_src = dlsrc;
    fi.flow.nw_src = htonl(nwsrc);
    fi.flow.dl_dst = dldst;
    fi.flow.nw_dst = htonl(nwdst);
    fi.datapath_id = dp;
    bool success = set_flow_in(fi, false);

    PyObject *result = NULL;
    if (success) {
        result = PyDict_New();
        if (result != NULL) {
            pyglue_setdict_string(result, "src_host_name",
                                  name_to_python(fi.src_host_netid->host->name,
                                                 datatypes->host_type()));
            pyglue_setdict_string(result, "src_host_groups",
                                  namelist_to_python(fi.src_host_netid->host->groups));
            pyglue_setdict_string(result, "src_host_netid",
                                  name_to_python(fi.src_host_netid->name,
                                                 datatypes->host_netid_type()));
            pyglue_setdict_string(result, "src_host_netid_groups",
                                  namelist_to_python(fi.src_host_netid->groups));
            pyglue_setdict_string(result, "src_users",
                                  users_to_python(fi.src_host_netid->host->users));
            pyglue_setdict_string(result, "src_location",
                                  name_to_python(fi.src_location.location->name,
                                                 datatypes->location_type()));
            pyglue_setdict_string(result, "src_location_groups",
                                  namelist_to_python(fi.src_location.location->groups));
            pyglue_setdict_string(result, "src_switch",
                                  name_to_python(fi.src_location.location->sw->name,
                                                 datatypes->switch_type()));
            pyglue_setdict_string(result, "src_switch_groups",
                                  namelist_to_python(fi.src_location.location->sw->groups));
            pyglue_setdict_string(result, "src_dladdr_groups",
                                  namelist_to_python(*fi.src_dladdr_groups));
            pyglue_setdict_string(result, "src_nwaddr_groups",
                                  namelist_to_python(*fi.src_nwaddr_groups));
            pyglue_setdict_string(result, "dst_host_name",
                                  name_to_python(fi.dst_host_netid->host->name,
                                                 datatypes->host_type()));
            pyglue_setdict_string(result, "dst_host_groups",
                                  namelist_to_python(fi.dst_host_netid->host->groups));
            pyglue_setdict_string(result, "dst_host_netid",
                                  name_to_python(fi.dst_host_netid->name,
                                                 datatypes->host_netid_type()));
            pyglue_setdict_string(result, "dst_host_netid_groups",
                                  namelist_to_python(fi.dst_host_netid->groups));
            pyglue_setdict_string(result, "dst_users",
                                  users_to_python(fi.dst_host_netid->host->users));
            PyObject *pydlocations = PyList_New(fi.dst_locations.size());
            if (pydlocations == NULL) {
                VLOG_ERR(lg, "Could not create location list for get_names.");
                Py_INCREF(Py_None);
                pydlocations = Py_None;
            } else {
                Flow_in_event::DestinationList::const_iterator dst = fi.dst_locations.begin();
                for (uint32_t i = 0; dst != fi.dst_locations.end(); ++i, ++dst) {
                    PyObject *pyloc = PyDict_New();
                    if (pyloc == NULL) {
                        VLOG_ERR(lg, "Could not create location for get_names");
                        Py_INCREF(Py_None);
                        pyloc = Py_None;
                    } else {
                        pyglue_setdict_string(pyloc, "locname",
                                              name_to_python(dst->authed_location.location->name,
                                                             datatypes->location_type()));
                        pyglue_setdict_string(pyloc, "locgroups",
                                              namelist_to_python(dst->authed_location.location->groups));
                        pyglue_setdict_string(pyloc, "swname",
                                              name_to_python(dst->authed_location.location->sw->name,
                                                             datatypes->switch_type()));
                        pyglue_setdict_string(pyloc, "swgroups",
                                              namelist_to_python(dst->authed_location.location->sw->groups));
                    }
                    if (PyList_SetItem(pydlocations, i, pyloc) != 0) {
                        VLOG_ERR(lg, "Could not set location list in get_names.");
                    }
                }
            }
            pyglue_setdict_string(result, "dst_locations", pydlocations);
            pyglue_setdict_string(result, "dst_dladdr_groups",
                                  namelist_to_python(*fi.dst_dladdr_groups));
            pyglue_setdict_string(result, "dst_nwaddr_groups",
                                  namelist_to_python(*fi.dst_nwaddr_groups));
        }
    } else {
        VLOG_ERR(lg, "Flow_in_event did not complete through chain.");
    }

    if (result == NULL) {
        Py_INCREF(Py_None);
        result = Py_None;
    }

    PyObject *ret = PyObject_CallFunctionObjArgs(callable, result, NULL);
    Py_DECREF(result);
    Py_XDECREF(ret);
}

inline
PyObject*
Authenticator::name_to_python(uint32_t id, PrincipalType t)
{
    Principal p = { t, id };
    return to_python(data_cache->get_name_ref(p));
}

PyObject*
Authenticator::namelist_to_python(const GroupList& ids)
{
    PyObject *pylist = PyList_New(ids.size());
    if (pylist == NULL) {
        VLOG_ERR(lg, "Could not create id list for get_names.");
        Py_RETURN_NONE;
    }

    PrincipalType group_t = datatypes->group_type();
    GroupList::const_iterator iter = ids.begin();
    for (uint32_t i = 0; iter != ids.end(); ++i, ++iter)
    {
        if (PyList_SetItem(pylist, i, name_to_python(*iter, group_t)) != 0) {
            VLOG_ERR(lg, "Could not set id list item in get_names.");
        }
    }
    return pylist;
}

PyObject*
Authenticator::users_to_python(const std::list<AuthedUser>& users)
{
    PyObject *pyusers = PyList_New(users.size());
    if (pyusers == NULL) {
        VLOG_ERR(lg, "Could not create user list for get_names.");
        Py_RETURN_NONE;
    }

    std::list<AuthedUser>::const_iterator user = users.begin();
    for (uint32_t i = 0; user != users.end(); ++i, ++user) {
        PyObject *pyuser = PyDict_New();
        if (pyuser == NULL) {
            VLOG_ERR(lg, "Could not create user dict for get_names.");
            Py_INCREF(Py_None);
            pyuser = Py_None;
        } else {
            pyglue_setdict_string(pyuser, "name",
                                  name_to_python(user->user->name,
                                                 datatypes->user_type()));
            pyglue_setdict_string(pyuser, "groups",
                                  namelist_to_python(user->user->groups));
        }
        if (PyList_SetItem(pyusers, i, pyuser) != 0) {
            VLOG_ERR(lg, "Could not set user list in get_names.");
        }
    }
    return pyusers;
#else
    VLOG_ERR(lg, "Cannot return names for host if Python disabled.");
#endif
}

Authenticator::LocEntry *
Authenticator::get_unauth_location()
{
    int64_t locid =
        data_cache->get_unauthenticated_id(datatypes->location_type());
    LocMap::iterator iter = locations.find(locid);
    if (iter != locations.end()) {
        return &iter->second;
    }
    int64_t switchid
        = data_cache->get_unauthenticated_id(datatypes->switch_type());
    update_switch(switchid, 0, datapathid::from_host(0));
    return update_location(locid, 0, switchid, 0, "");
}

Authenticator::HostNetEntry *
Authenticator::get_unauth_netid()
{
    int64_t netid =
        data_cache->get_unauthenticated_id(datatypes->host_netid_type());
    HostNetMap::iterator iter = host_netids.find(netid);
    if (iter != host_netids.end()) {
        return &iter->second;
    }
    int64_t hostid
        = data_cache->get_unauthenticated_id(datatypes->host_type());
    update_host(hostid);
    return update_host_netid(netid, hostid, false, false);
}

Authenticator::HostNetEntry *
Authenticator::get_auth_netid()
{
    int64_t netid =
        data_cache->get_authenticated_id(datatypes->host_netid_type());
    HostNetMap::iterator iter = host_netids.find(netid);
    if (iter != host_netids.end()) {
        return &iter->second;
    }
    int64_t hostid
        = data_cache->get_authenticated_id(datatypes->host_type());
    update_host(hostid);
    return update_host_netid(netid, hostid, false, false);
}

bool
Authenticator::auth_flow_host(const Flow_in_event& fi, bool auth_default)
{
    Packet_in_event *pi = new Packet_in_event(fi.datapath_id,
                                              ntohs(fi.flow.in_port),
                                              fi.buf, fi.total_len,
                                              fi.buffer_id, fi.reason);

    uint32_t nwaddr = ntohl(fi.flow.nw_src);
    const DLEntry *dlentry = get_dladdr_entry(fi.flow.dl_src);
    const HostNetEntry *netid
        = get_names_from_bindings(dlentry, get_nwaddr_entry(nwaddr));
    if (netid == NULL) {
        if (!auth_default) {
            delete pi;
            return false;
        }
        VLOG_DBG(lg, "Binding for host doesnt exist, using default.");
        netid = get_auth_netid();
    }

    Host_auth_event *ha = new Host_auth_event(pi->datapath_id, pi->in_port,
                                              fi.flow.dl_src, nwaddr,
                                              netid->entry->host->name,
                                              netid->entry->name,
                                              default_idle_timeout,
                                              default_hard_timeout,
                                              Host_event::AUTO_AUTHENTICATION);
    ha->to_post = pi;
    post(ha);
    return true;
}

const Authenticator::HostNetEntry *
Authenticator::get_names_from_bindings(const DLEntry *dlentry,
                                       const NWEntry *nwentry) const
{
    int64_t pri = 0;
    HostNetEntry *netid = NULL;

    if (dlentry != NULL && !dlentry->bindings.empty()) {
        const DLBinding& binding = dlentry->bindings.front();
        pri = binding.priority;
        netid = binding.netid;
    }

    if (nwentry != NULL && !nwentry->bindings.empty()) {
        const NWBinding& binding = nwentry->bindings.front();
        if (netid == NULL || binding.priority < pri || netid->entry->is_router) {
            pri = binding.priority;
            netid = binding.netid;
            return netid;
        }
    }

    if (dlentry != NULL && dlentry->zero != NULL && dlentry->zero->authed
        && !dlentry->zero->host_netid->is_router)
    {
        const HostNetEntry *authed
            = get_host_netid_entry(dlentry->zero->host_netid->name);
        if (authed != NULL) {
            return authed;
        }
    }
    return netid;
}

bool
Authenticator::get_location(const datapathid &dp, uint16_t port,
                            std::list<AuthedLocation> &als,
                            std::list<AuthedLocation>::iterator &al) const
{
    for (al = als.begin(); al != als.end(); ++al) {
        if (al->location->sw->dp == dp && al->location->port == port) {
            return true;
        }
    }

    LocEntry *location = get_active_location(dp, port);
    if (als.empty()
        || location == NULL
        || location->entry->is_virtual)
    {
        return false;
    }

    if (als.front().location->is_virtual) {
        al = als.begin();
        return true;
    }

    return get_on_path_location(dp, port, als, al);
}

bool
Authenticator::get_on_path_location(const datapathid& dp, uint16_t port,
                                    std::list<AuthedLocation>& als,
                                    std::list<AuthedLocation>::iterator &al) const
{
#if AUTH_WITH_ROUTING
    Routing_module::RoutePtr rte;
    Routing_module::RouteId rid;
    rid.dst = dp;
    for (al = als.begin(); al != als.end(); ++al) {
        rid.src = al->location->sw->dp;
        if (routing_mod->is_on_path_location(rid,
                                             al->location->port,
                                             port))
        {
            return true;
        }
    }
#endif
    return false;
}

// nwaddr should be network byte order
bool
Authenticator::is_internal_ip(uint32_t nwaddr) const
{
    std::vector<cidr_ipaddr>::const_iterator iter = internal_subnets.begin();
    for (; iter != internal_subnets.end(); ++iter) {
        VLOG_DBG(lg, "checking internal against nw:%x mask:%x",
                 iter->addr.addr, iter->mask);
        if ((nwaddr & iter->mask) == iter->addr.addr) {
            return true;
        }
    }
    VLOG_DBG(lg, "done checking for internal nw:%x", nwaddr);
    return false;
}

void
Authenticator::add_internal_subnet(const cidr_ipaddr& cidr)
{
    internal_subnets.push_back(cidr);
}

bool
Authenticator::remove_internal_subnet(const cidr_ipaddr& cidr)
{
    for (std::vector<cidr_ipaddr>::iterator iter = internal_subnets.begin();
         iter != internal_subnets.end(); ++iter)
    {
        if (*iter == cidr) {
            internal_subnets.erase(iter);
            return true;
        }
    }
    return false;
}

void
Authenticator::clear_internal_subnets()
{
    internal_subnets.clear();
}

void
Authenticator::expire_entities()
{
    timeval curtime = { 0, 0 };
    gettimeofday(&curtime, NULL);

    expire_host_locations(curtime);
    expire_hosts(curtime);

    curtime.tv_sec = expire_timer;
    curtime.tv_usec = 0;
    post(boost::bind(&Authenticator::expire_entities, this), curtime);
}

void
Authenticator::expire_host_locations(const timeval& curtime)
{
    for (DLMap::iterator dliter = hosts_by_dladdr.begin();
         dliter != hosts_by_dladdr.end();)
    {
        // timeout idle locations
        std::list<AuthedLocation>::iterator liter
            = dliter->second.locations.begin();
        for(; liter != dliter->second.locations.end(); ) {
            if (liter->idle_timeout != 0
                && liter->last_active + liter->idle_timeout <= curtime.tv_sec)
            {
                remove_dl_location(&dliter->second, liter,
                                   Host_event::IDLE_TIMEOUT, true);
            } else {
                ++liter;
            }
        }

        // expire unused address entries
        std::vector<uint32_t> remove_keys;
        for (DLNWMap::iterator dlnwiter = dliter->second.dlnwentries.begin();
             dlnwiter != dliter->second.dlnwentries.end(); ++dlnwiter)
        {
            if (!dlnwiter->second.authed
                && (dlnwiter->second.host_netid == NULL
                    || dlnwiter->second.exp_time <= curtime.tv_sec))
            {
//                 VLOG_DBG(lg, "Expiring address entry %s %s.",
//                          dliter->second.dladdr.string().c_str(),
//                          ipaddr(dlnwiter->second.nwaddr).string().c_str());

                remove_dlnw_host(&dlnwiter->second, Host_event::HARD_TIMEOUT,
                                 true);

                if (dlnwiter->first == 0) {
                    dliter->second.zero = NULL;
                }

                NWMap::iterator nwiter
                    = hosts_by_nwaddr.find(dlnwiter->second.nwaddr);
                if (nwiter != hosts_by_nwaddr.end()) {
                    std::list<DLNWEntry*>::iterator niter
                        = nwiter->second.dlnwentries.begin();
                    for (; niter != nwiter->second.dlnwentries.end(); ++niter) {
                        if (*niter == &dlnwiter->second) {
                            nwiter->second.dlnwentries.erase(niter);
                            if (nwiter->second.dlnwentries.empty()
                                && nwiter->second.bindings.empty())
                            {
                                hosts_by_nwaddr.erase(nwiter);
                            }
                            break;
                        }
                    }
                }
                remove_keys.push_back(dlnwiter->first);
            }
        }

        for (std::vector<uint32_t>::const_iterator riter = remove_keys.begin();
             riter != remove_keys.end(); ++riter)
        {
            dliter->second.dlnwentries.erase(*riter);
        }

        if (dliter->second.dlnwentries.empty()
            && dliter->second.bindings.empty()
            && dliter->second.exp_time <= curtime.tv_sec)
        {
//             VLOG_DBG(lg, "Expiring address entry %s.",
//                      dliter->second.dladdr.string().c_str());
            dliter = hosts_by_dladdr.erase(dliter);
        } else {
            ++dliter;
        }
    }

    for (NWMap::iterator nwiter = hosts_by_nwaddr.begin();
         nwiter != hosts_by_nwaddr.end();)
    {
        if (nwiter->second.dlnwentries.empty()
            && nwiter->second.bindings.empty()
            && nwiter->second.exp_time <= curtime.tv_sec)
        {
//             VLOG_DBG(lg, "Expiring address entry %s.",
//                      ipaddr(nwiter->first).string().c_str());
            nwiter = hosts_by_nwaddr.erase(nwiter);
        } else {
            ++nwiter;
        }
    }
}

void
Authenticator::expire_hosts(const timeval& curtime)
{
    for (HostMap::iterator hiter = hosts.begin();
         hiter != hosts.end(); ++hiter)
    {
        HostEntry *entry = &hiter->second;
        Host& host = *(entry->entry);
        if (host.users.size() == 1
            && host.users.front().user->name
            == data_cache->get_unauthenticated_id(datatypes->user_type()))
        {
            if (host.hard_timeout != 0
                && host.auth_time + host.hard_timeout <= curtime.tv_sec
                && host.name !=
                data_cache->get_unauthenticated_id(datatypes->host_type()))
            {
                // hard_timeout hosts
                remove_host(entry, datapathid::from_host(0), 0,
                            true, Host_event::HARD_TIMEOUT);
            }
        } else {
            // timeout users
            for (std::list<AuthedUser>::iterator uiter = host.users.begin();
                 uiter != host.users.end();)
            {
                AuthedUser& user = *uiter;
                ++uiter;
                if (user.hard_timeout != 0
                    && user.auth_time + user.hard_timeout <= curtime.tv_sec)
                {
                    remove_user(&users[user.user->name], entry,
                                User_event::HARD_TIMEOUT, true);
                } else if (user.idle_timeout != 0
                           && host.last_active + user.idle_timeout
                           <= curtime.tv_sec)
                {
                    remove_user(&users[user.user->name], entry,
                                User_event::IDLE_TIMEOUT, true);
                }
            }
        }
    }
}

#define CHECK_POISON_ERR(error, dp)                                            \
    if (error) {                                                               \
        if (error == EAGAIN) {                                                 \
            VLOG_DBG(lg, "Poison location on switch %"PRIx64" failed "         \
                     "with EAGAIN.", dp.as_host());                            \
        } else {                                                               \
            VLOG_ERR(lg, "Poison location on switch %"PRIx64" failed "         \
                     "with %d:%s.", dp.as_host(), error, strerror(error));     \
        }                                                                      \
        return;                                                                \
    }

void
Authenticator::poison_port(const datapathid& dp, uint16_t port) const
{
    ofp_match& match = ofm->match;

    match.wildcards = htonl(OFPFW_ALL & (~OFPFW_IN_PORT));
    match.in_port = htons(port);
    int err = send_openflow_command(dp, &ofm->header, false);
    CHECK_POISON_ERR(err, dp);
}

void
Authenticator::poison_location(const datapathid& dp,
                               const ethernetaddr& dladdr,
                               uint32_t nwaddr, bool wildcard_nw) const
{
    ofp_match& match = ofm->match;

    memcpy(match.dl_dst, dladdr.octet, ethernetaddr::LEN);
    match.wildcards = htonl(OFPFW_ALL & (~OFPFW_DL_DST));
    if (!wildcard_nw) {
        match.dl_type = ethernet::IP;
        match.nw_dst = htonl(nwaddr);
        match.wildcards &= htonl(~(OFPFW_NW_DST_MASK | OFPFW_DL_TYPE));
    }

    VLOG_DBG(lg, "Poisoning %s %s on location %"PRIx64".",
             dladdr.string().c_str(),
             wildcard_nw ? "" : ipaddr(nwaddr).string().c_str(),
             dp.as_host());

    int err = send_openflow_command(dp, &ofm->header, false);
    CHECK_POISON_ERR(err, dp);

    memcpy(match.dl_src, dladdr.octet, ethernetaddr::LEN);
    match.wildcards = htonl(OFPFW_ALL & (~OFPFW_DL_SRC));
    if (!wildcard_nw) {
        match.nw_src = match.nw_dst;
        match.wildcards &= htonl(~(OFPFW_NW_SRC_MASK | OFPFW_DL_TYPE));
    }

    err = send_openflow_command(dp, &ofm->header, false);
    CHECK_POISON_ERR(err, dp);
}

void
Authenticator::endpoints_updated(const Endpoints& endpoints, bool poison) const
{
    for (Endpoints::const_iterator iter = endpoints.begin();
         iter != endpoints.end(); ++iter)
    {
        for (EndpointValue::const_iterator viter = iter->second.begin();
             viter != iter->second.end(); ++viter)
        {
            if (poison
                && viter->second.find(iter->first->zero)
                != viter->second.end())
            {
                poison_location(viter->first->sw->dp,
                                iter->first->dladdr, 0, true);
                call_updated_fns(viter->first, iter->first, true);
            } else {
                for (DLNWHash::const_iterator niter = viter->second.begin();
                     niter != viter->second.end(); ++niter)
                {
                    if (poison) {
                        poison_location(viter->first->sw->dp,
                                        iter->first->dladdr,
                                        (*niter)->nwaddr, false);
                    }
                    call_updated_fns(viter->first, *niter, poison);
                }
            }
        }
    }
}

void
Authenticator::call_updated_fns(const boost::shared_ptr<Location>& lentry,
                                const DLNWEntry *nwentry, bool poisoned) const
{
    if (!nwentry->authed) {
        return;
    }

    for (std::list<EndpointUpdatedFn>::const_iterator fn = updated_fns.begin();
         fn != updated_fns.end(); ++fn)
    {
        (*fn)(lentry, nwentry, poisoned);
    }
}

void
Authenticator::call_updated_fns(const boost::shared_ptr<Location>& lentry,
                                const DLEntry *dlentry, bool poisoned) const
{
    for (DLNWMap::const_iterator iter = dlentry->dlnwentries.begin();
         iter != dlentry->dlnwentries.end(); ++iter)
    {
        call_updated_fns(lentry, &iter->second, poisoned);
    }
}

void
Authenticator::add_endpoint_updated_fn(const EndpointUpdatedFn& fn)
{
    updated_fns.push_back(fn);
}

#define OUI_MASK 0x3fffff000000ULL
#define OUI      0x002320000000ULL

bool
Authenticator::is_internal_mac(uint64_t hb_dladdr) const
{
    return ((OUI_MASK & hb_dladdr) == OUI);
}

}
}

REGISTER_COMPONENT(vigil::container::Simple_component_factory
                   <vigil::applications::Authenticator>,
                   vigil::applications::Authenticator);
