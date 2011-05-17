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
#include "dht-impl.hh"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/variant/get.hpp>

#include "vlog.hh"

using namespace std;
using namespace vigil;
using namespace vigil::applications::storage;

static Vlog_module lg("dht-impl");

DHT_entry::DHT_entry() : next_tid(0) { }

DHT_entry::DHT_entry(const Reference& id_) : id(id_), next_tid(0) { }

DHT_entry::~DHT_entry() { }

void
DHT_entry::put_trigger(DHT_ptr& ring,const Reference& current_row, 
                       const Trigger_function& f, 
                       const Async_storage::Put_trigger_callback& cb) {
    if (!(id == current_row)) {
        ring->dispatcher->post
            (boost::bind(cb, Result(Result::CONCURRENT_MODIFICATION, 
                                    "Row modified since retrieval."), 
                         Trigger_id("", Reference(),-1)));
        return;
    }

    Trigger_id tid = ring->trigger_instance(id, next_tid++);
    Trigger_map& triggers = nonsticky_triggers;
    triggers[tid] = f;
    ring->dispatcher->post(boost::bind(cb, Result(), tid));
}

const Result
DHT_entry::remove_trigger(DHT_ptr& ring, const Trigger_id& tid) {
    if (!(id == tid.ref)) {
        /* Trigger modified, a likely race condition between the
           trigger invocation and application. */
        return Result(Result::CONCURRENT_MODIFICATION,
                      "Row modified since trigger insertion.");
    }

    nonsticky_triggers.erase(tid);
    return Result();
}

void
Index_DHT_entry::get(const Content_DHT_ptr& content_ring, Context& ctxt,
                     const Async_storage::Get_callback& cb) const {
    if (refs.empty()) {
        content_ring->dispatcher->post
            (boost::bind(cb, Result(Result::NO_MORE_ROWS, "No more rows."), 
                         ctxt, Row()));
        return;
    }

    const Reference& ref = refs[0];
    //lg.dbg("id = %s, get index entry for %s", id.str().c_str(),
    //       ref.str().c_str());

    ctxt.index_row = id;
    content_ring->dispatcher->post(boost::bind(&Content_DHT::get, content_ring,
                                               content_ring, ctxt, ref, true, 
                                               cb));
}

void
Index_DHT_entry::get_next(const Content_DHT_ptr& content_ring, Context& ctxt,
                          const Async_storage::Get_callback& cb) const {
    /* Check the index entry has not been modified since the first
       get(). */
    if (!(ctxt.index_row == id)) {
        content_ring->dispatcher->post
            (boost::bind(cb, Result(Result::CONCURRENT_MODIFICATION, 
                                    "Index modified while iterating."),
                         ctxt, Row()));
        return;        
    }

    References::const_iterator j = refs.begin();
    for (; j != refs.end() && !(ctxt.current_row == *j); ++j);
    if (j != refs.end()) { ++j; };
    
    /* Attempt to get the next row. */
    if (j != refs.end()) {
        content_ring->dispatcher->post
            (boost::bind(&Content_DHT::get, content_ring, content_ring, ctxt, 
                         *j, true, cb));
    } else {
        content_ring->dispatcher->post
            (boost::bind(cb, Result(Result::NO_MORE_ROWS, "No more rows."), 
                         ctxt, Row()));
    }
}

