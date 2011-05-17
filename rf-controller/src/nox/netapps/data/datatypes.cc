/* Copyright 2009 (C) Nicira, Inc.
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

#include "datatypes.hh"

#include "vlog.hh"

#define DEFAULT_SWITCH        -1
#define DEFAULT_LOCATION      -2
#define DEFAULT_HOST          -3
#define DEFAULT_HOST_NETID    -4
#define DEFAULT_USER          -5
#define DEFAULT_ADDRESS       -6
#define DEFAULT_GROUP         -7
#define DEFAULT_GROUP_MEMBER  -8
#define PRINCIPALTYPE_INVALID  -9 // never set by database trigger

#define DEFAULT_MAC           -1
#define DEFAULT_IPV4_CIDR     -2
#define DEFAULT_IPV6          -3
#define DEFAULT_ALIAS         -4

namespace vigil {

static Vlog_module lg("datatypes");

std::size_t
PrincipalHash::operator()(const Principal& p) const
{
    return HASH_NAMESPACE::hash<PrincipalType>()(p.id);
}

bool
PrincipalEq::operator()(const Principal& a, const Principal& b) const
{
    return a.id == b.id && a.type == b.type;
}

std::size_t
Datatypes::DSNameHash::operator()(const DSName& d) const
{
    return HASH_NAMESPACE::hash<std::string>()(d.name);
}

bool
Datatypes::DSNameEq::operator()(const DSName& a, const DSName& b) const
{
    return a.ds_t == b.ds_t && a.name == b.name;
}

Datatypes::Datatypes(const container::Context* c, const json_object*)
    : Component(c)
{
    switch_t = DEFAULT_SWITCH;
    location_t = DEFAULT_LOCATION;
    host_t = DEFAULT_HOST;
    host_netid_t = DEFAULT_HOST_NETID;
    user_t = DEFAULT_USER;
    address_t = DEFAULT_ADDRESS;
    group_t = DEFAULT_GROUP;
    group_member_t = DEFAULT_GROUP_MEMBER;
    invalid_t = PRINCIPALTYPE_INVALID; 

    mac_t = DEFAULT_MAC;
    ipv4_cidr_t = DEFAULT_IPV4_CIDR;
    ipv6_t = DEFAULT_IPV6;
    alias_t = DEFAULT_ALIAS;
}

void
Datatypes::getInstance(const container::Context* ctxt,
                       Datatypes*& d)
{
    d = dynamic_cast<Datatypes*>
        (ctxt->get_by_interface(container::Interface_description
                                (typeid(Datatypes).name())));
}

const std::string&
Datatypes::principal_table_name(PrincipalType val) const
{
    static const std::string switch_s = "NOX_SWITCH";
    static const std::string location_s = "NOX_LOCATION";
    static const std::string host_s = "NOX_HOST";
    static const std::string host_netid_s = "NOX_HOST_NETID";
    static const std::string user_s = "NOX_USER";
    static const std::string address_s = "NOX_ADDRESS";
    static const std::string group_s = "NOX_GROUP";
    static const std::string group_member_s = "NOX_GROUP_MEMBER";
    static const std::string unknown = "";

    if (switch_t == val) {
        return switch_s;
    } else if (location_t == val) {
        return location_s;
    } else if (host_t == val) {
        return host_s;
    } else if (host_netid_t == val) {
        return host_netid_s;
    } else if (user_t == val) {
        return user_s;
    } else if (address_t == val) {
        return address_s;
    } else if (group_t == val) {
        return group_s;
    } else if (group_member_t == val) {
        return group_member_s;
    }
    return unknown;
}

const std::string&
Datatypes::principal_string(PrincipalType val) const
{
    static const std::string switch_s = "Switch";
    static const std::string location_s = "Location";
    static const std::string host_s = "Host";
    static const std::string host_netid_s = "Host_Netid";
    static const std::string user_s = "User";
    static const std::string address_s = "Address";
    static const std::string group_s = "Group";
    static const std::string group_member_s = "Group Member";
    static const std::string unknown = "";

    if (switch_t == val) {
        return switch_s;
    } else if (location_t == val) {
        return location_s;
    } else if (host_t == val) {
        return host_s;
    } else if (host_netid_t == val) {
        return host_netid_s;
    } else if (user_t == val) {
        return user_s;
    } else if (address_t == val) {
        return address_s;
    } else if (group_t == val) {
        return group_s;
    } else if (group_member_t == val) {
        return group_member_s;
    }
    return unknown;
}

const std::string&
Datatypes::address_string(AddressType val) const
{
    static const std::string mac_s = "Mac";
    static const std::string ipv4_cidr_s = "IPv4_cidr";
    static const std::string ipv6_s = "IPv6";
    static const std::string alias_s = "Alias";
    static const std::string unknown = "";

    if (mac_t == val) {
        return mac_s;
    } else if (ipv4_cidr_t == val) {
        return ipv4_cidr_s;
    } else if (ipv6_t == val) {
        return ipv6_s;
    } else if (alias_t == val) {
        return alias_s;
    }
    return unknown;
}

const std::string&
Datatypes::datasource_string(DatasourceType val) const
{
    static const std::string unknown = "";
    for (DSMap::const_iterator iter = datasource_types.begin();
         iter != datasource_types.end(); ++iter)
    {
        if (iter->second == val) {
            return iter->first;
        }
    }
    return unknown;
}

const std::string&
Datatypes::external_string(ExternalType val) const
{
    static const std::string unknown = "";
    for (EXMap::const_iterator iter = external_types.begin();
         iter != external_types.end(); ++iter)
    {
        if (iter->second == val) {
            return iter->first.name;
        }
    }
    return unknown;
}

const std::string&
Datatypes::attribute_string(AttributeType val) const
{
    static const std::string unknown = "";
    for (ATMap::const_iterator iter = attribute_types.begin();
         iter != attribute_types.end(); ++iter)
    {
        if (iter->second == val) {
            return iter->first.name;
        }
    }
    return unknown;
}

void
Datatypes::unset_principal_type(PrincipalType val)
{
    if (switch_t == val) {
        switch_t = DEFAULT_SWITCH;
    } else if (location_t == val) {
        location_t = DEFAULT_LOCATION;
    } else if (host_t == val) {
        host_t = DEFAULT_HOST;
    } else if (host_netid_t == val) {
        host_netid_t = DEFAULT_HOST_NETID;
    } else if (user_t == val) {
        user_t = DEFAULT_USER;
    } else if (address_t == val) {
        address_t = DEFAULT_ADDRESS;
    } else if (group_t == val) {
        group_t = DEFAULT_GROUP;
    } else if (group_member_t == val) {
        group_member_t = DEFAULT_GROUP_MEMBER;
    }
}

void
Datatypes::unset_address_type(AddressType val)
{
    if (mac_t == val) {
        mac_t = DEFAULT_MAC;
    } else if (ipv4_cidr_t == val) {
        ipv4_cidr_t = DEFAULT_IPV4_CIDR;
    } else if (ipv6_t == val) {
        ipv6_t = DEFAULT_IPV6;
    } else if (alias_t == val) {
        alias_t = DEFAULT_ALIAS;
    }
}

DatasourceType
Datatypes::datasource_type(const std::string& name) const
{
    DSMap::const_iterator ds = datasource_types.find(name);
    if (ds != datasource_types.end()) {
        return ds->second;
    }
    return -1;
}

ExternalType
Datatypes::external_type(DatasourceType ds_t, const std::string& name) const
{
    DSName dsname = { ds_t, name };
    EXMap::const_iterator ex = external_types.find(dsname);
    if (ex != external_types.end()) {
        return ex->second;
    }
    return -1;
}

AttributeType
Datatypes::attribute_type(DatasourceType ds_t, const std::string& name) const
{
    DSName dsname = { ds_t, name };
    ATMap::const_iterator at = attribute_types.find(dsname);
    if (at != attribute_types.end()) {
        return at->second;
    }
    return -1;
}

void
Datatypes::set_datasource_type(const std::string& name, DatasourceType ds_t)
{
    datasource_types[name] = ds_t;
}

void
Datatypes::set_external_type(DatasourceType ds_t, const std::string& name,
                             ExternalType ex_t)
{
    DSName dsname = { ds_t, name };
    external_types[dsname] = ex_t;
}

void
Datatypes::set_attribute_type(DatasourceType ds_t, const std::string& name,
                              AttributeType a_t)
{
    DSName dsname = { ds_t, name };
    attribute_types[dsname] = a_t;
}

void
Datatypes::unset_datasource_type(DatasourceType val)
{
    for (DSMap::iterator iter = datasource_types.begin();
         iter != datasource_types.end(); ++iter)
    {
        if (iter->second == val) {
            datasource_types.erase(iter);
            return;
        }
    }
}

void
Datatypes::unset_external_type(ExternalType val)
{
    for (EXMap::iterator iter = external_types.begin();
         iter != external_types.end(); ++iter)
    {
        if (iter->second == val) {
            external_types.erase(iter);
            return;
        }
    }
}

void
Datatypes::unset_attribute_type(AttributeType val)
{
    for (ATMap::iterator iter = attribute_types.begin();
         iter != attribute_types.end(); ++iter)
    {
        if (iter->second == val) {
            attribute_types.erase(iter);
            return;
        }
    }
}

#ifdef TWISTED_ENABLED

template <>
PyObject*
to_python(const Switch& sw)
{
    PyObject *pyswitch = PyDict_New();
    if (pyswitch == NULL) {
        VLOG_ERR(lg, "Could not create Python dict for switch.");
        Py_RETURN_NONE;
    }

    pyglue_setdict_string(pyswitch, "dp", to_python(sw.dp));
    pyglue_setdict_string(pyswitch, "name", to_python(sw.name));
    pyglue_setdict_string(pyswitch, "groups", to_python_list(sw.groups));
    return pyswitch;
}

template <>
PyObject*
to_python(const Location& location)
{
    PyObject *pylocation = PyDict_New();
    if (pylocation == NULL) {
        VLOG_ERR(lg, "Could not create Python dict for location.");
        Py_RETURN_NONE;
    }

    pyglue_setdict_string(pylocation, "port", to_python(location.port));
    pyglue_setdict_string(pylocation, "name", to_python(location.name));
    pyglue_setdict_string(pylocation, "portname", to_python(location.portname));
    pyglue_setdict_string(pylocation, "is_virtual", to_python(location.is_virtual));
    pyglue_setdict_string(pylocation, "groups", to_python_list(location.groups));
    pyglue_setdict_string(pylocation, "sw", to_python(*location.sw));
    return pylocation;
}

// collapses location into authed_location right now to save extra dictionary -
// should not do this?

template <>
PyObject*
to_python(const AuthedLocation& alocation)
{
    PyObject *pyalocation = to_python(*(alocation.location));
    if (pyalocation == Py_None) {
        return pyalocation;
    }
    pyglue_setdict_string(pyalocation, "last_active", to_python(alocation.last_active));
    pyglue_setdict_string(pyalocation, "idle_timeout", to_python(alocation.idle_timeout));
    return pyalocation;
}

template <>
PyObject*
to_python(const User& user)
{
    PyObject *pyuser = PyDict_New();
    if (pyuser == NULL) {
        VLOG_ERR(lg, "Could not create Python dict for user.");
        Py_RETURN_NONE;
    }
    pyglue_setdict_string(pyuser, "name", to_python(user.name));
    pyglue_setdict_string(pyuser, "groups", to_python_list(user.groups));
    return pyuser;
}

// collapses user into authed_user right now to save extra dictionary -
// should not do this?

template <>
PyObject*
to_python(const AuthedUser& auser)
{
    PyObject *pyauser = to_python(*(auser.user));
    if (pyauser == Py_None) {
        return pyauser;
    }
    pyglue_setdict_string(pyauser, "auth_time", to_python(auser.auth_time));
    pyglue_setdict_string(pyauser, "idle_timeout", to_python(auser.idle_timeout));
    pyglue_setdict_string(pyauser, "hard_timeout", to_python(auser.hard_timeout));
    return pyauser;
}

template <>
PyObject*
to_python(const Host& host)
{
    PyObject *pyhost = PyDict_New();
    if (pyhost == NULL) {
        VLOG_ERR(lg, "Could not create Python dict for host.");
        Py_RETURN_NONE;
    }
    pyglue_setdict_string(pyhost, "name", to_python(host.name));
    pyglue_setdict_string(pyhost, "groups", to_python_list(host.groups));
    pyglue_setdict_string(pyhost, "auth_time", to_python(host.auth_time));
    pyglue_setdict_string(pyhost, "last_active", to_python(host.last_active));
    pyglue_setdict_string(pyhost, "hard_timeout", to_python(host.hard_timeout));
    PyObject *pyusers = PyList_New(host.users.size());
    if (pyusers == NULL) {
        VLOG_ERR(lg, "Could not create user list for host.");
        Py_INCREF(Py_None);
        pyusers = Py_None;
    } else {
        std::list<AuthedUser>::const_iterator user = host.users.begin();
        for (uint32_t i = 0; user != host.users.end(); ++i, ++user) {
            if (PyList_SetItem(pyusers, i, to_python(*user)) != 0) {
                VLOG_ERR(lg, "Could not set user list in host.");
            }
        }
    }
    pyglue_setdict_string(pyhost, "users", pyusers);
    return pyhost;
}

template <>
PyObject*
to_python(const HostNetid& host_netid)
{
    PyObject *pyhost_netid = PyDict_New();
    if (pyhost_netid == NULL) {
        VLOG_ERR(lg, "Could not create Python dict for host.");
        Py_RETURN_NONE;
    }
    pyglue_setdict_string(pyhost_netid, "name", to_python(host_netid.name));
    pyglue_setdict_string(pyhost_netid, "groups", to_python_list(host_netid.groups));
    pyglue_setdict_string(pyhost_netid, "host", to_python(*host_netid.host));
    return pyhost_netid;
}

template <>
PyObject*
to_python(const Principal& p)
{
    PyObject *py_principal = PyDict_New();
    if (py_principal == NULL) {
        VLOG_ERR(lg, "Could not create Python dict for principal.");
        Py_RETURN_NONE;
    }
    pyglue_setdict_string(py_principal, "id", to_python(p.id));
    pyglue_setdict_string(py_principal, "type", to_python(p.type));
    return py_principal;
}

#endif

} // namespace vigil

using namespace vigil;
using namespace vigil::container;

REGISTER_COMPONENT(Simple_component_factory<Datatypes>, Datatypes);
