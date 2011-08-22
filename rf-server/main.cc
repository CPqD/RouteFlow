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
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include  <signal.h>

#include "ClientConnection.hh"
#include "rfserver.hh"
#include "ControllerConnection.hh"
#include "TCPSocket.hh"
#include "cpqd.hh"
#include "RFCtlMessageProcessor.hh"
#include "RawMessage.hh"

const uint32_t TCP_PORT = 5678;
const uint32_t MAX_CONNECTIONS = 10;

#define DEFAULTCTLIP "127.0.0.1"
#define DEFAULTCTLPORT 7890

using std::memset;

RouteFlowServer rfSrv(TCP_PORT, VIRT_N_FORWARD_PLANE);

int main(int argc, char **argv) {

	timeval t;
	int sockFd, connCount;
	socklen_t cliLength;
	struct sockaddr_in servAddr;
	int8_t ret = SUCCESS;

	ControllerConnection qfController;

	ttag_Address RemoteAdd;
	uint32_t TCPPort;

	TCPSocket * tcpSocket = NULL;
	RFCtlMessageProcessor * rfmp = NULL;
	RawMessage * msg = NULL;
	rfmp = new RFCtlMessageProcessor();
	msg = new RawMessage();

	string IP = DEFAULTCTLIP;
	TCPPort = DEFAULTCTLPORT;

	/*
	 * Initializing the TCP Socket.
	 */
	tcpSocket = new TCPSocket();

	/*
	 * Setting the FD of the TCP Socket.
	 */
	tcpSocket->open();
	ret = strToAddress(IP, RemoteAdd);

	/*
	 * Connecting to the controller via IP = RemoteAdd, port = port.
	 */
	uint32_t i = 0;
	do {
		ret = tcpSocket->connect(RemoteAdd, TCPPort);
		i++;
		syslog(LOG_INFO, "We are in attempt number: %d ", i);
		if (ret != SUCCESS) {
			sleep(2);
		}
	} while ((ret != SUCCESS));

	qfController.setCommunicator(tcpSocket);
	qfController.setMessageProcessor(rfmp);
	qfController.setMessage((IPCMessage *) msg);
	qfController.arg((void *) &rfSrv);

	/*
	 * Starting Controller Connection Thread
	 */
	qfController.start();

	rfSrv.qfCtlConnection = &qfController;

	ClientConnection qfClient;

	qfClient.arg((void *) &rfSrv);

	(void) (argc);
	(void) (argv);

	t.tv_sec = 0;
	t.tv_usec = 1000;

	do {
		sockFd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockFd < 0) {
			syslog(LOG_ERR, "Could not open a new socket");
			//		return -1;
		}
		sleep(2);
	} while (sockFd < 0);
	qfClient.start();

	/* TODO
	 *  Get TCP port from main arguments.
	 */
	uint16_t tcpport = TCP_PORT;

	/*VM Connection.*/
	std::memset((char *) &servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = INADDR_ANY;
	servAddr.sin_port = htons(tcpport);

	int bindsocketret = 0;

	while (bind(sockFd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
		syslog(LOG_INFO, "Could not bind server socket");
		sleep(2);
	}
	syslog(LOG_INFO, "Finally bound to VM sockets");

	listen(sockFd, MAX_CONNECTIONS);

	/*
           sockFd should be in "blocking" mode to avoid wasting
	   CPU time unnecessarily
         */
/*
	int flags = fcntl(sockFd, F_GETFL);
	flags &= ~O_NONBLOCK;
	fcntl(sockFd, F_SETFL, flags | O_NONBLOCK);
*/
	cliLength = sizeof(struct sockaddr_in);
	connCount = 0;
	while (1) {
		struct sockaddr_in vmAddr;
		int vmSockFd;

		vmSockFd = accept(sockFd, (struct sockaddr *) &vmAddr, &cliLength);

		if (vmSockFd > 0) {
			char buffer[ETH_PKT_SIZE];
			std::memset(buffer, 0, ETH_PKT_SIZE);

			if (rfSrv.recv_msg(vmSockFd, (RFMessage *) buffer, ETH_PKT_SIZE)
					<= 0) {
				close(vmSockFd);
				continue;
			}

			RFVMMsg * regMsg = (RFVMMsg *) buffer;
			RFVMMsg msg;

			msg.setDstIP(vmAddr.sin_addr.s_addr);
			msg.setSrcIP("10.4.1.26");

			while (rfSrv.lockvmList() < 0)
				;

			std::list<RFVirtualMachine>::iterator iter;
			bool alreadyregVmId = 0;

			for (iter = rfSrv.m_vmList.begin(); iter != rfSrv.m_vmList.end(); iter++) {
				if (iter->getVmId() == regMsg->getVMId()) {
					alreadyregVmId = 1;
				}
			}

			rfSrv.unlockvmList();

			if (RFP_CMD_REGISTER == regMsg->getType() && !alreadyregVmId) {
				uint64_t vmId = regMsg->getVMId();

				msg.setVMId(vmId);
				msg.setType(RFP_CMD_ACCPET);

				RFVirtualMachine newVM;
				newVM.setVmId(vmId);
				newVM.setSockFd(vmSockFd);
				newVM.setIpAddr(vmAddr.sin_addr.s_addr);
				newVM.setTcpPort(vmAddr.sin_port);
				newVM.setMode(RFP_VM_MODE_STANDBY);
				newVM.setVMAddr(&vmAddr);

				if (rfSrv.send_msg(vmSockFd, &msg) <= 0) {
					syslog(LOG_ERR, "Could not write to VM socket");
				}

				bool found = 0;
				uint8_t ports;

				/* TODO The code below appear on the method datapath_join_event
				 * Change it in further versions to let main.cc more clean.
				 */
				if (!rfSrv.m_dpIdleList.empty()) { //If there is an Idle Datapath.
					Dp2Vm_t vm2dptmp;
					vm2dptmp.dpId = rfSrv.Vm2DpMap(vmId);
					syslog(LOG_DEBUG, "Dp = %lld, VM = %lld", vm2dptmp.dpId,
							vmId);
					if (vm2dptmp.dpId != 0) { //The VM has a Datapath assigned to it?
						syslog(LOG_DEBUG, "Has a Datapath assigned.");
						std::list<Datapath_t>::iterator iter;
						for (iter = rfSrv.m_dpIdleList.begin(); iter
								!= rfSrv.m_dpIdleList.end(); iter++) {

							/*  The idle datapath belongs to some VM?*/
							if (vm2dptmp.dpId == iter->dpId) {
								newVM.setMode(RFP_VM_MODE_RUNNING); //Set VM mode to running (local definition)
								Datapath_t newDP;
								newDP.dpId = iter->dpId;
								newDP.nports = iter->nports;
								newVM.setDpId(newDP);
								ports = iter->nports;
								rfSrv.m_dpIdleList.erase(iter);
								found = 1;
								break;
							}
						}
					} else {
						std::list<Datapath_t>::iterator iter;
						for (iter = rfSrv.m_dpIdleList.begin(); iter
								!= rfSrv.m_dpIdleList.end(); iter++) {
							if (rfSrv.Dp2VmMap(iter->dpId == 0)) { //Is there an Idle Datapath without any VM assigned to it?
								newVM.setMode(RFP_VM_MODE_RUNNING); //Set VM mode to running (local definition)
								Datapath_t newDP;
								newDP.dpId = iter->dpId;
								newDP.nports = iter->nports;
								newVM.setDpId(newDP);
								ports = iter->nports;
								rfSrv.m_dpIdleList.erase(iter);
								vm2dptmp.vmId = newVM.getVmId();
								vm2dptmp.dpId = newVM.getDpId().dpId;
								rfSrv.m_Dp2VmList.push_front(vm2dptmp);
								syslog(
										LOG_DEBUG,
										"Dp = %lld, VM = %lld added to the List",
										vm2dptmp.dpId, vm2dptmp.vmId);
								found = 1;
								break;
							}
						}
					}
					if (found) {
						/* Install RIPv2 default flow. */
						rfSrv.send_flow_msg(newVM.getDpId().dpId, RIPv2);
						/* Install OSPF default flow. */
						rfSrv.send_flow_msg(newVM.getDpId().dpId, OSPF);

						/* Configuration message to the slave. */
						RFVMMsg logmsg;
						logmsg.setDstIP(newVM.getIpAddr());
						logmsg.setSrcIP("10.4.1.26");
						logmsg.setVMId(newVM.getVmId());
						logmsg.setType(RFP_CMD_VM_CONFIG);
						logmsg.setDpId(newVM.getDpId().dpId);
						logmsg.setMode(RFP_VM_MODE_RUNNING);
						logmsg.setWildcard(RFP_VM_MODE | RFP_VM_DPID
								| RFP_VM_NPORTS);
						logmsg.setNPorts(ports);

						if (rfSrv.send_msg(newVM.getSockFd(), &logmsg) <= 0) {
							syslog(LOG_ERR, "Could not write to VM socket");
						}
					} else {
						syslog(
								LOG_INFO,
								"The Datapath assigned to VM: %lld isn't connected",
								newVM.getVmId());
					}
				}

				while (rfSrv.lockvmList() < 0);


				rfSrv.m_vmList.push_back(newVM);
				syslog(LOG_INFO, "A new VM was registered: VmId=%lld", vmId);

				rfSrv.unlockvmList();

			}/* VmId already registered or msg type unknown. */
			else {
				msg.setType(RFP_CMD_REJECT);

				syslog(LOG_INFO, "Connection refused: VmId already registered.");

				if (rfSrv.send_msg(vmSockFd, &msg) <= 0) {
					syslog(LOG_ERR, "Could not write to VM socket");
				}

				close(vmSockFd);
			}

		}
	}

	close(sockFd);

	return 0;
}

