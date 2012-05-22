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
#include "pynat_enforcer.hh"

#include <boost/bind.hpp>
#include "swigpyrun.h"
#include "pyrt/pycontext.hh"

namespace vigil {
namespace applications {

PyNAT_enforcer::PyNAT_enforcer(PyObject* ctxt)
    : enforcer(0)
{
    if (!SWIG_Python_GetSwigThis(ctxt) || !SWIG_Python_GetSwigThis(ctxt)->ptr) {
        throw std::runtime_error("Unable to access Python context.");
    }

    c = ((PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr)->c;
}

void
PyNAT_enforcer::configure(PyObject* configuration)
{
    c->resolve(enforcer);
}

uint32_t
PyNAT_enforcer::add_rule(uint32_t priority, const Flow_expr& expr,
                         const Flow_action& action)
{
    return enforcer->add_rule(priority, expr, action);
}

void
PyNAT_enforcer::build()
{
    enforcer->build();
}

bool
PyNAT_enforcer::change_rule_priority(uint32_t id, uint32_t priority)
{
    return enforcer->change_rule_priority(id, priority);
}

bool
PyNAT_enforcer::delete_rule(uint32_t id)
{
    return enforcer->delete_rule(id);
}

void
PyNAT_enforcer::reset()
{
    enforcer->reset();
}

}
}
