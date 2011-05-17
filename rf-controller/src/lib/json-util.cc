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
#include "json-util.hh"
#include <fstream>
#include <iostream>
#include <string>

using namespace std;

namespace vigil {
    namespace json {
        json_object* load_document(const string& file) {
            
            // TODO: Add checks, return error etc
            ssize_t len;
            char* jsonstr;
            ifstream in;
            json_object* jo;
            
            // open file
            in.open(file.c_str());
            // get conf file len
            in.seekg(0, ios::end);
            len = in.tellg();
            in.seekg(0, ios::beg);
            // alloc, read string, close file
            jsonstr = new char [len+1];
            in.read(jsonstr,len);
            in.close();            
            jsonstr[len]='\0';
            // create and return json_object
            boost::shared_array<uint8_t> str;
            str.reset(new uint8_t[len+1]);
            memcpy((char *) str.get(),jsonstr, len);
            jo = new json_object(str.get(), len);
            delete(jsonstr);
            return jo;
        }
        
        json_object* get_dict_value(const json_object* jo, string key){
            //Make sure jo is a dictionary (add)
            json_dict::iterator di;
            json_dict* jodict = (json_dict*) jo->object;
            di = jodict->find(key);
            if (di==jodict->end()) {
                return NULL;
                }
            else {
                return di->second;
                }
        }
    }
}
