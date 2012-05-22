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


/*  Expose a subset of the kernel interface to python. This allows
 *  components to gain access to the list of components and their states
 *
 */

%{
#include "kernel.hh"
using namespace vigil;
%}

%include "std_string.i"
%include "std_list.i"

/* force swig to return time_t by value */
typedef long time_t;

/* Valid component states. */
enum Component_state { 
    NOT_INSTALLED = 0,        /* Component has been discovered. */
    DESCRIBED = 1,            /* Component (XML) description has been
                                 processed. */
    LOADED = 2,               /* Component library has been loaded, if
                                 necessary. */
    FACTORY_INSTANTIATED = 3, /* Component factory has been
                                 instantiated from the library. */
    INSTANTIATED = 4,         /* Component itself has been
                                 instantiated. */
    CONFIGURED = 5,           /* Component's configure() has been
                                 called. */
    INSTALLED = 6,            /* Component's install() has been
                                 called. */
    ERROR = 7                 /* Any state transition can lead to
                                 error state, if an unrecovarable
                                 error has occured. */
};

class Component_context {

public:
    Component_context(Kernel*);
    virtual ~Component_context();

    Component_state get_state() const;
    Component_state get_required_state() const; 

    /* Return a human-readable status report. */
    std::string get_status() const;

    /* Public container::Context methods to be provided for components. */
    std::string get_name() const;

};

namespace std{
    %template(CContextList) list<Component_context*>;
}

%nodefaultctor Kernel;
class Kernel {
    public:
        /* time in seconds since boot */
        time_t uptime() const;

        typedef uint64_t UUID;
        UUID uuid() const;
        uint64_t restarts() const;

        std::list<Component_context*> get_all() const;
    private:
        Kernel();
};
