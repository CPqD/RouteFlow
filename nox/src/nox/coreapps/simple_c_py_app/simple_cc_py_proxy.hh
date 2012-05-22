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
/* Proxy class to expose simple_c_py_app.hh to Python.
 * This file is only to be included from the SWIG interface file
 * (simple_c_py_proxy.i)
 */

#ifndef PYSIMPLY_C_PY_PROXY_HH__
#define PYSIMPLY_C_PY_PROXY_HH__

#include <Python.h>

#include "simple_cc_py_app.hh"
#include "pyrt/pyglue.hh"

namespace vigil {
namespace applications {

class simple_cc_py_proxy{
public:
    simple_cc_py_proxy(PyObject* ctxt);

    void configure(PyObject*);
    void install(PyObject*);

    // --
    // Proxy public interface methods here!!
    // --

protected:   

    simple_cc_py_app* scpa;
    container::Component* c;
}; // class simple_cc_py_proxy

} // namespace applications
} // namespace vigil

#endif //  PYSIMPLY_C_PY_PROXY_HH__
