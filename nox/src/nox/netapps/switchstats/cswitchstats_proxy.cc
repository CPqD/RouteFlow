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

#include "cswitchstats_proxy.hh"

#include "pyrt/pycontext.hh"
#include "swigpyrun.h"
#include "vlog.hh"

using namespace std;
using namespace vigil;
using namespace vigil::applications;

namespace {
Vlog_module lg("switchstats-proxy");
}

namespace vigil {
namespace applications {

cswitchstats_proxy::cswitchstats_proxy(PyObject* ctxt)
{
    if (!SWIG_Python_GetSwigThis(ctxt) || !SWIG_Python_GetSwigThis(ctxt)->ptr) {
        throw runtime_error("Unable to access Python context.");
    }
    
    c = ((PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr)->c;
}

void
cswitchstats_proxy::configure(PyObject* configuration) 
{
    c->resolve(csstats);    
}

void 
cswitchstats_proxy::install(PyObject*) 
{
}

float 
cswitchstats_proxy::get_global_conn_p_s(void){
    return csstats->get_global_conn_p_s();
}
float 
cswitchstats_proxy::get_switch_conn_p_s(datapathid dpid){
    return csstats->get_switch_conn_p_s(dpid.as_host());
}
float 
cswitchstats_proxy::get_loc_conn_p_s   (datapathid dpid, uint16_t port){
    uint64_t dpint = dpid.as_host();
    uint64_t loc = dpint + (((uint64_t) port) << 48);
    return csstats->get_loc_conn_p_s(loc);
}


}} // vigil::applications
