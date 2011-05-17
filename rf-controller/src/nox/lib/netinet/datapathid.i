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
%{
#include "netinet++/datapathid.hh"

using namespace vigil;
%}

%include "std_string.i"

struct datapathid
{
    datapathid();
    datapathid(const datapathid&);
    static datapathid from_host(uint64_t host_order);
    static datapathid from_net(uint64_t net_order);

    uint64_t as_host() const;
    uint64_t as_net() const;

    bool operator<(const datapathid&) const;
    bool operator<=(const datapathid&) const;
    bool operator>(const datapathid&) const;
    bool operator>=(const datapathid&) const;
    bool empty() const;

    std::string string() const;

    %extend {
       datapathid* __copy__(const PyObject *memodict) {
           return new datapathid(*$self);
       }
    }

    %extend {
       datapathid* __deepcopy__(const PyObject *memodict) {
           return new datapathid(*$self);
       }
    }

    %extend {
       char *__str__() {
           static char temp[64];
           snprintf(temp, sizeof temp, "%s", self->string().c_str());
           return &temp[0];
       }
    }

    %extend {
       bool __nonzero__() {
           return (!self->empty());
       }
    }

    %extend {
        bool operator == (const datapathid *other) {
             if (other == NULL) {
                 return false;
             }
             return *$self == *other;
        }

        bool operator != (const datapathid *other) {
             if (other == NULL) {
                 return true;
             }
             return *$self != *other;
        }
    }

    %extend {
        unsigned int __hash__() {
            return self->as_host();
        }
    }
};

%newobject create_datapathid_from_bytes;
%newobject create_datapathid_from_host;
%newobject create_datapathid_from_net;

%{
    datapathid *create_datapathid_from_bytes(const uint8_t data[6]) {
        return new datapathid(datapathid::from_bytes(data));
    }

    datapathid *create_datapathid_from_host(uint64_t id) {
        return new datapathid(datapathid::from_host(id));
    }

    datapathid *create_datapathid_from_net(uint64_t id) {
        return new datapathid(datapathid::from_net(id));
    }
%}

datapathid *create_datapathid_from_bytes(const uint8_t data[6]);
datapathid *create_datapathid_from_host(uint64_t id);
datapathid *create_datapathid_from_net(uint64_t id);
