/* Copyright 2009 (C) Nicira, Inc.
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

#include "datacache.hh"

#include <boost/bind.hpp>
#include "principal_event.hh"
#include "vlog.hh"

#define SWITCH     0
#define LOCATION   1
#define HOST       2
#define HOST_NETID 3
#define USER       4

#define UNAUTHENTICATED "unauthenticated"
#define AUTHENTICATED   "authenticated"
#define UNKNOWN         "unknown"

namespace vigil {
namespace applications {

static Vlog_module lg("data_cache");

Data_cache::Data_cache(const container::Context* c,
                       const json_object*)
    : Component(c), reserved(5),
      unauthenticated(UNAUTHENTICATED),
      authenticated(AUTHENTICATED), unknown(UNKNOWN)
{}

void
Data_cache::getInstance(const container::Context* ctxt,
                        Data_cache*& d)
{
    d = dynamic_cast<Data_cache*>
        (ctxt->get_by_interface(container::Interface_description
                                (typeid(Data_cache).name())));
}

void
Data_cache::configure(const container::Configuration*)
{
    resolve(datatypes);
    register_event(Principal_delete_event::static_get_name());
}

void
Data_cache::install()
{
    // ugly...
    Principal add = { 0, 0 };
    add.type = datatypes->switch_type();
    add.id = reserved[SWITCH].unauth;
    add_id_mapping(add, unauthenticated);
    add.id = reserved[SWITCH].auth;
    add_id_mapping(add, authenticated);
    add.id = reserved[SWITCH].unknown;
    add_id_mapping(add, unknown);

    add.type = datatypes->location_type();
    add.id = reserved[LOCATION].unauth;
    add_id_mapping(add, unauthenticated);
    add.id = reserved[LOCATION].auth;
    add_id_mapping(add, authenticated);
    add.id = reserved[LOCATION].unknown;
    add_id_mapping(add, unknown);

    add.type = datatypes->host_type();
    add.id = reserved[HOST].unauth;
    add_id_mapping(add, unauthenticated);
    add.id = reserved[HOST].auth;
    add_id_mapping(add, authenticated);
    add.id = reserved[HOST].unknown;
    add_id_mapping(add, unknown);

    add.type = datatypes->host_netid_type();
    add.id = reserved[HOST_NETID].unauth;
    add_id_mapping(add, unauthenticated);
    add.id = reserved[HOST_NETID].auth;
    add_id_mapping(add, authenticated);
    add.id = reserved[HOST_NETID].unknown;
    add_id_mapping(add, unknown);

    add.type = datatypes->user_type();
    add.id = reserved[USER].unauth;
    add_id_mapping(add, unauthenticated);
    add.id = reserved[USER].auth;
    add_id_mapping(add, authenticated);
    add.id = reserved[USER].unknown;
    add_id_mapping(add, unknown);
}

int64_t
Data_cache::get_authenticated_id(PrincipalType ptype) const
{
    int32_t i = get_reserved_index(ptype);
    if (i < 0) {
        VLOG_ERR(lg, "Requesting unsupported principal type reserved name.");
        return -1;
    }
    return reserved[i].auth;
}

int64_t
Data_cache::get_unauthenticated_id(PrincipalType ptype) const
{
    int32_t i = get_reserved_index(ptype);
    if (i < 0) {
        VLOG_ERR(lg, "Requesting unsupported principal type reserved name.");
        return -1;
    }
    return reserved[i].unauth;
}

int64_t
Data_cache::get_unknown_id(PrincipalType ptype) const
{
    int32_t i = get_reserved_index(ptype);
    if (i < 0) {
        VLOG_ERR(lg, "Requesting unsupported principal type reserved name.");
        return -1;
    }
    return reserved[i].unknown;
}


bool
Data_cache::is_reserved_name(const std::string& name)
{
    return name == unauthenticated || name == authenticated
        || name == unknown;
}

int64_t
Data_cache::get_reserved_index(PrincipalType ptype) const
{
    if (ptype == datatypes->switch_type()) {
        return SWITCH;
    } else if (ptype == datatypes->location_type()) {
        return LOCATION;
    } else if (ptype == datatypes->host_type()) {
        return HOST;
    } else if (ptype == datatypes->host_netid_type()) {
        return HOST_NETID;
    } else if (ptype == datatypes->user_type()) {
        return USER;
    }
    return -1;
}

void
Data_cache::update_reserved_id(PrincipalType ptype, int64_t id,
                               const std::string& name)
{
    int32_t i = get_reserved_index(ptype);
    if (i < 0) {
        return;
    }

    Principal del = { ptype, 0 };
    Principal add = { ptype, 0 };
    if (name == unauthenticated) {
        del.id = reserved[i].unauth;
        reserved[i].unauth = id;
        add.id = id;
    } else if (name == authenticated) {
        del.id = reserved[i].auth;
        reserved[i].auth = id;
        add.id = id;
    } else if (name == unknown) {
        del.id = reserved[i].unknown;
        reserved[i].unknown = id;
        add.id = id;
    } else {
        return;
    }

    delete_principal(del);
    add_id_mapping(add, name);
}

void
Data_cache::delete_principal(const Principal& principal)
{
    if (principal.type == datatypes->address_type()) {
        AddressType type = 0;
        std::string name;
        if (get_address(principal.id, type, name, false)) {
            remove_addr_binding(principal.id, type, name);
        }
    }

    post(new Principal_delete_event(principal.type, principal.id));

    std::list<PrincipalDeletedFn>::const_iterator fn
        = principal_deleted_fns.begin();
    for (; fn != principal_deleted_fns.end(); ++fn) {
        (*fn)(principal);
    }
    delete_id_mapping(principal);
}

void
Data_cache::update_switch(int64_t id, const std::string& name,
                          int64_t dp_priority, const datapathid& dp)
{
    Principal principal = { datatypes->switch_type(), id };
    add_id_mapping(principal, name);
    std::list<SwitchUpdatedFn>::const_iterator fn = switch_updated_fns.begin();
    for (; fn != switch_updated_fns.end(); ++fn) {
        (*fn)(id, dp_priority, dp);
    }
}

void
Data_cache::update_location(int64_t id, const std::string& name,
                            int64_t port_priority, int64_t switch_id,
                            int64_t port, const std::string& portname)
{
    Principal principal = { datatypes->location_type(), id };
    add_id_mapping(principal, name);
    std::list<LocationUpdatedFn>::const_iterator fn
        = location_updated_fns.begin();
    for (; fn != location_updated_fns.end(); ++fn) {
        (*fn)(id, port_priority, switch_id, port, portname);
    }
}

void
Data_cache::update_host(int64_t id, const std::string& name)
{
    Principal principal = { datatypes->host_type(), id };
    add_id_mapping(principal, name);
    std::list<HostUpdatedFn>::const_iterator fn = host_updated_fns.begin();
    for (; fn != host_updated_fns.end(); ++fn) {
        (*fn)(id);
    }
}

void
Data_cache::update_host_netid(int64_t id, const std::string& name,
                              int64_t host_id, bool is_router,
                              bool is_gateway)
{
    Principal principal = { datatypes->host_netid_type(), id };
    add_id_mapping(principal, name);
    std::list<NetidUpdatedFn>::const_iterator fn
        = netid_updated_fns.begin();
    for (; fn != netid_updated_fns.end(); ++fn) {
        (*fn)(id, host_id, is_router, is_gateway);
    }
}

void
Data_cache::update_netid_binding(int64_t addr_id, int64_t host_netid,
                                 int64_t priority, const std::string& addr,
                                 AddressType type, bool deleted)
{
    std::list<NetidBindingUpdatedFn>::const_iterator fn
        = netid_binding_updated_fns.begin();
    for (; fn != netid_binding_updated_fns.end(); ++fn) {
        (*fn)(addr_id, host_netid, priority, addr, type, deleted);
    }
}

void
Data_cache::update_user(int64_t id, const std::string& name)
{
    Principal principal = { datatypes->user_type(), id };
    add_id_mapping(principal, name);
    std::list<UserUpdatedFn>::const_iterator fn = user_updated_fns.begin();
    for (; fn != user_updated_fns.end(); ++fn) {
        (*fn)(id);
    }
}

void
Data_cache::update_group(int64_t id, const std::string& name)
{
    Principal principal = { datatypes->group_type(), id };
    add_id_mapping(principal, name);
    std::list<GroupUpdatedFn>::const_iterator fn = group_updated_fns.begin();
    for (; fn != group_updated_fns.end(); ++fn) {
        (*fn)(id);
    }
}

void
Data_cache::update_address(int64_t id, const std::string& name,
                           AddressType type)
{
    bool removed = false;
    bool add = true;
    AddressType oldtype = 0;
    std::string oldname;
    if (get_address(id, type, oldname, false)) {
        if (type != oldtype || name != oldname) {
            remove_addr_binding(id, oldtype, oldname);
            removed = true;
        } else {
            add = false;
        }
    }

    if (add && name != "") {
        Principal principal = { datatypes->address_type(), id };
        add_id_mapping(principal, name);
        address_types[id] = type;
        if (type == datatypes->mac_type()) {
            add_dladdr_id(id, name);
        } else if (type == datatypes->ipv4_cidr_type()) {
            add_cidr_id(id, name);
        } else {
            VLOG_WARN(lg, "Unknown address type %"PRId64" %s.",
                      type, name.c_str());
        }
        std::list<AddrGroupsUpdatedFn>::const_iterator fn
            = addr_groups_updated_fns.begin();
        for (; fn != addr_groups_updated_fns.end(); ++fn) {
            (*fn)(type, name);
        }
        if (removed) {
            std::list<AddressUpdatedFn>::const_iterator fn2
                = address_updated_fns.begin();
            for (; fn2 != address_updated_fns.end(); ++fn2) {
                (*fn2)(id);
            }
        }
    }
}

void
Data_cache::update_group_member(int64_t id, int64_t group,
                                const Principal& principal,
                                ModifiedState& mod_state)
{
    if (!remove_group_member(id, group, principal, false, mod_state)) {
        return;
    }

    Membership membership = { group, principal };
    memberships[id] = membership;

    ParentMap::iterator p_parents = parents.find(principal);
    if (p_parents == parents.end()) {
        parents[principal] = Parents(1, group);
    } else {
        p_parents->second.push_back(group);
    }

    hash_map<int64_t, Members>::iterator g_members = members.find(group);
    if (g_members == members.end()) {
        members[group] = Members(1, principal);
    } else {
        g_members->second.push_back(principal);
    }
    // add members
    add_members(membership.principal, mod_state);
    mod_state.groups.insert(group);
}

void
Data_cache::delete_group_member(int64_t id, ModifiedState& mod_state)
{
    Principal principal = { 0, 0 };
    remove_group_member(id, 0, principal, true, mod_state);
}

bool
Data_cache::remove_group_member(int64_t id, int64_t group,
                                const Principal& principal, bool deleted,
                                ModifiedState& mod_state)
{
    hash_map<int64_t, Membership>::iterator miter = memberships.find(id);
    if (miter == memberships.end()) {
        return true;
    }

    Membership& membership = miter->second;
    if (!deleted && membership.group == group
        && membership.principal.type == principal.type
        && membership.principal.id == principal.id)
    {
        return false;
    }

    bool not_found = true;
    ParentMap::iterator p_parents = parents.find(membership.principal);
    if (p_parents != parents.end()) {
        for (Parents::iterator parent = p_parents->second.begin();
             parent != p_parents->second.end(); ++parent)
        {
            if (*parent == membership.group) {
                not_found = false;
                p_parents->second.erase(parent);
                if (p_parents->second.empty()) {
                    parents.erase(p_parents);
                }
                break;
            }
        }
    }

    if (not_found) {
        VLOG_ERR(lg, "Member %"PRId64" %"PRId64" parent %"PRId64" not found.",
                 membership.principal.type, membership.principal.id,
                 membership.group);
    }

    not_found = true;
    hash_map<int64_t, Members>::iterator g_members
        = members.find(membership.group);
    if (g_members != members.end()) {
        for (Members::iterator member = g_members->second.begin();
             member != g_members->second.end(); ++member)
        {
            if (member->type == membership.principal.type
                && member->id == membership.principal.id)
            {
                not_found = false;
                g_members->second.erase(member);
                if (g_members->second.empty()) {
                    members.erase(g_members);
                }
                break;
            }
        }
    }

    if (not_found) {
        VLOG_ERR(lg, "Group %"PRId64" member %"PRId64" %"PRId64" not found.",
                 membership.group, membership.principal.type,
                 membership.principal.id);
    }

    add_members(membership.principal, mod_state);
    mod_state.groups.insert(membership.group);

    memberships.erase(miter);
    return true;
}

void
Data_cache::add_members(const Principal& principal,
                        ModifiedState& mod_state) const
{
    if (principal.type != datatypes->group_type()) {
        mod_state.principals.insert(principal);
        return;
    }

    if (mod_state.processed.find(principal.id) == mod_state.processed.end()) {
        mod_state.processed.insert(principal.id);
        hash_map<int64_t, Members>::const_iterator g_members
            = members.find(principal.id);
        if (g_members == members.end()) {
            return;
        }

        for (Members::const_iterator member = g_members->second.begin();
             member != g_members->second.end(); ++member)
        {
            add_members(*member, mod_state);
        }
    }
}

void
Data_cache::add_id_mapping(Principal& principal,
                           const std::string& name)
{
    id_to_name[principal] = name;
}

void
Data_cache::delete_id_mapping(const Principal& principal)
{
    id_to_name.erase(principal);
}

void
Data_cache::add_dladdr_id(int64_t id, const std::string& name)
{
    ethernetaddr dladdr(name);
    uint64_t key = dladdr.hb_long();
    hash_map<uint64_t, std::list<int64_t> >::iterator found
        = dladdr_to_id.find(key);
    if (found == dladdr_to_id.end()) {
        dladdr_to_id[key] = std::list<int64_t>(1, id);
    } else {
        found->second.push_back(id);
    }
}

void
Data_cache::add_cidr_id(int64_t id, const std::string& name)
{
    cidr_ipaddr cidr(name);
    if (cidr.get_prefix_len() == 32) {
        uint32_t key = ntohl(cidr.addr.addr);
        hash_map<uint32_t, std::list<int64_t> >::iterator found
            = nwaddr_to_id.find(key);
        if (found == nwaddr_to_id.end()) {
            found = nwaddr_to_id.insert(
                std::make_pair(key, std::list<int64_t>())).first;
        }
        found->second.push_back(id);
    } else {
        id_to_cidr[id] = cidr;
    }
}

void
Data_cache::remove_addr_binding(int64_t id, AddressType type,
                                const std::string name)
{
    address_types.erase(id);
    if (type == datatypes->mac_type()) {
        remove_dladdr_id(id, name);
    } else if (type == datatypes->ipv4_cidr_type()) {
        remove_cidr_id(id, name);
    }
    std::list<AddrGroupsUpdatedFn>::const_iterator fn
        = addr_groups_updated_fns.begin();
    for (; fn != addr_groups_updated_fns.end(); ++fn) {
        (*fn)(type, name);
    }
}

void
Data_cache::remove_dladdr_id(int64_t id, const std::string& name)
{
    ethernetaddr dladdr(name);
    hash_map<uint64_t, std::list<int64_t> >::iterator entry
        = dladdr_to_id.find(dladdr.hb_long());
    for (std::list<int64_t>::iterator iter = entry->second.begin();
         iter != entry->second.end(); ++iter)
    {
        if (*iter == id) {
            entry->second.erase(iter);
            if (entry->second.empty()) {
                dladdr_to_id.erase(entry);
            }
            break;
        }
    }
}

void
Data_cache::remove_cidr_id(int64_t id, const std::string& name)
{
    cidr_ipaddr cidr(name);
    if (cidr.get_prefix_len() == 32) {
        hash_map<uint32_t, std::list<int64_t> >::iterator entry
            = nwaddr_to_id.find(ntohl(cidr.addr.addr));
        for (std::list<int64_t>::iterator iter = entry->second.begin();
             iter != entry->second.end(); ++iter)
        {
            if (*iter == id) {
                entry->second.erase(iter);
                if (entry->second.empty()) {
                    nwaddr_to_id.erase(entry);
                }
                break;
            }
        }
    } else {
        id_to_cidr.erase(id);
    }
}

bool
Data_cache::get_name(const Principal& principal, std::string& name) const
{
    IDMap::const_iterator id_entry = id_to_name.find(principal);
    if (id_entry != id_to_name.end()) {
        name = id_entry->second;
        return true;
    }
    return false;
}

const std::string&
Data_cache::get_name_ref(const Principal& principal) const
{
    IDMap::const_iterator id_entry = id_to_name.find(principal);
    if (id_entry != id_to_name.end()) {
        return id_entry->second;
    }
    return unknown;
}

bool
Data_cache::get_address(int64_t id, AddressType& type,
                        std::string& name, bool expect_addr) const
{
    Principal principal = { datatypes->address_type(), id };
    hash_map<int64_t, AddressType>::const_iterator atype
        = address_types.find(id);
    if (atype == address_types.end()) {
        return false;
    }

    type = atype->second;

    IDMap::const_iterator id_entry = id_to_name.find(principal);
    if (id_entry == id_to_name.end()) {
        if (expect_addr) {
            VLOG_ERR(lg, "Address mapping for %"PRId64" doesn't exist.", id);
        }
        return false;
    }

    name = id_entry->second;
    return true;
}

void
Data_cache::get_groups(const Principal& principal, GroupList& group_list) const
{
    std::list<int64_t> p_parents;
    add_parents(principal, p_parents);
    set_group_list(p_parents, group_list);
}

void
Data_cache::get_groups(const ethernetaddr& dladdr, GroupList& group_list) const
{
    hash_map<uint64_t, std::list<int64_t> >::const_iterator ids =
        dladdr_to_id.find(dladdr.hb_long());
    if (ids == dladdr_to_id.end()) {
        group_list.clear();
        return;
    }

    std::list<int64_t> p_parents;
    Principal principal = { datatypes->address_type(), 0 };
    for (std::list<int64_t>::const_iterator id = ids->second.begin();
         id != ids->second.end(); ++id)
    {
        principal.id = *id;
        add_parents(principal, p_parents);
    }

    set_group_list(p_parents, group_list);
}

void
Data_cache::get_groups(const ipaddr& nwaddr, GroupList& group_list) const
{
    std::list<int64_t> p_parents;
    Principal principal = { datatypes->address_type(), 0 };

    hash_map<uint32_t, std::list<int64_t> >::const_iterator ids =
        nwaddr_to_id.find(ntohl(nwaddr.addr));
    if (ids != nwaddr_to_id.end()) {
        for (std::list<int64_t>::const_iterator id = ids->second.begin();
             id != ids->second.end(); ++id)
        {
            principal.id = *id;
            add_parents(principal, p_parents);
        }
    }

    hash_map<int64_t, cidr_ipaddr>::const_iterator cidr = id_to_cidr.begin();
    for (; cidr != id_to_cidr.end(); ++cidr) {
        if (cidr->second.matches(nwaddr)) {
            principal.id = cidr->first;
            add_parents(principal, p_parents);
        }
    }

    set_group_list(p_parents, group_list);
}

void
Data_cache::get_all_group_members(const std::list<int64_t>& group_list,
                                  PrincipalSet& principals) const
{
    ModifiedState mod_state;
    Principal group = { datatypes->group_type(), 0 };
    for (std::list<int64_t>::const_iterator iter = group_list.begin();
         iter != group_list.end(); ++iter)
    {
        group.id = *iter;
        add_members(group, mod_state);
    }

    principals.swap(mod_state.principals);
}

bool
Data_cache::add_group(int64_t group, std::list<int64_t>& parent_list) const
{
    for (std::list<int64_t>::iterator iter = parent_list.begin();
         iter != parent_list.end(); ++iter)
    {
        if (*iter > group) {
            parent_list.insert(iter, group);
            return true;
        } else if (*iter == group) {
            return false;
        }
    }
    parent_list.push_back(group);
    return true;
}

void
Data_cache::add_parents(const Principal& principal,
                        std::list<int64_t>& parent_list) const
{
    ParentMap::const_iterator p_parents = parents.find(principal);
    if (p_parents == parents.end()) {
        return;
    }

    for (Parents::const_iterator parent = p_parents->second.begin();
         parent != p_parents->second.end(); ++parent)
    {
        if (add_group(*parent, parent_list)) {
            Principal parent_principal = { datatypes->group_type(), *parent };
            add_parents(parent_principal, parent_list);
        }
    }
}

void
Data_cache::set_group_list(const std::list<int64_t>& parent_list,
                           GroupList& group_list) const
{
    group_list.resize(parent_list.size());
    GroupList::iterator group = group_list.begin();
    for (std::list<int64_t>::const_iterator iter = parent_list.begin();
         iter != parent_list.end(); ++iter, ++group)
    {
        *group = *iter;
    }
}

void
Data_cache::process_modified_state(const ModifiedState& mod_state) const
{
    if (!mod_state.principals.empty()) {
        std::list<GroupsUpdatedFn>::const_iterator fn
            = groups_updated_fns.begin();
        for (; fn != groups_updated_fns.end(); ++fn) {
            (*fn)(mod_state.principals);
        }
    }
    if (!mod_state.groups.empty()) {
        std::list<MembersUpdatedFn>::const_iterator fn
            = members_updated_fns.begin();
        for (; fn != members_updated_fns.end(); ++fn) {
            (*fn)(mod_state.groups);
        }
    }
}

}
}

REGISTER_COMPONENT(vigil::container::Simple_component_factory
                   <vigil::applications::Data_cache>,
                   vigil::applications::Data_cache);
