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
#include <list>
#include <sys/time.h>
#include "hash_map.hh"

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include "component.hh"
#include "vlog.hh"
#include "user_event_log/user_event_log.hh" 
#include "bindings_storage/bindings_storage.hh"
#include "data/datacache.hh"
#include "bootstrap-complete.hh"

using namespace std;
using namespace vigil;
using namespace vigil::container;
using namespace vigil::applications;
using namespace boost;


namespace vigil {
namespace applications { 

static Vlog_module lg("user-event-log-test");



class UserEventLogTest2
    : public Component
{
public:

    UserEventLogTest2(const container::Context* c,
                      const json_object* xml) 
        : Component(c), counter(0) {
    }

    void configure(const Configuration*) {
        resolve(uel);
        resolve(b_store);
        resolve(data_cache);
        register_handler<Bootstrap_complete_event>(
            boost::bind(&UserEventLogTest2::bootstrap_complete_cb,
                        this, _1)); 
    }

    void install() {} 

    Disposition bootstrap_complete_cb(const Event& e) {
        int64_t locid = 5;
        int64_t switchid = 5;
        int64_t userid = 5;
        int64_t hostid = 5;
        data_cache->update_switch(switchid, "noopenflow #1", 0, datapathid::from_host(1));
        b_store->add_name_for_location(datapathid::from_host(1), 0, switchid, Name::SWITCH);
        data_cache->update_location(locid, "the dance cube, port #1",
                                    0, switchid, 1, "eth0");
        b_store->add_name_for_location(datapathid::from_host(1), 1, locid, Name::LOCATION);
        data_cache->update_user(userid, "dan");
        data_cache->update_host(hostid, "javelina");

        b_store->store_location_binding(ethernetaddr(1), locid);
        b_store->store_host_binding(hostid, ethernetaddr(1), 1);
        b_store->store_user_binding(userid, hostid);

        // this is a hack for simplicity.  we assume that NDB writes will
        // complete in 2 seconds, which should be quite safe 
        timeval tv = { 2, 0 };
        post(boost::bind(&UserEventLogTest2::timer_callback, this), tv);
        return CONTINUE; 
    }

    void timer_callback() {
      char buf[128]; 
      snprintf(buf,128,"message #%d  source info: {su}, {sh}, {sl}", counter);
      string s(buf);

      ethernetaddr src_dladdr(1); 
      ethernetaddr dst_dladdr(9);
      LogEntry lentry("UserEventLogTest2",LogEntry::INFO, s); 
      lentry.addMacKey(src_dladdr,LogEntry::SRC); 
      lentry.addMacKey(dst_dladdr,LogEntry::DST);
      uel->log(lentry); 
      timeval tv = { 5, 0 };
      post(boost::bind(&UserEventLogTest2::timer_callback, this), tv);
      ++counter; 
    }

private:
   
    int counter; 
    User_Event_Log *uel;
    Bindings_Storage* b_store;
    Data_cache* data_cache;
};




REGISTER_COMPONENT(container::Simple_component_factory<UserEventLogTest2>, 
                   UserEventLogTest2);

}
} 

