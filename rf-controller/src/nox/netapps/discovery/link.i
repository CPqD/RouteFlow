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
%{
#include "core_events.hh"
#include "link-event.hh"
#include "pyrt/pycontext.hh"
#include "pyrt/pyevent.hh"
#include "pyrt/pyglue.hh"

using namespace vigil;
%}

%import "netinet/netinet.i"
%import "pyrt/event.i"

%include "common-defs.i"
%include "std_string.i"
%include "cstring.i"

struct Link_event
    : public Event
{
    enum Action {
         ADD,
         REMOVE
    };

    Link_event(datapathid dpsrc_, datapathid dpdst_,
               uint16_t sport_, uint16_t dport_,
               Action action_);

    Link_event();

    datapathid dpsrc;
    datapathid dpdst;
    uint16_t sport;
    uint16_t dport;
    Action action;

    static const std::string static_get_name();

%pythoncode
%{
    def __str__(self):
        action_map = {0:'ADD',1:'REMOVE'}
        return 'Link_event '+action_map[self.action]+' [dpsrc: '\
               +str(self.dpsrc)+' sport: ' + str(self.sport) + ' dpdst: '\
               +str(self.dpdst)+' dport: ' + str(self.dport) + ']'
%}

%extend {
    static void fill_python_event(const Event& e, PyObject* proxy) 
    {
        const Link_event& le = dynamic_cast<const Link_event&>(e);
        
        pyglue_setattr_string(proxy, "dpsrc", to_python(le.dpsrc));
        pyglue_setattr_string(proxy, "dpdst", to_python(le.dpdst));
        pyglue_setattr_string(proxy, "sport", to_python(le.sport));
        pyglue_setattr_string(proxy, "dport", to_python(le.dport));
        pyglue_setattr_string(proxy, "action",
                              PyString_FromString(
                                                  le.action == Link_event::ADD ? "add"
                                                  : le.action == Link_event::REMOVE ? "remove"
                                                  : "unknown"));

        ((Event*)SWIG_Python_GetSwigThis(proxy)->ptr)->operator=(e);
    }

    static void register_event_converter(PyObject *ctxt) {
        if (!SWIG_Python_GetSwigThis(ctxt) || 
            !SWIG_Python_GetSwigThis(ctxt)->ptr) {
            throw std::runtime_error("Unable to access Python context.");
        }
        
        vigil::applications::PyContext* pyctxt = 
            (vigil::applications::PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr;
        pyctxt->register_event_converter<Link_event>
            (&Link_event_fill_python_event);
    }
};

};
