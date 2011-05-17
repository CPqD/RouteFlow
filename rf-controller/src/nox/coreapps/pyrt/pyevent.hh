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
//-----------------------------------------------------------------------------
// Generic python event.  Manages passing of C++ objects to python by
// doing some grotesque RTTI.  Exposing a C++ event to Python requires
// the following contortions:
//
// - The C++ event class must inherit from python_compatible_event
//   and override the fill_python_event virtual function
// - create a static Event_type class member appropriately named 
//   (and initialized) within pyevent
// - add any to_python conversion specializations for members of the
//   new type which are to be exposed to python as attributes
//
//
//   TODO: This class should be extended to allow the passing of python
//   events back up to python by adding a PyObject* member which is sent
//   when the event is passed to C++.
//
//-----------------------------------------------------------------------------

#ifndef PYEVENT_HH
#define PYEVENT_HH

#include "pyglue.hh"

#include "event.hh"
#include "assert.hh"

#include <string>
#include <stdexcept>

namespace vigil
{

class pyevent : public Event
{
protected:
    pyevent(const pyevent&);
    
public:
    virtual ~pyevent();

    pyevent();    
    pyevent(Event_name name_);
    pyevent(Event_name name_, PyObject* arg);

    PyObject* python_arg;
};

//-----------------------------------------------------------------------------
inline
pyevent::pyevent() : Event(""), python_arg(NULL)
{ 
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
inline
pyevent::pyevent(Event_name name_) : 
    Event(name_), python_arg(NULL)
{ 
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
inline
pyevent::pyevent(Event_name name_, PyObject* arg) : 
    Event(name_), python_arg(arg)
{ 
    Py_INCREF(python_arg);
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
inline
pyevent::~pyevent()
{ 
    Py_XDECREF(python_arg);
}
//-----------------------------------------------------------------------------

}
#endif  // -- PYEVENT_HH