void 
Index_DHT_entry::put(Index_DHT_ptr& index_ring, const Reference& ref, 
                     const Row& row, const Trigger_function& tfunc, 
                     const Index_updated_callback& cb,
                     const Trigger_reason reason) {
    //lg.dbg("id = %s, Adding index entry for %s", id.str().c_str(),
    //         ref.str().c_str());
    /* Delete the previous reference to the primary row, if it exists. */
    /*for (References::iterator i = refs.begin(); i != refs.end(); ++i) {
        if (i->guid == ref.guid && *i < ref) { 
            refs.erase(i);
            break;
        }
        }*/

    /* Store the reference and execute triggers */
    refs.push_back(ref);
    id.version++;

    /* Enqueue the triggers for invocation */
    BOOST_FOREACH(Trigger_map::value_type t, nonsticky_triggers) { 
        index_ring->dispatcher->post(boost::bind(t.second, t.first, row,
                                                 reason));
    }
    
    nonsticky_triggers.clear();

    /* Replace any previous primary triggers */
    /*    Reference_trigger_def_map new_primary_triggers;
    BOOST_FOREACH(Reference_trigger_def_map::value_type t, primary_triggers) { 
        if (!(t.first.guid == ref.guid && t.first < ref)) {
            new_primary_triggers.insert(t); 
        }
        }
    primary_triggers = new_primary_triggers;*/

    /* Install the new trigger for the primary row */
    Trigger_id tid = index_ring->trigger_instance(id, next_tid++);
    primary_triggers[ref] = make_pair(tid, tfunc);

    index_ring->dispatcher->post
        (boost::bind(cb, pair<Trigger_id, Trigger_function>
                     (tid, boost::bind(&Index_DHT::primary_deleted_callback, 
                                       index_ring, index_ring, _1, _2,
                                       row, 
                                       id.guid, ref))));
}

void 
Index_DHT_entry::modify(Index_DHT_ptr& index_ring, const Reference& prev_ref, 
                        const Reference& ref, const Row& prev_row, 
                        const Row& row, const Trigger_function& tfunc, 
                        const Index_updated_callback& cb,
                        const Trigger_reason reason) {
    //lg.dbg("id %s, Modifying index entry for prev %s, now %s", id.str().c_str(),
    //      prev_ref.str().c_str(),
    //       ref.str().c_str());
    /* Delete the previous reference to the primary row, if it exists. */
    References::iterator i = refs.begin(); 
    for (; i != refs.end(); ++i) {
        if (*i == prev_ref) {
            refs.erase(i);
            break;
        }
    }

    /* Store the new reference and execute triggers */
    refs.push_back(ref);
    id.version++;

    //lg.dbg("refs # = %d", refs.size());

    /* Enqueue the triggers for invocation */
    BOOST_FOREACH(Trigger_map::value_type t, nonsticky_triggers) { 
        index_ring->dispatcher->post(boost::bind(t.second, t.first, prev_row,
                                                 reason));
    }
    
    nonsticky_triggers.clear();

    /* Remove any previous row related primary triggers */
    Reference_trigger_def_map new_primary_triggers;
    BOOST_FOREACH(Reference_trigger_def_map::value_type t, primary_triggers) { 
        if (!(t.first == prev_ref)) {
            new_primary_triggers.insert(t); 
        }
    }
    primary_triggers = new_primary_triggers;

    /* Install the new trigger for the primary row */
    Trigger_id tid = index_ring->trigger_instance(id, next_tid++);
    primary_triggers[ref] = make_pair(tid, tfunc);

    index_ring->dispatcher->post
        (boost::bind(cb, pair<Trigger_id, Trigger_function>
                     (tid, boost::bind(&Index_DHT::primary_deleted_callback, 
                                       index_ring, index_ring, _1, _2,
                                       row, id.guid, ref))));
}

const Result
Index_DHT_entry::remove(Index_DHT_ptr& index_ring, const Reference& ref, 
                        const Row& row, const Trigger_reason reason) {
    //lg.dbg("id %s, Removing index entry for %s", id.str().c_str(),
    //       ref.str().c_str());

    /* Remove the reference and enqueue the all triggers for
       invocation */
    References::iterator i = refs.begin();
    for (; i != refs.end(); ++i) {
        if (*i == ref) break;
    }

    if (i == refs.end()) {
        return Result(Result::CONCURRENT_MODIFICATION,
                      "Reference does not exist anymore.");
    }

    refs.erase(i);
    id.version++;

    BOOST_FOREACH(Trigger_map::value_type t, nonsticky_triggers) { 
        index_ring->dispatcher->post(boost::bind(t.second, t.first, row, 
                                                 reason));
    }

    /* Invoke only primary triggers matching to the removed primary
       row reference. */
    Reference_trigger_def_map new_primary_triggers;
    BOOST_FOREACH(Reference_trigger_def_map::value_type t, primary_triggers) { 
        if (t.first == ref) {
            index_ring->dispatcher->post(boost::bind(t.second.second, 
                                                     t.second.first, row,
                                                     reason));
        } else {
            new_primary_triggers.insert(t);
        }
    }
    
    primary_triggers = new_primary_triggers;
    nonsticky_triggers.clear();

    return Result();
}

