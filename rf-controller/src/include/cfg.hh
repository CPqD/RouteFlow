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
#ifndef CFG_HH
#define CFG_HH 1

#include <map>
#include <set>
#include <stdint.h>
#include <string>

#include "sha1.hh"
#include "hash_set.hh"

namespace vigil {

class Cfg
{
public:
    Cfg() : dirty(false) { }

    static const int cookie_len = 20;

    void load(const uint8_t *data, size_t len);
    void print(const std::string &banner);

    std::string to_string();

    void get_cookie(uint8_t *cookie);

    bool is_dirty() { return dirty; }
    void set_dirty(bool v) { update_cookie(); dirty = v; }

    bool has_key(const std::string &key);
    void get_prefix_keys(const std::string& prefix,
                         hash_set<std::string>& keys) const;

    std::string get_string(int idx, const std::string &key);
    int get_int(int idx, const std::string &key);
    bool get_bool(int idx, const std::string &key);
    int get_vlan(int idx, const std::string &key);

    void set_string(const std::string &key, const std::string &value);
    void set_bool(const std::string &key, bool value);
    void set_int(const std::string &key, int value);
    void set_vlan(const std::string &key, int value);

    void add_entry(const std::string &key, const std::string &value);

    void del_entry(const std::string &key, const std::string &value);
    void del_entry(const std::string &key, bool value);
    void del_entry(const std::string &key, int value);


private:
    bool dirty;

    bool update_cookie();
    uint8_t cfg_cookie[SHA1::hash_size];

    std::string get_nth_value(int idx, const std::string &key);

    std::multimap<std::string,std::string> cfg;
};


} /* namespace vigil */


#endif /* CFG_HH */
