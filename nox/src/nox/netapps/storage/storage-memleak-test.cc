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
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include "component.hh"
#include "storage/storage-blocking.hh"
#include "vlog.hh"
#include "malloc.h" 

using namespace std;
using namespace vigil;
using namespace vigil::container;

namespace {

static Vlog_module lg("storage-test");

/* A very rudimentary storage test. */
class Storage_test
    : public Component
{
public:

    Storage_test(const Context* c,
                      const json_object* xml) 
        : Component(c), table("FOO"), counter(0) {
    }

    void configure(const Configuration*) {
        resolve(storage_);
        storage = new applications::storage::Sync_storage(storage_);
    }

    void install() {
        printf("hello from storage test\n"); 
        // Start the component. For example, if any threads require
        // starting, do it now.
       timeval tv = { 1, 0} ; 
       post(boost::bind(&Storage_test::init_table,this), tv); 
    }

    void run_test();
    void init_table(); 

private:
    string table;
    int64_t counter; 
    applications::storage::Async_storage* storage_;
    applications::storage::Sync_storage* storage;
};

void Storage_test::init_table() { 
    printf("initializing storage test\n"); 


    applications::storage::Column_definition_map c;
    c["COL_1"] = "";
    c["COL_2"] = (int64_t)0;
    applications::storage::Index_list i;
    applications::storage::Index index_1;
    index_1.name = "INDEX_1";
    index_1.columns.push_back("COL_1");
    i.push_back(index_1);

    applications::storage::Index index_2;
    index_2.name = "INDEX_2";
    index_2.columns.push_back("COL_2");
    i.push_back(index_2);
    
    storage->create_table(table, c, i);

    run_test(); 
} 

void
Storage_test::run_test() {
  
    printf("do_test()  counter = %"PRId64"\n", counter); 
    struct mallinfo m = mallinfo();
    printf("**** mallinfo:  KB used = %d  *****\n", m.uordblks / 1000 ); 

    int start = counter; 
    counter += 10000;

    for (int i = start; i < counter; i++) {
        applications::storage::Row r;
        r["COL_1"] = boost::lexical_cast<string>(start);
        r["COL_2"] = (int64_t)i;
        storage->put(table, r);
    }

    for (int j = /*start*/ 0; j < counter; j++) {
            applications::storage::Query q;
            q["COL_2"] = (int64_t)j;       
            storage->get(table, q);
    }
        
    // delete everything
    for (int i = start; i < counter; i++) {
            applications::storage::Query q;
            q["COL_2"] = (int64_t)i;       
            applications::storage::Sync_storage::Get_result r = 
                storage->get(table, q);
            applications::storage::Context c = r.get<1>(); 
            storage->remove(c); 
    }

  
    printf("going to sleep.  counter = %lld \n", counter); 
    timeval tv = { 1, 0} ; 
    post(boost::bind(&Storage_test::run_test,this), tv); 
}

} // unnamed namespace

REGISTER_COMPONENT(container::Simple_component_factory<Storage_test>, 
                   Storage_test);

