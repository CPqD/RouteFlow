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

#include <cstring>
#include <stdlib.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <sstream>
#include "flwtable.hh"
#include "rfprotocol.hh"
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/if_ether.h>
#include <netinet/ether.h>
#include <netdb.h>
#include <sys/ioctl.h>

/*
 *
 * For communication with RT_NETLINK
 *
 */
struct rtnl_handle FlwTablePolling::rthNeigh;
struct rtnl_handle FlwTablePolling::rth;
int FlwTablePolling::family = AF_UNSPEC;
unsigned FlwTablePolling::groups = ~0U;
int FlwTablePolling::llink = 0;
int FlwTablePolling::laddr = 0;
int FlwTablePolling::lroute = 0;
Thread FlwTablePolling::HTPolling = Thread(&FlwTablePolling::HTPollingCb);
Thread FlwTablePolling::RTPolling = Thread(&FlwTablePolling::RTPollingCb);


using std::memset;
using std::strcmp;
using std::ifstream;
using std::ios;
using std::stringstream;

const int8_t cSep = ':'; /*Bytes separator in MAC address string like 00:aa:bb:cc:dd:ee*/
const uint8_t HOST_ADDR_MASK = 32;

const uint8_t LOCKED = 1;
const uint8_t UNLOCKED = 0;

extern map<string, Interface*> ifacesMap;
extern uint64_t gVmId;
extern RFSocket rfSock;

uint32_t FlwTablePolling::isInit = 0;


uint32_t FlwTablePolling::HTLock = UNLOCKED;
uint32_t FlwTablePolling::RTLock = UNLOCKED;

list<RouteEntry> FlwTablePolling::routeTable;
list<HostEntry> FlwTablePolling::hostTable;

/*
 * Thread Class
 */
void Thread::run() {
	std::cout << "thread is running." << std::endl; // ugly test: not thread-safe!

	callback(NULL); // ugly use. high coupling!
}

/*
 *  Host Class
 */

bool HostEntry::operator==(const HostEntry& h) {
	return ((this->intf == h.intf) && (this->ipAddr == h.ipAddr)
			&& (this->m_hwAddr[0] == h.m_hwAddr[0]) && (this->m_hwAddr[1]
			== h.m_hwAddr[1]) && (this->m_hwAddr[2] == h.m_hwAddr[2])
			&& (this->m_hwAddr[3] == h.m_hwAddr[3]) && (this->m_hwAddr[4]
			== h.m_hwAddr[4]) && (this->m_hwAddr[5] == h.m_hwAddr[5]));
}

/*
 *  Route Class
 */

bool RouteEntry::operator==(const RouteEntry& r) {
	return ((this->gwIpAddr == r.gwIpAddr) && (this->netAddr == r.netAddr)
			&& (this->netMask == r.netMask) && (this->intf == r.intf));
}

/*
 * FlwTable Class
 */

int32_t FlwTablePolling::strMac2Binary(const char *strMac, uint8_t *bMac) {
	for (int i = 0; i < IFHWADDRLEN; i++) {
		uint32_t j = 0;
		int8_t ch;

		//Convert letter into lower case.
		ch = tolower(*strMac++);

		if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f')) {
			return -1;
		}

		/*Convert into number.
		*   a. If character is digit then ch - '0'
		*	b. else (ch - 'a' + 10) it is done
		*	because addition of 10 takes correct value.
		*/
		j = isdigit(ch) ? (ch - '0') : (ch - 'a' + 10);
		ch = tolower(*strMac);

		if ((i < 5 && ch != cSep) || (i == 5 && ch != '\0' && !isspace(ch))) {
			++strMac;

			if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f')) {
				return -1;
			}

			j <<= 4;
			j += isdigit(ch) ? (ch - '0') : (ch - 'a' + 10);
			ch = *strMac;

			if (i < 5 && ch != cSep) {
				return -1;
			}
		}
		/* Store result.  */
		bMac[i] = (unsigned char) j;
		/* Skip cSep.  */
		++strMac;
	}

	return 0;
}

