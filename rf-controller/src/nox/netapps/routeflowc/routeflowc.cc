/* Copyright 2011 Fundação CPqD.
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

#include <boost/bind.hpp>
#include <boost/shared_array.hpp>
#include <signal.h>
#include <string>
#include <vector>
#include <map>
#include <utility>
#include <queue>
#include "assert.hh"
#include "component.hh"
#include "vlog.hh"
#include "packet-in.hh"
#include "datapath-join.hh"
#include "port-status.hh"
#include "datapath-leave.hh"
#include "desc-stats-in.hh"
#include "buffer.hh"
#include "openflow-default.hh"
#include "timeval.hh"
#include "discovery/link-event.hh"
#include "flow.hh"
#include "packets.h"
#include "threads/cooperative.hh"
#include "threads/native.hh"
#include "netinet++/datapathid.hh"
#include "netinet++/ethernet.hh"
#include "cmd_msg.h"
#include "event_msg.h"
#include "IPCMessage.hh"
#include "RawMessage.hh"
#include <linux/if_ether.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

namespace {

using namespace vigil;
using namespace vigil::container;
using std::list;
using std::map;
using std::multimap;
using std::pair;
using std::string;
using std::queue;

Vlog_module lg("routeflow");

const uint32_t TCP_PORT = 7890;
const uint32_t MAX_CONNECTIONS = 1;
uint64_t pckt_xid = 0;


int sock_fd = 0;
int server_sock_fd = 0;



struct packet_data{

	uint8_t packet[2048];
	uint32_t size;

}__attribute__((packed));

hash_map<uint64_t, packet_data> pack_buffer;




/*
 * The struct eth_data stores information about VM which comes
 * over the MAP Events.
 */
struct eth_data {

	uint8_t eth_dst[ETH_ALEN]; /* Destiny MAC address. */
	uint8_t eth_src[ETH_ALEN]; /* Source MAC address. */
	uint16_t eth_type; /* Packet type. */
	uint64_t vm_id; /* Number which identifies a Virtual Machine .*/
	uint8_t vm_port; /* Number of the Virtual Machine port */

}__attribute__((packed));


class RouteFlowC: public Component {
	Native_thread server_thread;
	Co_thread send_cmd;
	uint16_t tcpport;
	queue<base_msg> command;
	uint8_t cmd_mutex;
	queue<base_msg> events;
	queue<int> ports_num;

public:
	RouteFlowC(const Context* c, const json_object*) :
		Component(c) {
	}

	Disposition handle_link_event(const Event&);

	Disposition handle_datapath_join(const Event& e);

	Disposition handle_datapath_leave(const Event& e);

	Disposition handle_packet_in(const Event& e);

	Disposition handle_desc_in(const Event& e);

	//void add_link(const Link_event& le);

	//void del_link(const Link_event& le);

	void send_command();

	void process_message(base_msg * msg);

	void server();

	void configure(const Configuration*);

	void install();



};


void RouteFlowC::send_command()
{
        timeval t;

        t.tv_sec = 0;
        t.tv_usec = 1000;

        while(1)
        {
		if (!cmd_mutex)
		{
			cmd_mutex = 1;
        	        while(command.size())
                	{
        	                process_message(&command.front());
                	        command.pop();
	                }
			cmd_mutex = 0;
		}
	        this->send_cmd.sleep(t);
        }
}

/*
 * Process rf-server command messages.
 */
