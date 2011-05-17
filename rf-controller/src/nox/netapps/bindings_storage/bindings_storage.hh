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
#ifndef BINDINGS_STORAGE_HH
#define BINDINGS_STORAGE_HH 1

#include <list>
#include <string>

#include "component.hh"
#include "data/datatypes.hh"
#include "data/datacache.hh"
#include "netinet++/ethernetaddr.hh"
#include "serial_op_queue.hh"
#include "storage/storage.hh"
#include "storage/storage_util.hh"

#ifdef TWISTED_ENABLED

#include <Python.h>
#include "pyrt/pyglue.hh"

#endif

namespace vigil {
namespace applications {

struct nwhost {
    int64_t host;
    std::list<uint32_t> nwaddrs;
};

struct Loc {
    enum Port { NO_PORT = -1 };
    Loc() { }
    Loc(const datapathid &d, uint16_t p) : dpid(d), port(p) {}
    datapathid dpid;
    uint16_t port;
    bool operator==(const Loc &o) const {
        return (dpid == o.dpid && port == o.port);
    }
    // just for sorting to eliminate duplicate
    bool operator<(const Loc &o) const {
        uint64_t did = dpid.as_host();
        uint64_t oid = o.dpid.as_host();
        return (did == oid) ? (port < o.port) : (did < oid) ;
    }
};

// the NetEntity data structure represents a single "network entity",
// which is a (dpid,port,dladdr,nwaddr) tuple.
struct NetEntity {
    NetEntity() : port(0), dladdr((uint64_t)0), nwaddr(0) {
        dpid = datapathid::from_host(0);
    }
    NetEntity(const datapathid &d, uint16_t p,
              const ethernetaddr &m, uint32_t i)
        : dpid(d), port(p), dladdr(m), nwaddr(i) {}

    datapathid dpid;
    uint16_t port;
    ethernetaddr dladdr;
    uint32_t nwaddr;

    bool operator==(const NetEntity &other) const {
        return (dpid == other.dpid && port == other.port
                && dladdr == other.dladdr && nwaddr == other.nwaddr);
    }
};


struct Link {
    Link(const datapathid &d1, uint16_t p1, const datapathid &d2, uint16_t p2)
        : dpid1(d1), dpid2(d2), port1(p1), port2(p2) {}

    datapathid dpid1;
    datapathid dpid2;
    uint16_t port1;
    uint16_t port2;

    bool operator==(const Link &o) const {
        return (dpid1 == o.dpid1 && port1 == o.port1
                && dpid2 == o.dpid2 && port2 == o.port2);
    }
    bool matches(const datapathid & _dpid) {
        return dpid1 == _dpid || dpid2 == _dpid;
    }
    bool matches(const datapathid & _dpid, uint16_t _port) {
        return ((dpid1 == _dpid && port1 == _port)
                || (dpid2 == _dpid && port2 == _port));
    }

    void fillQuery(storage::Query &q) {
        q["dpid1"] = (int64_t) dpid1.as_host();
        q["port1"] = (int64_t) port1;
        q["dpid2"] = (int64_t) dpid2.as_host();
        q["port2"] = (int64_t) port2;
    }
};

struct Name {
    enum Type { NONE = 0, LOCATION, HOST, USER, SWITCH, PORT, LOC_TUPLE,
                LOCATION_GROUP, HOST_GROUP, USER_GROUP, SWITCH_GROUP };
    Name(const std::string &n, Type t, int64_t i=-1) : name(n), name_type(t), id(i) {}

    std::string name;
    Type name_type;
    int64_t id;

    bool operator==(const Name &o) const {
        return (name == o.name && name_type == o.name_type && id == o.id);
    }
    // just for sorting to eliminate duplicates
    bool operator<(const Name &o) const {
        return (id == o.id) ? (name_type < o.name_type) : (id < o.id);
    }
};

typedef std::list<Name> NameList;
typedef std::list<NetEntity> EntityList;
typedef std::list<Loc> LocationList;
typedef boost::function<void(NameList&)> Get_names_callback;
typedef boost::function<void(EntityList&)> Get_entities_callback;
typedef boost::function<void()> Clear_callback;
typedef boost::function<void(std::list<Link>&)> Get_links_callback;
typedef boost::function<void(LocationList&)> Get_locations_callback;

// Internal State Machine helpers

// Any operation that modifies the contents of a table should occur
// serially, so they should use Serial_Ops.

enum GetBindingsState { GET_LOCATIONS, GET_DLADDRS_BY_LOC, GET_DLADDRS_BY_HOST,
                        GET_HOSTS_BY_DLADDR, GET_HOSTS_BY_USER, GET_USERS, DONE };

struct Get_Bindings_Op;
typedef boost::shared_ptr<Get_Bindings_Op> Get_Bindings_Op_ptr;
typedef boost::function<void(const Get_Bindings_Op_ptr&)> Get_bindings_callback;

struct Get_Bindings_Op {
    Get_Bindings_Op(const Get_bindings_callback& cb,
                    GetBindingsState cur_state_, bool loc_tuples_)
        : callback(cb), cur_state(cur_state_), loc_tuples(loc_tuples_),
          index(0) {}
    Get_Bindings_Op(const Get_bindings_callback& cb,
                    GetBindingsState cur_state_, bool loc_tuples_,
                    const storage::Query& filter_)
        : callback(cb), cur_state(cur_state_), loc_tuples(loc_tuples_),
          filter(filter_), index(0) {}

