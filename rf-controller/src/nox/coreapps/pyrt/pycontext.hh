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
#ifndef CONTROLLER_PYCONTEXT_HH
#define CONTROLLER_PYCONTEXT_HH 1

#include <Python.h>
#include <string>

#include "buffer.hh"
#include "component.hh"
#include "hash_map.hh"
#include "hash_set.hh"
#include "pyrt.hh"

namespace vigil {

struct ethernetaddr;
class Kernel;
class Port;    

namespace applications {


/**
 * Python Context object wraps the C++ context instance and has a
 * pointer to the corresponding C++ PyComponent instance.  All Python
 * related state is held in PyRt.
 */
class PyContext {
public:
    typedef Python_event_manager::Event_converter Event_converter;

    PyContext(const container::Context* ctxt_, container::Component* c_, 
              Python_event_manager*);

    /* Resolves a component interface description into a component
     * instance. Returns a new reference.
     */
    PyObject* resolve(PyObject*);

    /* get a pointer to the kernel object exposing component
     * configuration internals to python */
    Kernel*   get_kernel();

    /* Get platform version string. */
    const char* get_version();

    /* Posts an event.
     */
    void post(Event*);

    /* Registers a Python only event */
    void register_event(const Event_name&);

    /* Registers a C++ to Python event converter. */
    template <typename T>
    inline
    void register_event_converter(const Event_converter& c) {
        register_event_converter(T::static_get_name(), c);
    }

    void register_event_converter(const Event_name&,
                                  const Event_converter&);

    void register_python_event(const Event_name&);
                              

    /* Registers a handler with a return closure to be called whenever
     * an event of name name_ is dispatched. Used in Python as
     * follows:
     *
     *   d.register_handler(p.get_name(), gen_closure_event(p2))
     *
     * Handler should return a Disposition.
     */    
    void register_handler(const Event_name&, PyObject* callable);
    
    uint32_t register_handler_on_match(PyObject* callable,
                                       uint32_t priority,
                                       const Packet_expr &expr);

    bool unregister_handler(uint32_t rule_id);

    void send_openflow_packet(uint64_t datapath_id, 
                              const Nonowning_buffer&,
                              uint16_t out_port, uint16_t in_port);

    void send_openflow_packet(uint64_t datapath_id, 
                              const Nonowning_buffer&, const Nonowning_buffer&,
                              uint16_t in_port);

    void send_openflow_buffer(uint64_t datapath_id, uint32_t buffer_id, 
                              uint16_t out_port, uint16_t in_port);

    void send_openflow_buffer(uint64_t datapath_id, uint32_t buffer_id,
                              const Nonowning_buffer&,
                              uint16_t in_port);

    /*
     * 'match' should be populated with values in network byte order as should 
     * 'actions'  All other values should be in host byte order (want this syntax?)
     *
     */
    void send_flow_command(uint64_t datapath_id, ofp_flow_mod_command command, 
                           const ofp_match& match, 
			   uint16_t idle_timeout, uint16_t hard_timeout, 
			   const Nonowning_buffer&, uint32_t buffer_id,
                           uint16_t priority=OFP_DEFAULT_PRIORITY,
			   uint64_t cookie=0);

    int close_openflow_connection(uint64_t datapathid);

    int send_add_snat(uint64_t dpid, uint16_t port, 
                    uint32_t ip_addr_start, uint32_t ip_addr_end,
                    uint16_t tcp_start, uint16_t tcp_end,
                    uint16_t udp_start, uint16_t udp_end,
                    ethernetaddr mac_addr, 
                    uint16_t mac_timeout=0);
    int send_del_snat(uint64_t dpid, uint16_t port);
    uint32_t get_switch_controller_ip(uint64_t dpid);
    uint32_t get_switch_ip(uint64_t dpid);

    int send_switch_command(uint64_t dpid, const std::string command, const std::string args );
    int send_switch_command(uint64_t dpid, const std::string command);
    int switch_reset(uint64_t dpid);
    int switch_update(uint64_t dpid);

    void send_stats_request(uint64_t datapath_id, ofp_stats_types type,
            const uint8_t* data, size_t data_size );

    void send_table_stats_request(uint64_t datapath_id);
    void send_port_stats_request(uint64_t datapath_id, uint16_t port);
    void send_desc_stats_request(uint64_t datapath_id);
    int send_port_mod(uint64_t datapath_id, uint16_t port_no, 
                      ethernetaddr addr, uint32_t mask, uint32_t config);
    void send_aggregate_stats_request(uint64_t datapath_ids, const
        struct ofp_match& match, uint8_t table_id);

    /* C++ context */
    const container::Context* ctxt;

    /* C++ component */
    container::Component* c;

private:
    Python_event_manager* pyem;
};

}
}

#endif
