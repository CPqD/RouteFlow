/* Copyright 2009 (C) Nicira, Inc.
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

#include <iostream>
#include <string.h>

#include "cfg.hh"
#include <boost/lexical_cast.hpp>

namespace vigil {

static std::pair<std::string,std::string>
parse_entry(std::string &entry)
{
    std::string::size_type pos = entry.find('=');
    if (pos == std::string::npos) {
        return std::make_pair("", "");
    }

    std::string key = entry.substr(0, pos);
    std::string value = entry.substr(pos+1);

    return std::make_pair(key, value);
}

void
Cfg::load(const uint8_t *data, size_t len) 
{
    cfg.clear();

    std::string str((const char *)data, len);

    // xxx This should sanity-check input more.  
    
    std::string::size_type last_pos = str.find_first_not_of('\n', 0);
    std::string::size_type pos = str.find_first_of('\n', last_pos);

    while (pos != std::string::npos || last_pos != std::string::npos) {
        std::string entry = str.substr(last_pos, pos - last_pos);
        std::pair<std::string,std::string> key_value = parse_entry(entry);

        if (key_value.first != "") {
            cfg.insert(key_value);
        }

        last_pos = str.find_first_not_of('\n', pos);
        pos = str.find_first_of('\n', last_pos);
    }

    update_cookie();
    dirty = false;
}

std::string
Cfg::to_string()
{
    std::multimap<std::string,std::string>::const_iterator it;
    std::set<std::string>::const_iterator kit;
    std::set<std::string> key_values;
    std::string str;

    for(it=cfg.begin(); it!=cfg.end(); it++) {
        key_values.insert((*it).first + "=" + (*it).second + "\n");
    }

    for (kit=key_values.begin(); kit!=key_values.end(); ++kit) {
        str += *kit;
    }

    return str;
}

bool
Cfg::has_key(const std::string &key)
{
    return cfg.find(key) != cfg.end() ? true : false;
}

void
Cfg::get_prefix_keys(const std::string& prefix,
                     hash_set<std::string>& keys) const
{
    std::multimap<std::string,std::string>::const_iterator it;

    keys.clear();

    for(it = cfg.begin(); it != cfg.end(); ++it) {
        if (it->first.find(prefix) == 0) {
            keys.insert(it->first);
        }
    }
}

bool
Cfg::update_cookie()
{
    SHA1 sha1;

    sha1.input(to_string());
    return sha1.digest(cfg_cookie);
}

void
Cfg::get_cookie(uint8_t *cookie)
{
    if (dirty) {
        update_cookie();
    }

    memcpy(cookie, cfg_cookie, sizeof(cfg_cookie));
}

void
Cfg::set_string(const std::string &key, const std::string &value)
{
    add_entry(key, value);
}

void
Cfg::set_int(const std::string &key, int value)
{
    add_entry(key, boost::lexical_cast<std::string>(value));
}

void
Cfg::set_bool(const std::string &key, bool value)
{
    add_entry(key, value ? "true" : "false");
}

void
Cfg::set_vlan(const std::string &key, int value)
{
    if (value < 0 || value > 4095) {
        return;
    }
    add_entry(key, boost::lexical_cast<std::string>(value));
}

void
Cfg::add_entry(const std::string &key, const std::string &value)
{
    cfg.insert(make_pair(key,value));
    dirty = true;
}

void
Cfg::del_entry(const std::string &key, const std::string &value)
{
    std::multimap<std::string,std::string>::iterator it;
    std::pair<std::multimap<std::string,std::string>::iterator,
        std::multimap<std::string,std::string>::iterator> ret;

    ret = cfg.equal_range(key);
    for (it=ret.first; it!=ret.second; ++it) {
        if ((*it).second == value) {
            cfg.erase(it);
            dirty = true;
            return;
        }
    }
}

void del_entry(const std::string &key, bool value) {
    del_entry(key, value ? "true" : "false");
}

void
Cfg::del_entry(const std::string &key, int value) {
    del_entry(key, boost::lexical_cast<std::string>(value));
}

void
Cfg::print(const std::string &banner)
{
    std::multimap<std::string,std::string>::iterator it;

    std::cout << banner << std::endl;

    for (it = cfg.begin(); it != cfg.end(); ++it) {
        std::cout << (*it).first << "=" << (*it).second << std::endl;
    }
}

std::string 
Cfg::get_nth_value(int idx, const std::string &key)
{
    std::multimap<std::string,std::string>::iterator it;
    std::pair<std::multimap<std::string,std::string>::iterator,
        std::multimap<std::string,std::string>::iterator> ret;

    ret = cfg.equal_range(key);
    int i=0;
    for (it=ret.first; it!=ret.second; ++it) {
        if (i == idx) {
            return (*it).second;
        }
        ++i;
    }

    return std::string("");
}

std::string
Cfg::get_string(int idx, const std::string &key)
{
    return get_nth_value(idx, key);
}

bool
Cfg::get_bool(int idx, const std::string &key)
{
    std::string value = get_nth_value(idx, key);
    return value == "true" ? true : false;
}

int
Cfg::get_int(int idx, const std::string &key)
{
    std::string value = get_nth_value(idx, key);
    try {
        return boost::lexical_cast<int>(value);
    }
    catch (boost::bad_lexical_cast &) {
        return 0;
    }
}

int
Cfg::get_vlan(int idx, const std::string &key)
{
    int vlan_id;

    std::string value = get_nth_value(idx, key);
    try {
        vlan_id = boost::lexical_cast<int>(value);
    }
    catch (boost::bad_lexical_cast &) {
        vlan_id = -1;
    }

    return vlan_id < 0 || vlan_id > 4095 ? -1 : vlan_id;
}

} /* namespace vigil */
