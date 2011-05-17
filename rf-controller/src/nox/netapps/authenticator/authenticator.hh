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
#ifndef AUTHENTICATOR_HH
#define AUTHENTICATOR_HH 1

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_array.hpp>

#include "component.hh"
#include "bindings_storage/bindings_storage.hh"
#include "data/datatypes.hh"
#include "data/datacache.hh"
#include "flow_in.hh"
#include "flow_util.hh"
#include "hash_map.hh"
#include "host_event.hh"
#include "netinet++/cidr.hh"
#include "user_event.hh"
#include "user_event_log/user_event_log.hh"

// enable to have locations added/removed based on topology i.e. so
// hosts wont have duplicate locations with one downstream from
// another.  Requires routing_module library to be loaded as
// dependency.  Often you'll want this enabled, but NOX is set up
// currently such that routing_module auto pulls in LLDP discovery
// component etc which a user may not always want running.

#define AUTH_WITH_ROUTING 1

#if AUTH_WITH_ROUTING
#include "routing/routing.hh"
#endif

namespace vigil {
namespace applications {

/** \ingroup noxcomponents
 *
 * Authenticator
 *
 */

class Authenticator
    : public container::Component {

public:
    Authenticator(const container::Context*, const json_object*);
    Authenticator() : Component(0) { }

    static void getInstance(const container::Context*, Authenticator*&);

    void configure(const container::Configuration*);
    void install();

    struct DLEntry;

    struct DLNWEntry {
        uint32_t nwaddr;
        bool authed;
        boost::shared_ptr<HostNetid> host_netid;
        boost::shared_ptr<GroupList> nwaddr_groups;  // both or just nw?
        DLEntry *dlentry;
        time_t exp_time;
    };

    struct HostNetEntry;

    struct DLBinding {
        int64_t id;
        int64_t priority;
        HostNetEntry *netid;
    };

    typedef hash_map<uint32_t, DLNWEntry> DLNWMap;

    struct DLEntry {
        ethernetaddr dladdr;
        boost::shared_ptr<GroupList> groups;
        DLNWMap dlnwentries;
        DLNWEntry *zero;
        std::list<AuthedLocation> locations;
        std::list<DLBinding> bindings;
        time_t exp_time;
    };

    struct NWBinding {
        int64_t id;
        int64_t priority;
        HostNetEntry *netid;
    };

    struct NWEntry {
        boost::shared_ptr<GroupList> groups;
        std::list<DLNWEntry*> dlnwentries;
        std::list<NWBinding> bindings;
        time_t exp_time;
    };

    struct LocEntry {
        boost::shared_ptr<Location> entry;
        std::list<DLEntry*> dlentries;
        bool active;
    };

    struct LocBinding {
        int64_t priority;
        LocEntry *location;
    };

    struct PortEntry {
        int64_t port;
        LocEntry *active_location;
        std::list<LocBinding> bindings;
    };

    struct SwitchEntry {
        boost::shared_ptr<Switch> entry;
        std::list<PortEntry> locations;
        bool active;
    };

    struct SwitchBinding {
        int64_t priority;
        SwitchEntry *sentry;
    };

    struct DPEntry {
        SwitchEntry *active_switch;
        std::list<SwitchBinding> bindings;
    };

    struct HostNetEntry {
        boost::shared_ptr<HostNetid> entry;
        std::list<DLNWEntry*> dlnwentries;
    };

    struct HostEntry {
        boost::shared_ptr<Host> entry;
        std::list<HostNetEntry*> netids;
    };

    struct UserEntry {
        boost::shared_ptr<User> entry;
        std::list<HostEntry*> hostentries;
    };

    void add_internal_subnet(const cidr_ipaddr&);
    bool remove_internal_subnet(const cidr_ipaddr&);
    void clear_internal_subnets();

    void get_names(const datapathid& dp, uint16_t inport,
                   const ethernetaddr& dlsrc, uint32_t nwsrc,
                   const ethernetaddr& dldst, uint32_t nwdst,
                   PyObject *callable);

    void reset_last_active(const datapathid& dp, uint16_t port,
                           const ethernetaddr& dladdr, uint32_t nwaddr);

    void set_default_hard_timeout(uint32_t to) { default_hard_timeout = to; }
    void set_default_idle_timeout(uint32_t to) { default_idle_timeout = to; }
    uint32_t get_default_hard_timeout() { return default_hard_timeout; }
    uint32_t get_default_idle_timeout() { return default_idle_timeout; }

    const DLEntry *get_dladdr_entry(const ethernetaddr& dladdr) const;
    const NWEntry *get_nwaddr_entry(uint32_t nwaddr) const;
    const DLNWEntry *get_dlnw_entry(const ethernetaddr& dladdr,
                                    uint32_t nwaddr) const;
    const SwitchEntry *get_switch_entry(const datapathid& dp) const;
    const LocEntry *get_location_entry(const datapathid& dp,
                                       uint16_t port) const;
    const SwitchEntry *get_switch_entry(int64_t id) const;
    const LocEntry *get_location_entry(int64_t id) const;
    const HostEntry *get_host_entry(int64_t id) const;
    const HostNetEntry *get_host_netid_entry(int64_t id) const;
    const UserEntry *get_user_entry(int64_t id) const;

    HostEntry* update_host(int64_t hostname);
    HostNetEntry* update_host_netid(int64_t host_netid, int64_t host_id,
                                    bool is_router, bool is_gateway);
    UserEntry* update_user(int64_t username);
    LocEntry* update_location(int64_t locname, int64_t priority,
                              int64_t switch_id, uint16_t port,
                              const std::string& portname);
    SwitchEntry* update_switch(int64_t switchname, int64_t priority,
                               const datapathid& dp);

    void delete_principal(const Principal& principal);
    void delete_host(int64_t hostname);
    void delete_host_netid(int64_t host_netid);
    void delete_user(int64_t username);
    void delete_location(int64_t locname, bool remove_binding = true);
    void delete_switch(int64_t switchname);

    typedef hash_map<int64_t, hash_set<int64_t> > MemberMap; // key = type,
                                                             // value = member IDs
    void all_updated(bool poison) const;
    void principal_updated(const Principal& principal, bool poison,
                           bool refetch_groups);
    void principals_updated(const PrincipalSet& principals, bool poison,
                            bool refetch_groups);
    void groups_updated(const std::list<int64_t>& groups, bool poison);
    void addr_updated(AddressType type, const std::string& addr,
                      bool poison, bool refetch_groups);

    int64_t get_port_number(const datapathid& dp,
                            const std::string& port_name) const;

    typedef boost::function<void(const boost::shared_ptr<Location>&,
                                 const DLNWEntry*, bool)> EndpointUpdatedFn;

    void add_endpoint_updated_fn(const EndpointUpdatedFn& fn);

    /** \brief Option to allow poisoning or not
     * Defaulted to true
     */
    bool poison_allowed;

private:
    typedef hash_map<uint64_t, DLEntry>      DLMap;
    typedef hash_map<uint32_t, NWEntry>      NWMap;
    typedef hash_map<uint64_t, DPEntry>      DPMap;
    typedef hash_map<int64_t, SwitchEntry>   SwitchMap;
    typedef hash_map<int64_t, LocEntry>      LocMap;
    typedef hash_map<int64_t, HostEntry>     HostMap;
    typedef hash_map<int64_t, HostNetEntry>  HostNetMap;
    typedef hash_map<int64_t, UserEntry>     UserMap;

    static const uint32_t NWADDR_TIMEOUT = 300;

    DLMap hosts_by_dladdr;
    NWMap hosts_by_nwaddr;
    DPMap switches_by_dp;

    SwitchMap switches;
    LocMap locations;
    HostMap hosts;
    HostNetMap host_netids;
    UserMap users;

    hash_map<uint64_t, SwitchEntry> dynamic_switches;
    hash_map<uint64_t, LocEntry> dynamic_locations;

    struct Queued_loc {
        datapathid dp;
        uint16_t port;
        std::string portname;
    };

    std::list<Queued_loc> queued_locs;

    bool routing;
    bool auto_auth;
    uint32_t expire_timer;

    boost::shared_array<uint8_t> raw_of;
    ofp_flow_mod *ofm;

    std::vector<cidr_ipaddr> internal_subnets;

    Datatypes *datatypes;
    Data_cache *data_cache;
    Bindings_Storage *bindings;
    User_Event_Log *user_log;
    std::list<EndpointUpdatedFn> updated_fns;
    uint32_t default_hard_timeout;
    uint32_t default_idle_timeout;
    char buf[1024];
#if AUTH_WITH_ROUTING
    Routing_module *routing_mod;
#endif

    const std::string& get_switch_name(int64_t id);
    const std::string& get_location_name(int64_t id);
    const std::string& get_host_name(int64_t id);
    const std::string& get_netid_name(int64_t id);
    const std::string& get_user_name(int64_t id);
    const std::string& get_group_name(int64_t id);

    DLEntry *get_dladdr_entry(const ethernetaddr& dladdr, const time_t& exp_time,
                              bool create=false);
    NWEntry *get_nwaddr_entry(uint32_t nwaddr, const time_t& exp_time,
                              bool create);
    DLNWEntry *get_dlnw_entry(const ethernetaddr& dladdr, uint32_t nwaddr,
                              const time_t& exp_time, bool create);
    DLNWEntry *get_dlnw_entry(DLEntry *dlentry, uint32_t nwaddr,
                              const time_t& exp_time, bool create);

    struct ptrhash {
        std::size_t operator()(const void *ptr) const;
    };

    struct ptreq {
        bool operator()(const void *a, const void *b) const;
    };

    struct sptrhash {
        std::size_t operator()(const boost::shared_ptr<Location>& ptr) const;
    };

    struct sptreq {
        bool operator()(const boost::shared_ptr<Location>& a,
                        const boost::shared_ptr<Location>& b) const;
    };

    typedef hash_set<const DLNWEntry*, ptrhash, ptreq> DLNWHash;
    typedef hash_map<boost::shared_ptr<Location>,
                     DLNWHash, sptrhash, sptreq> EndpointValue;
    typedef hash_map<const DLEntry*, EndpointValue,
                     ptrhash, ptreq> Endpoints;

    void group_updated(int64_t group, Endpoints& endpoints);
    void add_updated_principal(const Principal& principal,
                               Endpoints& endpoints, bool refetch_groups);
    void add_updated_cidr(const cidr_ipaddr& cidr, Endpoints& endpoints,
                          bool refetch_groups);
    void add_updated_entry(const SwitchEntry *entry, Endpoints& endpoints) const;
    void add_updated_entry(const LocEntry *entry, Endpoints& endpoints) const;
    void add_updated_entry(const HostEntry *entry, Endpoints& endpoints) const;
    void add_updated_entry(const HostNetEntry *entry, Endpoints& endpoints) const;
    void add_updated_entry(const UserEntry *entry, Endpoints& endpoints) const;
    void add_updated_entry(const DLEntry *entry, Endpoints& endpoints) const;
    void add_updated_entry(const NWEntry *entry, Endpoints& endpoints) const;
    void add_updated_entry(const DLNWEntry *entry, Endpoints& endpoints) const;
    LocEntry *get_unauth_location();
    HostNetEntry *get_unauth_netid();
    HostNetEntry *get_auth_netid();
    bool auth_flow_host(const Flow_in_event& fi, bool auth_default);
    const HostNetEntry *get_names_from_bindings(const DLEntry *dlentry,
                                          const NWEntry *nwentry) const;
    bool get_location(const datapathid &dp, uint16_t port,
                      std::list<AuthedLocation> &als,
                      std::list<AuthedLocation>::iterator &al) const;
    bool get_on_path_location(const datapathid& dp, uint16_t port,
                              std::list<AuthedLocation>& als,
                              std::list<AuthedLocation>::iterator &al) const;
    bool is_internal_ip(uint32_t nwaddr) const;
    void expire_entities();
    void expire_host_locations(const timeval& curtime);
    void expire_hosts(const timeval& curtime);
    void poison_port(const datapathid& dp, uint16_t port) const;
    void poison_location(const datapathid& dp, const ethernetaddr& dladdr,
                         uint32_t nwaddr, bool wildcard_nw) const;
    void endpoints_updated(const Endpoints& endpoints, bool poison) const;
    void call_updated_fns(const boost::shared_ptr<Location>& lentry,
                          const DLNWEntry *nwentry, bool poisoned) const;
    void call_updated_fns(const boost::shared_ptr<Location>& lentry,
                          const DLEntry *dlentry, bool poisoned) const;
    bool is_internal_mac(uint64_t hb_dladdr) const;

    /* authenticator_modify.cc */

    bool add_host(const Host_auth_event& ha);
    bool add_host2(const Host_auth_event& ha, LocEntry *lentry,
                   HostNetEntry *netid, HostEntry *host,
                   DLNWEntry *dlnwentry, const timeval& curtime);
    bool auth_zero(const Host_auth_event& ha, LocEntry *location,
                   HostNetEntry *netid, HostEntry *host,
                   DLNWEntry *dlnwentry, const timeval& curtime);
    void add_host_location(const Host_auth_event& ha, LocEntry *lentry,
                           HostEntry *host, DLEntry *dlentry,
                           const timeval& curtime);
    void remove_dl_location(DLEntry *dlentry,
                            std::list<AuthedLocation>::iterator& authed,
                            Host_event::Reason reason, bool poison);
    void remove_dl_location(DLEntry *dlentry, const datapathid& dp, uint16_t port,
                            bool ignore_port, Host_event::Reason reason, bool poison);
    void remove_dlnw_host(DLNWEntry *dlnwentry, Host_event::Reason reason,
                          bool poison);
    bool remove_dlnw_location(DLNWEntry *dlnwentry, const datapathid& dp,
                              uint16_t port, bool ignore_port,
                              Host_event::Reason reason);
    void remove_host(HostEntry *hentry, const datapathid& dp,
                     uint16_t port, bool ignore_port,
                     Host_event::Reason reason);
    void remove_host_netid(HostNetEntry *netid, const datapathid& dp,
                             uint16_t port, bool ignore_port,
                             Host_event::Reason reason);
    void remove_location_hosts(LocEntry *lentry, Host_event::Reason reason, bool poison);
    void remove_location_hosts(const datapathid& dp, uint16_t port,
                               bool ignore_port, Host_event::Reason reason,
                               bool poison);
    void remove_host_by_event(const Host_auth_event& ha);
    bool remove_host_by_event(const Host_auth_event& ha, DLNWEntry *dlnwentry);
    bool add_user(const User_auth_event& ua);
    void remove_unauth_user(HostEntry *hentry);
    void remove_user(UserEntry *user, HostEntry *host,
                     User_event::Reason reason, bool add_unauth);
    void remove_user(int64_t username, int64_t hostname,
                     User_event::Reason reason);
    DLEntry* new_dladdr(const ethernetaddr& dladdr, const time_t& exp_time);
    NWEntry* new_nwaddr(uint32_t nwaddr, const time_t& exp_time);
    DLNWEntry* new_dlnw_entry(DLEntry *dlentry, uint32_t nwaddr,
                              const time_t& exp_time);
    bool add_netid_binding(HostNetEntry *netid, int64_t host_id);
    void update_netid_binding(int64_t addr_id, int64_t host_netid,
                              int64_t priority, const std::string& addr,
                              AddressType addr_type, bool deleted);
    void add_dladdr_binding(HostNetEntry *netid, int64_t addr_id,
                            int64_t priority, const ethernetaddr& dladdr);
    void add_nwaddr_binding(HostNetEntry *netid, int64_t addr_id,
                            int64_t priority, const ipaddr& ip);
    void delete_dladdr_binding(HostNetEntry *netid, int64_t addr_id,
                               const ethernetaddr& dladdr,
                               Host_event::Reason reason);
    void delete_nwaddr_binding(HostNetEntry *netid, int64_t addr_id,
                               const ipaddr& ip,
                               Host_event::Reason reason);
    void delete_netid_host_binding(HostNetEntry *netid,
                                   Host_event::Reason reason);
    void delete_host_netid(HostNetEntry *netid);
    bool add_location_binding(LocEntry *lentry, int64_t priority,
                              int64_t switch_id, uint16_t port);
    void delete_location_binding(LocEntry *lentry);
    LocEntry* new_dynamic_location(SwitchEntry *sentry, uint16_t port);
    void activate_location(const datapathid& dp, uint16_t port,
                           const std::string& portname);
    void deactivate_location(const datapathid& dp, uint16_t port,
                             bool poison);
    void deactivate_location(const datapathid& dp, uint16_t port,
                             PortEntry& pentry, bool poison);
    void add_switch_binding(SwitchEntry *sentry, int64_t priority,
                            const datapathid& dp);
    void delete_switch_binding(SwitchEntry *sentry);
    SwitchEntry* new_dynamic_switch(const datapathid& dp);
    void activate_switch(const datapathid& dp);
    void deactivate_switch(const datapathid& dp, bool poison);
    void deactivate_switch(DPEntry& dentry, bool poison);
    void new_location_config(const datapathid& dp, uint16_t port,
                             const std::string& portname, bool is_redo);
    void queue_new_location(const datapathid& dp, uint16_t port,
                            const std::string& portname);
    void process_queue();
    void remove_queued_loc(const datapathid& dp, uint16_t port);
    void mark_as_virtual(LocEntry* location);
    LocEntry* get_active_location(const datapathid& dp, uint16_t port) const;

    Disposition handle_bootstrap(const Event& event);
    Disposition handle_datapath_join(const Event& event);
    Disposition handle_datapath_leave(const Event& event);
    Disposition handle_port_status(const Event& event);
    Disposition handle_link_change(const Event& event);
    Disposition handle_host_auth(const Event& event);
    Disposition handle_user_auth(const Event& event);
    Disposition handle_packet_in(const Event& event);

    bool set_flow_in(Flow_in_event &fi, bool packet_in);
    bool set_flow_src(Flow_in_event& fi, bool packet_in);
    void set_flow_dst(Flow_in_event &fi);
    DLNWEntry *cache_host(DLEntry *dlentry,
                          const NWEntry *nwentry, uint32_t nwaddr,
                          const time_t& exp_time);
    void make_primary(const time_t& curtime, DLEntry *dlentry,
                      std::list<AuthedLocation>::iterator& location);
    bool set_source_route_host(NWEntry *nwentry, Flow_in_event& fi);
    bool set_dest_route_host(NWEntry *nwentry, Flow_in_event& fi);
    void set_destinations(const std::list<AuthedLocation>& als,
                          Flow_in_event& fi);
    void set_empty_destinations(Flow_in_event& fi);
    void set_route_destinations(const std::list<AuthedLocation>& als,
                                Flow_in_event& fi);

#ifdef TWISTED_ENABLED
    PyObject* namelist_to_python(const GroupList& ids);
    PyObject* name_to_python(uint32_t id, PrincipalType t);
    PyObject* users_to_python(const std::list<AuthedUser>& users);
#endif
};

} // namespace applications
} // namespace vigil

#endif // AUTHENTICATOR_HH
