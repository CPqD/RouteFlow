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
%module "nox.lib.netinet"

%include common-defs.i
%include ipaddr.i 
%include cidr.i 
%include ethernetaddr.i
%include datapathid.i
%include packet-expr.i
%include flow.i

/*
 * Exposes to Python to avoid negative value returned by Python htonl/ntohl.
 */

%{
    uint32_t c_htonl(uint32_t val) {
        return htonl(val);
    }

    uint32_t c_ntohl(uint32_t val) {
        return ntohl(val);
    }
%}

uint32_t c_htonl(uint32_t val);
uint32_t c_ntohl(uint32_t val);
