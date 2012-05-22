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
#ifndef PYDATATYPES_HH
#define PYDATATYPES_HH 1

#include <Python.h>

#include "datatypes.hh"
#include "component.hh"

namespace vigil {

class PyDatatypes {
public:
    PyDatatypes(PyObject*);

    void configure(PyObject*);

    PrincipalType switch_type() const;
    PrincipalType location_type() const;
    PrincipalType host_type() const;
    PrincipalType host_netid_type() const;
    PrincipalType user_type() const;
    PrincipalType address_type() const;
    PrincipalType group_type() const;
    PrincipalType group_member_type() const;

    AddressType mac_type() const;
    AddressType ipv4_cidr_type() const;
    AddressType ipv6_type() const;
    AddressType alias_type() const;

    DatasourceType datasource_type(const std::string& name) const;
    ExternalType external_type(DatasourceType ds_t,
                               const std::string& name) const;
    AttributeType attribute_type(DatasourceType ds_t,
                                 const std::string& name) const;

    const std::string& principal_table_name(PrincipalType val) const;

    const std::string& principal_string(PrincipalType val) const;
    const std::string& address_string(AddressType val) const;
    const std::string& datasource_string(DatasourceType val) const;
    const std::string& external_string(ExternalType val) const;
    const std::string& attribute_string(AttributeType val) const;

    void set_switch_type(PrincipalType val);
    void set_location_type(PrincipalType val);
    void set_host_type(PrincipalType val);
    void set_host_netid_type(PrincipalType val);
    void set_user_type(PrincipalType val);
    void set_address_type(PrincipalType val);
    void set_group_type(PrincipalType val);
    void set_group_member_type(PrincipalType val);

    void set_mac_type(AddressType val);
    void set_ipv4_cidr_type(AddressType val);
    void set_ipv6_type(AddressType val);
    void set_alias_type(AddressType val);

    void set_datasource_type(const std::string& name, DatasourceType ds_t);
    void set_external_type(DatasourceType ds_t, const std::string& name,
                           ExternalType ex_t);
    void set_attribute_type(DatasourceType ds_t, const std::string& name,
                            AttributeType a_t);

    void unset_principal_type(PrincipalType val);
    void unset_address_type(AddressType val);
    void unset_datasource_type(DatasourceType val);
    void unset_external_type(ExternalType val);
    void unset_attribute_type(AttributeType val);

private:
    Datatypes *datatypes;
    container::Component* c;
};

}

#endif
