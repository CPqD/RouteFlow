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
#ifndef  CLASSIFIER_HH
#define  CLASSIFIER_HH

#include <errno.h>
#include <stdint.h>
#include <boost/scoped_ptr.hpp>
//#include <boost/shared_ptr.hpp>

#include "cnode.hh"
#include "errno_exception.hh"
#include "hash_map.hh"
#include "rule.hh"

/*
 * General packet classifier.
 *
 * Templated over the Expr and Action types used by the Rules the classifier
 * will hold.  These expressions are what are used to classify data (where data
 * can be a packet).  Currently only supports one tree internally, but could be
 * extended to have more if necessary.  First split on tree can be structured
 * or can be learned.  Essentially a wrapper for the Cnode class, but couples
 * with it a map of rule pointers allowing for easy removal of rules by ID
 * (instead of requiring a search of the entire tree).
 *
 * Expr should follow the model described by the example in "expr.hh".
 */

namespace vigil {

template<class Expr, typename Action>
class Classifier {

public:
    typedef Expr Expr_type;
    typedef Rule<Expr, Action>* Rule_ptr;
    typedef hash_map<uint32_t, Rule_ptr> Id_map;

    Classifier(uint32_t, int);
    Classifier();
    void reset(uint32_t, int);
    void reset();
    ~Classifier();

    uint32_t add_rule(uint32_t, const Expr&, const Action&);
    bool change_rule_priority(uint32_t, uint32_t);
    const Rule_ptr get_rule(uint32_t);
    bool delete_rule(uint32_t);
    template<typename Data>
    uint32_t delete_rules(const Data*);
    void build();
    void unbuild();
    void clean() { root->clean(); } /* deletes empty subtrees */

    template<typename Data>
    void get_rules(Cnode_result<Expr, Action, Data>&);
    void print() const;

private:
    boost::scoped_ptr<Cnode<Expr, Action> > root;
    std::vector<Cnode<Expr, Action>*> to_traverse;
    Id_map rules;
    uint32_t id_counter;

    uint32_t get_id();

