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
#include <net/if_arp.h>
#include <netdb.h>
#include <stdlib.h>
#include <cstring>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <cstring>
#include <list>
#include <sstream>
#include <string>
#include <syslog.h>
#include <map>
#include <ifaddrs.h>
#include <vector>
#include "rfprotocol.hh"
#include "interface.hh"
#include "flwtable.hh"
#include "rfslave.hh"

#define SYSLOGFACILITY   LOG_LOCAL7
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
using std::ofstream;

ofstream logfile;

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

/* Set the MAC address of the interface. */
int set_hwaddr_byname(const char * ifname, uint8_t hwaddr[], int16_t flags) {
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
	ifr.ifr_ifru.ifru_flags = flags & (~IFF_UP);

	if (-1 == ioctl(sock, SIOCSIFFLAGS, &ifr)) {
		perror("ioctl(SIOCSIFFLAGS) ");
		return -1;
	}

	ifr.ifr_ifru.ifru_hwaddr.sa_family = ARPHRD_ETHER;
	std::memcpy(ifr.ifr_ifru.ifru_hwaddr.sa_data, hwaddr, IFHWADDRLEN);

	if (-1 == ioctl(sock, SIOCSIFHWADDR, &ifr)) {
		perror("ioctl(SIOCSIFHWADDR) ");
		return -1;
	}

	ifr.ifr_ifru.ifru_flags = flags | IFF_UP;

	if (-1 == ioctl(sock, SIOCSIFFLAGS, &ifr)) {
		perror("ioctl(SIOCSIFFLAGS) ");
		return -1;
	}

	close(sock);

	return 0;
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
	int intfNum;

	if (getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs");
		exit( EXIT_FAILURE);
	}

	/* Walk through linked list, maintaining head pointer so we
	 can free list later. */

	std::vector<Interface *> Interfaces;
	Interface* tmpI = new Interface(0, "");
	intfNum = 0;

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		family = ifa->ifa_addr->sa_family;

		if (family == AF_PACKET && strcmp(ifa->ifa_name, "eth0") != 0
				&& strcmp(ifa->ifa_name, "lo") != 0) {
			if (0 == intfNum) {
				get_hwaddr_byname(ifa->ifa_name, gVmMAC);
			} else {
				set_hwaddr_byname(ifa->ifa_name, gVmMAC, ifa->ifa_flags);
				syslog(LOG_INFO, "Setting MAC Addr (%s)", ifa->ifa_name);
			}
			string ifaceName = ifa->ifa_name;
			size_t pos = ifaceName.find_first_of("123456789");
			string port_num = ifaceName.substr(pos, ifaceName.length() - pos
					+ 1);
			uint32_t port_id = atoi(port_num.c_str());
			tmpI = new Interface(port_id, ifaceName);
			tmpI->setHwAddr(gVmMAC);
			printf("Interface %s\n", ifa->ifa_name);
			Interfaces.push_back(tmpI);
			intfNum++;

			if (send_packet(ifa->ifa_name, gVmMAC, port_id, gVmId) == -1)
				syslog(LOG_INFO, "send_packet failed");
		}
	}

	/* Free list. */
	freeifaddrs(ifaddr);
	return Interfaces;

}

int init_Interfaces() {

	vector<Interface*> Interfaces = get_Interfaces();
	unsigned int i;
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
		syslog(LOG_DEBUG, "RFP_CMD_ACCPET");
		break;
	case RFP_CMD_REJECT:
		syslog(LOG_DEBUG, "RFP_CMD_REJECT");
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

		syslog(LOG_DEBUG, "RFP_CMD_VM_RESET");
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
		syslog(LOG_DEBUG, "RFP_CMD_VM_CONFIG");
	}
		break;
	default:
		syslog(LOG_DEBUG, "Invalid Message (%x)", msg->getType());
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

int rf_register(const char* iface, const char* server) {
	gVmId = get_vmId(iface);
	gVmIpAddr = get_ipaddr_byname(iface);

	syslog(LOG_INFO, "gVmId=%llx", gVmId);

	RFVMMsg msg;

	msg.setDstIP(server);
	msg.setSrcIP(gVmIpAddr);
	msg.setVMId(gVmId);
	msg.setType(RFP_CMD_REGISTER);

	return (rfSock.rfSend(msg));
}

int rf_connect(const char* iface, const char* server, int port) {
	if (rfSock.rfOpen() < 0) {
		syslog(LOG_ERR, "Error opening socket");
		return -1;
	}

	if (rfSock.rfConnect(htons(port), server) < 0) {
		rfSock.rfClose();
		syslog(LOG_ERR, "Error connecting to %s:%d", server, port);
		return -1;
	}

	syslog(LOG_NOTICE, "Connected to %s:%d", server, port);

	if (rf_register(iface,server) < 0) {
		syslog(LOG_ERR, "Error writing to socket");
		rfSock.rfClose();
		return -1;
	}

	RFMessage *pMsg;

	if ((pMsg = rfSock.rfRecv()) == NULL) {
		rfSock.rfClose();
		syslog(LOG_ERR, "Error receiving message: connection closed by peer.");
		return -1;
	}

	syslog(LOG_DEBUG, "Connection msg (%lx bytes)", pMsg->length());
	if (RFP_CMD_ACCPET != pMsg->getType()) {
		rfSock.rfClose();
		syslog(LOG_ERR, "Registration error type:%x", pMsg->getType());
		return -1;
	}

	syslog(LOG_NOTICE, "VM has been registered");
	delete (pMsg);

	return 0;
}

void print_help(char *prgname, int exval) {
	fprintf(stderr, "\n%s -s SERVER -p PORT -i IFACE\n\n",
			prgname);

	fprintf(stderr, "  -s SERVER       set IP address of rf-server\n");
	fprintf(stderr, "  -p PORT         set port number to connect\n");
	fprintf(stderr, "  -i IFACE        set interface name for binding\n");

	exit(exval);
}

int main(int argc, char *argv[]) {
	int opt, portno = -1;
	string serverIp = "";
	string connIface = "";

	if (argc == 1) {
		print_help(argv[0], 1);
		exit(1);
	}

	while ((opt = getopt(argc, argv, "s:p:i:")) != -1) {
		switch (opt) {
		case 's':
			serverIp = optarg;
			break;
		case 'p':
			portno = atoi(optarg);
			break;
		case 'i':
			connIface = optarg;
			break;
		case 'h':
		default:
			print_help(argv[0], 1);
		}
	}

	if (optind < argc || ((serverIp == "") || (portno == -1) || (connIface
			== "")))
		print_help(argv[0], 1);

	openlog("rf-slave", LOG_NDELAY | LOG_NOWAIT | LOG_PID, SYSLOGFACILITY);
	syslog(LOG_NOTICE, "starting RouteFlow FIB collector");

	while (1) { /* Main loop */
		if (rf_connect(connIface.c_str(), serverIp.c_str(), portno) < 0) {
			sleep(1);
			continue;
		}

		FlwTablePolling::init();

		/* Stay listening for some new message. */
		while (1) {
			RFMessage * pMsg;
			pMsg = rfSock.rfRecv();

			if (NULL != pMsg) {
				syslog(LOG_DEBUG, "New msg (%lx bytes)", pMsg->length());
				process_msg(pMsg);
				delete (pMsg);
			} else {
				/* Connection to Server lost */
				syslog(LOG_ERR, "Connection closed by peer");
				break;
			}
		}
		rfSock.rfClose();
	}

	return 0;
}
