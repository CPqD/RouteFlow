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
#ifndef SWITCH_MGR_HH
#define SWITCH_MGR_HH 1


#include <boost/bind.hpp>

#include "cfg.hh"
#include "component.hh"
#include "netinet++/datapathid.hh"
#include "ofmp-config-update.hh"
#include "ofmp-config-update-ack.hh"
#include "ofmp-resources-update.hh"

/*
 * Usage: Methods read/make changes to local copy of switch conf file.  Changes
 * are only pushed on calls to commit().  User should not give up the processor
 * during a chain of modifications as other applications will be modifying (and
 * possibly committing) the same local conf file.  Thus changes should be
 * committed/reverted before giving up control.
 *
 * commit() takes a callback as an argument that will be called with a boolean
 * signaling if the changes were successfully pushed down to the switch.
 * Successive commits taking place before the receipt of an ACK are made on top
 * of each other, meaning that later commits will fail if an earlier commit
 * does.
 */

namespace vigil {

class Switch_mgr
{
public:
    Switch_mgr(datapathid id) : mgmt_id(id), xid(0) {}

#if 0
    modify();
    bool add_entry(std::string &key, std::string &val);
#endif

    void revert() {
        local_cfg = last_commit;
    }

    void print_local_cfg(std::string& cfg_str) {
        local_cfg.print(cfg_str);
    }

    int commit(boost::function<void(bool)>&);

    // just says if local_cfg doesn't match global, not whether or not a commit
    // is needed because past commits may not have been ack-ed yet - need to
    // change?
    bool is_dirty() { return local_cfg.is_dirty(); }

    bool has_key(const std::string &key) {
        return local_cfg.has_key(key);
    }

    void get_prefix_keys(const std::string& prefix,
                         hash_set<std::string>& keys) const {
        local_cfg.get_prefix_keys(prefix, keys);
    }

    int get_int(int idx, const std::string &key) {
        return local_cfg.get_int(idx, key);
    };
    bool get_bool(int idx, const std::string &key) {
        return local_cfg.get_bool(idx, key);
    };
    std::string get_string(int idx, const std::string &key) {
        return local_cfg.get_string(idx, key);
    };
    int get_vlan(int idx, const std::string &key) {
        return local_cfg.get_vlan(idx, key);
    };

    void set_int(const std::string &key, int value) {
        local_cfg.set_int(key, value);
    };
    void set_bool(const std::string &key, bool value) {
        local_cfg.set_bool(key, value);
    };
    void set_string(const std::string &key, const std::string &value) {
        local_cfg.set_string(key, value);
    };
    void set_vlan(const std::string &key, int value) {
        local_cfg.set_vlan(key, value);
    };

    void del_entry(const std::string &key, bool value) {
        local_cfg.del_entry(key, value);
    }
    void del_entry(const std::string &key, int value) {
        local_cfg.del_entry(key, value);
    }
    void del_entry(const std::string &key, const std::string &value) {
        local_cfg.del_entry(key, value);
    }

    void set_capabilities(Cfg &cap) { capabilities = cap; }
    void set_config(const Cfg &cfg);

    bool port_is_virtual(const std::string &name);

    void config_ack_handler(const Ofmp_config_update_ack_event& ocua);
    void resources_update_handler(const Ofmp_resources_update_event& oru);

    void set_port_name_map(std::map<datapathid,std::string>m) {
        port_name_map = m;
    }

    const std::map<datapathid, std::string>& get_port_name_map() const {
        return port_name_map;
    }

    bool get_port_name(datapathid dpid, std::string& port_name) {
        std::map<datapathid,std::string>::iterator it;
        it = port_name_map.find(dpid);
        if (it == port_name_map.end()) {
            return false;
        }
        port_name = it->second;
        return true;
    }
    bool get_network_uuids(datapathid dpid, std::vector<std::string>& nets) {
        std::map<datapathid,std::vector<std::string> >::iterator it;
        it = net_uuid_map.find(dpid);
        if (it == net_uuid_map.end()) {
            return false;
        }
        nets = it->second;
        return true;
    }

    bool get_system_uuid(datapathid mgmt_id, std::string& sys_uuid) {
        std::map<datapathid,std::string>::iterator it;
        it = mgmt_uuid_map.find(mgmt_id);
        if (it == mgmt_uuid_map.end()) {
            return false;
        }
        sys_uuid = it->second;
        return true;
    }

    bool get_vif_details(std::string vif_name, Vif_details& details) {
        std::map<std::string,Vif_details>::iterator it;
        it = vif_details_map.find(vif_name);
        if (it == vif_details_map.end()) {
            return false;
        }
        details = it->second;
        return true;
    }

private:
    datapathid mgmt_id;

    // Mapping of datapath/management ids to name
    std::map<datapathid,std::string> port_name_map;

    // Mapping of management id to system UUID
    std::map<datapathid,std::string> mgmt_uuid_map;

    // Mapping of datapath id to list of network UUIDs
    std::map<datapathid,std::vector<std::string> > net_uuid_map;

    // Mapping of vif name to its details
    std::map<std::string,Vif_details> vif_details_map;

    Cfg global_cfg;
    uint8_t global_cookie[Cfg::cookie_len];

    Cfg local_cfg;
    Cfg last_commit;

    Cfg capabilities;

    struct CommitInfo {
        Cfg cfg;
        uint8_t cookie[Cfg::cookie_len];
        uint8_t old_cookie[Cfg::cookie_len];
        boost::function<void(bool)> callback;
    };
    typedef hash_map<uint32_t, CommitInfo> CommitMap;

    uint32_t xid;
    CommitMap commits;

    int send_ofmp_msg(ofmp_header *oh, size_t len);
};

} /* namespace vigil */

#endif /* SWITCH_MGR_HH */
