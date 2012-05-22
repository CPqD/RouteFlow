/* Copyright 2008, 2009 (C) Nicira, Inc.
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

#include <sstream>
#include "component.hh"
#include "storage/storage.hh" 
#include "storage/storage_util.hh"
#include "vlog.hh"
#include "user_event_log/user_event_log.hh" 
#include "threads/cooperative.hh"

#include <malloc.h> 

#include "nox.hh"

using namespace std;
using namespace vigil;
using namespace vigil::container;
using namespace vigil::applications;
using namespace boost;


namespace  {

static Vlog_module lg("user-event-log-test");

class UELMemleakTest
    : public Component
{
public:

    UELMemleakTest(const container::Context* c,
                const json_object* xml) 
        : Component(c), trigger_count(0), counter(0)  {
    }

    void configure(const Configuration*) {
    }

    void install() {
        resolve(uel);
        resolve(store); 
        resolve(b_store); 

        // disables auto-clear, which messes up 
        // our trigger logic
        uel->set_max_num_entries(-1);  
        test_sem = new Co_sema(); 
        timeval tv = { 1, 0 }; 
        post(boost::bind(&UELMemleakTest::set_trigger, this),tv);
    }

private:
    void waitForX(int x) {
      for(int i = 0; i < x; ++i) 
        test_sem->down(); 
    } 
    void set_trigger(); 
    void trigger_fired(const storage::Trigger_id & tid,const storage::Row&); 
    void trigger_inserted(const storage::Result &result,
        const storage::Trigger_id &tid);
    void remove_trigger_callback(const storage::Result &result); 
    void log_entry_cb(int64_t logid, int64_t ts, const string &app, int level, 
                      const string &msg, const PrincipalList &src_names, 
                      const PrincipalList &dst_names);
    void do_test(); 
    void do_clear();
    void clear_cb(); 
    void setup_bindings(); 

    User_Event_Log *uel; // create message 
    storage::Async_storage *store; // confirm that messages were created
    Bindings_Storage *b_store;
    int64_t triggered_logid; 
    int trigger_count, counter; 


    Co_thread thread;
    Co_sema *test_sem;
};


// insert trigger for user_event_log tables, and the names table for
// the bindings storage. 
void UELMemleakTest::set_trigger() {


      store->put_trigger(User_Event_Log::MAIN_TABLE_NAME,true, 
         boost::bind(&UELMemleakTest::trigger_fired, this, _1, _2),
         boost::bind(&UELMemleakTest::trigger_inserted, this, _1, _2) ); 
   
      store->put_trigger(User_Event_Log::NAME_TABLE_NAME,true, 
         boost::bind(&UELMemleakTest::trigger_fired, this, _1, _2),
         boost::bind(&UELMemleakTest::trigger_inserted, this, _1, _2) ); 
         
      // so we know when bindings operations have completed
      store->put_trigger(Bindings_Storage::HOST_TABLE_NAME,true,
         boost::bind(&UELMemleakTest::trigger_fired, this, _1, _2),
         boost::bind(&UELMemleakTest::trigger_inserted, this, _1, _2) );
      store->put_trigger(Bindings_Storage::USER_TABLE_NAME,true,
         boost::bind(&UELMemleakTest::trigger_fired, this, _1, _2),
         boost::bind(&UELMemleakTest::trigger_inserted, this, _1, _2) );
      store->put_trigger(Bindings_Storage::DLADDR_TABLE_NAME,true,
         boost::bind(&UELMemleakTest::trigger_fired, this, _1, _2),
         boost::bind(&UELMemleakTest::trigger_inserted, this, _1, _2) );
} 

// callback indicating that trigger insertion is finished
void UELMemleakTest::trigger_inserted(const storage::Result &result,
                                           const storage::Trigger_id &tid) {
  
  if(result.code != storage::Result::SUCCESS) { 
    lg.err("inserting table trigger failed: %s \n", result.message.c_str());
    return; 
  } 
  ++trigger_count; 
  if(trigger_count == 5) 
    setup_bindings();  
} 

void UELMemleakTest::setup_bindings() { 
    // for actual names, must register with data_cache else all come up as
    // unknown
    int64_t locid = 5;
    int64_t userid = 5;
    int64_t hostid = 5;
    b_store->store_location_binding(ethernetaddr(1), locid);
    b_store->store_host_binding(hostid, ethernetaddr(1), 1);
    b_store->store_user_binding(userid, hostid);
    waitForX(3); // note: trigger on name table only....
    do_test();
}

void UELMemleakTest::do_test() { 
  printf("do_test()  counter = %d \n", counter); 
  struct mallinfo m = mallinfo();
  printf("**** mallinfo:  KB used = %d  *****\n", m.uordblks / 1000 ); 

  string app_name = "c++ memleak test"; 
  int start = counter; 
  counter += 1000; 

  // write loop 
  for(int i = start; i < counter; ++i) { 
    stringstream ss; 
    ss << "log message # " << i ; 
    LogEntry e(app_name, LogEntry::INFO, ss.str()); 
    e.addMacKey(ethernetaddr(1),LogEntry::SRC); 
    e.addMacKey(ethernetaddr(2),LogEntry::DST);
    uel->log(e);
    waitForX(4); // adds main log entry, and three names rows 
                // (dan, javelina, and unknown;000000001) )
  } 
 

  // read loop 
  // i = 0 ==> leak
  // i = start ==> no leak
  for(int i = /*start*/ 0; i < counter; ++i) { 
      Log_entry_callback fn = boost::bind(&UELMemleakTest::log_entry_cb, 
                                          this,_1,_2,_3,_4,_5,_6,_7); 
      uel->get_log_entry(i, fn);
      test_sem->down();
  }

  printf("sleeping before clear... \n"); 
  timeval tv = { 2, 0 }; 
  post(boost::bind(&UELMemleakTest::do_clear, this),tv);
  //post(boost::bind(&UELMemleakTest::do_test, this),tv);

} 

void UELMemleakTest::do_clear() {
  printf("clearing \n"); 
  Clear_log_callback clc = boost::bind(&UELMemleakTest::clear_cb,this);
  uel->clear(clc); 
} 


void UELMemleakTest::clear_cb() { 
  printf("Log cleared.  Clearing semaphore then sleeping ... \n");
  // note: b/c we use the semaphore to determine when a write has
  // completed, we need to clear it otherwise it will have a high
  // value already due to triggers firing when we clear the table. 
  while(test_sem->try_down()) { } 

  store->debug(User_Event_Log::MAIN_TABLE_NAME);
  store->debug(User_Event_Log::NAME_TABLE_NAME);
  nox::timer_debug();

  timeval tv = { 2, 0 }; 
  post(boost::bind(&UELMemleakTest::do_test, this),tv);
} 


void UELMemleakTest::log_entry_cb(int64_t logid, int64_t ts, const string &app, 
                                  int level, const string &msg, 
                                  const PrincipalList &src_names, 
                                  const PrincipalList &dst_names){
    //printf("got log entry with logid = %lld \n", logid); 
    test_sem->up();
}
 
void UELMemleakTest::trigger_fired(const storage::Trigger_id & tid,
                                        const storage::Row& r) {
    //printf("trigger_fired : %s \n", tid.ring.c_str()); 
    test_sem->up(); 
} 




}  //unamed namespace 

REGISTER_COMPONENT(container::Simple_component_factory<UELMemleakTest>, 
                   UELMemleakTest);
