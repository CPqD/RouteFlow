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
#include "deferredcallback.hh"

#include "pyrt.hh"

#ifndef SWIG
#include "swigpyrun.h"
#endif // SWIG

#include "vlog.hh"

using namespace vigil;
using namespace vigil::applications;
using namespace std;

static Vlog_module lg("DeferredCallback");

DeferredCallback::DeferredCallback(const Callback& cb_)
  : cb(cb_) {
}

DeferredCallback::DeferredCallback() {

}

DeferredCallback::~DeferredCallback() {

}

void*
DeferredCallback::operator()(PyObject *result) {
    cb(result);
    return 0;
}

PyObject*
DeferredCallback::get_instance(const Callback& c)
{
    static PyObject* m = PyImport_ImportModule("nox.coreapps.pyrt.deferredcallback");
    if (!m) {
        lg.err("Could not retrieve NOX Python Deferred_callback module: %s",
               pretty_print_python_exception().c_str());
        return 0;
    }

    static swig_type_info* s =
        SWIG_TypeQuery("_p_vigil__applications__DeferredCallback");
    if (!s) {
        lg.err("Could not find DeferredCallback SWIG type_info");
        return 0;
    }

    DeferredCallback* cb = new DeferredCallback(c);

    // flag as used in *_wrap.cc....correct?
    return SWIG_Python_NewPointerObj(cb, s, SWIG_POINTER_OWN | 0);
}

bool
DeferredCallback::add_callbacks(PyObject *deferred, PyObject *cb, PyObject *ecb)
{
    static PyObject* method = PyString_FromString("addCallbacks");
    if (!method) {
        VLOG_ERR(lg, "Could not create add_callbacks method name: %s",
                 pretty_print_python_exception().c_str());
        return false;
    }

    PyObject* ret = PyObject_CallMethodObjArgs(deferred, method, cb, ecb, 0);
    Py_XDECREF(ret);
    if (PyErr_Occurred()) {
        const string exc = pretty_print_python_exception();
        VLOG_ERR(lg, "Could not call add_callbacks: %s", exc.c_str());

        return false;
    }
    return true;
}

bool
DeferredCallback::add_callback(PyObject* deferred, PyObject* cb) {
    PyObject *ecb = Py_None;
    Py_INCREF(ecb);
    bool result = add_callbacks(deferred, cb, ecb);
    Py_DECREF(ecb);
    return result;
}

bool
DeferredCallback::add_errback(PyObject* deferred, PyObject* ecb) {
    PyObject *cb = Py_None;
    Py_INCREF(cb);
    bool result = add_callbacks(deferred, cb, ecb);
    Py_DECREF(cb);
    return result;
}