void RouteFlowC::process_message(base_msg * msg) {

	if (msg->group == COMMAND) {
		switch (msg->type) {
		/*
		 * Install Flow message processing
		 */
		case FLOW: {
			ofp_flow_mod* ofm;
					flow flow_msg;
					bzero(&flow_msg, sizeof(flow));
					::memcpy(&flow_msg, &msg->payload, msg->pay_size);
					size_t size = msg->pay_size - sizeof(flow_msg.datapath_id);
					boost::shared_array<char> raw_of(new char[size]);
					ofm = (ofp_flow_mod*) raw_of.get();
					::memcpy(&ofm->header, &flow_msg.flow_mod, size);

					VLOG_INFO(lg, "Flow Command to datapath  with id %llx", flow_msg.datapath_id);
					if (send_openflow_command(datapathid::from_host(
							flow_msg.datapath_id), &ofm->header, true))
						VLOG_INFO(lg, "Couldn't send flow");
					else
						VLOG_INFO(lg, "Send flow");

					break;
				}

			/*
			 * Process packet messages.
			 */
		case SEND_PACKET: {

			send_packet pack_msg;
			bzero(&pack_msg, sizeof(send_packet));
			::memcpy(&pack_msg, &msg->payload, msg->pay_size);
			hash_map<uint64_t, packet_data>::const_iterator i;
			i = pack_buffer.find(pack_msg.pkt_id);

			/*Drops packet if there is not a datapath to send it. */
			if( !pack_msg.port_out){
				VLOG_DBG(lg, "Dropping packet");
				pack_buffer.erase(i);
				break;
			}
			if (i  != pack_buffer.end()){
				Array_buffer buff(i->second.size);
				memcpy(buff.data(),  i->second.packet, i->second.size);
				VLOG_DBG(lg, "datapath with id %llx, port number %d", pack_msg.datapath_id, pack_msg.port_out);
				if (send_openflow_packet(datapathid::from_host(pack_msg.datapath_id),
					(Buffer &) buff, pack_msg.port_out, OFPP_NONE, true) )
					VLOG_INFO(lg, "Failed to send packet to datapath with id %llx, port number %d", pack_msg.datapath_id, pack_msg.port_out);
				else VLOG_INFO(lg, "Sending packet to datapath with id %llx, port number %d", pack_msg.datapath_id, pack_msg.port_out);
				pack_buffer.erase(i);
			}

			break;
		}
		default:
			break;

	     }

	}

}

/*
 * Sends to the rf-server Link Event messages.
 *
Disposition RouteFlowC::handle_link_event(const Event& e) {
	const Link_event& le = assert_cast<const Link_event&> (e);

	base_msg base_message;
	link_event msg_event;

	base_message.group = EVENT;
	base_message.type = LINK_EVENT;

	msg_event.reason = le.action;
	msg_event.dp1 = le.dpsrc.as_host();
	msg_event.port_1 = le.sport;
	msg_event.dp2 = le.dpdst.as_host();
	msg_event.port_2 = le.dport;
	base_message.pay_size = sizeof(link_event);
	::memcpy(&base_message.payload, &msg_event, sizeof(link_event));

	if (server_sock_fd > 0) {

		if (send(server_sock_fd, &base_message, sizeof(base_msg), 0) == -1)
			perror("send");

	} else
		events.push(base_message);

	return CONTINUE;
}*/

/*
 * Sends Packet or Map Messages Event to the rf-server.
 */