int32_t FlwTablePolling::maskAddr2Int(uint32_t mask) {
	uint8_t nbit = 0;
	for (uint8_t i = 0; i < 32; i++) {
		if (((mask >> i) & 0x1) == 0x1) {
			nbit++;
		}
	}

	return nbit;
}

/*
 * Called by our thread.
 *
 */
void * FlwTablePolling::HTPollingCb(void * data) {
	data = NULL;

	/*
	 * Will block till new RT event comes.
	 *
	 */
	rtnl_listen(&rthNeigh, FlwTablePolling::updateHostTable, NULL);
}

/*
 * Called by our thread.
 *
 */
void * FlwTablePolling::RTPollingCb(void * data) {
	data = NULL;

	/*
	 * Will block till new RT event comes.
	 *
	 */
	rtnl_listen(&rth, FlwTablePolling::updateRouteTable, NULL);
}

int32_t FlwTablePolling::init() {
	if (!FlwTablePolling::isInit) {


		/* Open NETLINK socket and start our threads. */
		rtnl_open(&rth, RTMGRP_IPV4_MROUTE | RTMGRP_IPV4_ROUTE
				| RTMGRP_IPV6_MROUTE | RTMGRP_IPV6_ROUTE);
		rtnl_open(&rthNeigh, RTMGRP_NEIGH);
		FlwTablePolling::isInit = 1;
	}

	return 0;
}

int32_t FlwTablePolling::start() {
	if (FlwTablePolling::isInit) {

		HTPolling.start();
		RTPolling.start();

		return 0;
	}

	return -1;
}

int32_t FlwTablePolling::stop() {
	if (FlwTablePolling::isInit) {
		return 0;

	}
	return -1;
}

int FlwTablePolling::updateHostTable(const struct sockaddr_nl *who,
		struct nlmsghdr *n, void *arg) {
	struct ndmsg *ndmsg_ptr = (struct ndmsg *) NLMSG_DATA(n);
	struct rtattr *rtattr_ptr;


	char intf[IF_NAMESIZE + 1];
	memset(intf, 0, IF_NAMESIZE + 1);

	if (if_indextoname((unsigned int) ndmsg_ptr->ndm_ifindex, (char *) intf)
			== NULL)
		return 0;

	char ip[INET_ADDRSTRLEN];
	char mac[2 * IFHWADDRLEN + 5 + 1];

	memset(ip, 0, INET_ADDRSTRLEN);
	memset(mac, 0, 2 * IFHWADDRLEN + 5 + 1);

	rtattr_ptr = (struct rtattr *) RTM_RTA(ndmsg_ptr);
	int rtmsg_len = RTM_PAYLOAD(n);

	for (; RTA_OK(rtattr_ptr, rtmsg_len); rtattr_ptr = RTA_NEXT(rtattr_ptr,
			rtmsg_len)) {
		switch (rtattr_ptr->rta_type) {
		case RTA_DST:
			if (inet_ntop(AF_INET, RTA_DATA(rtattr_ptr), ip, 128) == NULL)
				return 0;
			break;
		case NDA_LLADDR:
			if (strncpy(mac, ether_ntoa(((ether_addr *) RTA_DATA(rtattr_ptr))),
					sizeof(mac)) == NULL)
				return 0;
			break;
		default:
			break;
		}
	}

	HostEntry hentry;
	map<string, Interface*>::iterator it;

	switch (n->nlmsg_type) {
	case RTM_NEWNEIGH:
		std::cout << "netlink->RTM_NEWNEIGH: ip=" << ip << ", mac=" << mac << std::endl;
		hentry.ipAddr = inet_addr(ip);
		FlwTablePolling::strMac2Binary(mac, hentry.m_hwAddr);
		hentry.intf = NULL;

		it = ifacesMap.find(intf);
		if (it != ifacesMap.end())
			hentry.intf = (*it).second;

		if (NULL == hentry.intf) {
			return 0;
		}

		FlwTablePolling::addFlowToHw(hentry);
		FlwTablePolling::hostTable.push_back(hentry);
		break;
	case RTM_DELNEIGH:
		std::cout << "netlink->RTM_DELNEIGH: ip=" << ip << ", mac=" << mac << std::endl;
		hentry.ipAddr = inet_addr(ip);
		FlwTablePolling::strMac2Binary(mac, hentry.m_hwAddr);
		hentry.intf = NULL;

		it = ifacesMap.find(intf);
		if (it != ifacesMap.end())
			hentry.intf = (*it).second;

		if (NULL == hentry.intf) {
			return 0;
		}

		FlwTablePolling::delFlowFromHw(hentry);
		break;
	}

	return 0;
}

