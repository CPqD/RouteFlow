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
#include "storage.hh"

#include <set>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include "sha1.hh"
#include "vlog.hh"

using namespace std;
using namespace vigil;
using namespace vigil::applications::storage;

static Vlog_module lg("storage");

GUID::GUID() {
    ::memset(guid, 0, sizeof(guid));
}

GUID::GUID(uint8_t* guid_) {
    ::memcpy(guid, guid_, sizeof(guid));
}

GUID::GUID(const int64_t guid_) {
    ::memset(guid, 0, sizeof(guid));
    ::memcpy(guid, &guid_, sizeof(guid_));
}

int64_t 
GUID::get_int() const {
    int64_t v;
    ::memcpy(&v, guid, sizeof(v));
    return v;
}

GUID 
GUID::get_max() {
    uint8_t guid[GUID_SIZE];
    ::memset(guid, 0xFF, sizeof(guid));
    return GUID(guid);
}

bool 
GUID::operator==(const GUID& other) const {
    return ::memcmp(guid, other.guid, sizeof(guid)) == 0;
}

bool 
GUID::operator<(const GUID& other) const {                   
    return ::memcmp(guid, other.guid, sizeof(guid)) < 0;
}

GUID
GUID::random() {
    // XXX use the SQLite random number generator
    
    static const int n = sizeof(uint8_t[GUID_SIZE]) / sizeof(int) + 1;
    uint8_t rnd[n * sizeof(int)];
    for (int i = 0; i < n; ++i) {
        *((int*)&rnd[i * sizeof(int)]) = ::rand();
    }

    return GUID(rnd);
}

const string
GUID::str() const {
    string ret;
    for (int i = 0; i < GUID_SIZE; ++i) {
        char buf[3];
        ::snprintf(buf, sizeof(buf), "%02x", guid[i]);
        ret += string(buf);
    }
    return ret;
}

bool
Index::operator==(const Column_value_map& c) const { 
    if (c.size() != columns.size()) {
        return false;
    }

    BOOST_FOREACH(Column_name column, columns) {
        if (c.find(column) == c.end()) { 
            return false;
        }
    }

    return true;
}

Reference::Reference() : version(ANY_VERSION), guid(), wildcard(true) { }

Reference::Reference(const int version_, const GUID& guid_,const bool wildcard_)
    : version(version_), guid(guid_), wildcard(wildcard_) { }

bool 
Reference::operator==(const Reference& other) const {
    if (wildcard || other.wildcard) {
        return true;
    }

    return (version == ANY_VERSION || other.version == ANY_VERSION ||
            version == other.version) &&
        guid == other.guid;
}

bool 
Reference::operator<(const Reference& other) const {
    if (guid < other.guid) return true;
    return guid == other.guid && version < other.version;
}

const std::string Reference::str() const {
    return string("wildcard: ") + (wildcard ? "yes" : "no") + ", GUID: " + 
        guid.str() + ", version: " + boost::lexical_cast<string>(version);
}

Context::Context() { }

Context::Context(const Table_name& table_) 
    : table(table_) { }
    
Context::Context(const Context& c) 
    : table(c.table), index(c.index), index_row(c.index_row), 
      initial_row(c.initial_row), current_row(c.current_row) { }

Async_storage::~Async_storage() {
    
}

void 
Async_storage::getInstance(const container::Context* ctxt, 
                           Async_storage*& storage) {
    storage = dynamic_cast<Async_storage*>
        (ctxt->get_by_interface(container::Interface_description
                                (typeid(Async_storage).name())));
}

Result::Result(const Code c, const string& m) 
    : code(c), message(m) {
}

bool
Result::operator==(const Result& other) const {
    return code == other.code;
}

bool
Result::is_success() const {
    return code == SUCCESS;
}

Trigger_id::Trigger_id() 
    : for_table(false), tid(-1) {
}

Trigger_id::Trigger_id(const string& ring_, const int tid_) 
    : for_table(true), ring(ring_), tid(tid_) {
}


