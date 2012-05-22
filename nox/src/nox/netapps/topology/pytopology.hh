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
#ifndef pytopology_proxy_HH
#define pytopology_proxy_HH

#include <Python.h>

#include "topology.hh"
#include "pyrt/pyglue.hh"

namespace vigil {
namespace applications {

struct PyLinkPorts {
    uint16_t src;
    uint16_t dst;

    PyLinkPorts(){;}
    PyLinkPorts(uint16_t s, uint16_t d):
        src(s), dst(d){
        }
};


class pytopology_proxy{
public:
    pytopology_proxy(PyObject* ctxt);

    void configure(PyObject*);
    void install(PyObject*);

    std::list<PyLinkPorts> get_outlinks(datapathid dpsrc, datapathid dpdst) const;
    bool is_internal(datapathid dp, uint16_t port) const;

protected:   

    Topology* topo;
    container::Component* c;
}; // class pytopology_proxy


} // applications
} // vigil

#endif  // pytopology_proxy_HH
