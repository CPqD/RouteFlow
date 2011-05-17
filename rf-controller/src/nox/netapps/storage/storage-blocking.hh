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
#ifndef STORAGE_BLOCKING_HH
#define STORAGE_BLOCKING_HH 1

#include <boost/tuple/tuple.hpp>

#include "storage.hh"
#include "threads/cooperative.hh"

namespace vigil {
namespace applications {
namespace storage {

/* A blocking wrapper for the non-transactional multi-key-value
 * storage.
 */
class Sync_storage {
public:
    typedef boost::tuple<Result, Context, Row> Get_result;
    typedef boost::tuple<Result, Trigger_id> Put_trigger_result;
    typedef boost::tuple<Result, GUID> Put_result;
    typedef boost::tuple<Result, Context> Modify_result;

    Sync_storage(Async_storage*);
    virtual ~Sync_storage();

    Get_result get(const Table_name&, const Query&) const;
    Get_result get_next(const Context&) const;
    const Put_trigger_result put_trigger(const Context&,
                                         const Trigger_function&) const;
    const Put_trigger_result put_trigger(const Table_name&,
                                         const bool sticky,
                                         const Trigger_function&) const;
    const Result remove_trigger(const Trigger_id&) const;
    const Result create_table(const Table_name&, const Column_definition_map&,
                              const Index_list&) const;
    const Result drop_table(const Table_name&) const;
    const Put_result put(const Table_name&, const Row&) const;
    const Modify_result modify(const Context&, const Row&) const;
    const Result remove(const Context&) const;

private:
    void callback(const Result&, Co_sema*, Result*) const;
    void get_callback(const Result&, const Context&, const Row&,
                      Co_sema*, Get_result*) const;
    void put_trigger_callback(const Result&, const Trigger_id&,
                              Co_sema*, Put_trigger_result*) const;
    void put_callback(const Result&, const GUID&, Co_sema*, Put_result*) const;
    void modify_callback(const Result&, const Context&, Co_sema*,
                         Modify_result*) const;

    Async_storage* storage;
};

} // namespace storage
} // namespace applications
} // namespace vigil

#endif
