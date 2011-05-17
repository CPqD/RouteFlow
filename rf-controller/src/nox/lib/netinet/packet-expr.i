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
#include "expr.hh"
#include "netinet++/ipaddr.hh"
#include "netinet++/ethernetaddr.hh"
using namespace vigil;
%}

class Packet_expr {

public:
    Packet_expr();
    ~Packet_expr();

    enum Expr_field {
        AP_SRC,
        AP_DST,
        DL_VLAN,
        DL_VLAN_PCP,
        DL_TYPE,
        DL_SRC,
        DL_DST,
        NW_SRC,
        NW_DST,
        NW_PROTO,
        TP_SRC,
        TP_DST,
        GROUP_SRC,
        GROUP_DST,
    };
};

%extend Packet_expr {
     
    void set_uint32_field(Expr_field f, uint32_t val)
    {
        uint32_t arr[Packet_expr::MAX_FIELD_LEN];

        for (uint32_t i = 1; i < Packet_expr::MAX_FIELD_LEN; i++) {
            arr[i] = 0;
        }

        if (Packet_expr::MAX_FIELD_LEN > 0) {
            arr[0] = val;
        }

        $self->set_field(f, arr);
    }

    void set_uint32_field(Expr_field f, int32_t val) {
        uint32_t arr[Packet_expr::MAX_FIELD_LEN];

        for (uint32_t i = 1; i < Packet_expr::MAX_FIELD_LEN; i++) {
            arr[i] = 0;
        }

        if (Packet_expr::MAX_FIELD_LEN > 0) {
            arr[0] = val;
        }

        $self->set_field(f, arr);
    }

    void set_ip_field(Expr_field f, ipaddr ip)
    {
        uint32_t arr[Packet_expr::MAX_FIELD_LEN];

        for (uint32_t i = 1; i < Packet_expr::MAX_FIELD_LEN; i++) {
            arr[i] = 0;
        }

        if (Packet_expr::MAX_FIELD_LEN > 0) {
            arr[0] = ip;
        }

        $self->set_field(f, arr);
    }

    void set_eth_field(Expr_field f, ethernetaddr eth)
    {
        uint32_t arr[Packet_expr::MAX_FIELD_LEN];

        for (uint32_t i = 1; i < Packet_expr::MAX_FIELD_LEN; i++) {
            arr[i] = 0;
        }

        if (ethernetaddr::LEN <= sizeof(uint32_t) * Packet_expr::MAX_FIELD_LEN) {
            memcpy(arr, eth.octet, ethernetaddr::LEN);
        } else {
            memcpy(arr, eth.octet, sizeof(uint32_t) * Packet_expr::MAX_FIELD_LEN);
        }

        $self->set_field(f, arr);
    }

    char *__str__() {
        static char temp[64];
        sprintf(temp,"%s", self->to_string().c_str());
        return &temp[0];
    }

};

