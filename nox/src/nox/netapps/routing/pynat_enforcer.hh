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
#ifndef PYNAT_ENFORCER_GLUE_HH
#define PYNAT_ENFORCER_GLUE_HH 1

#include <Python.h>

#include "component.hh"
#include "nat_enforcer.hh"

namespace vigil {
namespace applications {

class PyNAT_enforcer {
public:
    PyNAT_enforcer(PyObject* ctxt);

    void configure(PyObject*);

    uint32_t add_rule(uint32_t priority, const Flow_expr&, const Flow_action&);
    void build();
    bool change_rule_priority(uint32_t id, uint32_t priority);
    bool delete_rule(uint32_t id);
    void reset();

private:
    NAT_enforcer* enforcer;
    container::Component* c;
};

}
}

#endif
