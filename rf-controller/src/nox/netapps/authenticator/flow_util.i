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

%module "nox.netapps.authenticator.pyflowutil"

%{
#include "core_events.hh"
#include "pyrt/pycontext.hh"
#include "pyrt/pyevent.hh"
#include "pyrt/pyglue.hh"

#include "pyflow_util.hh"
#include "flow_in.hh"

using namespace vigil;
using namespace vigil::applications;
%}

%include "common-defs.i"

%import "netinet/netinet.i"
%import "pyrt/event.i"

/*
 * Exposes Flow_in_event to Python
 */

struct Flow_in_event
    : public Event
{

    Flow_in_event();

    static const std::string static_get_name();

%extend {
    static void fill_python_event(const Event& e, PyObject* proxy) const 
    {
        const Flow_in_event& fi = dynamic_cast<const Flow_in_event&>(e);

        pyglue_setattr_string(proxy, "active", to_python(fi.active));
        pyglue_setattr_string(proxy, "fn_applied", to_python(fi.fn_applied));
        pyglue_setattr_string(proxy, "fn_name", to_python(fi.fn_name));
        pyglue_setattr_string(proxy, "received_sec", to_python(fi.received.tv_sec));
        pyglue_setattr_string(proxy, "received_usec", to_python(fi.received.tv_usec));
        pyglue_setattr_string(proxy, "datapath_id", to_python(fi.datapath_id));
        pyglue_setattr_string(proxy, "flow", to_python(fi.flow));
        pyglue_setattr_string(proxy, "buf", to_python(fi.buf));
        pyglue_setattr_string(proxy, "total_len", to_python(fi.total_len));
        pyglue_setattr_string(proxy, "buffer_id", to_python(fi.buffer_id));
        pyglue_setattr_string(proxy, "reason", to_python(fi.reason));

        pyglue_setattr_string(proxy, "src_host_netid", to_python(*fi.src_host_netid));
        pyglue_setattr_string(proxy, "src_location", to_python(fi.src_location));
        pyglue_setattr_string(proxy, "src_dladdr_groups", to_python_list(*fi.src_dladdr_groups));
        pyglue_setattr_string(proxy, "src_nwaddr_groups", to_python_list(*fi.src_nwaddr_groups));

        pyglue_setattr_string(proxy, "dst_authed", to_python(fi.dst_authed));
        pyglue_setattr_string(proxy, "dst_host_netid", to_python(*fi.dst_host_netid));
        pyglue_setattr_string(proxy, "dst_locations", to_python(fi.dst_locations));
        pyglue_setattr_string(proxy, "dst_dladdr_groups", to_python_list(*fi.dst_dladdr_groups));
        pyglue_setattr_string(proxy, "dst_nwaddr_groups", to_python_list(*fi.dst_nwaddr_groups));

        pyglue_setattr_string(proxy, "route_source", route_source_to_python(fi.route_source));
        pyglue_setattr_string(proxy, "route_destinations", route_destinations_to_python(fi.route_destinations));

        ((Event*)SWIG_Python_GetSwigThis(proxy)->ptr)->operator=(e);
    }

    static void register_event_converter(PyObject *ctxt) {
        if (!SWIG_Python_GetSwigThis(ctxt) || 
            !SWIG_Python_GetSwigThis(ctxt)->ptr) {
            throw std::runtime_error("Unable to access Python context.");
        }
        
        vigil::applications::PyContext* pyctxt = 
            (vigil::applications::PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr;
        pyctxt->register_event_converter<Flow_in_event>
            (&Flow_in_event_fill_python_event);
    }
}

};


class Flow_expr {

public:
    enum Pred_t {
        LOCSRC,
        LOCDST,
        HSRC,
        HDST,
        HNETSRC,
        HNETDST,
        USRC,
        UDST,
        CONN_ROLE,
        GROUPSRC,
        GROUPDST,
        DLVLAN,
        DLVLANPCP,
        DLSRC,
        DLDST,
        DLTYPE,
        NWSRC,
        NWDST,
        NWPROTO,
        TPSRC,
        TPDST,
        SUBNETSRC,
        SUBNETDST,
        FUNC,
        MAX_PRED
    };

    enum Conn_role_t {
        REQUEST,
        RESPONSE,
    };

    enum Apply_side_t {
        ALWAYS_APPLY,
        APPLY_AT_SOURCE,
        APPLY_AT_DESTINATION
    };

    Flow_expr(uint32_t n_preds);
    ~Flow_expr();
    bool set_fn(PyObject *fn_);
    bool set_pred(uint32_t i, Pred_t type, uint64_t value);
    bool set_pred(uint32_t i, Pred_t type, int64_t value);

    Apply_side_t apply_side;
    uint32_t global_id;
};


class Flow_action {

public:
    enum Action_t {
        ALLOW,
        DENY,
        WAYPOINT,
        C_FUNC,
        PY_FUNC,
        NAT,
        MAX_ACTIONS,
    };

    Flow_action(Action_t type_, uint32_t n_args);
    Flow_action(Action_t type_);
    ~Flow_action();

    bool set_arg(uint32_t i, uint64_t arg);
    bool set_arg(uint64_t);
    bool set_arg(PyObject*);
};

%include "std_string.i"
%include "std_list.i"

%template(strlist) std::list<std::string>;

%include "pyflow_util.hh"

%pythoncode
%{
    from nox.lib.core import Component

    class PyFlowUtil(Component):
        def __init__(self, ctxt):
            Component.__init__(self, ctxt)
            self.flowutil = PyFlow_util(ctxt)
        
        def configure(self, configuration):
            self.flowutil.configure(configuration)
            Flow_in_event.register_event_converter(self.ctxt)

        def getInterface(self):
            return str(PyFlowUtil)

        def valid_fn_args(self, key, args):
            return self.flowutil.valid_fn_args(key, args)

        def set_action_argument(self, action, arg, fn_args):
            return self.flowutil.set_action_argument(action, arg, fn_args)

    def getFactory():
        class Factory():
            def instance(self, context):
                return PyFlowUtil(context)

        return Factory()
%}
