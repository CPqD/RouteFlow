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
#include "hash_map.hh"

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include "component.hh"
#include "storage/storage.hh" 
#include "storage/storage_util.hh"
#include "vlog.hh"
#include "tests.hh"
#include "user_event_log/user_event_log.hh" 
#include "threads/cooperative.hh"
#include "bindings_storage/bindings_storage.hh"

using namespace std;
using namespace vigil;
using namespace vigil::container;
using namespace vigil::applications;
using namespace boost;
using namespace vigil::testing;


namespace  {

static Vlog_module lg("user-event-log-test");

class UserEventLogTestCase
    : public Test_component
{
public:

    UserEventLogTestCase(const container::Context* c,
                const json_object* xml) 
        : Test_component(c), trigger_count(0), debug(false)   {
    }

    void configure(const Configuration*) {
    }

    void install() {
          resolve(uel);
          // disable auto-clearing of UEL 
          uel->set_max_num_entries(-1); 
          resolve(store); 
          resolve(b_store); 
          main_sem = new Co_sema(); 
          test_sem = new Co_sema(); 
    }

    void run_test();

    static const int MAX_ROUNDS = 5; 
private:
    
    void set_trigger(); 
    void trigger_fired(const storage::Trigger_id & tid,const storage::Row&); 
    void trigger_inserted(const storage::Result &result,
        const storage::Trigger_id &tid);
    void remove_trigger_callback(const storage::Result &result); 
    void exit_test(bool success);
    void log_entry_cb(int64_t logid, int64_t ts, const string &app, int level, 
      const string &msg, const NameList &src_names, const NameList &dst_names);
    void do_test(); 
    void clear_cb(); 

    User_Event_Log *uel; // create message 
    storage::Async_storage *store; // confirm that messages were created
    Bindings_Storage *b_store; 
    storage::Trigger_id tid1,tid2,tid3; // used to cancel trigger
    int64_t triggered_logid; 
    int trigger_count; 
    string cur_msg;
    NameList expected_src_names, expected_dst_names; 

    bool debug; 
    Co_thread thread;
    Co_sema *main_sem, *test_sem;
};

void UserEventLogTestCase::run_test() {
  thread.start(boost::bind(&UserEventLogTestCase::set_trigger, this));
  main_sem->down();
} 

// insert trigger for 'user_event_log' table
void UserEventLogTestCase::set_trigger() {

    // first clear the table
    b_store->clear(boost::bind(&UserEventLogTestCase::clear_cb,this)); 
    test_sem->down(); 

      store->put_trigger(User_Event_Log::MAIN_TABLE_NAME,true, 
         boost::bind(&UserEventLogTestCase::trigger_fired, this, _1, _2),
         boost::bind(&UserEventLogTestCase::trigger_inserted, this, _1, _2) ); 
   
      store->put_trigger(User_Event_Log::NAME_TABLE_NAME,true, 
         boost::bind(&UserEventLogTestCase::trigger_fired, this, _1, _2),
         boost::bind(&UserEventLogTestCase::trigger_inserted, this, _1, _2) ); 
         
      // so we know when bindings operations have completed
      store->put_trigger(Bindings_Storage::NAME_TABLE_NAME,true, 
         boost::bind(&UserEventLogTestCase::trigger_fired, this, _1, _2),
         boost::bind(&UserEventLogTestCase::trigger_inserted, this, _1, _2) ); 
} 

// callback indicating that trigger insertion is finished
void UserEventLogTestCase::trigger_inserted(const storage::Result &result,
                                           const storage::Trigger_id &tid) {
  
  if(result.code != storage::Result::SUCCESS) { 
    lg.err("inserting table trigger failed: %s \n", result.message.c_str());
    exit_test(false);  
    return; 
  } 
  ++trigger_count; 
  if(trigger_count == 1) {
    tid1 = tid; 
  } else if(trigger_count == 2) {
    tid2 = tid; 
  }else if(trigger_count == 3) { 
    tid3 = tid; 
    do_test(); 
  }
} 


void UserEventLogTestCase::do_test() { 
  if(debug) printf("running user_event_log c++ test\n"); 

  Log_entry_callback fn = boost::bind(&UserEventLogTestCase::log_entry_cb, 
                        this,_1,_2,_3,_4,_5,_6,_7); 
  string app_name = "c++ test"; 

  cur_msg = "log mesg 1";
  if(debug) printf("logging msg: %s \n", cur_msg.c_str());
  uel->log_simple(app_name,LogEntry::INFO, cur_msg); 
  test_sem->down(); 

  // expect nothing as far as names
  uel->get_log_entry(triggered_logid, fn);
  test_sem->down(); 

  // add mapping for MAC = 1 and MAC = 2
  b_store->store_binding_state(datapathid::from_host(1),1,1,1,
        "javelina",Name::HOST,false,0); 
  b_store->store_binding_state(datapathid::from_host(1),1,1,1,
      "dan",Name::USER,false,0); 
  b_store->store_binding_state(datapathid::from_host(2),2,2,2,
      "finland",Name::HOST,false, 0); 
  b_store->store_binding_state(datapathid::from_host(2),2,2,2,
      "teemu",Name::USER,false,0); 
  test_sem->down();
  test_sem->down();
  test_sem->down();
  test_sem->down();
 
  cur_msg = "MAC = 1 and MAC = 2 are BAD"; 
  if(debug) printf("logging msg: %s \n", cur_msg.c_str());
  LogEntry e1(app_name,LogEntry::INFO, cur_msg);
  e1.addMacKey(ethernetaddr(1),LogEntry::SRC); 
  e1.addMacKey(ethernetaddr(2),LogEntry::DST); 
  uel->log(e1); 
  test_sem->down();  // adds six name rows, one main row
  test_sem->down();
  test_sem->down();
  test_sem->down();
  test_sem->down();
  test_sem->down();
  test_sem->down();

  expected_src_names.clear();
  expected_src_names.push_back(Name("javelina",Name::HOST));
  expected_src_names.push_back(Name("dan",Name::USER));
  expected_src_names.push_back(Name("none;000000000001:1#none;000000000001#1",Name::LOCATION));
  expected_dst_names.clear();
  expected_dst_names.push_back(Name("finland",Name::HOST));
  expected_dst_names.push_back(Name("teemu",Name::USER));
  expected_dst_names.push_back(Name("none;000000000002:2#none;000000000002#2",Name::LOCATION));
  uel->get_log_entry(triggered_logid, fn);
  test_sem->down(); 
  
  cur_msg = "src IP = 1 is good, dest unknown";
  if(debug) printf("logging msg: %s \n", cur_msg.c_str());
  LogEntry e2(app_name,LogEntry::INFO, cur_msg);
  e2.addIPKey(1,LogEntry::SRC); 
  e2.addIPKey(0,LogEntry::DST); 
  uel->log(e2); 
  test_sem->down();
  test_sem->down();
  test_sem->down();
  test_sem->down();
  
  expected_src_names.clear();
  expected_src_names.push_back(Name("javelina",Name::HOST));
  expected_src_names.push_back(Name("dan",Name::USER));
  expected_src_names.push_back(Name("none;000000000001:1#none;000000000001#1",Name::LOCATION));
  expected_dst_names.clear();
  uel->get_log_entry(triggered_logid, fn);
  test_sem->down(); 
  
  cur_msg = "dst dpid = 2,port = 2 is good, src unknown";
  if(debug) printf("logging msg: %s \n", cur_msg.c_str());
  LogEntry e3(app_name,LogEntry::INFO, cur_msg);
  e3.addLocationKey(datapathid::from_host(0),0,LogEntry::SRC); 
  e3.addLocationKey(datapathid::from_host(2),2,LogEntry::DST); 
  uel->log(e3); 
  test_sem->down();
  test_sem->down();
  test_sem->down();
  test_sem->down();
  
  expected_src_names.clear();
  expected_dst_names.clear();
  expected_dst_names.push_back(Name("finland",Name::HOST));
  expected_dst_names.push_back(Name("teemu",Name::USER));
  expected_dst_names.push_back(Name("none;000000000002:2#none;000000000002#2",Name::LOCATION));
  uel->get_log_entry(triggered_logid, fn);
  test_sem->down(); 

  // done logging
  store->remove_trigger(tid1, 
        boost::bind(&UserEventLogTestCase::remove_trigger_callback,this,_1)); 
  store->remove_trigger(tid2, 
        boost::bind(&UserEventLogTestCase::remove_trigger_callback,this,_1)); 
  store->remove_trigger(tid3, 
        boost::bind(&UserEventLogTestCase::remove_trigger_callback,this,_1)); 

} 


void UserEventLogTestCase::log_entry_cb(int64_t logid, int64_t ts, 
                        const string &app, int level, const string &msg, 
                        const NameList &src_names, const NameList &dst_names){
  if(debug) { 
    printf("expected src names: \n"); 
    Bindings_Storage::print_names(expected_src_names); 
    printf("expected dst names: \n"); 
    Bindings_Storage::print_names(expected_dst_names); 
    printf("got src names: \n"); 
    Bindings_Storage::print_names(src_names); 
    printf("got dst names: \n"); 
    Bindings_Storage::print_names(dst_names); 
  } 

  BOOST_REQUIRE(src_names.size() == expected_src_names.size());  
  NameList::const_iterator it = src_names.begin(); 
  for( ; it != src_names.end(); it++) {  
    BOOST_REQUIRE(find(expected_src_names.begin(), expected_src_names.end(), 
          *it) != expected_src_names.end()); 
  }

  BOOST_REQUIRE(dst_names.size() == expected_dst_names.size());  
  it = dst_names.begin(); 
  for( ; it != dst_names.end(); it++) {  
    BOOST_REQUIRE(find(expected_dst_names.begin(), expected_dst_names.end(), 
          *it) != expected_dst_names.end()); 
  }


  test_sem->up(); 
} 

void UserEventLogTestCase::trigger_fired(const storage::Trigger_id & tid,
                                        const storage::Row& r) { 
  if(tid.ring == User_Event_Log::MAIN_TABLE_NAME) {
    try {
      // message inserted into table should be same one that we just logged 
      BOOST_REQUIRE(cur_msg == Storage_Util::get_col_as_type<string>(r,"msg"));
      triggered_logid = Storage_Util::get_col_as_type<int64_t>(r,"logid");
    }catch (exception &e) { 
      printf("%s \n", e.what());
      exit_test(false); 
      return;
    }
  } 
  test_sem->up(); 
} 


void UserEventLogTestCase::remove_trigger_callback(
                                    const storage::Result &result) { 
  if(result.code != storage::Result::SUCCESS) { 
    lg.err("removing table trigger failed: %s \n", result.message.c_str());
    exit_test(false); 
    return; 
  } 
  // exit test with success
  --trigger_count; 
  if(trigger_count == 0) 
    exit_test(true); 
} 

// use semephore to tell test runner that we are done
void UserEventLogTestCase::exit_test(bool success) { 
  if(!success) 
    BOOST_REQUIRE(false); // fail
  main_sem->up(); 
} 

void UserEventLogTestCase::clear_cb() { 
  test_sem->up(); // so we can wait for clear to finish 
} 


}  //unamed namespace 

BOOST_AUTO_COMPONENT_TEST_CASE(UserEventLogTest, UserEventLogTestCase);
