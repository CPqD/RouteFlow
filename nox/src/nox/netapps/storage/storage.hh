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
#ifndef STORAGE_HH
#define STORAGE_HH 1

#include <list>
#include <string>

#include <boost/function.hpp>
#include <boost/variant.hpp>
#include <boost/variant/static_visitor.hpp>

#include "component.hh"
#include "hash_map.hh"

/* Current TODO list:
 * 
 * - Lazy table removal.
 */
namespace vigil {
namespace applications {
namespace storage {

/* Globally Unique (Row) Identifier. */
struct GUID {
    static const int GUID_SIZE = 20;

    GUID();
    GUID(uint8_t*);
    GUID(const int64_t);

    bool operator==(const GUID&) const;
    bool operator<(const GUID&) const;
    int64_t get_int() const;
    const std::string str() const;

    uint8_t guid[GUID_SIZE];

    static GUID get_max();
    /* Generate a random GUID */
    static GUID random();
};

/* Table definitions */
typedef std::string Table_name;
typedef std::string Column_name;
typedef std::string Index_name;

typedef std::list<Column_name> Column_list;

/* Note, the string values of columns are in UTF-8. */
typedef boost::variant<int64_t, std::string, double, GUID> Column_value;
typedef hash_map<Column_name, Column_value> Column_value_map;
typedef Column_value_map Column_definition_map;
typedef Column_value_map Query;
typedef Column_value_map Row;

struct Index {
    Index_name name;
    Column_list columns;
    
    bool operator==(const Column_value_map&) const;
};
    
typedef std::list<Index> Index_list;

/* Reference to a single version of a row. */
struct Reference {
    static const int ANY_VERSION = -1;

    int version;
    GUID guid;
    bool wildcard;    
    
    /* Constructs a wildcard reference matching to anything. */
    Reference();
    Reference(const int version, const GUID&, const bool wildcard=false);

    bool operator==(const Reference&) const;    
    bool operator<(const Reference&) const;

    const std::string str() const;
};

/* Context stores all state over operations. */
struct Context {
    Table_name table;
    
    Index_name index;
    Reference index_row;
    
    Reference initial_row;
    Reference current_row;
    
    Context();
    Context(const Table_name&);
    Context(const Context&);
};

/* Storage operation result */
struct Result {
    enum Code {
        SUCCESS = 0,              /* OK */
        NO_MORE_ROWS,             /* Iteration complete */
        CONCURRENT_MODIFICATION,  /* Respective indices/rows modified
                                     in the middle of an operation. */
        EXISTING_TABLE,           /* Table already exists. */
        NONEXISTING_TABLE,        /* Table doesn't exist. */
        INVALID_ROW_OR_QUERY,     /* Invalid row or query. */
        INVALID_CONTEXT,          /* Context and query do not match. */
        CONNECTION_ERROR,         /* Network connectivity error. */
        UNKNOWN_ERROR,            /* Uncategorized error. */
        UNINITIALIZED             /* Storage internal use only. */
    };
    
    Code code;
    
    /* Brief error description. */
    std::string message;
    
    Result(const Code = SUCCESS, const std::string& = "");
    bool operator==(const Result&) const;
    bool is_success() const;
};

enum Trigger_reason {
    INSERT = 0, /* Row was inserted */
    MODIFY,     /* Row was modified */
    REMOVE,     /* Row was removed */
};

struct Trigger_id {
    bool for_table;
    std::string ring;
    
    Reference ref;
    int64_t tid;
    
    Trigger_id();
    Trigger_id(const std::string&, const int);
    Trigger_id(const std::string&, const Reference&, const int);
    Trigger_id(const bool, const std::string&, const Reference&, const int);

    bool operator==(const Trigger_id&) const;
    bool operator<(const Trigger_id&) const;
};

typedef boost::function<void(const Trigger_id&, const Row&, 
                             const Trigger_reason)> Trigger_function;

/* A non-blocking read/write non-transactional multi-key-value storage. */
class Async_storage { 
public:
    typedef boost::function<void(const Result&, const Context&, const Row&)> 
    Get_callback;
    typedef boost::function<void(const Result&, const Trigger_id&)> 
    Put_trigger_callback;
    typedef boost::function<void(const Result&)> Remove_trigger_callback;
    typedef boost::function<void(const Result&)> Create_table_callback;
    typedef boost::function<void(const Result&)> Drop_table_callback;
    typedef boost::function<void(const Result&, const GUID&)> Put_callback;
    typedef boost::function<void(const Result&, const Context&)> 
    Modify_callback;
    typedef boost::function<void(const Result&)> Remove_callback;

    virtual ~Async_storage();

    static void getInstance(const container::Context*, Async_storage*&);

    /* Query for row(s). Note, if no row is found, the Result object
       will have a status code of NO_MORE_ROWS. */
    virtual void get(const Table_name&,
                     const Query&,
                     const Get_callback&) = 0; 

    /* Get the next row. Note, if no row is found, the Result object
       will have a status code of NO_MORE_ROWS. */
    virtual void get_next(const Context&,
                          const Get_callback&) = 0;

    /* Create a trigger for previous query results. */
    virtual void put_trigger(const Context&,
                             const Trigger_function&,
                             const Put_trigger_callback&) = 0; 

    /* Create a trigger for a table. */
    virtual void put_trigger(const Table_name&,
                             const bool sticky,
                             const Trigger_function&,
                             const Put_trigger_callback&) = 0; 

    /* Remove a trigger */
    virtual void remove_trigger(const Trigger_id&, 
                                const Remove_trigger_callback&) = 0;

    /**
     * Create a table.
     */
    virtual void create_table(const Table_name&,
                              const Column_definition_map&,
                              const Index_list&,
                              const Create_table_callback&) = 0; 

    virtual void drop_table(const Table_name&,
                            const Drop_table_callback&) = 0; 

    /* Insert a new row. */
    virtual void put(const Table_name&,
                     const Row&,
                     const Put_callback&) = 0; 

    /* Modify an existing row */
    virtual void modify(const Context&,
                        const Row&,
                        const Modify_callback&) = 0;

    /* Remove a row. Note that the context is invalid after the call,
       and a new one should be retrieved with get() if necessary. */
    virtual void remove(const Context&, 
                        const Remove_callback&) = 0;

    /* Log current table and index sizes */
    virtual void debug(const Table_name&) = 0;

};

enum Column_type { COLUMN_INT = 0, COLUMN_TEXT , COLUMN_DOUBLE, COLUMN_GUID };
enum Table_type { PERSISTENT = 0, NONPERSISTENT };

/* Convenience function */
GUID compute_sguid(const Column_list&, const Column_value_map&);

/* Compare two table definitions for equality */
bool is_equal(const Column_definition_map&, const Index_list&,
              const Column_definition_map&, const Index_list&);

/* Visitor translating the column type into a SQL type and integer
   used in the meta tables. */
struct Column_type_collector
    : public boost::static_visitor<> 
{
    void operator()(const int64_t&) const;
    void operator()(const std::string&) const;    
    void operator()(const double&) const;
    void operator()(const GUID&) const;

    mutable std::string sql_type;
    mutable int64_t type;
    mutable Column_value storage_type;
};

} // namespace storage
} // namespace applications
} // namespace vigil

/* Hash function template specializations */
#include "storage-hash.hh"

#endif