    Classifier(const Classifier&);
    Classifier& operator=(const Classifier&);
};

/*
 * Constructs a tree that splits on 'split_field' first, with 'n_buckets'
 * initially.  'split_field' should be a valid single-bit mask understood by
 * Expr.  'n_buckets' should be a power of 2.
 */

template<class Expr, typename Action>
Classifier<Expr, Action>::Classifier(uint32_t split_field, int n_buckets)
    : id_counter(1)
{
    root.reset(new Cnode<Expr, Action>(split_field, n_buckets));
}


/*
 * Default constructor.
 */

template<class Expr, typename Action>
Classifier<Expr, Action>::Classifier()
    : id_counter(1)
{
    root.reset(new Cnode<Expr, Action>());
}


/*
 * Destructor
 */
template<class Expr, typename Action>
Classifier<Expr, Action>::~Classifier()
{
    root.reset();
    for (typename Id_map::const_iterator id = rules.begin();
         id != rules.end(); ++id)
    {
        delete id->second;
    }
}

/*
 * Resets tree with forced structure - removing all rules.
 */

template<class Expr, typename Action>
void
Classifier<Expr, Action>::reset(uint32_t split_field, int n_buckets)
{
    root.reset(new Cnode<Expr, Action>(split_field, n_buckets));
    for (typename Id_map::const_iterator id = rules.begin();
         id != rules.end(); ++id)
    {
        delete id->second;
    }
    rules.clear();
    id_counter = 1;
}


/*
 * Reset tree - removing all rules.
 */

template<class Expr, typename Action>
void
Classifier<Expr, Action>::reset()
{
    root.reset(new Cnode<Expr, Action>());
    for (typename Id_map::const_iterator id = rules.begin();
         id != rules.end(); ++id)
    {
        delete id->second;
    }
    rules.clear();
    id_counter = 1;
}


/*
 * Return the next available rule ID.  Throws ENOMEM (better errno) if ID could
 * not be allocated.
 */
template<class Expr, typename Action>
uint32_t
Classifier<Expr, Action>::get_id()
{
    bool loop = false;

    while (rules.find(id_counter) != rules.end()) {
        if (id_counter == ~((uint32_t)0)) {
            if (loop == true) {
                throw errno_exception(ENOMEM, "classifier::get_id");
            }
            loop = true;
            id_counter = 1;
        } else {
            id_counter++;
        }
    }

    uint32_t id = id_counter;
    if (id_counter == ~((uint32_t)0)) {
        id_counter = 1;
    }

    return id;
}

/*
 * Creates a rule from the passed in priority, expr, and action and adds it to
 * the classifier.  Returns the new rule's ID.
 */

template<class Expr, typename Action>
uint32_t
Classifier<Expr, Action>::add_rule(uint32_t priority, const Expr& expr,
                                   const Action& action)
{
    uint32_t new_id = get_id();

    std::pair<typename Id_map::iterator, bool> inserted;
    std::pair<uint32_t, Rule_ptr> entry(new_id,
                                        Rule_ptr(new Rule<Expr, Action>(new_id, priority,
                                                                expr, action)));

    inserted = rules.insert(entry);
    if (inserted.second == true) {
        try {
            root->add_rule(entry.second, 0);
        } catch (...) {
            rules.erase(inserted.first);
            delete entry.second;
            throw;
        }
    } else {
        delete entry.second;
        throw errno_exception(ENOMEM, "classifier::add_rule");
    }

    return new_id;
}


template<class Expr, typename Action>
bool
Classifier<Expr, Action>::change_rule_priority(uint32_t id, uint32_t priority)
{
    typename Id_map::iterator entry = rules.find(id);

    if (entry == rules.end()) {
        return false;
    }

    Cnode<Expr, Action> *node = entry->second->get_node();
    if (node == NULL) {
        return false;
    }

    return node->change_rule_priority(id, priority);
}

template<class Expr, typename Action>
const typename Classifier<Expr, Action>::Rule_ptr
Classifier<Expr, Action>::get_rule(uint32_t id)
{
    typename Id_map::iterator entry = rules.find(id);

    if (entry == rules.end()) {
        return NULL;
    }

    return entry->second;
}

/*
 * Deletes from the classifier the rules with id 'id'.  Returns 'true' if the
 * rule was found and deleted from the tree, 'false' if the rule was not found.
 * SHOULD ONLY BE CALLED IF CLASSIFIER IS IN A CONSISTENT STATE, ELSE RULE'S
 * NODE PTR MAY POINT INVALID MEMORY (only need to worry bout this if exception
 * thrown during classifier build())
 */

template<class Expr, typename Action>
bool
Classifier<Expr, Action>::delete_rule(uint32_t id)
{
    typename Id_map::iterator entry = rules.find(id);

    if (entry == rules.end()) {
        return false;
    }

    Cnode<Expr, Action> *node = entry->second->get_node();
    if (node != NULL) {
        node->remove_rule(entry->first);
    }

    delete entry->second;
    rules.erase(entry);
    return true;
}


/*
 * Deletes all rules from the classifier that match 'data', where data is some
 * matching type supported by Expr (e.g. a Flow or another Expr).
 * Returns the number of rules deleted.  Calls delete_rule, so should only be
 * called if tree is in a consistent state.
 */

template<class Expr, typename Action>
template<typename Data>
uint32_t
Classifier<Expr, Action>::delete_rules(const Data *data)
{
    Cnode_result<Expr, Action, Data> res(data);
    get_rules(res);

    uint32_t n_del = 0;

    while (const Rule<Expr, Action> *r = res.next()) {
        uint32_t id = r->id;
        if (delete_rule(r->id) == false) {
            printf("Rule %u not deleted\n", id);
        } else {
            n_del++;
        }
    }

    return n_del;
}

/*
 * Builds the tree.
 */

template<class Expr, typename Action>
void
Classifier<Expr, Action>::build()
{
    Cnode<Expr, Action> *tmp = root->build(0, false);
    if (tmp != root.get()) {
        root.reset(tmp);
    }
}


template<class Expr, typename Action>
void
Classifier<Expr, Action>::unbuild()
{
    Cnode<Expr, Action> *tmp = root->unbuild();
    if (tmp != root.get()) {
        root.reset(tmp);
    }
}

/*
 * Populates 'result's' priority queue with the rules that according to the
 * classifier structure may potentially match the data in 'result'.
 * Expr::matches() should be called to check if the expression fully matches
 * the data. (e.g. classifier may not have split on all defined fields, or
 * Expr may define fields that it does not allow Cnode to split on).
 * Cnode_result's iterator makes this matches() call internally.
 */

template<class Expr, typename Action>
template<typename Data>
void
Classifier<Expr, Action>::get_rules(Cnode_result<Expr, Action, Data>& result)
{
    root->traverse(result, to_traverse);
    while (!to_traverse.empty()) {
        Cnode<Expr, Action> *node = to_traverse.back();
        to_traverse.pop_back();
        node->traverse(result, to_traverse);
    }
}

template<class Expr, typename Action>
void
Classifier<Expr, Action>::print() const
{
    std::string str("*");
    root->print(str, false);
}

} // namespace vigil

#endif
