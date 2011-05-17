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
#ifndef DHT_HH
#define DHT_HH 1 

#include <map>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "component.hh"
#include "threads/cooperative.hh"
#include "storage.hh"

namespace vigil {
namespace applications {
namespace storage {

typedef std::pair<Trigger_id, Trigger_function> Trigger_definition;
typedef boost::function<void(const Trigger_definition)> Index_updated_callback;

typedef std::map<Trigger_id, Trigger_function> Trigger_map;
typedef std::map<Reference, Trigger_definition> Reference_trigger_def_map;

typedef std::string DHT_name;

class DHT;
class Index_DHT;
class Content_DHT;

typedef boost::shared_ptr<DHT> DHT_ptr; 
typedef boost::shared_ptr<Content_DHT> Content_DHT_ptr; 
typedef boost::shared_ptr<Index_DHT> Index_DHT_ptr; 

typedef hash_map<DHT_name, std::pair<Index_DHT_ptr, GUID> > GUID_index_ring_map;

class DHT_entry {
public:
    typedef std::vector<Reference> References;

    
    DHT_entry();
    DHT_entry(const Reference&);
    virtual ~DHT_entry(); 
   
    void put_trigger(DHT_ptr&, const Reference&, const Trigger_function&, 
                     const Async_storage::Put_trigger_callback&);
    const Result remove_trigger(DHT_ptr&, const Trigger_id&);

    virtual bool empty() = 0;

protected:
    /* Entry identity */
    Reference id;
    
    /* Application-level triggers */
    Trigger_map nonsticky_triggers;

    /* Local trigger id assignment sequence */
    int next_tid;
};

class Content_DHT_entry 
    : public DHT_entry {
public:
    Content_DHT_entry() { }

    Content_DHT_entry(const Reference& id_, const Row& row_) : 
        DHT_entry(id_), row(row_) { }

    void get(const Content_DHT_ptr&, Context&, const Reference&, const bool,
             const Async_storage::Get_callback&) const;
    void get_next(const Content_DHT_ptr&, Context&, 
                  const Async_storage::Get_callback&) const;
    const Result put(Content_DHT_ptr&, const Context&, 
                     const GUID_index_ring_map&, const Row&);
    const Result modify(Content_DHT_ptr&, Context&, const GUID_index_ring_map&,
                        const Row&);
    const Result remove(Content_DHT_ptr&, const Reference&); 

    /* Callbacks from Index DHTs */
    void index_entry_updated_callback(const Reference&, const Index_name&,
                                      const GUID&, const Row&,
                                      const Trigger_definition&);
    bool empty();

private:
    /* Primary entry: row content */
    Row row;

    /* Index triggers */
    Trigger_map index_triggers;

    /* Index entries */
    GUID_index_ring_map sguids;
};

class Index_DHT_entry 
    : public DHT_entry {
public:
    Index_DHT_entry() { }

    Index_DHT_entry(const Reference& id_) 
        : DHT_entry(id_) { }

    void get(const Content_DHT_ptr&, Context&, 
             const Async_storage::Get_callback&) const;

    void get_next(const Content_DHT_ptr&, Context&, 
                  const Async_storage::Get_callback&) const;

    void put(Index_DHT_ptr&, const Reference&, const Row&, 
             const Trigger_function&, const Index_updated_callback&,
             const Trigger_reason);

    void modify(Index_DHT_ptr&, const Reference&, const Reference&,
                const Row&, const Row&, const Trigger_function&,
                const Index_updated_callback&, const Trigger_reason);

    const Result remove(Index_DHT_ptr&, const Reference&, const Row&,
                        const Trigger_reason);

    bool empty();

private:
    /* Index entry: references to primary entries */
    References refs;

    /* Primary entry set triggers */
    Reference_trigger_def_map primary_triggers;
};

class DHT {
public:
    DHT(const DHT_name&, const container::Component*);
    virtual ~DHT();

    Trigger_id trigger_instance(const Reference&, const int);
    
