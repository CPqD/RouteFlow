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
// Provides the interface between the C++ event loop and twisted
// python's event loop.  oxidereactor is exported to python and replaces
// twisted'd default reactor.
//
// Calls from twisted to add/remove file descriptors waiting on events
// are relayed to the C++ event loop which must register the appropriate
// callback functions for those actions.
//-----------------------------------------------------------------------------

#ifndef TWISTED_GLUE_HH
#define TWISTED_GLUE_HH

#include <deque>
#include <boost/bind.hpp>

#include "config.h"
#include "component.hh"
#include "hash_map.hh"
#include "pyglue.hh"
#include "vlog.hh"

extern "C"
{
#include <Python.h>
}

namespace vigil {
namespace applications {

class delayedcall
    : public std::unary_function<void, void>
{
public:
    void cancel();

    void reset(long, long);

    void delay(bool, long, long);

    Timer timer;    
};

class oxidereactor
{
public:
    oxidereactor(PyObject*, PyObject*);
    ~oxidereactor();

    void configure(PyObject*);

    void install(PyObject*);

    // These functions must be exposed to Python, so we retain the
    // Python naming convection (mixed case).
    PyObject* addReader(int, PyObject*, PyObject*);
    PyObject* addWriter(int, PyObject*, PyObject*);
    PyObject* removeReader(int);
    PyObject* removeWriter(int);

    // Timer management
    PyObject* callLater(long, long, PyObject*);

    // Resolve
    PyObject* resolve(PyObject*, PyObject*);

private:
    void blocking_resolve(const char* name,
			  const boost::intrusive_ptr<PyObject>&);

    container::Component* c;
};

class vigillog 
{
public:
    vigillog();
    PyObject* fatal(int, PyObject*);
    PyObject* warn(int, PyObject*);
    PyObject* err(int, PyObject*);
    PyObject* info(int, PyObject*);
    PyObject* dbg(int, PyObject*);
    PyObject* mod_init(PyObject*);
    bool is_emer_enabled(int);
    bool is_err_enabled(int);
    bool is_warn_enabled(int);
    bool is_info_enabled(int);
    bool is_dbg_enabled(int);

private:
#ifdef LOG4CXX_ENABLED
    typedef hash_map<int, Vlog_module*> Logger_map;
    Logger_map loggers;

    int mod_id;
#endif
};

} // namespace applications
} // namespace vigil

#endif  //-- TWISTED_GLUE_HH
