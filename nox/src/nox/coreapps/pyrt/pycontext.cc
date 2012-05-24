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
#include "pycontext.hh"

#ifndef SWIG
#ifndef SWIGPYTHON
#include "swigpyrun.h"
#endif // SWIGPYTHON
#endif // SWIG

#include <iostream>
#include <vector>
#include <cstring>

#include <boost/bind.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/shared_array.hpp>
#include<boost/tokenizer.hpp>

#include "netinet++/ethernetaddr.hh"

#include "datapath-join.hh"
#include "datapath-leave.hh"
#include "switch-mgr.hh"
#include "switch-mgr-join.hh"
#include "switch-mgr-leave.hh"
#include "bootstrap-complete.hh"
#include "aggregate-stats-in.hh"
#include "desc-stats-in.hh"
#include "table-stats-in.hh"
#include "port-stats-in.hh"

#include "echo-request.hh"
#include "flow-mod-event.hh"
#include "flow-removed.hh"
#include "packet-in.hh"
#include "port-status.hh"
#include "pycomponent.hh"
#include "pyevent.hh"
#include "pyglue.hh"
#include "pyrt.hh"
#include "port.hh"
#include "shutdown-event.hh"
#include "threads/cooperative.hh"
#include "vlog.hh"
#include "openflow-default.hh"


using namespace std;
using namespace vigil;
using namespace boost;
using namespace vigil::applications;

namespace vigil {
namespace applications {

PyContext::PyContext(const container::Context* ctxt_,
                     container::Component* c_, Python_event_manager* pyem_)
    : ctxt(ctxt_), c(c_), pyem(pyem_) {

}

PyObject*
PyContext::resolve(PyObject* interface)
{
    // If component returned a (class) type, translate it to a string.
    if (interface && PyClass_Check(interface) && !PyInstance_Check(interface)) {
        PyObject* t = interface;
        interface = PyObject_Str(t);
        Py_DECREF(t);
    }

    if (!PyString_Check(interface)){
        PyErr_SetString(PyExc_TypeError, "resolve expects an interface "
                        "as a parameter.");
        return 0;
    }
    container::Interface_description i(PyString_AsString(interface));
    PyComponent* c = dynamic_cast<PyComponent*>(ctxt->get_by_interface(i));
    if (c) {
        PyObject *r = c->getPyObject();
        Py_INCREF(r);
        return r;
    } else {
        Py_RETURN_NONE;
    }
}

Kernel*
PyContext::get_kernel(){
    return ctxt->get_kernel();
}

const char*
PyContext::get_version() {
    return VERSION;
}

void
PyContext::post(Event* event) {
    c->post(event);
}

void
PyContext::register_event(const Event_name& name) {
    try {
        c->register_event(name);
    }
    catch (const std::exception& e) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, e.what());
    }
}

static void convert_python_event(const Event& e, PyObject* proxy) {
    const pyevent& sfe
                = dynamic_cast<const pyevent&>(e);

    if(sfe.python_arg){
        pyglue_setattr_string(proxy, "pyevent", sfe.python_arg);
        Py_INCREF(sfe.python_arg);
    }else{
        pyglue_setattr_string(proxy, "pyevent", Py_None);
        vlog().log(vlog().get_module_val("pyrt"), Vlog::LEVEL_ERR,
                   "Pyevent without internal python_arg set");
        Py_INCREF(Py_None);
    }

    ((Event*)SWIG_Python_GetSwigThis(proxy)->ptr)->operator=(e);
}

void
PyContext::register_python_event(const Event_name& name)
{
    register_event(name);
    // For python events, we register the generic pyevent event
    // converter
    register_event_converter(name,&convert_python_event);

}

void
PyContext::register_event_converter(const Event_name& name,
                                    const Event_converter& converter) {
    try {
        pyem->register_event_converter(name, converter);
    }
    catch (const std::exception& e) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, e.what());
    }
}

void
PyContext::register_handler(const Event_name& name,
                            PyObject* callable)
{
    if (!callable || !PyCallable_Check(callable)) {
        PyErr_SetString(PyExc_TypeError, "not a callable parameter");
        return;
    }

    // Use intrusive pointer to call INCREF and DECREF for all
    // handlers passed to the container.
    boost::intrusive_ptr<PyObject> cptr(callable, true);

    try {
        c->register_handler(name,
                            boost::bind(&Python_event_manager::
                                        call_python_handler, pyem,
                                        _1, cptr));
    }
    catch (const std::exception& e) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, e.what());
    }
}

