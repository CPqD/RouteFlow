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
/* Template for creating an asyncronous test using the testing
 * framework.  The basic idea is to create a cooperative thread for the
 * test and have it signal a semaphore when finished.
 */
#include "tests.hh"

#include <iostream>
#include <boost/bind.hpp>

#include "vlog.hh"
#include "threads/cooperative.hh"

using namespace std;
using namespace vigil;
using namespace vigil::container;
using namespace vigil::testing;

namespace {

static Vlog_module lg("async-test");

class AsyncTestCase
    : public Test_component
{
    public:
        AsyncTestCase(const Context* c,
                const json_object* xml) 
            : Test_component(c) {
            }

        void configure(const Configuration*) {
        }

        void install() {
            sem = new Co_sema(); 
        }

        void run_test();
        void run();

    private:    

        Co_thread thread;
        Co_sema* sem;
};

void
AsyncTestCase::run_test() {
    thread.start(boost::bind(&AsyncTestCase::run, this));
    lg.dbg("waiting for the test to finish");
    // Wait for the test to finish
    sem->down();
    lg.dbg("testing complete");
}

void 
AsyncTestCase::run(){
    lg.dbg("doig asyncronous testing work");
    // Do some testing and then fire the semaphore when finished 
    sem->up();
}


} // unnamed namespace

BOOST_AUTO_COMPONENT_TEST_CASE(AsyncTest, AsyncTestCase);
