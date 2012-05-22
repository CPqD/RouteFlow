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
#ifndef  CNODE_HH
#define  CNODE_HH

#include <list>
#include <string>
#include <vector>
#include <assert.h>
#include <stdio.h>

#include "cnode-result.hh"
#include "rule.hh"

/*
 * Classifier node structure.
 *
 * Basic building block of classifier, encompassing all of the classifier's
 * "smarts."
 */

namespace vigil {

template<class Expr, typename Action>
class Cnode {

public:
    typedef Rule<Expr, Action>* Rule_ptr;
    typedef std::list<Rule_ptr> Rule_list;

    Cnode(uint32_t, int, uint32_t = 0);
    Cnode(uint32_t);
    Cnode();
    ~Cnode();

    void add_rule(const Rule_ptr&, uint32_t, bool = true);
    bool change_rule_priority(uint32_t, uint32_t);
    bool remove_rule(uint32_t);
    Cnode<Expr, Action> *build(uint32_t, bool);
    Cnode<Expr, Action> *unbuild();
    bool clean();
    template<typename Data>
    void traverse(Cnode_result<Expr, Action, Data>&, std::vector<Cnode<Expr, Action>*>&) const;

    void print(std::string&, bool) const;

    static const uint32_t MASKS[];

private:
    uint32_t value;

    Rule_list rules;

    int bucket_mask;                //
    Cnode<Expr, Action> **buckets;  // For internal nodes only...
    uint32_t split_field;           //
    Cnode<Expr, Action> *any_node;  //

    Cnode<Expr, Action> *next;      // for chaining

    void add_rule_to_list(const Rule_ptr&);
    void build_children(uint32_t, bool);
    Cnode<Expr, Action>* split(uint32_t, bool);
    void split_node(uint32_t, uint32_t, int);
    void set_node_pointers();
    void add_node_rules(Cnode<Expr, Action> *);
    uint32_t best_split(uint32_t, uint32_t&, int&) const;
    uint32_t exp_rules_with_split(uint32_t, std::vector<bool>&,
                                  int, int&) const;

    static int get_bucket(uint32_t, int);

