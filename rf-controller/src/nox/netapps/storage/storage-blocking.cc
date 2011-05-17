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
#include "storage-blocking.hh"

#include <boost/bind.hpp>

using namespace vigil;
using namespace vigil::applications::storage;

Sync_storage::Sync_storage(Async_storage* storage_)
    : storage(storage_) { 

}

Sync_storage::~Sync_storage() {

}

Sync_storage::Get_result 
Sync_storage::get(const Table_name& table, const Query& query) const {
    Co_sema sem;
    Get_result result;
    storage->get(table, query, 
                 boost::bind(&Sync_storage::get_callback, this,
                             _1, _2,_3, &sem, &result));
    sem.down();
    return result;
}

void 
Sync_storage::get_callback(const Result& result, const Context& ctxt,
                           const Row& row, Co_sema* sem, 
                           Get_result* ret) const {
    *ret = Get_result(result, ctxt, row);
    sem->up();
}
    
Sync_storage::Get_result 
Sync_storage::get_next(const Context& ctxt) const {
    Co_sema sem;
    Get_result result;
    storage->get_next(ctxt, boost::bind(&Sync_storage::get_callback, 
                                        this, _1, _2,_3, &sem, &result));
    sem.down();
    return result;
}

/* Create a trigger for previous query results. */
const Sync_storage::Put_trigger_result 
Sync_storage::put_trigger(const Context& ctxt,
                          const Trigger_function& tf) const {
    Co_sema sem;
    Put_trigger_result result;
    storage->put_trigger(ctxt, tf,
                         boost::bind(&Sync_storage::
                                     put_trigger_callback, 
                                     this, _1, _2, &sem, &result));
    sem.down();
    return result;
}

void
Sync_storage::put_trigger_callback(const Result& result, 
                                   const Trigger_id& tid,
                                   Co_sema* sem, 
                                   Put_trigger_result* ret) const {
    *ret = Put_trigger_result(result, tid);
    sem->up();
}

const Sync_storage::Put_trigger_result 
Sync_storage::put_trigger(const Table_name& table,
                          const bool sticky,
                          const Trigger_function& tf) const {
    Co_sema sem;
    Put_trigger_result result;
    storage->put_trigger(table, sticky, tf,
                             boost::bind(&Sync_storage::
                                         put_trigger_callback, 
                                         this, _1, _2, &sem, &result));
    sem.down();
    return result;
}

const Result 
Sync_storage::remove_trigger(const Trigger_id& tid) const {
    Co_sema sem;
    Result result(Result::UNINITIALIZED);
    storage->remove_trigger(tid,
                            boost::bind(&Sync_storage::callback, 
                                        this, _1, &sem, &result));
    sem.down();
    assert(result.code != Result::UNINITIALIZED);
    return result;
}
    
void
Sync_storage::callback(const Result& result, Co_sema* sem, 
                       Result* ret) const {
    *ret = result;
    sem->up();
}

const Result 
Sync_storage::create_table(const Table_name& table, 
                           const Column_definition_map& columns,
                           const Index_list& indices) const {
    Co_sema sem;
    Result result;
    storage->create_table(table, columns, indices,
                          boost::bind(&Sync_storage::callback, 
                                      this, _1, &sem, &result));
    sem.down();
    return result;
}

const Result 
Sync_storage::drop_table(const Table_name& table) const {
    Co_sema sem;
    Result result;
    storage->drop_table(table, boost::bind(&Sync_storage::callback, 
                                           this, _1, &sem, &result));
    sem.down();
    return result;
}

const Sync_storage::Put_result 
Sync_storage::put(const Table_name& table, const Row& row) const {
    Co_sema sem;
    Put_result result;
    storage->put(table, row, 
                 boost::bind(&Sync_storage::put_callback, 
                             this, _1, _2, &sem, &result));
    sem.down();
    return result;
}

void 
Sync_storage::put_callback(const Result& result, const GUID& guid,
                           Co_sema* sem, Put_result* ret) const {
    *ret = Put_result(result, guid);
    sem->up();
}

const Sync_storage::Modify_result
Sync_storage::modify(const Context& ctxt, const Row& row) const {
    Co_sema sem;
    Modify_result result;
    storage->modify(ctxt, row,
                    boost::bind(&Sync_storage::modify_callback, 
                                this, _1, _2, &sem, &result));
    sem.down();
    return result;
}

void 
Sync_storage::modify_callback(const Result& result, const Context& ctxt,
                              Co_sema* sem, Modify_result* ret) const {
    *ret = Modify_result(result, ctxt);
    sem->up();
}

const Result
Sync_storage::remove(const Context& ctxt) const {
    Co_sema sem;
    Result result;
    storage->remove(ctxt, boost::bind(&Sync_storage::callback, 
                                      this, _1, &sem, &result));
    sem.down();
    return result;
}
