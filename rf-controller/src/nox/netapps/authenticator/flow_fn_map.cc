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
#include "flow_fn_map.hh"

#include <boost/bind.hpp>
#include "vlog.hh"

namespace vigil {

static Vlog_module lg("flow_fn_map");

static
Flow_fn_map::Flow_fn
gen_no_arg_fn(const std::vector<std::string>& args,
              const Flow_fn_map::Flow_fn& fn)
{
    return fn;
}

static
bool
valid_no_args(const std::vector<std::string>& args)
{
    return args.empty();
}

bool
Flow_fn_map::register_function(const std::string& key, const Flow_fn& fn)
{
    fn_entry value = { boost::bind(gen_no_arg_fn, _1, fn),
                       boost::bind(valid_no_args, _1) };
    return fns.insert(std::make_pair(key, value)).second;
}

bool
Flow_fn_map::register_function(const std::string& key,
                               const Generate_fn& generate,
                               const Validate_fn& validate)
{
    fn_entry value = { generate, validate };
    return fns.insert(std::make_pair(key, value)).second;
}

bool
Flow_fn_map::remove_function(const std::string& key)
{
    return fns.erase(key) != 0;
}

Flow_fn_map::Flow_fn
Flow_fn_map::get_function(const std::string& key,
                          const std::vector<std::string>& args) const
{
    hash_map<std::string, fn_entry>::const_iterator entry(fns.find(key));
    if (entry != fns.end()) {
        return entry->second.generate(args);
    }
    VLOG_WARN(lg, "No function for key %s.", key.c_str());
    return empty;
}

bool
Flow_fn_map::valid_fn_args(const std::string& key, const std::vector<std::string>& args) const
{
    hash_map<std::string, fn_entry>::const_iterator entry(fns.find(key));
    if (entry != fns.end()) {
        return entry->second.validate(args);
    }
    VLOG_WARN(lg, "No function for key %s.", key.c_str());
    return false;
}


} // namespace vigil
