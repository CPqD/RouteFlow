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
#include "deployer.hh"

#include <boost/foreach.hpp>
#include <sstream>
#include "json-util.hh"

using namespace std;
using namespace vigil;
using namespace vigil::container;

Deployer::~Deployer() { 

}

bool
Deployer::deploy(Kernel* kernel, const Component_name& name) {
    Component_name_context_map::iterator i = uninstalled_contexts.find(name);
    
    if (i == uninstalled_contexts.end()) {
        return false;
    }
    
    Component_context* ctxt = i->second;
    uninstalled_contexts.erase(i);
    
    kernel->install(ctxt, NOT_INSTALLED);
    return true;
}

const char*
Deployer::JSON_DESCRIPTION = "meta.json";

Deployer::Path_list
Deployer::scan(boost::filesystem::path p) {;
    using namespace boost::filesystem;

    Path_list description_files;

    if (!exists(p)) {
        return description_files;
    }

    directory_iterator end;
    for (directory_iterator j(p); j != end; ++j) {
        try {
            if (!is_directory(j->status()) && 
                j->path().leaf() == JSON_DESCRIPTION) {
                description_files.push_back(j->path());
                continue;
            }
            
            if (is_directory(j->status())) {
                Path_list result = scan(*j);
                description_files.insert(description_files.end(), 
                                         result.begin(), result.end());
            }
        } catch (...) {
            /* Ignore any directory browsing errors. */
        } 
    }

    return description_files;
}

Component_context_list
Deployer::get_contexts() const {
    Component_context_list l;

    for (Component_name_context_map::const_iterator i = 
             uninstalled_contexts.begin();
         i != uninstalled_contexts.end(); ++i) {
        l.push_back(i->second);
    }

    return l;
}

Component_configuration::Component_configuration() {
}

Component_configuration::Component_configuration(json_object* d,
                                            const Component_argument_list& args)
    : json_description(d), arguments(args)

{
    using namespace vigil::container;

    json_object* n = json::get_dict_value(d, "name");
    name = n->get_string(true);
    /*json_dict::iterator i;
    json_dict* jodict = (json_dict*) d->object;
    i = jodict->find("name");
    name = i->second->get_string(true);*/
    // TODO: parse keys
}

const string 
Component_configuration::get(const string& key) const {
    return kv[key];
}

const bool
Component_configuration::has(const string& key) const {
    return kv.find(key) != kv.end();
}

const list<string>
Component_configuration::keys() const {
    list<string> keys;
    
    for (hash_map<string, string>::iterator i = kv.begin(); 
         i != kv.end(); ++i) {
        keys.push_back(i->first);
    }
    
    return keys;
}

const Component_argument_list
Component_configuration::get_arguments() const {
    return arguments;
}


const hash_map<std::string,std::string> 
Component_configuration::get_arguments_list(char d1, char d2) const {
    hash_map<std::string,std::string> argmap;
       
    BOOST_FOREACH(const std::string& arg_str, arguments){
        std::stringstream args(arg_str);
	std::string arg;
	int argcount = 0;
	
	while (getline(args,arg, ',')) {
	    std::stringstream argsplit(arg);
	    std::string argid,argval,tmparg;
	    while (getline(argsplit, tmparg, '=')) {
	      switch (argcount) {
	      case 0:
		argid = tmparg;
		break;
	      case 1:
		argval = tmparg;
		break;
	      }
	      argcount++;
	    }
	    argmap.insert(std::make_pair(argid, argval));
	}
    }

    return argmap;
}