Disposition RouteFlowC::handle_packet_in(const Event& e) {
	const Packet_in_event& pi = assert_cast<const Packet_in_event&> (e);

	Flow flow(pi.in_port, *pi.get_buffer());
	base_msg base_message;
	packet_in msg_event;

	/* drop all LLDP packets. */
	if (flow.dl_type == ethernet::LLDP) {
		return CONTINUE;
	}

	if (flow.dl_type == htons(0x0A0A)) {

		VmtoOvs map_msg;

		Nonowning_buffer b(*pi.get_buffer());
		const eth_data* data = b.try_pull<eth_data> ();

		base_message.group = EVENT;
		base_message.type = MAP_EVENT;

		VLOG_INFO(lg, "Mapping packet arrived from Virtual Machine  %d  ovsport%d", data->vm_port, pi.in_port);

		map_msg.OvsPort = pi.in_port;
		map_msg.VmPort = data->vm_port;
		map_msg.vmId = data->vm_id;
		base_message.pay_size = sizeof(map_msg);
		::memcpy(&base_message.payload, &map_msg, sizeof(map_msg));

		if (server_sock_fd > 0) {

			if (send(server_sock_fd, &base_message, sizeof(base_msg), 0) == -1)
				perror("send");

		} else
			events.push(base_message);
		return CONTINUE;
	}

	/* Mount the message.*/
	base_message.group = EVENT;
	base_message.type = PACKET_IN;

	msg_event.datapath_id = pi.datapath_id.as_host();
	msg_event.port_in = pi.in_port;
	msg_event.pkt_id = pckt_xid;
	msg_event.type = flow.dl_type;
	base_message.pay_size = sizeof(msg_event);
	::memcpy(&base_message.payload, &msg_event, sizeof(msg_event));

	/* Add packet to the buffer. */
	packet_data pkt;
	bzero(&pkt.packet[0],sizeof(pkt.packet));
	memcpy(&pkt.packet[0], pi.get_buffer()->data(), pi.total_len);
	pkt.size = pi.total_len;
	pack_buffer.insert(std::make_pair(pckt_xid, pkt));


	if (pckt_xid == UINT64_MAX)
		pckt_xid = 0;
	else
		pckt_xid++;

	if (server_sock_fd > 0) {

		if (send(server_sock_fd, &base_message, sizeof(base_message), 0) == -1){
			VLOG_INFO(lg,"packet -in ERROR SENDING MESSAGE");
			return CONTINUE;
		}

	} else
		events.push(base_message);

	return CONTINUE;
}

Disposition RouteFlowC::handle_desc_in(const Event& e) {
	const Desc_stats_in_event& ds = assert_cast<const Desc_stats_in_event&> (e);

	base_msg base_message;
	datapath_join msg_event;
	uint32_t size = sizeof(base_msg);

	/* Mount the message.*/
	bzero(&base_message, size);
	base_message.group = EVENT;
	base_message.type = DATAPATH_JOIN;

	msg_event.datapath_id = ds.datapath_id.as_host();
	msg_event.no_ports = ports_num.front();
	ports_num.pop();
	strcpy(msg_event.hw_desc, ds.hw_desc.c_str());
	base_message.pay_size = sizeof(msg_event);

	::memcpy(&base_message.payload, &msg_event, sizeof(msg_event));
	if (server_sock_fd > 0) {

		if (send(server_sock_fd, &base_message, sizeof(base_msg), 0) == -1)
			perror("send");
	} else
		events.push(base_message);

	VLOG_INFO(lg, "A new datapath has been registered: id=%llx",
			ds.datapath_id.as_host());

	return CONTINUE;
}

/*
 *	Sends, on a datapath join event, the switch datapath id and number of ports to the rf-server.
 */
Disposition RouteFlowC::handle_datapath_join(const Event& e) {
	const Datapath_join_event& dj = assert_cast<const Datapath_join_event&> (e);

	ofp_stats_request* osr = NULL;
	size_t msize = sizeof(ofp_stats_request);
	boost::shared_array<uint8_t> raw_sr(new uint8_t[msize]);

	// Send OFPT_STATS_REQUEST
	osr = (ofp_stats_request*) raw_sr.get();
	osr->header.type = OFPT_STATS_REQUEST;
	osr->header.version = OFP_VERSION;
	osr->header.length = htons(msize);
	osr->header.xid = 0;
	osr->type = htons(OFPST_DESC);
	osr->flags = htons(0);
	ports_num.push(dj.ports.size());

	send_openflow_command(dj.datapath_id, &osr->header, false);

	return CONTINUE;
}

/*
 * 	Sends the datapathid to the rf-server on a datapath leave event.
 */
