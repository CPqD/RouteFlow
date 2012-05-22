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
#include "static-deployer.hh"

#include <boost/bind.hpp>

#include "deployer.hh"

using namespace vigil;
using namespace vigil::container;
using namespace std;

Static_component_context::
Static_component_context(Kernel* kernel, 
                         const Component_name& name, 
                         const Constructor_callback& constructor_,
                         const container::Interface_description& interface,
                         json_object* platform_conf)
    : Component_context(kernel), constructor(constructor_), factory(0) {    
    init_actions(name, interface, platform_conf);
}

Static_component_context::
Static_component_context(Kernel* kernel, 
                         const Component_name& name, 
                         const container::Component_factory* f,
                         json_object* platform_conf)
    : Component_context(kernel), constructor(0), factory(f) {    
    init_actions(name, f->get_interface(), platform_conf);
}

void
Static_component_context::init_actions(const Component_name& name,
                                       const Interface_description& interface,
                                       json_object* platform_conf) {
    using namespace boost;

    install_actions[DESCRIBED] = 
        bind(&Static_component_context::describe, this);
    install_actions[LOADED] = bind(&Static_component_context::load, this);
    install_actions[FACTORY_INSTANTIATED] = 
        bind(&Static_component_context::instantiate_factory, this);
    install_actions[INSTANTIATED] = 
        bind(&Static_component_context::instantiate, this);
    install_actions[CONFIGURED] = 
        bind(&Static_component_context::configure, this);
    install_actions[INSTALLED] = 
        bind(&Static_component_context::install, this);

    this->name = name;
    this->configuration = new Component_configuration();
    this->json_description = platform_conf;
    this->interface = interface;
}

void 
Static_component_context::describe() {
    /* Dependencies were introduced in the constructor */
    current_state = DESCRIBED;
}

void 
Static_component_context::load() {
    /* Statically linked components don't require loading */
    current_state = LOADED;
}

void 
Static_component_context::instantiate_factory() {
    /* Statically linked components don't require factory instantiating */
    current_state = FACTORY_INSTANTIATED;
}

void 
Static_component_context::instantiate() {
    try {
        component = factory ? 
            factory->instance(this, json_description) :
            constructor(this, json_description);
        current_state = INSTANTIATED;
    }
    catch (const std::exception& e) {
        error_message = e.what();
        current_state = ERROR;
    }
}

void 
Static_component_context::configure() {
    try {
        component->configure(configuration);
        current_state = CONFIGURED;
    }
    catch (const std::exception& e) {
        error_message = e.what();
        current_state = ERROR;
    }
}

void 
Static_component_context::install() {
    try {
        component->install();
        current_state = INSTALLED;
    }
    catch (const std::exception& e) {
        error_message = e.what();
        current_state = ERROR;
    }
}
