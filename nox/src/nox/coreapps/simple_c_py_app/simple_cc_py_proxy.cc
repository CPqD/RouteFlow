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

#include "simple_cc_py_proxy.hh"

#include "pyrt/pycontext.hh"
#include "swigpyrun.h"
#include "vlog.hh"

using namespace std;
using namespace vigil;
using namespace vigil::applications;

namespace {
Vlog_module lg("simple-c-py-proxy");
}

namespace vigil {
namespace applications {

/*
 * Get a pointer to the runtime context so we can resolve 
 * simply_c_py_app at configure time.
 */

simple_cc_py_proxy::simple_cc_py_proxy(PyObject* ctxt)
{
    if (!SWIG_Python_GetSwigThis(ctxt) || !SWIG_Python_GetSwigThis(ctxt)->ptr) {
        throw runtime_error("Unable to access Python context.");
    }
    
    c = ((PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr)->c;
}

/*
 * Get a handle to the simple_cc_py_app container on the C++ side.
 */

void
simple_cc_py_proxy::configure(PyObject* configuration) 
{
    c->resolve(scpa);    
    lg.dbg("Configure called in c++ wrapper");
}

void 
simple_cc_py_proxy::install(PyObject*) 
{
    lg.dbg("Install called in c++ wrapper");
}

} // namespace applications
} // namespace vigil
