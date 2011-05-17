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
#include "netinet++/ethernetaddr.hh"
%}

struct ethernetaddr
{
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    static const  unsigned int   LEN =   6;

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    uint8_t     octet[ethernetaddr::LEN];

    //-------------------------------------------------------------------------
    // Constructors/Detructor
    //-------------------------------------------------------------------------
    
    ethernetaddr();
    ethernetaddr(uint64_t id);
    ethernetaddr(const ethernetaddr&);

    // ------------------------------------------------------------------------
    // String Representation
    // ------------------------------------------------------------------------

    uint64_t    hb_long() const;
    uint64_t    nb_long() const;

    bool operator <  (const ethernetaddr&) const;
    bool operator <= (const ethernetaddr&) const;
    bool operator >  (const ethernetaddr&) const;
    bool operator >= (const ethernetaddr&) const;

    //-------------------------------------------------------------------------
    // Method: private(..)
    //
    // Check whether the private bit is set
    //-------------------------------------------------------------------------
    bool is_private() const;

    //-------------------------------------------------------------------------
    // Method: is_multicast(..)
    //
    // Check whether the multicast bit is set
    //-------------------------------------------------------------------------
    bool is_multicast() const;
    bool is_broadcast() const;

    bool is_zero() const;

    %extend {
       ethernetaddr* __copy__(const PyObject *memodict) {
           return new ethernetaddr(*$self);
       }
    }

    %extend {
       ethernetaddr* __deepcopy__(const PyObject *memodict) {
           return new ethernetaddr(*$self);
       }
    }

    %extend {
       char *__str__() {
           static char temp[64];
           sprintf(temp,"%s", self->string().c_str());
           return &temp[0];
       }
    }

    %extend {
       PyObject *binary_str() {
           PyObject *ret = Py_BuildValue("s#", $self->octet, ethernetaddr::LEN);
           return ret;
       }
    }

    %extend {
       bool __nonzero__() {
           return (!self->is_zero());
       }
    }

    %extend {
        bool operator == (const ethernetaddr *other) {
             if (other == NULL)
                 return false;
             return *$self == *other;
        }

        bool operator != (const ethernetaddr *other) {
             if (other == NULL)
                 return true;
             return *$self != *other;
        }
    }

    %extend {
        unsigned int __hash__() {
            return self->hb_long();
        }
    }
};

%newobject create_bin_eaddr;
%newobject create_eaddr;

%{
    ethernetaddr *create_bin_eaddr(const char *data) {
        try {
            ethernetaddr *e = new ethernetaddr();
            memcpy(e->octet, data, ethernetaddr::LEN);
            return e;
        } catch (...) {
            return NULL;
        }
    }

    ethernetaddr *create_eaddr(const char *str) {
        try {
            ethernetaddr *e = new ethernetaddr(str);
            return e;
        } catch (...) {
            return NULL;
        }
    }

    ethernetaddr *create_eaddr(uint64_t id) {
        try {
            return new ethernetaddr(id);
        } catch (...) {
            return NULL;
        }
    }

    ethernetaddr *create_eaddr(const ethernetaddr& addr) {
        try {
            return new ethernetaddr(addr);
        } catch (...) {
            return NULL;
        }
    }
%}

ethernetaddr *create_bin_eaddr(const char *data);
ethernetaddr *create_eaddr(const char *str);
ethernetaddr *create_eaddr(uint64_t id);
ethernetaddr *create_eaddr(const ethernetaddr& addr);

%include "std_list.i"

%template(ethlist) std::list<ethernetaddr>;
