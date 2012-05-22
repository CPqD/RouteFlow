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
/*
 * Simple example of C++ component that contains a python front-end. 
 */

#ifndef SIMPLE_C_APP_HH__
#define SIMPLE_C_APP_HH__

#include "component.hh"

namespace vigil {

using namespace vigil::container;    

class simple_cc_py_app
    : public Component {
public:
    simple_cc_py_app(const Context* c,
                    const json_object*) 
        : Component(c) {
    }

    virtual ~simple_cc_py_app() { ; }

    void configure(const Configuration*);
    void install();

    static void getInstance(const container::Context*, vigil::simple_cc_py_app*&);

private:
    
};

} // namespace vigil

#endif  // -- SIMPLE_C_APP_HH__
