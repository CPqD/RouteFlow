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

#ifndef QFSERVER_HH_
#define QFSERVER_HH_

#include <linux/if_ether.h>
#include <list>
#include <map>

#include "rfprotocol.hh"
#include "rfvm.hh"
#include "ControllerConnection.hh"
#include "openflow.h"

using std::list;
using std::multimap;

typedef enum rfoperation {
	RIPv2,           /* RIPv2 protocol.*/
	OSPF, 			 /* OSPF protocol. */
	CLEAR_FLOW_TABLE /* Clear flow table. */
} qfoperation_t;

/* Open vSwitch operations */
typedef enum ovs_operation {
	DROP_ALL, /* Drop all incoming packets. */
	VM_FLOW,  /* Flow to communicate two linked VM's. */
	VM_INFO,  /* Flow to send the MAP Event packet to the controller. */
	ARP,	 /* To not drop ARP packets */
	ICMP, 	 /* To not drop ICMP packets */
	CONTROLLER /*Send all traffic to the controller */
} ovs_operation_t;

typedef enum rf_operation{

	VIRTUAL_PLANE,
	VIRT_N_FORWARD_PLANE

}rf_operation_t;

/* Map between datapaths and VM's. */
typedef struct Dp2Vm {

	uint64_t dpId; /* Datapath id. */
	uint64_t vmId; /* Virtual Machine identificator. */

} Dp2Vm_t;

/*Map between VM ports and Open vSwitch ports. */
typedef struct Vm2Ovs {

   uint16_t Vm_port;  /* Number of the Virtual Machine port. */
   uint16_t Ovs_port; /* Number of the Open vSwitch port. */

} Vm2Ovs_t;


typedef struct {

	uint64_t src_dp;
	uint64_t dst_dp;
	uint8_t src_p;
	uint8_t dst_p;
	bool dp1_vmconn;
	bool dp2_vmconn;

} linkEv;

struct cmp {
	bool operator()(uint64_t a, uint64_t b) {
		return a < b;
	}
};

class RouteFlowServer {

public:
	list<RFVirtualMachine> m_vmList;             /* List of Virtual Machines. */
	list<Datapath_t> m_dpIdleList;				 /* List of datapaths without a VM */
	int32_t ListvmLock;	                         /* Controls the access of the VM's List. */
	uint16_t tcpport;                            /* Port to connect with VM's. */
	list<Dp2Vm_t> m_Dp2VmList;                   /* List of VM's and datapath id map. */
	rf_operation_t operation;                    /* RouteFLow operation type */
	Datapath_t ovsdp;                            /* Open vSwitch Datapath info. */
	multimap<uint64_t, Vm2Ovs_t, cmp> Vm2OvsList;/* Map between VM number and his
												    ports with the respective datapath. */
	ControllerConnection * qfCtlConnection;      /* Controls connection with the
													rf-controller. */


	RouteFlowServer(uint16_t srvPort, rf_operation_t op);


	int32_t
	datapath_join_event(uint64_t dpId, char hw_desc[], uint16_t numPorts);

	int32_t datapath_leave_event(uint64_t dpId);

	int32_t ovs_port_mapping_event(uint64_t vmId, uint16_t vmPort,
			uint16_t inPort);

	int32_t packet_in_event(uint64_t dpId, uint32_t inPort, uint64_t pktId,
			uint16_t pktSize, uint16_t pktType);

	int32_t port_status_event(uint8_t reason, uint64_t dpId, uint16_t port);

	int32_t add_link_event(uint64_t dpsrc, uint16_t sport, uint64_t dpdst,
			uint16_t dport);

	int32_t del_link_event(uint64_t dpsrc, uint16_t sport, uint64_t dpdst,
			uint16_t dport);

	int32_t handle_link_event(uint16_t reason, uint64_t dpsrc, uint16_t sport,
			uint64_t dpdst, uint16_t dport);

	int32_t lockvmList();

	int32_t unlockvmList();

	uint64_t Dp2VmMap(uint64_t dp);

	uint64_t Vm2DpMap(uint64_t vm);

	bool Dp2VmListStatus(uint64_t dp);

	int send_flow_msg(uint64_t dp, qfoperation_t operation);

	void send_ovs_flow(uint64_t dp1, uint64_t dp2, uint8_t port1,
			uint8_t port2, ovs_operation_t ovs_op);

	int process_msg(RFMessage * msg);

	int recv_msg(int fd, RFMessage * msg, const int32_t size);

	int send_msg(int fd, RFMessage * msg);

	int send_IPC_flowmsg(uint64_t dpid, uint8_t msg[], size_t size);

	int send_IPC_packetmsg(uint64_t dpid, uint64_t MsgId, uint16_t port_out);

	int VmToOvsMapping(uint64_t vmId, uint16_t VmPort, uint16_t OvsPort);
};

#endif /* QFSERVER_HH_ */
