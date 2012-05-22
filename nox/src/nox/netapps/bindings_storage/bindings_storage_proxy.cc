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

#include "bindings_storage_proxy.hh"
#include "pyrt/pycontext.hh"
#include "swigpyrun.h"
#include "vlog.hh"

using namespace std;
using namespace vigil;
using namespace vigil::applications;

namespace {
  Vlog_module lg("bindings_storage_proxy");
}

namespace vigil {
namespace applications {

Bindings_Storage_Proxy::Bindings_Storage_Proxy(PyObject* ctxt)
    : b_store(0)
{
    if (!SWIG_Python_GetSwigThis(ctxt) || !SWIG_Python_GetSwigThis(ctxt)->ptr) {
        throw runtime_error("Unable to access Python context.");
    }

    c = ((PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr)->c;
}

void
Bindings_Storage_Proxy::configure(PyObject* configuration) {
    c->resolve(b_store);
}

void
Bindings_Storage_Proxy::install(PyObject*) {}

void
Bindings_Storage_Proxy::python_callback(PyObject *args,
                                        boost::intrusive_ptr<PyObject> cb)
{
    Co_critical_section c;
    PyObject* ret = PyObject_CallObject(cb.get(), args);
    if (ret == 0) {
        const string exc = pretty_print_python_exception();
        lg.err("Python callback invocation failed:\n%s", exc.c_str());
    }

    Py_DECREF(args);
    Py_XDECREF(ret);
}


void
Bindings_Storage_Proxy::get_names_callback(const NameList &name_list,
                                           boost::intrusive_ptr<PyObject> cb)
{
    PyObject* args = PyTuple_New(1);
    PyTuple_SetItem(args, 0, to_python_list(name_list));
    python_callback(args,cb);
}

PyObject *
Bindings_Storage_Proxy::get_names_dispatch(
    boost::function<void(Get_names_callback &f)> dispatch_fn,
    PyObject *cb)
{
    try {
        if (!cb || !PyCallable_Check(cb)) { throw "Invalid callback"; }

        boost::intrusive_ptr<PyObject> cptr(cb, true);
        Get_names_callback f = boost::bind(
            &Bindings_Storage_Proxy::get_names_callback,this,_1,cptr);
        dispatch_fn(f);
        Py_RETURN_NONE;
    } catch (const char* msg) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, msg);
        return 0;
    }
}

PyObject*
Bindings_Storage_Proxy::get_names_by_ap(const datapathid &dpid,
                                        uint16_t port, PyObject *cb)
{
    return get_names_dispatch(boost::bind(&Bindings_Storage::get_names_by_ap,
                                          b_store, dpid, port, _1),cb);
}

PyObject*
Bindings_Storage_Proxy::get_names_by_mac(const ethernetaddr &mac,
                                         PyObject *cb)
{
    return get_names_dispatch(boost::bind(&Bindings_Storage::get_names_by_mac,
                                          b_store, mac, _1),cb);
}

PyObject*
Bindings_Storage_Proxy::get_names_by_ip(uint32_t ip,
                                        PyObject *cb)
{
    return get_names_dispatch(boost::bind(&Bindings_Storage::get_names_by_ip,
                                          b_store, ip, _1),cb);
}


PyObject*
Bindings_Storage_Proxy::get_names(const datapathid &dpid,
                                  uint16_t port, const ethernetaddr &mac,
                                  uint32_t ip, PyObject *cb)
{
    return get_names_dispatch(boost::bind(&Bindings_Storage::get_names,
                                          b_store, dpid,port,mac,ip, _1),cb);
}

PyObject*
Bindings_Storage_Proxy::get_names(const storage::Query &query, bool loc_tuples,
                                  PyObject *cb)
{
    return get_names_dispatch(boost::bind(&Bindings_Storage::get_names,
                                          b_store, query, loc_tuples, _1), cb);
}

PyObject*
Bindings_Storage_Proxy::get_host_users(int64_t hostname,
                                       PyObject *cb)
{
    return get_names_dispatch(boost::bind(&Bindings_Storage::get_host_users,
                                          b_store, hostname, _1), cb);
}

PyObject*
Bindings_Storage_Proxy::get_user_hosts(int64_t username,
                                       PyObject *cb)
{
    return get_names_dispatch(boost::bind(&Bindings_Storage::get_user_hosts,
                                          b_store, username, _1), cb);
}

PyObject *
Bindings_Storage_Proxy::get_all_names(int name_type, PyObject *cb)
{
    return get_names_dispatch(boost::bind(&Bindings_Storage::get_all_names,
                                          b_store, (Name::Type) name_type, _1),cb);
}

void
Bindings_Storage_Proxy::get_entities_callback(const EntityList &entity_list,
                                              boost::intrusive_ptr<PyObject> cb)
{
    PyObject* args = PyTuple_New(1);
    PyTuple_SetItem(args, 0, to_python_list(entity_list));
    python_callback(args,cb);
}

PyObject*
Bindings_Storage_Proxy::get_entities_by_name(int64_t name,
                                             int name_type,
                                             PyObject *cb)
{
    try {
        if (!cb || !PyCallable_Check(cb)) { throw "Invalid callback"; }

        boost::intrusive_ptr<PyObject> cptr(cb, true);
        Get_entities_callback f = boost::bind(
            &Bindings_Storage_Proxy::get_entities_callback,this,_1,cptr);
        b_store->get_entities_by_name(name, (Name::Type)name_type,f); 
        Py_RETURN_NONE;
    } catch (const char* msg) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, msg);
        return 0;
    }
}

