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
#include "netinet++/cidr.hh"

using namespace vigil;
%}

%import "ipaddr.i"

struct cidr_ipaddr
{
    ipaddr addr;
    uint32_t mask;

    cidr_ipaddr();
    cidr_ipaddr(const ipaddr&, uint8_t mask_bit_len);
    cidr_ipaddr(const char*);

    bool matches(const ipaddr&) const;

    uint32_t get_prefix_len() const;

    %extend {
       cidr_ipaddr* __copy__(const PyObject *memodict) {
           return new cidr_ipaddr(*$self);
       }
    }

    %extend {
       cidr_ipaddr* __deepcopy__(const PyObject *memodict) {
           return new cidr_ipaddr(*$self);
       }
    }

     %extend {
        char *__str__() {
            static char temp[INET_ADDRSTRLEN + 4];
            sprintf(temp,"%s", self->string().c_str());
            return &temp[0];
        }
     }

    %extend {
        bool operator == (const cidr_ipaddr *other) {
             if (other == NULL)
                 return false;
             return *$self == *other;
        }

        bool operator != (const cidr_ipaddr *other) {
             if (other == NULL)
                 return true;
             return *$self != *other;
        }
    }

    %extend {
        unsigned int __hash__() {
            //is hashing CIDRs into 32 bits a well studied problem?
            return self->addr;
        }
    }

}; // -- struct cidr_ipaddr

%newobject create_cidr_ipaddr;

%{
    cidr_ipaddr *create_cidr_ipaddr(const char *cidr) {
        try {
            return new cidr_ipaddr(cidr);
        } catch (...) {
            return NULL;
        }
    }
%}

cidr_ipaddr *create_cidr_ipaddr(const char *ip);

%include "std_list.i"

%template(cidrlist) std::list<cidr_ipaddr>;
