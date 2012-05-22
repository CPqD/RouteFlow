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

#ifndef DATATYPES_HH
#define DATATYPES_HH 1

#include <string>
#include <list>
#include <vector>

#include "config.h"
#include "component.hh"
#include "hash_map.hh"
#include "hash_set.hh"
#include "pyrt/pyglue.hh"

namespace vigil {

typedef int64_t PrincipalType;
typedef int64_t AddressType;
typedef int64_t DatasourceType;
typedef int64_t ExternalType;
typedef int64_t AttributeType;

struct Principal {
    PrincipalType type;
    int64_t id;
};

typedef std::list<Principal> PrincipalList;

// want to make a class and default hash/eq?
struct PrincipalHash {
    std::size_t operator()(const Principal& p) const;
};

struct PrincipalEq {
    bool operator()(const Principal& a, const Principal& b) const;
};

typedef hash_set<Principal, PrincipalHash, PrincipalEq> PrincipalSet;

class Datatypes
    : public container::Component
{

public:
    Datatypes(const container::Context* c, const json_object*);

    static void getInstance(const container::Context*, Datatypes*&);

    void configure(const container::Configuration*) {}
    void install() {}

    PrincipalType switch_type() const { return switch_t; }
    PrincipalType location_type() const { return location_t; }
    PrincipalType host_type() const { return host_t; }
    PrincipalType host_netid_type() const { return host_netid_t; }
    PrincipalType user_type() const { return user_t; }
    PrincipalType address_type() const { return address_t; }
    PrincipalType group_type() const { return group_t; }
    PrincipalType group_member_type() const { return group_member_t; }
    PrincipalType invalid_type() const { return invalid_t; }

    AddressType mac_type() const { return mac_t; }
    AddressType ipv4_cidr_type() const { return ipv4_cidr_t; }
    AddressType ipv6_type() const { return ipv6_t; }
    AddressType alias_type() const { return alias_t; }

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

    void set_switch_type(PrincipalType val) { switch_t = val; }
    void set_location_type(PrincipalType val) { location_t = val; }
    void set_host_type(PrincipalType val) { host_t = val; }
    void set_host_netid_type(PrincipalType val) { host_netid_t = val; }
    void set_user_type(PrincipalType val) { user_t = val; }
    void set_address_type(PrincipalType val) { address_t = val; }
    void set_group_type(PrincipalType val) { group_t = val; }
    void set_group_member_type(PrincipalType val) { group_member_t = val; }

    void set_mac_type(AddressType val) { mac_t = val; }
    void set_ipv4_cidr_type(AddressType val) { ipv4_cidr_t = val; }
    void set_ipv6_type(AddressType val) { ipv6_t = val; }
    void set_alias_type(AddressType val) { alias_t = val; }

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
    struct DSName {
        DatasourceType ds_t;
        std::string name;
    };

    struct DSNameHash {
        std::size_t operator()(const DSName& d) const;
    };

    struct DSNameEq {
        bool operator()(const DSName& a, const DSName& b) const;
    };

    typedef hash_map<std::string, DatasourceType> DSMap;
    typedef hash_map<DSName, ExternalType, DSNameHash, DSNameEq> EXMap;
    typedef hash_map<DSName, AttributeType, DSNameHash, DSNameEq> ATMap;

    PrincipalType switch_t;
    PrincipalType location_t;
    PrincipalType host_t;
    PrincipalType host_netid_t;
    PrincipalType user_t;
    PrincipalType address_t;
    PrincipalType group_t;
    PrincipalType group_member_t;
    PrincipalType invalid_t; 

    AddressType mac_t;
    AddressType ipv4_cidr_t;
    AddressType ipv6_t;
    AddressType alias_t;

    DSMap datasource_types;
    EXMap external_types;
    ATMap attribute_types;
}; // class Datatypes

typedef std::vector<int64_t> GroupList;

struct Switch {
    datapathid dp;
    int64_t    name;
    GroupList  groups;
};

struct Location {
    uint16_t    port;
    int64_t     name;
    std::string portname;
    bool        is_virtual;
    GroupList   groups;
    boost::shared_ptr<Switch> sw;
};

struct AuthedLocation {
    time_t   last_active;
    uint32_t idle_timeout;
    boost::shared_ptr<Location> location;
};

struct User {
    int64_t   name;
    GroupList groups;
};

struct AuthedUser {
    time_t   auth_time;
    uint32_t idle_timeout;
    uint32_t hard_timeout;
    boost::shared_ptr<User> user;
};

struct Host {
    int64_t   name;
    GroupList groups;
    time_t    auth_time;
    time_t    last_active;
    uint32_t  hard_timeout;
    std::list<AuthedUser> users;
};

struct HostNetid {
    int64_t     name;
    std::string addr_binding;
    AddressType addr_type;
    bool        is_router;
    bool        is_gateway;
    GroupList   groups;
    boost::shared_ptr<Host> host;
};

#ifdef TWISTED_ENABLED

template <>
PyObject*
to_python(const Switch& sw);

template <>
PyObject*
to_python(const Location& location);

template <>
PyObject*
to_python(const AuthedLocation& alocation);

template <>
PyObject*
to_python(const User& user);

template <>
PyObject*
to_python(const AuthedUser& auser);

template <>
PyObject*
to_python(const Host& host);

template <>
PyObject*
to_python(const HostNetid& host_netid);

template <>
PyObject*
to_python(const Principal& p);

#endif

}

#endif
