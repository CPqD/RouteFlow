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
#ifndef JSON_HH
#define JSON_HH 1

#include <list>
#include <string>

#include <boost/shared_array.hpp>

#include "json_object.hh"

namespace vigil {
    namespace json {

        /*
         * JSON convenience functions component developers may find useful
         * while accessing a JSON object tree passed for them.
         */
         
        /* Load a JSON file on a json_object*. */
        json_object* load_document(const std::string& file);
        
        /* Get value from json dict given a key. */
        json_object* get_dict_value(const json_object* jo, std::string key);
    }
}

#endif