bool 
Index_DHT_entry::empty() {
    return nonsticky_triggers.empty() && refs.empty();
}

void
Content_DHT_entry::get(const Content_DHT_ptr& content_ring, Context& ctxt, 
                       const Reference& id_, const bool exact_match,
                       const Async_storage::Get_callback& cb) const 
{
    /* If version mismatch, index DHT is in inconsistent state
       momentarily.  Caller should re-try if caller told so. */
    if (!(id == id_) && exact_match) {
        content_ring->dispatcher->post
            (boost::bind(cb, Result(Result::CONCURRENT_MODIFICATION, 
                                    "Row modified since its retrieval/"
                                    "index creation."), ctxt, Row()));
        return;
    }
    ctxt.initial_row = id;
    ctxt.current_row = id;

    content_ring->dispatcher->post(boost::bind(cb, Result(), ctxt, row));
}

void
Content_DHT_entry::get_next(const Content_DHT_ptr& content_ring, Context& ctxt, 
                            const Async_storage::Get_callback& cb) const {
    ctxt.current_row = id;
    content_ring->dispatcher->post(boost::bind(cb, Result(), ctxt, row));
}

typedef map<GUID, boost::function<void()> > Table_GUID_update_function_map;
typedef map<DHT_name, Table_GUID_update_function_map> GUID_update_function_map;

const Result
Content_DHT_entry::put(Content_DHT_ptr& content_ring, const Context& ctxt, 
                       const GUID_index_ring_map& new_sguids, const Row& row_) {
    /* Check the version, i.e., that no one inserted a row between. */
    if (!(id == ctxt.current_row)) {
        return Result(Result::CONCURRENT_MODIFICATION, 
                      "Row modified since retrieval.");
    }

    Trigger_map t(nonsticky_triggers.begin(), nonsticky_triggers.end());
    nonsticky_triggers.clear();

    /* Update the row content and wipe any index triggers. */
    Reference prev = id;
    id.version++;
    row = row_;
    sguids = new_sguids;
    assert(index_triggers.empty());

    GUID_update_function_map updates;
    
    /* Create new index entries and their corresponding triggers. */
    BOOST_FOREACH(GUID_index_ring_map::value_type j, new_sguids) {
        const DHT_name& index_name = j.first;
        Index_DHT_ptr& index_ring = j.second.first;
        const GUID& sguid = j.second.second;

        Trigger_function tfunc = boost::bind(&Content_DHT::
                                             index_entry_deleted_callback, 
                                             content_ring, content_ring, id, 
                                             _1, _2);
        Index_updated_callback cb = boost::bind(&Content_DHT::
                                                index_entry_updated_callback,
                                                content_ring, id, index_name, 
                                                sguid, row, _1);
        updates[index_name][sguid] = 
            boost::bind(&Index_DHT::put, index_ring, index_ring,
                        sguid, id, row, tfunc, cb, INSERT);
    }

    //assert(new_sguids.size() == updates.size());

    /* Execute the index updates */
    BOOST_FOREACH(GUID_update_function_map::value_type i, updates) {
        BOOST_FOREACH(Table_GUID_update_function_map::value_type j, i.second) {
            content_ring->dispatcher->post(j.second);
        }
    }
    
    /* Execute application level index triggers. */
    BOOST_FOREACH(Trigger_map::value_type i, t) { 
        content_ring->dispatcher->post(boost::bind(i.second,i.first, row,
                                                   INSERT));
    }

    /* Invoke table triggers */
    content_ring->invoke_table_triggers(row, INSERT);
    
    return Result();
}

