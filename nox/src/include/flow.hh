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
#ifndef FLOW_HH
#define FLOW_HH 1

#define IP_ADDR_LEN 4

#include <cstring>
#include <iosfwd>
#include "netinet++/ethernetaddr.hh"
#include "openflow/openflow.h"
#include "openflow-pack-raw.hh"

namespace vigil {

class Buffer;

struct Flow {
  /** Input switch port. 
   */
  uint16_t in_port;       
  /** Input VLAN. 
   */
  uint16_t dl_vlan;      
  /** Input VLAN priority. 
   */ 
  uint8_t dl_vlan_pcp;   
  /** Ethernet source address. 
   */
  ethernetaddr dl_src;    
  /** Ethernet destination address. 
   */
  ethernetaddr dl_dst;    
  /** Ethernet frame type. 
   */
  uint16_t dl_type;     
  /** IP source address. 
   */
  uint32_t nw_src;     
  /** IP destination address. 
   */   
  uint32_t nw_dst;   
  /** IP protocol. 
   */
  uint8_t nw_proto;
  /** IP ToS (actually DSCP field, 6 bits). 
   */  
  uint8_t nw_tos;
  /** TCP/UDP source port. 
   */
  uint16_t tp_src;        
  /** TCP/UDP destination port. 
   */
  uint16_t tp_dst;        
  /** Cookie value
   */
  uint64_t cookie;

  /** Empty constructor
   */
  Flow() :
    in_port(0), dl_vlan(0), dl_vlan_pcp(0), 
    dl_src(), dl_dst(), dl_type(0),
    nw_src(0), nw_dst(0), 
    nw_proto(0), nw_tos(0),
    tp_src(0), tp_dst(0), cookie(0) { }
  /** Copy constructor
   */
  Flow(const Flow& flow_, uint64_t cookie_=0);
  /** Constructor from packet
   */
  Flow(uint16_t in_port_, const Buffer&, uint64_t cookie_=0);
  /** Constructor from ofp_match
   */
  Flow(const ofp_match& match, uint64_t cookie_=0);
  /** Constructor from ofp_match
   */
  Flow(const ofp_match* match, uint64_t cookie_=0);
  /** Detail constructor
   */
  Flow(uint16_t in_port_, uint16_t dl_vlan_, uint8_t dl_vlan_pcp_,
       ethernetaddr dl_src_, ethernetaddr dl_dst_, uint16_t dl_type_, 
       uint32_t nw_src_, uint32_t nw_dst_, uint8_t nw_proto_,
       uint16_t tp_src_, uint16_t tp_dst_, uint8_t nw_tos_=0, 
       uint64_t cookie_=0) :
    in_port(in_port_), dl_vlan(dl_vlan_), dl_vlan_pcp(dl_vlan_pcp_), 
    dl_src(dl_src_), dl_dst(dl_dst_), dl_type(dl_type_),
    nw_src(nw_src_), nw_dst(nw_dst_), 
    nw_proto(nw_proto_), nw_tos(nw_tos_),
    tp_src(tp_src_), tp_dst(tp_dst_), cookie(cookie_) { }
  /** Compare function
   */
  static bool matches(const Flow&, const Flow&);
  /** \brief String representation
   */
  const std::string to_string() const;
  /** \brief Return of_match that is exact match to flow in host order
   *
   * @return of_match with exact match
   */
  const of_match get_exact_match() const;
  /** \brief Return hash code
   */
  uint64_t hash_code() const;
  
};
bool operator==(const Flow& lhs, const Flow& rhs);
bool operator!=(const Flow& lhs, const Flow& rhs);
std::ostream& operator<<(std::ostream&, const Flow&);

} // namespace vigil

ENTER_HASH_NAMESPACE
template <>
struct hash<vigil::Flow> {
  std::size_t operator() (const vigil::Flow& flow) const {
    return HASH_NAMESPACE::hash<uint64_t>()(flow.hash_code());
  }
};
EXIT_HASH_NAMESPACE

#endif /* flow.hh */
