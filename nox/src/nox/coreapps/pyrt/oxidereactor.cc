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
#include "oxidereactor.hh"

#include <assert.h>
#include <iostream>
#include <poll.h>
#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/intrusive_ptr.hpp>

#include "netinet++/ipaddr.hh"
#include "pycontext.hh"
#include "resolver.hh"
#include "threads/cooperative.hh"

using namespace std;
using namespace vigil;
using namespace vigil::applications;

static Vlog_module lg("reactor");

typedef boost::function<void()> voidcb;
typedef vigil::hash_map<int, voidcb> Fd_callback_map;
enum {
    FD_READER,
    FD_WRITER,
    N_FD_TYPES
};

#define NOT_REACHED() abort()

// TODO: move static's into the class

/* Callbacks requested for each type of event on a FD.  These are "sticky":
 * Python code has to explicitly call in to disable them. */
static Fd_callback_map fd_callbacks[N_FD_TYPES];

/* Called from the Python interpreter? */
static bool in_python;

struct FdEvent {
    int fd;
    int revents;
};

/* Callback maintenance. */
static co_thread* pycb_thread;  /* Callback maintenance FSM. */
static bool in_pycb_thread = true; /* Is the FSM running? Initial
                                      value of true guarantees the
                                      Python runtime initialization
                                      doesn't try to re-enter Python
                                      if module importing causes
                                      reactor methods being called. */
static FdEvent* events;         /* Set of events that pycb_thread waits for. */
static size_t n_events;         /* Elements in use in events[]. */
static size_t max_events;       /* Elements allocated in events[]. */

void
delayedcall::cancel() {
    timer.cancel();
}

void
delayedcall::delay(bool neg, long sec, long usec) {
    timer.delay(neg, sec, usec);
}

void
delayedcall::reset(long sec, long usec) {
    timer.reset(sec, usec);
}

static void
do_callback(int fd, unsigned int type)
{
    assert(type < N_FD_TYPES);
    Fd_callback_map::iterator i = fd_callbacks[type].find(fd);
    if (i != fd_callbacks[type].end()) {
        i->second();
    }
}

static void
run_python_callbacks()
{
    in_pycb_thread = true;

    /* If we're not being called from Python, execute any pending callbacks.
     * (If we're being called from Python, we can't, because the Python
     * interpreter is not reentrant.  That's OK, because FD events are
     * "sticky": poll() will report the events again next time around.) */
    if (!in_python) {
        for (size_t i = 0; i < n_events; ++i) {
            FdEvent& e = events[i];
            if (e.revents) {
                /* The callbacks can attempt to re-enter the FSM thread by
                 * calling addReader, etc., but revise_events() will silently
                 * drop these attempts.  That's OK, because we're going to
                 * re-create the set of events anyhow before we return. */
                if (e.revents & (POLLERR | POLLHUP)) {
                    do_callback(e.fd, FD_READER);
                    do_callback(e.fd, FD_WRITER);
                } else if (e.revents & POLLIN) {
                    do_callback(e.fd, FD_READER);
                } else if (e.revents & POLLOUT) {
                    do_callback(e.fd, FD_WRITER);
                } else {
                    NOT_REACHED();
                }
            }
        }
    }

    n_events = 0;
    for (size_t i = 0; i < N_FD_TYPES; ++i) {
        n_events += fd_callbacks[i].size();
    }

    if (n_events > max_events) {
        max_events = n_events;
        delete[] events;
        events = new FdEvent[max_events];
    }

    n_events = 0;
    for (size_t i = 0; i < N_FD_TYPES; ++i) {
        co_fd_wait_type type
            = i == FD_READER ? CO_FD_WAIT_READ : CO_FD_WAIT_WRITE;
        BOOST_FOREACH (Fd_callback_map::value_type j, fd_callbacks[i]) {
            int fd = j.first;
            FdEvent& e = events[n_events++];
            e.fd = fd;
            co_fd_wait(fd, type, &e.revents);
        }
    }
    co_fsm_block();
    in_pycb_thread = false;
}

static void
revise_events() 
{
    assert(!in_python);
    if (!in_pycb_thread) {
        in_python = true;
        co_fsm_run(pycb_thread);
        in_python = false;
    }
}