const Result
Content_DHT_entry::modify(Content_DHT_ptr& content_ring, Context& ctxt, 
                          const GUID_index_ring_map& new_sguids, 
                          const Row& row_) {
    /* Check the version */
    if (!(id == ctxt.current_row)) {
        return Result(Result::CONCURRENT_MODIFICATION, 
                      "Row modified since its retrieval.");
    }

    Trigger_map t(nonsticky_triggers.begin(), nonsticky_triggers.end());
    nonsticky_triggers.clear();

    /* Update the row content and wipe any index triggers. */
    const Reference prev_id = id;
    const Row prev_row = row;
    id.version++; 
    row = row_;
    ctxt.current_row = id;
    index_triggers.clear();

    GUID_update_function_map updates;

    //lg.dbg("Modifying row %s, %d, %d", prev_id.str().c_str(),
    //       sguids.size(), new_sguids.size());

    /* Compute the sets */
    BOOST_FOREACH(GUID_index_ring_map::value_type j, new_sguids) {
        const DHT_name& index_name = j.first;
        Index_DHT_ptr& index_ring = j.second.first;
        const GUID& sguid = j.second.second;

        if (!(sguids[index_name].second == sguid)) {
            /* New index entry */
            Trigger_function tfunc = 
                boost::bind(&Content_DHT::index_entry_deleted_callback, 
                            content_ring, content_ring, id, _1, _2);
            Index_updated_callback cb = 
                boost::bind(&Content_DHT::index_entry_updated_callback,
                            content_ring, id, index_name, sguid, row, _1);
            updates[index_name][sguid] = 
                boost::bind(&Index_DHT::put, index_ring,index_ring,
                            sguid, id, row, tfunc, cb, MODIFY);
            //lg.dbg("----> putting index %s for %s", sguid.str().c_str(),
            //       id.str().c_str());

            /* Remove the earlier index entry with different sguid */
            const GUID& prev_sguid = sguids[index_name].second;
            cb = boost::bind(&Content_DHT::index_entry_updated_callback,
                             content_ring, id, index_name, prev_sguid, prev_row, _1);
            updates[index_name][prev_sguid] = 
                boost::bind(&Index_DHT::remove, index_ring, index_ring,
                            prev_sguid, prev_id, prev_row, cb, MODIFY);
            //lg.dbg("----> removing index %s for %s", prev_sguid.str().c_str(),
            //      prev_id.str().c_str());
        } else {
            /* Modified index entry */
            Trigger_function tfunc = 
                boost::bind(&Content_DHT::index_entry_deleted_callback, 
                            content_ring, content_ring, id, _1, _2);
            Index_updated_callback cb = 
                boost::bind(&Content_DHT::index_entry_updated_callback,
                            content_ring, id, index_name, sguid, prev_row, _1);
            /* boost::bind supports max. 10 parameters by default,
               hence the std::vector usage. */
            std::vector<Reference> t; 
            t.push_back(prev_id);
            t.push_back(id);
            updates[index_name][sguid] = 
                boost::bind(&Index_DHT::modify, index_ring, index_ring, sguid,
                            t, prev_row, row, tfunc, cb, MODIFY);
            //lg.dbg("----> modifying index %s", sguid.str().c_str());
        }

        sguids.erase(index_name);
    }
    
    /* Replace the current index guid set. */
    sguids = new_sguids;

    /* Invoke the index_updates */
    BOOST_FOREACH(GUID_update_function_map::value_type i, updates) {
        BOOST_FOREACH(Table_GUID_update_function_map::value_type j, i.second) {
            content_ring->dispatcher->post(j.second);
        }
    }

    /* Invoke row triggers */
    BOOST_FOREACH(Trigger_map::value_type i, t) { 
        content_ring->dispatcher->post(boost::bind(i.second, i.first,
                                                   prev_row, MODIFY));
    }

    /* Invoke table triggers */
    content_ring->invoke_table_triggers(prev_row, MODIFY);
    
    return Result();
}

