/*
 *	Copyright 2011 Fundação CPqD
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *	 you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *	 Unless required by applicable law or agreed to in writing, software
 *	 distributed under the License is distributed on an "AS IS" BASIS,
 *	 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <syslog.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <boost/bind.hpp>
#include <boost/shared_array.hpp>
#include <string>
#include <cstring>
#include <vector>

#include "openflow.h"
#include "rfserver.hh"
#include "../include/base_msg.h"
#include "cmd_msg.h"
#include "IPCCommunicator.hh"
#include "event_msg.h"

# define UINT32_MAX		(4294967295U)

const uint8_t LOCKED = 1;
const uint8_t UNLOCKED = 0;
int ovs_portson = 0;

using std::map;
using std::pair;

/*
 * Class Constructor.
 * */
RouteFlowServer::RouteFlowServer(uint16_t srvPort, rf_operation_t op) {

	/*
	 * Set the TCP port and the operation type.
	 */
	tcpport = srvPort;
	operation = op;

	this->ListvmLock = UNLOCKED;
}

/*
 * Handle Link Event Messages from the rf-controller.
 * */
int32_t RouteFlowServer::handle_link_event(uint16_t reason, uint64_t dpsrc,
		uint16_t sport, uint64_t dpdst, uint16_t dport) {

	if (this->operation == VIRTUAL_PLANE) {
		if (reason == ADD) {
			return add_link_event(dpsrc, sport, dpdst, dport);
		} else if (reason == REMOVE) {
			return del_link_event(dpsrc, sport, dpdst, dport);
		} else {
			syslog(LOG_ERR, "[RFSERVER] Handle Link: Wrong reason (not "
				"ADD nor REMOVE");
			return -1;
		}
	} else
		return 0;
}

/* Install flow in Open vSwitch for a detected link */
int32_t RouteFlowServer::add_link_event(uint64_t dpsrc, uint16_t sport,
		uint64_t dpdst, uint16_t dport) {

	uint64_t dp1, dp2;
	dp1 = Dp2VmMap(dpsrc);
	dp2 = Dp2VmMap(dpdst);
	if (dp1 != 0 && dp2 != 0) {

		send_ovs_flow(dpsrc, dpdst, sport, dport, VM_FLOW);

	} else {
		/* There is not a datapath associated with some VM
		 * TODO put the links in an Idle Link List*/
		return -1;

	}

	return SUCCESS;
}

/* Delete an Open vSwitch Flow
 * TODO put this code in ovs_send_flow and use this method here
 */
int32_t RouteFlowServer::del_link_event(uint64_t dpsrc, uint16_t sport,
		uint64_t dpdst, uint16_t dport) {

	ofp_flow_mod* ofm;
	size_t size = sizeof *ofm;

	/*
	 * Takes the respective ports of the datapaths in Open vSwitch.
	 */
	uint16_t portsrc = 0;
	uint16_t portdst = 0;
	multimap<uint64_t, Vm2Ovs_t, cmp>::iterator it;
	pair<multimap<uint64_t, Vm2Ovs_t, cmp>::iterator, multimap<uint64_t,
			Vm2Ovs_t, cmp>::iterator> range;
	range = Vm2OvsList.equal_range(Dp2VmMap(dpsrc));
	for (it = range.first; it != range.second; ++it)
		if ((*it).second.Vm_port == sport) {
			portsrc = (*it).second.Ovs_port;
			break;
		}

	range = Vm2OvsList.equal_range(Dp2VmMap(dpdst));
	for (it = range.first; it != range.second; ++it)
		if ((*it).second.Vm_port == dport) {
			portdst = (*it).second.Ovs_port;
			break;
		}

	boost::shared_array<char> raw_of(new char[size]);
	ofm = (ofp_flow_mod*) raw_of.get();

	/*Open Flow Header. */
	ofm->header.version = OFP_VERSION;
	ofm->header.type = OFPT_FLOW_MOD;
	ofm->header.length = htons(size);
	ofm->header.xid = 0;

	std::memset(&(ofm->match), 0, sizeof(struct ofp_match));

	ofm->match.wildcards = htonl(OFPFW_ALL & (~OFPFW_IN_PORT));
	ofm->cookie = htonl(0);
	ofm->match.in_port = htons(portsrc);

	ofm->command = htons(OFPFC_DELETE);
	ofm->idle_timeout = htons(0);
	ofm->hard_timeout = htons(0);
	ofm->buffer_id = htonl(UINT32_MAX);
	ofm->out_port = htons(portdst);
	ofm->priority = htons(OFP_DEFAULT_PRIORITY);
	ofm->flags = htons(0);

	uint8_t ofm_array[size];
	memcpy(&ofm_array, &ofm->header, size);

	send_IPC_flowmsg(ovsdp.dpId, ofm_array, size);

	return 0;
}

