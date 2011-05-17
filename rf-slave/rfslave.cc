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

#include <stdio.h>
#include <net/ethernet.h>
#include <linux/if_ether.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <stdlib.h>
#include <cstring>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <fcntl.h>
#include <cstring>
#include <list>
#include <sstream>
#include <string>
#include <map>
#include <ifaddrs.h>
#include <vector>
#include "rfprotocol.hh"
#include "interface.hh"
#include "flwtable.hh"

#define BUFFER_SIZE      23           /* Packet size. */
#define ETH_ADRLEN       6            /* Mac Address size. */
#define ETH_P_RFP        0x0A0A       /* QF protocol. */

using std::cout;
using std::endl;
using std::list;
using std::string;
using std::stringstream;
using std::vector;
using std::map;

const uint8_t intfNumMax = 4;

RFSocket rfSock;
map<string, Interface*> ifacesMap;
uint64_t gVmId;
uint32_t gVmIpAddr;
uint8_t gVmMAC[IFHWADDRLEN];

int send_packet(char ethName[], uint8_t srcAddress[ETH_ADRLEN], uint8_t port,
		uint64_t VMid) {

	char buffer[BUFFER_SIZE];
	uint16_t ethType;
	struct sockaddr_ll sll;
	struct ifreq req;
	uint8_t dstAddress[ETH_ADRLEN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	int SockFd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_RFP));

	strcpy(req.ifr_name, ethName);

	if (ioctl(SockFd, SIOCGIFFLAGS, &req) < 0) {
		fprintf(stderr, "ERROR! ioctl() call has failed: %s\n", strerror(errno));
		exit(1);
	}

	/* If the interface is down we can't send the packet. */
	printf("FLAG %d\n", req.ifr_flags & IFF_UP);
	if (!(req.ifr_flags & IFF_UP))
		return -1;

	/* Get the interface index. */
	if (ioctl(SockFd, SIOCGIFINDEX, &req) < 0) {
		fprintf(stderr, "ERROR! ioctl() call has failed: %s\n", strerror(errno));
		exit(1);
	}

	int ifindex = req.ifr_ifindex;

	int addrLen = sizeof(struct sockaddr_ll);

	if (ioctl(SockFd, SIOCGIFHWADDR, &req) < 0) {
		fprintf(stderr, "ERROR! ioctl() call has failed: %s\n", strerror(errno));
		exit(1);
	}
	int i;
	for (i = 0; i < ETH_ADRLEN; i++)
		srcAddress[i] = (uint8_t) req.ifr_hwaddr.sa_data[i];

	memset(&sll, 0, sizeof(struct sockaddr_ll));
	sll.sll_family = PF_PACKET;
	sll.sll_ifindex = ifindex;

	if (bind(SockFd, (struct sockaddr *) &sll, addrLen) < 0) {
		fprintf(stderr, "ERROR! bind() call has failed: %s\n", strerror(errno));
		exit(1);
	}

	memset(buffer, 0, BUFFER_SIZE);

	memcpy((void *) buffer, (void *) dstAddress, ETH_ADRLEN);
	memcpy((void *) (buffer + ETH_ADRLEN), (void *) srcAddress, ETH_ADRLEN);
	ethType = htons(ETH_P_RFP);
	memcpy((void *) (buffer + 2 * ETH_ADRLEN), (void *) &ethType,
			sizeof(uint16_t));
	memcpy((void *) (buffer + 14), (void *) &VMid, sizeof(uint64_t));
	memcpy((void *) (buffer + 22), (void *) &port, sizeof(uint8_t));

	return (sendto(SockFd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &sll,
			(socklen_t) addrLen));

}

void error(const char *msg) {
	perror(msg);
	exit(0);
}

/* Get the MAC address of the interface. */
int get_hwaddr_byname(const char * ifname, uint8_t hwaddr[]) {
	struct ifreq ifr;
	int sock;

	if ((NULL == ifname) || (NULL == hwaddr)) {
		return -1;
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		return -1;
	}

	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
	ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';

	if (-1 == ioctl(sock, SIOCGIFHWADDR, &ifr)) {
		perror("ioctl(SIOCGIFHWADDR) ");
		return -1;
	}

	std::memcpy(hwaddr, ifr.ifr_ifru.ifru_hwaddr.sa_data, IFHWADDRLEN);

	close(sock);

	return 0;
}

/* Get the IP address of the interface. */
uint32_t get_ipaddr_byname(const char * ifname) {
	struct ifreq ifr;
	int sock;

	if (NULL == ifname) {
		return -1;
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		return -1;
	}

	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
	ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';

	if (-1 == ioctl(sock, SIOCGIFADDR, &ifr)) {
		perror("ioctl(SIOCGIFADDR) ");
		return -1;
	}

	close(sock);

	return ((struct sockaddr_in *) (&ifr.ifr_addr))->sin_addr.s_addr;
}

/* Get the interface associated VM identification number. */
uint64_t get_vmId(const char *ifname) {
	uint8_t mac[6];

	if (NULL == ifname) {
		return 0;
	}

	get_hwaddr_byname(ifname, mac);

	stringstream vmId;
	vmId << (int) mac[0] << (int) mac[1] << (int) mac[2] << (int) mac[3]
			<< (int) mac[4] << (int) mac[5];

	int64_t dVmId = atoll(vmId.str().c_str());

	return (dVmId > 0) ? dVmId : 0;
}

