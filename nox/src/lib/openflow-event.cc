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
// --
// TODO:
//   This should really be moved to a class ...
// --
#include "openflow-event.hh"

#include <map>
#include <netinet/in.h>
#include <boost/bind.hpp>
#include <boost/static_assert.hpp>
#include <inttypes.h>
#include <cstddef>
#include "assert.hh"
#include "buffer.hh"
#include "datapath.hh"
#include "openflow/nicira-ext.h"
#include "packet-in.hh"
#include "port-status.hh"
#include "datapath-join.hh"
#include "echo-request.hh"
#include "flow-stats-in.hh"
#include "aggregate-stats-in.hh"
#include "desc-stats-in.hh"
#include "table-stats-in.hh"
#include "port-stats-in.hh"
#include "queue-stats-in.hh"
#include "queue-config-in.hh"
#include "error-event.hh"
#include "flow-removed.hh"
#include "ofmp-config-update.hh"
#include "ofmp-config-update-ack.hh"
#include "ofmp-resources-update.hh"
#include "barrier-reply.hh"
#include "openflow-msg-in.hh"
#include "openflow.hh"
#include "vlog.hh"

using namespace vigil;

namespace {

static Vlog_module lg("openflow-event");

Event* handle_nx_vendor(boost::shared_ptr<Openflow_connection> oconn,
                        const nicira_header *nh, std::auto_ptr<Buffer> buf);


Event*
handle_packet_in(boost::shared_ptr<Openflow_connection> oconn,
                 const ofp_packet_in* opi, std::auto_ptr<Buffer> packet)
{
    datapathid datapath_id = oconn->get_datapath_id();

    /* jump past packet ofp_header and get access to packet data
     * */
    packet->pull(offsetof(ofp_packet_in, data));
    lg.dbg("received packet-in event from %s (len:%zu)",
           datapath_id.string().c_str(), packet->size());
    return new Packet_in_event(datapath_id, opi, packet);
}

Event*
handle_flow_removed(boost::shared_ptr<Openflow_connection> oconn,
                    const ofp_flow_removed *ofr, std::auto_ptr<Buffer> buf)
{
    datapathid datapath_id = oconn->get_datapath_id();

    lg.dbg("received flow expired event from %s",
           datapath_id.string().c_str());
    return new Flow_removed_event(datapath_id, ofr, buf);
}

Event*
handle_queue_config_reply(boost::shared_ptr<Openflow_connection> oconn,
			  const ofp_queue_get_config_reply *oqgcr, 
			  std::auto_ptr<Buffer> buf)
{
    datapathid datapath_id = oconn->get_datapath_id();
    return new Queue_config_in_event(datapath_id, oqgcr, buf);
}

Event*
handle_port_status(boost::shared_ptr<Openflow_connection> oconn,
                   const ofp_port_status *ops, std::auto_ptr<Buffer> buf)
{
    datapathid datapath_id = oconn->get_datapath_id();

    lg.dbg("received port status event from %s",
           datapath_id.string().c_str());
    return new Port_status_event(datapath_id, ops, buf);
}

Event*
handle_features_reply(boost::shared_ptr<Openflow_connection> oconn,
                  const ofp_switch_features *osf, std::auto_ptr<Buffer>)
{
    datapathid datapath_id = oconn->get_datapath_id();

    lg.err("ignoring additional features reply event from %s", 
            datapath_id.string().c_str());
    return NULL;
}

Event*
handle_stats_queue_reply(boost::shared_ptr<Openflow_connection> oconn,
			 const ofp_stats_reply *osr, std::auto_ptr<Buffer> buf)
{
    datapathid datapath_id = oconn->get_datapath_id();
    return new Queue_stats_in_event(datapath_id, osr, buf);
}

Event*
handle_stats_aggregate_reply(boost::shared_ptr<Openflow_connection> oconn,
                             const ofp_stats_reply *osr, std::auto_ptr<Buffer> buf)
{
    int len = htons(osr->header.length);
    datapathid datapath_id = oconn->get_datapath_id();

    if ( (len - sizeof(ofp_stats_reply)) != sizeof(ofp_aggregate_stats_reply)){
        lg.err("handle_stats_aggregate_reply has invalid length %d", 
                len);
        return 0;
    }

    return new Aggregate_stats_in_event(datapath_id, osr, buf);
}

Event*
handle_stats_table_reply(boost::shared_ptr<Openflow_connection> oconn,
                         const ofp_stats_reply *osr, std::auto_ptr<Buffer> buf)
{
    int len = htons(osr->header.length);
    datapathid datapath_id = oconn->get_datapath_id();

    if ( (len - sizeof(ofp_stats_reply)) % sizeof(ofp_table_stats)){
        lg.err("handle_stats_table_reply has invalid length %d", 
                len);
        return 0;
    }

    Table_stats_in_event* tsie = new Table_stats_in_event(datapath_id, osr, buf);
    len -= sizeof(ofp_stats_reply);
    ofp_table_stats* ots = (struct ofp_table_stats*)osr->body;
    for (int i = 0; i < len / sizeof(ofp_table_stats); ++i){
        tsie->add_table(
                (int)(ots->table_id),
                ots->name,
                htonl(ots->max_entries),
                htonl(ots->active_count),
                htonll(ots->lookup_count),
                htonll(ots->matched_count));
        ots++;
    }

    return tsie;
}

Event*
handle_stats_port_reply(boost::shared_ptr<Openflow_connection> oconn,
                        const ofp_stats_reply *osr, std::auto_ptr<Buffer> buf)
{
    int len = htons(osr->header.length);
    datapathid datapath_id = oconn->get_datapath_id();

    if ( (len - sizeof(ofp_stats_reply)) % sizeof(ofp_port_stats)){
        lg.err("handle_stats_port_reply has invalid length %d", 
                len);
        return 0;
    }

    Port_stats_in_event* psie = new Port_stats_in_event(datapath_id, osr, buf);
    len -= sizeof(ofp_stats_reply);
    ofp_port_stats* ops = (struct ofp_port_stats*)osr->body;
    for (int i = 0; i < len / sizeof(ofp_port_stats); ++i){
        psie->add_port(ops);
        ops++;
    }

    return psie;
}

Event*
handle_stats_desc_reply(boost::shared_ptr<Openflow_connection> oconn,
                        const ofp_stats_reply *osr, std::auto_ptr<Buffer> buf)
{
    int len = htons(osr->header.length);
    datapathid datapath_id = oconn->get_datapath_id();

    if ( (len - sizeof(ofp_stats_reply)) != sizeof(ofp_desc_stats)){
        lg.err("handle_stats_desc_reply has invalid length %d", 
                len);
        return 0;
    }

    return new Desc_stats_in_event(datapath_id, osr, buf);
}

Event*
handle_stats_flow_reply(boost::shared_ptr<Openflow_connection> oconn,
                        const ofp_stats_reply *osr, std::auto_ptr<Buffer> buf)
{
    int len = htons(osr->header.length);
    datapathid datapath_id = oconn->get_datapath_id();

    len -= (int) sizeof *osr;
    if (len < 0) {
        lg.err("handle_stats_flow_reply has invalid length %d", len);
        return 0;
    }
    return new Flow_stats_in_event(datapath_id, osr, buf);
}

Event*
handle_stats_reply(boost::shared_ptr<Openflow_connection> oconn,
                   const ofp_stats_reply *osr, std::auto_ptr<Buffer> buf)
{
    datapathid datapath_id = oconn->get_datapath_id();

    lg.dbg("received stats reply from %s", datapath_id.string().c_str());

    switch(htons(osr->type)) {
        case OFPST_DESC:
            return handle_stats_desc_reply(oconn, osr, buf);
        case OFPST_TABLE:
            return handle_stats_table_reply(oconn, osr, buf);
        case OFPST_PORT:
            return handle_stats_port_reply(oconn, osr, buf);
        case OFPST_AGGREGATE:
            return handle_stats_aggregate_reply(oconn, osr, buf);
        case OFPST_FLOW:
            return handle_stats_flow_reply(oconn, osr, buf);
        case OFPST_QUEUE:
            return handle_stats_queue_reply(oconn, osr, buf);
        default:    
            lg.warn("unhandled reply type %d", htons(osr->type));
            return NULL; 
    }
}

Event*
handle_ofmp_config_update(boost::shared_ptr<Openflow_connection> oconn,
        const ofmp_config_update *ocu, std::auto_ptr<Buffer> buf)
{
    // This message comes from the management channel, so the 
    // connection's "datapath id" is the managment id.
    datapathid mgmt_id = oconn->get_datapath_id();

    if (buf->size() < sizeof(ofmp_config_update)) {
        lg.warn("ofmp config update packet missing header");
        return NULL;
    }

    return new Ofmp_config_update_event(mgmt_id, ocu, buf->size());
}

Event*
handle_ofmp_config_update_ack(boost::shared_ptr<Openflow_connection> oconn,
        const ofmp_config_update_ack *ocua, std::auto_ptr<Buffer> buf)
{
    // This message comes from the management channel, so the 
    // connection's "datapath id" is the managment id.
    datapathid mgmt_id = oconn->get_datapath_id();

    if (buf->size() < sizeof(ofmp_config_update_ack)) {
        lg.warn("ofmp config ack packet missing header");
        return NULL;
    }

    return new Ofmp_config_update_ack_event(mgmt_id, ocua, buf->size());
}

Event*
handle_ofmp_resources_update(boost::shared_ptr<Openflow_connection> oconn,
        const ofmp_resources_update *oru, std::auto_ptr<Buffer> buf)
{
    // This message comes from the management channel, so the 
    // connection's "datapath id" is the managment id.
    datapathid mgmt_id = oconn->get_datapath_id();

    if (buf->size() < sizeof(ofmp_resources_update)) {
        lg.warn("ofmp resources packet missing header");
        return NULL;
    }

    return new Ofmp_resources_update_event(mgmt_id, oru, buf->size());
}

Event*
handle_ofmp_extended_data(boost::shared_ptr<Openflow_connection> oconn,
        const ofmp_extended_data *oed, std::auto_ptr<Buffer> buf)
{
    int len = buf->size() - sizeof *oed;
    uint8_t *ptr;

    if (buf->size() <= sizeof *oed) {
        lg.warn("ofmp extended data missing header");
        return NULL;
    }

    oconn->ext_data_xid = oed->header.header.header.xid;

    ptr = oconn->ext_data_buffer->put(len);
    memcpy(ptr, oed->data, len);

    if (!(oed->flags & OFMPEDF_MORE_DATA)) {
        /* An embedded message must be greater than the size of an
         * OpenFlow message. */
        if (oconn->ext_data_buffer->size() < 65536) {
            lg.warn("Received short embedded message: %zd\n",
                    oconn->ext_data_buffer->size());
            return NULL;
        }

        /* Make sure that this is a management message and that there's
         * not an embedded extended data message. */
        ofmp_header *oh = oconn->ext_data_buffer->try_at<ofmp_header>(0);
        if ((oh->header.vendor != htonl(NX_VENDOR_ID))
                || (oh->header.subtype != htonl(NXT_MGMT))
                || (oh->type == htonl(OFMPT_EXTENDED_DATA))) {
            lg.warn("received bad embedded extended message\n");
            return NULL;
        }
        oh->header.header.xid = oconn->ext_data_xid;
        oh->header.header.length = 0;

        nicira_header *nh = (nicira_header *)oconn->ext_data_buffer->data();
        Event *event = handle_nx_vendor(oconn, nh, oconn->ext_data_buffer);
        oconn->ext_data_buffer.reset(new Array_buffer(0));

        return event;
    }

    return NULL;
}

Event*
handle_nx_vendor(boost::shared_ptr<Openflow_connection> oconn,
        const nicira_header *nh, std::auto_ptr<Buffer> buf)
{
    if (buf->size() < sizeof(struct nicira_header)) {
        lg.warn("nicira packet missing header");
        return NULL;
    }

    if (ntohl(nh->subtype) == NXT_MGMT) {
        if (buf->size() < sizeof(struct ofmp_header)) {
            lg.warn("ofmp packet missing header");
            return NULL;
        }
        const ofmp_header *oh = (const ofmp_header *)nh;

        /* Reset the extended data buffer if we're certain that this isn't a
         * continuation of an existing extended data message. */
        if (oconn->ext_data_xid != nh->header.xid) {
            if (oconn->ext_data_buffer->size() > 0) {
                oconn->ext_data_buffer.reset(new Array_buffer(0));
            }
        }

        if (oh->type == htons(OFMPT_CONFIG_UPDATE)) {
            return handle_ofmp_config_update(oconn, 
                    (const ofmp_config_update *)oh, buf);
        } else if (oh->type == htons(OFMPT_CONFIG_UPDATE_ACK)) {
            return handle_ofmp_config_update_ack(oconn, 
                    (const ofmp_config_update_ack *)oh, buf);
        } else if (oh->type == htons(OFMPT_RESOURCES_UPDATE)) {
            return handle_ofmp_resources_update(oconn, 
                    (const ofmp_resources_update *)oh, buf);
        } else if (oh->type == htons(OFMPT_ERROR)) {
            ofmp_error_msg *oem = buf->try_at<ofmp_error_msg>(0);
            lg.warn("received ofmp error with type %d and code %d\n",
                    ntohs(oem->type), ntohs(oem->code));
            return NULL;
        } else if (oh->type == htons(OFMPT_EXTENDED_DATA)) {
            return handle_ofmp_extended_data(oconn, 
                    (const ofmp_extended_data *)oh, buf);
        } else {
            lg.warn("unsupported ofmp type %d", ntohs(oh->type));
            return NULL;
        }
    } else {
        lg.warn("unsupported nicira packet type %d", ntohl(nh->subtype));
        return NULL;
    }
}

Event*
handle_vendor(boost::shared_ptr<Openflow_connection> oconn,
              const ofp_vendor_header *ovh, std::auto_ptr<Buffer> buf)
{
    datapathid datapath_id = oconn->get_datapath_id();

    lg.dbg("received vendor extension from %s", 
            datapath_id.string().c_str());

    // xxx Should this just generate a new event, and an app can listen
    // xxx and rethrow vendor-specific ones?

    switch (ntohl(ovh->vendor)) {
        case NX_VENDOR_ID:
            return handle_nx_vendor(oconn,
                    (const nicira_header *)ovh, buf);

        default:
            lg.warn("unknown vendor 0x%08x", htonl(ovh->vendor));
            return NULL; 
    }
}

Event*
handle_barrier_reply(boost::shared_ptr<Openflow_connection> oconn,
		     const ofp_header* oh, std::auto_ptr<Buffer> buf)
{
    datapathid datapath_id = oconn->get_datapath_id();
    lg.dbg("received barrier reply from %s",
            datapath_id.string().c_str());
    return new Barrier_reply_event(datapath_id, oh, buf);
}


Event*
handle_echo_request(boost::shared_ptr<Openflow_connection> oconn,
                    const ofp_header* oh, std::auto_ptr<Buffer> packet)
{
    datapathid datapath_id = oconn->get_datapath_id();

    packet->pull(sizeof(ofp_header));
    lg.dbg("received echo-request event from %s (len:%zu)",
           datapath_id.string().c_str(), packet->size());
    return new Echo_request_event(datapath_id, oh, packet);
}

Event*
handle_error(boost::shared_ptr<Openflow_connection> oconn,
             const ofp_error_msg *oem, std::auto_ptr<Buffer> packet)
{
    uint16_t type = ntohs(oem->type);
    uint16_t code = ntohs(oem->code);
    datapathid datapath_id = oconn->get_datapath_id();

    lg.err("received Openflow error packet from dpid=%s: "
           "type=%d, code=%d, %zu bytes of data\n",
           datapath_id.string().c_str(), type, code,
           packet->size() - offsetof(ofp_error_msg, data));
    return new Error_event(datapath_id, oem, packet);
}

template <class Packet>
Event*
handle_packet
(Event* (*handler)(boost::shared_ptr<Openflow_connection> oconn,
                              const Packet*, std::auto_ptr<Buffer>),
              boost::shared_ptr<Openflow_connection> oconn,
              const ofp_header* oh, std::auto_ptr<Buffer> packet,
              size_t min_size = sizeof(Packet))
{
    // Attempt to ensure that Packet is a OpenFlow packet format.
    BOOST_STATIC_ASSERT(offsetof(Packet, header.version) == 0);

    if (packet->size() < min_size) {
        lg.dbg("openflow packet too short");
    }

    const Packet* p = reinterpret_cast<const Packet*>(oh);
    return handler(oconn, p, packet);
}

} // null namespace

