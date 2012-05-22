/* Copyright 2008, 2009 (C) Nicira, Inc.
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

%module "nox.netapps.authenticator.pyauth"

%{
#include "core_events.hh"
#include "pyrt/pycontext.hh"
#include "pyrt/pyevent.hh"
#include "pyrt/pyglue.hh"

#include "authenticator.hh"
#include "host_event.hh"
#include "switch_event.hh"
#include "pyauth.hh"
#include "user_event.hh"

using namespace vigil;
using namespace vigil::applications;
%}

%import "netinet/netinet.i"
%import "pyrt/event.i"

%include "common-defs.i"
%include "std_list.i"
%template(grouplist) std::list<int64_t>;

%include "pyauth.hh"

/*
 * Exposes host events, user events, and Authenticator to Python
 */

/* Possible reasons for various host events. */

struct Host_event {
    enum Reason {
        AUTHENTICATION_EVENT,     // Add reasons
        AUTO_AUTHENTICATION,
        NWADDR_AUTO_ADD,
        DEAUTHENTICATION_EVENT,   // Remove reasons
        NWADDR_AUTO_REMOVE,       // want this?
        INTERNAL_LOCATION,
        BINDING_CHANGE,
        HARD_TIMEOUT,
        IDLE_TIMEOUT,
        SWITCH_LEAVE,
        LOCATION_LEAVE,
        HOST_DELETE,
        HOST_NETID_DELETE
    };
};

