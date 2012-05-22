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

#ifndef FLOW_UTIL_HH
#define FLOW_UTIL_HH 1

#include "config.h"

#ifdef TWISTED_ENABLED
#include <Python.h>
#else
class PyObject;
#endif // TWISTED_ENABLED

#include <boost/variant.hpp>
#include <vector>

#include "component.hh"
#include "flow_fn_map.hh"

namespace vigil {

/*
 * Flow expression used in rules.  Implements the interface
 * described in "expr.hh".  Literal types denoted by Pred_t enum.  'NUM_FIELDS'
 * denotes the number of fields the classifier can branch on.  Splittable fiels
 * should have a Pred_t value < NUM_FIELDS while non-splittable field Pred_t
 * values should be >= that value.  Negated predicates are represented by their
 * Pred_t value multiplied by -1.
 */

class Flow_expr {

public:
    // Constants
    static const uint32_t NUM_FIELDS = 21;     // num fields that can be split
                                               // on by Cnode
    static const uint32_t LEAF_THRESHOLD = 1;  // min num rules that should be
                                               // saved by a split for it to be
                                               // worth the hash cost.

    bool is_wildcard(uint32_t field) const;
    bool splittable(uint32_t path) const;
    bool get_field(uint32_t field, uint32_t& value) const;

    enum Pred_t {
        LOCSRC,
        LOCDST,
        HSRC,
        HDST,
        HNETSRC,
        HNETDST,
        USRC,
        UDST,
        CONN_ROLE,
        GROUPSRC,
        GROUPDST,
        DLVLAN,
        DLVLANPCP,
        DLSRC,
        DLDST,
        DLTYPE,
        NWSRC,
        NWDST,
        NWPROTO,
        TPSRC,
        TPDST,
        APPLY_SIDE,
        SUBNETSRC,  // start of un-splittable preds
        SUBNETDST,  // mask should be upper 32 of arg, ip lower
        FUNC,
        MAX_PRED
    };

    enum Conn_role_t {
        REQUEST,
        RESPONSE,
    };

    enum Apply_side_t {
        ALWAYS_APPLY,
        APPLY_AT_SOURCE,
        APPLY_AT_DESTINATION
    };

    struct Pred {
        Pred_t type;
        uint64_t val;
    };

    Flow_expr(uint32_t n_preds) : m_fn(NULL), m_preds(n_preds),
                                  apply_side(ALWAYS_APPLY) { }
    ~Flow_expr() { }

    bool set_pred(uint32_t i, Pred_t type, uint64_t value);
    bool set_pred(uint32_t i, Pred_t type, int64_t value);
    bool set_fn(PyObject *fn);

    PyObject *m_fn;
    std::vector<Pred> m_preds;
    Apply_side_t apply_side;
    uint32_t global_id;

private:
    Flow_expr();

}; // class Flow_expr

/*
 * Flow action used in rules.  Action types denoted by Action_t enum.
 * Boost::variant member for actions with arguments.  Argument is ignored for
 * Action_t values < 'ARG_ACTION'
 */

class Flow_action {

public:
    // action types
    enum Action_t {
        ALLOW,
        DENY,
        WAYPOINT,
        C_FUNC,
        PY_FUNC,
        NAT,
        MAX_ACTIONS,
    };

    struct C_func_t {
        std::string name;
        Flow_fn_map::Flow_fn fn;
    };

    static const uint32_t ARG_ACTION = WAYPOINT;  // min Action_t value of
                                                  // actions with arguments

    Flow_action(Action_t type_, uint32_t n_args)
        : type(type_), arg(std::vector<uint64_t>(n_args, 0)) { }
    Flow_action(Action_t type_)
        : type(type_), arg(0) { }

    ~Flow_action();

    bool set_arg(uint32_t i, uint64_t arg);       // set vector index arg
    bool set_arg(PyObject *);                     // set PyObject arg
    bool set_arg(uint64_t);                       // set single int arg
    bool set_arg(const C_func_t&);                // set function arg

    Action_t get_type() { return type; }

    typedef boost::variant<uint64_t,
                           PyObject*,
                           C_func_t,
                           std::vector<uint64_t> > Arg_union;

    Action_t type;
    Arg_union arg;

private:
    Flow_action();

}; // class Flow_action

class Flow_util
    : public container::Component
{

public:
    Flow_util(const container::Context*, const json_object*);

    static void getInstance(const container::Context*, Flow_util*&);

    void configure(const container::Configuration*);
    void install() { }

    Flow_fn_map fns;
}; // class Flow_util

}

#endif
