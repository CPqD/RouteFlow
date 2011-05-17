/* Copyright 2008, 2009 (C) Nicira, Inc.
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
#include "pyflow_util.hh"

#include "swigpyrun.h"
#include "pyrt/pycontext.hh"

namespace vigil {
namespace applications {

PyFlow_util::PyFlow_util(PyObject* ctxt)
    : flow_util(0)
{
    if (!SWIG_Python_GetSwigThis(ctxt) || !SWIG_Python_GetSwigThis(ctxt)->ptr) {
        throw std::runtime_error("Unable to access Python context.");
    }

    c = ((PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr)->c;
}

void
PyFlow_util::configure(PyObject* configuration) {
    c->resolve(flow_util);
}

bool
PyFlow_util::valid_fn_args(const std::string& key,
                           const std::list<std::string>& fn_args)
{
    return flow_util->fns.valid_fn_args(key,
                                            std::vector<std::string>(fn_args.begin(),
                                                                     fn_args.end()));
}

bool
PyFlow_util::set_action_argument(Flow_action& action,
                                 const std::string& arg,
                                 const std::list<std::string>& fn_args)
{
    if (action.get_type() != Flow_action::C_FUNC) {
        return false;
    }

    Flow_action::C_func_t fn = { arg,
                                 flow_util->fns.get_function
                                 (arg, std::vector<std::string>(fn_args.begin(),
                                                                fn_args.end())) } ;
    if (fn.fn.empty()) {
        return false;
    }

    return action.set_arg(fn);
}

}
}

