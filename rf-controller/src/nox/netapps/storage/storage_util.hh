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
#ifndef __STORAGE_UTILS_HH_
#define __STORAGE_UTILS_HH_ 

#include <inttypes.h>
#include <sstream> 
#include <list> 
#include <string> 
#include "storage/storage.hh" 
#include <boost/bind.hpp>
#include <boost/function.hpp>

using namespace vigil::applications; 

// currently unused, but useful for debugging 
struct Row_String_Converter : public boost::static_visitor<> 
{
  mutable char buf[1024];//mutable b/c visitor patter requires const methods 
  
  void operator()(const int64_t& i) const { 
    snprintf(buf,1024,"%"PRId64, i); 
  } 
  void operator()(const std::string& s) const { 
    snprintf(buf,1024,"%s", s.c_str());  
  }
  void operator() (const storage::GUID& guid) const { 
    snprintf(buf,1024,"%s",guid.str().c_str()); 
  } 
  void operator() (const double &d) const { 
    snprintf(buf,1024,"%f", d);  
  } 
}; 

class invalid_column : public std::exception { 
  std::string err_msg; 
  public: 
    invalid_column(std::string e) : err_msg(e) {}
    ~invalid_column() throw() {}
    const char* what() const throw() { return err_msg.c_str(); } 
}; 

// predicate fn used for remove_all
static bool return_true(const storage::Row &row) { return true; }

struct Storage_Util { 
    
typedef boost::function<bool(const storage::Row &)> Remove_row_predicate;

static void print_row(const storage::Row &row) {
  Row_String_Converter converter; 

  storage::Row::const_iterator it = row.begin();
  for( ; it != row.end(); ++it) { 
    boost::apply_visitor(converter, it->second);
    printf("key = '%s' value = '%s' \n", it->first.c_str(), converter.buf);
  }
}

static std::string row_as_string(const storage::Row &row) {
  Row_String_Converter converter; 
  std::stringstream ss; 
  storage::Row::const_iterator it = row.begin();
  for( ; it != row.end(); ++it) { 
    boost::apply_visitor(converter, it->second);
    ss << "key = '" << it->first << "' value = '" << converter.buf << "'\n";
  }
  return ss.str(); 
}


template <class T> 
static inline T get_col_as_type(const storage::Row &row, std::string col_name) {
    storage::Row::const_iterator it = row.find(col_name); 
    if(it == row.end()) 
      throw invalid_column("no column with name '" + col_name + "'"); 
    else 
      return boost::get<T>(it->second); 
}

// WARNING: this function does not provide transactional semantics
// If it returns a failure, it is possible that some but not all of
// the rows were removed.  It is offered as a convenience function 
static void non_trans_remove_all(storage::Async_storage *store, 
            std::string table_name, storage::Query q, 
            storage::Async_storage::Remove_callback rcb) {

  boost::function<bool(const storage::Row &)> fn(&return_true); 
  non_trans_remove(store,table_name, q, rcb, fn); 
}

static void non_trans_remove(storage::Async_storage *store, 
            std::string table_name, storage::Query q, 
            storage::Async_storage::Remove_callback rcb,
            Remove_row_predicate pred) {

  store->get(table_name, q, boost::bind(&Storage_Util::remove_get_cb,
                                          store,_1,_2,_3, q, rcb,pred)); 
}



// helper functions for non_trans_remove_all
static void remove_get_cb(storage::Async_storage *store, 
                                const storage::Result &result, 
            const storage::Context &ctx, const storage::Row &row, 
              storage::Query &q, storage::Async_storage::Remove_callback rcb,
              Remove_row_predicate pred){
  if(result.code == storage::Result::NO_MORE_ROWS) { 
    rcb(storage::Result()); // this is normal exit, return success   
    return; 
  } else if(result.code != storage::Result::SUCCESS) { 
    rcb(result);
    return; 
  }
  if(pred(row)) {  
    store->remove(ctx, boost::bind(&Storage_Util::remove_remove_cb,
                                                store,_1,ctx, q, rcb,pred));
  } else {
    store->get_next(ctx, boost::bind(&Storage_Util::remove_get_cb,
                                          store,_1,_2,_3, q, rcb,pred)); 
  }
} 

// need to tack on a context and cb for this function, so it can invoke get_next
static void remove_remove_cb(storage::Async_storage *store, 
      const storage::Result &result, const storage::Context &ctx, 
      storage::Query &q, storage::Async_storage::Remove_callback rcb,
      Remove_row_predicate pred){
  if(result.code != storage::Result::SUCCESS) {
    rcb(result);
    return; 
  }
  store->get(ctx.table, q, boost::bind(&Storage_Util::remove_get_cb,
                                          store,_1,_2,_3, q, rcb,pred)); 
  //store->get_next(ctx, boost::bind(&Storage_Util::remove_get_cb,
  //                                        store,_1,_2,_3, q, rcb,pred)); 
} 

// helper to remove all duplicates from a list 
template <typename T> 
static void squash_list(std::list< T > &l) {
  if(l.size() <= 1) return; 

  l.sort(); 
  typename std::list< T >::iterator it = l.begin();
  T last_id = *it;
  ++it; 
  while(it != l.end()) {
    if(last_id == *it) {
      l.erase(it);
      if(l.size() == 1) return; 
      it = l.begin(); // restart to get valid iterator
    } 
    last_id = *it;
    ++it;
  } 
} 



}; 

#endif 
