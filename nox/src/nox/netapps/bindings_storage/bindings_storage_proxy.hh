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
#ifndef BINDINGS_STORAGE_PROXY_HH
#define BINDINGS_STORAGE_PROXY_HH 1

#include <Python.h>

#include "bindings_storage.hh"
#include "pyrt/pyglue.hh"

using namespace std;

namespace vigil {
namespace applications {

class Bindings_Storage_Proxy{
public:
    Bindings_Storage_Proxy(PyObject* ctxt);

    void configure(PyObject*);
    void install(PyObject*);

    PyObject *get_names_by_ap(const datapathid &dpid, uint16_t port,
                              PyObject *cb);
    PyObject *get_names_by_mac(const ethernetaddr &mac,
                               PyObject *cb);
    PyObject *get_names_by_ip(uint32_t ip, PyObject *cb);
    PyObject *get_names(const datapathid &dpid, uint16_t port,
                        const ethernetaddr &mac, uint32_t ip, PyObject *cb);

    PyObject *get_names(const storage::Query &query,
                        bool loc_tuples, PyObject *cb);

    PyObject *get_host_users(int64_t hostname, PyObject *cb);
    PyObject *get_user_hosts(int64_t username, PyObject *cb);

    PyObject *get_all_names(int name_type, PyObject *cb);

    PyObject *get_entities_by_name(int64_t name, int name_type,
                                   PyObject *cb);

    // functions for link bindings
    PyObject* get_links(const datapathid dpid,uint16_t port, PyObject *cb);
    PyObject* get_links(const datapathid dpid, PyObject *cb);
    PyObject* get_all_links(PyObject *cb);
    void add_link(const datapathid &dpid1, uint16_t port1,
                  const datapathid &dpid2, uint16_t port2);

    void remove_link(const datapathid &dpid1, uint16_t port1,
                  const datapathid &dpid2, uint16_t port2);

    void clear_links() { b_store->clear_links(); }

    PyObject* get_names_for_location(const datapathid &dpid, uint16_t port,
                                     int name_type, PyObject *cb);

    PyObject* get_location_by_name(int64_t name,
                                   int name_type, PyObject *cb);


protected:

    Bindings_Storage* b_store;
    container::Component* c;

    void get_names_callback(const NameList &name_list,
                            boost::intrusive_ptr<PyObject> cb);

    void get_entities_callback(const EntityList &entity_list,
                            boost::intrusive_ptr<PyObject> cb);

    PyObject *get_names_dispatch(
            boost::function<void(Get_names_callback &f)> dispatch_fn,
            PyObject *cb);
    void python_callback(PyObject *args, boost::intrusive_ptr<PyObject> cb);

    PyObject *get_links_dispatch(
            boost::function<void(Get_links_callback &f)> dispatch_fn,
            PyObject *cb);

    void get_links_callback(const list<Link> &link_list,
                            boost::intrusive_ptr<PyObject> cb);

    PyObject *get_location_dispatch(
        boost::function<void(Get_locations_callback &f)> dispatch_fn,
        PyObject *cb);
    void get_location_by_name_callback(
        const LocationList & locations,
        boost::intrusive_ptr<PyObject> cb);

};



}
}

#endif
