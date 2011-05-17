/* Copyright 2008 (C) Nicira, Inc.
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
#include "pycomponent.hh"

#ifndef SWIG
#include "swigpyrun.h"
#endif // SWIG

#include <Python.h>
#include <cstdio>
#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include "deferredcallback.hh"
#include "pyrt/pyrt.hh"
#include "pycontext.hh"
#include "vlog.hh"
#include "json-util.hh"

using namespace std;
using namespace vigil;
using namespace vigil::applications;
using namespace vigil::container;

static Vlog_module lg("pycomponent");

PyComponent::PyComponent(const Context* c, const json_object* conf)
    : Component(c), pyobj(0) {
    // Import the Python component implementation module and get the factory.
    json_object* mod = json::get_dict_value(conf, "python"); 
    string module = mod->get_string(true);
    
    lg.dbg("Importing Python module %s", module.c_str());

    PyObject* m = PyImport_ImportModule(module.c_str());
    if (!m) {
        throw runtime_error("cannot import a Python module '" + module +
                            "':\n" + pretty_print_python_exception());
    }

    PyObject* d = PyModule_GetDict(m);
    // d is borrowed, don't DECREF

    PyObject* func = PyDict_GetItemString(d, "getFactory");
    // func is borrowed, don't DECREF
    if (!func) {
        const string exception_msg =  pretty_print_python_exception();
        Py_DECREF(m);
        throw runtime_error("cannot find the Python getFactory method:\n" +
                            exception_msg);
    }

    PyObject* pyf = PyObject_CallObject(func, 0);
    if (!pyf) {
        const string exception_msg =  pretty_print_python_exception();
        Py_DECREF(m);
        throw runtime_error("cannot instantiate a Python factory class:\n" +
                            exception_msg);
    }

    Py_DECREF(m);

    PyRt* rt = 0;
    resolve(rt);
    pyctxt = rt->create_python_context(ctxt, this);

    PyObject* method = PyString_FromString("instance");
    if (!method) {
        const string exception_msg =  pretty_print_python_exception();
        throw runtime_error("cannot construct an 'instance' method name: " +
                            exception_msg);
    }

    pyobj = PyObject_CallMethodObjArgs(pyf, method, pyctxt, 0);
    if (!pyobj) {
        const string exception_msg =  pretty_print_python_exception();
        Py_DECREF(method);
        Py_DECREF(pyf);
        throw runtime_error("cannot construct a Python module '" + module+"': "+
                            exception_msg);
    }

    Py_DECREF(method);
    Py_DECREF(pyf);

    init();
}

PyComponent::PyComponent(const Context* c, PyObject* pyobjc_)
    : Component(c), pyobj(pyobjc_) {
    init();
}

void
PyComponent::init() {
    Py_INCREF(pyobj);

    PyObject* method = PyString_FromString("getInterface");
    if (!method) {
        const string exception_msg =  pretty_print_python_exception();
        throw runtime_error("cannot construct a 'getInterface' method name: " +
                            exception_msg);
    }

    PyObject* pi = PyObject_CallMethodObjArgs(pyobj, method, 0);
    if (!pi) {
        const string exception_msg =  pretty_print_python_exception();
        throw runtime_error("getInterface failed: " + exception_msg);
    }

    // If component returned a (class) type, translate it to a string.
    if (pi && PyType_Check(pi)) {
        PyObject* t = pi;
        pi = PyObject_Str(t);
        Py_DECREF(t);
    }

    if (!pi || !PyString_Check(pi)) {
        const string exception_msg =  pretty_print_python_exception();
        Py_DECREF(method);
        throw runtime_error("cannot retrieve the Python component interface "
                            "description: " + exception_msg);
    }

    Py_DECREF(method);

    char* s = PyString_AsString(pi);
    if (!s) {
        const string exception_msg =  pretty_print_python_exception();
        Py_DECREF(pi);
        throw runtime_error("cannot convert the Python component interface "
                            "description: " + exception_msg);
    }

    py_interface = string(s);
    Py_DECREF(pi);
}

PyComponent::~PyComponent() {
    Py_XDECREF(pyobj);
}

Interface_description
PyComponent::get_interface() const {
    return py_interface;
}

/* Translate the Twisted failure into a pretty printed exception
   string. Note, Twisted deferred returns a Failure object, even if
   the user didn't pass Failure object to the original errback
   call. */
static string get_twisted_failure(PyObject* failure) {
    static PyObject* method = PyString_FromString("raiseException");
    if (!method) {
        VLOG_ERR(lg, "Could not create 'raiseException' method name: %s",
                 pretty_print_python_exception().c_str());
        return "unknown error";
    }

    PyObject* ret = PyObject_CallMethodObjArgs(failure, method, 0);
    Py_XDECREF(ret);
    return pretty_print_python_exception();
}

static void complete(bool success, string* result, Co_sema* sem,
                     PyObject* failure) {
    *result = success ? "" : get_twisted_failure(failure);
    sem->up();
}