uint32_t
PyContext::register_handler_on_match(PyObject* callable,
                                     uint32_t priority,
                                     const Packet_expr &expr) {
    if (!callable || !PyCallable_Check(callable)) {
        PyErr_SetString(PyExc_TypeError, "not a callable parameter");
        return 0;
    }

    try {
        boost::intrusive_ptr<PyObject> cptr(callable, true);
        return c->register_handler_on_match
            (priority, expr, boost::bind(&Python_event_manager::
                                         call_python_handler, pyem, _1, cptr));
    }
    catch (const std::exception& e) {
        /* Unable to convert the arguments. */
        PyErr_SetString(PyExc_TypeError, e.what());
        return 0;
    }
}

void
PyContext::send_openflow_packet(uint64_t datapath_id,
                                const Nonowning_buffer& packet,
                                uint16_t out_port, uint16_t in_port) {
    c->send_openflow_packet(datapathid::from_host(datapath_id), packet,
                            out_port, in_port, false);
}


void
PyContext::send_openflow_packet(uint64_t datapath_id,
                                const Nonowning_buffer& packet,
                                const Nonowning_buffer& actions,
                                uint16_t in_port) {
    c->send_openflow_packet(datapathid::from_host(datapath_id), packet,
                            (ofp_action_header*)actions.data(), actions.size(),
                            in_port, false);
}

void
PyContext::send_openflow_buffer(uint64_t datapath_id,
                                uint32_t buffer_id,
                                uint16_t out_port, uint16_t in_port) {
    c->send_openflow_packet(datapathid::from_host(datapath_id), buffer_id,
                            out_port, in_port, false);
}

void
PyContext::send_openflow_buffer(uint64_t datapath_id,
                                uint32_t buffer_id,
                                const Nonowning_buffer& actions,
                                uint16_t in_port) {
    c->send_openflow_packet(datapathid::from_host(datapath_id), buffer_id,
                            (ofp_action_header*)actions.data(),
                            actions.size(), in_port, false);
}

void
PyContext::send_flow_command(uint64_t datapath_id,
                             ofp_flow_mod_command command,
                             const ofp_match& match, 
			     uint16_t idle_timeout, uint16_t hard_timeout,
                             const Nonowning_buffer& actions,
                             uint32_t buffer_id, 
			     uint16_t priority , uint64_t cookie) {
    ofp_flow_mod* ofm = NULL;
    size_t size = sizeof *ofm + actions.size();
    boost::shared_array<uint8_t> raw_of(new uint8_t[size]);
    ofm = (ofp_flow_mod*) raw_of.get();

    ofm->header.version = OFP_VERSION;
    ofm->header.type = OFPT_FLOW_MOD;
    ofm->header.length = htons(size);
    ofm->header.xid = htonl(0);
    ofm->match = match;
    ofm->cookie = cookie;
    ofm->command = htons(command);
    ofm->idle_timeout = htons(idle_timeout);
    ofm->hard_timeout = htons(hard_timeout);
    ofm->buffer_id = htonl(buffer_id);
    ofm->out_port = htons(OFPP_NONE);
    ofm->priority = htons(priority);
    ofm->flags = htons(ofd_flow_mod_flags());

    if (actions.size() > 0) {
        ::memcpy(ofm->actions, actions.data(), actions.size());
    }

    int error = c->send_openflow_command(datapathid::from_host(datapath_id),
                                         &ofm->header, false);
    if (error == EAGAIN) {
        vlog().log(vlog().get_module_val("pyrt"), Vlog::LEVEL_ERR,
                   "unable to send flow setup for dpid: %"PRIx64"\n",
                    datapath_id);
    }
}

int
PyContext::close_openflow_connection(uint64_t datapath_id)
{
    return c->close_openflow_connection(datapathid::from_host(datapath_id));
}

int
PyContext::send_switch_command(uint64_t dpid, const std::string command)
{
    return send_switch_command(dpid, command, "");
}

int
PyContext::send_switch_command(uint64_t dpid, const std::string
        command, const std::string args )
{
    vector<string> vargs;

    if(args != ""){
        tokenizer<> tok(args);
        for(tokenizer<>::iterator beg=tok.begin(); beg!=tok.end();++beg){
            vargs.push_back(*beg);
        }
    }

    return c->send_switch_command(datapathid::from_host(dpid), command, vargs);
}

int
PyContext::switch_reset(uint64_t dpid)
{
    return c->switch_reset(datapathid::from_host(dpid));
}

int
PyContext::switch_update(uint64_t dpid)
{
    return c->switch_update(datapathid::from_host(dpid));
}