/* Get all names of the interfaces in the system. */
std::vector<Interface*> get_Interfaces() {

	struct ifaddrs *ifaddr, *ifa;
	int family;

	if (getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}

	/* Walk through linked list, maintaining head pointer so we
	 can free list later. */

	std::vector<Interface *> Interfaces;
	Interface* tmpI = new Interface(0, "");

	uint64_t vmId = get_vmId("eth0");

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		family = ifa->ifa_addr->sa_family;

		uint8_t mac[6];

		if (family == AF_PACKET && strcmp(ifa->ifa_name, "eth0") != 0
				&& strcmp(ifa->ifa_name, "lo") != 0) {
			get_hwaddr_byname(ifa->ifa_name, mac);
			string ifaceName = ifa->ifa_name;
			size_t pos = ifaceName.find_first_of("123456789");
			string port_num = ifaceName.substr(pos, ifaceName.length() - pos
					+ 1);
			uint32_t port_id = atoi(port_num.c_str());
			tmpI = new Interface(port_id, ifaceName);
			tmpI->setHwAddr(mac);
			printf("Interface %s\n", ifa->ifa_name);
			Interfaces.push_back(tmpI);
			cout << "Send " << send_packet(ifa->ifa_name, mac, port_id, vmId)
					<< endl;
		}
	}

	/* Free list. */
	freeifaddrs(ifaddr);
	return Interfaces;

}

int init_Interfaces() {

	vector<Interface*> Interfaces = get_Interfaces();
	int i;
	for (i = 0; i < Interfaces.size(); i++) {
		ifacesMap[Interfaces.at(i)->getName()] = (Interface *) Interfaces.at(i);
	}

	return 0;
}

/* Process VM configuration messages
 * received from controller.
 */
int process_msg(RFMessage * msg) {
	switch (msg->getType()) {
	case RFP_CMD_ACCPET:
		cout << "RFP_CMD_ACCPET" << endl;
		break;
	case RFP_CMD_REJECT:
		cout << "RFP_CMD_REJECT" << endl;
		break;

		/* Clear all interfaces associated with the VM.  */
	case RFP_CMD_VM_RESET: {
		FlwTablePolling::stop();

		std::map<string, Interface *>::iterator iter;
		for (iter = ifacesMap.begin(); iter != ifacesMap.end(); iter++) {
			delete ((*iter).second);
			(*iter).second = NULL;
		}

		ifacesMap.clear();

		cout << "RFP_CMD_VM_RESET" << endl;
	}
		break;
		/* VM Configuration */
	case RFP_CMD_VM_CONFIG: {
		RFVMMsg * VmMsg = (RFVMMsg *) msg;

		uint8_t config = VmMsg->getWildcard();
		if (config & RFP_VM_NPORTS) {
			/*TODO Get the number of ports the switch has.*/
			init_Interfaces();
		} else {
			printf("Config 2\n");
			init_Interfaces();
		}

		FlwTablePolling::start();
		cout << "RFP_CMD_VM_CONFIG" << endl;
	}
		break;
	default:
		cout << "Invalid Message (" << msg->getType() << ")" << endl;
		break;
	}

	return 0;
}

/* Receives VM configuration message from the controller. */
int recv_msg(int fd, RFMessage * msg, int32_t size) {
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

int main(int argc, char *argv[]) {
	int portno;

	if (argc < 4) {
		cout << "usage " << argv[0] << "rfslave <server_ipaddr> <port> <intf>"
				<< endl;
		return -1;
	}

	string serverIp(argv[1]);
	portno = atoi(argv[2]);

	bool regOK = 0;

	while (!regOK) {
		if (rfSock.rfOpen() < 0) {
			error("ERROR opening socket");
		}

		gVmId = get_vmId(argv[3]);
		gVmIpAddr = get_ipaddr_byname(argv[3]);
		get_hwaddr_byname(argv[3], gVmMAC);

		cout << "gVmId=" << gVmId << endl;

		cout << "Connection to " << serverIp.c_str() << ":" << portno << endl;

		if (rfSock.rfConnect(htons(portno), serverIp) < 0) {
			rfSock.rfClose();
			error("ERROR connecting");
		}

		cout << "Connected to " << serverIp.c_str() << ":" << portno << endl;

		RFVMMsg msg;

		msg.setDstIP(serverIp);
		msg.setSrcIP(gVmIpAddr);
		msg.setVMId(gVmId);
		msg.setType(RFP_CMD_REGISTER);

		RFMessage * pMsg;

		int n = rfSock.rfSend(msg);
		if (n < 0) {
			error("ERROR writing to socket");
		}

		pMsg = rfSock.rfRecv();

		if (NULL == pMsg) {
			rfSock.rfClose();
			cout << "error receiving message" << endl;
			return -1;
		}
		cout << "Connection msg (" << pMsg->length() << "bytes)" << endl;
		if (RFP_CMD_ACCPET == pMsg->getType()) {
			cout << "VM has been registered" << endl;
			regOK = 1;
		} else {
			rfSock.rfClose();
			cout << "Error type:" << pMsg->getType() << endl;
			sleep(15);
		}
		delete (pMsg);
	}
	FlwTablePolling::init();

	/* Stay listening for some new message. */
	while (1) {
		int nfds = rfSock.getFd();
		RFMessage * pMsg;
		pMsg = rfSock.rfRecv();

		if (NULL != pMsg) {
			if (pMsg->length() == 0) {
				/* Server close connection. */
				break;
			}
			cout << "new msg (" << pMsg->length() << "bytes)" << endl;
			process_msg(pMsg);
			delete (pMsg);
		} else {
			continue;
		}
	}
	rfSock.rfClose();
	return 0;
}
