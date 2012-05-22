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
#include "expr.hh"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "flow.hh"

#include "netinet++/ipaddr.hh"
#include "vlog.hh"
#include "cnode.hh"

namespace vigil {

static Vlog_module log("expr");

template<>
bool
get_field<Packet_expr, Flow>(uint32_t field, const Flow& flow,
                             uint32_t idx, uint32_t& value)
{
    if (idx != 0) {
        return false;
    }

    switch (field) {
    case Packet_expr::AP_SRC:
        value = flow.in_port;
        return true;
    case Packet_expr::AP_DST:
        assert(0);
    case Packet_expr::DL_VLAN:
        value = flow.dl_vlan;
        return true;
    case Packet_expr::DL_VLAN_PCP:
        value = flow.dl_vlan_pcp;
        return true;
    case Packet_expr::DL_TYPE:
        value = flow.dl_type;
        return true;
    case Packet_expr::DL_SRC:
        memcpy(&value, flow.dl_src.octet + 2, sizeof value);
        return true;
    case Packet_expr::DL_DST:
        memcpy(&value, flow.dl_dst.octet + 2, sizeof value);
        return true;
    case Packet_expr::NW_SRC:
        value = flow.nw_src;
        return true;
    case Packet_expr::NW_DST:
        value = flow.nw_dst;
        return true;
    case Packet_expr::NW_PROTO:
        value = flow.nw_proto;
        return true;
    case Packet_expr::TP_SRC:
        value = flow.tp_src;
        return true;
    case Packet_expr::TP_DST:
        value = flow.tp_dst;
        return true;
    }
    log.warn("unretrievable field %u", field);
    return false;
}

template<>
bool
get_field<Packet_expr, Packet_expr>(uint32_t field, const Packet_expr& expr,
                                    uint32_t idx, uint32_t& value)
{
    if (idx != 0) {
        return false;
    }
    return expr.get_field(field, value);
}

bool
Packet_expr::get_field(uint32_t field, uint32_t& value) const
{
    if ((Cnode<Packet_expr, void*>::MASKS[field] & wildcards) != 0)
        return false;

    switch (field) {
    case AP_SRC:
        value = ap_src;
        return true;
    case AP_DST:
        value = ap_dst;
        return true;
    case DL_VLAN:
        value = dl_vlan;
        return true;
    case DL_VLAN_PCP:
        value = dl_vlan_pcp;
        return true;
    case DL_TYPE:
        value = dl_proto;
        return true;
    case DL_SRC:
        memcpy(&value, dl_src + 2, sizeof value);
        return true;
    case DL_DST:
        memcpy(&value, dl_dst + 2, sizeof value);
        return true;
    case NW_SRC:
        value = nw_src;
        return true;
    case NW_DST:
        value = nw_dst;
        return true;
    case NW_PROTO:
        value = nw_proto;
        return true;
    case TP_SRC:
        value = tp_src;
        return true;
    case TP_DST:
        value = tp_dst;
        return true;
    case GROUP_SRC:
        value = group_src;
        return true;
    case GROUP_DST:
        value = group_dst;
        return true;
    }
    log.warn("unretrievable field %u", field);
    return false;
}


void
Packet_expr::set_field(Expr_field field, const uint32_t value[MAX_FIELD_LEN])
{
    switch (field) {
    case AP_SRC:
        ap_src = value[0];
        break;
    case AP_DST:
        ap_dst = value[0];
        break;
    case DL_VLAN:
        dl_vlan = value[0];
        break;
    case DL_VLAN_PCP:
        dl_vlan_pcp = value[0];
        break;
    case DL_TYPE:
        dl_proto = value[0];
        break;
    case DL_SRC:
        memcpy(dl_src, value, sizeof dl_src);
        break;
    case DL_DST:
        memcpy(dl_dst, value, sizeof dl_dst);
        break;
    case NW_SRC:
        nw_src = value[0];
        break;
    case NW_DST:
        nw_dst = value[0];
        break;
    case NW_PROTO:
        nw_proto = value[0];
        break;
    case TP_SRC:
        tp_src = value[0];
        break;
    case TP_DST:
        tp_dst = value[0];
        break;
    case GROUP_SRC:
        group_src = value[0];
        break;
    case GROUP_DST:
        group_dst = value[0];
        break;
    default:
        log.warn("unknown field %u", field);
        return;
    }

    wildcards = wildcards & ~(Cnode<Packet_expr, void*>::MASKS[field]);
}

bool
Packet_expr::is_wildcard(uint32_t field) const
{
    return (wildcards & Cnode<Packet_expr, void*>::MASKS[field]) != 0;
}


bool
Packet_expr::splittable(uint32_t path) const
{
    return (~path & ~wildcards) != 0;
}


template <>
bool
matches(uint32_t rule_id, const Packet_expr& expr, const Flow& flow)
{
    assert(!((~expr.wildcards) & (Cnode<Packet_expr, void*>::MASKS[Packet_expr::AP_DST]
                             | Cnode<Packet_expr, void*>::MASKS[Packet_expr::GROUP_SRC]
                             | Cnode<Packet_expr, void*>::MASKS[Packet_expr::GROUP_DST])));

    return (((expr.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::AP_SRC])
             || expr.ap_src == flow.in_port)
            && ((expr.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::DL_VLAN])
                || expr.dl_vlan == flow.dl_vlan)
            && ((expr.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::DL_VLAN_PCP])
                || expr.dl_vlan_pcp == flow.dl_vlan_pcp)
            && ((expr.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::DL_TYPE])
                || expr.dl_proto == flow.dl_type)
            && ((expr.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::DL_SRC])
                || memcmp(expr.dl_src, flow.dl_src.octet, sizeof expr.dl_src) == 0)
            && ((expr.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::DL_DST])
                || memcmp(expr.dl_dst, flow.dl_dst.octet, sizeof expr.dl_dst) == 0)
            && ((expr.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::NW_SRC])
                || expr.nw_src == flow.nw_src)
            && ((expr.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::NW_DST])
                || expr.nw_dst == flow.nw_dst)
            && ((expr.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::NW_PROTO])
                || expr.nw_proto == flow.nw_proto)
            && ((expr.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::TP_SRC])
                || expr.tp_src == flow.tp_src)
            && ((expr.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::TP_DST])
                || expr.tp_dst == flow.tp_dst));
}

