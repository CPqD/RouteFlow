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
#ifndef CLASSIFIER_TEST_HH
#define CLASSIFIER_TEST_HH

#include "classifier.hh"

/*
 * Classifier test class.
 *
 * Compares Classifier results to a linear
 * list classifier's results.
 */

template<class Expr, typename Action>
class Classifier_t {

public:
    typedef std::list<vigil::Rule<Expr, Action> > Rule_list;

    uint32_t check_add_rule(uint32_t, const Expr&, Action);
    bool check_delete_rule(uint32_t);

    template<class Data>
    bool check_delete_rules(const Data *data);

    template<class Data>
    bool check_lookup(const Data *);

    void build() { classifier.build(); }
    void unbuild() { classifier.unbuild(); }
    void clean() { classifier.clean(); }
    void print() const { classifier.print(); }

    vigil::Classifier<Expr, Action>& get_classifier()
        { return classifier; }

private:
    vigil::Classifier<Expr, Action> classifier;
    std::list<vigil::Rule<Expr, Action> > linear;

    template<class Data>
    bool resolve_priority(typename Rule_list::const_iterator&,
                          const Data *, const vigil::Rule<Expr, Action>*,
                          vigil::Cnode_result<Expr, Action, Data>&) const;
};


/*
 * Adds a rule to both classifiers.  Checks that the add either fails on both
 * classifiers or succeeds on both classifiers.  Returns true if outcomes are
 * equivalent, else false.
 */

template<class Expr, typename Action>
uint32_t
Classifier_t<Expr, Action>::check_add_rule(uint32_t priority, const Expr& expr,
                                           Action action)
{
    uint32_t id = classifier.add_rule(priority, expr, action);
    if (id == 0) {
        return 0;
    }

    for (typename Rule_list::const_iterator iter = linear.begin();
         iter != linear.end(); ++iter)
    {
        if (iter->id == id) {
            return 0;
        }
    }

    for (typename Rule_list::iterator iter = linear.begin();
         iter != linear.end(); ++iter)
    {
        if (iter->priority >= priority) {
            linear.insert(iter, vigil::Rule<Expr, Action>(id, priority, expr, action));
            return id;
        }
    }

    linear.push_back(vigil::Rule<Expr, Action>(id, priority, expr, action));
    return id;
}

/*
 * Deletes a rule from both classifiers.  Checks that the delete either fails
 * on both classifiers or succeeds on both classifiers.  Returns true if
 * outcomes are equivalent, else false.
 */

template<class Expr, typename Action>
bool
Classifier_t<Expr, Action>::check_delete_rule(uint32_t id)
{
    bool success = classifier.delete_rule(id);

    for (typename Rule_list::iterator iter = linear.begin();
         iter != linear.end(); ++iter)
    {
        if (iter->id == id) {
            linear.erase(iter);
            return success == true;
        }
    }
    return success == false;
}


/*
 * Deletes rules matching a criteria from both classifiers.  Checks that the
 * same number of rules are deleted from both classifiers.  Returns true if
 * outcomes are equivalent, else false.
 */

template<class Expr, typename Action>
template<class Data>
bool
Classifier_t<Expr, Action>::check_delete_rules(const Data *data)
{
    uint32_t c_del = classifier.delete_rules(data);

    typename Rule_list::iterator iter = linear.begin();

    uint32_t l_del = 0;
    while (iter != linear.end()) {
        if (matches(iter->id, iter->expr, *data)) {
            iter = linear.erase(iter);
            l_del++;
        } else {
            ++iter;
        }
    }

    return c_del == l_del;
}


/*
 * Checks that lookup of 'data' in the two classifiers returns equivalent rule
 * sets.  Returns true if the results are identical, else false.
 */

template<class Expr, typename Action>
template<class Data>
bool
Classifier_t<Expr, Action>::check_lookup(const Data *data)
{
    vigil::Cnode_result<Expr, Action, Data> result(data);

    classifier.get_rules(result);

    typename Rule_list::const_iterator iter = linear.begin();

    while (true) {
        const vigil::Rule<Expr, Action> *match = result.next();
        bool match_found = false;
        while (iter != linear.end()) {
            if (matches(iter->id, iter->expr, *data)) {
                if (match == NULL || (match->priority != iter->priority)) {
                    return false;
                } else if (iter->id == match->id) {
                    ++iter;
                } else if (!resolve_priority(iter, data, match, result)) {
                    return false;
                }
                match_found = true;
                break;
            }
            ++iter;
        }

        if (!match_found && iter == linear.end()) {
            return match == NULL;
        }
    }
}


/*
 * Matching rules of equivalent priority might be encountered in a different
 * order by the linear classifier.  This reconciles that by comparing rules of
 * the same priority to the result set concurrently.  Returns true if the
 * matches for the current priority are equivalent between the two classifiers.
 */

template<class Expr, typename Action>
template<class Data>
bool
Classifier_t<Expr, Action>::resolve_priority(typename Rule_list::const_iterator& liter,
                                             const Data *data,
                                             const vigil::Rule<Expr, Action> *match,
                                             vigil::Cnode_result<Expr, Action, Data>& result) const
{
    std::list<vigil::Rule<Expr, Action> > ms;

    ms.push_back(*liter);
    for (++liter; liter != linear.end() && liter->priority == match->priority;
        ++liter)
    {
        if (matches(liter->id, liter->expr, *data)) {
            ms.push_back(*liter);
        }
    }

    while (match != NULL) {
        typename Rule_list::iterator iter = ms.begin();
        for (; iter != ms.end(); ++iter)
        {
            if (iter->id == match->id) {
                break;
            }
        }

        if (iter == ms.end()) {
            return false;
        }

        ms.erase(iter);
        if (ms.empty()) {
            return true;
        }

        match = result.next();
    }

    return false;
}

#endif
