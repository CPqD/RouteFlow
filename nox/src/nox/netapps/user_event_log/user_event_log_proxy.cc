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
/*
*/

#include "user_event_log_proxy.hh"
#include "threads/cooperative.hh"
#include "pyrt/pycontext.hh"
#include "swigpyrun.h"
#include "vlog.hh"
#include "pyrt/pyglue.hh"

using namespace std;
using namespace vigil;
using namespace vigil::applications;

namespace {
Vlog_module lg("user_event_log_proxy");
}

namespace vigil {
namespace applications {

/*
 * Get a pointer to the runtime context so we can resolve 
 * user_event_log at configure time.
 */

user_event_log_proxy::user_event_log_proxy(PyObject* ctxt) : uel(0) 
{
    if (!SWIG_Python_GetSwigThis(ctxt) || !SWIG_Python_GetSwigThis(ctxt)->ptr) {
        throw runtime_error("Unable to access Python context.");
    }
    
    c = ((PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr)->c;
}

void user_event_log_proxy::log_simple(const string &app_name, int level, 
                                              const string &msg){
  uel->log_simple(app_name, (LogEntry::Level) level, msg); 
} 

void user_event_log_proxy::log(const LogEntry &entry){
  uel->log(entry); 
} 
   
int user_event_log_proxy::get_max_logid() {
  return uel->get_max_logid(); 
} 

int user_event_log_proxy::get_min_logid() {
  return uel->get_min_logid(); 
} 

void user_event_log_proxy::set_max_num_entries(int num) { 
  uel->set_max_num_entries(num); 
} 

void user_event_log_proxy::python_callback(PyObject *args, 
                                  boost::intrusive_ptr<PyObject> cb) { 
    Co_critical_section c;
    PyObject* ret = PyObject_CallObject(cb.get(), args);
    if (ret == 0) {
        const string exc = pretty_print_python_exception();
        lg.err("Python callback invocation failed:\n%s", exc.c_str());
    }

    Py_DECREF(args);
    Py_XDECREF(ret);    
} 
    

PyObject *user_event_log_proxy::get_log_entry(int logid, PyObject *cb){
  try {
        if (!cb || !PyCallable_Check(cb)) { throw "Invalid callback"; }

        boost::intrusive_ptr<PyObject> cptr(cb, true);
        Log_entry_callback f = boost::bind(
        &user_event_log_proxy::get_log_callback,this,_1,_2,_3,_4,_5,_6,_7,cptr);
        uel->get_log_entry((int64_t)logid, f); 
        Py_RETURN_NONE;
    }
    catch (const char* msg) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, msg);
        return 0;
    }
} 

void user_event_log_proxy::get_log_callback(int64_t logid, int64_t ts, 
            const string &app, 
            int level, const string &msg, const PrincipalList &src_names, 
            const PrincipalList &dst_names, boost::intrusive_ptr<PyObject> cb) {
    Co_critical_section c;
    
    PyObject* args = PyTuple_New(7);
    PyTuple_SetItem(args, 0, PyInt_FromLong(logid));
    PyTuple_SetItem(args, 1, PyLong_FromLong(ts)); 
    PyTuple_SetItem(args, 2, PyString_FromString(app.c_str()));
    PyTuple_SetItem(args, 3, PyInt_FromLong(level));
    PyTuple_SetItem(args, 4, PyString_FromString(msg.c_str()));
    PyTuple_SetItem(args, 5, to_python_list(src_names));
    PyTuple_SetItem(args, 6, to_python_list(dst_names));
    
    python_callback(args,cb); 
} 

PyObject * user_event_log_proxy::get_logids_for_name(int64_t id, 
                                    int64_t type, PyObject* cb) {
  try {
        if (!cb || !PyCallable_Check(cb)) { throw "Invalid callback"; }

        boost::intrusive_ptr<PyObject> cptr(cb, true);
        Get_logids_callback f = boost::bind(
                &user_event_log_proxy::get_logids_callback,this,_1,cptr);
        uel->get_logids_for_name(id, (PrincipalType) type,f); 
        Py_RETURN_NONE;
    }
    catch (const char* msg) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, msg);
        return 0;
    }

}

void user_event_log_proxy::get_logids_callback(const list<int64_t> &logids, 
                                    boost::intrusive_ptr<PyObject> cb) {
    Co_critical_section c;
    
    PyObject* logid_tuple = PyTuple_New(logids.size()); 
    list<int64_t>::const_iterator it = logids.begin(); 
    int i = 0; 
    for( ; it != logids.end(); ++it) { 
      PyTuple_SetItem(logid_tuple, i, PyLong_FromLong(*it)); 
      ++i;
    } 
     
    PyObject* args = PyTuple_New(1);
    PyTuple_SetItem(args,0,logid_tuple); 
    python_callback(args,cb); 
} 

PyObject *user_event_log_proxy::clear(PyObject *cb){
  try {
        if (!cb || !PyCallable_Check(cb)) { throw "Invalid callback"; }

        boost::intrusive_ptr<PyObject> cptr(cb, true);
        Clear_log_callback f = boost::bind(
          &user_event_log_proxy::clear_callback,this,_1, cptr);
        uel->clear(f); 
        Py_RETURN_NONE;
    }
    catch (const char* msg) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, msg);
        return 0;
    }
} 

PyObject *user_event_log_proxy::remove(int max_logid, PyObject *cb){
  try {
        if (!cb || !PyCallable_Check(cb)) { throw "Invalid callback"; }

        boost::intrusive_ptr<PyObject> cptr(cb, true);
        Clear_log_callback f = boost::bind(
          &user_event_log_proxy::clear_callback,this,_1,cptr);
        uel->remove(max_logid, f); 
        Py_RETURN_NONE;
    }
    catch (const char* msg) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, msg);
        return 0;
    }
} 

void user_event_log_proxy::clear_callback(const storage::Result &r,
                    boost::intrusive_ptr<PyObject> cb){
    PyObject* args = PyTuple_New(0);
    python_callback(args,cb); 
}


void
user_event_log_proxy::configure(PyObject* configuration) 
{
    c->resolve(uel);    
}

void 
user_event_log_proxy::install(PyObject*) 
{
}

} // namespace applications
} // namespace vigil
