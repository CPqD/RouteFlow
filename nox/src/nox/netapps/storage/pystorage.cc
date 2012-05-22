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
#include "pystorage.hh"

#include <Python.h>

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>

#include "pyrt/pycontext.hh"
#include "pystorage-common.hh"
#include "threads/cooperative.hh"
#include "vlog.hh"

using namespace std;
using namespace vigil;
using namespace vigil::applications;
using namespace vigil::applications::storage;

static Vlog_module lg("pystorage");

PyStorage::PyStorage(PyObject* ctxt)
    : storage(0) {
    if (!SWIG_Python_GetSwigThis(ctxt) || !SWIG_Python_GetSwigThis(ctxt)->ptr) {
        throw runtime_error("Unable to access Python context.");
    }
    
    c = ((PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr)->c;
}
                    
void
PyStorage::configure(PyObject*) {
    c->resolve(storage);
}

void 
PyStorage::install(PyObject*) {
    /* Nothing here */
}

static void python_callback(PyObject* args, PyObject_ptr cb) {
    Co_critical_section c;
    PyObject* ret = PyObject_CallObject(cb.get(), args);
    if (ret == 0) {
        const string exc = pretty_print_python_exception();
        lg.err("Python callback invocation failed:\n%s", exc.c_str());
    }

    Py_DECREF(args);
    Py_XDECREF(ret);
}

static void callback(const Result& result, PyObject_ptr cb) {
    PyObject* py_result = 0;
    try {
        py_result = to_python(result);
    }
    catch (const runtime_error& e) {
        py_result = to_python(Result(Result::UNKNOWN_ERROR, e.what()));
    }

    PyObject* t = PyTuple_New(1);
    PyTuple_SET_ITEM(t, 0, py_result);
        
    python_callback(t, cb);
}

static PyObject_ptr check_callback(PyObject* cb) {
    if (!cb || !PyCallable_Check(cb)) { 
        throw "Invalid callback"; 
    }

    return PyObject_ptr(cb, true);
}

PyObject* 
PyStorage::create_table(PyObject* table, PyObject* columns, PyObject* indices,
                        PyObject* cb) {
    try {
        PyObject_ptr cptr = check_callback(cb);
        const Table_name t = from_python<Table_name>(table);
        const Column_definition_map c = 
            from_python<Column_definition_map>(columns);
        const Index_list i = from_python<Index_list>(indices);

        Async_storage::Create_table_callback f = boost::bind(&callback,_1,cptr);
        storage->create_table(t, c, i, f);
        Py_RETURN_NONE;
    }
    catch (const runtime_error& e) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, e.what());
        return 0;
    }
}

PyObject* 
PyStorage::drop_table(PyObject* table, PyObject* cb) {
    try {
        PyObject_ptr cptr = check_callback(cb);
        const Table_name t = from_python<Table_name>(table);

        Async_storage::Drop_table_callback f = boost::bind(&callback, _1, cptr);
        storage->drop_table(t, f);
        Py_RETURN_NONE;
    }
    catch (const runtime_error& e) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, e.what());
        return 0;
    }
}

static void get_callback(const Result& result, const Context& ctxt, 
                         const Row& row, PyObject_ptr cb) {
    PyObject* t = PyTuple_New(3);

    try {
        PyTuple_SET_ITEM(t, 0, to_python(result));
        PyTuple_SET_ITEM(t, 1, to_python(ctxt));
        PyTuple_SET_ITEM(t, 2, to_python(row));
    }
    catch (const runtime_error& e) {
        Py_DECREF(t);

        t = PyTuple_New(3);
        PyTuple_SET_ITEM(t, 0, 
                         to_python(Result(Result::UNKNOWN_ERROR, e.what())));
        PyTuple_SET_ITEM(t, 1, to_python(Context()));
        PyTuple_SET_ITEM(t, 2, to_python(Row()));
    }

    python_callback(t, cb);
}

PyObject*
PyStorage::get(PyObject* table, PyObject* query, PyObject* cb) {
    try {
        PyObject_ptr cptr = check_callback(cb);
        const Table_name t = from_python<Table_name>(table);
        const Query q = from_python<Query>(query);

        Async_storage::Get_callback f = 
            boost::bind(&get_callback, _1, _2, _3, cptr);
        storage->get(t, q, f);
        Py_RETURN_NONE;
    }
    catch (const runtime_error& e) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, e.what());
        return 0;
    }
}

PyObject*
PyStorage::get_next(PyObject* context, PyObject* cb) {
    try {
        const Context c = from_python<Context>(context);
        PyObject_ptr cptr = check_callback(cb);

        Async_storage::Get_callback f = 
            boost::bind(&get_callback, _1, _2, _3, cptr);
        storage->get_next(c, f);
        Py_RETURN_NONE;
    }
    catch (const runtime_error& e) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, e.what());
        return 0;
    }
}

static void put_callback(const Result& result, const GUID& guid, 
                         PyObject_ptr cb) {
    PyObject* t = PyTuple_New(2);

    try {
        PyTuple_SET_ITEM(t, 0, to_python(result));
        PyTuple_SET_ITEM(t, 1, to_python(guid));
    }
    catch (const runtime_error& e) {
        Py_DECREF(t);

        t = PyTuple_New(2);
        PyTuple_SET_ITEM(t, 0, 
                         to_python(Result(Result::UNKNOWN_ERROR, e.what())));
        PyTuple_SET_ITEM(t, 1, to_python(GUID()));
    }

    python_callback(t, cb);
}

