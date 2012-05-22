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
#include "pyauth.hh"

#include "swigpyrun.h"
#include "pyrt/pycontext.hh"
#include "vlog.hh"

namespace vigil {
namespace applications {

static Vlog_module lg("pyauth");

PyAuthenticator::PyAuthenticator(PyObject* ctxt)
    : authenticator(0)
{
    if (!SWIG_Python_GetSwigThis(ctxt) || !SWIG_Python_GetSwigThis(ctxt)->ptr) {
        throw std::runtime_error("Unable to access Python context.");
    }

    c = ((PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr)->c;
}

void
PyAuthenticator::configure(PyObject* configuration)
{
    c->resolve(authenticator);
    c->resolve(datatypes);
    c->resolve(data_cache);
}

void
PyAuthenticator::add_internal_subnet(const cidr_ipaddr& cidr)
{
    authenticator->add_internal_subnet(cidr);
}

bool
PyAuthenticator::remove_internal_subnet(const cidr_ipaddr& cidr)
{
    return authenticator->remove_internal_subnet(cidr);
}

void
PyAuthenticator::clear_internal_subnets()
{
    authenticator->clear_internal_subnets();
}

int64_t
PyAuthenticator::get_authed_hostname(const ethernetaddr& dladdr,
                                     uint32_t nwaddr)
{
    const Authenticator::DLNWEntry *dlnwentry
        = authenticator->get_dlnw_entry(dladdr, nwaddr);
    if (dlnwentry != NULL && dlnwentry->authed) {
        return dlnwentry->host_netid->host->name;
    }
    return data_cache->get_unauthenticated_id(datatypes->host_type());
}

PyObject*
PyAuthenticator::get_authed_locations(const ethernetaddr& dladdr,
                                      uint32_t nwaddr)
{
    const Authenticator::DLNWEntry *dlnwentry
        = authenticator->get_dlnw_entry(dladdr, nwaddr);
    if (dlnwentry == NULL || !dlnwentry->authed) {
        PyObject *pylocs = PyList_New(0);
        if (pylocs == NULL) {
            VLOG_ERR(lg, "Could not create location list.");
            Py_RETURN_NONE;
        }
        return pylocs;
    }

    uint32_t i = 0;
    PyObject *pylocs = PyList_New(dlnwentry->dlentry->locations.size());
    if (pylocs == NULL) {
        VLOG_ERR(lg, "Could not create location list.");
        Py_RETURN_NONE;
    }
    std::list<AuthedLocation>::const_iterator iter
        = dlnwentry->dlentry->locations.begin();
    for (; iter != dlnwentry->dlentry->locations.end(); ++iter, ++i) {
        uint64_t dpport = iter->location->sw->dp.as_host()
            + (((uint64_t)iter->location->port) << 48);
        if (PyList_SetItem(pylocs, i, to_python(dpport)) != 0) {
            VLOG_ERR(lg, "Could not set location list.");
        }
    }
    return pylocs;
}

PyObject*
PyAuthenticator::get_authed_addresses(int64_t host)
{
    const Authenticator::HostEntry *hentry
        = authenticator->get_host_entry(host);
    hash_map<uint64_t, std::vector<uint32_t> > addrs;
    hash_map<uint64_t, std::vector<uint32_t> >::iterator found;

    if (hentry != NULL) {
        std::list<Authenticator::HostNetEntry*>::const_iterator iter
            = hentry->netids.begin();
        for (; iter != hentry->netids.end(); ++iter) {
            std::list<Authenticator::DLNWEntry*>::iterator diter
                = (*iter)->dlnwentries.begin();
            for (; diter != (*iter)->dlnwentries.end(); ++diter) {
                if ((*diter)->authed) {
                    uint64_t dladdr = (*diter)->dlentry->dladdr.hb_long();
                    found = addrs.find(dladdr);
                    if (found == addrs.end()) {
                        addrs[dladdr]
                            = std::vector<uint32_t>(1, (*diter)->nwaddr);
                    } else {
                        found->second.push_back((*diter)->nwaddr);
                    }
                }
            }
        }
    }

    PyObject *pyaddrs = PyList_New(addrs.size());
    if (pyaddrs == NULL) {
        VLOG_ERR(lg, "Could not create address list.");
        Py_RETURN_NONE;
    }

    uint32_t i = 0;
    for (found = addrs.begin(); found != addrs.end(); ++found, ++i) {
        PyObject *pyentry = PyList_New(2);
        if (pyentry == NULL) {
            Py_DECREF(pyaddrs);
            VLOG_ERR(lg, "Could not create address list entry.");
            Py_RETURN_NONE;
        }
        PyList_SetItem(pyentry, 0, to_python(found->first));
        PyList_SetItem(pyentry, 1, to_python_list(found->second));
        if (PyList_SetItem(pyaddrs, i, pyentry) != 0) {
            VLOG_ERR(lg, "Could not set address list.");
        }
    }
    return pyaddrs;
}

bool
PyAuthenticator::is_virtual_location(const datapathid& dp, uint16_t port)
{
    const Authenticator::LocEntry *lentry
        = authenticator->get_location_entry(dp, port);
    return lentry != NULL && lentry->entry->is_virtual;
}

void
PyAuthenticator::get_names(const datapathid& dp, uint16_t inport,
                           const ethernetaddr& dlsrc, uint32_t nwsrc,
                           const ethernetaddr& dldst, uint32_t nwdst,
                           PyObject *callable)
{
    authenticator->get_names(dp, inport, dlsrc, nwsrc, dldst, nwdst, callable);
}

void
PyAuthenticator::all_updated(bool poison)
{
    authenticator->all_updated(poison);
}

void
PyAuthenticator::principal_updated(int64_t ptype, int64_t id, bool poison)
{
    Principal principal = { ptype, id };
    authenticator->principal_updated(principal, poison, false);
}

void
PyAuthenticator::groups_updated(const std::list<int64_t>& groups,
                                bool poison)
{
    authenticator->groups_updated(groups, poison);
}

bool
PyAuthenticator::is_switch_active(const datapathid& dp)
{
    const Authenticator::SwitchEntry *sentry
        = authenticator->get_switch_entry(dp);
    return (sentry != NULL && sentry->active);
}

bool
PyAuthenticator::is_netid_active(int64_t netid)
{
    const Authenticator::HostNetEntry *hentry
        = authenticator->get_host_netid_entry(netid);

    if (hentry == NULL) {
        return false;
    }

    for (std::list<Authenticator::DLNWEntry*>::const_iterator i
             = hentry->dlnwentries.begin();
         i != hentry->dlnwentries.end(); ++i)
    {
        if ((*i)->authed) {
            return true;
        }
    }
    return false;
}

int64_t
PyAuthenticator::get_port_number(const datapathid& dp,
                                 const std::string& port_name)
{
    return authenticator->get_port_number(dp, port_name);
}

}
}