void FlwTablePolling::print_test() {
	struct in_addr addr;
	list<HostEntry>::iterator iter;

	std::cout << "Host Table:" << std::endl;
	for (iter = FlwTablePolling::hostTable.begin(); iter
			!= FlwTablePolling::hostTable.end(); iter++) {
		addr.s_addr = iter->ipAddr;
		std::cout << iter->intf->getName().c_str() << " " << inet_ntoa(addr)
				<< " " << std::hex << int(iter->m_hwAddr[0]) << ":"
				<< int(iter->m_hwAddr[1]) << ":" << int(iter->m_hwAddr[2])
				<< ":" << int(iter->m_hwAddr[3]) << ":"
				<< int(iter->m_hwAddr[4]) << ":" << int(iter->m_hwAddr[5])
				<< std::endl;
	}

	struct in_addr net;
	struct in_addr gw;
	struct in_addr mask;
	list<RouteEntry>::iterator iter2;
	std::cout << std::endl << "Route Table:" << std::endl;

	for (iter2 = FlwTablePolling::routeTable.begin(); iter2
			!= FlwTablePolling::routeTable.end(); iter2++) {
		net.s_addr = iter2->netAddr;
		gw.s_addr = iter2->gwIpAddr;
		mask.s_addr = iter2->netMask;

		std::cout << inet_ntoa(net) << " " << inet_ntoa(gw) << " "
				<< inet_ntoa(mask) << " " << iter2->intf->getName().c_str()
				<< std::endl;
	}

}

