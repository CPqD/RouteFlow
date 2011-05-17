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
#include "netinet++/ipaddr.hh"

using namespace vigil;
%}

struct ipaddr
{
    uint32_t addr;

    // ------------------------------------------------------------------------
    // Constructors 
    // ------------------------------------------------------------------------

    ipaddr();
    ipaddr(uint32_t);
    ipaddr(const ipaddr&);
    ipaddr(const char*);

    // ------------------------------------------------------------------------
    // Binary Operators 
    // ------------------------------------------------------------------------

    ipaddr  operator ~  ();
    ipaddr  operator &  (const ipaddr&);
    ipaddr  operator &  (int);
    ipaddr& operator &= (const ipaddr&);
    ipaddr& operator &= (uint32_t);
    ipaddr  operator |  (const ipaddr&);
    ipaddr  operator |  (uint32_t);
    ipaddr& operator |= (const ipaddr&);
    ipaddr& operator |= (uint32_t);

    // ------------------------------------------------------------------------
    // Mathematical operators
    // ------------------------------------------------------------------------

    ipaddr operator += (int);
    ipaddr operator +  (int);

    int    operator -  (const ipaddr &);
    ipaddr operator -  (int);

    // ------------------------------------------------------------------------
    // Comparison Operators
    // ------------------------------------------------------------------------

    bool operator == (uint32_t);
    bool operator != (uint32_t);
    bool operator <  (uint32_t);
    bool operator <  (const ipaddr&);
    bool operator <= (uint32_t);
    bool operator <= (const ipaddr&);
    bool operator >  (uint32_t);
    bool operator >  (const ipaddr&);
    bool operator >= (uint32_t);
    bool operator >= (const ipaddr&);

    %extend {
       ipaddr* __copy__(const PyObject *memodict) {
           return new ipaddr(*$self);
       }
    }

    %extend {
       ipaddr* __deepcopy__(const PyObject *memodict) {
           return new ipaddr(*$self);
       }
    }

     %extend {
        char *__str__() {
            static char temp[INET_ADDRSTRLEN];
            sprintf(temp,"%s", self->string().c_str());
            return &temp[0];
        }
     }

     %extend {
        bool __nonzero__() {
            return (self->addr != 0);
        }
     }

    %extend {
        bool operator == (const ipaddr *other) {
             if (other == NULL)
                 return false;
             return *$self == *other;
        }

        bool operator != (const ipaddr *other) {
             if (other == NULL)
                 return true;
             return *$self != *other;
        }
    }

    %extend {
        unsigned int __hash__() {
            return self->addr;
        }
    }
}; // -- struct ipaddr

%newobject create_bin_ipaddr;
%newobject create_ipaddr;

%{
    ipaddr *create_bin_ipaddr(const char *data) {
        try {
            ipaddr *ip = new ipaddr();
            memcpy(&ip->addr, data, sizeof ip->addr);
            return ip;
        } catch (...) {
            return NULL;
        }
    }

    ipaddr *create_ipaddr(uint32_t ip) {
        try {
            ipaddr *i = new ipaddr(ip);
            return i;
        } catch (...) {
            return NULL;
        }
    }

    ipaddr *create_ipaddr(const ipaddr &ip) {
        try {
            ipaddr *i = new ipaddr(ip);
            return i;
        } catch (...) {
            return NULL;
        }
    }

    ipaddr *create_ipaddr(const char *ip) {
        try {
            ipaddr *i = new ipaddr(ip);
            return i;
        } catch (...) {
            return NULL;
        }
    }
%}

ipaddr *create_bin_ipaddr(const char *data);
ipaddr *create_ipaddr(uint32_t ip);
ipaddr *create_ipaddr(const ipaddr &ip);
ipaddr *create_ipaddr(const char *ip);
