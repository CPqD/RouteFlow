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
#ifndef DATA_CACHE_HH
#define DATA_CACHE_HH 1

#include "component.hh"
#include "data/datatypes.hh"
#include "netinet++/datapathid.hh"
#include "netinet++/ethernetaddr.hh"
#include "netinet++/ipaddr.hh"
#include "netinet++/cidr.hh"
namespace vigil {
namespace applications {

class Data_cache
    : public container::Component {

public:
    Data_cache(const container::Context*, const json_object*);
    Data_cache() : Component(0) { }

    static void getInstance(const container::Context*, Data_cache*&);

    void configure(const container::Configuration*);
    void install();

    int64_t get_authenticated_id(PrincipalType ptype) const;
    int64_t get_unauthenticated_id(PrincipalType ptype) const;
    int64_t get_unknown_id(PrincipalType ptype) const;

    const std::string& get_authenticated_name() const { return authenticated; };
    const std::string& get_unauthenticated_name() const { return unauthenticated; };
    const std::string& get_unknown_name() const { return unknown; };

    bool is_reserved_name(const std::string& name);

    bool get_name(const Principal& principal, std::string& name) const;
    const std::string& get_name_ref(const Principal& principal) const;
    bool get_address(int64_t id, AddressType& type,
                     std::string& name, bool expect_addr=true) const;
    void get_groups(const Principal& principal, GroupList& group_list) const;
    void get_groups(const ethernetaddr& dladdr, GroupList& group_list) const;
    void get_groups(const ipaddr& nwaddr, GroupList& group_list) const;
    void get_all_group_members(const std::list<int64_t>& group_list,
                               PrincipalSet& principals) const;

    typedef boost::function<void(int64_t)>               PrincipalUpdatedFn;
    typedef boost::function<void(int64_t, int64_t,
                                 const datapathid& dp)>  SwitchUpdatedFn;
    typedef boost::function<void(int64_t, int64_t,
                                 int64_t, uint16_t,
                                 const std::string&)>    LocationUpdatedFn;
    typedef PrincipalUpdatedFn                           HostUpdatedFn;
    typedef boost::function<void(int64_t, int64_t,
                                 bool, bool)>            NetidUpdatedFn;
    typedef boost::function<void(int64_t, int64_t,
                                 int64_t,
                                 const std::string&,
                                 AddressType, bool)>     NetidBindingUpdatedFn;
    typedef PrincipalUpdatedFn                           UserUpdatedFn;
    typedef PrincipalUpdatedFn                           GroupUpdatedFn;
    typedef PrincipalUpdatedFn                           AddressUpdatedFn;
    typedef boost::function<void(const Principal&)>      PrincipalDeletedFn;
    typedef boost::function<void(const
                                 hash_set<int64_t>&)>    MembersUpdatedFn;
    typedef boost::function<void(const PrincipalSet&)>   GroupsUpdatedFn;
    typedef boost::function<void(AddressType,
                                 const std::string&)>    AddrGroupsUpdatedFn;

    void add_switch_updated(const SwitchUpdatedFn& fn)
        { switch_updated_fns.push_back(fn); }
    void add_location_updated(const LocationUpdatedFn& fn)
        { location_updated_fns.push_back(fn); }
    void add_host_updated(const HostUpdatedFn& fn)
        { host_updated_fns.push_back(fn); }
    void add_netid_updated(const NetidUpdatedFn& fn)
        { netid_updated_fns.push_back(fn); }
    void add_netid_binding_updated(const NetidBindingUpdatedFn& fn)
        { netid_binding_updated_fns.push_back(fn); }
    void add_user_updated(const UserUpdatedFn& fn)
        { user_updated_fns.push_back(fn); }
    void add_group_updated(const GroupUpdatedFn& fn)
        { group_updated_fns.push_back(fn); }
    void add_address_updated(const AddressUpdatedFn& fn)
        { address_updated_fns.push_back(fn); }
    void add_principal_deleted(const PrincipalDeletedFn& fn)
        { principal_deleted_fns.push_back(fn); }
    void add_members_updated(const MembersUpdatedFn& fn)
        { members_updated_fns.push_back(fn); }
    void add_groups_updated(const GroupsUpdatedFn& fn)
        { groups_updated_fns.push_back(fn); }
    void add_addr_groups_updated(const AddrGroupsUpdatedFn& fn)
        { addr_groups_updated_fns.push_back(fn); }

    struct ModifiedState {
        PrincipalSet principals;
        hash_set<int64_t> groups;
        hash_set<int64_t> processed;
    };

    void delete_principal(const Principal& principal);
    void update_reserved_id(PrincipalType ptype, int64_t id,
                            const std::string& name);
    void update_switch(int64_t id, const std::string& name,
                       int64_t dp_priority, const datapathid& dp);
    void update_location(int64_t id, const std::string& name,
                         int64_t port_priority, int64_t switch_id,
                         int64_t port, const std::string& portname);
    void update_host(int64_t id, const std::string& name);
    void update_host_netid(int64_t id, const std::string& name,
                           int64_t host_id, bool is_router, bool is_gateway);
    void update_netid_binding(int64_t addr_id, int64_t host_netid,
                              int64_t priority, const std::string& addr,
                              AddressType type, bool deleted);
    void update_user(int64_t id, const std::string& name);
    void update_group(int64_t id, const std::string& name);
    void update_address(int64_t id, const std::string& name,
                        AddressType type);
    void update_group_member(int64_t id, int64_t group,
                             const Principal& principal,
                             ModifiedState& mod_state);
    void delete_group_member(int64_t id, ModifiedState& mod_state);
    void process_modified_state(const ModifiedState& mod_state) const;

private:
    struct Membership {
        int64_t group;
        Principal principal;
    };

    typedef std::list<int64_t> Parents;
    typedef std::list<Principal> Members;
    typedef hash_map<Principal, Parents, PrincipalHash, PrincipalEq> ParentMap;
    typedef hash_map<Principal, std::string, PrincipalHash, PrincipalEq> IDMap;

    Datatypes *datatypes;

    std::list<SwitchUpdatedFn>     switch_updated_fns;
    std::list<LocationUpdatedFn>   location_updated_fns;
    std::list<HostUpdatedFn>       host_updated_fns;
    std::list<NetidUpdatedFn>      netid_updated_fns;
    std::list<NetidBindingUpdatedFn> netid_binding_updated_fns;
    std::list<UserUpdatedFn>       user_updated_fns;
    std::list<GroupUpdatedFn>      group_updated_fns;
    std::list<AddressUpdatedFn>    address_updated_fns;
    std::list<PrincipalDeletedFn>  principal_deleted_fns;
    std::list<MembersUpdatedFn>    members_updated_fns;
    std::list<GroupsUpdatedFn>     groups_updated_fns;
    std::list<AddrGroupsUpdatedFn> addr_groups_updated_fns;

    hash_map<int64_t, Membership>  memberships;  // key is ID
    hash_map<int64_t, Members>     members;      // key is GROUP_ID
    ParentMap                      parents;

    IDMap                          id_to_name;
    hash_map<int64_t, AddressType> address_types;

    hash_map<uint64_t, std::list<int64_t> >  dladdr_to_id;
    hash_map<uint32_t, std::list<int64_t> >  nwaddr_to_id;
    hash_map<int64_t, cidr_ipaddr>           id_to_cidr;

    struct ReservedNames {
        ReservedNames() : unauth(0), auth(-1), unknown(-2) { }
        int64_t unauth;
        int64_t auth;
        int64_t unknown;
    };

    std::vector<ReservedNames> reserved;

    const std::string unauthenticated;
    const std::string authenticated;
    const std::string unknown;

    int64_t get_reserved_index(PrincipalType ptype) const;

    bool remove_group_member(int64_t id, int64_t group,
                             const Principal& principal, bool deleted,
                             ModifiedState& mod_state);
    void add_members(const Principal& principal,
                     ModifiedState& mod_state) const;
    void add_id_mapping(Principal& principal, const std::string& name);
    void delete_id_mapping(const Principal& principal);
    void add_dladdr_id(int64_t id, const std::string& name);
    void add_cidr_id(int64_t id, const std::string& name);
    void remove_addr_binding(int64_t id, AddressType type,
                             const std::string name);
    void remove_dladdr_id(int64_t id, const std::string& name);
    void remove_cidr_id(int64_t id, const std::string& name);
    bool add_group(int64_t group, std::list<int64_t>& parent_list) const;
    void add_parents(const Principal& principal,
                     std::list<int64_t>& parent_list) const;
    void set_group_list(const std::list<int64_t>& parent_list,
                        GroupList& group_list) const;
};

}
}

#endif
