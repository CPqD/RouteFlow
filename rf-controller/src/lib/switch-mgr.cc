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


#include "assert.hh"
#include "nox.hh"
#include "openflow.hh"
#include "openflow/openflow-mgmt.h"
#include "switch-mgr.hh"
#include "vlog.hh"

#ifndef MIN
#define MIN(x,y) ((x) > (y) ? (y) : (x))
#endif

namespace vigil {

static Vlog_module lg("switch_mgr");

// if fails, caller should call "revert" to clear local changes if desired

int
Switch_mgr::send_ofmp_msg(ofmp_header *oh, size_t len)
{
    if (len < 65535) {
        return nox::send_openflow_command(mgmt_id,
                &oh->header.header, false);
    } else {
        struct ofmp_header *header = (struct ofmp_header *)oh;
        uint32_t xid = header->header.header.xid;
        size_t remain = len;
        uint8_t *ptr = (uint8_t *)oh;

        /* Mark the OpenFlow header with a zero length to indicate some
         * funkiness. 
         */
        header->header.header.length = 0;

        while (remain > 0) {
            uint8_t new_buffer[65536];
            struct ofmp_extended_data *oed = (ofmp_extended_data *)new_buffer;
            size_t new_len = MIN(65535 - sizeof *oed, remain);

            memset(new_buffer, '\0', 65536);

            oed->header.header.header.type = OFPT_VENDOR;
            oed->header.header.header.version = OFP_VERSION;
            oed->header.header.header.length = htons(new_len+sizeof *oed);
            oed->header.header.header.xid = xid;

            /* Nicira header */
            oed->header.header.vendor = htonl(NX_VENDOR_ID);
            oed->header.header.subtype = htonl(NXT_MGMT);

            /* OFMP header */
            oed->header.type = htons(OFMPT_EXTENDED_DATA);
            oed->type = header->type;
            if (remain > new_len) {
                oed->flags |= OFMPEDF_MORE_DATA;
            }

            /* Copy the entire original message, including the OpenFlow
             * header, since management protocol structure definitions
             * include these headers.
             */
            memcpy(oed->data, ptr, new_len);
            
            for (int i=0; i<10; ++i) {
                int retval = nox::send_openflow_command(mgmt_id,
                        &oed->header.header.header, false);
                if (retval == EAGAIN) {
                    usleep(100);
                    continue;
                } else if (retval) {
                    VLOG_WARN(lg, "send of extended data failed");
                    return retval;
                } else {
                    break;
                }
            }

            remain -= new_len;
            ptr += new_len;
        }
    }
    return 0;
}

int
Switch_mgr::commit(boost::function<void(bool)>& cb)
{
    std::auto_ptr<Buffer> buf;
    ofmp_config_update *ocu;

    CommitMap::iterator commit_iter = commits.insert(
        std::make_pair(xid, CommitInfo())).first;
    CommitInfo& commit_info = commit_iter->second;

    if (xid == UINT32_MAX) {
        xid = 0;
    } else {
        ++xid;
    }

    commit_info.cfg = local_cfg;
    commit_info.callback = cb;
    commit_info.cfg.get_cookie(commit_info.cookie);
    last_commit.get_cookie(commit_info.old_cookie);

    std::string cfg_str = commit_info.cfg.to_string();
    size_t data_len = sizeof *ocu + cfg_str.length();
    buf.reset(new Array_buffer(data_len));
    ocu = (ofmp_config_update *)buf->data();

    /* OpenFlow header */
    ocu->header.header.header.type = OFPT_VENDOR;
    ocu->header.header.header.version = OFP_VERSION;
    ocu->header.header.header.length = htons(data_len);
    ocu->header.header.header.xid = htonl(commit_iter->first);

    /* Nicira header */
    ocu->header.header.vendor = htonl(NX_VENDOR_ID);
    ocu->header.header.subtype = htonl(NXT_MGMT);

    /* OFMP header */
    ocu->header.type = htons(OFMPT_CONFIG_UPDATE);

    ocu->format = htonl(OFMPCAF_SIMPLE);
    memcpy(ocu->cookie, commit_info.old_cookie, Cfg::cookie_len);

    memcpy(ocu->data, cfg_str.c_str(), cfg_str.length());

    int success =  send_ofmp_msg((ofmp_header *)ocu, data_len);
    if (success != 0) {
        commits.erase(commit_iter);
    } else {
        VLOG_DBG(lg, "Commiting change %"PRIu32" %s",
                 ntohl(ocu->header.header.header.xid),
                 cfg_str.c_str());
        last_commit = local_cfg;
    }

    return success;
}

void
Switch_mgr::set_config(const Cfg &cfg) {
    global_cfg = cfg;
    global_cfg.get_cookie(global_cookie);
    global_cfg.set_dirty(false); // just in case

    VLOG_DBG(lg, "%"PRIx64": setting new configuration",
             mgmt_id.as_host());

    if (!local_cfg.is_dirty()) {
        last_commit = local_cfg = global_cfg;
        return;
    }

    uint8_t cookie[Cfg::cookie_len];
    last_commit.get_cookie(cookie);
    if (!memcmp(global_cookie, cookie, Cfg::cookie_len)) {
        VLOG_DBG(lg, "%"PRIx64": last matches global", mgmt_id.as_host());
        local_cfg.get_cookie(cookie);
        if (!memcmp(global_cookie, cookie, Cfg::cookie_len)) {
            VLOG_DBG(lg, "%"PRIx64": local matches global", mgmt_id.as_host());
            local_cfg.set_dirty(false);
        } else {
            VLOG_DBG(lg, "%"PRIx64": local doesn't match global",
                     mgmt_id.as_host());
        }
    } else {
        VLOG_DBG(lg, "%"PRIx64": last doesn't match global", mgmt_id.as_host());
    }
}

void
Switch_mgr::config_ack_handler(const Ofmp_config_update_ack_event& ocua)
{
    VLOG_DBG(lg, "Processing config ack %"PRIu32".", ocua.xid);
    CommitMap::iterator commit_iter = commits.find(ocua.xid);
    if (commit_iter == commits.end()) {
        lg.warn("Commit %"PRIu32" not found.", ocua.xid);
        return;
    }

    CommitInfo& commit_info = commit_iter->second;

    if (ocua.success) {
        if (memcmp(commit_info.cookie, ocua.cookie, Cfg::cookie_len)) {
            lg.warn("XXX %"PRIx64": successful ack has mismatched cookie?!",
                    mgmt_id.as_host());
        } else if (memcmp(commit_info.old_cookie, global_cookie,
                          Cfg::cookie_len))
        {
            lg.warn("XXX %"PRIx64": expect a successful ack not matching global?",
                    mgmt_id.as_host());
        } else {
            set_config(commit_info.cfg);
        }
    } else {
        lg.dbg("%"PRIx64": update failed. reverting to global.",
               mgmt_id.as_host());
        last_commit = local_cfg = global_cfg;
    }

    if (!commit_info.callback.empty()) {
        commit_info.callback(ocua.success);
    }
    commits.erase(commit_iter);
}

void
Switch_mgr::resources_update_handler(const Ofmp_resources_update_event& oru)
{
    if (oru.port_name_map != port_name_map) {
        port_name_map = oru.port_name_map;
    }

    if (oru.mgmt_uuid_map != mgmt_uuid_map) {
        mgmt_uuid_map = oru.mgmt_uuid_map;
    }

    if (oru.net_uuid_map != net_uuid_map) {
        net_uuid_map = oru.net_uuid_map;
    }

    vif_details_map = oru.vif_details_map;
}

bool
Switch_mgr::port_is_virtual(const std::string &name)
{
    return name.find("vif") == 0 || name.find("tap") == 0 ? true : false;
}

} /* namespace vigil */