Disposition RouteFlowC::handle_datapath_leave(const Event& e) {
	const Datapath_leave_event& dl = assert_cast<const Datapath_leave_event&> (
			e);

	base_msg base_message;
	datapath_leave msg_event;

	/* Mount the message.*/
	base_message.group = EVENT;
	base_message.type = DATAPATH_LEAVE;

	msg_event.datapath_id = dl.datapath_id.as_host();
	base_message.pay_size = sizeof(msg_event);
	::memcpy(&base_message.payload, &msg_event, sizeof(msg_event));
	VLOG_INFO(lg, "Datapath %llx has leaved", dl.datapath_id.as_host() );
	if (server_sock_fd > 0) {

		if (send(server_sock_fd, &base_message, sizeof(base_msg), 0) == -1)
			perror("send");

	} else
		events.push(base_message);

	return CONTINUE;
}

/*
 * Connects with the rf-server and stay listening for
 * server messages.
 */
void RouteFlowC::server() {

	timeval t;
	int conn_count;
	socklen_t cli_lenght;
	struct sockaddr_in server_addr;

	t.tv_sec = 0;
	t.tv_usec = 1000;

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		VLOG_ERR(lg, "Could not open a new socket");
		return;
	}

	std::memset((char *) &server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(tcpport);
	if (bind(sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr))
			< 0) {
		close(sock_fd);
		VLOG_ERR(lg, "Could not bind server socket");
		return;
	} else

		listen(sock_fd, MAX_CONNECTIONS);

	cli_lenght = sizeof(struct sockaddr_in);
	conn_count = 0;
	VLOG_INFO(lg, "Waiting for server connection");

	while (1) {
		struct sockaddr_in vm_addr;
		server_sock_fd = accept(sock_fd, (struct sockaddr *) &vm_addr,
				&cli_lenght);
		if (server_sock_fd > 0) {
			VLOG_INFO(lg, "Connected with RouteFlow server");
			/* Send all occured events while the server wasn't connected */
			while (events.size()) {
				if (send(server_sock_fd, &events.front(), sizeof(base_msg), 0)
						== -1)
					perror("send");
				events.pop();
			}
			while (1) {
				base_msg base_message;
				uint32_t size = sizeof(base_msg);
				int recv_dbg = recv(server_sock_fd, &base_message, size, 0);
				if (recv_dbg < 0)
					VLOG_ERR(lg, "Couldn't receive message");
				else if (recv_dbg == 0) {
					VLOG_INFO(lg, "Lost connection. Waiting for a new server connection");
					break;
				}
				else
				{
					VLOG_INFO(lg, "Received Message from server");
					
					while(cmd_mutex)
					{
						usleep(1000);
					}
					cmd_mutex = 1;
					command.push(base_message);
					cmd_mutex = 0;
				}
			}
		}

	}

}

/*
 * Checks if there is a TCP port defined by the user.
 * If not, uses the default port.
 */
void RouteFlowC::configure(const Configuration* config) {

	cmd_mutex = 0;
	/*
	 * Get command line arguments.
	 */
	const hash_map<string, string> argmap = config->get_arguments_list();
	hash_map<string, string>::const_iterator i;
	i = argmap.find("tcpport");
	if (i != argmap.end()) {
		printf("Configure Port %s", i->second.c_str());
		tcpport = (uint16_t) atoi(i->second.c_str());
	} else
		tcpport = TCP_PORT;

}

void RouteFlowC::install() {

	this->server_thread.start(boost::bind(&RouteFlowC::server, this));
	this->send_cmd.start(boost::bind(&RouteFlowC::send_command, this));
	//register_handler<Link_event> (boost::bind(&RouteFlowC::handle_link_event,
	//		this, _1));
	register_handler<Packet_in_event> (boost::bind(
			&RouteFlowC::handle_packet_in, this, _1));
	register_handler<Datapath_join_event> (boost::bind(
			&RouteFlowC::handle_datapath_join, this, _1));
	register_handler<Datapath_leave_event> (boost::bind(
			&RouteFlowC::handle_datapath_leave, this, _1));
	register_handler<Desc_stats_in_event> (boost::bind(
			&RouteFlowC::handle_desc_in, this, _1));

}

REGISTER_COMPONENT(container::Simple_component_factory<RouteFlowC>, RouteFlowC)
;

}