PyObject*
PyStorage::put(PyObject* table, PyObject* row, PyObject* cb) {
    try {
        const Table_name t = from_python<Table_name>(table);
        const Row r = from_python<Row>(row);
        PyObject_ptr cptr = check_callback(cb);

        Async_storage::Put_callback f = boost::bind(&put_callback, _1, _2,cptr);
        storage->put(t, r, f);
        Py_RETURN_NONE;
    }
    catch (const runtime_error& e) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, e.what());
        return 0;
    }
}

static void modify_callback(const Result& result, const Context& ctxt,
                            PyObject_ptr cb) {
    PyObject* t = PyTuple_New(2);

    try {
        PyTuple_SET_ITEM(t, 0, to_python(result));
        PyTuple_SET_ITEM(t, 1, to_python(ctxt));
    }
    catch (const runtime_error& e) {
        Py_DECREF(t);

        t = PyTuple_New(2);
        PyTuple_SET_ITEM(t, 0, 
                         to_python(Result(Result::UNKNOWN_ERROR, e.what())));
        PyTuple_SET_ITEM(t, 1, to_python(Context()));
    }

    python_callback(t, cb);
}

PyObject*
PyStorage::modify(PyObject* context, PyObject* row, PyObject* cb) {
    try {
        const Context c = from_python<Context>(context);
        const Row r = from_python<Row>(row);
        PyObject_ptr cptr = check_callback(cb);

        Async_storage::Modify_callback f =  
            boost::bind(&modify_callback, _1, _2, cptr);
        storage->modify(c, r, f);
        Py_RETURN_NONE;
    }
    catch (const runtime_error& e) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, e.what());
        return 0;
    }
}

PyObject*
PyStorage::remove(PyObject* context, PyObject* cb) {
    try {
        const Context c = from_python<Context>(context);
        PyObject_ptr cptr = check_callback(cb);

        Async_storage::Remove_callback f = boost::bind(&callback, _1, cptr);
        storage->remove(c, f);
        Py_RETURN_NONE;
    }
    catch (const runtime_error& e) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, e.what());
        return 0;
    }
}

static void put_trigger_callback(const Result& result, const Trigger_id& tid,
                                 PyObject_ptr cb) {
    PyObject* t = PyTuple_New(2);

    try {
        PyTuple_SET_ITEM(t, 0, to_python(result));
        PyTuple_SET_ITEM(t, 1, to_python(tid));
    }
    catch (const runtime_error& e) {
        Py_DECREF(t);

        t = PyTuple_New(2);
        PyTuple_SET_ITEM(t, 0, 
                         to_python(Result(Result::UNKNOWN_ERROR, e.what())));
        PyTuple_SET_ITEM(t, 1, to_python(Trigger_id()));
    }

    python_callback(t, cb);
}

static void trigger_callback(const Trigger_id& tid, const Row& row, 
                             const Trigger_reason reason,
                             PyObject_ptr trigger_func) {
    PyObject* t = PyTuple_New(3);

    try {
        PyTuple_SET_ITEM(t, 0, to_python(tid));
        PyTuple_SET_ITEM(t, 1, to_python(row));
        PyTuple_SET_ITEM(t, 2, to_python(reason));
        
        python_callback(t, trigger_func);
    }
    catch (const runtime_error& e) {
        Py_DECREF(t);
        
        lg.err("Unable to invoke a trigger: %s", e.what());
    }
}

PyObject*
PyStorage::put_row_trigger(PyObject* context, PyObject* trigger_func, 
                           PyObject* cb) {
    try {
        const Context c = from_python<Context>(context);
        PyObject_ptr cptr = check_callback(cb);
        PyObject_ptr tptr(trigger_func, true);
        Trigger_function t = boost::bind(&trigger_callback, _1, _2, _3, tptr);

        Async_storage::Put_trigger_callback f = 
            boost::bind(&put_trigger_callback, _1, _2, cptr);
        storage->put_trigger(c, t, f);
        Py_RETURN_NONE;
    }
    catch (const runtime_error& e) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, e.what());
        return 0;
    }
}

PyObject*
PyStorage::put_table_trigger(PyObject* table, PyObject* sticky, 
                             PyObject* trigger_func, PyObject* cb) {
    try {
        const Table_name t = from_python<Table_name>(table);
        const bool s = from_python<bool>(sticky);

        PyObject_ptr tptr(trigger_func, true);
        Trigger_function tf = boost::bind(&trigger_callback, _1, _2, _3, tptr);

        PyObject_ptr cptr = check_callback(cb);
        Async_storage::Put_trigger_callback f = 
            boost::bind(&put_trigger_callback, _1, _2, cptr);
        storage->put_trigger(t, s, tf, f);
        Py_RETURN_NONE;
    }
    catch (const runtime_error& e) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, e.what());
        return 0;
    }
}

PyObject*
PyStorage::remove_trigger(PyObject* tid, PyObject* cb) {
    try {
        const Trigger_id t = from_python<Trigger_id>(tid);

        PyObject_ptr cptr = check_callback(cb);
        Async_storage::Remove_trigger_callback f = 
            boost::bind(&callback, _1, cptr);
        storage->remove_trigger(t, f);
        Py_RETURN_NONE;
    }
    catch (const runtime_error& e) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, e.what());
        return 0;
    }
}