Trigger_id::Trigger_id(const string& ring_, const Reference& ref_, 
                       const int tid_) 
    : for_table(false), ring(ring_), ref(ref_), tid(tid_) {
}

Trigger_id::Trigger_id(const bool for_table_, const string& ring_, 
                       const Reference& ref_, const int tid_) 
    : for_table(for_table_), ring(ring_), ref(ref_), tid(tid_) {
}

bool 
Trigger_id::operator==(const Trigger_id& other) const {
    return ring == other.ring && for_table == other.for_table && 
        ref == other.ref && tid == other.tid;
}

bool 
Trigger_id::operator<(const Trigger_id& other) const {
    if (ring < other.ring) return true;
    if (ring > other.ring) return false;

    if (ref < other.ref) return true;
    if (!(ref == other.ref)) return false;

    return tid < other.tid;
}

void Column_type_collector::operator()(const int64_t& i) const { 
    sql_type = "INTEGER"; 
    type = COLUMN_INT; 
    storage_type = (int64_t)0;
}

void Column_type_collector::operator()(const string& s) const { 
    sql_type = "TEXT";
    type = COLUMN_TEXT; 
    storage_type = string("");
}

void Column_type_collector::operator()(const double& d) const { 
    sql_type = "DOUBLE"; 
    type = COLUMN_DOUBLE; 
    storage_type = (double)0;
}

void Column_type_collector::operator()(const GUID& guid) const { 
    sql_type = "INTEGER";
    type = COLUMN_GUID; 
    storage_type = GUID();
}

class SHA1_digest
    : public boost::static_visitor<> 
{
public:
    void operator()(const int64_t& i) const {sha.input((uint8_t*)&i,sizeof(i));}
    void operator()(const string& s) const { sha.input((uint8_t*)s.c_str(), 
                                                       s.length()); }
    void operator()(const double& d) const { sha.input((uint8_t*)&d,sizeof(d));}
    void operator()(const GUID& g) const { sha.input(g.guid, sizeof(g.guid)); }

    GUID digest() const {
        GUID guid;
        sha.digest(guid.guid);
        return guid;
    }

private:
    mutable SHA1 sha;
};

namespace vigil {
namespace applications {
namespace storage {

GUID compute_sguid(const Column_list& columns, 
                   const Column_value_map& values) {
    SHA1_digest digest;
    set<Column_name> sorted;

    BOOST_FOREACH(Column_name v, columns) { sorted.insert(v); }
    BOOST_FOREACH(Column_name v, sorted) {
        Column_value_map::const_iterator r = values.find(v);
        boost::apply_visitor(digest, r->second);
    }
    
    return digest.digest();
}

bool is_equal(const Column_definition_map& c1, const Index_list& i1,
              const Column_definition_map& c2, const Index_list& i2) {
    if (c1.size() != c2.size() || i1.size() != i2.size()) {
        return false;
    }

    BOOST_FOREACH(Column_definition_map::value_type v, c1) {
        if (c2.find(v.first) == c2.end() ||
            !(c2.find(v.first)->second == v.second)) {
            return false;
        }
    }

    typedef hash_map<Index_name, Column_list> Index_map;
    Index_map h1;
    Index_map h2;

    BOOST_FOREACH(Index v, i1) { h1[v.name] = v.columns; }
    BOOST_FOREACH(Index v, i2) { h2[v.name] = v.columns; }

    if (h1.size() != h2.size()) {
        return false;
    }

    BOOST_FOREACH(Index_map::value_type v, h1) {
        if (h2.find(v.first) == h2.end() ||
            !(h2.find(v.first)->second == v.second)) {
            return false;
        }
    }

    return true;
}

}
}
}

using namespace vigil;
using namespace vigil::container;

namespace {
    struct Null_component
        : public Component {
        Null_component(const container::Context* c, const json_object*)
            : Component(c) { };
        void configure(const Configuration*) { }
        void install() { }
    };
}

REGISTER_COMPONENT(Simple_component_factory<Null_component>, Null_component);
