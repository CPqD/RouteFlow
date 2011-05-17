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

#include "pytopology.hh"

#include "pyrt/pycontext.hh"
#include "swigpyrun.h"
#include "vlog.hh"

using namespace std;
using namespace vigil;
using namespace vigil::applications;

namespace {
Vlog_module lg("pytopology");
}

pytopology_proxy::pytopology_proxy(PyObject* ctxt)
{
    if (!SWIG_Python_GetSwigThis(ctxt) || !SWIG_Python_GetSwigThis(ctxt)->ptr) {
        throw runtime_error("Unable to access Python context.");
    }
    
    c = ((PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr)->c;
}

void
pytopology_proxy::configure(PyObject* configuration) 
{
    c->resolve(topo);    
}

void 
pytopology_proxy::install(PyObject*) 
{
}

bool 
pytopology_proxy::is_internal(datapathid dp, uint16_t port) const
{
    return topo->is_internal(dp, port);
}

std::list<PyLinkPorts> 
pytopology_proxy::get_outlinks(datapathid dpsrc, datapathid dpdst) const
{
    std::list<PyLinkPorts> returnme;
    std::list<Topology::LinkPorts>  list;

    list =  topo->get_outlinks(dpsrc, dpdst);
    for(std::list<Topology::LinkPorts>::iterator i = list.begin(); 
            i != list.end(); ++i){
        returnme.push_back(PyLinkPorts((*i).src,(*i).dst));
    }
    return returnme; 
}