    virtual void put_trigger(DHT_ptr&, const Context&,
                             const Trigger_function&, 
                             const Async_storage::Put_trigger_callback&) = 0;

    virtual void remove_trigger(DHT_ptr&, const Trigger_id&,
                                const Async_storage::
                                Remove_trigger_callback&) = 0;

    const container::Component* dispatcher;
    
protected:
    const DHT_name name;         

    /* Mutex protecting the ring */
    mutable Co_mutex mutex;
};

class Content_DHT 
    : public std::map<GUID, Content_DHT_entry>,
      public DHT
{
public:
    Content_DHT(const DHT_name&, const container::Component*);

    void get(Content_DHT_ptr&, Context&, const Reference&, const bool,
             const Async_storage::Get_callback&) const;
    
    void get_next(Content_DHT_ptr&, Context&, 
                  const Async_storage::Get_callback&) const;

    void put(Content_DHT_ptr&, const GUID_index_ring_map&, const Row&, 
             const Async_storage::Put_callback&);
    
    void modify(Content_DHT_ptr&, const Context&, 
                const GUID_index_ring_map&, const Row&, 
                const Async_storage::Modify_callback&);

    void remove(Content_DHT_ptr&, const Context&,
                const Async_storage::Remove_callback&);

    void put_trigger(DHT_ptr&, const Context&,
                     const Trigger_function&, 
                     const Async_storage::Put_trigger_callback&);

    void put_trigger(const bool sticky, 
                     const Trigger_function&, 
                     const Async_storage::Put_trigger_callback&);
    
    void remove_trigger(DHT_ptr&, const Trigger_id&,
                        const Async_storage::Remove_trigger_callback&);

    void invoke_table_triggers(const Row&, const Trigger_reason);

    /* Callbacks from the Index DHTs */
    void index_entry_deleted_callback(Content_DHT_ptr&, const Reference&, 
                                      const Trigger_id&, const Row&);
    void index_entry_updated_callback(const Reference&, const Index_name&,
                                      const GUID&, const Row&,
                                      const Trigger_definition&);
protected:
    friend class Content_DHT_entry;

private:
    void internal_remove(Content_DHT_ptr&, const Reference&,
                         const Async_storage::Remove_callback&);

    Trigger_map sticky_table_triggers;
    Trigger_map nonsticky_table_triggers;
    uint64_t next_tid;
};

class Index_DHT 
    : public std::map<GUID, Index_DHT_entry>,
      public DHT
{
public:
    Index_DHT(const DHT_name&, const container::Component*);

    void get(const Content_DHT_ptr&, Context&, const Reference&, 
             const Async_storage::Get_callback&) const;
    
    void get_next(const Content_DHT_ptr&, Context&, 
                  const Async_storage::Get_callback&) const;

    /* Create a new reference to the primary row.
     *
     * Return a trigger to call as the primary row is
     * modified/deleted */
    void put(Index_DHT_ptr&, const GUID&, const Reference&, 
             const Row&,const Trigger_function&, const Index_updated_callback&,
             const Trigger_reason);
    
    void modify(Index_DHT_ptr&, const GUID&, const std::vector<Reference>&,
                const Row&, const Row&, const Trigger_function&, 
                const Index_updated_callback&, const Trigger_reason);
    
    void remove(Index_DHT_ptr&, const GUID&, const Reference&, const Row&, 
                const Index_updated_callback&, const Trigger_reason);

    void put_trigger(DHT_ptr&, const Context&, const Trigger_function&,
                     const Async_storage::Put_trigger_callback&);

    void remove_trigger(DHT_ptr&, const Trigger_id&,
                        const Async_storage::Remove_trigger_callback&);

    void primary_deleted_callback(Index_DHT_ptr&, const Trigger_id&, const Row&,
                                  const Row&, const GUID&, const Reference&);
protected:
    friend class Index_DHT_entry;
};

} // namespace storage
} // namespace applications
} // namespace vigil

#endif
