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
#ifndef CONTROLLER_PYCOMPONENT_HH
#define CONTROLLER_PYCOMPONENT_HH 1

#include <Python.h>

#include "component.hh"

namespace vigil {
namespace applications {

/**
 * The components manages and interacts with the Python side component
 * implementation.
 */
class PyComponent
    : public container::Component {
public:
    PyComponent(const container::Context*, const json_object*);
    PyComponent(const container::Context*, PyObject*);
    ~PyComponent();

    void configure(const container::Configuration*);
    void install();

    /* TODO: convenience methods to access the container */

    /* Gets the interface description */
    container::Interface_description get_interface() const;

    /* Gets the Python object providing the component services. */
    PyObject* getPyObject();

protected:
    /* Initialization routines shared by the constructors */
    void init();

    PyObject* pyobj;
    PyObject* pyctxt;

    /* Python component interface translated into a string */
    std::string py_interface;
};

} // namespace applications
} // namespace vigil

#endif 
