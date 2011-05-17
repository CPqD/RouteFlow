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
#include "simple_cc_py_app.hh"

#include "vlog.hh"


namespace vigil {

using namespace vigil::container;

static Vlog_module lg("simple-c-py-component");

void 
simple_cc_py_app::configure(const Configuration*) 
{
    // Parse the configuration, register event handlers, and
    // resolve any dependencies.
    lg.dbg("Configure called in C++");
}

void 
simple_cc_py_app::install() 
{
    // Start the component. For example, if any threads require
    // starting, do it now.
    lg.dbg("Install called in C++");
}

void 
simple_cc_py_app::getInstance(const container::Context* ctxt, vigil::simple_cc_py_app*& scpa) {
    scpa = dynamic_cast<simple_cc_py_app*>
        (ctxt->get_by_interface(container::Interface_description
                                (typeid(simple_cc_py_app).name())));
}


} // namespace vigil

namespace {
REGISTER_COMPONENT(vigil::container::Simple_component_factory<vigil::simple_cc_py_app>, 
                   vigil::simple_cc_py_app);
} // unnamed namespace