oxidereactor::oxidereactor(PyObject* ctxt, PyObject *name) {
    if (!SWIG_Python_GetSwigThis(ctxt) || !SWIG_Python_GetSwigThis(ctxt)->ptr) {
        throw runtime_error("Unable to access Python context.");
    }
    
    c = ((PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr)->c;

    pycb_thread = co_fsm_create(&co_group_coop, run_python_callbacks);
}

void
oxidereactor::configure(PyObject* configuration) {

}

void 
oxidereactor::install(PyObject*) {
    
}

static PyObject*
add_callback(int fd, PyObject* blob, PyObject* f, unsigned int type)
{
    assert(type < N_FD_TYPES);

    PyObject *args;
    if (!f || !PyCallable_Check(f)) {
        PyErr_SetString(PyExc_TypeError, "'f' object is not callable");
        return 0;
    }

    args = PyTuple_New(1);
    // Grabs ownership of Blob.
    PyTuple_SetItem(args, 0, blob);
    Py_INCREF(blob);

    boost::intrusive_ptr<PyObject> cptr(f, true);
    boost::intrusive_ptr<PyObject> aptr(args, false);

    boost::function<void ()> c = 
        boost::bind(call_python_function<boost::intrusive_ptr<PyObject> >, 
                    cptr, aptr);

    fd_callbacks[type][fd] = c;
    revise_events();

    Py_RETURN_NONE;
}

static void
remove_callback(int fd, unsigned int type)
{
    if (fd_callbacks[type].erase(fd)) {
        revise_events();
    }
}

PyObject*
oxidereactor::addReader(int fd, PyObject* blob, PyObject* callback)
{
    return add_callback(fd, blob, callback, FD_READER);
}

PyObject*
oxidereactor::removeReader(int fd)
{
    remove_callback(fd, FD_READER);
    Py_RETURN_NONE;
}

PyObject*
oxidereactor::addWriter(int fd, PyObject* blob, PyObject* callback)
{
    add_callback(fd, blob, callback, FD_WRITER);
    Py_RETURN_NONE;
}

PyObject*
oxidereactor::removeWriter(int fd)
{
    remove_callback(fd, FD_WRITER);
    Py_RETURN_NONE;
}

PyObject*
oxidereactor::callLater(long delay_secs, long delay_usecs, 
                        PyObject* pydelayedcall) 
{
    if (!pydelayedcall || !PyCallable_Check(pydelayedcall)) {
        PyErr_SetString(PyExc_TypeError, "'f' object is not callable");
        return 0;
    }
    
    PyObject* pdc = PyObject_GetAttrString(pydelayedcall, (char*)"dc");
    Py_DECREF(pdc); // pdc is a new reference.
    delayedcall* dc = (delayedcall*)(SWIG_Python_GetSwigThis(pdc))->ptr;
    const timeval tv = {delay_secs, delay_usecs};
    {
        boost::intrusive_ptr<PyObject> cptr(pydelayedcall, true);
        boost::intrusive_ptr<PyObject> aptr(0, false);
        
        container::Component::Timer_Callback cb = 
            boost::bind(call_python_function<
                        boost::intrusive_ptr<PyObject> >, cptr, aptr);
        dc->timer = c->post(cb, tv);
    }   

    Py_RETURN_NONE;
}

void
oxidereactor::blocking_resolve(const char* name,
			       const boost::intrusive_ptr<PyObject>& cb) {
    ipaddr addr;
    string resolved_address;

    if (get_host_by_name(name, addr) == 0) {
        addr.fill_string(resolved_address);
    }

    PyObject* t = PyTuple_New(1);
    PyTuple_SET_ITEM(t, 0, PyString_FromString(resolved_address.c_str()));

    boost::intrusive_ptr<PyObject> aptr(t, false);
    container::Component::Timer_Callback f = 
      boost::bind(call_python_function<
		  boost::intrusive_ptr<PyObject> >, cb, aptr);

    const timeval tv = { 0, 0 };
    c->post(f, tv);
}


PyObject*
oxidereactor::resolve(PyObject* name, PyObject* callback) {
    const timeval tv = { 0, 0 };
    container::Component::Timer_Callback f = 
      boost::bind(&oxidereactor::blocking_resolve, this,
		  PyString_AsString(name),
		  boost::intrusive_ptr<PyObject>(callback, true));
    c->post(f, tv);

    Py_RETURN_NONE;
}

oxidereactor::~oxidereactor()
{
    // Nothing here...
}

// copied from WaitForThreadShutdown() in python source (Modules/main.c)
//void
//oxidereactor::finalize_python()
//{
    // XXX FIXME Currently causing a SEGFAULT

    // PyObject *result;
    // PyThreadState *tstate = PyThreadState_GET();
    // PyObject *threading = PyMapping_GetItemString(tstate->interp->modules,
    //         (char*)"threading");
    // if (threading == NULL) {
    //     /* threading not imported */
    //     PyErr_Clear();
    //     return;
    // }
    // result = PyObject_CallMethod(threading, (char*)"_shutdown", (char*)"");
    // if (result == NULL){
    //     PyErr_WriteUnraisable(threading);
    // } else{
    //     Py_DECREF(result);
    // }
    // Py_DECREF(threading);

    // Py_BEGIN_ALLOW_THREADS
    // Py_Finalize();
    // Py_END_ALLOW_THREADS
//}

vigillog::vigillog() 
#ifdef LOG4CXX_ENABLED
    : mod_id(0)
#endif
{
}

PyObject*
vigillog::fatal(int mod, PyObject* msg)
{   
#ifdef LOG4CXX_ENABLED
    Logger_map::iterator i = loggers.find(mod);
    if (i == loggers.end()) {
        lg.err("Invalid mod id (%d) received from Python, can't log %s", mod,
               PyString_AsString(msg));
    } else {
        i->second->emer("%s", PyString_AsString(msg));
    }
#else
    vlog().output(mod, Vlog::LEVEL_EMER, PyString_AsString(msg));
#endif
    Py_RETURN_NONE;
}


PyObject*
vigillog::err(int mod, PyObject* msg)
{   
#ifdef LOG4CXX_ENABLED
    Logger_map::iterator i = loggers.find(mod);
    if (i == loggers.end()) {
        lg.err("Invalid mod id (%d) received from Python, can't log %s", mod,
               PyString_AsString(msg));
    } else {
        i->second->err("%s", PyString_AsString(msg));
    }
#else
    vlog().output(mod, Vlog::LEVEL_ERR, PyString_AsString(msg));
#endif
    Py_RETURN_NONE;
}

PyObject*
vigillog::warn(int mod, PyObject* msg)
{
#ifdef LOG4CXX_ENABLED
    Logger_map::iterator i = loggers.find(mod);
    if (i == loggers.end()) {
        lg.err("Invalid mod id (%d) received from Python, can't log %s", mod,
               PyString_AsString(msg));
    } else {
        i->second->warn("%s", PyString_AsString(msg));
    }
#else
    vlog().output(mod, Vlog::LEVEL_WARN, PyString_AsString(msg));
#endif
    Py_RETURN_NONE;
}

PyObject*
vigillog::info(int mod, PyObject* msg)
{
#ifdef LOG4CXX_ENABLED
    Logger_map::iterator i = loggers.find(mod);
    if (i == loggers.end()) {
        lg.err("Invalid mod id (%d) received from Python, can't log %s", mod,
               PyString_AsString(msg));
    } else {
        i->second->info("%s", PyString_AsString(msg));
    }
#else
    vlog().output(mod, Vlog::LEVEL_INFO, PyString_AsString(msg));
#endif
    Py_RETURN_NONE;
}

PyObject*
vigillog::dbg(int mod, PyObject* msg)
{
#ifdef LOG4CXX_ENABLED
    Logger_map::iterator i = loggers.find(mod);
    if (i == loggers.end()) {
        lg.err("Invalid mod id (%d) received from Python, can't log %s", mod,
               PyString_AsString(msg));
    } else {
        i->second->dbg("%s", PyString_AsString(msg));
    }
#else
    vlog().output(mod, Vlog::LEVEL_DBG, PyString_AsString(msg));
#endif
    Py_RETURN_NONE;
}

PyObject*
vigillog::mod_init(PyObject* module)
{
    if (!module || !PyString_Check(module)) {
        PyErr_SetString(PyExc_TypeError, "'module' object is not callable");
        return 0;
    }

#ifdef LOG4CXX_ENABLED
    ++mod_id;
    loggers[mod_id] = new Vlog_module(PyString_AsString(module));
    return PyInt_FromLong(mod_id);
#else
    return PyInt_FromLong(vlog().get_module_val(PyString_AsString(module)));
#endif
}

bool
vigillog::is_emer_enabled(int mod) {
#ifdef LOG4CXX_ENABLED
    Logger_map::iterator i = loggers.find(mod);
    if (i == loggers.end()) {
        return false;
    } else {
        return i->second->is_emer_enabled();
    }
#else
    return vlog().is_loggable(mod, Vlog::LEVEL_EMER);
#endif
}

bool
vigillog::is_err_enabled(int mod) {
#ifdef LOG4CXX_ENABLED
    Logger_map::iterator i = loggers.find(mod);
    if (i == loggers.end()) {
        return false;
    } else {
        return i->second->is_err_enabled();
    }
#else
    return vlog().is_loggable(mod, Vlog::LEVEL_ERR);
#endif
}

bool
vigillog::is_warn_enabled(int mod) {
#ifdef LOG4CXX_ENABLED
    Logger_map::iterator i = loggers.find(mod);
    if (i == loggers.end()) {
        return false;
    } else {
        return i->second->is_warn_enabled();
    }
#else
    return vlog().is_loggable(mod, Vlog::LEVEL_WARN);
#endif
}

bool
vigillog::is_info_enabled(int mod) {
#ifdef LOG4CXX_ENABLED
    Logger_map::iterator i = loggers.find(mod);
    if (i == loggers.end()) {
        return false;
    } else {
        return i->second->is_info_enabled();
    }
#else
    return vlog().is_loggable(mod, Vlog::LEVEL_INFO);
#endif
}

bool
vigillog::is_dbg_enabled(int mod) {
#ifdef LOG4CXX_ENABLED
    Logger_map::iterator i = loggers.find(mod);
    if (i == loggers.end()) {
        return false;
    } else {
        return i->second->is_dbg_enabled();
    }
#else
    return vlog().is_loggable(mod, Vlog::LEVEL_DBG);
#endif
}