    Cnode(const Cnode&);
    Cnode& operator=(const Cnode&);
};

template<class Expr, typename Action>
const uint32_t Cnode<Expr, Action>::MASKS[] = {
    1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4,
    1 << 5, 1 << 6, 1 << 7, 1 << 8, 1 << 9,
    1 << 10, 1 << 11, 1 << 12, 1 << 13, 1 << 14,
    1 << 15, 1 << 16, 1 << 17, 1 << 18, 1 << 19,
    1 << 20, 1 << 21, 1 << 22, 1 << 23, 1 << 24,
    1 << 25, 1 << 26, 1 << 27, 1 << 28, 1 << 29,
    1 << 30, 1 << 31
};

/*
 * Constructs a node split on 'field' with 'n_buckets', and value 'value_'.
 * 'value_' defaults to 0.  'field' should be an int 0 <= 'field' < 32 that can
 * be passed to an Expr to get a field's value. 'n_buckets' must be a power of
 * 2.
 *
 * Common use case: when caller wants to force the structure of the first level
 * of a tree.  Also used when creating a copy of the node to branch.
 */

template<class Expr, typename Action>
Cnode<Expr, Action>::Cnode(uint32_t field, int n_buckets, uint32_t value_)
    : value(value_), bucket_mask(n_buckets - 1), split_field(field),
      any_node(NULL), next(NULL)
{
    assert(split_field < 32);
    assert(n_buckets > 0 && ((n_buckets & (n_buckets - 1)) == 0));
    buckets = new Cnode<Expr, Action>*[n_buckets];
    memset(buckets, 0, n_buckets * (sizeof *buckets));
}


/*
 * Constructs a leaf node with value equal to 'value_'.
 *
 * Common use case: a node needs to be created for a specific rule's value for
 * a field.
 */

template<class Expr, typename Action>
Cnode<Expr, Action>::Cnode(uint32_t value_)
    : value(value_), bucket_mask(-1), buckets(NULL), any_node(NULL), next(NULL)
{ }


/*
 * Default constructor:  constructs a leaf node, setting value = 0
 */

template<class Expr, typename Action>
Cnode<Expr, Action>::Cnode()
    : value(0), bucket_mask(-1), buckets(NULL), any_node(NULL), next(NULL)
{ }


/*
 * Destructs all descendents.  Modifies any Rule 'node' pointers in
 * any destroyed rule lists that point to self to point to NULL.  This check is
 * needed because the tree being deleted could be a backup of a master tree, in
 * which case the node pointers  will be pointing to the master.
 */

template<class Expr, typename Action>
Cnode<Expr, Action>::~Cnode()
{

    for (typename Rule_list::iterator iter = rules.begin();
         iter != rules.end(); ++iter)
    {
        if ((*iter)->node == this) {
            (*iter)->node = NULL;
        }
    }

    if (bucket_mask < 0) {
        return;
    }

    for (int i = 0; i <= bucket_mask; i++) {
        while (buckets[i] != NULL) {
            Cnode<Expr, Action> *tmp = buckets[i];
            buckets[i] = tmp->next;
            delete tmp;
        }
    }

    delete [] buckets;
    if (any_node != NULL) {
        delete any_node;
    }
}

template<class Expr, typename Action>
void
Cnode<Expr, Action>::add_rule_to_list(const Rule_ptr& rule)
{
    for (typename Rule_list::iterator iter = rules.begin();
         iter != rules.end(); ++iter)
    {
        if (rule->priority <= (*iter)->priority) {
            rules.insert(iter, rule);
            return;
        }
    }
    rules.push_back(rule);
}


/*
 * Adds 'rule' to the sub-tree rooted at the current node, either adding the
 * rule to the node's 'rules' list, or recursively calling 'add_rule' on the
 * appropriate child node.  'path' denotes the fields that were branched on to
 * reach the current node (used to determine if the rule has only wildcard
 * fields left, meaning it can be added to the current node even if the node
 * splits).  'set_pointer' signals if the rule's Cnode pointer should be
 * modified to point to the node it is added to.  When a client is adding a
 * rule this should be true, however when a duplicate copy of the tree is being
 * made, 'set_pointer' should be set to false because the master tree shouldn't
 * be undergoing any changes.  Currently 'set_pointer' defaults to true.
 * If an exception is thrown, the caller can assume that 'rule' has not been
 * added to the node.
 */

template<class Expr, typename Action>
void
Cnode<Expr, Action>::add_rule(const Rule_ptr& rule, uint32_t path,
                              bool set_pointer)
{
    const Expr& expr = rule->expr;

    if (bucket_mask < 0 || !expr.splittable(path)) {
        add_rule_to_list(rule);
        if (set_pointer) {
            rule->node = this;
        }
        return;
    }

    Cnode<Expr, Action> *child;
    uint32_t rule_value;

    if (expr.get_field(split_field, rule_value)) {
        int bucket = get_bucket(rule_value, bucket_mask);
        for (child = buckets[bucket]; child != NULL; child = child->next) {
            if (child->value == rule_value) {
                break;
            }
        }

        if (child == NULL) {
            child = new Cnode(rule_value);
            child->next = buckets[bucket];
            buckets[bucket] = child;
        }
    } else {
        if (any_node == NULL) {
            any_node = new Cnode();
        }
        child = any_node;
    }

    child->add_rule(rule, path | MASKS[split_field], set_pointer);
}


/*
 * Change priority of rule with 'id' in this node to 'priority' and reorder in
 * rule list as appropriate.
 */

template<class Expr, typename Action>
bool
Cnode<Expr, Action>::change_rule_priority(uint32_t id, uint32_t priority)
{
    for (typename Rule_list::iterator iter = rules.begin();
         iter != rules.end(); ++iter)
    {
        if ((*iter)->id == id) {
            Rule_ptr r = *iter;
            rules.erase(iter);
            r->priority = priority;
            add_rule_to_list(r);
            return true;
        }
    }

    return false;
}


/*
 * Removes rule with 'id' from the node's 'rules' lists and sets its Cnode
 * pointer to NULL if it is pointing to the current node.  Returns 'true' if
 * the rule was found and thus removed from the list, else returns 'false'.
 */

template<class Expr, typename Action>
bool
Cnode<Expr, Action>::remove_rule(uint32_t id)
{
    for (typename Rule_list::iterator iter = rules.begin();
         iter != rules.end(); ++iter)
    {
        if ((*iter)->id == id) {
            if ((*iter)->node == this) {
                (*iter)->node = NULL;
            }
            rules.erase(iter);
            return true;
        }
    }

    return false;
}


/*
 * Builds the node.  'path' denotes the fields that were split on to reach the
 * node, to avoid attempts to check for optimal splitting on redundant fields.
 * To avoid an inconsistent tree should an exception be thrown during building,
 * a copy of any node in the master tree that should be split is made, the
 * branching is completed on the copy, and then eventually the copy is swapped
 * into the master tree.  Thus if true, 'is_copy' signals that the current node
 * is not part of the master tree and thus can be split on directly.
 * Conversely, if false, if the current node need be split (or, if already
 * split, any of its descendents need be split), a copy should be made first,
 * and then the split be performed on the copy.  Returns a pointer to the
 * newest version of the current node, which will be 'this' if the current node
 * itself did not need to be copied, or a pointer to a newly allocated node if
 * the node was part of the master, but needed to be modified.  The caller then
 * is in charge of making sure the parent of the node build() was called on
 * points to the new node.
 *
 * Rule Cnode pointers are set if 'new_this' is the root of the sub-tree that
 * will be switched into the master tree.
 *
 * If an exception is thrown, guarantees that the master tree is still
 * consistent, and frees any copies that were created in the build attempt.
 */

template<class Expr, typename Action>
Cnode<Expr, Action> *
Cnode<Expr, Action>::build(uint32_t path, bool is_copy)
{
    Cnode<Expr, Action> *new_this = this;

    if (bucket_mask < 0) {
        if (rules.size() <= Expr::LEAF_THRESHOLD) {
            return this;
        }
        new_this = split(path, is_copy);
        if (new_this->bucket_mask < 0) {
            return new_this;
        }
    }

    path = path | MASKS[new_this->split_field];

    bool new_this_is_copy = is_copy || (new_this != this);

    try {
        new_this->build_children(path, new_this_is_copy);
    } catch (...) {
        // if this is the head of the copied tree, free it
        if (!is_copy && new_this_is_copy) {
            delete new_this;
        }
        throw;
    }

    // if this is the head of the copied tree, build was
    // successful - now can set the rule pointers.
    if (!is_copy && new_this_is_copy) {
        new_this->set_node_pointers();
    }

    return new_this;
}


/*
 * Builds the children of a node.  If the node is a copy, the children can be
 * built knowing that copies will not be made of them.  If however the node is
 * not a copy, each built child may be a new sub-tree, in which case the old
 * one should be freed.
 */

template<class Expr, typename Action>
void
Cnode<Expr, Action>::build_children(uint32_t path, bool is_copy)
{
    if (is_copy) {
        for (int i = 0; i <= bucket_mask; i++) {
            for (Cnode<Expr, Action> *child = buckets[i]; child != NULL;
                 child = child->next)
            {
                child->build(path, is_copy);
            }
        }

        if (any_node) {
            any_node->build(path, is_copy);
        }
        return;
    }

    for (int i = 0; i <= bucket_mask; i++) {
        Cnode<Expr, Action> *child = buckets[i];
        Cnode<Expr, Action> **prev = &buckets[i];
        while (child != NULL) {
            child = child->build(path, is_copy);
            if (child != *prev) {
                delete *prev;
                *prev = child;
            }
            prev = &((*prev)->next);
            child = *prev;
        }
    }

    if (any_node) {
        Cnode<Expr, Action> *new_node = any_node->build(path, is_copy);
        if (new_node != any_node) {
            delete any_node;
            any_node = new_node;
        }
    }
}



/*
 * Unbuilds the node, unbuilding child nodes if necessary.  First makes a copy
 * of the node, adds all sub-tree's rules to it without modifying the rules'
 * Cnode pointers.  Once the unbuilding has completed, sets the rules' Cnode
 * pointers to point to the copied node, and returns a pointer to the
 * newly-unbuilt node.  Caller is in charge of making sure the parent of the
 * node unbuild() was called on points to the new node.  If the node was not
 * branched on, and thus does not need to be unbuilt, 'this' is returned.
 *
 * If an exception is thrown during the unbuilding process, the tree remains in
 * a consistent state and the copied memory is freed.
 */

template<class Expr, typename Action>
Cnode<Expr, Action>*
Cnode<Expr, Action>::unbuild()
{
    if (bucket_mask < 0) {
        return this;
    }

    Cnode<Expr, Action> *new_this = new Cnode<Expr, Action>(value);

    try {
        new_this->add_node_rules(this);
    }  catch (...) {
        delete new_this;
        throw;
    }

    new_this->set_node_pointers();
    return new_this;
}


/*
 * Optimally splits a node on a field.  'path' denotes the fields that were
 * split on to reach the node, and thus the fields do not need to be checked
 * for optimality.  'is_copy' signals if the node split() is being called on is
 * a copy of a node in the master tree, or is actually the master tree,
 * determining if the node will be copied or modified itself as described in
 * the build().  Rules' Cnode pointers are not modified here because other
 * nodes may need to be built as well.  Thus set_pointers on this node must be
 * called somewhere higher in the call chain if this node is split.  Returns a
 * pointer to 'this' if the node is either not split or it is a copy, else
 * returns a pointer to the new version of the current node.
 *
 * Splitting heuristic: Selects the field that results in the minimum average
 * number of rules needed to be checked linearly for a given packet after the
 * split, assuming the packet will only take on values that rules have been
 * defined on (which isn't actually the case).  Only splits if the average
 * savings is >= Expr::LEAF_THRESHOLD.
 */

template<class Expr, typename Action>
Cnode<Expr, Action> *
Cnode<Expr, Action>::split(uint32_t path, bool is_copy)
{
    int n_distinct;
    uint32_t n_branch_rules;
    uint32_t field = best_split(path, n_branch_rules, n_distinct);

    if (field == Expr::NUM_FIELDS || (rules.size() - n_branch_rules < Expr::LEAF_THRESHOLD)) {
        return this;
    }

    int n_buckets = 1;
    while (n_buckets < n_distinct * 3) {
        n_buckets = n_buckets << 1;
    }

    if (is_copy) {
        split_node(path, field, n_buckets);
        return this;
    }

    Cnode<Expr, Action> *new_node = new Cnode<Expr, Action>(field, n_buckets, value);
    new_node->next = next;

    try {
        for (typename Rule_list::iterator iter = rules.begin();
             iter != rules.end(); ++iter)
        {
            new_node->add_rule(*iter, path, false);
        }
    } catch (...) {
        delete new_node;
        throw;
    }

    return new_node;

}


template<class Expr, typename Action>
void
Cnode<Expr, Action>::split_node(uint32_t path, uint32_t field, int n_buckets)
{
    buckets = new Cnode<Expr, Action>*[n_buckets];
    memset(buckets, 0, n_buckets * (sizeof *buckets));
    bucket_mask = n_buckets - 1;
    split_field = field;
    any_node = NULL;

    Rule_list tmp;
    tmp.swap(rules);

    for (typename Rule_list::iterator iter = tmp.begin();
         iter != tmp.end(); ++iter)
    {
        add_rule(*iter, path, false);
    }
}

/*
 * Sets all the sub-tree's rules' Cnode pointers to point to their respective
 * nodes in the sub-tree.  Called when a copy of the tree is ready to be the
 * master.
 */

template<class Expr, typename Action>
void
Cnode<Expr, Action>::set_node_pointers()
{
    for (typename Rule_list::iterator iter = rules.begin();
         iter != rules.end(); ++iter)
    {
        (*iter)->node = this;
    }

    if (bucket_mask < 0) {
        return;
    }

    for (int i = 0; i <= bucket_mask; i++) {
        for (Cnode<Expr, Action> *child = buckets[i]; child != NULL;
             child = child->next)
        {
            child->set_node_pointers();
        }
    }

    if (any_node != NULL) {
        any_node->set_node_pointers();
    }
}


template<class Expr, typename Action>
void
Cnode<Expr, Action>::add_node_rules(Cnode<Expr, Action> *node)
{
    Rule_list tmp(node->rules);
    rules.merge(tmp, Rule<Expr, Action>::list_compare);

    if (node->bucket_mask < 0) {
        return;
    }

    for (int i = 0; i <= node->bucket_mask; i++) {
        for (Cnode<Expr, Action> *child = node->buckets[i]; child != NULL;
             child = child->next)
        {
            add_node_rules(child);
        }
    }

    if (node->any_node) {
        add_node_rules(node->any_node);
    }
}


/*
 * Returns the optimal field to split the node's list of rules on.  'path'
 * denotes the fields already split on to reach the current node.  Sets
 * 'min_rules' to the avg. number of rules that would need to be checked after
 * the split (includes ANYs), and sets min_distinct to the number of distinct
 * hash buckets the rules hashed to.  Returns Expr::NUM_FIELDS if a split
 * cannot be made (e.g. all fields have been split on).
 */

template<class Expr, typename Action>
uint32_t
Cnode<Expr, Action>::best_split(uint32_t path, uint32_t& min_rules,
                                int& min_distinct) const
{
    int n_rules = rules.size();
    int n_buckets = 1;

// need limit?
    while (n_buckets < n_rules * 3) {
        n_buckets = n_buckets << 1;
    }

    std::vector<bool> tmp_buckets(n_buckets);

    min_rules = 0;
    uint32_t best_field = Expr::NUM_FIELDS;
    int n_distinct;

    for (uint32_t i = 0; i < Expr::NUM_FIELDS; i++) {
        if ((path & MASKS[i]) == 0) {
            uint32_t exp = exp_rules_with_split(i, tmp_buckets,
                                                n_buckets-1, n_distinct);
            if (best_field == Expr::NUM_FIELDS || exp < min_rules) {
                min_rules = exp;
                best_field = i;
                min_distinct = n_distinct;
            }
        }
    }

    return best_field;
}


/*
 * Returns the expected number of rules that would be need to be checked after
 * a split of 'rules' on field 'field'.  Sets 'n_distinct' to be the number of
 * distinct hash buckets the rules hashed to.
 */

template<class Expr, typename Action>
uint32_t
Cnode<Expr, Action>::exp_rules_with_split(uint32_t field,
                                          std::vector<bool>& tmp_buckets,
                                          int mask,
                                          int& n_distinct) const
{
    int bucket;
    uint32_t n_anys, n_def, rule_value;
    n_anys = n_def = n_distinct = 0;

    for (int i = 0; i <= mask; i++) {
        tmp_buckets[i] = false;
    }

    for (typename Rule_list::const_iterator iter = rules.begin();
         iter != rules.end(); ++iter)
    {
        if ((*iter)->expr.get_field(field, rule_value)) {
            n_def++;
            bucket = get_bucket(rule_value, mask);
            if (tmp_buckets[bucket] == false) {
                tmp_buckets[bucket] = true;
                n_distinct++;
            }
        } else {
            n_anys++;
        }
    }

    if (n_distinct == 0) {
        return n_anys;
    }

    return n_anys + (n_def / n_distinct);
}


#define HASH_MULTIPLIER 2654435761U


/*
 * Returns the hash bucket for the value pointed to by 'value' in a hash table
 * with bucket mask 'bucket_mask'.
 */

template<class Expr, typename Action>
int
Cnode<Expr, Action>::get_bucket(uint32_t value, int mask)
{
    return (value * HASH_MULTIPLIER) & mask;
}


/*
 * Traverses the tree according to the data in 'result', asking the Expr class
 * for 'data's' values of the different split fields.  Populates 'result's'
 * priority queue with rules that may apply to the data. The Cnode_result
 * iterator further checks if the rule actually applies to the 'data' by
 * calling matches().  'data' can be a Flow for example.
 */

template<class Expr, typename Data>
bool get_field(uint32_t, const Data&, uint32_t, uint32_t&);

template<class Expr, typename Action>
template<typename Data>
void
Cnode<Expr, Action>::traverse(Cnode_result<Expr, Action, Data>& result,
                              std::vector<Cnode<Expr, Action>*>& to_traverse) const
{
    uint32_t rule_value;

    result.push(rules);

    if (bucket_mask < 0) {
        return;
    }

    if (any_node != NULL) {
        to_traverse.push_back(any_node);
    }

    uint32_t idx = 0;

    while (get_field<Expr, Data>(split_field, *(result.data), idx, rule_value)) {
        int bucket = get_bucket(rule_value, bucket_mask);
        for (Cnode<Expr, Action> *child = buckets[bucket]; child != NULL;
             child = child->next)
        {
            if (child->value == rule_value) {
                to_traverse.push_back(child);
                break;
            }
        }
        idx++;
    }

    if (idx == 0) {
        for (int i = 0; i <= bucket_mask; i++) {
            for (Cnode<Expr, Action> *child = buckets[i]; child != NULL;
                 child = child->next)
            {
                to_traverse.push_back(child);
            }
        }
    }
}


/*
 * Deletes any empty subtrees by recursively calling clean() on child nodes.
 * Returns 'true' if the node contains no subtrees and no rules (and thus the
 * node can be deleted by the parent), else 'false'.
 */

template<class Expr, typename Action>
bool
Cnode<Expr, Action>::clean()
{
    bool is_clean = rules.size() == 0;

    if (bucket_mask < 0) {
        return is_clean;
    }

    for (int i = 0; i <= bucket_mask; i++) {
        Cnode<Expr, Action> *child = buckets[i];
        Cnode<Expr, Action> **prev = &buckets[i];
        while (child != NULL) {
            if (child->clean()) {
                *prev = child->next;
                delete child;
                child = *prev;
            } else {
                is_clean = false;
                prev = &child->next;
                child = child->next;
            }
        }
    }

    if (any_node != NULL) {
        if (any_node->clean()) {
            delete any_node;
            any_node = NULL;
        } else {
            is_clean = false;
        }
    }

    return is_clean;
}

/*
 * Prints out the tree in the following format:
 * <level> <value> LEAF | SPLIT <field> <rule IDs>
 *
 * A node's parent is the first one preceding it in the list that has the lower
 * level.
 */

template<class Expr, typename Action>
void
Cnode<Expr, Action>::print(std::string& level, bool any) const
{
    printf("%s ", level.c_str());
    if (any) {
        printf("any");
    } else {
        printf("0x");
        const uint8_t *byte_ptr = (const uint8_t *) &value;
        for (uint32_t i = 0; i < sizeof value; i++)
            printf("%.2hhx", byte_ptr[i]);
    }

    if (bucket_mask < 0) {
        printf(" LEAF");
    } else {
        printf(" SPLIT %u", split_field);
    }

    for (typename Rule_list::const_iterator iter = rules.begin();
         iter != rules.end(); ++iter)
    {
        printf(" %u", (*iter)->id);
    }

    printf("\n");

    if (bucket_mask < 0) {
        return;
    }

    level.append(1, '*');

    if (any_node != NULL) {
        any_node->print(level, true);
    }

    for (int i = 0; i <= bucket_mask; i++) {
        for (Cnode *child = buckets[i]; child != NULL;
             child = child->next)
        {
            child->print(level, false);
        }
    }

    level.erase(--level.end());
}

} // namespace vigil

#endif
