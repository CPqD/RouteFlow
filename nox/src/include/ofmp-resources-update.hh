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
#ifndef OFMP_RESOURCES_UPDATE_HH
#define OFMP_RESOURCES_UPDATE_HH 1

#include "cfg.hh"
#include "event.hh"
#include "openflow/openflow-mgmt.h"

namespace vigil {

/** \ingroup noxevents
 *
 * Ofmp_resources_update_events are thrown each time the resources update
 * message is sent to NOX.
 *
 */

struct Vif_details
{
    std::string vif_uuid;
    ethernetaddr vif_mac;
    std::string vm_uuid;
    std::string net_uuid;
};

struct Ofmp_resources_update_event
    : public Event
{
    Ofmp_resources_update_event(datapathid id_,
            const ofmp_resources_update *oru, int msg_len);

    static const Event_name static_get_name() {
        return "Ofmp_resources_update_event";
    }

    datapathid mgmt_id;

    // Mapping of datapath/management ids to name 
    std::map<datapathid,std::string> port_name_map;

    // These UUIDs should really be their own class
    // Mapping of management id to system UUID
    std::map<datapathid,std::string> mgmt_uuid_map;

    // Mapping of datapath id to list of network UUIDs
    std::map<datapathid,std::vector<std::string> > net_uuid_map;

    // Mapping of vif name to its details
    std::map<std::string,Vif_details> vif_details_map;

private:
    Ofmp_resources_update_event(const Ofmp_resources_update_event&);
    Ofmp_resources_update_event& operator=(const Ofmp_resources_update_event&);
};

inline
Ofmp_resources_update_event::Ofmp_resources_update_event(datapathid id_,
            const ofmp_resources_update *oru, int msg_len)
        : Event(static_get_name()), mgmt_id(id_) 
{
    int data_len;
    uint8_t *ptr = (uint8_t *)oru->data;

    data_len = msg_len - sizeof(*oru);
    if (data_len <= 0) {
        printf("xxx received bad ofmp resource reply: %d\n", data_len);
        return;
    }

    while (data_len >= sizeof(struct ofmp_tlv)) {
        struct ofmp_tlv *tlv = (struct ofmp_tlv *)ptr;
        size_t len = ntohs(tlv->len);

        if (tlv->type == htons(OFMPTSR_END)) {
            data_len -= sizeof(struct ofmp_tlv);

            if (data_len) {
                printf("xxx badly terminated resource tlv\n");
            }
            return;
        }

        /* xxx Design a better way to do this */
        switch (ntohs(tlv->type)) {
        case OFMPTSR_DP: {
            struct ofmptsr_dp *dp_tlv = (struct ofmptsr_dp *)ptr;
            if (len != sizeof(*dp_tlv)) {
                printf("xxx datapath resource tlv too short: %zu!\n", len);
                return;
            }

            dp_tlv->name[sizeof(dp_tlv->name)-1] = '\0';
            port_name_map.insert(
                    std::map<datapathid,std::string>::value_type(
                    datapathid(datapathid::from_net(dp_tlv->dp_id)),
                    std::string((const char *)dp_tlv->name)));
            break;
        }

        case OFMPTSR_DP_UUID: {
            struct ofmptsr_dp_uuid *dp_uuid_tlv;
            std::vector<std::string> networks; 

            dp_uuid_tlv = (struct ofmptsr_dp_uuid *)ptr;
            if (len < sizeof(*dp_uuid_tlv)) {
                printf("datapath uuid resource tlv too short: %zd", len);
                return;
            }

            if ((len - sizeof(*dp_uuid_tlv)) % OFMP_UUID_LEN != 0) {
                printf("datapath uuid bad length: %zd", len);
                return;
            }
            int n_uuid = (len - sizeof(*dp_uuid_tlv)) / OFMP_UUID_LEN;

            for (int i=0; i<n_uuid; i++) {
                networks.push_back(
                        std::string((const char *)ptr
                                +sizeof(*dp_uuid_tlv)+(i*OFMP_UUID_LEN),
                        OFMP_UUID_LEN));
            }
            net_uuid_map.insert(
                    std::map<datapathid,std::vector<std::string> >::value_type(
                    datapathid(datapathid::from_net(dp_uuid_tlv->dp_id)),
                    networks));
            break;
        }


        case OFMPTSR_MGMT_UUID: {
            struct ofmptsr_mgmt_uuid *mgmt_uuid_tlv;

            mgmt_uuid_tlv = (struct ofmptsr_mgmt_uuid *)ptr;
            if (len != sizeof(*mgmt_uuid_tlv)) {
                printf("mgmt uuid resource tlv too short: %zd", len);
                return;
            }

            mgmt_uuid_map.insert(
                    std::map<datapathid,std::string>::value_type(
                    datapathid(datapathid::from_net(mgmt_uuid_tlv->mgmt_id)),
                    std::string((const char *)mgmt_uuid_tlv->uuid, 
                            OFMP_UUID_LEN)));
            break;
        }

        case OFMPTSR_VIF: {
            struct ofmptsr_vif *vif_tlv;
            Vif_details details;

            vif_tlv = (struct ofmptsr_vif *)ptr;
            if (len != sizeof(*vif_tlv)) {
                printf("vif resource tlv too short: %zd", len);
                return;
            }

            details.vif_uuid = std::string((const char *)vif_tlv->vif_uuid, 
                    OFMP_UUID_LEN);
            details.vif_mac = ethernetaddr(ntohll(vif_tlv->vif_mac));
            details.vm_uuid = std::string((const char *)vif_tlv->vm_uuid, 
                    OFMP_UUID_LEN);
            details.net_uuid = std::string((const char *)vif_tlv->net_uuid, 
                    OFMP_UUID_LEN);

            vif_details_map.insert(
                    std::map<std::string,Vif_details>::value_type(
                    std::string((const char *)vif_tlv->name), details));
            break;
        }

        default:
            printf("xxx unknown resource tlv: %d\n", ntohs(tlv->type));
            break;
        }

        ptr += len;
        data_len -= len;
    }

    if (data_len) {
        printf("xxx resource tlv list ended abruptly\n");
    }
}

} // namespace vigil

#endif /* ofmp-resources-update.hh */