struct Host_auth_event
    : public Event
{
    enum Action {
        AUTHENTICATE,
        DEAUTHENTICATE,
    };

    enum Enabled_field {
        EF_SWITCH,
        EF_LOCATION,
        EF_DLADDR,
        EF_NWADDR,
        EF_HOSTNAME,
        EF_HOST_NETID,
        EF_ALL
    };

    // AUTHENTICATE constructor
    Host_auth_event(datapathid datapath_id_, uint16_t port_,
                    ethernetaddr dladdr_, uint32_t nwaddr_, int64_t hostname_,
                    int64_t host_netid_, uint32_t idle_timeout_,
                    uint32_t hard_timeout_, Host_event::Reason reason_);

    // DEAUTHENTICATE constructor
    Host_auth_event(datapathid datapath_id_, uint16_t port_,
                    ethernetaddr dladdr_, uint32_t nwaddr_, int64_t hostname_,
                    int64_t host_netid_, uint32_t enabled_fields_,
                    Host_event::Reason reason_);

    // -- only for use within python
    Host_auth_event();

    static const Event_name static_get_name();

    Action              action;
    datapathid          datapath_id;
    uint16_t            port;
    ethernetaddr        dladdr;
    uint32_t            nwaddr;         // set to zero if no IP to auth
    int64_t             hostname;
    int64_t             host_netid;
    uint32_t            idle_timeout;
    uint32_t            hard_timeout;
    uint32_t            enabled_fields; // bit_mask of fields to observe
    Host_event::Reason  reason;

%extend {
    static void fill_python_event(const Event& e, PyObject* proxy) const 
    {
        const Host_auth_event& ha = dynamic_cast<const Host_auth_event&>(e);

        pyglue_setattr_string(proxy, "action", to_python((uint32_t)(ha.action)));
        pyglue_setattr_string(proxy, "datapath_id", to_python(ha.datapath_id));
        pyglue_setattr_string(proxy, "port", to_python(ha.port));
        pyglue_setattr_string(proxy, "dladdr", to_python(ha.dladdr));
        pyglue_setattr_string(proxy, "nwaddr", to_python(ha.nwaddr));
        pyglue_setattr_string(proxy, "hostname", to_python(ha.hostname));
        pyglue_setattr_string(proxy, "host_netid", to_python(ha.host_netid));
        pyglue_setattr_string(proxy, "idle_timeout", to_python(ha.idle_timeout));
        pyglue_setattr_string(proxy, "hard_timeout", to_python(ha.hard_timeout));
        pyglue_setattr_string(proxy, "enabled_fields", to_python(ha.enabled_fields));
        pyglue_setattr_string(proxy, "reason", to_python((uint32_t)(ha.reason)));

        ((Event*)SWIG_Python_GetSwigThis(proxy)->ptr)->operator=(e);
    }

    static void register_event_converter(PyObject *ctxt) {
        if (!SWIG_Python_GetSwigThis(ctxt) || 
            !SWIG_Python_GetSwigThis(ctxt)->ptr) {
            throw std::runtime_error("Unable to access Python context.");
        }
        
        vigil::applications::PyContext* pyctxt = 
            (vigil::applications::PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr;
        pyctxt->register_event_converter<Host_auth_event>
            (&Host_auth_event_fill_python_event);
    }
}

};

struct Host_bind_event
    : public Event
{
    enum Action {
        ADD,
        REMOVE
    };

    // Location binding constructor
    Host_bind_event(Action action_, datapathid datapath_id_, uint16_t port_,
                    int64_t switchname_, int64_t locname_,
                    ethernetaddr dladdr_, int64_t hostname_,
                    int64_t host_netid_, Host_event::Reason reason_);

    // Nwaddr binding constructor
    Host_bind_event(Action action_, ethernetaddr dladdr_, uint32_t nwaddr_,
                    int64_t hostname_, int64_t host_netid_,
                    Host_event::Reason reason_);

    // -- only for use within python
    Host_bind_event();

    static const Event_name static_get_name();

    Action              action;
    datapathid          datapath_id;  // set to zero if dladdr or nwaddr binding
    uint16_t            port;
    int64_t             switchname;
    int64_t             locname;
    ethernetaddr        dladdr;       // should never be 0!
    uint32_t            nwaddr;       // set to zero if loc or dladdr binding
    int64_t             hostname;
    int64_t             host_netid;
    Host_event::Reason  reason;

%extend {
    static void fill_python_event(const Event& e, PyObject* proxy) const 
    {
        const Host_bind_event& hb = dynamic_cast<const Host_bind_event&>(e);

        pyglue_setattr_string(proxy, "action", to_python((uint32_t)(hb.action)));
        pyglue_setattr_string(proxy, "datapath_id", to_python(hb.datapath_id));
        pyglue_setattr_string(proxy, "port", to_python(hb.port));
        pyglue_setattr_string(proxy, "switchname", to_python(hb.switchname));
        pyglue_setattr_string(proxy, "locname", to_python(hb.locname));
        pyglue_setattr_string(proxy, "dladdr", to_python(hb.dladdr));
        pyglue_setattr_string(proxy, "nwaddr", to_python(hb.nwaddr));
        pyglue_setattr_string(proxy, "hostname", to_python(hb.hostname));
        pyglue_setattr_string(proxy, "host_netid", to_python(hb.host_netid));
        pyglue_setattr_string(proxy, "reason", to_python((uint32_t)(hb.reason)));

        ((Event*)SWIG_Python_GetSwigThis(proxy)->ptr)->operator=(e);
    }

    static void register_event_converter(PyObject *ctxt) {
        if (!SWIG_Python_GetSwigThis(ctxt) || 
            !SWIG_Python_GetSwigThis(ctxt)->ptr) {
            throw std::runtime_error("Unable to access Python context.");
        }
        
        vigil::applications::PyContext* pyctxt = 
            (vigil::applications::PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr;
        pyctxt->register_event_converter<Host_bind_event>
            (&Host_bind_event_fill_python_event);
    }
}

};

struct Host_join_event
    : public Event
{
    enum Action {
        JOIN,
        LEAVE
    };

    Host_join_event(Action action_, int64_t hostname_,
                    Host_event::Reason reason_);

    // -- only for use within python
    Host_join_event();

    static const Event_name static_get_name();

    Action              action;
    int64_t             hostname;
    Host_event::Reason  reason;

%extend {
    static void fill_python_event(const Event& e, PyObject* proxy) const 
    {
        const Host_join_event& hj = dynamic_cast<const Host_join_event&>(e);

        pyglue_setattr_string(proxy, "action", to_python((uint32_t)(hj.action)));
        pyglue_setattr_string(proxy, "hostname", to_python(hj.hostname));
        pyglue_setattr_string(proxy, "reason", to_python((uint32_t)(hj.reason)));

        ((Event*)SWIG_Python_GetSwigThis(proxy)->ptr)->operator=(e);
    }

    static void register_event_converter(PyObject *ctxt) {
        if (!SWIG_Python_GetSwigThis(ctxt) || 
            !SWIG_Python_GetSwigThis(ctxt)->ptr) {
            throw std::runtime_error("Unable to access Python context.");
        }
        
        vigil::applications::PyContext* pyctxt = 
            (vigil::applications::PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr;
        pyctxt->register_event_converter<Host_join_event>
            (&Host_join_event_fill_python_event);
    }
}

};

struct User_event {
    enum Reason {
        AUTHENTICATION_EVENT,
        DEAUTHENTICATION_EVENT,
        HARD_TIMEOUT,
        IDLE_TIMEOUT,
        HOST_DELETE
    };
};

struct User_auth_event
    : public Event
{
    enum Action {
        AUTHENTICATE,
        DEAUTHENTICATE,
    };

    // AUTHENTICATE CONSTRUCTOR
    User_auth_event(int64_t username_, int64_t hostname_,
                    uint32_t idle_timeout_, uint32_t hard_timeout_,
                    User_event::Reason reason_);

    // DEAUTHENTICATE CONSTRUCTOR
    User_auth_event(int64_t username_, int64_t hostname_,
                    User_event::Reason reason_);

    // -- only for use within python
    User_auth_event();

    static const Event_name static_get_name();

    Action              action;
    int64_t             username;
    int64_t             hostname;
    uint32_t            idle_timeout;
    uint32_t            hard_timeout;
    User_event::Reason  reason;

%extend {
    static void fill_python_event(const Event& e, PyObject* proxy) const 
    {
        const User_auth_event& ua = dynamic_cast<const User_auth_event&>(e);

        pyglue_setattr_string(proxy, "action", to_python((uint32_t)(ua.action)));
        pyglue_setattr_string(proxy, "username", to_python(ua.username));
        pyglue_setattr_string(proxy, "hostname", to_python(ua.hostname));
        pyglue_setattr_string(proxy, "idle_timeout", to_python(ua.idle_timeout));
        pyglue_setattr_string(proxy, "hard_timeout", to_python(ua.hard_timeout));
        pyglue_setattr_string(proxy, "reason", to_python((uint32_t)(ua.reason)));

        ((Event*)SWIG_Python_GetSwigThis(proxy)->ptr)->operator=(e);
    }

    static void register_event_converter(PyObject *ctxt) {
        if (!SWIG_Python_GetSwigThis(ctxt) || 
            !SWIG_Python_GetSwigThis(ctxt)->ptr) {
            throw std::runtime_error("Unable to access Python context.");
        }
        
        vigil::applications::PyContext* pyctxt = 
            (vigil::applications::PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr;
        pyctxt->register_event_converter<User_auth_event>
            (&User_auth_event_fill_python_event);
    }
}

};

struct User_join_event
    : public Event
{
    enum Action {
        JOIN,
        LEAVE
    };

    User_join_event(Action action_, int64_t username_, int64_t hostname_,
                    User_event::Reason reason_);

    // -- only for use within python
    User_join_event();

    static const Event_name static_get_name();

    Action              action;
    int64_t             username;
    int64_t             hostname;
    User_event::Reason  reason;

%extend {
    static void fill_python_event(const Event& e, PyObject* proxy) const 
    {
        const User_join_event& uj = dynamic_cast<const User_join_event&>(e);

        pyglue_setattr_string(proxy, "action", to_python((uint32_t)(uj.action)));
        pyglue_setattr_string(proxy, "username", to_python(uj.username));
        pyglue_setattr_string(proxy, "hostname", to_python(uj.hostname));
        pyglue_setattr_string(proxy, "reason", to_python((uint32_t)(uj.reason)));

        ((Event*)SWIG_Python_GetSwigThis(proxy)->ptr)->operator=(e);
    }

    static void register_event_converter(PyObject *ctxt) {
        if (!SWIG_Python_GetSwigThis(ctxt) ||
            !SWIG_Python_GetSwigThis(ctxt)->ptr) {
            throw std::runtime_error("Unable to access Python context.");
        }
        
        vigil::applications::PyContext* pyctxt = 
            (vigil::applications::PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr;
        pyctxt->register_event_converter<User_join_event>
            (&User_join_event_fill_python_event);
    }
}

};

struct Switch_bind_event
    : public Event
{
    enum Action {
        JOIN,
        LEAVE
    };

    Switch_bind_event(Action action_, const datapathid& dp,
                      int64_t switchname_);

    // -- only for use within python
    Switch_bind_event();

    static const Event_name static_get_name();

    Action              action;
    datapathid          datapath_id;
    int64_t             switchname;

%extend {
    static void fill_python_event(const Event& e, PyObject* proxy) const 
    {
        const Switch_bind_event& sj = dynamic_cast<const Switch_bind_event&>(e);

        pyglue_setattr_string(proxy, "action", to_python((uint32_t)(sj.action)));
        pyglue_setattr_string(proxy, "datapath_id", to_python(sj.datapath_id));
        pyglue_setattr_string(proxy, "switchname", to_python(sj.switchname));

        ((Event*)SWIG_Python_GetSwigThis(proxy)->ptr)->operator=(e);
    }

    static void register_event_converter(PyObject *ctxt) {
        if (!SWIG_Python_GetSwigThis(ctxt) || 
            !SWIG_Python_GetSwigThis(ctxt)->ptr) {
            throw std::runtime_error("Unable to access Python context.");
        }
        
        vigil::applications::PyContext* pyctxt = 
            (vigil::applications::PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr;
        pyctxt->register_event_converter<Switch_bind_event>
            (&Switch_bind_event_fill_python_event);
    }
}

};

%pythoncode
%{
    from nox.lib.core import Component

    class PyAuth(Component):
        def __init__(self, ctxt):
            Component.__init__(self, ctxt)
            self.authenticator = PyAuthenticator(ctxt)
        
        def configure(self, configuration):
            self.authenticator.configure(configuration)
            Host_auth_event.register_event_converter(self.ctxt)
            Host_bind_event.register_event_converter(self.ctxt)
            Host_join_event.register_event_converter(self.ctxt)
            User_auth_event.register_event_converter(self.ctxt)
            User_join_event.register_event_converter(self.ctxt)
            Switch_bind_event.register_event_converter(self.ctxt)

        def getInterface(self):
            return str(PyAuth)

        def add_internal_subnet(self, cidr):
            self.authenticator.add_internal_subnet(cidr)

        def remove_internal_subnet(self, cidr):
            self.authenticator.remove_internal_subnet(cidr)

        def clear_internal_subnets(self):
            self.authenticator.clear_internal_subnets()

        def get_authed_hostname(self, dladdr, nwaddr):
            return self.authenticator.get_authed_hostname(dladdr, nwaddr)

        def get_authed_locations(self, dladdr, nwaddr):
            return self.authenticator.get_authed_locations(dladdr, nwaddr)

        def get_authed_addresses(self, hostid):
            return self.authenticator.get_authed_addresses(hostid)

        def is_virtual_location(self, dp, port):
            return self.authenticator.is_virtual_location(dp, port)

        def get_names(self, dp, inport, dlsrc, nwsrc, dldst, nwdst, callable):
            self.authenticator.get_names(dp, inport, dlsrc, nwsrc, dldst, nwdst, callable)

        def all_updated(self, poison):
            self.authenticator.all_updated(poison)

        def principal_updated(self, ptype, id, poison):
            self.authenticator.principal_updated(ptype, id, poison)
            
        def groups_updated(self, ids, poison):
            self.authenticator.groups_updated(ids, poison)

        def is_switch_active(self, dp):
            return self.authenticator.is_switch_active(dp)

        def is_netid_active(self, netid):
            return self.authenticator.is_netid_active(netid)

        def get_port_number(self, dp, port_name):
            return self.authenticator.get_port_number(dp, port_name)

    def getFactory():
        class Factory():
            def instance(self, context):
                return PyAuth(context)

        return Factory()
%}
