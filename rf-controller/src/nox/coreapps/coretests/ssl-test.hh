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

#ifndef SSL_TEST_HH__
#define SSL_TEST_HH__

#include "component.hh"
#include "config.h"

#include "json_object.hh"

using namespace vigil;
using namespace vigil::container;

class SSLTest
    : public Component {

public:
    SSLTest(const Context* c, const json_object*)
        : Component(c) {
    }

    void configure(const Configuration*);

    Disposition handle_bootstrap(const Event& e);

    void install();

private:

};

REGISTER_COMPONENT(container::Simple_component_factory<SSLTest>,
                   SSLTest);

#endif // SSL_TEST_HH__