int32_t RouteFlowServer::ovs_port_mapping_event(uint64_t vmId, uint16_t vmPort,
		uint16_t inPort) {

	Vm2Ovs_t vm2ovs;
	vm2ovs.Vm_port = vmPort;
	vm2ovs.Ovs_port = inPort;
	Vm2OvsList.insert(pair<uint64_t, Vm2Ovs_t> (vmId, vm2ovs));

	return 0;
}

/*
 * Handles packets from any datapath to it's specific Vm.
 */
int32_t RouteFlowServer::packet_in_event(uint64_t dpId, uint32_t inPort,
		uint64_t pktId, uint16_t pktSize, uint16_t pktType) {

	/* Unused variables */
	(void) (pktId);
	(void) (pktSize);
	uint16_t port = 0;

	/* drop all LLDP packets */
	if (pktType == htons(0x88cc)) {
		return 0;
	}

	syslog(
			LOG_INFO,
			"[RFSERVER] A packet has been received from datapath %llx, port %d",
			dpId, inPort);

	int found = 0;
	std::list<RFVirtualMachine>::iterator iter;

	/*Handles packets from any Vm to it's specific datapath */
	if (dpId == ovsdp.dpId) {

		if (m_Dp2VmList.empty()) {
			send_IPC_packetmsg(0, pktId, 0);
			syslog(LOG_INFO, "[RFSERVER] "
				"There is not a VM associated with a Datapath)");
			return -1;

		}
		/* finds the DP port to send the packet
		 *  Ugly and inneficient. We must think better about the data structures
		 *  because we need the Vm dapatathid and the port number from
		 *  where the packet left. And with packet_in event we get only the OVS info, so
		 * how we have a multimap between  Vm and ovs ports with the Vm Id as the key
		 * we cant' get the values that we need in a more efficent way.
		 */
		multimap<uint64_t, Vm2Ovs_t, cmp>::iterator it;
		for (it = Vm2OvsList.begin(); it != Vm2OvsList.end(); it++) {
			if ((*it).second.Ovs_port == inPort) {
				dpId = Vm2DpMap((*it).first);
				port = (*it).second.Vm_port;
				send_IPC_packetmsg(dpId, pktId, port);
				syslog(
						LOG_INFO,
						"[RFSERVER] A packet has been transmitted to Datapath %llx port=%d)",
						dpId, port);
				return 0;

			}
		}

	}

	while (this->lockvmList() < 0) {
		usleep(1000);
	}
	/* Send packet to VM */
	for (iter = m_vmList.begin(); iter != m_vmList.end(); iter++) {
		if (iter->getDpId().dpId == dpId) {

			/* finds the VM port to send the packet.
			 * */
			uint8_t port = 0;
			multimap<uint64_t, Vm2Ovs_t, cmp>::iterator it;
			pair<multimap<uint64_t, Vm2Ovs_t, cmp>::iterator, multimap<
					uint64_t, Vm2Ovs_t, cmp>::iterator> range;
			range = Vm2OvsList.equal_range(iter->getVmId());
			for (it = range.first; it != range.second; ++it)
				if ((*it).second.Vm_port == inPort) {
					port = (*it).second.Ovs_port;
					send_IPC_packetmsg(ovsdp.dpId, pktId, port);
					syslog(
							LOG_INFO,
							"[RFSERVER] A packet has been transmitted to VM %lld (dpid=%llx port=%d)",
							iter->getVmId(), iter->getDpId().dpId, port);
					break;
				}

			found = 1;
			break;
		}
	}

	this->unlockvmList();
	if (!found) {
		syslog(LOG_INFO, "[RFSERVER] Could not send a packet to VM");
	}

	return 0;
}

