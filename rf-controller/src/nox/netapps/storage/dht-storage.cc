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
#include "dht-storage.hh"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/get.hpp>
#include <boost/variant/static_visitor.hpp>

#include "hash_set.hh"
#include "sha1.hh"
#include "vlog.hh"

using namespace std;
using namespace vigil;
using namespace vigil::applications::storage;
using namespace vigil::container;

static Vlog_module lg("dht-storage");

/* TODO:
 *
 * 1) The class does not use locking for now, since a) it's not
 *    strictly speaking required in this single-node implementation of
 *    the storage, and b) on NOX, Python is invoked while in a
 *    critical section and therefore (the invoked) Python applications
 *    can't call back to the C++ storage implementation -- as they do
 *    if they use any storage functionality -- if it uses
 *    locking. This should be refactored.
 *
 */

#define RETURN_ERROR(CODE, MESSAGE)                                     \
    do {                                                                \
        post(boost::bind(cb, Result(Result::CODE, #MESSAGE)));          \
        return;                                                         \
    } while (0);

Async_DHT_storage::Async_DHT_storage(const container::Context* c,
                                     const json_object*)
    : Component(c) {

}

Index_list
Async_DHT_storage::to_list(const Async_DHT_storage::Index_map& m) {
    Index_list l;
    BOOST_FOREACH(Index_map::value_type v, m) { l.push_back(v.second); }
    return l;
}

void
Async_DHT_storage::create_table(const Table_name& table,
                                const Column_definition_map& columns_,
                                const Index_list& indices_,
                                const Create_table_callback& cb) {
    //Co_scoped_mutex l(&mutex);

    Column_definition_map columns = columns_;
    columns["GUID"] = GUID();

    // Index names should have table name as a prefix
    Index_list indices = indices_;
    BOOST_FOREACH(Index_list::value_type& i, indices) {
        i.name = table + "_" + i.name;
    }

    if (tables.find(table) != tables.end()) {
        if (is_equal(columns, indices,
                     tables[table].first, to_list(tables[table].second))) {
            lg.dbg("No need to create a table: %s", table.c_str());
            post(boost::bind(cb, Result()));
        } else {
            post(boost::bind(cb, Result(Result::EXISTING_TABLE,
                                        "Table already exists with a different "
                                        "schema.")));
        }

        return;
    }

    lg.dbg("Creating a table: %s", table.c_str());

    /* Check for duplicate index entries */
    hash_set<Index_name> dupecheck;
    BOOST_FOREACH(Index_list::value_type& i,indices) {
        dupecheck.insert(i.name);
    }
    if (dupecheck.size() != indices.size()) {
        post(boost::bind(cb, Result(Result::INVALID_ROW_OR_QUERY,
                                    "duplicate indices defined.")));
        return;
    }

    /* Create the index rings */
    Index_map m;

    BOOST_FOREACH(Index_list::value_type i, indices) {
            Index v = i;
            m[v.name] = v;
            index_dhts[v.name] = Index_DHT_ptr(new Index_DHT(v.name, this));
    }

    /* Create the table ring */
    content_dhts[table] = Content_DHT_ptr(new Content_DHT(table, this));
    tables[table] = make_pair(columns, m);

    cb(Result());
}

void
Async_DHT_storage::drop_table(const Table_name& table,
                              const Drop_table_callback& cb) {
    //Co_scoped_mutex l(&mutex);

    if (tables.find(table) == tables.end()) {
        post(boost::bind(cb, Result(Result::NONEXISTING_TABLE, table +
                                    " does not exist.")));
        return;
    }

    lg.dbg("Dropping a table %s.", table.c_str());
        
    //lg.dbg("COMMITing the table drop.");

    /* Remove the main table */
    content_dhts.erase(table);

    /* Remove the indices */
    BOOST_FOREACH(Index_map::value_type i, tables[table].second) {
            index_dhts.erase(i.first);
    }

    tables.erase(table);

    cb(Result());
}

class type_checker
    : public boost::static_visitor<bool> {
public:
    template <typename T, typename U>
    bool operator()(const T&, const U&) const { return false; }

    template <typename T>
    bool operator()(const T&, const T&) const { return true; }
};

bool
Async_DHT_storage::validate_column_types(const Table_name& table,
                                         const Column_value_map& c,
                                         const Result::Code& err_code,
                                         const boost::function
                                         <void(const Result&)>& cb) const {
    Table_definition_map::const_iterator t = tables.find(table);
    if (t == tables.end()) {
        post(boost::bind
             (cb, Result(err_code, "table '" + table + "' doesn't exist.")));
        return false;
    }

    const Table_definition& tdef = t->second;
    const Column_definition_map& cdef = tdef.first;

    BOOST_FOREACH(Column_value_map::value_type t, c) {
        const Column_name& cn = t.first;
        const Column_value cv = t.second;
        Column_definition_map::const_iterator j = cdef.find(cn);
        if (j == cdef.end() ||
            !boost::apply_visitor(type_checker(), cv, j->second)) {
            post(boost::bind(cb, Result(err_code, "column '" + cn + "' "
                                        "doesn't exist or has invalid type ")));
            return false;
        }
    }

    return true;
}

void
Async_DHT_storage::get(const Table_name& table,
                       const Query& query,
                       const Get_callback& cb) {
    //Co_scoped_mutex l(&mutex);
    Context ctxt(table);

    if (!validate_column_types(table, query, Result::INVALID_ROW_OR_QUERY,
                               boost::bind(cb, _1, Context(), Row()))) {
        return;
    }

    Content_DHT_ptr& content_ring = content_dhts[table];

    if (query.empty()) {
        post(boost::bind(&Content_DHT::get, content_ring, content_ring,
                         ctxt, Reference(), false, cb));

    } else {
        Query::const_iterator q = query.find("GUID");
        if (q == query.end()) {
            try {
                const Index index = identify_index(table, query);
                ctxt.index = index.name;

                const Index_DHT_ptr& index_ring = index_dhts[index.name];
                post(boost::bind(&Index_DHT::get, index_ring, content_ring,
                                 ctxt, Reference(Reference::ANY_VERSION,
                                                 compute_sguid(index.columns,
                                                               query)), cb));
            } catch (const exception&) {
                post(boost::bind(cb, Result(Result::INVALID_ROW_OR_QUERY,
                                            "no matching index"),
                                 Context(), Row()));
            }

        } else {
            /* Find the content row directly w/o the use of index. */
            post(boost::bind(&Content_DHT::get, content_ring, content_ring,
                             ctxt, Reference(Reference::ANY_VERSION,
                                             boost::get<GUID>(q->second)),
                             false, cb));
        }
    }
}

void
Async_DHT_storage::get_next(const Context& ctxt,
                            const Get_callback& cb) {
    Context new_ctxt(ctxt);
    Index_DHT_ptr index_ring;
    Content_DHT_ptr content_ring;
    {
        //Co_scoped_mutex l(&mutex);

        if (content_dhts.find(new_ctxt.table) == content_dhts.end()) {
            post(boost::bind(cb, Result(Result::CONCURRENT_MODIFICATION,
                                        "Table removed while iterating."),
                             ctxt, Row()));
            return;
        }

        content_ring = content_dhts[new_ctxt.table];

        if (new_ctxt.index != "") {
            /* The original query used a secondary index */
            if (index_dhts.find(new_ctxt.index) == index_dhts.end()) {
                post(boost::bind(cb, Result(Result::CONCURRENT_MODIFICATION,
                                            "Table removed while iterating."),
                                 ctxt, Row()));
                return;
            }

            index_ring = index_dhts[new_ctxt.index];
        }
    }

    if (index_ring) {
        /* The original query used a secondary index */
        index_ring->get_next(content_ring, new_ctxt, cb);
    } else {
        /* Original query did not use a secondary index. Search for
           the next, and wrap if necessary. */
        content_ring->get_next(content_ring, new_ctxt, cb);
    }
}

void
Async_DHT_storage::put(const Table_name& table,
                       const Row& row,
                       const Async_storage::Put_callback& cb) {
    Context ctxt(table);
    Content_DHT_ptr content_ring;
    GUID_index_ring_map sguids;
    {
        //Co_scoped_mutex l(&mutex);
        if (!validate_column_types(table, row, Result::INVALID_ROW_OR_QUERY,
                                   boost::bind(cb, _1, GUID()))) {
            return;
        }

        /* Pre-compute the index GUIDs per the index for the content
           ring. */
        Index_map& indices = tables[table].second;
        BOOST_FOREACH(Index_map::value_type v, indices) {
            sguids[v.second.name] =
                make_pair(index_dhts[v.second.name],
                          compute_sguid(v.second.columns,row));
        }

        content_ring = content_dhts[table];
    }

    // TODO: replication

    content_ring->put(content_ring, sguids, row, cb);
}

void
Async_DHT_storage::modify(const Context& ctxt,
                          const Row& row,
                          const Async_storage::Modify_callback& cb) {
    Content_DHT_ptr content_ring;
    GUID_index_ring_map sguids;
    {
        //Co_scoped_mutex l(&mutex);
        if (!validate_column_types(ctxt.table, row,
                                   Result::CONCURRENT_MODIFICATION,
                                   boost::bind(cb,_1,Context()))) { return; }

        /* Pre-compute the index GUIDs per the index for the content
           ring. */
        Index_map& indices = tables[ctxt.table].second;
        BOOST_FOREACH(Index_map::value_type v, indices) {
            sguids[v.first] = make_pair(index_dhts[v.first],
                                        compute_sguid(v.second.columns, row));

        }

        content_ring = content_dhts[ctxt.table];
    }

    /* Validate the row and the context match */
    if (row.find("GUID") != row.end() &&
        !(boost::get<GUID>(row.find("GUID")->second) == ctxt.current_row.guid)){
        post(boost::bind(cb, Result(Result::INVALID_ROW_OR_QUERY,
                                    "Row and context don't match."),
                         Context()));
        return;
    }

    // TODO: replication

    content_ring->modify(content_ring, ctxt, sguids, row, cb);
}

void
Async_DHT_storage::remove(const Context& ctxt,
                          const Async_storage::Remove_callback& cb) {
    Content_DHT_ptr content_ring;
    {
        //Co_scoped_mutex l(&mutex);

        if (tables.find(ctxt.table) == tables.end()) {
            RETURN_ERROR(CONCURRENT_MODIFICATION, table + " removed.");
        }

        content_ring = content_dhts[ctxt.table];
    }

    // TODO: replication

    content_ring->remove(content_ring, ctxt, cb);
}

void
Async_DHT_storage::debug(const Table_name& table) {
    lg.dbg("statistics for %s:", table.c_str());
    lg.dbg("main table, entries = %zu", content_dhts[table]->size());
    BOOST_FOREACH(const Index_map::value_type& i, tables[table].second) {
        Index_name name = i.first;
        lg.dbg("index table, entries = %zu", index_dhts[name]->size());
    }
}

#define RETURN_TRIGGER_ERROR(CODE, MESSAGE)                             \
    do {                                                                \
        post(boost::bind(cb, Result(Result::CODE, #MESSAGE),            \
                         Trigger_id("", Reference(),-1)));              \
        return;                                                         \
    } while (0);

void
Async_DHT_storage::put_trigger(const Context& ctxt, const Trigger_function& f,
                               const Async_storage::Put_trigger_callback& cb) {
    boost::shared_ptr<DHT> ring;
    {
        //Co_scoped_mutex l(&mutex);

        if (tables.find(ctxt.table) == tables.end()) {
            RETURN_TRIGGER_ERROR(CONCURRENT_MODIFICATION, "Table dropped.");
        }

        if (ctxt.index == "") {
            ring = content_dhts[ctxt.table];
        } else {
            Index_map& indices = tables[ctxt.table].second;
            Index_map::const_iterator i = indices.find(ctxt.index);
            if (i == indices.end()) {
                RETURN_TRIGGER_ERROR(CONCURRENT_MODIFICATION, "Index dropped.");
            }

            ring = index_dhts[i->first];
        }
    }

    ring->put_trigger(ring, ctxt, f, cb);
}

void
Async_DHT_storage::put_trigger(const Table_name& table, const bool sticky,
                               const Trigger_function& f,
                               const Async_storage::Put_trigger_callback& cb) {
    //Co_scoped_mutex l(&mutex);

    if (tables.find(table) == tables.end()) {
        RETURN_TRIGGER_ERROR(CONCURRENT_MODIFICATION, "Table dropped.");
    }

    content_dhts[table]->put_trigger(sticky, f, cb);
}

void
Async_DHT_storage::remove_trigger(const Trigger_id& tid,
                                  const Async_storage::Remove_callback& cb) {
    boost::shared_ptr<DHT> ring;
    {
        //Co_scoped_mutex l(&mutex);

        hash_map<DHT_name, Index_DHT_ptr>::iterator i =
            index_dhts.find(tid.ring);
        if (i != index_dhts.end()) {
            ring = i->second;
        } else {
            hash_map<DHT_name, Content_DHT_ptr>::iterator j =
                content_dhts.find(tid.ring);
            if (j != content_dhts.end()) {
                ring = j->second;
            } else {
                post(boost::bind(cb, Result(Result::CONCURRENT_MODIFICATION,
                                            "Table removed.")));
                return;
            }
        }
    }

    ring->remove_trigger(ring, tid, cb);
}

void
Async_DHT_storage::configure(const Configuration*) {

}

void
Async_DHT_storage::install() {
}

vigil::applications::storage::Index
Async_DHT_storage::identify_index(const Table_name& table,
                                  const Query& q) const {
    Table_definition_map::const_iterator t = tables.find(table);
    const Index_map& indices = t->second.second;

    BOOST_FOREACH(Index_map::value_type v, indices) {
        if (v.second == q) { return v.second; }
    }

    throw invalid_argument("cannot find the index");
}

REGISTER_COMPONENT(vigil::container::Simple_component_factory<Async_DHT_storage>,
                   Async_storage);
