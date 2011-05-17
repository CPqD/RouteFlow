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
/* Proxy class to expose user_event_log.hh to Python.
 * This file is only to be included from the SWIG interface file
 * (user_event_log_proxy.i)
 */

#ifndef USER_EVENT_LOG_PROXY_HH__
#define USER_EVENT_LOG_PROXY_HH__

#include <Python.h>

#include "user_event_log.hh"
#include "pyrt/pyglue.hh"

using namespace std;

namespace vigil {
namespace applications {

class user_event_log_proxy{
  
  public:
    user_event_log_proxy(PyObject* ctxt);

    void configure(PyObject*);
    void install(PyObject*);

    void log_simple(const string &app_name, int level, const string &msg);
    void log(const LogEntry &entry);
    
    // because python int may be 32-bits, we only ask for logid to be an
    // int here, and then cast it to int64_t to make the user_event_log
    // call
    PyObject* get_log_entry(int logid, PyObject *cb);
    int get_max_logid();
    int get_min_logid();
    void set_max_num_entries(int num);  
    PyObject* get_logids_for_name(int64_t id,int64_t type,PyObject* cb); 

    PyObject *clear(PyObject *cb);
    PyObject *remove(int maxlogid, PyObject *cb);

  protected:   

    User_Event_Log* uel;
    container::Component* c;

    void get_log_callback(int64_t logid, int64_t ts, const string &app, 
            int level, const string &msg, const PrincipalList &src_names, 
            const PrincipalList &dst_names, boost::intrusive_ptr<PyObject> cb); 

    void get_logids_callback(const list<int64_t> &logids, 
                                    boost::intrusive_ptr<PyObject> cb);

    void python_callback(PyObject *args, 
                        boost::intrusive_ptr<PyObject> cb);

    void clear_callback(const storage::Result &r,
                        boost::intrusive_ptr<PyObject> cb); 

}; // class user_event_log_proxy

} // namespcae applications
} // namespace vigil

#endif //  USER_EVENT_LOG_PROXY_HH__
