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
#ifndef OFMP_CONFIG_UPDATE_ACK_HH
#define OFMP_CONFIG_UPDATE_ACK_HH 1

#include "cfg.hh"
#include "event.hh"
#include "openflow/openflow-mgmt.h"

namespace vigil {

/** \ingroup noxevents
 *
 * Ofmp_config_update_ack_events are thrown each time a config update
 * ACK is sent to NOX.
 *
 */

struct Ofmp_config_update_ack_event
    : public Event
{
    Ofmp_config_update_ack_event(datapathid id_,
            const ofmp_config_update_ack *ocua, int msg_len);

    static const Event_name static_get_name() {
        return "Ofmp_config_update_ack_event";
    }

    datapathid mgmt_id;

    bool success;
    uint32_t format;
    uint32_t flags;
    uint32_t xid;
    uint8_t cookie[Cfg::cookie_len];

private:
    Ofmp_config_update_ack_event(const Ofmp_config_update_ack_event&);
    Ofmp_config_update_ack_event& operator=(const Ofmp_config_update_ack_event&);
};

inline
Ofmp_config_update_ack_event::Ofmp_config_update_ack_event(datapathid id_,
        const ofmp_config_update_ack *ocua, int msg_len)
    : Event(static_get_name()), mgmt_id(id_)
{
    format = ntohl(ocua->format);
    flags = ntohl(ocua->flags);
    xid = ntohl(ocua->header.header.header.xid);
    success = (flags & OFMPCUAF_SUCCESS) ? true : false;
    memcpy(cookie, ocua->cookie, Cfg::cookie_len);
}
    
} // namespace vigil

#endif /* ofmp-config-update-ack.hh */