const Result
Content_DHT_entry::remove(Content_DHT_ptr& content_ring, 
                          const Reference& ref) {
    /* Check the version */
    if (!(id == ref)) return Result(Result::CONCURRENT_MODIFICATION,
                                    "Row modified since its retrieval.");
    const Reference prev_id = id;
    id.version++;

    ///* Gather all triggers to invoke */
    Trigger_map t(nonsticky_triggers.begin(), nonsticky_triggers.end());
    //t.insert(index_triggers.begin(), index_triggers.end());
    nonsticky_triggers.clear();
    index_triggers.clear();

    /* Invoke triggers in increasing GUID order. */
    BOOST_FOREACH(Trigger_map::value_type i, t) { 
        content_ring->dispatcher->post(boost::bind(i.second, i.first, row,
                                                   REMOVE));
    }

    GUID_update_function_map updates;

    /* Remove index entries */
    BOOST_FOREACH(GUID_index_ring_map::value_type j, sguids) {
        const DHT_name& index_name = j.first;        
        Index_DHT_ptr& index_ring = j.second.first;
        const GUID& sguid = j.second.second;

        Index_updated_callback cb = 
            boost::bind(&Content_DHT::index_entry_updated_callback,
                        content_ring, id, index_name, sguid, row, _1);
        updates[index_name][sguid] = 
            boost::bind(&Index_DHT::remove, index_ring, index_ring,
                        sguid, prev_id, row, cb, REMOVE);
    }

    /* Invoke the index_updates */
    BOOST_FOREACH(GUID_update_function_map::value_type i, updates) {
        BOOST_FOREACH(Table_GUID_update_function_map::value_type j, i.second) {
            content_ring->dispatcher->post(j.second);
        }
    }

    /* Invoke table triggers */
    content_ring->invoke_table_triggers(row, REMOVE);

    /* Remove the row content */
    row.clear();

    return Result();
}

void
Content_DHT_entry::index_entry_updated_callback(const Reference& ref,
                                                const Index_name& index_name,
                                                const GUID& sguid,
                                                const Row& row,
                                                const Trigger_definition& tdef)
{
    if (!(id == ref)) { return; }
    
    //const Trigger_map::size_type size_before = index_triggers.size();
    Trigger_id tid(index_name, Reference(0, sguid), 0);
    index_triggers[tid] = tdef.second;

    /* Confirm the trigger was inserted properly. */
    //    assert(size_before + 1 == index_triggers.size());
}

bool 
Content_DHT_entry::empty() {
    return nonsticky_triggers.empty() && index_triggers.empty() && row.empty();
}

DHT::DHT(const DHT_name& n, const container::Component* c) 
    : dispatcher(c), name(n) { }

DHT::~DHT() { }

Trigger_id 
DHT::trigger_instance(const Reference& ref, const int tid) {
    return Trigger_id(name, ref, tid);
}

Content_DHT::Content_DHT(const DHT_name& name_, const container::Component* c_) 
    : DHT(name_, c_), next_tid(0) { }

void
Content_DHT::get(Content_DHT_ptr& this_, Context& ctxt, const Reference& id, 
                 const bool exact_match, 
                 const Async_storage::Get_callback& cb) const {
    const_iterator j = exact_match ? find(id.guid) : lower_bound(id.guid);
    /* If nothing found DHT, the ring must be empty (if wildcard GUID
       reference given) or no specific entry found (if exact match
       GUID given). */
    if (j == end()) {
        if (exact_match) {
            dispatcher->post
                (boost::bind(cb, Result(Result::CONCURRENT_MODIFICATION, 
                                        "Can't find specified row"), 
                             Context(), Row()));
        } else {
            dispatcher->post
                (boost::bind(cb, Result(Result::NO_MORE_ROWS), 
                             Context(), Row()));
        }

        return;
    }

    j->second.get(this_, ctxt, id, exact_match, cb);
}

void
Content_DHT::get_next(Content_DHT_ptr& this_, Context& ctxt, 
                      const Async_storage::Get_callback& cb) const {
    /* Search for the next and wrap as necessary. */
    const_iterator i = upper_bound(ctxt.current_row.guid);
    if (i == end()) {
        i = lower_bound(GUID());
        if (i == end()) {
            dispatcher->post
                (boost::bind(cb, Result(Result::NO_MORE_ROWS, "End of rows."), 
                             ctxt, Row()));
            return;
        }
    }

    /* If the initial GUID is found again, the iteration is
       complete. */
    if (i->first == ctxt.initial_row.guid) {
        dispatcher->post
            (boost::bind(cb, Result(Result::NO_MORE_ROWS, "End of rows."), 
                         ctxt, Row()));
        return;
    }
    
    /* If the next GUID would be pass the initial GUID, while the
       previous wasn't, the iteration must be complete too. */
    if (ctxt.current_row.guid < ctxt.initial_row.guid && 
        !(i->first < ctxt.initial_row.guid)) {
        dispatcher->post
            (boost::bind(cb, Result(Result::NO_MORE_ROWS, "End of rows."), 
                         ctxt, Row()));
        return;
    }
    
    return i->second.get_next(this_, ctxt, cb);
}

