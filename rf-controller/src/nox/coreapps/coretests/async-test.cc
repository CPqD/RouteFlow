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


#include "async-test.hh"

#include <boost/bind.hpp>
#include <errno.h>
#include <string>
#include "vlog.hh"

#include "testharness/testdefs.hh"

#include "threads/cooperative.hh"

#include "bootstrap-complete.hh"


using namespace vigil;
using namespace std;


Vlog_module lg("async-test");

class AsyncTestCase
{
    public:
        AsyncTestCase(){
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
    NOX_TEST_ASSERT(1, "Async event complete");
    ::exit(0);
}

void 
AsyncTestCase::run(){
    lg.dbg("doing asyncronous testing work");
    // Do some testing and then fire the semaphore when finished 
    sem->up();
}


void 
AsyncTest::configure(const Configuration*)
{
    register_handler<Bootstrap_complete_event>
        (boost::bind(&AsyncTest::handle_bootstrap, this,
                     _1));

}

void 
AsyncTest::install()
{
}

Disposition
AsyncTest::handle_bootstrap(const Event& e)
{
    AsyncTestCase* t = new AsyncTestCase();
    t->run_test();
    delete t;
    return CONTINUE;
}
