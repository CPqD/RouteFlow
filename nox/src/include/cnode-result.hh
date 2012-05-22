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
#ifndef CNODE_RESULT_HH
#define CNODE_RESULT_HH

#include "rule.hh"

/*
 * Classifier result rule set holder.
 *
 * Templated over the Expr and Action types used by the Rules the result set
 * will hold.  Is also templated over the data type that the Rules' expressions
 * should be checked for matches against (e.g. a packet, or another Expr, etc).
 *
 * Usage: Should be constructed with the data to classify, and then passed to
 * Classifier::get_rules(), which will fill in the set's priority queue with
 * matching rules.  Matches at this point are only based on the tree traversal,
 * and don't necessarily imply a full match, and thus iteration through the
 * result set should be done with the next() method, which will skip over any
 * rules in the priority queue that don't fully match 'data'.
 */

#define DEFAULT_VEC_SIZE  10

namespace vigil {

template<class Expr, typename Action, typename Data>
class Cnode_result {

public:
    friend class Cnode<Expr, Action>;

    typedef Rule<Expr, Action>* Rule_ptr;
    typedef std::list<Rule_ptr> Rule_list;

    Cnode_result(const Data *data_)
        : data(data_), num_lists(0) {}

    void push(const Rule_list&);
    const Rule<Expr, Action>* next();
    void set_data(const Data *data_) { data = data_; }
    void clear() { num_lists = 0; }

private:
    struct current_rule {
        current_rule()
            : ismatch(false), set(false) {}

        current_rule(const Rule_list& rules)
            : rule(rules.begin()), end(rules.end()), ismatch(false), set(true) { }

        current_rule(const current_rule& other)
            : ismatch(false), set(other.set) {
            if (set) {
                rule = other.rule;
                end = other.end;
                ismatch = other.ismatch;
            }
        }

        current_rule& operator=(const current_rule& other) {
            set = other.set;
            if (set) {
                rule = other.rule;
                end = other.end;
                ismatch = other.ismatch;
            }
            return *this;
        }
        typename std::list<Rule_ptr>::const_iterator rule;
        typename std::list<Rule_ptr>::const_iterator end;
        bool ismatch;
        bool set;
    };

    const Data *data;
    uint32_t num_lists;
    std::vector<current_rule> traversed;

    Cnode_result();
    Cnode_result(const Cnode_result&);
    Cnode_result& operator=(const Cnode_result&);
};


template<class Expr, typename Action, typename Data>
void
Cnode_result<Expr, Action, Data>::push(const Rule_list& rules)
{
    if (!rules.empty()) {
        current_rule r(rules);
        if (num_lists == traversed.size()) {
            traversed.push_back(r);
        } else {
            traversed[num_lists] = r;
        }
        ++num_lists;
    }
}


/*
 * Returns the next rule in the priority queue, popping it off the list.
 * shared_ptr points to NULL if no matching rules remain.
 */

template<class Expr, typename Data>
bool matches(uint32_t, const Expr&, const Data&);

template<class Expr, typename Action, typename Data>
const Rule<Expr, Action>*
Cnode_result<Expr, Action, Data>::next()
{
    const Rule<Expr, Action>* match = NULL;
    uint32_t min_pri = 0;
    uint32_t min_idx = 0;

    for (uint32_t i = 0; i < num_lists;) {
        current_rule& current = traversed[i];
        const Rule<Expr, Action>* rule = *(current.rule);
        if (match == NULL || rule->priority < min_pri) {
            if (current.ismatch || matches(rule->id, rule->expr, *data)) {
                match = rule;
                min_pri = match->priority;
                min_idx = i;
                current.ismatch = true;
                ++i;
            } else {
                ++(current.rule);
                if (current.rule == current.end) {
                    current = traversed[--num_lists];
                }
            }
        } else {
            ++i;
        }
    }

    if (match != NULL) {
        current_rule& current = traversed[min_idx];
        ++(current.rule);
        if (current.rule == current.end) {
            current = traversed[--num_lists];
        } else {
            current.ismatch = false;
        }
    }

    return match;

}

}

#endif
