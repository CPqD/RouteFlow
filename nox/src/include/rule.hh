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
#ifndef  RULE_HH
#define  RULE_HH

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

namespace vigil {

template<class Expr, typename Action>
class Cnode;

template<class Expr, typename Action>
class Rule {

public:
    friend class Cnode<Expr, Action>;

    typedef Rule<Expr, Action>* Rule_ptr;

    static bool list_compare(Rule_ptr& rule1, Rule_ptr& rule2)
    {
        return rule1->priority < rule2->priority;
    }

    Rule(uint32_t id_, uint32_t priority_,
         const Expr& expr_, const Action& action_)
        : id(id_), priority(priority_), expr(expr_), action(action_), node(NULL) { }

    Rule(const Rule& r)
        : id(r.id), priority(r.priority), expr(r.expr),
          action(r.action), node(NULL) { }

    ~Rule() { }

    Cnode<Expr, Action> *get_node() const { return node; }

    const uint32_t id;
    uint32_t priority;
    const Expr expr;
    const Action action;

private:
    Cnode<Expr, Action> *node;

    Rule();
    Rule& operator=(const Rule&);
};


} // namespace vigil

#endif