template <>
bool
matches(uint32_t rule_id, const Packet_expr& expr, const Packet_expr& to_match)
{
    if ((expr.wildcards | to_match.wildcards) != to_match.wildcards)
        return false;

    return (((to_match.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::AP_SRC])
             || expr.ap_src == to_match.ap_src)
            && ((to_match.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::AP_DST])
                || expr.ap_dst == to_match.ap_dst)
            && ((to_match.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::DL_VLAN])
                || expr.dl_vlan == to_match.dl_vlan)
            && ((to_match.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::DL_VLAN_PCP])
                || expr.dl_vlan_pcp == to_match.dl_vlan_pcp)
            && ((to_match.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::DL_TYPE])
                || expr.dl_proto == to_match.dl_proto)
            && ((to_match.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::DL_SRC])
                || !memcmp(expr.dl_src, to_match.dl_src, sizeof expr.dl_src))
            && ((to_match.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::DL_DST])
                || !memcmp(expr.dl_dst, to_match.dl_dst, sizeof expr.dl_dst))
            && ((to_match.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::NW_SRC])
                || expr.nw_src == to_match.nw_src)
            && ((to_match.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::NW_DST])
                || expr.nw_dst == to_match.nw_dst)
            && ((to_match.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::NW_PROTO])
                || expr.nw_proto == to_match.nw_proto)
            && ((to_match.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::TP_SRC])
                || expr.tp_src == to_match.tp_src)
            && ((to_match.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::TP_DST])
                || expr.tp_dst == to_match.tp_dst)
            && ((to_match.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::GROUP_SRC])
                || expr.group_src == to_match.group_src)
            && ((to_match.wildcards & Cnode<Packet_expr, void*>::MASKS[Packet_expr::GROUP_DST])
                || expr.group_dst == to_match.group_dst));
}


void
Packet_expr::print() const
{
    printf("%s\n", to_string().c_str());
}

void
Packet_expr::print(uint32_t field) const
{
    printf("%s", to_string(field).c_str());
}

const std::string
Packet_expr::to_string(uint32_t field) const
{
    char str[32]; // Yay, magic numbers /mc
    ethernetaddr ea;

    switch(field) {
        case AP_SRC:
            sprintf(str,"apsrc=%hu,", ap_src);
            break;
        case AP_DST:
            sprintf(str,"apdst=%hu,", ap_dst);
            break;
        case DL_VLAN:
            sprintf(str,"dlvlan=%hu,", dl_vlan);
            break;
        case DL_VLAN_PCP:
            sprintf(str,"dlvlanpcp=%hu,", dl_vlan_pcp);
            break;
        case DL_TYPE:
            sprintf(str,"dlproto=%hu,", dl_proto);
            break;
        case DL_SRC:
            sprintf(str,"dlsrc=");
            ea.set_octet(&dl_src[0]);
            return str + ea.string();
            break;
        case DL_DST:
            sprintf(str, "dldst=");
            ea.set_octet(&dl_src[0]);
            return str + ea.string();
            break;
        case NW_SRC:
            return "nwsrc="+ipaddr(nw_src).string();
            break;
        case NW_DST:
            return "nwdst="+ipaddr(nw_src).string();
            break;
        case NW_PROTO:
            sprintf(str,"nwproto=%hhu,", nw_proto);
            break;
        case TP_SRC:
            sprintf(str,"tpsrc=%hu,", tp_src);
            break;
        case TP_DST:
            sprintf(str,"tpdst=%hu,", tp_dst);
            break;
        case GROUP_SRC:
            sprintf(str,"groupsrc=%u,", group_src);
            break;
        case GROUP_DST:
            sprintf(str,"groupdst=%u,", group_dst);
            break;
        default:
            log.warn("unknown field %u", field);
            return "";
    }
    return std::string(str);
}

const std::string
Packet_expr::to_string() const
{
    std::string str("<expr:");

    for (uint32_t i = 1; i <= GROUP_DST; i = i << 1) {
        if ((Cnode<Packet_expr, void*>::MASKS[i] & wildcards) == 0)
            str += to_string(i);
    }
    str += ">";

    return str;
}

} // namespace vigil