void
Content_DHT::put(Content_DHT_ptr& this_, const GUID_index_ring_map& new_sguids, 
                 const Row& row, const Async_storage::Put_callback& cb) {
    Context ctxt(name);
    Row row_ = row;

    /* Generate a new GUID for a new row, if necessary */
    if (row_.find("GUID") == row_.end()) {
        row_["GUID"] = GUID::random();
    }

    try {
        ctxt.current_row = Reference(0, boost::get<GUID>(row_["GUID"]));
    }
    catch (const boost::bad_get& e) { 
        dispatcher->post
            (boost::bind(cb, Result(Result::INVALID_ROW_OR_QUERY,
                                    "GUID given with invalid type."), GUID()));
        return;
    }

    //Co_scoped_mutex l(&mutex);

    /* Update the content ring and invoke the triggers, if success. */
    iterator r = find(ctxt.current_row.guid);
    if (r == end()) {
        Content_DHT_entry e = Content_DHT_entry(ctxt.current_row, row_);
        const Result result = e.put(this_, ctxt, new_sguids, row_);
        (*this)[ctxt.current_row.guid] = e;
        dispatcher->post(boost::bind(cb, result, ctxt.current_row.guid));
    } else {
        dispatcher->post
            (boost::bind(cb, Result(Result::CONCURRENT_MODIFICATION, 
                                    "Row does exist."), GUID()));
    }
}

void
Content_DHT::modify(Content_DHT_ptr& this_, const Context& ctxt, 
                    const GUID_index_ring_map& new_sguids, 
                    const Row& row, const Async_storage::Modify_callback& cb) {
    //Co_scoped_mutex l(&mutex);

    /* Update the content ring and let the entry trigger any
       callbacks. */
    iterator r = find(ctxt.current_row.guid);
    if (r == end()) {
        dispatcher->post(boost::bind(cb,Result(Result::CONCURRENT_MODIFICATION,
                                               "Row removed since retrieval."),
                                     Context(ctxt)));
        return;
    }

    Context new_ctxt(ctxt);
    const Result result = r->second.modify(this_, new_ctxt, new_sguids, row);

    dispatcher->post(boost::bind(cb, result, new_ctxt));
}

void 
Content_DHT::remove(Content_DHT_ptr& this_, const Context& ctxt, 
                    const Async_storage::Remove_callback& cb) {
    internal_remove(this_, ctxt.current_row, cb);
}

void 
Content_DHT::internal_remove(Content_DHT_ptr& this_, 
                             const Reference& current_row,
                             const Async_storage::Remove_callback& cb) {
    //Co_scoped_mutex l(&mutex);

    iterator r = find(current_row.guid);
    if (r == end()) {
        dispatcher->post
            (boost::bind(cb, Result(Result::CONCURRENT_MODIFICATION, 
                                    "Row removed since retrieval.")));
        return;
    }

    const Result result = r->second.remove(this_, current_row);
    if (r->second.empty()) {
        erase(current_row.guid);
    } else {
        /* The entry removal isn't empty yet, can't wipe it. */
    }


    dispatcher->post(boost::bind(cb, result));
}

void
Content_DHT::put_trigger(DHT_ptr& this_, const Context& ctxt, 
                         const Trigger_function& f, 
                         const Async_storage::Put_trigger_callback& cb) {
    //Co_scoped_mutex l(&mutex);

    /* Update the content ring and let the entry trigger any
       callbacks. */
    iterator r = find(ctxt.current_row.guid);
    if (r == end()) {
        dispatcher->post
            (boost::bind(cb, Result(Result::CONCURRENT_MODIFICATION, 
                                    "Row removed since retrieval."), 
                         Trigger_id("", Reference(),-1)));
        return;
    }

    r->second.put_trigger(this_, ctxt.current_row, f, cb);
}

void
Content_DHT::put_trigger(const bool sticky, const Trigger_function& f, 
                         const Async_storage::Put_trigger_callback& cb) {
    //Co_scoped_mutex l(&mutex);

    Trigger_id tid(name, next_tid++);
    if (sticky) {
        sticky_table_triggers[tid] = f;
    } else {
        nonsticky_table_triggers[tid] = f;
    }

    dispatcher->post(boost::bind(cb, Result(), tid));
}