void RouteFlowServer::send_ovs_flow(uint64_t dp1, uint64_t dp2, uint8_t port1,
		uint8_t port2, ovs_operation_t ovs_op) {

	ofp_flow_mod* ofm;
	size_t size = sizeof *ofm;

	if (ovs_op != DROP_ALL)
		size = sizeof *ofm + sizeof(ofp_action_output);

	boost::shared_array<char> raw_of(new char[size]);
	ofm = (ofp_flow_mod*) raw_of.get();

	/*Open Flow Header.
	 * */
	ofm->header.version = OFP_VERSION;
	ofm->header.type = OFPT_FLOW_MOD;
	ofm->header.length = htons(size);
	ofm->header.xid = 0;

	std::memset(&(ofm->match), 0, sizeof(struct ofp_match));

	/* Drop all packets.
	 * */
	if (ovs_op == DROP_ALL) {

		ofm->match.wildcards = htonl(OFPFW_ALL);
		ofm->cookie = htonl(0);
		ofm->priority = htons(1);

	} else {

		ofp_action_output& action = *((ofp_action_output*) ofm->actions);
		memset(&action, 0, sizeof(ofp_action_output));
		if (ovs_op == VM_FLOW) {

			/*
			 * Takes the respective ports of the datapaths in Open vSwitch.
			 */
			uint8_t portsrc = 0;
			uint8_t portdst = 0;
			multimap<uint64_t, Vm2Ovs_t, cmp>::iterator it;
			pair<multimap<uint64_t, Vm2Ovs_t, cmp>::iterator, multimap<
					uint64_t, Vm2Ovs_t, cmp>::iterator> range;
			range = Vm2OvsList.equal_range(Dp2VmMap(dp1));
			for (it = range.first; it != range.second; ++it)
				if ((*it).second.Vm_port == port1) {
					portsrc = (*it).second.Ovs_port;
					break;
				}

			range = Vm2OvsList.equal_range(Dp2VmMap(dp2));
			for (it = range.first; it != range.second; ++it)
				if ((*it).second.Vm_port == port2) {
					portdst = (*it).second.Ovs_port;
					break;
				}

			ofm->match.wildcards = htonl(OFPFW_ALL & (~OFPFW_IN_PORT));
			ofm->cookie = htonl(0);
			ofm->match.in_port = htons(portsrc);
			action.type = htons(OFPAT_OUTPUT);
			action.len = htons(sizeof(ofp_action_output));
			action.port = htons(portdst);
			action.max_len = htons(ETH_DATA_LEN);
			ofm->priority = htons(OFP_DEFAULT_PRIORITY);

			/* Flow to send the packets to do the mapping between Vm and Open vSwitch ports. */
		} else {

			ofm->match.wildcards = htonl(OFPFW_ALL & (~OFPFW_DL_TYPE));
			if (ovs_op == VM_INFO)
				ofm->match.dl_type = htons(0x0A0A);
			else if (ovs_op == ARP)
				ofm->match.dl_type = htons(0x0806);
			else if (ovs_op == ICMP) {
				ofm->match.wildcards = htonl(OFPFW_ALL & (~OFPFW_DL_TYPE)
						& (~OFPFW_NW_PROTO));
				ofm->match.dl_type = htons(0x0800);
				ofm->match.nw_proto = 1;
			} else {
				ofm->match.wildcards = htonl(OFPFW_ALL);

			}
			ofm->cookie = htonl(0);
			ofm->priority = htons(OFP_DEFAULT_PRIORITY);
			action.type = htons(OFPAT_OUTPUT);
			action.len = htons(sizeof(ofp_action_output));
			action.port = htons(OFPP_CONTROLLER);
			action.max_len = htons(ETH_DATA_LEN);

		}

	}
	ofm->command = htons(OFPFC_ADD);
	ofm->buffer_id = htonl(UINT32_MAX);
	ofm->idle_timeout = htons(OFP_FLOW_PERMANENT);
	ofm->hard_timeout = htons(OFP_FLOW_PERMANENT);
	ofm->flags = htons(0);

	uint8_t ofm_array[size];
	memcpy(&ofm_array, &ofm->header, size);

	if (send_IPC_flowmsg(ovsdp.dpId, ofm_array, size) == 0)
		syslog(LOG_INFO, "[RFSERVER] Flow was send to the Open vSwitch");
}

/*
 *	For each new datapath a new instance of Quagga should be created.
 */