int FlwTablePolling::updateRouteTable(const struct sockaddr_nl *who,
		struct nlmsghdr *n, void *arg) {
	struct rtmsg *rtmsg_ptr = (struct rtmsg *) NLMSG_DATA(n);

	if (!((n->nlmsg_type == RTM_NEWROUTE || n->nlmsg_type == RTM_DELROUTE)
			&& rtmsg_ptr->rtm_table == RT_TABLE_MAIN)) {
		return 0;
	}

	char net[INET_ADDRSTRLEN];
	char gw[INET_ADDRSTRLEN];
	char intf[IF_NAMESIZE + 1];

	memset(net, 0, INET_ADDRSTRLEN);
	memset(gw, 0, INET_ADDRSTRLEN);
	memset(intf, 0, IF_NAMESIZE + 1);

	struct rtattr *rtattr_ptr;
	rtattr_ptr = (struct rtattr *) RTM_RTA(rtmsg_ptr);
	int rtmsg_len = RTM_PAYLOAD(n);

	for (; RTA_OK(rtattr_ptr, rtmsg_len); rtattr_ptr = RTA_NEXT(rtattr_ptr,
			rtmsg_len)) {
		switch (rtattr_ptr->rta_type) {
		case RTA_DST:
			if (inet_ntop(AF_INET, RTA_DATA(rtattr_ptr), net, 128) == NULL)
				return 0;
			break;
		case RTA_GATEWAY:
			if (inet_ntop(AF_INET, RTA_DATA(rtattr_ptr), gw, 128) == NULL)
				return 0;
			break;
		case RTA_OIF:
			if (if_indextoname(*((int *) RTA_DATA(rtattr_ptr)), (char *) intf)
					== NULL)
				return 0;
		default:
			break;
		}
	}

	/* Skipping routes to directly attached networks (next-hop field is blank) */
	{
		struct in_addr gwAddr;
		if (inet_aton(gw, &gwAddr) == 0)
		{
			return 0;
		}
	}

	struct in_addr convMascara;
	convMascara.s_addr = htonl(~((1 << (32 - rtmsg_ptr->rtm_dst_len)) - 1));
	char mask[INET_ADDRSTRLEN];
	snprintf(mask, sizeof(mask), "%s", inet_ntoa(convMascara));

	RouteEntry rentry;
	map<string, Interface*>::iterator it;

	switch (n->nlmsg_type) {
	case RTM_NEWROUTE:
		std::cout << "netlink->RTM_NEWROUTE: net=" << net << ", mask=" << mask << ", gw=" << gw << std::endl;
		rentry.netAddr = inet_addr(net);
		rentry.gwIpAddr = inet_addr(gw);
		rentry.netMask = inet_addr(mask);
		rentry.intf = NULL;

		if (INADDR_NONE == rentry.gwIpAddr) {
			return 0;
		}

		it = ifacesMap.find(intf);
		if (it != ifacesMap.end())
			rentry.intf = (*it).second;

		if (NULL == rentry.intf) {
			return 0;
		}

		FlwTablePolling::addFlowToHw(rentry);
		FlwTablePolling::routeTable.push_back(rentry);
		break;
	case RTM_DELROUTE:
		std::cout << "netlink->RTM_DELROUTE: net=" << net << ", mask=" << mask << ", gw=" << gw << std::endl;
		rentry.netAddr = inet_addr(net);
		rentry.gwIpAddr = inet_addr(gw);
		rentry.netMask = inet_addr(mask);
		rentry.intf = NULL;

		it = ifacesMap.find(intf);
		if (it != ifacesMap.end())
			rentry.intf = (*it).second;

		if (NULL == rentry.intf) {
			return 0;
		}

		FlwTablePolling::delFlowFromHw(rentry);
		FlwTablePolling::routeTable.remove(rentry);
		break;
	}

	return 0;
}

char *FlwTablePolling::mac_ntoa(unsigned char *ptr) {
	static char address[30];
	sprintf(address, "%02X:%02X:%02X:%02X:%02X:%02X", ptr[0], ptr[1], ptr[2],
			ptr[3], ptr[4], ptr[5]);
	return (address);
}

void FlwTablePolling::fakeReq(char *hostAddr, const char *intf) {
	int s;
	struct arpreq req;
	struct hostent *hp;
	struct sockaddr_in *sin;
	char *host = hostAddr;
	static char address[30];

	bzero((caddr_t) &req, sizeof(req));

	sin = (struct sockaddr_in *) &req.arp_pa;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = inet_addr(host);

	if (sin->sin_addr.s_addr == -1) {
		if (!(hp = gethostbyname(host))) {
			fprintf(stderr, "arp: %s ", host);
			herror((char *) NULL);
			return;
		}
		bcopy((char *) hp->h_addr, (char *) &sin->sin_addr,
				sizeof(sin->sin_addr));
	}

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket() failed.");
		return;
	}

	connect(s, (struct sockaddr *) sin, sizeof(struct sockaddr));
	close(s);
}

