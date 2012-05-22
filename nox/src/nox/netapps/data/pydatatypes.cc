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
#include "pydatatypes.hh"

#include "swigpyrun.h"
#include "pyrt/pycontext.hh"

namespace vigil {

PyDatatypes::PyDatatypes(PyObject* ctxt)
    : datatypes(0)
{
    if (!SWIG_Python_GetSwigThis(ctxt) || !SWIG_Python_GetSwigThis(ctxt)->ptr) {
        throw std::runtime_error("Unable to access Python context.");
    }

    c = ((applications::PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr)->c;
}

void
PyDatatypes::configure(PyObject* configuration)
{
    c->resolve(datatypes);
}

PrincipalType
PyDatatypes::switch_type() const
{
    return datatypes->switch_type();
}

PrincipalType
PyDatatypes::location_type() const
{
    return datatypes->location_type();
}

PrincipalType
PyDatatypes::host_type() const
{
    return datatypes->host_type();
}

PrincipalType
PyDatatypes::host_netid_type() const
{
    return datatypes->host_netid_type();
}

PrincipalType
PyDatatypes::user_type() const
{
    return datatypes->user_type();
}

PrincipalType
PyDatatypes::address_type() const
{
    return datatypes->address_type();
}

PrincipalType
PyDatatypes::group_type() const
{
    return datatypes->group_type();
}

PrincipalType
PyDatatypes::group_member_type() const
{
    return datatypes->group_member_type();
}

AddressType
PyDatatypes::mac_type() const
{
    return datatypes->mac_type();
}

AddressType
PyDatatypes::ipv4_cidr_type() const
{
    return datatypes->ipv4_cidr_type();
}

AddressType
PyDatatypes::ipv6_type() const
{
    return datatypes->ipv6_type();
}

AddressType
PyDatatypes::alias_type() const
{
    return datatypes->alias_type();
}

DatasourceType
PyDatatypes::datasource_type(const std::string& name) const
{
    return datatypes->datasource_type(name);
}

ExternalType
PyDatatypes::external_type(DatasourceType ds_t,
                           const std::string& name) const
{
    return datatypes->external_type(ds_t, name);
}

AttributeType
PyDatatypes::attribute_type(DatasourceType ds_t,
                            const std::string& name) const
{
    return datatypes->attribute_type(ds_t, name);
}

const std::string&
PyDatatypes::principal_table_name(PrincipalType val) const
{
    return datatypes->principal_table_name(val);
}

const std::string&
PyDatatypes::principal_string(PrincipalType val) const
{
    return datatypes->principal_string(val);
}

const std::string&
PyDatatypes::address_string(AddressType val) const
{
    return datatypes->address_string(val);
}

const std::string&
PyDatatypes::datasource_string(DatasourceType val) const
{
    return datatypes->datasource_string(val);
}

const std::string&
PyDatatypes::external_string(ExternalType val) const
{
    return datatypes->external_string(val);
}

const std::string&
PyDatatypes::attribute_string(AttributeType val) const
{
    return datatypes->attribute_string(val);
}

void
PyDatatypes::set_switch_type(PrincipalType val)
{
    datatypes->set_switch_type(val);
}

void
PyDatatypes::set_location_type(PrincipalType val)
{
    datatypes->set_location_type(val);
}

void
PyDatatypes::set_host_type(PrincipalType val)
{
    datatypes->set_host_type(val);
}

void
PyDatatypes::set_host_netid_type(PrincipalType val)
{
    datatypes->set_host_netid_type(val);
}

void
PyDatatypes::set_user_type(PrincipalType val)
{
    datatypes->set_user_type(val);
}

void
PyDatatypes::set_address_type(PrincipalType val)
{
    datatypes->set_address_type(val);
}

void
PyDatatypes::set_group_type(PrincipalType val)
{
    datatypes->set_group_type(val);
}

void
PyDatatypes::set_group_member_type(PrincipalType val)
{
    datatypes->set_group_member_type(val);
}

void
PyDatatypes::set_mac_type(AddressType val)
{
    datatypes->set_mac_type(val);
}

void
PyDatatypes::set_ipv4_cidr_type(AddressType val)
{
    datatypes->set_ipv4_cidr_type(val);
}

void
PyDatatypes::set_ipv6_type(AddressType val)
{
    datatypes->set_ipv6_type(val);
}

void
PyDatatypes::set_alias_type(AddressType val)
{
    datatypes->set_alias_type(val);
}

void
PyDatatypes::set_datasource_type(const std::string& name, DatasourceType ds_t)
{
    datatypes->set_datasource_type(name, ds_t);
}

void
PyDatatypes::set_external_type(DatasourceType ds_t, const std::string& name,
                               ExternalType ex_t)
{
    datatypes->set_external_type(ds_t, name, ex_t);
}

void
PyDatatypes::set_attribute_type(DatasourceType ds_t, const std::string& name,
                                AttributeType a_t)
{
    datatypes->set_attribute_type(ds_t, name, a_t);
}

void
PyDatatypes::unset_principal_type(PrincipalType val)
{
    datatypes->unset_principal_type(val);
}

void
PyDatatypes::unset_address_type(AddressType val)
{
    datatypes->unset_address_type(val);
}

void
PyDatatypes::unset_datasource_type(DatasourceType val)
{
    datatypes->unset_datasource_type(val);
}

void
PyDatatypes::unset_external_type(ExternalType val)
{
    datatypes->unset_external_type(val);
}

void
PyDatatypes::unset_attribute_type(AttributeType val)
{
    datatypes->unset_attribute_type(val);
}

}

