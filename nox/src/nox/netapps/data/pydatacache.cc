/* Copyright 2009 (C) Nicira, Inc.
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
#include "pydatacache.hh"

#include "swigpyrun.h"
#include "pyrt/pycontext.hh"

namespace vigil {
namespace applications {

PyData_cache::PyData_cache(PyObject* ctxt)
    : data_cache(0)
{
    if (!SWIG_Python_GetSwigThis(ctxt) || !SWIG_Python_GetSwigThis(ctxt)->ptr) {
        throw std::runtime_error("Unable to access Python context.");
    }

    c = ((applications::PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr)->c;
}

void
PyData_cache::configure(PyObject* configuration)
{
    c->resolve(data_cache);
}

int64_t
PyData_cache::get_authenticated_id(PrincipalType ptype) const
{
    return data_cache->get_authenticated_id(ptype);
}

int64_t
PyData_cache::get_unauthenticated_id(PrincipalType ptype) const
{
    return data_cache->get_authenticated_id(ptype);
}

int64_t
PyData_cache::get_unknown_id(PrincipalType ptype) const
{
    return data_cache->get_authenticated_id(ptype);
}

const std::string&
PyData_cache::get_authenticated_name() const
{
    return data_cache->get_authenticated_name();
}

const std::string&
PyData_cache::get_unauthenticated_name() const
{
    return data_cache->get_unauthenticated_name();
}

const std::string&
PyData_cache::get_unknown_name() const
{
    return data_cache->get_unknown_name();
}

const std::string&
PyData_cache::get_name(PrincipalType type, int64_t id) const
{
    Principal principal = { type, id };
    return data_cache->get_name_ref(principal);
}

}
}
