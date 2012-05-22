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

%module "nox.netapps.topology"

%{
#include "pytopology.hh"
#include "pyrt/pycontext.hh"
using namespace vigil;
using namespace vigil::applications;
%}

%import "netinet/netinet.i"

%include "common-defs.i"
%include "std_list.i"

struct PyLinkPorts {
    uint16_t src;
    uint16_t dst;
};

%template(LLinkSet) std::list<PyLinkPorts>;

class pytopology_proxy{
public:

    pytopology_proxy(PyObject* ctxt);

    void configure(PyObject*);
    void install(PyObject*);

    std::list<PyLinkPorts> get_outlinks(datapathid dpsrc, datapathid dpdst) const;
    bool is_internal(datapathid dp, uint16_t port) const;

protected:   

    topology* topo;
    container::Component* c;
}; // class pytopology_proxy


%pythoncode
%{
from nox.lib.core import Component

class pytopology(Component):
    """
    An adaptor over the C++ based Python bindings to
    simplify their implementation.
    """  
    def __init__(self, ctxt):
        self.pytop = pytopology_proxy(ctxt)

    def configure(self, configuration):
        self.pytop.configure(configuration)

    def install(self):
        pass

    def getInterface(self):
        return str(pytopology)

    def get_outlinks(self, dpsrc, dpdst):
        return self.pytop.get_outlinks(dpsrc, dpdst)

    def is_internal(self, dp, port):
        return self.pytop.is_internal(dp, port)


def getFactory():
    class Factory():
        def instance(self, context):
                    
            return pytopology(context)

    return Factory()
%}