int32_t RouteFlowServer::datapath_join_event(uint64_t dpId, char hw_desc[],
		uint16_t numPorts) {

	/* Register the new datapath_id */
	int found = 0;

	std::list<RFVirtualMachine>::iterator iter;
	while (this->lockvmList() < 0) {
		usleep(1000);
	}

	/*
	 *  Verify if the datapath_id belongs to Open vSwitch
	 *  through the datapath port name.
	 */

	if (strcmp(hw_desc, "rfovs") == 0) {
		ovsdp.dpId = dpId;
		ovsdp.nports = numPorts;
		/*
		 *  If an Open vSwitch is find and the operation is VIRTUAL_PLANE, to
		 *  keep all the traffic on the virtual plane install a flow to drop
		 *  the routing packets e other three to forward icmp,arp and packets with
		 *  virtual machine information to the controller.
		 */
		if (this->operation == VIRTUAL_PLANE) {
			send_ovs_flow(0, 0, 0, 0, VM_INFO);
			send_ovs_flow(0, 0, 0, 0, ARP);
			send_ovs_flow(0, 0, 0, 0, ICMP);
			send_ovs_flow(0, 0, 0, 0, DROP_ALL);
		} else
			send_ovs_flow(0, 0, 0, 0, CONTROLLER);

		syslog(LOG_INFO, "[RFSERVER] Connected with Open vSwitch ");

		/*unlocks the VmList */
		this->unlockvmList();
		return 0;
	}

	/*
	 *  If there is a Vm associated with this datapath use it,
	 *  else take a Standby VM and register it to the datapath.
	 */

	uint64_t VmIdtmp = Dp2VmMap(dpId);
	if (VmIdtmp != 0) {
		for (iter = m_vmList.begin(); iter != m_vmList.end(); iter++) {
			if (iter->getVmId() == VmIdtmp) {
				iter->setMode(RFP_VM_MODE_RUNNING);
				Datapath_t newDp;
				newDp.dpId = dpId;
				newDp.nports = numPorts; //The size of the vector is number of ports + 1
				iter->setDpId(newDp);
				RFVMMsg msg;

				msg.setDstIP(iter->getIpAddr());
				msg.setSrcIP("10.4.1.26");
				msg.setVMId(iter->getVmId());
				msg.setType(RFP_CMD_VM_CONFIG);
				msg.setDpId(dpId);
				msg.setMode(RFP_VM_MODE_RUNNING);
				msg.setWildcard(RFP_VM_MODE | RFP_VM_DPID | RFP_VM_NPORTS);
				msg.setNPorts(newDp.nports);

				if (RouteFlowServer::send_msg(iter->getSockFd(), &msg) <= 0) {
					syslog(LOG_ERR, "[RFSERVER] Could not write to VM socket");
				}

				found = 1;
				break;
			}
		}
	} else {
		for (iter = m_vmList.begin(); iter != m_vmList.end(); iter++) {
			if (iter->getMode() == RFP_VM_MODE_STANDBY && Vm2DpMap(
					iter->getVmId()) == 0) {
				Dp2Vm_t tmp_dp2vm;
				iter->setMode(RFP_VM_MODE_RUNNING);
				Datapath_t newDp;
				newDp.dpId = dpId;
				newDp.nports = numPorts;
				iter->setDpId(newDp);
				RFVMMsg msg;

				msg.setDstIP(iter->getIpAddr());
				msg.setSrcIP("10.4.1.26");
				msg.setVMId(iter->getVmId());
				msg.setType(RFP_CMD_VM_CONFIG);
				msg.setDpId(dpId);
				msg.setMode(RFP_VM_MODE_RUNNING);
				msg.setWildcard(RFP_VM_MODE | RFP_VM_DPID | RFP_VM_NPORTS);
				syslog(LOG_INFO, "numero de portas = %d", newDp.nports);
				msg.setNPorts(newDp.nports);
				if (RouteFlowServer::send_msg(iter->getSockFd(), &msg) <= 0) {
					syslog(LOG_ERR, "[RFSERVER] Could not write to VM socket");
				}
				tmp_dp2vm.dpId = dpId;
				tmp_dp2vm.vmId = iter->getVmId();
				m_Dp2VmList.push_front(tmp_dp2vm);
				found = 1;
				break;
			}
		}
	}

	this->unlockvmList();
	if (!found) {
		syslog(LOG_INFO, "[RFSERVER] Could not register a new dapathid");
		Datapath_t newidleDp;
		newidleDp.dpId = dpId;
		newidleDp.nports = numPorts;
		this->m_dpIdleList.push_back(newidleDp);
		return 0;
	}

	/* Clean table was already done. */

	/* Install flow to receive RIPv2 packets. */

	send_flow_msg(dpId, RIPv2);

	/* Install flow to receive OSPF packets. */

	send_flow_msg(dpId, OSPF);

	syslog(LOG_INFO, "[RFSERVER] A new datapath has been registered: id=%llx",
			dpId);

	return 0;
}

