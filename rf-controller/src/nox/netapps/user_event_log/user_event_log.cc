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
#include <inttypes.h>
#include <list>
#include <sys/time.h>
#include <sstream> 
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include "storage/storage_util.hh"
#include "storage/storage-blocking.hh"
#include "vlog.hh"

#include "user_event_log.hh"

using namespace std;
using namespace vigil;
using namespace vigil::container;
using namespace vigil::applications;


// the User_Event_Log component uses two NDB tables.  The main log
// table has the following scheme: 
//
// | logid (int), ts (int), app (string), level (int), msg (string) |
//
// logid is the only unique value in this table.  
//
// the second table is the 'name table' which stores the high-level
// names (if any) associated with each log entry, identified by a logid.
// there can be an arbitrary number of name table rows per logid. 
//
// | logid (int), name (string), name_type (int), direction (int) |
//
// Direction indicates whether this name is a 'source' or a 'destination'
// name for this log entry


namespace vigil {
namespace applications { 

static Vlog_module lg("user_event_log");

const string User_Event_Log::MAIN_TABLE_NAME = "user_event_log"; 
const string User_Event_Log::NAME_TABLE_NAME = "user_event_log_names"; 

void User_Event_Log::configure(const container::Configuration* conf) {
  resolve(np_store);
  resolve(b_store);
  resolve(datatypes);
  Component_argument_list clist = conf->get_arguments(); 
  Component_argument_list::const_iterator cit = clist.begin();
  while(cit != clist.end()){
    char *key = strdup(cit->c_str()); 
    char *value = strchr(key,'='); 
    if(value != NULL) {
      *value = NULL;
      ++value;
      if(!strcmp(key,"max_num_entries")) {  
        this->max_num_entries = atoi(value);
      } 
    }
    free(key); 
    ++cit; 
  } 
}

void User_Event_Log::install() {
        create_table();
        
        timeval tv = make_timeval(10,0); 
        post(boost::bind(&User_Event_Log::check_excess_entries,this), tv); 
}


void User_Event_Log::create_table() { 
  storage::Column_definition_map main_table_def; 
  main_table_def["logid"] = (int64_t)0; 
  main_table_def["ts"] = (int64_t)0;
  main_table_def["app"] = ""; 
  main_table_def["level"] = (int64_t)0;
  main_table_def["msg"] = ""; 

  storage::Index_list main_table_indices; 
  storage::Index id_only;
  id_only.columns.push_back("logid");
  id_only.name = "maintable_logid_only"; 
  main_table_indices.push_back(id_only); 

  storage::Sync_storage sync_store(np_store);
  storage::Result result = sync_store.create_table(MAIN_TABLE_NAME, 
                                main_table_def, main_table_indices);  
  if(result.code == storage::Result::SUCCESS){ 
      is_ready = true;
  } else {
      lg.err("create table '%s' failed: %s \n", 
          MAIN_TABLE_NAME.c_str(), result.message.c_str()); 
  }

  storage::Column_definition_map name_table_def; 
  name_table_def["logid"] = (int64_t)0; 
  name_table_def["uid"] = (int64_t)0; 
  name_table_def["principal_type"] = (int64_t)0;
  name_table_def["direction"] = (int64_t)0;
  
  storage::Index_list name_table_indices; 
  storage::Index n_id_only;
  n_id_only.columns.push_back("logid");
  n_id_only.name = "nametable_logid_only"; 
  name_table_indices.push_back(n_id_only); 
  storage::Index name_and_type; 
  name_and_type.columns.push_back("uid");
  name_and_type.columns.push_back("principal_type");
  name_and_type.name = "nametable_name_and_type";
  name_table_indices.push_back(name_and_type); 

  result = sync_store.create_table(NAME_TABLE_NAME, name_table_def, 
                                              name_table_indices);  
  if(result.code == storage::Result::SUCCESS){ 
      is_ready = true;
  } else {
      lg.err("create table '%s' failed: %s \n", 
          NAME_TABLE_NAME.c_str(), result.message.c_str()); 
  }
  
} 

// helper function to initialize a storage row 
bool User_Event_Log::init_main_row(storage::Row &log_entry, 
          const string &app_name, LogEntry::Level level, const string &msg){ 

  if(!is_ready){
      lg.err("failed create log entry, database not initialized \n"); 
      return false; 
  }
  const struct timeval tv = do_gettimeofday(); 
  log_entry["app"] = app_name;
  log_entry["level"] = (int64_t)level; 
  log_entry["msg"] = msg; 
  log_entry["ts"] = (int64_t) tv.tv_sec; 
  log_entry["logid"] = next_log_id++; 
  return true; 
}

void User_Event_Log::log_simple(const string &app_name, LogEntry::Level level,
                                    const string &msg) { 
  AddEntryInfo_ptr info = AddEntryInfo_ptr(new AddEntryInfo()); 

  if(init_main_row(info->log_row,app_name,level,msg))
      run_log_fsm(info); 
}

void User_Event_Log::log(const LogEntry & entry) { 

  AddEntryInfo_ptr info = AddEntryInfo_ptr(new AddEntryInfo(
          entry.src_key_query,entry.dst_key_query,
          entry.src_locname_query,entry.dst_locname_query,
          entry.src_names,entry.dst_names));
  
  if(init_main_row(info->log_row,entry.app,entry.level,entry.msg))
      run_log_fsm(info); 
} 


// Add Log Entry state machine:  
//
// States:
// READ_SRC_LOCNAMES: If log came with a location set for the source,
//                    look it up directly from the bindings_storage
// READ_DST_LOCNAMES: If log came with a location set for the destination,
//                    look it up directly from the bindings_storage
// READ_SRC_NAMES : If log came with src address info, query the 
//                  Bindings_Store component to find names associated 
//                  with that source address
// READ_DST_NAMES : If log came with dst address info, query the 
//                  Bindings_Store component to find names associated 
//                  with that dst address
// WRITE_MAIN_ENTRY: Do NDB write in the main 'user_event_log' table
// WRITE_NAMES     : Each call to run_log_fsm in this state will write
//                   a single entry to to the 'user_event_log_names' table
void User_Event_Log::run_log_fsm(AddEntryInfo_ptr info) {

  switch(info->cur_state) {
    case READ_SRC_LOCNAMES:
      if(info->src_locname_query.size() > 0) {
        b_store->get_names_for_location(info->src_locname_query,
          boost::bind(&User_Event_Log::get_names_cb,this,_1,info),
          Name::LOC_TUPLE); 
        return;
      } 
      info->cur_state = READ_DST_LOCNAMES; // else fall through
    case READ_DST_LOCNAMES:
      if(info->dst_locname_query.size() > 0) { 
        b_store->get_names_for_location(info->dst_locname_query,
          boost::bind(&User_Event_Log::get_names_cb,this,_1,info), 
          Name::LOC_TUPLE); 
        return;
      }  
      info->cur_state = READ_SRC_NAMES; // else fall through
    case READ_SRC_NAMES:
      if(info->src_key_query.size() > 0) { 
          b_store->get_names(info->src_key_query, true,
                             boost::bind(&User_Event_Log::get_names_cb,this,_1,info)); 
        return;
      } 
      info->cur_state = READ_DST_NAMES; // else fall through
    case READ_DST_NAMES:
      if(info->dst_key_query.size() > 0) { 
          b_store->get_names(info->dst_key_query, true,
                             boost::bind(&User_Event_Log::get_names_cb,this,_1,info)); 
        return; 
      }
      info->cur_state = WRITE_MAIN_ENTRY; // else fall through
    case WRITE_MAIN_ENTRY:
      np_store->put(MAIN_TABLE_NAME, info->log_row, 
              boost::bind(&User_Event_Log::write_callback, this, _1,info)); 
      return;
    case WRITE_NAMES:
       write_single_name_entry(info);
       return; 
  } 

  lg.err("invalid state in run_log_fsm \n"); 
}

// This function is needed because binding storage uses a different
// (outdated) set of enums to indicate principal types.  Until we update
// binding storage (and all of the code that accesses it) the user
// event log translates the bindings storage types to real principal types
// here
PrincipalType User_Event_Log::nametype_to_principaltype(Name::Type t) {
    switch (t) { 
      case Name::LOCATION: return datatypes->location_type(); 
      case Name::HOST: return datatypes->host_type(); 
      case Name::USER: return datatypes->user_type(); 
      case Name::SWITCH: return datatypes->switch_type(); 
      case Name::LOCATION_GROUP: return datatypes->group_type(); 
      case Name::HOST_GROUP: return datatypes->group_type(); 
      case Name::USER_GROUP: return datatypes->group_type(); 
      case Name::SWITCH_GROUP: return datatypes->group_type();
      default: return datatypes->invalid_type(); 
  } 
} 

// write a single entry to the names table
void User_Event_Log::write_name_row(AddEntryInfo_ptr info, 
                                    const Principal &p, int dir){
  int64_t logid = -1; 
  try { 
    storage::Row r;
    logid = Storage_Util::get_col_as_type<int64_t>(info->log_row,"logid");
    r["logid"] = logid; 
    r["direction"] = (int64_t) dir; 
    r["uid"] = p.id;
    r["principal_type"] = (int64_t) p.type;

    np_store->put(NAME_TABLE_NAME, r, 
        boost::bind(&User_Event_Log::write_callback,this,_1,info));  
  } catch (exception &e) { 
    lg.err("Error writing row for uid %"PRId64", logid = %"PRId64" : %s \n", 
          p.id , logid, e.what()); 
  } 
} 

// finds the next name entry that we need to write, removing it
// from info->src_names or info->dst_names, removing them once a
// write is dispatched.
// Once both lists are empty, it returns to the ADD state machine  
void User_Event_Log::write_single_name_entry(AddEntryInfo_ptr info){

  if(info->src_names.size() > 0) {
      Principal p = info->src_names.front(); 
      info->src_names.pop_front();
      write_name_row(info,p, 0); 
  }else if (info->dst_names.size() > 0) { 
      Principal p = info->dst_names.front();  
      info->dst_names.pop_front(); 
      write_name_row(info,p,1); 
  } //else, all done
} 

// callback for writes to either NDB table
// if the previous write was to the main table, the next write
// will be to the name table. 
void User_Event_Log::write_callback(const storage::Result & result,
                                    AddEntryInfo_ptr info){
    if(result.code != storage::Result::SUCCESS) {
      lg.err("write to NDB failed with error: %s \n", result.message.c_str()); 
      return; 
    } 
   if(info->cur_state == WRITE_MAIN_ENTRY){  
     info->cur_state = WRITE_NAMES; 
   }
   run_log_fsm(info); 
}

// The current semantics of LogEntry says
// that if a name is added via SetName() with a particular Direction and Type,
// the bindings lookup should NOT add any additional names of that type
// and direction to the log entry.  Thus, this method adds all names in
// 'from_lookup' to 'existing' except those that have the same type as
// a name already in 'existing'.  
void User_Event_Log::merge_names(PrincipalList &existing, 
                                const PrincipalList &from_lookup) {
  bool needs_user = true, needs_host = true, needs_location = true;
  bool needs_switch = true, needs_group = true; 

  PrincipalList::const_iterator it = existing.begin(); 
  for(; it != existing.end(); it++) { 
    PrincipalType type = it->type;
    if(type == datatypes->user_type()) needs_user = false;
    else if(type == datatypes->host_type()) needs_host = false;
    else if(type == datatypes->location_type()) needs_location = false;
    else if(type == datatypes->switch_type()) needs_switch = false;
    else if(type == datatypes->group_type()) needs_group = false;
  } 
 
  it = from_lookup.begin(); 
  for(; it != from_lookup.end(); it++) { 
    PrincipalType type = it->type;
    if(type == datatypes->user_type() && needs_user)
      existing.push_back(*it);
    else if(type == datatypes->host_type() && needs_host) 
      existing.push_back(*it);
    else if(type == datatypes->location_type() && needs_location) 
      existing.push_back(*it);
    else if(type == datatypes->switch_type() && needs_switch) 
      existing.push_back(*it);
    else if(type == datatypes->group_type() && needs_group) 
      existing.push_back(*it);
  } 

} 


// callback to Bindings_Store query for source LOCATION names
void User_Event_Log::get_names_cb(const NameList &names,AddEntryInfo_ptr info) {
 
  PrincipalList converted_list; 
  NameList::const_iterator it = names.begin();
  for(; it != names.end(); it++) { 
    Principal p; 
    p.id = it->id; 
    p.type = nametype_to_principaltype(it->name_type);
    if(p.type != datatypes->invalid_type()){ 
      converted_list.push_back(p); 
    } else { 
      lg.err("Invalid convertion to PrincipalType for Name::Type = %d\n", 
              it->name_type); 
    }
  } 

  if(info->cur_state == READ_SRC_LOCNAMES){
    merge_names(info->src_names,converted_list);
    info->cur_state = READ_DST_LOCNAMES;
  } else if(info->cur_state == READ_DST_LOCNAMES){
    merge_names(info->dst_names,converted_list);
    info->cur_state = READ_SRC_NAMES; 
  } else if(info->cur_state == READ_SRC_NAMES){
    merge_names(info->src_names, converted_list);
    info->cur_state = READ_DST_NAMES; 
  } else if(info->cur_state == READ_DST_NAMES){
    merge_names(info->dst_names, converted_list);  
    info->cur_state = WRITE_MAIN_ENTRY;  
  } else { 
    lg.err("Invalid state %d in get_names_cb \n", info->cur_state); 
    return; 
  }
  
  run_log_fsm(info); 
} 


// public function to get a log entry 
// this will require two NDB table reads, one to the main table
// and one to the name table.  
void User_Event_Log::get_log_entry(int64_t logid, Log_entry_callback &cb) {
  GetEntryInfo_ptr info = GetEntryInfo_ptr(new GetEntryInfo(logid,cb)); 
  storage::Query q;
  q["logid"] = logid;
  np_store->get(MAIN_TABLE_NAME,q,boost::bind(&User_Event_Log::read_main_cb,
                            this, _1,_2,_3,info));  
} 

// callback for NDB main table read launched by get_log_entry() 
void User_Event_Log::read_main_cb(const storage::Result & result, const storage::Context & ctx, 
                                const storage::Row &row, GetEntryInfo_ptr info){ 
   
    if(result.code == storage::Result::NO_MORE_ROWS) { 
      //lg.dbg("get_log_entry for non-existent logid = %"PRId64"\n", info->logid); 
    }else if(result.code != storage::Result::SUCCESS)  {
      lg.err("read_main had NDB failure: %s \n", result.message.c_str()); 
    } else { // SUCCESS

      try { 
        info->msg = Storage_Util::get_col_as_type<string>(row,"msg");          
        info->level = (int) Storage_Util::get_col_as_type<int64_t>(row,"level");
        info->app = Storage_Util::get_col_as_type<string>(row,"app");
        info->ts = (int)Storage_Util::get_col_as_type<int64_t>(row,"ts");

        // now start a query for the names 
        storage::Query q;
        q["logid"] = info->logid;
        np_store->get(NAME_TABLE_NAME,q,
            boost::bind(&User_Event_Log::read_names_cb,
              this, _1,_2,_3,info));  
        return; // return if no problems...

      } catch (exception &e) {
        lg.err("Exception reading main row for logid = %"PRId64": %s \n", 
            info->logid, e.what());
      } 
    }
    finish_get_entry(info); // on failure or NO_MORE_ROWS
}

// callback for NDB read of the name table, launched by read_main_cb()     
void User_Event_Log::read_names_cb(const storage::Result & result, const storage::Context & ctx, 
                                const storage::Row &row, GetEntryInfo_ptr info){ 
      
    if(result.code == storage::Result::NO_MORE_ROWS) { 
      finish_get_entry(info); // finished reading all rows
    }else if(result.code != storage::Result::SUCCESS)  {
      lg.err("read_names had NDB failure: %s (code = %d) \n", 
          result.message.c_str(), result.code); 
      finish_get_entry(info);
    } else { // SUCCESS, read next row 
      try { 
        Principal p; 
        p.id = Storage_Util::get_col_as_type<int64_t>(row,"uid");
        p.type = (PrincipalType)
          Storage_Util::get_col_as_type<int64_t>(row,"principal_type"); 
        if(Storage_Util::get_col_as_type<int64_t>(row,"direction") == 0) 
          info->src_names.push_back(p); 
        else
          info->dst_names.push_back(p); 

      } catch (exception &e) {
        lg.err("Exception read name row for logid = %"PRId64": %s \n", 
            info->logid, e.what()); 
      }

      // try to read more rows, even if we had an exception 
      np_store->get_next(ctx,boost::bind(&User_Event_Log::read_names_cb,
            this, _1,_2,_3,info));
   } 
}

// helper to do callback and free memory 
void User_Event_Log::finish_get_entry(GetEntryInfo_ptr info) {
      post(boost::bind(info->callback, info->logid, info->ts, info->app, 
              info->level,info->msg,info->src_names,info->dst_names)); 
}

// spawns a lookup in the name-table for all rows associated with the
// provided (name,type).  
void User_Event_Log::get_logids_for_name(int64_t id, PrincipalType t,
                                          Get_logids_callback &cb) {
  storage::Query q;
  q["uid"] = id;
  q["principal_type"] = (int64_t) t;
  GetLogidsInfo_ptr info = GetLogidsInfo_ptr(new GetLogidsInfo(cb));
  np_store->get(NAME_TABLE_NAME,q,boost::bind(&User_Event_Log::get_logids_cb,
        this, _1,_2,_3,info));  
} 

// processes single rows from the get_logids query, adding the logid of
// each to a list, and performing the callback when all rows have been seen
// or an error occurs
void User_Event_Log::get_logids_cb(const storage::Result & result, const storage::Context & ctx, 
                                const storage::Row &row, GetLogidsInfo_ptr info){ 
    if(result.code == storage::Result::NO_MORE_ROWS) { 
        Storage_Util::squash_list(info->logids); 
        post(boost::bind(info->callback, info->logids)); // success 
    }else if(result.code != storage::Result::SUCCESS)  {
        lg.err("get_logids_cb had NDB failure: %s (code = %d) \n", 
          result.message.c_str(), result.code); 
        post(boost::bind(info->callback, info->logids)); // return what we have
    } else { // SUCCESS, read next row 
      try { 
          int64_t id = Storage_Util::get_col_as_type<int64_t>(row,"logid");
          info->logids.push_back(id);
      } catch (exception &e) {
        lg.err("Exception in get_logids: %s \n", e.what()); 
      }

      // try to read more rows, even if we had an exception 
      np_store->get_next(ctx,boost::bind(&User_Event_Log::get_logids_cb,
            this, _1,_2,_3,info));
   } 

}

// this is the predicate function passed to Storage_Util::non_trans_remove .
// A predicate that just returns true indicates that all matching rows should be
// removed
static bool remove_all(const storage::Row &row) { 
  return true; 
} 

// removes a single entry 
void User_Event_Log::remove(int64_t logid, Clear_log_callback &cb) {
  storage::Query q;
  q["logid"] = logid; 
  boost::function<bool(const storage::Row &)> fn = boost::bind(&remove_all,_1);
  Storage_Util::non_trans_remove(np_store,MAIN_TABLE_NAME,q, 
        boost::bind(&User_Event_Log::remove_cb1,this,_1, cb,logid),fn);
}

// continues remove() by removing all name table entries 
void 
User_Event_Log::remove_cb1(const storage::Result &r, 
                          Clear_log_callback cb,int64_t logid) { 
  if(r.code != storage::Result::SUCCESS) {  
        lg.err("remove_cb1 NDB error: %s \n", r.message.c_str());
        post(boost::bind(cb,r)); 
        return; 
  } 
 
  storage::Query q;
  q["logid"] = logid; 
  boost::function<bool(const storage::Row &)> fn = boost::bind(&remove_all,_1);  
  Storage_Util::non_trans_remove(np_store,NAME_TABLE_NAME,q, 
        boost::bind(&User_Event_Log::remove_cb2,this,_1, cb),fn);
} 

void  // performs callback to indicate that remove() operation has finished 
User_Event_Log::remove_cb2(const storage::Result &r, Clear_log_callback cb) { 
  if(r.code != storage::Result::SUCCESS) {  
        lg.err("remove_cb2 NDB error: %s \n", r.message.c_str());
  }
  post(boost::bind(cb,r)); 
}

void User_Event_Log::internal_clear_cb(const storage::Result &r, 
    int64_t cur_logid, int retry_count, 
    int64_t stop_logid, Clear_log_callback final_cb) {
 
  bool can_retry = retry_count < 5; 
  if(!can_retry) 
    lg.err("failed to clear logid = %"PRId64" after 5 attempts. giving up.\n",
          cur_logid);
  
  if(r.code != storage::Result::SUCCESS && can_retry) {
    ++retry_count; 
    lg.dbg("retrying remove for logid %"PRId64", count = %d \n",
          cur_logid,retry_count); 
  }else { // move on to the next logid 
    ++cur_logid;
  }

  // only remove entries older than stop_logid
  if(cur_logid < stop_logid) { 
    Clear_log_callback cb = boost::bind(&User_Event_Log::internal_clear_cb,
                        this,_1,cur_logid,retry_count,stop_logid,final_cb); 
    // need to post, otherwise we'll just get the same concurrent 
    // modification again in the case of a retry 
    post(boost::bind(&User_Event_Log::remove, this, cur_logid, cb)); 
  } else {
    lg.dbg("finished removing entries with logid < %"PRId64"\n", stop_logid); 
    post(boost::bind(final_cb,r));  
  } 
} 

void User_Event_Log::internal_clear_finished() { } 

void User_Event_Log::check_excess_entries() {
  
  if(this->max_num_entries > 0) { 
    int64_t old_min_logid = this->min_existing_logid; 
    this->min_existing_logid = this->next_log_id - this->max_num_entries;
    if(this->min_existing_logid < 0) 
      this->min_existing_logid = 0; 
    if(old_min_logid != this->min_existing_logid 
        && this->min_existing_logid > 0) { 
      lg.dbg("removing all entries with %"PRId64" <= logid < %"PRId64"\n",
          old_min_logid, this->min_existing_logid); 
      Clear_log_callback final_cb = 
          boost::bind(&User_Event_Log::internal_clear_finished,this);
      Clear_log_callback fn = boost::bind(&User_Event_Log::internal_clear_cb,
          this,_1, old_min_logid,0,this->min_existing_logid,final_cb); 
      remove(old_min_logid, fn);  
    } 
  } 
  timeval tv = make_timeval(10,0); 
  post(boost::bind(&User_Event_Log::check_excess_entries,this), tv); 
} 

// note: this method does not retry if we run into an error.
// it is primarily for unit tests / debugging 
void User_Event_Log::clear(Clear_log_callback &cb) { 
      lg.dbg("clearing all user-event-log messages"); 
      Clear_log_callback remove_cb = 
            boost::bind(&User_Event_Log::internal_clear_cb,
            this,_1, this->min_existing_logid,0, this->next_log_id,cb); 
      remove(this->min_existing_logid, remove_cb); 
} 



void
User_Event_Log::getInstance(const container::Context* ctxt,
                           User_Event_Log*& h) {
    h = dynamic_cast<User_Event_Log*>
        (ctxt->get_by_interface(container::Interface_description
                                (typeid(User_Event_Log).name())));
}


REGISTER_COMPONENT(container::Simple_component_factory<User_Event_Log>, 
                   User_Event_Log);

}
} 