void 
Content_DHT::remove_trigger(DHT_ptr& this_, const Trigger_id& tid,
                            const Async_storage::Remove_callback& cb) {
    //Co_scoped_mutex l(&mutex);
    if (tid.for_table) {
        sticky_table_triggers.erase(tid);
        nonsticky_table_triggers.erase(tid);
        dispatcher->post(boost::bind(cb, Result()));
    } else {
        iterator r = find(tid.ref.guid);
        if (r == end()) {
            /* Trigger removed, a likely race condition between the
               trigger invocation and application. Ignore. */
            dispatcher->post
                (boost::bind(cb, Result(Result::CONCURRENT_MODIFICATION, "Row "
                                        "removed since trigger creation.")));
        } else {
            const Result result = r->second.remove_trigger(this_, tid);
            dispatcher->post(boost::bind(cb, result));
        }
    }
}

void
Content_DHT::invoke_table_triggers(const Row& row,
                                   const Trigger_reason reason) {
    /* Gather table triggers to invoke */
    BOOST_FOREACH(Trigger_map::value_type v, sticky_table_triggers) {
        dispatcher->post(boost::bind(v.second, v.first, row, reason));
    }
    
    BOOST_FOREACH(Trigger_map::value_type v,nonsticky_table_triggers) {
        dispatcher->post(boost::bind(v.second, v.first, row, reason));
    }

    nonsticky_table_triggers.clear();
}

void 
Content_DHT::index_entry_updated_callback(const Reference& ref, 
                                          const Index_name& index_name,
                                          const GUID& sguid, const Row& row,
                                          const Trigger_definition& tdef) {
    /* If there's no trigger, this was due to remove(). */
    if (tdef.second.empty()) return;

    //Co_scoped_mutex l(&mutex);

    iterator r = find(ref.guid);
    if (r == end()) {
        // XXX: unnecessary?
        /* Primary entry doesn't exist anymore and hence, the trigger
           should be invoked immediately as well. */
        //dispatcher->post(boost::bind(tdef.second, tdef.first, row, REMOVE));
        return;
    }

    r->second.index_entry_updated_callback(ref, index_name, sguid, row, tdef);
}

static void callback_1(Result) { /* Ignore the result */ }

void 
Content_DHT::index_entry_deleted_callback(Content_DHT_ptr& this_,
                                          const Reference& ref,
                                          const Trigger_id& tid,
                                          const Row& row) {
    /* Ignore the result of the operation; if nothing wasn't deleted,
       the trigger invocation was merely delayed on its way or
       something. */
    internal_remove(this_, ref, &callback_1);
}

Index_DHT::Index_DHT(const DHT_name& name_, const container::Component* c_) 
    : DHT(name_, c_) { }

void
Index_DHT::get(const Content_DHT_ptr& content_ring, Context& ctxt,
               const Reference& id, 
               const Async_storage::Get_callback& cb) const {
    //Co_scoped_mutex l(&mutex);

    /* If nothing found, it must be a miss. */
    const const_iterator i = find(id.guid);
    ctxt.index_row = id;

    if (i == end()) {
        dispatcher->post(boost::bind(cb, Result(Result::NO_MORE_ROWS, 
                                                "No more rows."), 
                                     ctxt, Row()));
        return;
    }

    /* Find the content row pointed by the index entry */
    const Index_DHT_entry& index_entry = i->second;
    index_entry.get(content_ring, ctxt, cb);
}

void
Index_DHT::get_next(const Content_DHT_ptr& content_ring, 
                    Context& ctxt, const Async_storage::Get_callback& cb) const{
    /* If no index entry found, the DHT must be momentarily in
       inconsistent state. Let the application resolve the
       conflict. */
    //Co_scoped_mutex l(&mutex);

    const_iterator i = find(ctxt.index_row.guid);
    if (i == end()) {
        dispatcher->post
            (boost::bind(cb, Result(Result::CONCURRENT_MODIFICATION, 
                                    "Index modified while iterating."), 
                         ctxt, Row()));
        return;
    }
    
    i->second.get_next(content_ring, ctxt, cb);
}

static void callback_2(const Trigger_definition&) { /* Ignore */ }

