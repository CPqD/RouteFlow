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
%module "nox.netapps.routing.pynatenforcer"

%{
#include "pynat_enforcer.hh"
using namespace vigil;
using namespace vigil::applications;
%}

%include "common-defs.i"

%import "authenticator/flow_util.i"
%include "pynat_enforcer.hh"

%pythoncode
%{
    from nox.lib.core import Component

    class PyNatEnforcer(Component):
        def __init__(self, ctxt):
            self.enforcer = PyNAT_enforcer(ctxt)

        def configure(self, configuration):
            self.enforcer.configure(configuration)

        def getInterface(self):
            return str(PyNatEnforcer)

        def add_rule(self, pri, expr, action):
            return self.enforcer.add_rule(pri, expr, action)

        def build(self):
            self.enforcer.build()

        def change_rule_priority(self, id, priority):
            return self.enforcer.change_rule_priority(id, priority)

        def delete_rule(self, id):
            return self.enforcer.delete_rule(id)

        def reset(self):
            self.enforcer.reset()

    def getFactory():
        class Factory():
            def instance(self, context):
                return PyNatEnforcer(context)

        return Factory()

%}
