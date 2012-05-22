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
#include "flow.hh"
using namespace vigil;
%}

struct Flow {
    uint16_t in_port;       /* Input switch port. */
    uint16_t dl_vlan;       /* Input VLAN. */
    uint8_t dl_vlan_pcp;    /* Input VLAN priority. */
    ethernetaddr dl_src;    /* Ethernet source address. */
    ethernetaddr dl_dst;    /* Ethernet destination address. */
    uint16_t dl_type;       /* Ethernet frame type. */
    uint32_t nw_src;        /* IP source address. */
    uint32_t nw_dst;        /* IP destination address. */
    uint8_t nw_proto;       /* IP protocol. */
    uint16_t tp_src;        /* TCP/UDP source port. */
    uint16_t tp_dst;        /* TCP/UDP destination port. */
};
