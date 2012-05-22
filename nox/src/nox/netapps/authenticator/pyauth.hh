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
#ifndef CONTROLLER_PYHOSTGLUE_HH
#define CONTROLLER_PYHOSTGLUE_HH 1

#include <Python.h>

#include "authenticator.hh"
#include "component.hh"

/*
 * Proxy authenticator "component" used to set authenticator's python functions
 * and permit access to the authenticator's "updated_group" method.
 */

namespace vigil {
namespace applications {

class PyAuthenticator {
public:
    PyAuthenticator(PyObject*);

    void configure(PyObject*);

    void add_internal_subnet(const cidr_ipaddr&);
    bool remove_internal_subnet(const cidr_ipaddr&);
    void clear_internal_subnets();

    int64_t get_authed_hostname(const ethernetaddr& dladdr, uint32_t nwaddr);
    PyObject* get_authed_locations(const ethernetaddr& dladdr,
                                   uint32_t nwaddr);
    PyObject* get_authed_addresses(int64_t host);
    bool is_virtual_location(const datapathid& dp, uint16_t port);
    void get_names(const datapathid& dp, uint16_t inport,
                   const ethernetaddr& dlsrc, uint32_t nwsrc,
                   const ethernetaddr& dldst, uint32_t nwdst,
                   PyObject *callable);

    void all_updated(bool poison);
    void principal_updated(int64_t ptype, int64_t id, bool poison);
    void groups_updated(const std::list<int64_t>& groups, bool poison);

    bool is_switch_active(const datapathid& dp);
    bool is_netid_active(int64_t netid);

    int64_t get_port_number(const datapathid& dp, const std::string& port_name);

private:
    Authenticator *authenticator;
    Datatypes     *datatypes;
    Data_cache    *data_cache;
    container::Component* c;
};

}
}

#endif