uint64_t RouteFlowServer::Dp2VmMap(uint64_t dp) {
	std::list<Dp2Vm_t>::iterator iter;

	for (iter = m_Dp2VmList.begin(); iter != m_Dp2VmList.end(); iter++) {
		if (iter->dpId == dp) {
			return iter->vmId;
		}
	}
	return 0;
}

uint64_t RouteFlowServer::Vm2DpMap(uint64_t vm) {
	std::list<Dp2Vm_t>::iterator iter;

	for (iter = m_Dp2VmList.begin(); iter != m_Dp2VmList.end(); iter++) {
		if (iter->vmId == vm) {
			return iter->dpId;
		}
	}
	return 0;
}

/*
 * Send a flow message to the datapath id, depending on the operation choice
 * defined above.
 */
int RouteFlowServer::send_flow_msg(uint64_t dp_id, qfoperation_t operation) {

	ofp_flow_mod* ofm;
	size_t size = sizeof *ofm;

	if (operation != CLEAR_FLOW_TABLE) {
		size = sizeof *ofm + sizeof(ofp_action_output);
	}

	boost::shared_array<char> raw_of(new char[size]);
	ofm = (ofp_flow_mod*) raw_of.get();

	ofm->header.version = OFP_VERSION;
	ofm->header.type = OFPT_FLOW_MOD;
	ofm->header.length = htons(size);
	ofm->header.xid = 0;

	std::memset(&(ofm->match), 0, sizeof(struct ofp_match));

	if (operation == RIPv2) {
		syslog(LOG_DEBUG, "[RFSERVER] Configuring flow table for RIPv2");
		ofm->match.wildcards = htonl(OFPFW_ALL & (~OFPFW_DL_TYPE)
				& (~OFPFW_NW_PROTO) & (~((uint32_t) 63 << OFPFW_NW_DST_SHIFT)));
		ofm->match.dl_type = htons(0x0800);
		ofm->match.nw_proto = 0x11;
		ofm->match.nw_dst = inet_addr("224.0.0.9"); /* RIPv2 Destination IP.*/
	} else if (operation == OSPF) {
		syslog(LOG_DEBUG, "[RFSERVER] Configuring flow table for OSPF");
		ofm->match.wildcards = htonl(OFPFW_ALL & (~OFPFW_DL_TYPE)
				& (~OFPFW_NW_PROTO));
		ofm->match.dl_type = htons(0x0800);
		ofm->match.nw_proto = 0x59;
	} else if (operation == CLEAR_FLOW_TABLE) {
		ofm->match.wildcards = htonl(0xffffffff);
	}
	ofm->cookie = htonl(0);
	ofm->out_port = htons(OFPP_NONE);
	if (operation == CLEAR_FLOW_TABLE) {
		syslog(LOG_DEBUG, "[RFSERVER] Clearing flow table");
		ofm->command = htons(OFPFC_DELETE);
		ofm->buffer_id = htonl(0);
		ofm->idle_timeout = htons(0);
		ofm->hard_timeout = htons(0);
		ofm->priority = htons(0);
		ofm->flags = htons(0);
	} else {

		ofm->command = htons(OFPFC_ADD);
		ofm->buffer_id = htonl(UINT32_MAX);
		ofm->idle_timeout = htons(OFP_FLOW_PERMANENT);
		ofm->hard_timeout = htons(OFP_FLOW_PERMANENT);
		ofm->priority = htons(OFP_DEFAULT_PRIORITY);
		ofm->flags = htons(0);

		ofp_action_output& action = *((ofp_action_output*) ofm->actions);
		memset(&action, 0, sizeof(ofp_action_output));
		action.type = htons(OFPAT_OUTPUT);
		action.len = htons(sizeof(ofp_action_output));
		action.port = htons(OFPP_CONTROLLER);
		action.max_len = htons(ETH_DATA_LEN);
	}

	uint8_t ofm_array[size];
	memcpy(&ofm_array, &ofm->header, size);
	send_IPC_flowmsg(dp_id, ofm_array, size);

	return 0;
}

/*
 * 	For each datapath_leave event the respective Quagga inrecv_msgstance should
 * 	be destroyed.
 */
