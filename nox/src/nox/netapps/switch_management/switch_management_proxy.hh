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
#ifndef SWITCH_MANAGEMENT_PROXY_HH
#define SWITCH_MANAGEMENT_PROXY_HH

#include <list>
#include <Python.h>

#include "netinet++/datapathid.hh"
#include "switch_management.hh"

namespace vigil {
namespace applications {

class Switch_management_proxy {

public: 
    Switch_management_proxy (PyObject* ctxt);

    void configure(PyObject*);
    void install(PyObject*);

    datapathid get_mgmt_id(const datapathid &dpid);

    bool switch_mgr_is_active(const datapathid &mgmt_id);

    std::string get_port_name(const datapathid &mgmt_id,
            const datapathid &port_id);

    std::string mgmtid_to_system_uuid(const datapathid &mgmt_id); 

    /*
     * Overwrite all existing entries matching key with zero or more
     * entries (one for each item in vals)
     * TODO: add callback when complete
     */
    PyObject* set_all_entries(datapathid mgmt_id, std::string key,
                              std::list<std::string> vals);

protected:   
    Switch_management* switch_mgmt;
    container::Component* c;
    PyObject *pydeferred_class;
    PyObject *pymethod_name;

    void callback(bool success, PyObject *deferred);

} ; // class Switch_management_proxy 

}} // namespace vigil::applications

#endif  //  SWITCH_MANAGEMENT_PROXY_HH