namespace vigil {

Event*
openflow_msg_to_event(boost::shared_ptr<Openflow_connection> oconn, 
		      std::auto_ptr<Buffer> p)
{
    if (p->size() < sizeof(struct ofp_header)) 
    {
      lg.warn("openflow packet missing header");
      return NULL;
    }

    const ofp_header* oh = &p->at<ofp_header>(0);
    if (oh->version != OFP_VERSION) 
    {
      lg.warn("bad openflow version %"PRIu8, oh->version);
      return NULL;
    }

    return new Openflow_msg_event(oconn->get_datapath_id(), oh, p);
}

Event*
openflow_packet_to_event(boost::shared_ptr<Openflow_connection> oconn, std::auto_ptr<Buffer> p)
{
    if (p->size() < sizeof(struct ofp_header)) {
        lg.warn("openflow packet missing header");
        return NULL;
    }

    const ofp_header* oh = &p->at<ofp_header>(0);
    if (oh->version != OFP_VERSION) {
        lg.warn("bad openflow version %"PRIu8, oh->version);
        return NULL;
    }

   
    switch (oh->type) {
    case OFPT_PACKET_IN:
        return handle_packet(handle_packet_in, oconn, oh, p);
    case OFPT_FLOW_REMOVED:
        return handle_packet(handle_flow_removed, oconn, oh, p);
    case OFPT_QUEUE_GET_CONFIG_REPLY:
        return handle_packet(handle_queue_config_reply, oconn, oh, p);
    case OFPT_PORT_STATUS:
        return handle_packet(handle_port_status, oconn, oh, p);
    case OFPT_FEATURES_REPLY:
        return handle_packet(handle_features_reply, oconn, oh, p);
    case OFPT_STATS_REPLY:
        return handle_packet(handle_stats_reply, oconn, oh, p);
    case OFPT_BARRIER_REPLY:
        return handle_barrier_reply(oconn, oh, p);
    case OFPT_ECHO_REQUEST:
        return handle_echo_request(oconn, oh, p);
    case OFPT_VENDOR:
        return handle_packet(handle_vendor, oconn, oh, p);
    case OFPT_ECHO_REPLY:
        return NULL;
        // TODO OFPT_CLOSE
    case OFPT_ERROR:
        return handle_packet(handle_error, oconn, oh, p);
    default:
        lg.err("unhandled openflow packet type %"PRIu8, oh->type);
        return NULL;
    }
}

} // namespace vigil