void 
Index_DHT::primary_deleted_callback(Index_DHT_ptr& this_, 
                                    const Trigger_id&, const Row&,
                                    const Row& row, const GUID& sguid, 
                                    const Reference& ref) {
    /* Ignore the result of the operation; if nothing wasn't deleted,
       the trigger invocation was merely delayed on its way or
       something. */
    remove(this_, sguid, ref, row, &callback_2, REMOVE);
}

void
Index_DHT::put(Index_DHT_ptr& this_, const GUID& sguid, 
               const Reference& ref, const Row& row,
               const Trigger_function& tfunc, const Index_updated_callback& cb,
               const Trigger_reason reason) {
    //Co_scoped_mutex l(&mutex);

    //lg.dbg("putting index %s, now %s", sguid.str().c_str(),
    //       ref.str().c_str());

    iterator r = find(sguid);
    if (r == end()) {
        Index_DHT_entry entry = Index_DHT_entry(Reference(0, sguid));
        entry.put(this_, ref, row, tfunc, cb, reason);
        (*this)[sguid] = entry;       
    } else {
        r->second.put(this_, ref, row, tfunc, cb, reason);
    }
}

void
Index_DHT::modify(Index_DHT_ptr& this_, const GUID& sguid, 
                  const std::vector<Reference>& t,
                  const Row& prev_row, const Row& new_row, 
                  const Trigger_function& tfunc, 
                  const Index_updated_callback& cb, 
                  const Trigger_reason reason) {
    //Co_scoped_mutex l(&mutex);
    const Reference& prev_ref = t[0];
    const Reference& ref = t[1];
    //lg.dbg("modifying index %s, prev %s, now %s", sguid.str().c_str(),
    //       prev_ref.str().c_str(), ref.str().c_str());

    iterator r = find(sguid);
    if (r == end()) {
        Index_DHT_entry entry = Index_DHT_entry(Reference(0, sguid));
        entry.put(this_, ref, new_row, tfunc, cb, reason);
        (*this)[sguid] = entry;
    } else {
        r->second.modify(this_, prev_ref, ref, prev_row, new_row, 
                         tfunc, cb, reason);
    }
}

void
Index_DHT::remove(Index_DHT_ptr& this_, const GUID& sguid, 
                  const Reference& ref, const Row& row, 
                  const Index_updated_callback& cb,
                  const Trigger_reason reason) {
    //Co_scoped_mutex l(&mutex);

    //lg.dbg("removing index %s, now %s", sguid.str().c_str(),
    //       ref.str().c_str());

    iterator r = find(sguid);
    if (r == end()) {
        /* Nothing to do. */
        dispatcher->post(boost::bind(cb, pair<Trigger_id, Trigger_function>()));
        return;
    }

    Index_DHT_entry& entry = r->second;
    entry.remove(this_, ref, row, reason);
    if (entry.empty()) {
        /* No more references left. */
        erase(r);
    } else {
        /* Entry has still content. Don't wipe it. */
    }
    
    dispatcher->post(boost::bind(cb, pair<Trigger_id, Trigger_function>()));
}

void
Index_DHT::put_trigger(DHT_ptr& this_, const Context& ctxt, 
                       const Trigger_function& f, 
                       const Async_storage::Put_trigger_callback& cb) {
    //Co_scoped_mutex l(&mutex);

    /* If necessary create a hash entry where to insert the
       trigger. */

    iterator i = find(ctxt.index_row.guid);
    if (i == end()) {
        (*this)[ctxt.index_row.guid] = Index_DHT_entry(ctxt.index_row);
        i = find(ctxt.index_row.guid);
    }
    
    i->second.put_trigger(this_, ctxt.index_row, f, cb);
}

void 
Index_DHT::remove_trigger(DHT_ptr& this_, const Trigger_id& tid,
                          const Async_storage::Remove_trigger_callback& cb) {
    //Co_scoped_mutex l(&mutex);

    iterator r = find(tid.ref.guid);
    if (r == end()) {
        /* Trigger removed, a likely race condition between the
           trigger invocation and application. Ignore. */
    } else {
        r->second.remove_trigger(this_, tid);
    }

    dispatcher->post(boost::bind(cb, Result()));
}