int PyContext::send_add_snat(uint64_t dpid, uint16_t port,
                    uint32_t ip_addr_start, uint32_t ip_addr_end,
                    uint16_t tcp_start, uint16_t tcp_end,
                    uint16_t udp_start, uint16_t udp_end,
                    ethernetaddr mac_addr, uint16_t mac_timeout) {
    return c->send_add_snat(datapathid::from_host(dpid),
                  port,ip_addr_start,ip_addr_end,
                  tcp_start,tcp_end,udp_start,udp_end, mac_addr, mac_timeout);
}

int PyContext::send_del_snat(uint64_t dpid, uint16_t port){
    return c->send_del_snat(datapathid::from_host(dpid), port);
}

uint32_t PyContext::get_switch_controller_ip(uint64_t dpid){
    return c->get_switch_controller_ip(datapathid::from_host(dpid));
}

uint32_t PyContext::get_switch_ip(uint64_t dpid){
    return c->get_switch_ip(datapathid::from_host(dpid));
}


void
PyContext::send_table_stats_request(uint64_t datapath_id)
{
    send_stats_request(datapath_id, OFPST_TABLE, 0, 0);
}

void
PyContext::send_port_stats_request(uint64_t datapath_id, uint16_t port)
{
    ofp_port_stats_request psr;
    psr.port_no = htons(port);
    send_stats_request(datapath_id, OFPST_PORT, (const
    uint8_t*)&psr, sizeof(struct ofp_port_stats_request));
}

void
PyContext::send_desc_stats_request(uint64_t datapath_id)
{
    send_stats_request(datapath_id, OFPST_DESC, 0, 0);
}

void
PyContext::send_aggregate_stats_request(uint64_t datapath_id, const struct ofp_match& match, uint8_t table_id)
{
    ofp_aggregate_stats_request  asr;
    asr.table_id = table_id;
    asr.match    = match;
    asr.out_port = htons(OFPP_NONE);
    send_stats_request(datapath_id, OFPST_AGGREGATE, (const
    uint8_t*)&asr, sizeof(struct ofp_aggregate_stats_request));
}


void
PyContext::send_stats_request(uint64_t datapath_id, ofp_stats_types type, const uint8_t* data, size_t data_size )
{
    ofp_stats_request* osr = NULL;
    size_t msize = sizeof(ofp_stats_request) + data_size;
    boost::shared_array<uint8_t> raw_sr(new uint8_t[msize]);

    // Send OFPT_STATS_REQUEST
    osr = (ofp_stats_request*) raw_sr.get();
    osr->header.type    = OFPT_STATS_REQUEST;
    osr->header.version = OFP_VERSION;
    osr->header.length  = htons(msize);
    osr->header.xid     = 0;
    osr->type           = htons(type);
    osr->flags          = htons(0); /* CURRENTLY NONE DEFINED */

    if(data){
        ::memcpy(osr->body, data, data_size );
    }

    int error = c->send_openflow_command(datapathid::from_host(datapath_id),
                                         &osr->header, false);
    if (error == EAGAIN) {
        vlog().log(vlog().get_module_val("pyrt"), Vlog::LEVEL_ERR,
                   "unable to send stats request for dpid: %"PRIx64"\n",
                    datapath_id);
    }
}

int
PyContext::send_port_mod(uint64_t datapath_id, uint16_t port_no, ethernetaddr addr, uint32_t mask, uint32_t config )
{
    ofp_port_mod* opm = NULL;
    size_t msize = sizeof(ofp_port_mod);
    boost::shared_array<uint8_t> raw_sr(new uint8_t[msize]);

    // Send OFPT_STATS_REQUEST
    opm = (ofp_port_mod*) raw_sr.get();
    opm->header.type    = OFPT_PORT_MOD;
    opm->header.version = OFP_VERSION;
    opm->header.length  = htons(msize);
    opm->header.xid     = 0;
    opm->port_no        = htons(port_no);
    opm->config         = htonl(config);
    opm->mask           = htonl(mask);
    opm->advertise      = htonl(0);
    memcpy(opm->hw_addr, addr.octet, sizeof(opm->hw_addr));

    int error = c->send_openflow_command(datapathid::from_host(datapath_id),
                                         &opm->header, false);
    if (error == EAGAIN) {
        vlog().log(vlog().get_module_val("pyrt"), Vlog::LEVEL_ERR,
                   "unable to send port_mod for dpid: %"PRIx64"\n",
                    datapath_id);
    }

    return error;
}


bool
PyContext::unregister_handler(uint32_t rule_id) {
    return c->unregister_handler(rule_id);
}

} // applications
} // vigil
