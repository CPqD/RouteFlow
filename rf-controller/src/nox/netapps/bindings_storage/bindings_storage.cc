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
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <sstream>

#include "assert.hh"
#include "bindings_storage.hh"
#include "storage/storage-blocking.hh"
#include "vlog.hh"

namespace vigil {
namespace applications {

static Vlog_module lg("bindings_storage");

const std::string Bindings_Storage::HOST_TABLE_NAME = "bindings_host";
const std::string Bindings_Storage::USER_TABLE_NAME = "bindings_user";
const std::string Bindings_Storage::DLADDR_TABLE_NAME = "bindings_dladdr";
const std::string Bindings_Storage::LINK_TABLE_NAME = "bindings_link";
const std::string Bindings_Storage::LOCATION_TABLE_NAME = "bindings_location";

Bindings_Storage::Bindings_Storage(const container::Context* c,
                                   const json_object*)
    : Component(c), np_store(0), datatypes(0), data_cache(0),
      host_serial_queue(this,"Host Queue",lg),
      user_serial_queue(this,"User Queue",lg),
      dladdr_serial_queue(this,"Dladdr Queue",lg),
      link_serial_queue(this, "Link Queue", lg),
      location_serial_queue(this, "Location Queue", lg)
{}

void
Bindings_Storage::configure(const container::Configuration*)
{
    resolve(np_store);
    resolve(datatypes);
    resolve(data_cache);
}

void
Bindings_Storage::install()
{
    create_tables();
}

void
Bindings_Storage::create_tables()
{
    // this is called from install, so use blocking
    // storage calls to create the table
    storage::Sync_storage sync_store(np_store);

    storage::Column_definition_map host_table_def;
    host_table_def["host"] = (int64_t)0;
    host_table_def["nwaddr"] = (int64_t)0;
    host_table_def["dladdr"] = (int64_t)0;

    storage::Index_list host_table_indices;

    storage::Index all_host_fields;
    all_host_fields.columns.push_back("host");
    all_host_fields.columns.push_back("nwaddr");
    all_host_fields.columns.push_back("dladdr");
    all_host_fields.name = "all_host_fields";
    host_table_indices.push_back(all_host_fields);

    storage::Index host_only;
    host_only.columns.push_back("host");
    host_only.name = "host_only";
    host_table_indices.push_back(host_only);

    storage::Index dladdr_only;
    dladdr_only.columns.push_back("dladdr");
    dladdr_only.name = "dladdr_only";
    host_table_indices.push_back(dladdr_only);

    storage::Index nwaddr_only;
    nwaddr_only.columns.push_back("nwaddr");
    nwaddr_only.name = "nwaddr_only";
    host_table_indices.push_back(nwaddr_only);

    storage::Result result =
        sync_store.create_table(HOST_TABLE_NAME, host_table_def,
                                host_table_indices);
    if (result.code != storage::Result::SUCCESS) {
        lg.err("create table '%s' failed: %s \n", HOST_TABLE_NAME.c_str(),
               result.message.c_str());
        return;
    }

    storage::Column_definition_map user_table_def;
    user_table_def["user"] = (int64_t)0;
    user_table_def["host"] = (int64_t)0;

    storage::Index_list user_table_indices;

    storage::Index user_and_host;
    user_and_host.columns.push_back("user");
    user_and_host.columns.push_back("host");
    user_and_host.name = "user_and_host";
    user_table_indices.push_back(user_and_host);

    storage::Index user_only;
    user_only.columns.push_back("user");
    user_only.name = "user_only";
    user_table_indices.push_back(user_only);

    user_table_indices.push_back(host_only);

    result = sync_store.create_table(USER_TABLE_NAME, user_table_def,
                                     user_table_indices);
    if (result.code != storage::Result::SUCCESS) {
        lg.err("create table '%s' failed: %s \n", USER_TABLE_NAME.c_str(),
               result.message.c_str());
        return;
    }

    storage::Column_definition_map dladdr_table_def;
    dladdr_table_def["dladdr"] = (int64_t)0;
    dladdr_table_def["location"] = (int64_t)0;

    storage::Index_list dladdr_table_indices;

    storage::Index dladdr_and_loc;
    dladdr_and_loc.columns.push_back("dladdr");
    dladdr_and_loc.columns.push_back("location");
    dladdr_and_loc.name = "dladdr_and_loc";
    dladdr_table_indices.push_back(dladdr_and_loc);

    dladdr_table_indices.push_back(dladdr_only);

    storage::Index location_only;
    location_only.columns.push_back("location");
    location_only.name = "location_only";
    dladdr_table_indices.push_back(location_only);

    result = sync_store.create_table(DLADDR_TABLE_NAME, dladdr_table_def,
                                     dladdr_table_indices);
    if (result.code != storage::Result::SUCCESS) {
        lg.err("create table '%s' failed: %s \n", DLADDR_TABLE_NAME.c_str(),
               result.message.c_str());
        return;
    }

    storage::Column_definition_map link_table_def;
    link_table_def["dpid1"] = (int64_t)0;
    link_table_def["port1"] = (int64_t)0;
    link_table_def["dpid2"] = (int64_t)0;
    link_table_def["port2"] = (int64_t)0;

    // only use a single index, for deletion
    // for search, we iterate through all rows
    storage::Index_list link_table_indices;
    storage::Index all_link_fields;
    all_link_fields.name = "all_link_fields";
    all_link_fields.columns.push_back("dpid1");
    all_link_fields.columns.push_back("port1");
    all_link_fields.columns.push_back("dpid2");
    all_link_fields.columns.push_back("port2");
    link_table_indices.push_back(all_link_fields);

    result = sync_store.create_table(LINK_TABLE_NAME, link_table_def,
                                     link_table_indices);
    if (result.code != storage::Result::SUCCESS) {
        lg.err("create table '%s' failed: %s \n", LINK_TABLE_NAME.c_str(),
               result.message.c_str());
        return;
    }

    storage::Column_definition_map location_table_def;
    location_table_def["dpid"] = (int64_t)0;
    location_table_def["port"] = (int64_t)0;
    location_table_def["name"] = (int64_t)0;
    location_table_def["name_type"] = (int64_t)0;

    // only use a single index, for deletion
    // for search, we iterate through all rows
    storage::Index_list location_table_indices;

    storage::Index no_name_fields; // for delete-no-name, lookup
    no_name_fields.name = "no_name_fields";
    no_name_fields.columns.push_back("name_type");
    no_name_fields.columns.push_back("dpid");
    no_name_fields.columns.push_back("port");
    location_table_indices.push_back(no_name_fields);

    storage::Index loc_dpid_only; // for delete-all
    loc_dpid_only.name = "loc_dpid_only";
    loc_dpid_only.columns.push_back("dpid");
    location_table_indices.push_back(loc_dpid_only);

    storage::Index loc_name_fields;
    loc_name_fields.name = "loc_name_fields";
    loc_name_fields.columns.push_back("name");
    loc_name_fields.columns.push_back("name_type");
    location_table_indices.push_back(loc_name_fields);

    storage::Index loc_net_fields;
    loc_net_fields.name = "loc_net_fields";
    loc_net_fields.columns.push_back("dpid");
    loc_net_fields.columns.push_back("port");
    location_table_indices.push_back(loc_net_fields);

    storage::Index all_loc_fields; // for delete-single
    all_loc_fields.name = "all_loc_fields";
    all_loc_fields.columns.push_back("name");
    all_loc_fields.columns.push_back("name_type");
    all_loc_fields.columns.push_back("dpid");
    all_loc_fields.columns.push_back("port");
    location_table_indices.push_back(all_loc_fields);

    result = sync_store.create_table(LOCATION_TABLE_NAME, location_table_def,
                                     location_table_indices);
    if (result.code != storage::Result::SUCCESS) {
        lg.err("create table '%s' failed: %s \n", LOCATION_TABLE_NAME.c_str(),
               result.message.c_str());
        return;
    }
}

void
Bindings_Storage::store_location_binding(const ethernetaddr& dladdr,
                                         int64_t location)
{
    storage::Query q;
    q["dladdr"] = (int64_t)(dladdr.hb_long());
    q["location"] = (int64_t) location;
    storage::Async_storage::Put_callback pcb =
        boost::bind(&Bindings_Storage::finish_put, this, _1, _2,
                    &dladdr_serial_queue);
    Serial_Op_fn fn = boost::bind(&storage::Async_storage::put, np_store,
                                  DLADDR_TABLE_NAME, q, pcb);
    dladdr_serial_queue.add_serial_op(fn);
}

void
Bindings_Storage::store_host_binding(int64_t host,
                                     const ethernetaddr& dladdr,
                                     uint32_t nwaddr)
{
    storage::Query q;
    q["host"] = (int64_t) host;
    q["dladdr"] = (int64_t)(dladdr.hb_long());
    q["nwaddr"] = (int64_t) nwaddr;
    storage::Async_storage::Put_callback pcb =
        boost::bind(&Bindings_Storage::finish_put, this, _1, _2,
                    &host_serial_queue);
    Serial_Op_fn fn = boost::bind(&storage::Async_storage::put, np_store,
                                  HOST_TABLE_NAME, q, pcb);
    host_serial_queue.add_serial_op(fn);
}

void
Bindings_Storage::store_user_binding(int64_t user, int64_t host)
{
    storage::Query q;
    q["host"] = (int64_t) host;
    q["user"] = (int64_t) user;
    storage::Async_storage::Put_callback pcb =
        boost::bind(&Bindings_Storage::finish_put, this, _1, _2,
                    &user_serial_queue);
    Serial_Op_fn fn = boost::bind(&storage::Async_storage::put, np_store,
                                  USER_TABLE_NAME, q, pcb);
    user_serial_queue.add_serial_op(fn);
}

void
Bindings_Storage::finish_put(const storage::Result& result,
                             const storage::GUID& guid,
                             Serial_Op_Queue *serial_queue)
{
    if (result.code != storage::Result::SUCCESS) {
        lg.err("finish_put NDB error: %s.", result.message.c_str());
    }

    serial_queue->finished_serial_op();
}

void
Bindings_Storage::remove_location_binding(const ethernetaddr& dladdr,
                                          int64_t location)
{
    storage::Query q;
    q["dladdr"] = (int64_t)(dladdr.hb_long());
    q["location"] = (int64_t) location;
    storage::Async_storage::Remove_callback rcb =
        boost::bind(&Bindings_Storage::finish_remove, this, _1,
                    &dladdr_serial_queue);
    Serial_Op_fn fn = boost::bind(Storage_Util::non_trans_remove_all, np_store,
                                  DLADDR_TABLE_NAME, q, rcb);
    dladdr_serial_queue.add_serial_op(fn);
}

void
Bindings_Storage::remove_host_binding(int64_t host,
                                      const ethernetaddr& dladdr,
                                      uint32_t nwaddr)
{
    storage::Query q;
    q["host"] = (int64_t) host;
    q["dladdr"] = (int64_t)(dladdr.hb_long());
    q["nwaddr"] = (int64_t) nwaddr;
    storage::Async_storage::Remove_callback rcb =
        boost::bind(&Bindings_Storage::finish_remove, this, _1,
                    &host_serial_queue);
    Serial_Op_fn fn = boost::bind(Storage_Util::non_trans_remove_all, np_store,
                                  HOST_TABLE_NAME, q, rcb);
    host_serial_queue.add_serial_op(fn);
}

void
Bindings_Storage::remove_user_binding(int64_t user, int64_t host)
{
    storage::Query q;
    q["host"] = (int64_t) host;
    q["user"] = (int64_t) user;
    storage::Async_storage::Remove_callback rcb =
        boost::bind(&Bindings_Storage::finish_remove, this, _1,
                    &user_serial_queue);
    Serial_Op_fn fn = boost::bind(Storage_Util::non_trans_remove_all,
                                  np_store, USER_TABLE_NAME, q, rcb);
    user_serial_queue.add_serial_op(fn);
}

void
Bindings_Storage::finish_remove(const storage::Result& result,
                                Serial_Op_Queue *serial_queue)
{
    if (result.code != storage::Result::SUCCESS) {
        lg.err("finish_remove NDB error: %s.", result.message.c_str());
    }

    serial_queue->finished_serial_op();
}

void
Bindings_Storage::clear_table(const std::string& table_name,
                              Serial_Op_Queue *serial_queue)
{
    storage::Query q; // empty query

    storage::Async_storage::Remove_callback rcb =
        boost::bind(&Bindings_Storage::finish_remove, this, _1, serial_queue);
    Serial_Op_fn fn = boost::bind(Storage_Util::non_trans_remove_all,
                                  np_store, table_name, q, rcb);
    serial_queue->add_serial_op(fn);
}

void
Bindings_Storage::clear_bindings()
{
    clear_table(USER_TABLE_NAME, &user_serial_queue);
    clear_table(HOST_TABLE_NAME, &host_serial_queue);
    clear_table(DLADDR_TABLE_NAME, &dladdr_serial_queue);
}


void
Bindings_Storage::get_all_names(Name::Type name_type,
                                const Get_names_callback &cb)
{
    Get_All_Names_Op_ptr info(new Get_All_Names_Op(cb, name_type));

    std::string tablename;
    if (name_type == Name::USER) {
        tablename = USER_TABLE_NAME;
        info->attr = "user";
        info->principal_type = datatypes->user_type();
    } else if (name_type == Name::HOST) {
        tablename = HOST_TABLE_NAME;
        info->attr = "host";
        info->principal_type = datatypes->host_type();
    } else if (name_type == Name::LOCATION
               || name_type == Name::PORT)
    {
        tablename = LOCATION_TABLE_NAME;
        info->check_type = true;
        info->attr = "name";
        info->principal_type = datatypes->location_type();
    } else if (name_type == Name::SWITCH
               || name_type == Name::PORT)
    {
        tablename = LOCATION_TABLE_NAME;
        info->check_type = true;
        info->attr = "name";
        info->principal_type = datatypes->switch_type();
    } else {
        lg.err("get_all_names() called with invalid type = %d \n",
               name_type);
        post(boost::bind(cb, NameList()));
        return;
    }
    storage::Query q; // empty query retrives all rows
    np_store->get(tablename, q,
                  boost::bind(&Bindings_Storage::get_all_names_cb,
                              this, _1, _2, _3, info));
}

void
Bindings_Storage::get_all_names_cb(const storage::Result& result,
                                   const storage::Context& ctx,
                                   const storage::Row& row,
                                   const Get_All_Names_Op_ptr& info)
{
    if (result.code != storage::Result::SUCCESS) {
        if (result.code != storage::Result::NO_MORE_ROWS) {
            lg.err("get_all_names_cb NDB error: %s \n", result.message.c_str());
        }
        post(boost::bind(info->callback, info->names_found));
        return;
    }

    // we retrieve all entries, so we must reject those that don't match
    // the requested name type
    try {
        if (!info->check_type
            || info->name_type == ((Name::Type)
                                   Storage_Util::get_col_as_type<int64_t>(row, "name_type")))
        {
            Principal principal
                = { info->principal_type,
                    (int64_t) Storage_Util::get_col_as_type<int64_t>(row, info->attr) };
            Name name(data_cache->get_name_ref(principal), info->name_type);
            info->names_found.push_back(name);
        }
    } catch (std::exception &e) {
        lg.err("exception reading row in get_all_names(): %s \n", e.what());
    }
    np_store->get_next(ctx, boost::bind(&Bindings_Storage::get_all_names_cb,
                                        this, _1, _2, _3, info));
}

void
Bindings_Storage::return_loctuples(NameList& names,
                                   const Get_Bindings_Op_ptr& bindings,
                                   const Get_names_callback& cb)
{
    bindings->names.splice(bindings->names.end(), names);

    hash_map<int64_t, Loc>::const_iterator location;
    uint32_t i = 0;
    for (location = bindings->location_info.begin();
         location != bindings->location_info.end(); ++location, ++i)
    {
        if (i == bindings->index) {
            ++(bindings->index);
            get_names_for_location(location->second.dpid,
                                   location->second.port,
                                   Name::LOC_TUPLE,
                                   boost::bind(&Bindings_Storage::return_loctuples,
                                               this, _1, bindings, cb));
            return;
        }
    }
    post(boost::bind(cb, bindings->names));
}

void
Bindings_Storage::return_host_users(const Get_Bindings_Op_ptr& bindings,
                                    bool ret_users, const Get_names_callback& cb)
{
    hash_map<int64_t, std::list<int64_t> >::const_iterator uhost;
    std::list<int64_t>::const_iterator user;
    Principal principal = { ret_users ? datatypes->user_type()
                            : datatypes->host_type(), 0 };
    for (uhost = bindings->host_users.begin();
         uhost != bindings->host_users.end(); ++uhost)
    {
        if (ret_users) {
            for (user = uhost->second.begin(); user != uhost->second.end(); ++user) {
                principal.id = *user;
                const std::string& name
                    = data_cache->get_name_ref(principal);
                bindings->names.push_back(Name(name, Name::USER, *user));
            }
        } else {
            principal.id = uhost->first;
            const std::string& name
                = data_cache->get_name_ref(principal);
            bindings->names.push_back(Name(name, Name::HOST, uhost->first));
        }
    }
    post(boost::bind(cb, bindings->names));
}

void
Bindings_Storage::return_names(const Get_Bindings_Op_ptr& bindings,
                               const Get_names_callback& cb)
{
    Principal principal = { datatypes->user_type(), 0 } ;
    hash_map<int64_t, std::list<int64_t> >::const_iterator uhost;
    std::list<int64_t>::const_iterator user;
    for (uhost = bindings->host_users.begin();
         uhost != bindings->host_users.end(); ++uhost)
    {
        for (user = uhost->second.begin(); user != uhost->second.end(); ++user) {
            principal.id = *user;
            const std::string& name
                = data_cache->get_name_ref(principal);
            bindings->names.push_back(Name(name, Name::USER, *user));
        }
    }

    principal.type = datatypes->host_type();
    hash_map<uint64_t, std::list<nwhost> >::const_iterator dlnw;
    std::list<nwhost>::const_iterator host;
    for (dlnw = bindings->dladdr_nwhosts.begin();
         dlnw != bindings->dladdr_nwhosts.end(); ++dlnw)
    {
        for (host = dlnw->second.begin(); host != dlnw->second.end(); ++host) {
            principal.id = host->host;
            const std::string& name
                = data_cache->get_name_ref(principal);
            bindings->names.push_back(Name(name, Name::HOST, host->host));
        }
    }

    if (bindings->loc_tuples) {
        NameList tmp;
        bindings->index = 0;
        return_loctuples(tmp, bindings, cb);
        return;
    }

    principal.type = datatypes->location_type();
    hash_map<int64_t, Loc>::const_iterator location;
    for (location = bindings->location_info.begin();
         location != bindings->location_info.end(); ++location)
    {
        principal.id = location->first;
        const std::string& name
            = data_cache->get_name_ref(principal);
        bindings->names.push_back(Name(name, Name::LOCATION, location->first));
    }
    post(boost::bind(cb, bindings->names));
}

void
Bindings_Storage::return_entities(const Get_Bindings_Op_ptr& bindings,
                                  const Get_entities_callback& cb)
{
    hash_map<uint64_t, std::list<int64_t> >::const_iterator dladdr;
    std::list<int64_t>::const_iterator locname;
    hash_map<int64_t, Loc>::const_iterator loc;
    hash_map<uint64_t, std::list<nwhost> >::const_iterator dlnw;
    std::list<nwhost>::const_iterator host;
    std::list<uint32_t>::const_iterator nw;

    EntityList entities;

    for (dladdr = bindings->dladdr_locations.begin();
         dladdr != bindings->dladdr_locations.end(); ++dladdr)
    {
        for (locname = dladdr->second.begin();
             locname != dladdr->second.end(); ++locname)
        {
            loc = bindings->location_info.find(*locname);
            if (loc != bindings->location_info.end()) {
                dlnw = bindings->dladdr_nwhosts.find(dladdr->first);
                if (dlnw != bindings->dladdr_nwhosts.end()) {
                    for (host = dlnw->second.begin();
                         host != dlnw->second.end(); ++host)
                    {
                        for (nw = host->nwaddrs.begin();
                             nw != host->nwaddrs.end(); ++nw)
                        {
                            entities.push_back(NetEntity(loc->second.dpid,
                                                         loc->second.port,
                                                         ethernetaddr(dladdr->first),
                                                         *nw));
                        }
                    }
                } else {
                    VLOG_ERR(lg, "No dladdr/nwaddr bindings found.");
                }
            } else {
                VLOG_ERR(lg, "Location info for name not found.");
            }
        }
    }
    post(boost::bind(cb, entities));
}

void
Bindings_Storage::get_names_by_mac(const ethernetaddr &mac,
                                   const Get_names_callback &cb)
{
    // don't care about location info?
    Get_Bindings_Op_ptr info(new Get_Bindings_Op(boost::bind(&Bindings_Storage::return_names,
                                                             this, _1, cb),
                                                 GET_LOCATIONS, false));
    storage::Query q;
    q["dladdr"] = (int64_t) mac.hb_long();
    np_store->get(DLADDR_TABLE_NAME, q,
                  boost::bind(&Bindings_Storage::get_dladdrs_cb,
                              this, _1, _2, _3, info));
}

void
Bindings_Storage::get_names_by_ap(const datapathid &dpid, uint16_t port,
                                  const Get_names_callback &cb)
{
    storage::Query q;
    std::list<Loc> locations(1, Loc(dpid, port));
    get_names_by_ap2(locations, false, q,
                     boost::bind(&Bindings_Storage::return_names,
                                 this, _1, cb));
}

void
Bindings_Storage::get_names_by_ap2(std::list<Loc>& locations,
                                   bool loc_tuples,
                                   const storage::Query& q,
                                   const Get_bindings_callback &cb)
{
    Get_Bindings_Op_ptr info(new Get_Bindings_Op(cb, GET_DLADDRS_BY_LOC, loc_tuples, q));
    Loc& loc = locations.front();
    get_names_for_location(loc.dpid, loc.port, Name::LOCATION,
                           boost::bind(&Bindings_Storage::get_names_by_ap3,
                                       this, _1, locations, info));
}

void
Bindings_Storage::get_names_by_ap3(const NameList& locnames,
                                   std::list<Loc>& locations,
                                   const Get_Bindings_Op_ptr& info)
{
    for (NameList::const_iterator name = locnames.begin();
         name != locnames.end(); ++name)
    {
        info->location_info[name->id] = locations.front();
    }

    locations.pop_front();

    if (locations.empty()) {
        run_get_bindings_fsm(info);
    } else {
        Loc& loc = locations.front();
        get_names_for_location(loc.dpid, loc.port, Name::LOCATION,
                               boost::bind(&Bindings_Storage::get_names_by_ap3,
                                           this, _1, locations, info));
    }
}

void
Bindings_Storage::get_names_by_ip(uint32_t ip,
                                  const Get_names_callback &cb)
{
    Get_Bindings_Op_ptr info(new Get_Bindings_Op(boost::bind(&Bindings_Storage::return_names,
                                                             this, _1, cb),
                                                 GET_DLADDRS_BY_HOST, false));
    storage::Query q;
    q["nwaddr"] = (int64_t)ip;
    np_store->get(HOST_TABLE_NAME, q,
                  boost::bind(&Bindings_Storage::get_hosts_cb,
                              this, _1, _2, _3, info));
}

void
Bindings_Storage::get_names(const datapathid &dpid, uint16_t port,
                            const ethernetaddr &mac, uint32_t ip,
                            const Get_names_callback &cb)
{
    storage::Query q;
    q["dladdr"] = (int64_t) mac.hb_long();
    q["nwaddr"] = (int64_t) ip;
    std::list<Loc> locations(1, Loc(dpid, port));
    get_names_by_ap2(locations, false, q,
                     boost::bind(&Bindings_Storage::return_names,
                                 this, _1, cb));
}

void
Bindings_Storage::get_names(const storage::Query &query, bool loc_tuples,
                            const Get_names_callback &cb)
{
    bool dp = false;
    bool port = false;
    uint32_t nwfilters = 0;
    storage::Query filter;
    storage::Query::const_iterator q = query.begin();
    for ( ; q != query.end(); ++q) {
        if (q->first == "dpid") {
            dp = true;
        } else if (q->first == "port") {
            port = true;
        } else if (q->first == "dladdr") {
            filter["dladdr"] = q->second;
            ++nwfilters;
        } else if (q->first == "nwaddr") {
            filter["nwaddr"] = q->second;
            ++nwfilters;
        } else {
            VLOG_ERR(lg, "Unknown filter name %s.", q->first.c_str());
            post(boost::bind(cb, NameList()));
            return;
        }
    }

    if (dp != port) {
        VLOG_ERR(lg, "Must query on whole location.");
        post(boost::bind(cb, NameList()));
        return;
    } else if (dp) {
        datapathid dpid = datapathid::from_host((uint64_t) Storage_Util::get_col_as_type<int64_t>(query, "dpid"));
        uint16_t port = (uint16_t) Storage_Util::get_col_as_type<int64_t>(query, "port");
        std::list<Loc> locations(1, Loc(dpid, port));
        get_names_by_ap2(locations, loc_tuples, filter,
                         boost::bind(&Bindings_Storage::return_names, this, _1, cb));
    } else {
        Get_Bindings_Op_ptr info(new Get_Bindings_Op(
                                     boost::bind(&Bindings_Storage::return_names,
                                                 this, _1, cb),
                                     GET_DLADDRS_BY_HOST, loc_tuples));
        if (nwfilters > 1) {
            // because no index on addresses alone
            info->filter = filter;
            filter.erase("dladdr");
        }
        np_store->get(HOST_TABLE_NAME, filter,
                      boost::bind(&Bindings_Storage::get_hosts_cb,
                                  this, _1, _2, _3, info));
    }
}


void
Bindings_Storage::get_locations_cb(const storage::Result& result,
                                   const storage::Context& ctx,
                                   const storage::Row& row,
                                   const Get_Bindings_Op_ptr& info)
{
    if (result.code != storage::Result::SUCCESS) {
        if (result.code != storage::Result::NO_MORE_ROWS) {
            lg.err("get_locations_cb NDB error: %s \n", result.message.c_str());
        }
        run_get_bindings_fsm(info);
        return;
    }

    try {
        int64_t name = Storage_Util::get_col_as_type<int64_t>(row, "name");
        datapathid dpid = datapathid::from_host((uint64_t) Storage_Util::get_col_as_type<int64_t>(row, "dpid"));
        uint16_t port = (uint16_t) Storage_Util::get_col_as_type<int64_t>(row, "port");
        info->location_info[name] = Loc(dpid, port);
    } catch (std::exception &e) {
        lg.err("exception reading row in get_locations_cb(): %s \n", e.what());
    }
    np_store->get_next(ctx, boost::bind(&Bindings_Storage::get_locations_cb,
                                        this, _1, _2, _3, info));
}

void
Bindings_Storage::get_dladdrs_cb(const storage::Result& result,
                                 const storage::Context& ctx,
                                 const storage::Row& row,
                                 const Get_Bindings_Op_ptr& info)
{
    if (result.code != storage::Result::SUCCESS) {
        if (result.code != storage::Result::NO_MORE_ROWS) {
            lg.err("get_dladdrs_cb NDB error: %s \n", result.message.c_str());
        }
        run_get_bindings_fsm(info);
        return;
    }

    try {
        uint64_t dladdr = (uint64_t) Storage_Util::get_col_as_type<int64_t>(row, "dladdr");
        storage::Query::const_iterator q = info->filter.find("dladdr");
        bool add = (q == info->filter.end());
        if (!add) {
            uint64_t other = (uint64_t) Storage_Util::get_col_as_type<int64_t>(info->filter, "dladdr");
            add = (other == dladdr);
        }
        if (add) {
            int64_t location = (int64_t) Storage_Util::get_col_as_type<int64_t>(row, "location");
            info->dladdr_locations[dladdr].push_back(location);
        }
    } catch (std::exception &e) {
        lg.err("exception reading row in get_dladdrs_cb(): %s \n", e.what());
    }
    np_store->get_next(ctx, boost::bind(&Bindings_Storage::get_dladdrs_cb,
                                        this, _1, _2, _3, info));
}

void
Bindings_Storage::get_hosts_cb(const storage::Result& result,
                               const storage::Context& ctx,
                               const storage::Row& row,
                               const Get_Bindings_Op_ptr& info)
{
    if (result.code != storage::Result::SUCCESS) {
        if (result.code != storage::Result::NO_MORE_ROWS) {
            lg.err("get_hosts_cb NDB error: %s \n", result.message.c_str());
        }
        run_get_bindings_fsm(info);
        return;
    }

    try {
        uint64_t dl = (uint64_t) Storage_Util::get_col_as_type<int64_t>(row, "dladdr");
        storage::Query::const_iterator q = info->filter.find("dladdr");
        bool add = (q == info->filter.end());
        if (!add) {
            uint64_t other = (uint64_t) Storage_Util::get_col_as_type<int64_t>(info->filter, "dladdr");
            add = (other == dl);
        }
        if (add) {
            uint32_t nwaddr = (uint32_t) Storage_Util::get_col_as_type<int64_t>(row, "nwaddr");
            q = info->filter.find("nwaddr");
            add = (q == info->filter.end());
            if (!add) {
                uint32_t othernw = (uint32_t) Storage_Util::get_col_as_type<int64_t>(info->filter, "nwaddr");
                add = (othernw == nwaddr);
            }
            if (add) {
                ethernetaddr dladdr(dl);
                int64_t host = (int64_t) Storage_Util::get_col_as_type<int64_t>(row, "host");
                bool inserted = false;
                hash_map<uint64_t, std::list<nwhost> >::iterator entry =
                    info->dladdr_nwhosts.find(dl);
                if (entry != info->dladdr_nwhosts.end()) {
                    for (std::list<nwhost>::iterator nw = entry->second.begin();
                         nw != entry->second.end(); ++nw)
                    {
                        if (nw->host == host) {
                            nw->nwaddrs.push_back(nwaddr);
                            inserted = true;
                            break;
                        }
                    }
                }
                if (!inserted) {
                    nwhost nwentry = { host, std::list<uint32_t>(1, nwaddr) };
                    info->dladdr_nwhosts[dl].push_back(nwentry);
                }
            }
        }
    } catch (std::exception &e) {
        lg.err("exception reading row in get_hosts_cb(): %s \n", e.what());
    }
    np_store->get_next(ctx, boost::bind(&Bindings_Storage::get_hosts_cb,
                                        this, _1, _2, _3, info));
}

void
Bindings_Storage::get_users_cb(const storage::Result& result,
                               const storage::Context& ctx,
                               const storage::Row& row,
                               const Get_Bindings_Op_ptr& info)
{
    if (result.code != storage::Result::SUCCESS) {
        if (result.code != storage::Result::NO_MORE_ROWS) {
            lg.err("get_users_cb NDB error: %s \n", result.message.c_str());
        }
        run_get_bindings_fsm(info);
        return;
    }

    try {
        int64_t host = Storage_Util::get_col_as_type<int64_t>(row, "host");
        int64_t user = Storage_Util::get_col_as_type<int64_t>(row, "user");
        info->host_users[host].push_back(user);
    } catch (std::exception &e) {
        lg.err("exception reading row in get_users_cb(): %s \n", e.what());
    }
    np_store->get_next(ctx, boost::bind(&Bindings_Storage::get_users_cb,
                                        this, _1, _2, _3, info));
}

void
Bindings_Storage::run_get_bindings_fsm(const Get_Bindings_Op_ptr& info)
{
    uint32_t i = 0;
    storage::Query q;
    hash_map<int64_t, Loc>::const_iterator loc;
    hash_map<uint64_t, std::list<nwhost> >::const_iterator host;
    std::list<nwhost>::const_iterator nw;
    hash_map<uint64_t, std::list<int64_t> >::const_iterator dladdr;
    hash_map<int64_t, std::list<int64_t> >::const_iterator uhost;

    switch (info->cur_state) {
    case GET_LOCATIONS:
        for (dladdr = info->dladdr_locations.begin();
             dladdr != info->dladdr_locations.end(); ++dladdr)
        {
            for (std::list<int64_t>::const_iterator ap = dladdr->second.begin();
                 ap != dladdr->second.end(); ++ap, ++i)
            {
                if (i == info->index) {
                    ++(info->index);
                    if (info->location_info.find(*ap)
                        == info->location_info.end())
                    {
                        q["name"] = (int64_t) (*ap);
                        q["name_type"] = (int64_t) Name::LOCATION;
                        np_store->get(LOCATION_TABLE_NAME, q,
                                      boost::bind(&Bindings_Storage::get_locations_cb,
                                                  this, _1, _2, _3, info));
                        return;
                    }
                }
            }
        }
        info->index = 0;
        if (info->dladdr_nwhosts.empty()) {
            info->cur_state = GET_HOSTS_BY_DLADDR;
        } else if (info->host_users.empty()) {
            info->cur_state = GET_USERS;
        } else {
            break;
        }
        run_get_bindings_fsm(info);
        return;
    case GET_DLADDRS_BY_LOC:
        for (loc = info->location_info.begin();
             loc != info->location_info.end(); ++loc, ++i)
        {
            if (i == info->index) {
                ++(info->index);
                q["location"] = (int64_t) loc->first;
                np_store->get(DLADDR_TABLE_NAME, q,
                              boost::bind(&Bindings_Storage::get_dladdrs_cb,
                                          this, _1, _2, _3, info));
                return;
            }
        }
        info->index = 0;
        info->cur_state = GET_HOSTS_BY_DLADDR;
        run_get_bindings_fsm(info);
        return;
    case GET_DLADDRS_BY_HOST:
        for (host = info->dladdr_nwhosts.begin();
             host != info->dladdr_nwhosts.end(); ++host, ++i)
        {
            if (i == info->index) {
                ++(info->index);
                q["dladdr"] = (int64_t) host->first;
                np_store->get(DLADDR_TABLE_NAME, q,
                              boost::bind(&Bindings_Storage::get_dladdrs_cb,
                                          this, _1, _2, _3, info));
                return;
            }
        }
        info->index = 0;
        info->cur_state = GET_LOCATIONS;
        run_get_bindings_fsm(info);
        return;
    case GET_HOSTS_BY_DLADDR:
        for (dladdr = info->dladdr_locations.begin();
             dladdr != info->dladdr_locations.end(); ++dladdr, ++i)
        {
            if (i == info->index) {
                ++(info->index);
                q["dladdr"] = (int64_t) dladdr->first;
                np_store->get(HOST_TABLE_NAME, q,
                              boost::bind(&Bindings_Storage::get_hosts_cb,
                                          this, _1, _2, _3, info));
                return;
            }
        }
        info->index = 0;
        info->cur_state = GET_USERS;
        run_get_bindings_fsm(info);
        return;
    case GET_HOSTS_BY_USER:
        for (uhost = info->host_users.begin();
             uhost != info->host_users.end(); ++uhost, ++i)
        {
            if (i == info->index) {
                ++(info->index);
                q["host"] = (int64_t) uhost->first;
                np_store->get(HOST_TABLE_NAME, q,
                              boost::bind(&Bindings_Storage::get_hosts_cb,
                                          this, _1, _2, _3, info));
                return;
            }
        }
        info->index = 0;
        info->cur_state = GET_DLADDRS_BY_HOST;
        run_get_bindings_fsm(info);
        return;
    case GET_USERS:
        for (host = info->dladdr_nwhosts.begin();
             host != info->dladdr_nwhosts.end(); ++host)
        {
            for (nw = host->second.begin();
                 nw != host->second.end(); ++nw, ++i)
            {
                if (i == info->index) {
                    ++(info->index);
                    if (info->host_users.find(nw->host)
                        == info->host_users.end())
                    {
                        q["host"] = (int64_t) nw->host;
                        np_store->get(USER_TABLE_NAME, q,
                                      boost::bind(&Bindings_Storage::get_users_cb,
                                                  this, _1, _2, _3, info));
                        return;
                    }
                }
            }
        }
        break;
    case DONE:
        break;
    default:
        VLOG_ERR(lg, "Unknown state %u.", info->cur_state);
    }
    info->callback(info);
}

void
Bindings_Storage::get_host_users(int64_t hostname,
                                 const Get_names_callback& cb)
{
    storage::Query q;
    q["host"] = hostname;
    Get_Bindings_Op_ptr info(new Get_Bindings_Op(boost::bind(&Bindings_Storage::return_host_users,
                                                             this, _1, true, cb),
                                                 DONE, false));
    np_store->get(USER_TABLE_NAME, q,
                  boost::bind(&Bindings_Storage::get_users_cb, this, _1, _2, _3, info));
}

void
Bindings_Storage::get_user_hosts(int64_t username,
                                 const Get_names_callback& cb)
{
    storage::Query q;
    q["user"] = username;
    Get_Bindings_Op_ptr info(new Get_Bindings_Op(boost::bind(&Bindings_Storage::return_host_users,
                                                             this, _1, false, cb),
                                                 DONE, false));
    np_store->get(USER_TABLE_NAME, q,
                  boost::bind(&Bindings_Storage::get_users_cb, this, _1, _2, _3, info));
}

void
Bindings_Storage::get_entities_by_name(int64_t name,
                                       Name::Type name_type,
                                       const Get_entities_callback &cb)
{
    storage::Query q;
    if (name_type == Name::USER) {
        Get_Bindings_Op_ptr info(new Get_Bindings_Op(boost::bind(&Bindings_Storage::return_entities,
                                                                 this, _1, cb),
                                                     GET_HOSTS_BY_USER, false));
        q["user"] = name;
        np_store->get(USER_TABLE_NAME, q,
                      boost::bind(&Bindings_Storage::get_users_cb, this,
                                  _1, _2, _3, info));
    } else if (name_type == Name::HOST) {
        Get_Bindings_Op_ptr info(new Get_Bindings_Op(boost::bind(&Bindings_Storage::return_entities,
                                                                 this, _1, cb),
                                                     GET_DLADDRS_BY_HOST, false));
        q["host"] = name;
        np_store->get(HOST_TABLE_NAME, q,
                      boost::bind(&Bindings_Storage::get_hosts_cb, this,
                                  _1, _2, _3, info));
    } else if (name_type == Name::LOCATION) {
        Get_Bindings_Op_ptr info(new Get_Bindings_Op(boost::bind(&Bindings_Storage::return_entities,
                                                                 this, _1, cb),
                                                     GET_LOCATIONS, false));
        q["location"] = name;
        np_store->get(DLADDR_TABLE_NAME, q,
                      boost::bind(&Bindings_Storage::get_dladdrs_cb, this,
                                  _1, _2, _3, info));
    } else {
        // this just wraps another "external function", get_location_by_name()
        // which handles lookup for the following name types:
        // location, switch, port, loc_tuple
        Get_bindings_callback gb_cb = boost::bind(&Bindings_Storage::return_entities,
                                                  this, _1, cb);
        get_location_by_name(name, name_type,
                             boost::bind(&Bindings_Storage::get_names_by_ap2,
                                         this, _1, false, q, gb_cb));
    }
}

// links are now directional
void
Bindings_Storage::add_link(const datapathid &dpid1, uint16_t port1,
                           const datapathid &dpid2, uint16_t port2)
{
    Link to_add = Link(dpid1, port1, dpid2, port2);
    Get_links_callback cb = boost::bind(&Bindings_Storage::add_link_cb1, this,
                                        _1, to_add);
    Serial_Op_fn fn =
        boost::bind(&Bindings_Storage::get_all_links, this, cb);
    link_serial_queue.add_serial_op(fn);
}

void
Bindings_Storage::add_link_cb1(const std::list<Link> links, Link &to_add)
{
    std::list<Link>::const_iterator it = links.begin();
    for (; it != links.end(); ++it) {
        if (*it == to_add) {
            lg.err("Invalid attempt to add link already in the table: "
                   " (dp = %"PRId64", port = %d) -> (dp = %"PRId64", port = %d) \n",
                   to_add.dpid1.as_host(), to_add.port1,
                   to_add.dpid2.as_host(), to_add.port2);
            link_serial_queue.finished_serial_op();
            return; // all done
        }
    }

    // not a duplicate, we can proceed with insertion
    storage::Query q;
    to_add.fillQuery(q);
    storage::Async_storage::Put_callback pcb =
        boost::bind(&Bindings_Storage::finish_put, this, _1, _2,
                    &link_serial_queue);
    np_store->put(LINK_TABLE_NAME, q, pcb);
}

void
Bindings_Storage::remove_link(const datapathid &dpid1, uint16_t port1,
                              const datapathid &dpid2, uint16_t port2)
{
    Link to_delete = Link(dpid1, port1, dpid2, port2);

    storage::Query q;
    to_delete.fillQuery(q);
    storage::Async_storage::Remove_callback rcb =
        boost::bind(&Bindings_Storage::finish_remove, this, _1,
                    &link_serial_queue);

    Serial_Op_fn fn = boost::bind(&Storage_Util::non_trans_remove_all,
                                  np_store, LINK_TABLE_NAME, q, rcb);
    link_serial_queue.add_serial_op(fn);
}

void
Bindings_Storage::get_all_links(const Get_links_callback &cb)
{
    Get_Links_Op_ptr op(new Get_Links_Op(GL_ALL, cb));
    run_get_links_fsm(op);
}

// want to keep these as retrieving all links and then filtering?
void
Bindings_Storage::get_links(const datapathid dpid,
                            const Get_links_callback &cb)
{
    Get_Links_Op_ptr op(new Get_Links_Op(GL_DP, cb));
    op->filter_dpid = dpid;
    run_get_links_fsm(op);
}

void
Bindings_Storage::get_links(const datapathid dpid, uint16_t port,
                            const Get_links_callback &cb)
{
    Get_Links_Op_ptr op(new Get_Links_Op(GL_DP_Port, cb));
    op->filter_dpid = dpid;
    op->filter_port = port;
    run_get_links_fsm(op);
}

void
Bindings_Storage::run_get_links_fsm(const Get_Links_Op_ptr& op,
                                    GetLinksState next_state)
{
    if (next_state != GL_NONE) {
        op->cur_state = next_state;
    }

    storage::Query q; // empty query
    switch (op->cur_state) {
    case GL_FETCH_ALL:
        np_store->get(LINK_TABLE_NAME, q,
                      boost::bind(&Bindings_Storage::get_links_cb,
                                  this, _1, _2, _3, op));
        return;
    case GL_FILTER_AND_CALLBACK:
        filter_link_list(op);
        break;
    default:
        lg.err("Invalid state %d in run_get_link_fsm \n", op->cur_state);
    }

    post(boost::bind(op->callback, op->links));
}

void
Bindings_Storage::filter_link_list(const Get_Links_Op_ptr& op)
{
    if (op->type != GL_DP and op->type != GL_DP_Port) {
        return; // don't filter anything
    }

    std::list<Link>::iterator it = op->links.begin();
    std::list<Link> filtered_list;
    for ( ; it != op->links.end(); ++it) {
        bool switch_match = op->type == GL_DP && it->matches(op->filter_dpid);
        bool all_match = op->type == GL_DP_Port
            && it->matches(op->filter_dpid, op->filter_port);
        if (switch_match || all_match) {
            filtered_list.push_back(*it);
        }
    }
    op->links.swap(filtered_list);
}

void
Bindings_Storage::get_links_cb(const storage::Result &result,
                               const storage::Context & ctx,
                               const storage::Row &row,
                               const Get_Links_Op_ptr& op)
{
    if (result.code != storage::Result::SUCCESS
        && result.code != storage::Result::NO_MORE_ROWS)
    {
        lg.err("NDB error on get_links_internal_cb: %s \n",
               result.message.c_str());
        run_get_links_fsm(op, GL_FILTER_AND_CALLBACK);
        return;
    }

    assert(op->cur_state == GL_FETCH_ALL);

    if (result.code == storage::Result::NO_MORE_ROWS) {
        run_get_links_fsm(op, GL_FILTER_AND_CALLBACK);
        return;
    }

    // result.code == SUCCESS
    try {
        uint64_t dp1 = (uint64_t) Storage_Util::get_col_as_type<int64_t>(row, "dpid1");
        uint16_t p1 = (uint16_t) Storage_Util::get_col_as_type<int64_t>(row, "port1");
        uint64_t dp2 = (uint64_t) Storage_Util::get_col_as_type<int64_t>(row, "dpid2");
        uint16_t p2 = (uint16_t) Storage_Util::get_col_as_type<int64_t>(row, "port2");
        Link l(datapathid::from_host(dp1), p1, datapathid::from_host(dp2), p2);
        op->links.push_back(l);
    } catch (std::exception &e) {
        // print error but keep trying to read more rows
        lg.err("get_links_cb exception: %s \n", e.what());
    }

    np_store->get_next(ctx, boost::bind(&Bindings_Storage::get_links_cb,
                                        this, _1, _2, _3, op));
}

// removes all link entries.
void
Bindings_Storage::clear_links()
{
    clear_table(LINK_TABLE_NAME, &link_serial_queue);
}

void
Bindings_Storage::add_name_for_location(const datapathid &dpid,
                                        uint16_t port, int64_t name,
                                        Name::Type name_type)
{
    storage::Query q;
    q["dpid"] = (int64_t) dpid.as_host();

    if (name_type == Name::LOCATION || name_type == Name::PORT) {
        q["port"] = (int64_t) port;
    } else if (name_type == Name::SWITCH) {
        q["port"] = (int64_t) Loc::NO_PORT;
    } else {
        lg.err("invalid name type %d in add_name_for_location\n",
               (int)name_type);
        return;
    }

    q["name"] = (int64_t) name;
    q["name_type"] = (int64_t) name_type;

    storage::Async_storage::Put_callback pcb =
        boost::bind(&Bindings_Storage::finish_put, this, _1, _2,
                    &location_serial_queue);
    Serial_Op_fn fn = boost::bind(&storage::Async_storage::put,
                                  np_store, LOCATION_TABLE_NAME, q, pcb);
    location_serial_queue.add_serial_op(fn);
}

void
Bindings_Storage::remove_name_for_location(const datapathid &dpid,
                                           uint16_t port, int64_t name,
                                           Name::Type name_type)
{
    storage::Query q;
    int64_t port64 = (name_type == Name::SWITCH) ? Loc::NO_PORT : port;

    q["dpid"] = (int64_t) dpid.as_host();

    if (!(name_type == Name::SWITCH && name == 0)) {
        q["port"] = port64;
        if (name != 0) {
            q["name_type"] = (int64_t) name_type;
            q["name"] = (int64_t) name;
        }
    }

    storage::Async_storage::Remove_callback rcb =
        boost::bind(&Bindings_Storage::finish_remove, this, _1,
                    &location_serial_queue);

    Serial_Op_fn fn = boost::bind(&Storage_Util::non_trans_remove_all,
                                  np_store, LOCATION_TABLE_NAME, q, rcb);
    location_serial_queue.add_serial_op(fn);
}

void
Bindings_Storage::get_names_for_location(const datapathid &dpid,
                                         uint16_t port,
                                         Name::Type name_type,
                                         const Get_names_callback &cb)
{
    storage::Query q;
    q["dpid"] = (int64_t) dpid.as_host();
    if (name_type == Name::LOCATION || name_type == Name::PORT
       || name_type == Name::LOC_TUPLE)
    {
        q["port"] = (int64_t) port;
    } else if (name_type == Name::SWITCH) {
        q["port"] = (int64_t) Loc::NO_PORT;
    } else {
        lg.err("invalid name type %d in get_names_for_location\n", (int)name_type);
        post(boost::bind(cb, NameList()));
        return;
    }

    if (name_type == Name::LOC_TUPLE) {
        q["name_type"] = (int64_t) Name::LOCATION;
    } else {
        q["name_type"] = (int64_t) name_type;
    }
    get_names_for_location(q, cb, name_type);
}

void
Bindings_Storage::get_names_for_location(storage::Query &q,
                                         const Get_names_callback &cb,
                                         Name::Type type)
{
    Get_LocNames_Op_ptr op(new Get_LocNames_Op(cb, q, type));
    np_store->get(LOCATION_TABLE_NAME, q,
                  boost::bind(&Bindings_Storage::get_locnames_cb,
                              this, _1, _2, _3, op));
}

void
Bindings_Storage::get_locnames_cb(const storage::Result &result,
                                  const storage::Context & ctx,
                                  const storage::Row &row,
                                  const Get_LocNames_Op_ptr& op)
{
    if (result.code != storage::Result::SUCCESS
       && result.code != storage::Result::NO_MORE_ROWS)
    {
        // this call commonly sees concurrent mods when net event log
        // entries are generated due to a datapath leave.
        if (result.code == storage::Result::CONCURRENT_MODIFICATION) {
            lg.err("NDB error on get_locnames_cb (ok if transient): %s \n",
                   result.message.c_str());
        } else {
            lg.err("NDB error on get_locnames_cb: %s \n",
                   result.message.c_str());
        }
        goto do_callback;
    }

    if (result.code == storage::Result::NO_MORE_ROWS) {
        if((op->loc_names.size() == 0 && op->type == Name::LOCATION)
           || op->type == Name::LOC_TUPLE)
        {
            // continue lookups to find a switch name and port name
            // if it exists.  First, look for the switch name
            storage::Query switch_query = op->query;
            switch_query["port"] = (int64_t) Loc::NO_PORT;
            switch_query["name_type"] = (int64_t) Name::SWITCH;
            get_names_for_location(switch_query,
                                   boost::bind(&Bindings_Storage::get_locnames_cb2,
                                               this, _1, op),
                                   Name::SWITCH);
            return;
        } else {
            goto do_callback; // all done
        }
    }

    // result.code == SUCCESS
    try {
        Principal principal
            = { 0, Storage_Util::get_col_as_type<int64_t>(row, "name") };
        Name::Type type
            = (Name::Type) Storage_Util::get_col_as_type<int64_t>(row, "name_type");
        if (type == Name::SWITCH) {
            principal.type = datatypes->switch_type();
        } else {
            principal.type = datatypes->location_type();
        }
        const std::string& name = data_cache->get_name_ref(principal);
        op->loc_names.push_back(Name(name, type, principal.id));
    } catch (std::exception &e) {
        // print error but keep trying to read more rows
        lg.err("get_locnames_cb exception: %s \n", e.what());
    }

    np_store->get_next(ctx, boost::bind(&Bindings_Storage::get_locnames_cb,
                                        this, _1, _2, _3, op));
    return;

do_callback:
    post(boost::bind(op->callback, op->loc_names));
}

// continuation of look-up for switch-name and port name
void
Bindings_Storage::get_locnames_cb2(const NameList &names,
                                   const Get_LocNames_Op_ptr& op)
{
    std::string s;
    if (names.size() > 0) {
        s = names.front().name;
    } else {
        int64_t dp_i = 0;
        try {
            dp_i = Storage_Util::get_col_as_type<int64_t>(op->query, "dpid");
            // this should no longer happen now that we have a discovered directory
            lg.dbg("failed to find name for switch with dpid = %"PRId64"\n",dp_i);
            datapathid dpid = datapathid::from_host((uint64_t) dp_i);
            std::stringstream ss;
            ss << "none;" << dpid.string();
            s = ss.str();
        } catch (std::exception &e) {
            lg.err("get_locnames_cb2 failed to read dpid: %s \n", e.what());
        }
    }
    storage::Query portname_query = op->query;
    portname_query["name_type"] = (int64_t) Name::PORT;
    get_names_for_location(portname_query,
                           boost::bind(&Bindings_Storage::get_locnames_cb3,
                                       this, _1, op, s),
                           Name::PORT);
}

void
Bindings_Storage::str_replace(std::string &str, const std::string &target,
                              const std::string &replace)
{
    size_t pos = 0;
    for (; (pos = str.find(target,pos)) != std::string::npos;) {
        str.replace(pos, target.length(), replace);
        pos += replace.size();
    }
}

// callback for look-up of port name
void
Bindings_Storage::get_locnames_cb3(const NameList &names,
                                   const Get_LocNames_Op_ptr& op,
                                   std::string& switch_name)
{
    std::string portname;
    if (names.size() > 0) {
        portname = names.front().name;
    } else {
        uint16_t port = 0;
        try {
            port = (uint16_t)
                Storage_Util::get_col_as_type<int64_t>(op->query, "port");
        } catch (std::exception &e) {
            lg.err("get_locnames_cb3 failed to read port: %s \n", e.what());
        }
        std::stringstream ss;
        ss << port;
        portname = ss.str();
    }

    // If no name was found, we want to add a single
    // switchname:port entry to op->loc_names, whether op->type
    // is LOCATION or LOC_TUPLE
    if (op->loc_names.size() == 0) {
        std::stringstream ss;
        ss << switch_name << ":" << portname;
        op->loc_names.push_back(Name(ss.str(), Name::LOCATION, -1));
    }

    // Additionally, if the type was LOC_TUPLE, we want to append
    // "#switch-name#portname" to each entry that is already in
    // op->loc_names
    if (op->type == Name::LOC_TUPLE) {
        NameList::iterator it = op->loc_names.begin();
        for (; it != op->loc_names.end(); ++it) {
            std::string locname = it->name;
            str_replace(locname, "\\", "\\\\");
            str_replace(locname, "#", "\\#");
            str_replace(switch_name, "\\", "\\\\");
            str_replace(switch_name, "#", "\\#");
            str_replace(portname, "\\", "\\\\");
            str_replace(portname, "#", "\\#");

            std::stringstream ss;
            ss << locname << "#" << switch_name << "#" << portname;
            it->name = ss.str();
        }
    }
    // all done, do callback
    post(boost::bind(op->callback, op->loc_names));
}

void
Bindings_Storage::get_location_by_name(int64_t name,
                                       Name::Type name_type,
                                       const Get_locations_callback &cb)
{
    Get_Loc_By_Name_Op_ptr op(new Get_Loc_By_Name_Op(cb));

    storage::Query q;
    q["name"] = name;
    q["name_type"] = (int64_t) name_type;
    np_store->get(LOCATION_TABLE_NAME, q,
                  boost::bind(&Bindings_Storage::get_loc_by_name_cb,
                              this, _1, _2, _3, op));

}

void
Bindings_Storage::get_loc_by_name_cb(const storage::Result &result,
                                     const storage::Context & ctx,
                                     const storage::Row &row,
                                     const Get_Loc_By_Name_Op_ptr& op)
{
    if (result.code != storage::Result::SUCCESS
       && result.code != storage::Result::NO_MORE_ROWS)
    {
        lg.err("NDB error on get_loc_by_name_cb: %s \n",
               result.message.c_str());
        goto do_callback;
    }

    if (result.code == storage::Result::NO_MORE_ROWS) {
        goto do_callback;
    }

    // result.code == SUCCESS
    try {
        int64_t dpid = Storage_Util::get_col_as_type<int64_t>(row, "dpid");
        int64_t port = Storage_Util::get_col_as_type<int64_t>(row, "port");
        op->locations.push_back(Loc(datapathid::from_host((uint64_t)dpid), (uint16_t) port));
    } catch (std::exception &e) {
        // print error but keep trying to read more rows
        lg.err("get_loc_by_name_cb exception: %s \n", e.what());
    }

    np_store->get_next(ctx, boost::bind(&Bindings_Storage::get_loc_by_name_cb,
                                        this, _1, _2, _3, op));
    return;

do_callback:
    post(boost::bind(op->callback, op->locations));
}


void
Bindings_Storage::print_names(const NameList &name_list)
{
    if(name_list.size() == 0) {
        printf("[ Empty Name List ] \n");
    }

    NameList::const_iterator it = name_list.begin();
    for( ; it != name_list.end(); it++) {
        printf("name = %s  type = %d \n", it->name.c_str(), it->name_type);
    }
}

void
Bindings_Storage::getInstance(const container::Context* ctxt,
                              Bindings_Storage*& h)
{
    h = dynamic_cast<Bindings_Storage*>
        (ctxt->get_by_interface(container::Interface_description
                                (typeid(Bindings_Storage).name())));
}

REGISTER_COMPONENT(container::Simple_component_factory<Bindings_Storage>,
                   Bindings_Storage);


}
}
