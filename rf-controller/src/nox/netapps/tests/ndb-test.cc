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
#include <ctime>
#include <list>
#include <sys/time.h>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include "component.hh"
#include "ndb/ndb.hh"
#include "tests.hh"
#include "vlog.hh"

using namespace std;
using namespace vigil;
using namespace vigil::container;
using namespace vigil::testing;

namespace {

static Vlog_module lg("ndb-test");

/* A very rudimentary network database performance test. */
class NDBTestCase
    : public Test_component
{
public:

    NDBTestCase(const Context* c,
                const json_object* xml) 
        : Test_component(c) {
    }

    void configure(const Configuration*) {
        resolve(ndb);
    }

    void install() {
        // Start the component. For example, if any threads require
        // starting, do it now.
    }

    void invalid();

    void run_test();

private:
    NDB* ndb;
};


void
NDBTestCase::run_test() {
    const string table = "table1";
    
    NDB::ColumnDef_List columns;
    NDB::IndexDef_List indices;
    columns.push_back(make_pair<string, Op::ValueType>("foo", Op::INT));
    columns.push_back(make_pair<string, Op::ValueType>("bar", Op::TEXT));

    ndb->create_table(table, columns, indices, 0);

    clock_t start = clock();
    
    NDB::OpStatus status;
    for (int p = 0; p < 10000; p++) {
        list<boost::shared_ptr<PutOp> > l;
        list<boost::shared_ptr<GetOp> > dep;
        boost::shared_ptr<list<boost::shared_ptr<Op::KeyValue> > >
            q(new list<boost::shared_ptr<Op::KeyValue> >);
        boost::shared_ptr<Op::KeyValue> kv1(new Op::KeyValue("foo", (int64_t)1));
        boost::shared_ptr<Op::KeyValue> kv2(new Op::KeyValue("bar", "bar"));
        q->push_back(kv1);
        q->push_back(kv2);
        boost::shared_ptr<PutOp> p(new PutOp(table, q));
        l.push_back(p);

        status = ndb->execute(l, dep, 0);
        BOOST_REQUIRE(status == NDB::OK);
    }

    clock_t ends = clock();

    ndb->drop_table(table);

    lg.dbg("Seconds %f", ((double)(ends - start)) / CLOCKS_PER_SEC);
}

void 
NDBTestCase::invalid() {
    lg.dbg("entry invalidated.");
}
} // unnamed namespace

BOOST_AUTO_COMPONENT_TEST_CASE(NDBTest, NDBTestCase);
