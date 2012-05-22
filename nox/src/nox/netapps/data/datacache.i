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
%module "nox.netapps.data.pydatacache"

%{
#include "core_events.hh"
#include "pyrt/pycontext.hh"
#include "pyrt/pyevent.hh"
#include "pyrt/pyglue.hh"

#include "principal_event.hh"
#include "pydatacache.hh"

using namespace vigil;
%}

%include "common-defs.i"
%include "std_string.i"

%import "datatypes.i"
%import "pyrt/event.i"

%include "pydatacache.hh"

struct Principal_delete_event
    : public Event
{
    Principal_delete_event(PrincipalType type_,
                           int64_t id_);

    // -- only for use within python
    Principal_delete_event();

    static const Event_name static_get_name();

    PrincipalType type;
    int64_t id;

%pythoncode
%{
    def __str__(self):
        return 'Principal_delete_event '+ 'type: '+str(self.type) +\
               ' , id: ' + str(self.id) +']'
%}

%extend {

    static void fill_python_event(const Event& e, PyObject* proxy) const 
    {
        const Principal_delete_event& pd = dynamic_cast<const Principal_delete_event&>(e);

        pyglue_setattr_string(proxy, "type", to_python(pd.type));
        pyglue_setattr_string(proxy, "id", to_python(pd.id));

        ((Event*)SWIG_Python_GetSwigThis(proxy)->ptr)->operator=(e);
    }

    static void register_event_converter(PyObject *ctxt) {
        if (!SWIG_Python_GetSwigThis(ctxt) || 
            !SWIG_Python_GetSwigThis(ctxt)->ptr) {
            throw std::runtime_error("Unable to access Python context.");
        }
        
        vigil::applications::PyContext* pyctxt = 
            (vigil::applications::PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr;
        pyctxt->register_event_converter<Principal_delete_event>
            (&Principal_delete_event_fill_python_event);
    }
}

};