int32_t RouteFlowServer::datapath_leave_event(uint64_t dpId) {

	bool found = 0;
	std::list<RFVirtualMachine>::iterator iter;
	while (this->lockvmList() < 0) {
		usleep(1000);
	}
	for (iter = m_vmList.begin(); iter != m_vmList.end(); iter++) {
		if (iter->getDpId().dpId == dpId) {
			iter->setMode(RFP_VM_MODE_STANDBY);
			syslog(LOG_DEBUG, "[RFSERVER] Datapath %lld disconnected", dpId);
			RFVMMsg msg;

			msg.setDstIP("10.4.1.26");
			msg.setSrcIP("10.4.1.26");
			msg.setVMId(iter->getVmId());
			msg.setType(RFP_CMD_VM_RESET);

			if (RouteFlowServer::send_msg(iter->getSockFd(), &msg) <= 0) {
				syslog(LOG_ERR, "[RFSERVER] Could not write to VM socket");
			}

			found = 1;
			break;
		}
	}

	this->unlockvmList();

	if (!found) {
		std::list<Datapath_t>::iterator iter;
		for (iter = m_dpIdleList.begin(); iter != m_dpIdleList.end(); iter++) {
			if (iter->dpId == dpId) {
				this->m_dpIdleList.erase(iter);
				break;
			}
		}
		syslog(LOG_WARNING,
				"[RFSERVER] The datapath %lld has left, but it was not "
					"registered", dpId);
	}

	return 0;
}

/*
 * Locks the vm list, to avoid concurrent reading errors.
 */
int32_t RouteFlowServer::lockvmList() {

	if (LOCKED == this->ListvmLock) {
		return -1;
	}

	this->ListvmLock = LOCKED;

	return 0;
}
/*
 * Unlocks the vm List, so other thread can use it.
 */
int32_t RouteFlowServer::unlockvmList() {

	this->ListvmLock = UNLOCKED;

	return 0;
}

/*
 * Take action according to the type of message received from the slave.
 * Currently the messages can be to send a packet throught a switch port, install a flow or remove a flow.
 */