void
Bindings_Storage_Proxy::add_link(const datapathid &dpid1, uint16_t port1,
                                 const datapathid &dpid2, uint16_t port2)
{
    b_store->add_link(dpid1,port1,dpid2,port2);
}

void
Bindings_Storage_Proxy::remove_link(const datapathid &dpid1,
                                    uint16_t port1, const datapathid &dpid2,
                                    uint16_t port2)
{
    b_store->remove_link(dpid1,port1,dpid2,port2);
}

PyObject*
Bindings_Storage_Proxy::get_all_links(PyObject *cb)
{
    return get_links_dispatch(boost::bind(&Bindings_Storage::get_all_links,
                                          b_store, _1),cb);
}

PyObject*
Bindings_Storage_Proxy::get_links(const datapathid dpid,PyObject *cb)
{
    return get_links_dispatch(boost::bind(&Bindings_Storage::get_links,
                                          b_store, dpid, _1),cb);
}

PyObject*
Bindings_Storage_Proxy::get_links(const datapathid dpid,
                                  uint16_t port, PyObject *cb)
{
    return get_links_dispatch(boost::bind(&Bindings_Storage::get_links,
                                          b_store, dpid,port, _1),cb);
}

PyObject *
Bindings_Storage_Proxy::get_links_dispatch(
    boost::function<void(Get_links_callback &f)> dispatch_fn,
    PyObject *cb)
{
    try {
        if (!cb || !PyCallable_Check(cb)) { throw "Invalid callback"; }

        boost::intrusive_ptr<PyObject> cptr(cb, true);
        Get_links_callback f = boost::bind(
            &Bindings_Storage_Proxy::get_links_callback,this,_1,cptr);
        dispatch_fn(f);
        Py_RETURN_NONE;
    } catch (const char* msg) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, msg);
        return 0;
    }
}

void
Bindings_Storage_Proxy::get_links_callback(const list<Link> &link_list,
                                           boost::intrusive_ptr<PyObject> cb)
{
    PyObject* args = PyTuple_New(1);
    PyTuple_SetItem(args, 0, to_python_list(link_list));
    python_callback(args,cb);
}

PyObject*
Bindings_Storage_Proxy::get_names_for_location(const datapathid &dpid, uint16_t port,
                                               int name_type, PyObject *cb)
{
    return get_names_dispatch(
        boost::bind(&Bindings_Storage::get_names_for_location,
                    b_store, dpid, port,(Name::Type)name_type, _1),cb);
}

PyObject*
Bindings_Storage_Proxy::get_location_by_name(int64_t name,
                                             int name_type, PyObject *cb)
{
    return get_location_dispatch(boost::bind(
                                     &Bindings_Storage::get_location_by_name,
                                     b_store, name, (Name::Type)name_type, _1),cb);
}

PyObject *
Bindings_Storage_Proxy::get_location_dispatch(
    boost::function<void(Get_locations_callback &f)> dispatch_fn,
    PyObject *cb)
{
    try {
        if (!cb || !PyCallable_Check(cb)) { throw "Invalid callback"; }

        boost::intrusive_ptr<PyObject> cptr(cb, true);
        Get_locations_callback f = boost::bind(
            &Bindings_Storage_Proxy::get_location_by_name_callback,
            this,_1,cptr);
        dispatch_fn(f);
        Py_RETURN_NONE;
    } catch (const char* msg) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, msg);
        return 0;
    }
}

void
Bindings_Storage_Proxy::get_location_by_name_callback(
    const LocationList & locations,
    boost::intrusive_ptr<PyObject> cb)
{
    PyObject* args = PyTuple_New(1);
    PyTuple_SetItem(args, 0, to_python_list(locations));
    python_callback(args,cb);
}


} // namespace applications
} // namespace vigil
