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
#ifndef  PACKET_EXPR_HH
#define  PACKET_EXPR_HH

#include <string>

#include <stdint.h>

#include "cnode.hh"
#include "cnode-result.hh"

/*
 * Example implementation of the Expr model used by Classifier, Cnode,
 * Cnode_result, and Rule.
 *
 * An Expr describes the set of packets a given rule should be applied to.  How
 * an expression class chooses to describe these sets is completely up to the
 * implementation.  However, to utilize the general classification mechanism
 * encoded in the Cnode structure, an Expr should expose the following
 * interface.
 *
 * Overview:
 *
 * An Expr should define a constant 'NUM_FIELDS' equal to the number of
 * different fields it would like the Cnode tree to attempt to split rules of
 * its type on.  Note that an Expr can be defined over additional fields that
 * it would not like Cnode to split on, it should just not include these fields
 * in the 'NUM_FIELDS' constant.  Cnode represents each of the 'NUM_FIELDS'
 * fields by an integer value less than 'NUM_FIELDS' and greater than or equal
 * to zero.  An Expr can be wildcarded on any number of these fields (i.e. the
 * Expr allows 'any' value on the field).  A path in Cnode is then denoted by a
 * bit mask in which the ith bit is on if and only if field i has been split
 * on.
 *
 * The required constants and methods needed to be implemented by an Expr in
 * order to be used in a Cnode tree are described below.
 */

namespace vigil {

class Flow;

class Packet_expr {

public:

/*************************** BEGIN REQUIRED INTERFACE *************************/

    /* Constants */

    static const uint32_t NUM_FIELDS = 13;     // num fields that can be split
                                               // on by Cnode
    static const uint32_t LEAF_THRESHOLD = 1;  // min num rules that should be
                                               // saved by a split for it to be
                                               // worth the hash cost.


    /*
     * Returns 'true' if the expr is wildcarded on 'field', else 'false'.
     */

    bool is_wildcard(uint32_t field) const;

    /*
     * Returns 'true' if the expr, having already been split on all fields
     * denoted by 'path', could potentially be split on any other fields.
     * Needed in order to avoid copying rules to a node's children when it is
     * known in advance that the rule will apply to all possible descendents.
     * Returns 'false' if no further splits can be made (i.e. expr is
     * wildcarded on all fields that have yet to be split on).
     */

    bool splittable(uint32_t path) const;

    /*
     * Sets the memory pointed to by 'value' equal to the expression's value
     * for 'field'.  Returns 'false' if 'value' has not been set (i.e. when
     * expr is wildcarded on 'field'), else returns 'true'.  Should set any of
     * the MAX_FIELD_LEN * 4 bytes in 'value' not used by 'field' to 0.
     */

    bool get_field(uint32_t field, uint32_t& value) const;

/**************************** END REQUIRED INTERFACE **************************/

    Packet_expr() : wildcards(~0) { }
    ~Packet_expr() { }

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

    static const int MAX_FIELD_LEN = 2;

    void set_field(Expr_field, const uint32_t [MAX_FIELD_LEN]);

    void print() const;
    void print(uint32_t) const;

    const std::string to_string() const;
    const std::string to_string(uint32_t field) const;

    uint16_t ap_src;
    uint16_t ap_dst;    //how have data during classification
    uint16_t dl_vlan;
    uint16_t dl_vlan_pcp;
    uint16_t dl_proto;
    uint8_t dl_src[6];
    uint8_t dl_dst[6];
    uint32_t nw_src;
    uint32_t nw_dst;
    uint16_t tp_src;
    uint16_t tp_dst;
    uint32_t group_src;
    uint32_t group_dst;
    uint32_t wildcards;
    uint8_t nw_proto;
};


/*
 * This function should be defined for every Data type that
 * Classifier::traverse might get called on.  This example Expr assumes it
 * will get called on either a Flow or another Packet_expr.  Sets the
 * memory pointed to by 'value' equal to the data's value for 'field' where
 * 'field' is one of the above-described single-bit masks.  Returns 'true'
 * if 'value' was set, else 'false' (e.g. when the data does not have a
 * value for 'field'.  Should set any of the MAX_FIELD_LEN * 4 bytes in
 * 'value' not used by 'field' equal to '0'.
 */

template <>
bool
get_field<Packet_expr, Flow>(uint32_t field, const Flow& flow,
                             uint32_t idx, uint32_t& value);
template <>
bool
get_field<Packet_expr, Packet_expr>(uint32_t field, const Packet_expr& expr,
                                    uint32_t idx, uint32_t& value);

/*
 * This function should be defined for every Data type that
 * Classifier::traverse might get called on.  It is used by Cnode_result to
 * iterate through only the valid rules.  This example Expr assumes it will
 * get called on either a Flow or another Packet_expr.  Returns
 * 'true' if the expr matches the packet argument.  This is where the expr
 * can check for fields that the Cnode tree either has not or cannot
 * split on.  Returns 'false' if the expr does not match the argument.
 */

template <>
bool matches(uint32_t rule_id, const Packet_expr&, const Flow&);

template <>
bool matches(uint32_t rule_id, const Packet_expr&, const Packet_expr&);

} // namespace vigil


#endif