int RouteFlowServer::process_msg(RFMessage * msg) {
	RFMsgType msgType = msg->getType();
	syslog(LOG_INFO, "[RFSERVER] A new msg(%d) arrived from %s", msgType,
			msg->getStrSrcIP().c_str());

	switch (msgType) {
	case RFP_CMD_FLOW_INSTALL: {
		RFFlowMsg * pktMsg = (RFFlowMsg *) msg;
		uint64_t vmId = pktMsg->getVMId();
		const FlowRule& rules = pktMsg->getRule();
		const FlowAction& actions = pktMsg->getAction();
		uint8_t found = 0;

		struct in_addr ip;
		ip.s_addr = rules.ip;
		syslog(
				LOG_DEBUG,
				"[RFSERVER] Flow Install msg received, Dst_Ip = %s , Outport = %d",
				inet_ntoa(ip), actions.dstPort);

		std::list<RFVirtualMachine>::iterator iter;
		for (iter = m_vmList.begin(); iter != m_vmList.end(); iter++) {
			if (iter->getVmId() == vmId) {
				syslog(LOG_DEBUG, "[RFSERVER] Virtual Machine Id %lld",
						iter->getVmId());

				ofp_flow_mod* ofm;
				size_t size = sizeof *ofm + (2 * sizeof(ofp_action_dl_addr))
						+ sizeof(ofp_action_output);
				boost::shared_array<char> raw_of(new char[size]);
				ofm = (ofp_flow_mod*) raw_of.get();

				ofm->header.version = OFP_VERSION;
				ofm->header.type = OFPT_FLOW_MOD;
				ofm->header.length = htons(size);
				ofm->header.xid = 0;

				std::memset(&(ofm->match), 0, sizeof(struct ofp_match));
				ofm->match.wildcards = htonl(OFPFW_ALL & ~(((uint32_t) 31
						+ rules.mask) << OFPFW_NW_DST_SHIFT) & ~OFPFW_DL_TYPE
						& ~OFPFW_DL_DST);

				ofm->match.dl_type = htons(0x0800);
				ofm->match.nw_dst = rules.ip;
				memcpy(ofm->match.dl_dst, &actions.srcMac, ETH_ALEN);
				syslog(LOG_DEBUG, "[RFSERVER] DstIp %d, Mask = %d", rules.ip,
						rules.mask);

				ofm->cookie = htonl(0);
				ofm->command = htons(OFPFC_ADD);
				ofm->buffer_id = htonl(UINT32_MAX);

				if (rules.mask == 32) {
					ofm->idle_timeout = htons(300);
				} else {
					ofm->idle_timeout = htons(OFP_FLOW_PERMANENT);
				}

				ofm->hard_timeout = htons(OFP_FLOW_PERMANENT);
				ofm->priority = htons((OFP_DEFAULT_PRIORITY + rules.mask));
				ofm->flags = htons(0);

				uint8_t *pActions = (uint8_t *) ofm->actions;

				/*Action: change the dst MAC address.*/
				ofp_action_dl_addr& dldstset =
						*((ofp_action_dl_addr*) ofm->actions);
				memset(&dldstset, 0, sizeof(ofp_action_dl_addr));
				dldstset.type = htons(OFPAT_SET_DL_DST);
				dldstset.len = htons(sizeof(ofp_action_dl_addr));
				memcpy(dldstset.dl_addr, actions.dstMac, ETH_ALEN);
				pActions += sizeof(ofp_action_dl_addr);

				/*Action: change the src MAC address.*/
				ofp_action_dl_addr& dlsrcset =
						*((ofp_action_dl_addr*) (pActions));
				memset(&dlsrcset, 0, sizeof(ofp_action_dl_addr));
				dlsrcset.type = htons(OFPAT_SET_DL_SRC);
				dlsrcset.len = htons(sizeof(ofp_action_dl_addr));
				memcpy(dlsrcset.dl_addr, actions.srcMac, ETH_ALEN);
				pActions += sizeof(ofp_action_dl_addr);

				/*Action: forward to port actions.dstPort. */
				ofp_action_output& output = *((ofp_action_output*) (pActions));
				memset(&output, 0, sizeof(ofp_action_output));
				output.type = htons(OFPAT_OUTPUT);
				output.len = htons(sizeof(ofp_action_output));
				output.max_len = 0;
				output.port = htons(actions.dstPort);

				syslog(
						LOG_DEBUG,
						"[RFSERVER] SrcMac = %p , DstMac = %p , Outport = %p, Ofm = %p",
						&dlsrcset, &dldstset, &output, ofm->actions);

				uint8_t ofm_array[size];
				memcpy(&ofm_array, &ofm->header, size);

				send_IPC_flowmsg(iter->getDpId().dpId, ofm_array, size);
				found = 1;

				break;
			}
		}

		if (!found) {
			syslog(LOG_INFO, "[RFSERVER] Could not find the datapath Id");
		}

		break;
	}
	case RFP_CMD_FLOW_REMOVE: {
		RFFlowMsg * pktMsg = (RFFlowMsg *) msg;
		uint64_t vm_id = pktMsg->getVMId();
		const FlowRule& rules = pktMsg->getRule();
		const FlowAction& actions = pktMsg->getAction();
		uint8_t found = 0;

		struct in_addr ip;
		ip.s_addr = rules.ip;
		syslog(LOG_DEBUG, " [RFSERVER]Flow Remove msg received, Dst_Ip = %s",
				inet_ntoa(ip));

		std::list<RFVirtualMachine>::iterator iter;
		for (iter = m_vmList.begin(); iter != m_vmList.end(); iter++) {
			if (iter->getVmId() == vm_id) {
				syslog(LOG_DEBUG, "[RFSERVER] Virtual Machine Id %lld",
						iter->getVmId());

				ofp_flow_mod* ofm;
				size_t size = sizeof *ofm + sizeof(ofp_action_output);
				boost::shared_array<char> raw_of(new char[size]);
				ofm = (ofp_flow_mod*) raw_of.get();

				ofm->header.version = OFP_VERSION;
				ofm->header.type = OFPT_FLOW_MOD;
				ofm->header.length = htons(size);
				ofm->header.xid = 0;

				ofm->match.wildcards = htonl(OFPFW_ALL & ~(((uint32_t) 31
						+ rules.mask) << OFPFW_NW_DST_SHIFT) & ~OFPFW_DL_TYPE
						& ~OFPFW_DL_DST);
				ofm->match.dl_type = htons(0x0800);
				ofm->match.nw_dst = rules.ip;
				memcpy(ofm->match.dl_dst, &actions.srcMac, ETH_ALEN);

				ofm->cookie = htonl(0);
				ofm->out_port = htons(OFPP_NONE);
				ofm->command = htons(OFPFC_MODIFY_STRICT);
				ofm->buffer_id = htonl(UINT32_MAX);
				ofm->idle_timeout = htons(60);
				ofm->hard_timeout = htons(OFP_FLOW_PERMANENT);
				ofm->priority = htons((OFP_DEFAULT_PRIORITY + rules.mask));
				ofm->flags = htons(0);

				uint8_t *pActions = (uint8_t *) ofm->actions;

				ofp_action_output& output = *((ofp_action_output*) (pActions));
				memset(&output, 0, sizeof(ofp_action_output));
				output.type = htons(OFPAT_OUTPUT);
				output.len = htons(sizeof(ofp_action_output));
				output.max_len = 0;
				output.port = htons(0);

				uint8_t ofm_array[size];
				memcpy(&ofm_array, &ofm->header, size);

				send_IPC_flowmsg(iter->getDpId().dpId, ofm_array, size);
				found = 1;

				break;
			}
		}

		if (!found) {
			syslog(LOG_INFO, "[RFSERVER] Could not find the datapath Id");
		}

		break;
	}
	default:
		syslog(LOG_INFO, "[RFSERVER] Invalid Message (%d)", msgType);
		break;
	}

	return 0;
}