static PyObject* get_deferred_class() {
    PyObject* defer = PyImport_ImportModule("twisted.internet.defer");
    if (PyErr_Occurred()) {
        const string exception_msg = pretty_print_python_exception();
        throw runtime_error("cannot import twisted.internet.defer module: " +
                            exception_msg);
    }

    PyObject* deferred = PyObject_GetAttrString(defer, "Deferred");
    if (PyErr_Occurred()) {
        const string exception_msg = pretty_print_python_exception();
        throw runtime_error("cannot find twisted.internet.defer.Deferred "
                            "class:" + exception_msg);
    }

    return deferred;
}

void
PyComponent::configure(const Configuration* conf) {
    PyObject* kv = PyDict_New();

    // Transform the configuration object into a PyDict
    BOOST_FOREACH(string key, conf->keys()) {
        const string value = conf->get(key);
        PyObject* s = PyString_FromString(value.c_str());
        PyDict_SetItemString(kv, key.c_str(), s);
        Py_DECREF(s);
    }

    // Transform the arguments into a PyList and insert into
    // the configuration PyDict.
    PyObject* args = PyList_New(0);

    BOOST_FOREACH(Component_argument arg, conf->get_arguments()) {
        PyObject* s = PyString_FromString(arg.c_str());
        PyList_Append(args, s);
        Py_DECREF(s);
    }

    PyDict_SetItemString(kv, "arguments", args);
    Py_DECREF(args);

    Co_sema sem;
    PyObject* deferred = 0;
    PyObject* r = 0;
    PyObject* method = 0;
    string result;

    try {
        deferred = get_deferred_class();
        method = PyString_FromString("configure");

        r = PyObject_CallMethodObjArgs(pyobj, method, kv, 0);
        if (PyErr_Occurred()) {
            const string exception_msg = pretty_print_python_exception();
            throw runtime_error("cannot configure the Python component: " +
                                exception_msg);
        } else if (r == Py_None) {
            sem.up();
        } else if (PyObject_IsInstance(r, deferred)) {
            PyObject* success_cb = DeferredCallback::get_instance
                (boost::bind(&complete, true, &result, &sem,_1));
            PyObject* error_cb = DeferredCallback::get_instance
                (boost::bind(&complete, false, &result, &sem, _1));
            if (!success_cb || !error_cb) {
                Py_XDECREF(success_cb);
                Py_XDECREF(error_cb);
                throw runtime_error("cannot construct Deferred object");
            }

            bool failure =
                !DeferredCallback::add_callbacks(r, success_cb, error_cb);
            Py_DECREF(success_cb);
            Py_DECREF(error_cb);

            if (failure) {
                throw runtime_error("cannot access Deferred object");
            }
        } else {
            lg.warn("configure() did not return a Deferred object");
            sem.up();
        }

        // Wait asynchronous execution to complete
        sem.down();

        if (result.length()) {
            throw runtime_error(result);
        }

    } catch (const runtime_error& e) {
        Py_XDECREF(r);
        Py_XDECREF(method);
        Py_XDECREF(kv);
        throw;
    }

    Py_XDECREF(r);
    Py_XDECREF(method);
    Py_XDECREF(kv);
}

void
PyComponent::install() {
    Co_sema sem;
    PyObject* deferred = 0;
    PyObject* r = 0;
    PyObject* method = 0;
    string result;

    try {
        deferred = get_deferred_class();
        method = PyString_FromString("install");
        r = PyObject_CallMethodObjArgs(pyobj, method, 0);

        if (PyErr_Occurred()) {
            const string exception_msg = pretty_print_python_exception();
            throw runtime_error("cannot install the Python component: " +
                                exception_msg);
        } else if (r == Py_None) {
            sem.up();
        } else if (PyObject_IsInstance(r, deferred)) {
            PyObject* success_cb = DeferredCallback::get_instance
                (boost::bind(&complete, true, &result, &sem,_1));
            PyObject* error_cb = DeferredCallback::get_instance
                (boost::bind(&complete, false, &result, &sem, _1));
            if (!success_cb || !error_cb) {
                Py_XDECREF(success_cb);
                Py_XDECREF(error_cb);
                throw runtime_error("cannot construct Deferred object");
            }

            bool failure =
                !DeferredCallback::add_callbacks(r, success_cb, error_cb);
            Py_DECREF(success_cb);
            Py_DECREF(error_cb);

            if (failure) {
                throw runtime_error("cannot access Deferred object");
            }

        } else {
            lg.warn("install() did not return a Deferred object");
            sem.up();
        }

        // Wait asynchronous execution to complete
        sem.down();

        if (result.length()) {
            throw runtime_error(result);
        }

    } catch (const runtime_error& e) {
        Py_XDECREF(r);
        Py_XDECREF(method);
        throw;
    }

    Py_XDECREF(r);
    Py_XDECREF(method);
}

PyObject*
PyComponent::getPyObject() {
    return pyobj;
}
