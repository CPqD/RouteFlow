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
#ifndef PYRT_HH
#define PYRT_HH 1

#include <Python.h>

#include <boost/intrusive_ptr.hpp>

#include "component.hh"
#include "deployer.hh"
#include "threads/cooperative.hh"

namespace vigil {
namespace applications {

/* Python event manager manages the event specific converters, both
   their registration and their user.
 */
class Python_event_manager {
public:
    typedef boost::function<void(const Event&, PyObject*)> Event_converter;

    virtual ~Python_event_manager();

    /* Register a function callback capable of transforming a C++
       event into a Python event. */
    virtual void register_event_converter(const Event_name&,
                                          const Event_converter&);

    /* Invokes the Python callable with a given event. */
    virtual Disposition call_python_handler(const Event&, 
                                            boost::intrusive_ptr<PyObject>&);

    /* Creates a Python context object containing C++ PyContext object */
    virtual PyObject* create_python_context(const container::Context*, 
                                            container::Component*);
private:
    /* Event converters */
    hash_map<Event_name, Event_converter> converters;
};

/* Python runtime component is a deployer responsible for Python
 * components.  In addition, it initializes the Python runtime before
 * it attempts to install any Python components.
 */
class PyRt
    : public container::Component, 
      public Deployer,
      public Python_event_manager {
public:
    PyRt(const container::Context*, const json_object*);
    
    static void getInstance(const container::Context*, PyRt*&);

    void configure(const container::Configuration*);
    void install();

protected:
    void initialize_oxide_reactor();
};

class Python_component_context 
    : public Component_context {
public:
    Python_component_context(Kernel*, const container::Component_name&, 
                             const std::string& component_home_path, 
                             json_object*);
                             
    /* Actions implementing state transitions */
    void describe();
    void load();
    void instantiate_factory();
    void instantiate();
    void configure();
    void install();
    
private:
    container::Configuration* configuration;
};

/* Pretty-prints the current Python exception into a string. */
const std::string pretty_print_python_exception();

} 
}

#endif
