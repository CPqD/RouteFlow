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
// ------------------------------------------------------------
// nox API
//
// The following methods are exposed to network applications
// and consist of the full set of functions available for
// hooking into network events and managing the network.
//
// These methods, in addition to the NDB, provide the 
// full network API.
// ------------------------------------------------------------

/** \defgroup noxapi NOX API
 *
 * These classes and functions provides the basic APIs in NOX.
 * These basic functionalities are supported by NOX's core,
 * which form the basics for the functionalities added by components.
 */


#ifndef nox_HH__ 
#define nox_HH__

#include <string>
#include <vector>

#include <boost/function.hpp>
#include "netinet++/datapathid.hh"
#include "netinet++/ethernetaddr.hh"
#include "packet-classifier.hh"
#include "timer-dispatcher.hh"
#include "switch_auth.hh" 
#include "switch-mgr.hh" 

namespace vigil
{

class Buffer;
class Openflow_connection;
class Openflow_connection_factory;
class Co_sema;

namespace nox {

typedef boost::function<void()> Callback;

void init();

/* Get a reference to the main poll loop. */
Poll_loop* get_poll_loop();

void connect(Openflow_connection_factory*, bool reliable);
void register_conn(Openflow_connection*, Co_sema*);
void run();

void register_handler(const Event_name& name,
                      boost::function<Disposition(const Event&)>,
                      int order);
bool unregister_handler (uint32_t rule_id);

uint32_t register_handler_on_match(uint32_t priority, const Packet_expr &expr, 
                                   Pexpr_action callback);
// TODO unregister_handler_on_match

// global hook to register a class to determine if a switch is
// allowed to connect to NOX.  This functionality cannot be part
// of builtin, because it likely requires access to components 
// in order to access directories or CDB. 
// Currently, only a single Switch_Auth component can be
// registered.  This makes things simple in light of the fact that
// switch authentication calls return asynchronously.  Having only
// a single switch auth component also allows that component to take
// action (e.g., notify the administrator) when an unapproved switch
// appears on the network. 
void register_switch_auth(Switch_Auth* auth);  

void post_event(Event*);

Timer post_timer(const Callback& callback);
Timer post_timer(const Callback& callback, const timeval& duration);
void timer_debug();

boost::shared_ptr<Switch_mgr> mgmtid_to_swm(datapathid mgmt_id);
datapathid dpid_to_mgmtid(datapathid dpid);
bool active_mgmt(const datapathid& mgmtid);

uint32_t allocate_openflow_xid();
int send_openflow_command(const datapathid&, const ofp_header* oh,
                          bool block);
int send_openflow_packet_out(const datapathid&,
                             uint32_t buffer_id, uint16_t out_port,
                             uint16_t in_port, bool block);
int send_openflow_packet_out(const datapathid&,
                             uint32_t buffer_id, 
                             const ofp_action_header actions[], 
                             uint16_t actions_len,
                             uint16_t in_port, bool block);
int send_openflow_packet_out(const datapathid&, 
                             const Buffer& packet, uint16_t out_port, 
                             uint16_t in_port, bool block);  
int send_openflow_packet_out(const datapathid&, 
                             const Buffer& packet, 
                             const ofp_action_header actions[], 
                             uint16_t actions_len,
                             uint16_t in_port, bool block);  

int close_openflow_connection(const datapathid&);
    
int send_add_snat(const datapathid &dpid, uint16_t port, 
                    uint32_t ip_addr_start, uint32_t ip_addr_end,
                    uint16_t tcp_start, uint16_t tcp_end,
                    uint16_t udp_start, uint16_t udp_end,
                    ethernetaddr mac_addr, 
                    uint16_t mac_timeout=0);
int send_del_snat(const datapathid &dpid, uint16_t port);
uint32_t get_switch_controller_ip(const datapathid &dpid);
uint32_t get_switch_ip(const datapathid &dpid);

int fetch_switch_logs(datapathid, const std::string& output_file,
                      const boost::function<void(int error,
                                                 const std::string& msg)>&);

int send_switch_command(datapathid, const std::string& , const std::vector<std::string> );
int switch_reset(datapathid);
int switch_update(datapathid);

} // namespace vigil::nox
} // namespace vigil

#endif  // nox_HH__ 
