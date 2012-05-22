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
#ifndef FLOW_FN_MAP_HH
#define FLOW_FN_MAP_HH 1

#include "hash_map.hh"
#include "flow_in.hh"

/*
 * Holds string --> Flow_fn mappings.
 */

namespace vigil {

class Flow_fn_map
{
public:
    typedef boost::function<void(const Flow_in_event&)> Flow_fn;
    typedef boost::function<bool(const std::vector<std::string>&)> Validate_fn;
    typedef boost::function<Flow_fn(const std::vector<std::string>&)> Generate_fn;

    Flow_fn_map() {}
    ~Flow_fn_map() {}

    // returns false if fn cannot be registered (string key already exists).
    // assumes functions takes 0 string arguments
    bool register_function(const std::string&, const Flow_fn&);

    // returns false if fn cannot be registered (string key already exists)
    bool register_function(const std::string&, const Generate_fn&, const Validate_fn&);

    // returns false if function not present to remove
    bool remove_function(const std::string&);

    // returns an empty function if mapping does not exists, else the fn
    Flow_fn get_function(const std::string&,
                         const std::vector<std::string>& args) const;

    // returns true if arguments are valid for function
    bool valid_fn_args(const std::string&, const std::vector<std::string>&) const;

private:
    struct fn_entry {
        Generate_fn generate;
        Validate_fn validate;
    };

    hash_map<std::string, fn_entry> fns;
    Flow_fn empty;

    Flow_fn_map(const Flow_fn_map&);
    Flow_fn_map& operator=(const Flow_fn_map&);
};

} // namespace vigil

#endif
