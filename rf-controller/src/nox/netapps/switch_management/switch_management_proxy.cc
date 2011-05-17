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

#include "switch_management_proxy.hh"
#include "nox.hh"

#include "boost/foreach.hpp"
#include "pyrt/pycontext.hh"
#include "swigpyrun.h"
#include "vlog.hh"

using namespace std;
using namespace vigil;
using namespace vigil::applications;

namespace {
Vlog_module lg("switch_management_proxy");
}

namespace vigil {
namespace applications {

Switch_management_proxy::Switch_management_proxy(PyObject* ctxt)
{
    pydeferred_class = pymethod_name = NULL;
    if (!SWIG_Python_GetSwigThis(ctxt) || !SWIG_Python_GetSwigThis(ctxt)->ptr) {
        throw runtime_error("Unable to access Python context.");
    }
    
    c = ((PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr)->c;
}

void
Switch_management_proxy::configure(PyObject* configuration) 
{
    c->resolve(switch_mgmt);
}

void 
Switch_management_proxy::install(PyObject*) 
{
}

datapathid
Switch_management_proxy::get_mgmt_id(const datapathid &dpid) {
    return nox::dpid_to_mgmtid(dpid);
}

string
Switch_management_proxy::mgmtid_to_system_uuid(const datapathid &mgmt_id)
{
    boost::shared_ptr<Switch_mgr> swm = nox::mgmtid_to_swm(mgmt_id);
    if (swm == NULL) {
        lg.warn("Failed to resolve switch manager for id %s",
                mgmt_id.string().c_str());
        return "";
    }
    std::string server_uuid; 
    if (!swm->get_system_uuid(mgmt_id, server_uuid)) { 
        lg.warn("Failed to get system uuid for mgmt-id %s",
                mgmt_id.string().c_str());
        return "";
    } 
    return server_uuid;
}

bool
Switch_management_proxy::switch_mgr_is_active(const datapathid& mgmt_id)
{
    boost::shared_ptr<Switch_mgr> swm = nox::mgmtid_to_swm(mgmt_id);
    return swm != NULL;
}

string
Switch_management_proxy::get_port_name(const datapathid &mgmt_id,
                                       const datapathid &port_dpid)
{
    boost::shared_ptr<Switch_mgr> swm = nox::mgmtid_to_swm(mgmt_id);
    if (swm == NULL) {
        lg.warn("Failed to resolve switch manager for id %s",
                mgmt_id.string().c_str());
        return "";
    }
    std::string port_name;
    if (!swm->get_port_name(port_dpid, port_name)) {
        lg.warn("Failed to resolve port in switch manager for id %s",
                mgmt_id.string().c_str());
    }
    return port_name;
}

PyObject*
Switch_management_proxy::set_all_entries(datapathid mgmt_id, string key,
        list<string> vals)
{
    if (pydeferred_class == NULL) {
        PyObject* pydeferred_mod = PyImport_ImportModule("twisted.internet.defer");
        if (!pydeferred_mod) {
            const string msg = pretty_print_python_exception();
            lg.warn("Failed to import required twisted module:\n%s", msg.c_str());
            Py_RETURN_NONE;
        }

        pydeferred_class = PyObject_GetAttrString(pydeferred_mod, "Deferred");
        Py_DECREF(pydeferred_mod);
        if (!pydeferred_class || !PyCallable_Check(pydeferred_class)) {
            const string msg = pretty_print_python_exception();
            lg.warn("Failed to locate twisted Deferred class:\n%s", msg.c_str());
            Py_RETURN_NONE;
        }
    }

    if (pymethod_name == NULL) {
        pymethod_name = PyString_FromString("callback");
        if (!pymethod_name) {
            const string msg = pretty_print_python_exception();
            lg.warn("Cannot create method name:\n%s", msg.c_str());
            Py_RETURN_NONE;
        }
    }

    PyObject* deferred = PyObject_CallObject(pydeferred_class, 0);
    if (!deferred) {
        const string msg = pretty_print_python_exception();
        lg.err("cannot instantiate deferred object:\n%s", msg.c_str());
        Py_RETURN_NONE;
    }

    //TODO: this isn't being polished right now because the mgmt
    //interface may be changing
    boost::shared_ptr<Switch_mgr> swm = nox::mgmtid_to_swm(mgmt_id);
    if (swm == NULL) {
        lg.warn("Failed to resolve switch manager for id %s",
                mgmt_id.string().c_str());
        Py_DECREF(deferred);
        Py_RETURN_NONE; //XXX retry
    }
    while (swm->has_key(key)) {
        swm->del_entry(key, swm->get_string(0, key));
    }
    BOOST_FOREACH(string s, vals) {
        swm->set_string(key, s);
    }

    boost::function<void(bool)> cb
        = boost::bind(&Switch_management_proxy::callback,
                      this, _1, deferred);
    if (swm->commit(cb) != 0) {
        Py_DECREF(deferred);
        Py_RETURN_NONE;
    }

    Py_INCREF(deferred); // for callback
    return deferred;
}

void
Switch_management_proxy::callback(bool success, PyObject *deferred)
{
    PyObject *res = NULL;
    if (success) {
        res = PyObject_CallMethodObjArgs(deferred, pymethod_name,
                                         Py_True, NULL);
    } else {
        res = PyObject_CallMethodObjArgs(deferred, pymethod_name,
                                         Py_False, NULL);
    }

    if (!res) {
        const string msg = pretty_print_python_exception();
        lg.err("cannot call deferred:\n%s", msg.c_str());
    } else {
        Py_DECREF(res);
    }
    Py_DECREF(deferred);
}

}} // vigil::applications
