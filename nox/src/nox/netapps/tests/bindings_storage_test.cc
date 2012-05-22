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
#include "tests.hh"
#include "bindings_storage/bindings_storage.hh" 
#include "threads/cooperative.hh"
#include "storage/storage.hh" 
#include "storage/storage_util.hh" 

using namespace std;
using namespace vigil;
using namespace vigil::container;
using namespace vigil::applications;
using namespace boost;
using namespace vigil::testing;


namespace  {

static Vlog_module lg("bindings-storage-test");

class BindingsStorageTestCase
    : public Test_component
{
public:

    BindingsStorageTestCase(const vigil::container::Context* c,
                const json_object* xml) 
        : Test_component(c), b_store(NULL), trigger_count(0),debug(false)  {}

    ~BindingsStorageTestCase() { } 

    void configure(const Configuration*) {
      resolve(np_store); 
      assert(np_store); 
      
      resolve(b_store); 
      assert(b_store); 
    }

    void install() {
            main_sem = new Co_sema(); 
            test_sem = new Co_sema(); 
    }

    void waitForX(int x) {
      for(int i = 0; i < x; ++i) 
        test_sem->down(); 
    } 

    void run_test();
    void do_test();

private:
    Bindings_Storage *b_store;
    int trigger_count;  
    Async_storage *np_store; 
    NameList expected_names;
    EntityList expected_entities;

    void set_triggers(); 
    void trigger_fired(const storage::Trigger_id & tid, const storage::Row &r); 
    void trigger_inserted(const storage::Result &result,const storage::Trigger_id &tid);
    void verify_expected_names(const NameList &name_list);
    void print_entities(const EntityList &entity_list);
    void verify_expected_entities(const EntityList &entity_list);
    void remove_trigger_callback(const storage::Result &result); 
    void clear_cb(); 

    bool debug; 
    Co_thread thread;
    Co_sema *main_sem, *test_sem;
    storage::Trigger_id tid1,tid2; // used to cancel triggers
};

void BindingsStorageTestCase::run_test() {
  if(debug) printf("starting bindings storage test case\n"); 
  b_store->clear(boost::bind(&BindingsStorageTestCase::clear_cb, this)); 
} 

void BindingsStorageTestCase::clear_cb() { 
  if(debug) printf("in clear_cb\n"); 
  thread.start(boost::bind(&BindingsStorageTestCase::set_triggers, this));
  main_sem->down();
} 

// used as a callback to name lookups to test if returned names are equal
// to those that were expected 
void BindingsStorageTestCase::verify_expected_names(const NameList &name_list){
 
  if(debug) { 
    printf("expected names: \n"); 
    Bindings_Storage::print_names(expected_names); 
    printf("got names: \n"); 
    Bindings_Storage::print_names(name_list);
  } 
  BOOST_REQUIRE(name_list.size() == expected_names.size());  

  NameList::const_iterator it = name_list.begin(); 
  for( ; it != name_list.end(); it++) {  
    BOOST_REQUIRE(find(expected_names.begin(), expected_names.end(), *it) 
                  != expected_names.end()); 
  }

  test_sem->up();
} 

void BindingsStorageTestCase::verify_expected_entities(
                                          const EntityList &entity_list) {
  if(debug) { 
    printf("expected entities: \n"); 
    print_entities(expected_entities); 
    printf("got entities: \n"); 
    print_entities(entity_list);
  } 
  BOOST_REQUIRE(entity_list.size() == expected_entities.size());  
 
  EntityList::const_iterator it = entity_list.begin(); 
  for( ; it != entity_list.end(); it++) {  
      BOOST_REQUIRE(
          find(expected_entities.begin(), expected_entities.end(), *it) 
          != expected_entities.end()); 
  }

  test_sem->up();
} 

void BindingsStorageTestCase::print_entities(const EntityList &entity_list) {

  EntityList::const_iterator it = entity_list.begin(); 
  for( ; it != entity_list.end(); it++) {  
    printf("dpid = %"PRId64" port = %d dladdr = %"PRId64" nwaddr = %d \n", 
        it->dpid.as_host(),it->port,it->dladdr.hb_long(),it->nwaddr); 
  }
} 

void BindingsStorageTestCase::do_test() {
    // 2 rows changed
    b_store->store_binding_state(datapathid::from_host(1),1,1,1,
        "javelina",Name::HOST,false,0); 

    // 1 row changed 
    b_store->store_binding_state(datapathid::from_host(1),1,1,1,
        "dan",Name::USER,false,0);
    
    // this entry has its own IP, but should have the same ID as the 
    // previous rows this entry has its own IP, but should have the 
    // same ID as the previous rows 
    // 2 rows changed 
    b_store->store_binding_state(datapathid::from_host(1),
        1,1,2,"dan-2",Name::USER,true,1);
    

    // test case where two hosts are "behind" a router.  mac address and 
    // AP are the same, but IP is different and b/c neither "owns_dl", 
    // the ip_in_db is 0 
    b_store->store_binding_state(datapathid::from_host(9),9,9,100,
        "behind-router 1", Name::HOST, false, 0);
    b_store->store_binding_state(datapathid::from_host(9),9,9,101,
        "behind-router 2", Name::HOST, false,0);

    // do all semaphore waiting last, to make sure serialization
    // of ADDs works
    // 2 + 1 + 2 + 2 + 2
    waitForX(9); 
    
    if(debug) printf("TEST: get names where mac = 1 \n"); 
    expected_names.clear(); 
    expected_names.push_back(Name("dan",Name::USER)); 
    expected_names.push_back(Name("dan-2",Name::USER)); 
    expected_names.push_back(Name("javelina",Name::HOST)); 
    expected_names.push_back(Name("none;000000000001:1",Name::LOCATION)); 
    b_store->get_names_by_mac(1, 
      boost::bind(&BindingsStorageTestCase::verify_expected_names, this, _1)); 
    waitForX(1);  
    
    // note: because IP 2 was given IP 1 as a ip_in_db, this 
    // should return values that were bound to either IP 1 or IP 2
    if(debug) printf("TEST: get names where ip = 2 \n"); 
    expected_names.clear(); 
    expected_names.push_back(Name("dan",Name::USER)); 
    expected_names.push_back(Name("dan-2",Name::USER)); 
    expected_names.push_back(Name("javelina",Name::HOST)); 
    expected_names.push_back(Name("none;000000000001:1",Name::LOCATION)); 
    b_store->get_names_by_ip(2, 
      boost::bind(&BindingsStorageTestCase::verify_expected_names, this, _1)); 
    waitForX(1);  

    if(debug) printf("TEST: get names with (dpid,port,mac,ip) = (1,1,1,2) \n"); 
    // this test should be exactly the same as the last one, for the same reason
    b_store->get_names(datapathid::from_host(1),1,1,2,
        boost::bind(&BindingsStorageTestCase::verify_expected_names,this, _1)); 
    waitForX(1);  

    // name lookup for non-existent entity 
    expected_names.clear(); 
    if(debug) printf("TEST: get names with non-existent IP \n"); 
    b_store->get_names_by_ip(400, 
      boost::bind(&BindingsStorageTestCase::verify_expected_names, this, _1)); 
    waitForX(1);  

    // test search based on name
    if(debug) printf("TEST: get_entities with name = javelina \n"); 
    expected_entities.clear(); 
    expected_entities.push_back(NetEntity(datapathid::from_host(1),1,1,1)); 
    expected_entities.push_back(NetEntity(datapathid::from_host(1),1,1,2)); 
    b_store->get_entities_by_name("javelina",Name::HOST, 
                boost::bind(&BindingsStorageTestCase::verify_expected_entities,
                  this, _1)); 
    waitForX(1);  

    // entity lookup with non-existent name
    if(debug) printf("TEST: get_entities with non-existent name \n"); 
    expected_entities.clear(); 
    b_store->get_entities_by_name("does-not-exist",Name::HOST, 
      boost::bind(&BindingsStorageTestCase::verify_expected_entities,this, _1)); 
    waitForX(1);  
   
    if(debug) printf("TEST: get_names with ip = 101 \n"); 
    b_store->get_names_by_ip(101, 
      boost::bind(&BindingsStorageTestCase::verify_expected_names, this, _1));
    expected_names.clear(); 
    expected_names.push_back(Name("behind-router 2",Name::HOST));
    expected_names.push_back(Name("none;000000000009:9",Name::LOCATION)); 
    waitForX(1);  
   
    // removes a single row from the  bindings_name table 
    if(debug) printf("TEST: remove 'behind-router 2' from name table \n"); 
    b_store->remove_binding_state(datapathid::from_host(9),9,9,101,
        "behind-router 2", Name::HOST);
    waitForX(1);  
     
    expected_names.clear(); // should return only a location name
    expected_names.push_back(Name("none;000000000009:9",Name::LOCATION)); 
    b_store->get_names_by_ip(101, 
      boost::bind(&BindingsStorageTestCase::verify_expected_names, this, _1)); 
    waitForX(1);  
     
    // this call removes 5 rows.  Two from the id-table and three 
    // from the names-table
    if(debug) printf("TEST: remove_machine for (1,1,1,1) \n"); 
    b_store->remove_machine(datapathid::from_host(1),1,1,1, true); 
    waitForX(5);  
    expected_names.clear();
    b_store->get_names_by_mac(1, 
      boost::bind(&BindingsStorageTestCase::verify_expected_names, this, _1));
    waitForX(1);  

    // test that "owns dl" allows us to delete only one IP
    b_store->store_binding_state(datapathid::from_host(6),7,8,0,
        "Host A (ip = 0)",Name::HOST,false, 0);
    waitForX(2);  
    
    b_store->store_binding_state(datapathid::from_host(6),7,8,200,
        "Host A (ip = 200)",Name::HOST, true, 1);
    waitForX(2);  

    expected_names.clear();
    expected_names.push_back(Name("Host A (ip = 0)",Name::HOST));
    expected_names.push_back(Name("Host A (ip = 200)",Name::HOST));
    expected_names.push_back(Name("none;000000000006:7",Name::LOCATION)); 
    b_store->get_names_by_ap(datapathid::from_host(6),7, 
      boost::bind(&BindingsStorageTestCase::verify_expected_names, this, _1)); 
    waitForX(1);  

    // now we remove only one of the IP addresses, leaving the IP = 0 entry
    b_store->remove_machine(datapathid::from_host(6),7,8,200,false);
    waitForX(1);  
    
    expected_names.clear();
    expected_names.push_back(Name("Host A (ip = 0)",Name::HOST));
    expected_names.push_back(Name("none;000000000006:7",Name::LOCATION)); 
    b_store->get_names_by_ap(datapathid::from_host(6),7, 
      boost::bind(&BindingsStorageTestCase::verify_expected_names, this, _1)); 
    waitForX(1);  
    
    // use 'name-less' call to bind state
    b_store->store_binding_state(datapathid::from_host(30),30,30,30, false);
    waitForX(1);  

    // we should make sure no name entry was added. 
    // note: this test is a bit suspect, b/c the above semaphore call
    // assumes that only one row is written.  if in fact a name row was
    // being written, it may not have been completed, causing this next
    // get_names call to return no results, even though the name was 
    // still being inserted.  I check manually that this was not the case
    // by adding a second test_sem->down() above and making sure the test
    // hung. 
    expected_names.clear(); 
    expected_names.push_back(Name("none;00000000001e:30",Name::LOCATION)); 
    b_store->get_names_by_ip(30, 
      boost::bind(&BindingsStorageTestCase::verify_expected_names, this, _1)); 
    waitForX(1);  
    
    if(debug) printf("TEST: get_entity, single user on two different hosts\n"); 
    b_store->store_binding_state(datapathid::from_host(31),31,31,31,
        "bob", Name::USER, false, 0);
    b_store->store_binding_state(datapathid::from_host(32),32,32,32,
        "bob", Name::USER, false,0);
    waitForX(4);  
    expected_entities.clear(); 
    expected_entities.push_back(NetEntity(datapathid::from_host(31),31,31,31)); 
    expected_entities.push_back(NetEntity(datapathid::from_host(32),32,32,32)); 
    b_store->get_entities_by_name("bob",Name::USER, 
                boost::bind(&BindingsStorageTestCase::verify_expected_entities,
                  this, _1)); 
    waitForX(1);  

    

    if(debug) printf("TEST: confirm that removes are serialized \n"); 
    expected_names.clear(); 
    b_store->get_names(datapathid::from_host(1),1,1,2,
        boost::bind(&BindingsStorageTestCase::verify_expected_names,this, _1)); 
    waitForX(1);  

    np_store->remove_trigger(tid1, 
      boost::bind(&BindingsStorageTestCase::remove_trigger_callback,this,_1)); 
    np_store->remove_trigger(tid2, 
      boost::bind(&BindingsStorageTestCase::remove_trigger_callback,this,_1)); 
    
    // main_sem->up() called by callbacks for trigger removal 
} 

void BindingsStorageTestCase::trigger_fired(const storage::Trigger_id & tid,
                                        const storage::Row& r) { 
  
    if(tid.ring == Bindings_Storage::ID_TABLE_NAME) {
      if(debug) { 
        printf("bindings_id row modified:\n"); 
        Storage_Util::print_row(r); 
      }
    } else if(tid.ring == Bindings_Storage::NAME_TABLE_NAME){ 
      if(debug) { 
        printf("bindings_name row modified \n"); 
        Storage_Util::print_row(r);
      }
    }

    test_sem->up();  // we do a sem->up() per row changed
} 

// insert trigger for 'bindings_id' and bindings_name' table
void BindingsStorageTestCase::set_triggers() {
   
      np_store->put_trigger(Bindings_Storage::ID_TABLE_NAME,true, 
         boost::bind(&BindingsStorageTestCase::trigger_fired, this, _1, _2),
         boost::bind(&BindingsStorageTestCase::trigger_inserted, this, _1, _2) ); 
          
      np_store->put_trigger(Bindings_Storage::NAME_TABLE_NAME,true, 
         boost::bind(&BindingsStorageTestCase::trigger_fired, this, _1, _2),
         boost::bind(&BindingsStorageTestCase::trigger_inserted, this, _1, _2) ); 
} 

// callback indicating that trigger insertion is finished
void BindingsStorageTestCase::trigger_inserted(const storage::Result &result,
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
    
void BindingsStorageTestCase::remove_trigger_callback(
    const storage::Result &result) { 
  if(result.code != storage::Result::SUCCESS) 
    lg.err("removing table trigger failed: %s \n", result.message.c_str());
  --trigger_count;
  if(trigger_count == 0) 
    main_sem->up();  
} 


}  //unamed namespace 

BOOST_AUTO_COMPONENT_TEST_CASE(BindingsStorageTest, BindingsStorageTestCase);