/*
 * Returns the size of the routeflow message received.
 */
int RouteFlowServer::recv_msg(int fd, RFMessage * msg, int32_t size) {
	char buffer[ETH_PKT_SIZE];

	std::memset(buffer, 0, ETH_PKT_SIZE);

	int rcount = read(fd, buffer, sizeof(RFMessage));

	if (rcount > 0) {
		RFMessage * recvMsg = (RFMessage *) buffer;
		int32_t length = recvMsg->length() - rcount;
		rcount += read(fd, buffer + sizeof(RFMessage), length);

		if (rcount > size) {
			rcount = size;
		}

		std::memcpy(msg, buffer, rcount);
	}

	return rcount;
}

int RouteFlowServer::send_msg(int fd, RFMessage * msg) {
	return write(fd, msg, msg->length());
}

int RouteFlowServer::send_IPC_flowmsg(uint64_t dpid, uint8_t msg[], size_t size) {

	/*
	 * Creating new message for the flow installation.
	 */
	t_result ret;
	base_msg base_message;
	bzero(&base_message, sizeof(base_msg));
	base_message.group = COMMAND;
	base_message.type = FLOW;
	flow flow_cmd;
	flow_cmd.datapath_id = dpid;
	memcpy(&flow_cmd.flow_mod, msg, size);
	base_message.pay_size = sizeof(flow_cmd.datapath_id) + size;
	::memcpy(&base_message.payload, &flow_cmd, base_message.pay_size);

	RawMessage * controller_msg;
	controller_msg = new RawMessage();
	controller_msg->parseBytes((uint8_t *) &base_message, sizeof(base_msg));

	IPCCommunicator * Comm =
			(IPCCommunicator *) qfCtlConnection->getCommunicator();
	do {
		ret = Comm->sendMessage((IPCMessage *) controller_msg);
		if (ret == -1)
			usleep(1000);
		else
			syslog(LOG_INFO, "[RFSERVER] Msg sent successfully to %llx", dpid);
	} while (ret == -1);

	delete(controller_msg);

	return ret;
}

int RouteFlowServer::send_IPC_packetmsg(uint64_t dpid, uint64_t MsgId,
		uint16_t port_out) {
	/*
	 * Creating new message for the flow installation.
	 */
	t_result ret = 0;
	base_msg base_message;
	base_message.group = COMMAND;
	base_message.type = SEND_PACKET;
	send_packet pack_cmd;
	pack_cmd.datapath_id = dpid;
	pack_cmd.pkt_id = MsgId;
	pack_cmd.port_out = port_out;
	base_message.pay_size = sizeof(pack_cmd);
	::memcpy(&base_message.payload, &pack_cmd, sizeof(pack_cmd));

	RawMessage * controller_msg;
	controller_msg = new RawMessage();
	controller_msg->parseBytes((uint8_t *) &base_message, sizeof(base_message));

	IPCCommunicator * Comm =
			(IPCCommunicator *) qfCtlConnection->getCommunicator();

	ret = Comm->sendMessage((IPCMessage *) controller_msg);

	delete(controller_msg);

	return ret;
}

int RouteFlowServer::VmToOvsMapping(uint64_t vmId, uint16_t VmPort,
		uint16_t OvsPort) {

	Vm2Ovs_t vm2ovs;
	vm2ovs.Vm_port = VmPort;
	vm2ovs.Ovs_port = OvsPort;
	Vm2OvsList.insert(pair<uint64_t, Vm2Ovs_t> (vmId, vm2ovs));

	return 0;
}
