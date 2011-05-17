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

#ifndef FLWTABLE_HH_
#define FLWTABLE_HH_

#include <list>
#include <map>
#include <net/if.h>
#include <netinet/in.h>
#include "interface.hh"
#include "libnetlink.hh"
#include <pthread.h>

/*
 * By now, there's one thread to rule just route updates.
 * TODO: Improve the code to allow a callback parameter and permit ARP updates handling.
 */
class Thread {
private:
	void * (*callback)(void *);

	void run();

	static void* helper(void* arg) {
		Thread* t = static_cast<Thread*> (arg);
		t->run();
		pthread_exit(0);
		return 0;
	}

	pthread_t m_tid;

public:
	Thread(void * (*cbFunction)(void *)) {
		callback = cbFunction;
	}

	void start() {
		pthread_create(&m_tid, 0, helper, this);
	}

	void waitTillFinished() {
		pthread_join(m_tid, 0);
	}
};

using std::map;
using std::list;

const int32_t REFRESH = 120; /* 120 ms */

/* Route information. */
class RouteEntry {
public:
	in_addr_t gwIpAddr; /* Next Hop.*/
	in_addr_t netAddr;	/* IP address.*/
	in_addr_t netMask;  /* Network mask.*/
	Interface * intf;	/* Interface. */

	bool operator==(const RouteEntry& r);
};

/*Host information */
class HostEntry {
public:
	in_addr_t ipAddr; 			   /* IP address.  */
	uint8_t m_hwAddr[IFHWADDRLEN]; /* MAC address. */
	Interface * intf;			   /* Interface.   */

	bool operator==(const HostEntry& h);
};

class FlwTablePolling {

	static struct rtnl_handle rth;
	static struct rtnl_handle rthNeigh;
	static int family;
	static unsigned groups;
	static int llink;
	static int laddr;
	static int lroute;


	static Thread HTPolling;
	static Thread RTPolling;

	static uint32_t isInit;
	static uint32_t HTLock;
	static uint32_t RTLock;

	static list<RouteEntry> routeTable;
	static list<HostEntry> hostTable;


	static int32_t addFlowToHw(const RouteEntry& route);
	static int32_t addFlowToHw(const HostEntry& host);
	static int32_t delFlowFromHw(const RouteEntry& route);
	static int32_t delFlowFromHw(const HostEntry& host);

	static int32_t strMac2Binary(const char *strMac, uint8_t *bMac);
	static int32_t maskAddr2Int(uint32_t mask);

public:


	static void * RTPollingCb(void * data);
	static void * HTPollingCb(void * data);
	static char * mac_ntoa(unsigned char *ptr);
	static void fakeReq(char *hostAddr, const char *intf);

	static int32_t init();
	static int32_t start();
	static int32_t stop();
	static void print_test();


	static int updateHostTable(const struct sockaddr_nl *who,
			struct nlmsghdr *n, void *arg);
	static int updateRouteTable(const struct sockaddr_nl *who,
			struct nlmsghdr *n, void *arg);

	static int32_t lockHostTable();
	static int32_t unlockHostTable();
	static int32_t lockRouteTable();
	static int32_t unlockRouteTable();
};

#endif /* FLWTABLE_HH_ */