    hash_map<int64_t, std::list<int64_t> > host_users;
    hash_map<uint64_t, std::list<nwhost> > dladdr_nwhosts;
    hash_map<uint64_t, std::list<int64_t> > dladdr_locations;
    hash_map<int64_t, Loc> location_info;

    Get_bindings_callback callback;
    GetBindingsState cur_state;
    bool loc_tuples;
    storage::Query filter;
    uint32_t index;
    NameList names;
};

struct Get_All_Names_Op {
    Get_All_Names_Op(Get_names_callback cb, Name::Type type)
        : callback(cb), name_type(type), check_type(false) {}

    Get_names_callback callback;
    Name::Type name_type;
    PrincipalType principal_type;
    bool check_type;
    std::string attr;
    NameList names_found; // names found so far
};

typedef boost::shared_ptr<Get_All_Names_Op> Get_All_Names_Op_ptr;


enum GetLinksState { GL_FETCH_ALL, GL_FILTER_AND_CALLBACK, GL_NONE };
enum GetLinksType { GL_ALL, GL_DP , GL_DP_Port };

struct Get_Links_Op {
    Get_Links_Op (GetLinksType t, const Get_links_callback &cb)
    : cur_state(GL_FETCH_ALL), callback(cb), type(t) {}

    std::list<Link> links;
    GetLinksState cur_state;
    Get_links_callback callback;
    GetLinksType type;
    datapathid filter_dpid;
    uint16_t filter_port;
};

typedef boost::shared_ptr<Get_Links_Op> Get_Links_Op_ptr;

struct Get_LocNames_Op {
    Get_LocNames_Op (const Get_names_callback &cb, const storage::Query &q,
                     Name::Type t) : callback(cb), query(q), type(t) {}
    NameList loc_names;
    Get_names_callback callback;
    storage::Query query; // query specifying the dpid and port
    Name::Type type; // type of location being looked for
};

typedef boost::shared_ptr<Get_LocNames_Op> Get_LocNames_Op_ptr;


struct Get_Loc_By_Name_Op {
    Get_Loc_By_Name_Op (const Get_locations_callback &cb) : callback(cb) {}
    LocationList locations;
    Get_locations_callback callback;
};

typedef boost::shared_ptr<Get_Loc_By_Name_Op> Get_Loc_By_Name_Op_ptr;


/** \ingroup noxcomponents
 *
 *
 * Bindings_Storage mirrors the Authenticator's internal data structures
 * within the NDB.  This serves two goals:  1) this data can be archived to
 * reconstruct network state at a later point in time, and 2) other
 * components can query the current binding state via Bindings_Storage
 * interfaces.
 *
 * To manage names, the component has NDB tables:<br>
 * <br>
 * | host (int), user (int) |<br>
 * | host (int), dladdr (int), nwaddr (int) |<br>
 * | dladdr (int), location (int) |<br>
 * <br>
 * All operations that add or remove name binding state are serialized by
 * Bindings_Storage, so that no such operations run in parallel, though
 * this serialization is hidden from the caller.  Thus, if two subsequent
 * calls both add binding state, the modifications to NDB state from the
 * first complete before the NDB modifications specified by the second
 * call begin.  Likewise, if the component receives an add and then a remove,
 * it will complete all NDB operations for the add before processing the
 * remove.  Calls to lookup NDB state are not serialized, so if an add is
 * followed quickly by a get, the get result may not include data from the
 * add.
 *
 *
 * TODO: document the bindings_location and bindings_link tables.
 */


class Bindings_Storage
    : public container::Component
{
public:

    // NDB table names
    static const std::string HOST_TABLE_NAME;
    static const std::string USER_TABLE_NAME;
    static const std::string DLADDR_TABLE_NAME;
    static const std::string LINK_TABLE_NAME;
    static const std::string LOCATION_TABLE_NAME;

    Bindings_Storage(const container::Context* c,const json_object*);

    void configure(const container::Configuration*);
    void install();

    static void getInstance(const container::Context* ctxt,
                            Bindings_Storage*& h);

    // does a lookup for all names of a particular type (e.g., USER,
    // HOST, LOCATION, SWITCH).  This represents all names that have
    // currently active bindings.  Obviously, if you are on a large
    // network, the callback may be called with a very large list, so
    // use this sparingly.
    void get_all_names(Name::Type name_type, const Get_names_callback &cb);

    void store_location_binding(const ethernetaddr& dladdr, int64_t location);
    void store_host_binding(int64_t host, const ethernetaddr& dladdr,
                            uint32_t nwaddr);
    void store_user_binding(int64_t user, int64_t host);

    void remove_location_binding(const ethernetaddr& dladdr, int64_t location);
    void remove_host_binding(int64_t host, const ethernetaddr& dladdr,
                             uint32_t nwaddr);
    void remove_user_binding(int64_t user, int64_t host);

     // does a lookup based on a network identifier and finds all USER
     // or HOST names associated with it.  Because this call hits the NDB,
     // results are returned via a callback
    void get_names_by_ap(const datapathid &dpid, uint16_t port,
                         const Get_names_callback &cb);
    void get_names_by_mac(const ethernetaddr &mac,
                          const Get_names_callback &cb);
    void get_names_by_ip(uint32_t ip, const Get_names_callback &cb);
    void get_names(const datapathid &dpid, uint16_t port,
                   const ethernetaddr &mac, uint32_t ip,
                   const Get_names_callback &cb);

    // similar to above calls, but generic query
    void get_names(const storage::Query &query,
                   bool loc_tuples, const Get_names_callback &cb);

    void get_host_users(int64_t hostname,
                        const Get_names_callback& cb);
    void get_user_hosts(int64_t username,
                        const Get_names_callback& cb);

    // does a lookup based on a principal name and calls back
    // with a list of NetEntity Objects
    // Valid principal types are (USER, HOST, LOCATION, PORT).
    void get_entities_by_name(int64_t name, Name::Type name_type,
                              const Get_entities_callback &ge_cb);

    // removes all host name binding state stored by the component
    void clear_bindings();

    // Link functions: the following functions deal only with
    // bindings, which do not actually have names

    void add_link(const datapathid &dpid1, uint16_t port1,
                  const datapathid &dpid2, uint16_t port2);

    void remove_link(const datapathid &dpid1, uint16_t port1,
                     const datapathid &dpid2, uint16_t port2);

    void get_all_links(const Get_links_callback &cb);
    void get_links(const datapathid dpid, const Get_links_callback &cb);
    void get_links(const datapathid dpid, uint16_t port,
                   const Get_links_callback &cb);

    // removes all link binding state stored by the component
    void clear_links();

    // location/switch functions

    // add a switch/location as indicated by name_type
    // If name_type is Name::SWITCH, the port field is ignored
    void add_name_for_location(const datapathid &dpid, uint16_t port,
                               int64_t name, Name::Type name_type);

    // remove a switch/location as indicated by name_type
    // if name == 0, removes all names for this switch/location.
    // If name_type is Name::SWITCH, the port field is ignored
    void remove_name_for_location(const datapathid &dpid, uint16_t port,
                                  int64_t name, Name::Type name_type);

    // find all names of type 'name_type' associated this this
    // 'dpid' and 'port' (if name_type is Name::SWITCH, port
    // is ignored).  Valid types include SWITCH, LOCATION, LOC_TUPLE, PORT.
    void get_names_for_location(const datapathid &dpid, uint16_t port,
                                Name::Type name_type,
                                const Get_names_callback &cb);
    void get_names_for_location(storage::Query &q,
                                const Get_names_callback &cb, Name::Type type);

    // returns a list of location objects that are associated with
    // a particular name.  If name_type is Name::SWITCH only
    // the dpid of the location object will be valid.
    void get_location_by_name(int64_t locationname, Name::Type name_type,
                              const Get_locations_callback &cb);

    // utility function
    static void print_names(const NameList &name_list);
    static inline void str_replace(std::string &str, const std::string &target,
                                   const std::string &replace);

private:
    storage::Async_storage *np_store;
    Datatypes *datatypes;
    Data_cache *data_cache;

    Serial_Op_Queue host_serial_queue;
    Serial_Op_Queue user_serial_queue;
    Serial_Op_Queue dladdr_serial_queue;
    Serial_Op_Queue link_serial_queue;
    Serial_Op_Queue location_serial_queue;

    // functions related to creating the table
    void create_tables();

    // modify table cb
    void finish_put(const storage::Result& result, const storage::GUID& guid,
                    Serial_Op_Queue *serial_queue);
    void finish_remove(const storage::Result& result,
                       Serial_Op_Queue *serial_queue);
    void clear_table(const std::string& table_name,
                     Serial_Op_Queue *serial_queue);

    void get_all_names_cb(const storage::Result& result,
                          const storage::Context& ctx, const storage::Row& row,
                          const Get_All_Names_Op_ptr& info);
    void return_loctuples(NameList& names,
                          const Get_Bindings_Op_ptr& bindings,
                          const Get_names_callback& cb);
    void return_host_users(const Get_Bindings_Op_ptr& bindings,
                           bool ret_users, const Get_names_callback& cb);
    void return_names(const Get_Bindings_Op_ptr& bindings,
                      const Get_names_callback& cb);
    void return_entities(const Get_Bindings_Op_ptr& bindings,
                         const Get_entities_callback& cb);
    void get_names_by_ap2(std::list<Loc>& locations,
                          bool loc_tuples,
                          const storage::Query& q,
                          const Get_bindings_callback& cb);
    void get_names_by_ap3(const NameList& locnames,
                          std::list<Loc>& locations,
                          const Get_Bindings_Op_ptr& info);
    void get_locations_cb(const storage::Result& result,
                          const storage::Context& ctx,
                          const storage::Row& row,
                          const Get_Bindings_Op_ptr& info);
    void get_dladdrs_cb(const storage::Result& result,
                        const storage::Context& ctx,
                        const storage::Row& row,
                        const Get_Bindings_Op_ptr& info);
    void get_hosts_cb(const storage::Result& result,
                      const storage::Context& ctx,
                      const storage::Row& row,
                      const Get_Bindings_Op_ptr& info);
    void get_users_cb(const storage::Result& result,
                      const storage::Context& ctx,
                      const storage::Row& row,
                      const Get_Bindings_Op_ptr& info);
    void run_get_bindings_fsm(const Get_Bindings_Op_ptr& info);

    // functions related to adding links
    void add_link_cb1(const std::list<Link> links, Link& to_add);

    // functions related to lookup of links
    void run_get_links_fsm(const Get_Links_Op_ptr& op,
                           GetLinksState next_state = GL_NONE);
    void filter_link_list(const Get_Links_Op_ptr& op);
    void get_links_cb(const storage::Result &result,
                      const storage::Context & ctx,
                      const storage::Row &row,
                      const Get_Links_Op_ptr& op);

    // private functions for location/switch bindings

    void get_locnames_cb(const storage::Result &result,
                         const storage::Context & ctx,
                         const storage::Row &row,
                         const Get_LocNames_Op_ptr& op);
    void get_locnames_cb2(const NameList &names,
                          const Get_LocNames_Op_ptr& op);
    void get_locnames_cb3(const NameList &names,
                          const Get_LocNames_Op_ptr& op,
                          std::string& switch_name);
    void get_loc_by_name_cb(const storage::Result &result,
                            const storage::Context & ctx,
                            const storage::Row &row,
                            const Get_Loc_By_Name_Op_ptr& op);
};


#ifdef TWISTED_ENABLED

// including these python functions in the main binding_storage.hh
// because they are needed by the python proxy classes of other
// components as well (e.g., user_event_log) that will not necessariliy
// include pybindings_storage as a dependancy.

#endif

}
}

