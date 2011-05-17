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
#ifndef PYSTORAGE_COMMON_HH
#define PYSTORAGE_COMMON_HH 1

#include "pystorage.hh"

#include <Python.h>

#include <stdexcept>
#include <string>

#include <boost/foreach.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>

#include "pyrt/pyrt.hh"

namespace boost {

/* Functions for the intrusive_ptr to manage the Python object
   references */
inline
void intrusive_ptr_add_ref(PyObject* pobj) {
    Py_INCREF(pobj);
}

inline
void intrusive_ptr_release(PyObject* pobj) {
    Py_DECREF(pobj);
}

}

namespace vigil {
namespace applications {
namespace storage {

typedef boost::intrusive_ptr<PyObject> PyObject_ptr;

template<typename T>
T from_python(PyObject*);

template<typename T> 
PyObject* to_python(const T&);

/* Translate Python strings and unicode strings to UTF-8 strings */
template<>
inline
std::string from_python(PyObject* s) {
    if (s && PyString_Check(s)) {
        return std::string(PyString_AsString(s));
    } else if (s && PyUnicode_Check(s)) {
        PyObject* u = PyUnicode_AsUTF8String(s);
        if (!u) {
            throw std::runtime_error("Invalid string (unable to encode "
                                     "as UTF-8)");
        }
        
        std::string r(PyString_AsString(u));
        Py_DECREF(u);
        return r;
    }

    throw std::runtime_error("Invalid string");
}

template<>
inline
bool from_python(PyObject* b) {
    if (!b || !(b == Py_True || b == Py_False)) {
        throw std::runtime_error("Invalid bool");
    }

    return b == Py_True;
}

template<>
inline
PyObject* to_python(const bool& v) {
    PyObject* ret = v ? Py_True : Py_False;
    Py_INCREF(ret);
    return ret;
}

template<>
inline
int32_t from_python(PyObject* i) {
    if (!i || !PyInt_Check(i)) {
        throw std::runtime_error("Invalid int32");
    }
    
    return PyLong_AsLong(i);
}

template<>
inline
int64_t from_python(PyObject* i) {
    if (!i || !(PyLong_Check(i) || PyInt_Check(i))) {
        throw std::runtime_error("Invalid int64");
    }
    
    return PyLong_AsLongLong(i);
}

template<>
inline
PyObject* to_python(const int64_t& i) {
    return PyLong_FromLongLong(i);
}

template<>
inline
PyObject* to_python(const int32_t& i) {
    return PyInt_FromLong(i);
}

template<>
inline
double from_python(PyObject* d) {
    if (!d || !PyFloat_Check(d)) {
        throw std::runtime_error("Invalid double object");
    }
    
    return PyFloat_AsDouble(d);
}

template<>
inline
PyObject* to_python(const double& d) {
    return PyFloat_FromDouble(d);
}

template<>
inline
PyObject* to_python(const std::string& s) {
    PyObject* str = PyUnicode_DecodeUTF8(s.c_str(), s.length(), 0);
    if (!str) {
        throw std::runtime_error("Decoding an UTF-8 string to unicode string "
                                 "failed:\n" + pretty_print_python_exception());
    }
    return str;
}

template<>
inline
PyObject* to_python(const Trigger_reason& reason) {
    return PyInt_FromLong(reason);
}

template<>
inline
GUID from_python(PyObject* g_) {
    uint8_t* buffer;
    Py_ssize_t length;

    if (!PyTuple_Check(g_) || PyTuple_GET_SIZE(g_) != 2) {
        throw std::runtime_error("GUID not a 2-tuple");
    }

    PyObject* g = PyTuple_GET_ITEM(g_, 1);
    if (!PyString_Check(g) || PyString_GET_SIZE(g) != GUID::GUID_SIZE ||
        PyString_AsStringAndSize(g, (char**)&buffer, &length) == -1) {
        PyErr_Clear();
        throw std::runtime_error("GUID not a proper binary string");
    }
    
    return GUID(buffer);
}

template<>
inline
PyObject* to_python(const GUID& guid) {
    PyObject* obj = PyTuple_New(2);
    PyTuple_SET_ITEM(obj, 0, to_python<std::string>("GUID"));
    PyTuple_SET_ITEM(obj, 1, PyString_FromStringAndSize((const char*)guid.guid, 
                                                        GUID::GUID_SIZE));
    return obj;

}

template<>
inline
Reference from_python(PyObject* ref) {
    if (!ref || !PyTuple_Check(ref) || PyTuple_GET_SIZE(ref) != 3) {
        throw std::runtime_error("Invalid Reference object");
    }

    return Reference(PyInt_AsLong(PyTuple_GetItem(ref, 0)),
                     from_python<GUID>(PyTuple_GET_ITEM(ref, 1)),
                     from_python<bool>(PyTuple_GET_ITEM(ref, 2)));
}

template<>
inline
PyObject* to_python(const Reference& ref) {
    PyObject* py_ref = PyTuple_New(3);
    try {
        PyTuple_SET_ITEM(py_ref, 0, PyInt_FromLong(ref.version));
        PyTuple_SET_ITEM(py_ref, 1, to_python(ref.guid));
        PyObject* py_wildcard = 
            ref.wildcard ? Py_True : Py_False; Py_INCREF(py_wildcard);
        PyTuple_SET_ITEM(py_ref, 2, py_wildcard);
        
        return py_ref;
    }
    catch (...) {
        Py_DECREF(py_ref);
        throw;
    }
}

template<>
inline
Trigger_id from_python(PyObject* tid) {
    if (!tid || !PyTuple_Check(tid) || PyTuple_GET_SIZE(tid) != 4) {
        throw std::runtime_error("Invalid Trigger id object");
    }
    
    return Trigger_id(from_python<bool>(PyTuple_GET_ITEM(tid, 0)),
                      from_python<std::string>(PyTuple_GET_ITEM(tid, 1)),
                      from_python<Reference>(PyTuple_GET_ITEM(tid, 2)),
                      from_python<int64_t>(PyTuple_GET_ITEM(tid, 3)));
}

template<>
inline
PyObject* to_python(const Trigger_id& tid) {
    PyObject* py_tid = PyTuple_New(4);
    try {
        PyTuple_SET_ITEM(py_tid, 0, to_python(tid.for_table));
        PyTuple_SET_ITEM(py_tid, 1, to_python(tid.ring));
        PyTuple_SET_ITEM(py_tid, 2, to_python(tid.ref));
        PyTuple_SET_ITEM(py_tid, 3, to_python(tid.tid));
        
        return py_tid;
    }
    catch (...) {
        Py_DECREF(py_tid);
        throw;
    }
}

template<>
inline
Column_value_map from_python(PyObject* cv) {
    if (!cv || !PyDict_Check(cv)) {
        throw std::runtime_error("Invalid column names or values");
    }

    Column_value_map m;
    PyObject* keys = PyDict_Keys(cv);
        
    try {
        for (Py_ssize_t i = 0; i < PyList_GET_SIZE(keys); ++i) {
            PyObject* k = PyList_GET_ITEM(keys, i);
            const Column_name name = from_python<std::string>(k);
            
            PyObject* v = PyDict_GetItem(cv, k);
            Column_value value;
            if (PyString_Check(v)) {
                m[name] = from_python<std::string>(v);
            } else if (PyUnicode_Check(v)) { 
                PyObject_ptr str(PyUnicode_AsUTF8String(v), false);
                if (!str.get()) {
                    throw std::runtime_error("Encoding an unicode string to "
                                             "UTF-8 failed:\n" + 
                                             pretty_print_python_exception());
                }

                m[name] = from_python<std::string>(str.get());
            } else if (PyInt_Check(v)) { 
                m[name] = from_python<int64_t>(v);
            } else if (PyLong_Check(v)) { 
                m[name] = from_python<int64_t>(v);
            } else if (PyFloat_Check(v)) { 
                m[name] = from_python<double>(v);
            } else if (PyTuple_Check(v) && PyTuple_GET_SIZE(v) == 2) { 
                m[name] = from_python<GUID>(v); 
            } else if (PyType_Check(v)) {
                if (PyType_IsSubtype((PyTypeObject*)v, &PyString_Type)) {
                    m[name] = "";
                } else if (PyType_IsSubtype((PyTypeObject*)v, &PyInt_Type)) {
                    m[name] = (int64_t)0;
                } else if (PyType_IsSubtype((PyTypeObject*)v, &PyFloat_Type)) {
                    m[name] = (double)0;
                } else if (PyType_IsSubtype((PyTypeObject*)v, &PyTuple_Type)) {
                    m[name] = GUID();
                } else {
                    throw std::runtime_error("unknown Python type object");
                }
            } else {
                throw std::runtime_error("unknown Python column type");
            }
        }

        Py_DECREF(keys);
        return m;
    }
    catch (...) {
        Py_DECREF(keys);        
        throw;
    }
}

/* Visitor for computing a SHA-1 digest out of a set of column values. */
struct Column_value_converter
    : public boost::static_visitor<> 
{
    void operator()(const int64_t& i) const { obj = to_python(i); }
    void operator()(const std::string& s) const { obj = to_python(s); }
    void operator()(const double& d) const { obj = to_python(d); }
    void operator()(const GUID& guid) const { obj = to_python(guid); }

    mutable PyObject* obj;
};

template <>
inline
PyObject* to_python(const Column_value_map& row) {
    PyObject* r = PyDict_New();
    try {
        Column_value_converter converter;
        
        BOOST_FOREACH(Column_value_map::value_type v, row) {
            boost::apply_visitor(converter, v.second);
            PyObject* key = to_python(v.first);
            PyObject* value = converter.obj;
            PyDict_SetItem(r, key, value); Py_DECREF(key); Py_DECREF(value);
        }

        return r;
    }
    catch (...) {
        Py_DECREF(r);
        throw;
    }
}

template<>
inline
Index_list from_python(PyObject* indices) {
    if (!indices || !PyTuple_Check(indices)) {
        throw std::runtime_error("Indices not defined as a tuple.");
    }

    Index_list r;
    for (Py_ssize_t j = 0; j < PyTuple_GET_SIZE(indices); ++j) {
        PyObject* index = PyTuple_GET_ITEM(indices, j);
        if (!index || !PyTuple_Check(index) || PyTuple_GET_SIZE(index) != 2) {
            throw std::runtime_error("Index not defined as a tuple or "
                                     "has wrong length");
        }

        Index i;
        i.name = from_python<std::string>(PyTuple_GET_ITEM(index, 0));
        PyObject* columns = PyTuple_GET_ITEM(index, 1);
        if (!columns || !PyTuple_Check(columns)) {
            throw std::runtime_error("Index columns not defined as a tuple");
        }

        for (Py_ssize_t k = 0; k < PyTuple_GET_SIZE(columns); ++k) {
            Column_name n = 
                from_python<std::string>(PyTuple_GET_ITEM(columns, k));
            i.columns.push_back(n);
        }

        r.push_back(i);
    }
        
    return r;
}

/* Convert the Python storage context to C++ context */
template<>
inline
Context from_python(PyObject* ctxt) {
    if (!ctxt || !PyTuple_Check(ctxt) || PyTuple_GET_SIZE(ctxt) != 5) {
        throw std::runtime_error("Invalid Context object");
    }

    Table_name table = from_python<Table_name>(PyTuple_GET_ITEM(ctxt, 0));
    Index_name index = from_python<Index_name>(PyTuple_GET_ITEM(ctxt, 1));
    Reference index_row = from_python<Reference>(PyTuple_GET_ITEM(ctxt, 2));
    Reference initial_row = from_python<Reference>(PyTuple_GET_ITEM(ctxt, 3));
    Reference current_row = from_python<Reference>(PyTuple_GET_ITEM(ctxt, 4));
    
    Context c(table);
    c.table = table;
    c.index = index;
    c.index_row = index_row;
    c.initial_row = initial_row;
    c.current_row = current_row;
    return c;
}

template<>
inline
PyObject* to_python(const Context& ctxt)  {
    PyObject* py_ctxt = PyTuple_New(5);
    
    try {
        PyTuple_SET_ITEM(py_ctxt, 0, to_python(ctxt.table));
        PyTuple_SET_ITEM(py_ctxt, 1, to_python(ctxt.index));
        PyTuple_SET_ITEM(py_ctxt, 2, to_python(ctxt.index_row));
        PyTuple_SET_ITEM(py_ctxt, 3, to_python(ctxt.initial_row));
        PyTuple_SET_ITEM(py_ctxt, 4, to_python(ctxt.current_row));
        
        return py_ctxt;
    }
    catch (...) {
        Py_DECREF(py_ctxt);
        throw;
    }
}

/* Convert the C++ result to Python result */
template<>
inline
PyObject* to_python(const Result& result) {
    PyObject* py_result = PyTuple_New(2);

    try {
        PyTuple_SET_ITEM(py_result, 0, PyInt_FromLong(result.code));
        PyTuple_SET_ITEM(py_result, 1, to_python(result.message));
        
        return py_result;
    }
    catch (...) {
        Py_DECREF(py_result);
        throw;
    }
}

}
}
}

#endif /* PYSTORAGE_COMMON_HH */