int32_t FlwTablePolling::addFlowToHw(const RouteEntry& route) {
	list<HostEntry>::iterator iter;
	uint8_t dstMac[8] = { 0, 0, 0, 0, 0, 0 };
	uint8_t found = 0, tries = 0;

	/*
	 * It is need to resolve the gateway's IP in order to install a route flow.
	 * The MAC address of the next-hop is required in flow action, which is used
	 * to re-write the layer 2 header before forward the packet.
	 *
	 */

	while ((tries < 50) && !found) {
		for (iter = FlwTablePolling::hostTable.begin(); iter
				!= FlwTablePolling::hostTable.end(); iter++) {
			if (iter->ipAddr == route.gwIpAddr) {
				found = 1;
				std::memcpy(dstMac, iter->m_hwAddr, IFHWADDRLEN);
				break;
			}
		}
		if (!found) {
			struct in_addr gw;

			gw.s_addr = route.gwIpAddr;
			FlwTablePolling::fakeReq(inet_ntoa(gw),
					route.intf->getName().c_str());
			usleep(20000);
		}
		tries++;
	}

	if (!found)
		return -1;

	RFFlowMsg rfmsg;

	/* Set Action. */
	rfmsg.setType(RFP_CMD_FLOW_INSTALL);
	rfmsg.setVMId(gVmId);
	rfmsg.setAction(route.intf->getPortId(), route.intf->getHwAddr(), dstMac);

	/* Set Rule. */
	rfmsg.setRule(route.netAddr, FlwTablePolling::maskAddr2Int(route.netMask));

	/* Send msg. */
	rfSock.rfSend(rfmsg);

	return 0;
}

int32_t FlwTablePolling::addFlowToHw(const HostEntry& host) {
	RFFlowMsg rfmsg;

	rfmsg.setType(RFP_CMD_FLOW_INSTALL);
	rfmsg.setVMId(gVmId);

	/* Set Action. */
	rfmsg.setAction(host.intf->getPortId(), host.intf->getHwAddr(),
			host.m_hwAddr);

	/* Set Rule. */
	rfmsg.setRule(host.ipAddr, HOST_ADDR_MASK);

	/* Send msg. */
	rfSock.rfSend(rfmsg);

	return 0;
}

int32_t FlwTablePolling::delFlowFromHw(const RouteEntry& route) {
	/*
	 * It is not needed to resolve the gateway's IP on route deletion.
	 * The MAC address of the next-hop is required in flow action and this
	 * field is useless when deleting flows.
	 *
	 */

	RFFlowMsg rfmsg;

	rfmsg.setType(RFP_CMD_FLOW_REMOVE);
	rfmsg.setVMId(gVmId);

	/* Set Action. */
	uint8_t dstmac[] = { 0, 0, 0, 0, 0, 0 };
	rfmsg.setAction(0, route.intf->getHwAddr(), dstmac);

	/* Set Rule. */
	rfmsg.setRule(route.netAddr, FlwTablePolling::maskAddr2Int(route.netMask));

	/* Send msg. */
	rfSock.rfSend(rfmsg);

	return 0;
}

int32_t FlwTablePolling::delFlowFromHw(const HostEntry& host) {
	RFFlowMsg rfmsg;

	rfmsg.setType(RFP_CMD_FLOW_REMOVE);
	rfmsg.setVMId(gVmId);

	/* Set Action. */
	rfmsg.setAction(host.intf->getPortId(), host.intf->getHwAddr(),
			host.m_hwAddr);

	/* Set Rule. */
	rfmsg.setRule(host.ipAddr, HOST_ADDR_MASK);

	/* Send msg. */
	rfSock.rfSend(rfmsg);

	return 0;
}

int32_t FlwTablePolling::lockHostTable() {
	if (LOCKED == FlwTablePolling::HTLock) {
		return -1;
	}

	FlwTablePolling::HTLock = LOCKED;

	return 0;
}

int32_t FlwTablePolling::unlockHostTable() {
	FlwTablePolling::HTLock = UNLOCKED;

	return 0;
}

int32_t FlwTablePolling::lockRouteTable() {
	if (LOCKED == FlwTablePolling::RTLock) {
		return -1;
	}

	FlwTablePolling::RTLock = LOCKED;

	return 0;
}

int32_t FlwTablePolling::unlockRouteTable() {
	FlwTablePolling::RTLock = UNLOCKED;

	return 0;
}
