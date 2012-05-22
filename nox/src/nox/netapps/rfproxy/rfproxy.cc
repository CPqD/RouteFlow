#include <stdint.h>
#include <linux/if_ether.h>
#include <boost/bind.hpp>
#include <queue>

#include "rfproxy.hh"
#include "assert.hh"
#include "component.hh"
#include "vlog.hh"
#include "packet-in.hh"
#include "datapath-join.hh"
#include "port-status.hh"
#include "datapath-leave.hh"
#include "buffer.hh"
#include "flow.hh"
#include "netinet++/ethernet.hh"
#include "packets.h"

#include "ipc/MongoIPC.h"
#include "ipc/RFProtocol.h"
#include "ipc/RFProtocolFactory.h"
#include "rftable/RFTable.h"
#include "types/IPAddress.h"
#include "types/MACAddress.h"
#include "openflow/rfofmsg.h"
#include "converter.h"
#include "defs.h"

namespace vigil {

static Vlog_module lg("rfproxy");


void rfproxy::configure(const Configuration* c)
{
    lg.dbg(" Configure called ");
}

void rfproxy::install()
{
    rftable = new RFTable(MONGO_ADDRESS,MONGO_DB_NAME,RF_TABLE_NAME);

    // Start the IPC
    ipc = new MongoIPCMessageService(MONGO_ADDRESS, MONGO_DB_NAME, RFPROXY_ID);
    factory = new RFProtocolFactory();
    ipc->listen(RFSERVER_RFPROXY_CHANNEL, factory, this, false);

    register_handler<Packet_in_event>
        (boost::bind(&rfproxy::handle_packet_in, this, _1));
    register_handler<Datapath_join_event>
        (boost::bind(&rfproxy::handle_datapath_join, this, _1));
    register_handler<Datapath_leave_event>
        (boost::bind(&rfproxy::handle_datapath_leave, this, _1));
}

void rfproxy::getInstance(const Context* c, rfproxy*& component)
{
    component = dynamic_cast<rfproxy*>
        (c->get_by_interface(container::Interface_description(
            typeid(rfproxy).name())));
}

bool rfproxy::process(const string &from, const string &to, const string &channel, IPCMessage& msg) {
    int type = msg.get_type();
    if (type == DATAPATH_CONFIG) {
        DatapathConfig *config = dynamic_cast<DatapathConfig*>(&msg);

        // Create flow install message
        uint64_t dp_id = config->get_dp_id();
        uint8_t *flow = create_config_msg((DATAPATH_CONFIG_OPERATION) config->get_operation_id());

		if (send_openflow_command(datapathid::from_host(dp_id), (ofp_header*) flow, true))
			VLOG_INFO(lg,
                "Error installing flow (config) to dp=0x%llx",
                config->get_dp_id());
		else
			VLOG_INFO(lg,
			    "Flow (config) was installed to dp=0x%llx",
			    config->get_dp_id());
    }
    else if (type == FLOW_MOD) {
        FlowMod* flowmsg = dynamic_cast<FlowMod*>(&msg);

        // Convert message data to raw format
        uint64_t dp_id = flowmsg->get_dp_id();
        uint32_t address = flowmsg->get_address().toUint32();
        uint32_t netmask = flowmsg->get_netmask().toCIDRMask();
        uint8_t src_hwaddress[IFHWADDRLEN];
        flowmsg->get_src_hwaddress().toArray(src_hwaddress);
        uint8_t dst_hwaddress[IFHWADDRLEN];
        flowmsg->get_dst_hwaddress().toArray(dst_hwaddress);
        uint32_t dst_port = (uint32_t) flowmsg->get_dst_port();

        MSG ofmsg;
        if (not flowmsg->get_is_removal())
            // Create flow install message
            ofmsg = create_flow_install_msg(address, netmask, src_hwaddress, dst_hwaddress, dst_port);
        else
            // Create flow removal message
            ofmsg = create_flow_remove_msg(address, netmask, src_hwaddress);

        // TODO: different messages for flow install and remove.
		if (send_openflow_command(datapathid::from_host(dp_id), (ofp_header*) ofmsg, true))
			VLOG_INFO(lg,
			    "Error installing flow to dp=0x%llx",
			    flowmsg->get_dp_id());
		else
			VLOG_INFO(lg,
			    "Flow was installed to dp=0x%llx",
			    flowmsg->get_dp_id());

	    free(ofmsg);
    }
    return true;
}

Disposition rfproxy::handle_datapath_join(const Event& e) {
    const Datapath_join_event& dj = assert_cast<const Datapath_join_event&> (e);
    DatapathJoin msg(dj.datapath_id.as_host(), dj.ports.size() - 1, dj.datapath_id.as_host() == RFVS_DPID);
    ipc->send(RFSERVER_RFPROXY_CHANNEL, RFSERVER_ID, msg);
    VLOG_INFO(lg,
        "Datapath join message sent to the server for dp=0x%llx",
        dj.datapath_id.as_host());
    
    // TODO: remove this code
    // Sending a stats request is necessary because the modified version of
    // switchstats.py relies on this behavior. Fix it and then remove this.
	ofp_stats_request* osr = NULL;
	size_t msize = sizeof(ofp_stats_request);
	boost::shared_array<uint8_t> raw_sr(new uint8_t[msize]);
	// Request switch description. Generates a desc_in event.
	osr = (ofp_stats_request*) raw_sr.get();
	osr->header.type = OFPT_STATS_REQUEST;
	osr->header.version = OFP_VERSION;
	osr->header.length = htons(msize);
	osr->header.xid = 0;
	osr->type = htons(OFPST_DESC);
	osr->flags = htons(0);
	// Send OFPT_STATS_REQUEST
	send_openflow_command(dj.datapath_id, &osr->header, true);
        
    return CONTINUE;
}

Disposition rfproxy::handle_datapath_leave(const Event& e) {
    const Datapath_leave_event& dl = assert_cast<const Datapath_leave_event&> (e);
    DatapathLeave dlm(dl.datapath_id.as_host());
    ipc->send(RFSERVER_RFPROXY_CHANNEL, RFSERVER_ID, dlm);
    VLOG_INFO(lg,
        "A datapath_leave message was sent to the server (dp=0x%llx)",
        dl.datapath_id.as_host());
    return CONTINUE;
}

Disposition rfproxy::handle_packet_in(const Event& e) {
    const Packet_in_event& pi = assert_cast<const Packet_in_event&> (e);
	Flow flow(pi.in_port, *pi.get_buffer());

	// Drop all LLDP packets.
	if (flow.dl_type == ethernet::LLDP) {
		return CONTINUE;
	}

    // We have a packet giving a network mapping coming from some VM
	if (flow.dl_type == htons(0x0A0A)) {
        // Extract the packet data
		Nonowning_buffer b(*pi.get_buffer());
		const eth_data* data = b.try_pull<eth_data> ();
        VLOG_INFO(lg,
            "Mapping packet from VM id=%lld ovsport=%d",
            data->vm_id,
            pi.in_port);

        // Create a map message and send it
        VMMap mapmsg;
        mapmsg.set_vm_id(data->vm_id);
        mapmsg.set_vm_port(data->vm_port);
        mapmsg.set_vs_id(pi.datapath_id.as_host());
        mapmsg.set_vs_port(pi.in_port);
        ipc->send(RFSERVER_RFPROXY_CHANNEL, RFSERVER_ID, mapmsg);

		return CONTINUE;
	}

	map<string, string> query;
    vector<RFEntry> results;
    query.clear();
	// We need to know from which plane the packet came: virtual or control
	query[VS_ID] = to_string<uint64_t>(pi.datapath_id.as_host());
    results = rftable->get_entries(query);

	// The packet came from the data plane
	if (results.empty()) {
	    query[VS_ID] = "*";
		query[DP_ID] = to_string<uint64_t>(pi.datapath_id.as_host());
	    // Query the mapping for the vs_id and port related to the dp id and port
	    query[DP_PORT] = to_string<uint16_t>(pi.in_port);
        results = rftable->get_entries(query);
        if (results.empty() || results.front().get_vm_id() == "") {
            VLOG_DBG(lg, "Datapath not associated with a VM");
            return CONTINUE;
        }
        else {
            uint64_t vs_id = string_to<uint64_t>(results.front().get_vs_id());
            uint16_t vs_port = string_to<uint16_t>(results.front().get_vs_port());
            
            // Deal with VLAN tags added by Pronto switches - OK
	        // But strip any VLAN headers before adding
	        // TODO: double tagging? ethernet checksum?
            if (flow.dl_vlan != htons(OFP_VLAN_NONE)) {
	            packet_data pkt;
	            bzero(&pkt.packet[0],sizeof(pkt.packet));
		        // Copy ETH header
		        memcpy(pkt.packet, pi.get_buffer()->data(), sizeof(eth_header));
		        // Update ETH_TYPE accordingly
		        *((uint16_t *)(&pkt.packet[6 + 6])) = flow.dl_type;
		        // Copy rest of the packet
		        memcpy(pkt.packet + sizeof(eth_header),
				       pi.get_buffer()->data() + sizeof(eth_header) + sizeof(vlan_header),
				       pi.total_len - (sizeof(eth_header) + sizeof(vlan_header)));
		        pkt.size = pi.total_len - sizeof(vlan_header);

		        Array_buffer buff(pkt.size);
		        memcpy(buff.data(),  pkt.packet, pkt.size);

                if (send_openflow_packet(
                        datapathid::from_host(vs_id),
                        (Buffer &) buff,
                        vs_port,
                        OFPP_NONE,
                        true))
                    VLOG_DBG(lg,
                        "Failed to send packet dl_type=%d to virtual plane through virtual switch id=0x%llx port=%d",
                        flow.dl_type,
                        vs_id,
                        vs_port);
                else
                    VLOG_DBG(lg,
                        "Sent packet dl_type=%d to virtual plane through virtual switch id=0x%llx port=%d",
                        flow.dl_type,
                        vs_id,
                        vs_port);
            }
            else {
                if (send_openflow_packet(
                        datapathid::from_host(vs_id),
                        *pi.get_buffer(),
                        vs_port,
                        OFPP_NONE,
                        true))
                    VLOG_DBG(lg,
                        "Failed to send packet dl_type=%d to dp=0x%llx port=%d",
                        flow.dl_type,
                        vs_id,
                        vs_port);
                else
                    VLOG_DBG(lg,
                        "Sent packet dl_type=%d to dp=0x%llx port=%d",
                        flow.dl_type,
                        vs_id,
                        vs_port);
            }
        }
    }
    // The packet came from the virtual plane
    else {
        query[VS_PORT] = to_string<uint16_t>(pi.in_port);
        results = rftable->get_entries(query);
        if (results.empty() || results.front().get_dp_id() == "") {
            VLOG_DBG(lg, "Datapath not associated with a VM");
            return CONTINUE;
        }
        else {
            uint64_t dp_id = string_to<uint64_t>(results.front().get_dp_id());
            uint16_t dp_port = string_to<uint64_t>(results.front().get_dp_port());
                    
            if (send_openflow_packet(
                datapathid::from_host(dp_id),
                *pi.get_buffer(),
                dp_port,
                OFPP_NONE,
                true))
                VLOG_DBG(lg,
                    "Failed to send packet (dl_type=%d) to dp=0x%llx port=%d",
                    flow.dl_type,
                    dp_id,
                    dp_port);
            else
                VLOG_DBG(lg,
                    "Sent packet (dl_type=%d) to dp=0x%llx port=%d",
                    flow.dl_type,
                    dp_id,
                    dp_port);
            }
    }
    return CONTINUE;
}

REGISTER_COMPONENT(Simple_component_factory<rfproxy>, rfproxy);
} // vigil namespace
