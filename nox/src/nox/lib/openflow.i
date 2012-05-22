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
%module "nox.lib.openflow"

%{
#include "openflow/openflow.h"
#include "netinet++/ethernetaddr.hh"

using namespace vigil;
%}

%include "common-defs.i"
%include "openflow/openflow.h"

%extend ofp_match {
    void set_dl_src(uint8_t *addr) {
        memcpy($self->dl_src, addr, ethernetaddr::LEN);
    }
    void set_dl_dst(uint8_t *addr) {
        memcpy($self->dl_dst, addr, ethernetaddr::LEN);
    }
};
