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
#include <string>
#include "pycontext.cc"

using namespace std;
using namespace vigil;
using namespace vigil::applications;
%}

%include "utf8_string.i"

// Let SWIG know that post will manage the destruction of the pyevent
%delobject PyContext::post;

%include "buffer.i"

/* For the ofp_ types */
%import "openflow.i"

/* For ethernetaddr */
%import "netinet/netinet.i"

/* To signal an unset buffer_id */
#define UINT32_MAX    4294967295U

namespace vigil {
namespace applications {

class PyContext {
    %rename(send_openflow_buffer_port) send_openflow_buffer(
            uint64_t datapath_id, uint32_t buffer_id,
            uint16_t out_port, uint16_t in_port);
    %rename(send_openflow_buffer_acts) send_openflow_buffer(
            uint64_t datapath_id, uint32_t buffer_id,
            const Nonowning_buffer& actions, uint16_t in_port);
    %rename(send_openflow_packet_port) send_openflow_packet(
            uint64_t datapath_id, const Nonowning_buffer&, 
            uint16_t out_port, uint16_t in_port);
    %rename(send_openflow_packet_acts) send_openflow_packet(
            uint64_t datapath_id, const Nonowning_buffer&,
            const Nonowning_buffer& actions, uint16_t in_port);
public:
    /*
     * Resolves a component interface description into a component
     * instance.
     */
    PyObject* resolve(PyObject*);

    Kernel*   get_kernel();

    const char* get_version();

    void post(Event*);

    void register_event(const Event_name&);
    void register_python_event(const Event_name&);

    void register_handler(const Event_name&, PyObject* callable);

    uint32_t register_handler_on_match(PyObject* callable,
                                       uint32_t priority,
                                       const Packet_expr&);

    bool unregister_handler(uint32_t rule_id);

    void send_openflow_packet(uint64_t datapath_id, const Nonowning_buffer&,
                              uint16_t out_port, uint16_t in_port);

    void send_openflow_packet(uint64_t datapath_id, const Nonowning_buffer&,
                              const Nonowning_buffer& actions, uint16_t in_port);

    void send_openflow_buffer(uint64_t datapath_id, uint32_t buffer_id, 
                              uint16_t out_port, uint16_t in_port);

    void send_openflow_buffer(uint64_t datapath_id, uint32_t buffer_id, 
                              const Nonowning_buffer& actions,
                              uint16_t in_port);
    
    void send_flow_command(uint64_t datapath_id, ofp_flow_mod_command, 
                           const ofp_match&, uint16_t idle_timeout,
                           uint16_t hard_timeout, const Nonowning_buffer& actions,
                           uint32_t buffer_id, uint16_t priority);

    int close_openflow_connection(uint64_t datapathid);

    int send_switch_command(uint64_t, const std::string, const std::string);
    int switch_reset(uint64_t datapathid);
    int switch_update(uint64_t datapathid);

    int send_add_snat(uint64_t dpid, uint16_t port, 
                    uint32_t ip_addr_start, uint32_t ip_addr_end,
                    uint16_t tcp_start, uint16_t tcp_end,
                    uint16_t udp_start, uint16_t udp_end,
                    ethernetaddr mac_addr,  
                    uint16_t mac_timeout=0);
    int send_del_snat(uint64_t dpid, uint16_t port);

    uint32_t get_switch_controller_ip(uint64_t dpid); 
    uint32_t get_switch_ip(uint64_t dpid); 

    void send_table_stats_request(uint64_t datapath_id);
    void send_port_stats_request(uint64_t datapath_id, uint16_t port);
    void send_desc_stats_request(uint64_t datapath_id);
    int  send_port_mod(uint64_t datapath_id, uint16_t port_no, 
                       ethernetaddr addr, uint32_t mask, uint32_t config);
    void send_aggregate_stats_request(uint64_t datapath_ids, const
        struct ofp_match& match, uint8_t table_id);
private:
    PyContext();

};

} // namespace applications
} // namespace vigil
