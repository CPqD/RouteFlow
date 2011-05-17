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
#include <inttypes.h>
#include <list>
#include <sys/time.h>
#include "hash_map.hh"

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include "component.hh"
#include "vlog.hh"
#include "bindings_storage/bindings_storage.hh" 
#include "threads/cooperative.hh"
#include "storage/storage.hh" 
#include "storage/storage_util.hh" 

using namespace std;
using namespace vigil;
using namespace vigil::container;
using namespace vigil::applications;
using namespace boost;


namespace  {

static Vlog_module lg("bs memleak test");

class BSMemLeakTest
    : public Component
{
public:

    BSMemLeakTest(const vigil::container::Context* c,
                const json_object* xml) 
        : Component(c), b_store(NULL), trigger_count(0), counter(0)   {}

    ~BSMemLeakTest() { } 

    void configure(const Configuration*) {
      resolve(np_store); 
      assert(np_store); 
      
      resolve(b_store); 
      assert(b_store); 
    }

    void install() {
            test_sem = new Co_sema(); 
            timeval tv = { 2, 0 }; 
            post(boost::bind(&BSMemLeakTest::set_triggers, this),tv);
    }

    void waitForX(int x) {
      for(int i = 0; i < x; ++i) 
        test_sem->down(); 
    } 

    void do_test();

private:
    Bindings_Storage *b_store;
    int trigger_count;  
    Async_storage *np_store; 

    void set_triggers(); 
    void trigger_fired(const storage::Trigger_id & tid, const storage::Row &r); 
    void trigger_inserted(const storage::Result &result,const storage::Trigger_id &tid);
    void print_names(const NameList &name_list);
    void print_entities(const EntityList &entity_list);
    void remove_trigger_callback(const storage::Result &result); 
    void clear_cb();

    int64_t counter; 
    Co_thread thread;
    Co_sema *test_sem;
    storage::Trigger_id tid1,tid2; // used to cancel triggers
};


void BSMemLeakTest::print_names(const NameList &name_list) {
/*  if(name_list.size() == 0) 
    printf("[ Empty Name List ] \n"); 

  NameList::const_iterator it = name_list.begin(); 
  for( ; it != name_list.end(); it++) 
    printf("name = %s  type = %d \n", it->first.c_str(), it->second); 
  */ 
    //printf("in get_names \n"); 
    test_sem->up();   
} 

void BSMemLeakTest::print_entities(const EntityList &entity_list) {
/*
  EntityList::const_iterator it = entity_list.begin(); 
  for( ; it != entity_list.end(); it++) {  
    printf("dpid = %lld port = %d dladdr = %lld nwaddr = %d \n", 
        it->dpid.as_host(),it->port,it->dladdr.hb_long(),it->nwaddr); 
  }
  */
  test_sem->up();
} 

void BSMemLeakTest::do_test() {
    printf("do_test() \n"); 
    
    int start = counter;
    counter += 1000; 
    printf("counter = %"PRId64" \n", counter); 

    for(int i = start; i < counter; ++i) {
      //printf("inserting row\n"); 
      b_store->store_binding_state(datapathid::from_host(i),i,i,i,
        "javelina",Name::HOST,false,0); 
      waitForX(2); 
    } 
    
    for(int i = start; i < counter; ++i) {
      //printf("getting names \n"); 
      b_store->get_names_by_ap(datapathid::from_host(i), i,  
        boost::bind(&BSMemLeakTest::print_names, this, _1)); 
      waitForX(1);  
    } 
   
    b_store->clear(boost::bind(&BSMemLeakTest::clear_cb,this)); 
}

void BSMemLeakTest::clear_cb() { 
  delete test_sem; 
  test_sem = new Co_sema(); 
  printf("Cleared binding store.  sleeping.... \n"); 
  timeval tv = { 2, 0 }; 
  post(boost::bind(&BSMemLeakTest::do_test, this),tv);
} 

void BSMemLeakTest::trigger_fired(const storage::Trigger_id & tid,
                                        const storage::Row& r) { 
  //printf("trigger fired \n");   
  test_sem->up();  // we do a sem->up() per row changed
} 

// insert trigger for 'bindings_id' and bindings_name' table
void BSMemLeakTest::set_triggers() {
   
      np_store->put_trigger(Bindings_Storage::ID_TABLE_NAME,true, 
         boost::bind(&BSMemLeakTest::trigger_fired, this, _1, _2),
         boost::bind(&BSMemLeakTest::trigger_inserted, this, _1, _2) ); 
          
      np_store->put_trigger(Bindings_Storage::NAME_TABLE_NAME,true, 
         boost::bind(&BSMemLeakTest::trigger_fired, this, _1, _2),
         boost::bind(&BSMemLeakTest::trigger_inserted, this, _1, _2) ); 
} 

// callback indicating that trigger insertion is finished
void BSMemLeakTest::trigger_inserted(const storage::Result &result,
                                           const storage::Trigger_id &tid) {
  
  if(result.code != storage::Result::SUCCESS) { 
    lg.err("inserting table trigger failed: %s \n", result.message.c_str());
    return; 
  } 

  ++trigger_count; 
  if(trigger_count == 1) {
    tid1 = tid; 
  }else if(trigger_count == 2) { 
    tid2 = tid; 
    do_test();
  }
} 
    


}  //unamed namespace 

REGISTER_COMPONENT(container::Simple_component_factory<BSMemLeakTest>, 
                   BSMemLeakTest);

