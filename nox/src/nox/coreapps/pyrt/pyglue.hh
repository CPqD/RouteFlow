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
//-----------------------------------------------------------------------------
// Methods to convert C++ objects to python objects.  These are used to
// add attributes to C++ events which are passed up to python.
// Specializations for to_python<> should be declared here.
//-----------------------------------------------------------------------------

#ifndef PY_GLUE_HH
#define PY_GLUE_HH 1

#include "config.h"

#include <iostream>
#ifdef TWISTED_ENABLED

#include <Python.h>

#include <string>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <map>

#include "hash_set.hh"
#include "pyrt.hh"
#include "threads/cooperative.hh"
#include "vlog.hh"

//-----------------------------------------------------------------------------
//  Used by intrusive_ptr<T> to manage python callback lifetimes
//-----------------------------------------------------------------------------

struct ofp_flow_mod;
struct ofp_flow_stats;
struct ofp_match;

namespace boost
{

inline
void intrusive_ptr_add_ref(PyObject* pobj) {
    Py_INCREF(pobj);
}

inline
void intrusive_ptr_release(PyObject* pobj) {
    Py_DECREF(pobj);
}
}

namespace vigil
{

struct ethernetaddr;
struct ipaddr;
class Buffer;
class Port;
class Table_stats;
class Port_stats;
class datapathid;
struct Flow;
struct Flow_stats;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/* Note, this throws a runtime_error if the attribute does not
   exist. */
void pyglue_setattr_string(PyObject* src,
                           const std::string& astr, PyObject* attr);
/* Note, this throws a bad_alloc exception if the key can't be
   allocated. */
void pyglue_setdict_string(PyObject* dict,
                           const std::string& key_, PyObject* value);

//-----------------------------------------------------------------------------
// Convert c++ types to python objects
//-----------------------------------------------------------------------------

/* Creates new reference! */
template <typename T>
PyObject* to_python(const T& type ) ;

template <>
inline
PyObject*
to_python(const unsigned char& type )
{
    return PyInt_FromLong((int)type);
}

template <> 
inline
PyObject* 
to_python(const unsigned short& type ) 
{
    return PyInt_FromLong((int)type);
}

template <>
inline
PyObject*
to_python(const unsigned int& type )
{
    return PyLong_FromUnsignedLongLong((unsigned long long)type);
}

template <> 
inline
PyObject* 
to_python(const unsigned long& type ) 
{
  return PyLong_FromUnsignedLongLong((unsigned long long)type);
}

template <> 
inline
PyObject* 
to_python(const unsigned long long& type ) 
{
  return PyLong_FromUnsignedLongLong((unsigned long long)type);
}

template <>
inline
PyObject*
to_python(const char& type )
{
    return PyInt_FromLong((int)type);
}

template <> 
inline
PyObject* 
to_python(const short& type ) 
{
    return PyInt_FromLong((int)type);
}

template <> 
inline
PyObject* 
to_python(const int& type ) 
{
    return PyLong_FromLong(type);
}

template <> 
inline
PyObject* 
to_python(const long& type ) 
{
    return PyLong_FromLong(type);
}

template <> 
inline
PyObject* 
to_python(const long long& type ) 
{
    return PyLong_FromLongLong(type);
}

template <> 
inline
PyObject* 
to_python(const double& type ) 
{
    return PyFloat_FromDouble(type);
}

template <>
inline
PyObject*
to_python(const bool& b)
{
    return PyBool_FromLong(b);
}

template <>
inline
PyObject*
to_python(const std::string& str)
{
    PyObject* ret = PyUnicode_DecodeUTF8(str.c_str(), str.size(), NULL);
    if (!ret) {
        PyErr_SetString(PyExc_ValueError,
                "Failed to decode string as utf-8");
        return NULL;
    }
    return ret;
}

template <>
PyObject*
to_python(const Buffer& b);

template <>
PyObject*
to_python(const boost::shared_ptr<Buffer>& p);

template <>
PyObject*
to_python(const Port& p);

template <>
PyObject*
to_python(const Table_stats& p);

template <>
PyObject*
to_python(const Port_stats& p);

template <>
PyObject*
to_python(const ofp_flow_mod& m);

template <>
PyObject*
to_python(const ofp_flow_stats&);

template <>
PyObject*
to_python(const Flow_stats&);

template <>
PyObject*
to_python(const ofp_match& m);

template <>
PyObject*
to_python(const std::vector<Port>& p);

template <>
PyObject*
to_python(const std::vector<Table_stats>& p);

template <>
PyObject*
to_python(const std::vector<Port_stats>& p);

template <>
PyObject*
to_python(const ethernetaddr&);

template <>
PyObject*
to_python(const ipaddr&);

template <>
PyObject*
to_python(const datapathid&);

template <>
PyObject*
to_python(const vigil::Flow&);

template <class A, class B> 
inline 
PyObject*
to_python(const std::pair<A,B> p) {

  PyObject *t = PyTuple_New(2); 
  if(!t) return 0;
  PyObject *a = to_python(p.first);
  PyObject *b = to_python(p.second);
  if (!a || !b) {
      Py_DECREF(t);
      Py_XDECREF(a);
      Py_XDECREF(b);
      return 0;
  }
  PyTuple_SetItem(t,0,a);
  PyTuple_SetItem(t,1,b);
  return t; 
} 

/**
 * Converts a STL sequence container into a Python list.
 */
template <typename T, 
          template <typename, 
                    typename = std::allocator<T> > class Cont>
inline
PyObject* 
to_python_list(const Cont<T>& q) {
    using namespace std;
    
    PyObject *l = PyList_New(0);
    if (!l) {
        return 0;
    }

    BOOST_FOREACH (const typename Cont<T>::value_type& i, q) {
        PyObject *op = to_python(i);
        if (!op) {
            Py_DECREF(l);
            return 0;
        }

        PyList_Append(l, op);
        Py_DECREF(op);
    }

    return l;    
}

/**
 * Converts a hash_set into a Python list.
 */
template <typename T>
inline
PyObject*
to_python_list(const hash_set<T>& set)
{
    PyObject *l = PyList_New(set.size());
    if (!l) {
        return 0;
    }

    typename hash_set<T>::const_iterator elem(set.begin());
    for (uint32_t i = 0; elem != set.end(); ++i, ++elem) {
        PyObject *e = to_python(*elem);
        if (!e) {
            Py_DECREF(l);
            return 0;
        }
        if (PyList_SetItem(l, i, e) != 0) {
            Py_DECREF(e);
            Py_DECREF(l);
            return 0;
        }
    }
    return l;
}

/**
 * Converts a STL sequence container into a Python tuple.
 */
template <typename T, 
          template <typename, 
                    typename = std::allocator<T> > class Cont>
inline
PyObject*
to_python_tuple(const Cont<T>& q) {
    using namespace std;
    PyObject *tuple = PyTuple_New(q.size());
    if (!tuple) {
        return 0;
    }
    size_t pos = 0;
    BOOST_FOREACH (const typename Cont<T>::value_type& i, q) {
        PyObject *op = to_python(i);
        if (!op) {
            Py_DECREF(tuple);
            return 0;
        }
        PyTuple_SET_ITEM(tuple, pos++, op); /* Steals reference. */
    }
    return tuple;
}

template <typename T>
T from_python(PyObject*);

//-----------------------------------------------------------------------------
//   Call Python functions from C++ and pass in args
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
template <typename T>
inline
void    
call_python_function (boost::intrusive_ptr<PyObject> callable, T arg)
{
    Co_critical_section in_critical_section;

    PyObject* carg    = to_python<T>(arg);
    PyObject* pyret   = 0; 
    PyObject* pyargs  = PyTuple_New(1);

    assert(carg);
    assert(pyargs);

    PyTuple_SetItem(pyargs, 0, carg);
    pyret = PyObject_CallObject(callable.get(), pyargs);

    Py_DECREF(pyargs);
    Py_XDECREF(pyret);
    if (!pyret) {
        vlog().
            log(vlog().get_module_val("pyrt"), Vlog::LEVEL_ERR, 
                "unable to call a Python function/method:\n%s",
                vigil::applications::pretty_print_python_exception().c_str());
    }
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
template <>
void
call_python_function(boost::intrusive_ptr<PyObject> callable,
                     boost::intrusive_ptr<PyObject> args);

} // namespace vigil


#endif // TWISTED_ENABLED

#endif // PY_GLUE_HH
