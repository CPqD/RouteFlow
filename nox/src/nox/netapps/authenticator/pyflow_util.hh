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
#ifndef PYFLOW_UTIL_HH
#define PYFLOW_UTIL_HH 1

#include <Python.h>

#include "flow_util.hh"
#include "component.hh"


namespace vigil {
namespace applications {

class PyFlow_util {
public:
    PyFlow_util(PyObject*);

    void configure(PyObject*);

    // Return true if valid args for Flow_fn key
    bool valid_fn_args(const std::string& key,
                       const std::list<std::string>&);
    // Set action argument to Flow_fn that arg maps to
    bool set_action_argument(Flow_action&, const std::string& arg,
                             const std::list<std::string>& fn_args);

private:
    Flow_util* flow_util;
    container::Component* c;
};

}
}

#endif