namespace vigil {

#ifdef TWISTED_ENABLED

// TODO:  apparently to_python<datapathid> returns a Long,
// which is lame.  It would be nice to fix that, or perhaps
// just swig out NetEntity, Link, and Location

// NOTE: this declaration must be in namespace vigil
template <>
inline
PyObject *to_python(const NetEntity &entity) {

    PyObject* main_tuple = PyTuple_New(4);
    PyTuple_SetItem(main_tuple,0,to_python(entity.dpid));
    PyTuple_SetItem(main_tuple,1,to_python(entity.port));
    PyTuple_SetItem(main_tuple,2,to_python(entity.dladdr));
    PyTuple_SetItem(main_tuple,3,to_python(entity.nwaddr));
    return main_tuple;
}

template <>
inline
PyObject *to_python(const Link &link) {

    PyObject* main_tuple = PyTuple_New(4);
    PyTuple_SetItem(main_tuple,0,to_python(link.dpid1));
    PyTuple_SetItem(main_tuple,1,to_python(link.port1));
    PyTuple_SetItem(main_tuple,2,to_python(link.dpid2));
    PyTuple_SetItem(main_tuple,3,to_python(link.port2));
    return main_tuple;
}

template <>
inline
PyObject *to_python(const Loc &loc) {
    PyObject* main_tuple = PyTuple_New(2);
    PyTuple_SetItem(main_tuple,0,to_python(loc.dpid));
    PyTuple_SetItem(main_tuple,1,to_python(loc.port));
    return main_tuple;
}


template <>
inline
PyObject *to_python(const Name &n) {

    PyObject* main_tuple = PyTuple_New(3);
    PyTuple_SetItem(main_tuple,0,to_python(n.name));
    PyTuple_SetItem(main_tuple,1,to_python((int)n.name_type));
    PyTuple_SetItem(main_tuple,2,to_python((int)n.id));
    return main_tuple;
}


#endif

}// end vigil


#endif
