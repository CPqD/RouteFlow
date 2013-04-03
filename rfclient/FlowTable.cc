#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/ether.h>
#include <netdb.h>
#include <sys/socket.h>
#include <time.h>

#include <string>
#include <vector>
#include <cstring>
#include <iostream>

#include "ipc/RFProtocol.h"
#include "converter.h"
#include "FlowTable.h"

using namespace std;

#define FULL_IPV4_MASK ((in_addr){ 0xffffffff })
#define FULL_CIDR_MASK 32
#define FULL_IPV6_MASK ((in6_addr){{{ 0xff, 0xff, 0xff, 0xff, \
                                      0xff, 0xff, 0xff, 0xff, \
                                      0xff, 0xff, 0xff, 0xff, \
                                      0xff, 0xff, 0xff, 0xff }}})
#define EMPTY_MAC_ADDRESS "00:00:00:00:00:00"

const MACAddress FlowTable::MAC_ADDR_NONE(EMPTY_MAC_ADDRESS);

int FlowTable::family = AF_UNSPEC;
unsigned FlowTable::groups = ~0U;
int FlowTable::llink = 0;
int FlowTable::laddr = 0;
int FlowTable::lroute = 0;

struct rtnl_handle FlowTable::rthNeigh;
boost::thread FlowTable::HTPolling;
struct rtnl_handle FlowTable::rth;
boost::thread FlowTable::RTPolling;
boost::thread FlowTable::GWResolver;

map<string, Interface> FlowTable::interfaces;
vector<uint32_t>* FlowTable::down_ports;
IPCMessageService* FlowTable::ipc;
uint64_t FlowTable::vm_id;

typedef std::pair<RouteModType,RouteEntry> PendingRoute;
SyncQueue<PendingRoute> FlowTable::pendingRoutes;
list<RouteEntry> FlowTable::routeTable;
boost::mutex hostTableMutex;
map<string, HostEntry> FlowTable::hostTable;

// TODO: implement a way to pause the flow table updates when the VM is not
//       associated with a valid datapath

void FlowTable::HTPollingCb() {
    rtnl_listen(&rthNeigh, FlowTable::updateHostTable, NULL);
}

void FlowTable::RTPollingCb() {
    rtnl_listen(&rth, FlowTable::updateRouteTable, NULL);
}

void FlowTable::start(uint64_t vm_id, map<string, Interface> interfaces,
                      IPCMessageService* ipc, vector<uint32_t>* down_ports) {
    FlowTable::vm_id = vm_id;
    FlowTable::interfaces = interfaces;
    FlowTable::ipc = ipc;
    FlowTable::down_ports = down_ports;

    rtnl_open(&rth, RTMGRP_IPV4_MROUTE | RTMGRP_IPV4_ROUTE
                  | RTMGRP_IPV6_MROUTE | RTMGRP_IPV6_ROUTE);
    rtnl_open(&rthNeigh, RTMGRP_NEIGH);

    HTPolling = boost::thread(&FlowTable::HTPollingCb);
    RTPolling = boost::thread(&FlowTable::RTPollingCb);
    GWResolver = boost::thread(&FlowTable::GWResolverCb);

    GWResolver.join();
}

void FlowTable::clear() {
    FlowTable::routeTable.clear();
    boost::lock_guard<boost::mutex> lock(hostTableMutex);
    FlowTable::hostTable.clear();
}

void FlowTable::interrupt() {
    HTPolling.interrupt();
    RTPolling.interrupt();
    GWResolver.interrupt();
}

void FlowTable::GWResolverCb() {
    while (true) {
        boost::this_thread::interruption_point();

        PendingRoute re;
        FlowTable::pendingRoutes.wait_and_pop(re);

        RouteEntry* existingEntry = NULL;
        std::list<RouteEntry>::iterator itRoutes = FlowTable::routeTable.begin();
        for (; itRoutes != FlowTable::routeTable.end(); itRoutes++) {
            if (re.second == (*itRoutes)) {
                existingEntry = &(*itRoutes);
                break;
            }
        }

        if (existingEntry != NULL && re.first == RMT_ADD) {
            fprintf(stdout, "Received duplicate route addition\n");
            continue;
        }

        if (existingEntry == NULL && re.first == RMT_DELETE) {
            fprintf(stdout, "Received route removal but route cannot be found.\n");
            continue;
        }

        /* If we can't resolve the gateway, put it to the end of the queue. */
        if (FlowTable::sendToHw(re.first, re.second) != 0) {
            FlowTable::pendingRoutes.push(re);
            continue;
        }

        if (re.first == RMT_ADD) {
            FlowTable::routeTable.push_back(re.second);
        } else if (re.first == RMT_DELETE) {
            FlowTable::routeTable.remove(re.second);
        } else {
            fprintf(stderr, "Received unexpected RouteModType (%d)\n", re.first);
        }
    }
}

/**
 * Get the local interface corresponding to the given interface number.
 *
 * On success, overwrites given interface pointer with the active interface
 * and returns 0;
 * On error, prints to stderr with appropriate message and returns -1.
 */
int FlowTable::getInterface(const char *intf, const char *type,
                            Interface* iface) {
    map<string, Interface>::iterator it = interfaces.find(intf);

    if (it == interfaces.end()) {
        fprintf(stderr, "Interface %s not found, dropping %s entry\n",
                intf, type);
        return -1;
    }

    if (not it->second.active) {
        fprintf(stderr, "Interface %s inactive, dropping %s entry\n",
                intf, type);
        return -1;
    }

    *iface = it->second;
    return 0;
}

int FlowTable::updateHostTable(const struct sockaddr_nl *, struct nlmsghdr *n, void *) {
    struct ndmsg *ndmsg_ptr = (struct ndmsg *) NLMSG_DATA(n);
    struct rtattr *rtattr_ptr;

    char intf[IF_NAMESIZE + 1];
    memset(intf, 0, IF_NAMESIZE + 1);

    boost::this_thread::interruption_point();

    if (if_indextoname((unsigned int) ndmsg_ptr->ndm_ifindex, (char *) intf) == NULL) {
        perror("HostTable");
        return 0;
    }

    /*
    if (ndmsg_ptr->ndm_state != NUD_REACHABLE) {
        cout << "ndm_state: " << (uint16_t) ndmsg_ptr->ndm_state << endl;
        return 0;
    }
    */

    char ip[INET_ADDRSTRLEN];
    char mac[2 * IFHWADDRLEN + 5 + 1];

    memset(ip, 0, INET_ADDRSTRLEN);
    memset(mac, 0, 2 * IFHWADDRLEN + 5 + 1);

    rtattr_ptr = (struct rtattr *) RTM_RTA(ndmsg_ptr);
    int rtmsg_len = RTM_PAYLOAD(n);

    for (; RTA_OK(rtattr_ptr, rtmsg_len); rtattr_ptr = RTA_NEXT(rtattr_ptr, rtmsg_len)) {
        switch (rtattr_ptr->rta_type) {
        case RTA_DST:
            if (inet_ntop(AF_INET, RTA_DATA(rtattr_ptr), ip, 128) == NULL) {
                perror("HostTable");
                return 0;
            }
            break;
        case NDA_LLADDR:
            if (strncpy(mac, ether_ntoa(((ether_addr *) RTA_DATA(rtattr_ptr))), sizeof(mac)) == NULL) {
                perror("HostTable");
                return 0;
            }
            break;
        default:
            break;
        }
    }

    HostEntry hentry;

    hentry.address = IPAddress(IPV4, ip);
    hentry.hwaddress = MACAddress(mac);
    if (getInterface(intf, "host", &hentry.interface) != 0) {
        return 0;
    }

    if (strlen(mac) == 0) {
        fprintf(stderr, "Received host entry with blank mac. Ignoring\n");
        return 0;
    }

    switch (n->nlmsg_type) {
        case RTM_NEWNEIGH: {
            FlowTable::sendToHw(RMT_ADD, hentry);
            boost::lock_guard<boost::mutex> lock(hostTableMutex);
            FlowTable::hostTable[hentry.address.toString()] = hentry;

            std::cout << "netlink->RTM_NEWNEIGH: ip=" << ip << ", mac=" << mac
                      << std::endl;
            break;
        }
        /* TODO: enable this? It is causing serious problems. Why?
        case RTM_DELNEIGH: {
            std::cout << "netlink->RTM_DELNEIGH: ip=" << ip << ", mac=" << mac << std::endl;
            FlowTable::sendToHw(RMT_DELETE, hentry);
            // TODO: delete from hostTable
            boost::lock_guard<boost::mutex> lock(hostTableMutex);
            break;
        }
        */
    }

    return 0;
}

int FlowTable::updateRouteTable(const struct sockaddr_nl *, struct nlmsghdr *n, void *) {
    struct rtmsg *rtmsg_ptr = (struct rtmsg *) NLMSG_DATA(n);

    boost::this_thread::interruption_point();

    if (!((n->nlmsg_type == RTM_NEWROUTE || n->nlmsg_type == RTM_DELROUTE) && rtmsg_ptr->rtm_table == RT_TABLE_MAIN)) {
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

    for (; RTA_OK(rtattr_ptr, rtmsg_len); rtattr_ptr = RTA_NEXT(rtattr_ptr, rtmsg_len)) {
        switch (rtattr_ptr->rta_type) {
        case RTA_DST:
            inet_ntop(AF_INET, RTA_DATA(rtattr_ptr), net, 128);
            break;
        case RTA_GATEWAY:
            inet_ntop(AF_INET, RTA_DATA(rtattr_ptr), gw, 128);
            break;
        case RTA_OIF:
            if_indextoname(*((int *) RTA_DATA(rtattr_ptr)), (char *) intf);
            break;
        case RTA_MULTIPATH: {
            struct rtnexthop *rtnhp_ptr = (struct rtnexthop *) RTA_DATA(
                    rtattr_ptr);
            int rtnhp_len = RTA_PAYLOAD(rtattr_ptr);

            if (rtnhp_len < (int) sizeof(*rtnhp_ptr)) {
                break;
            }

            if (rtnhp_ptr->rtnh_len > rtnhp_len) {
                break;
            }

            if_indextoname(rtnhp_ptr->rtnh_ifindex, (char *) intf);

            int attrlen = rtnhp_len - sizeof(struct rtnexthop);

            if (attrlen) {
                struct rtattr *attr = RTNH_DATA(rtnhp_ptr);

                for (; RTA_OK(attr, attrlen); attr = RTA_NEXT(attr, attrlen))
                    if ((attr->rta_type == RTA_GATEWAY)) {
                        inet_ntop(AF_INET, RTA_DATA(attr), gw, 128);
                        break;
                    }
            }
        }
            break;
        default:
            break;
        }
    }

    /* Skipping routes to directly attached networks (next-hop field is blank) */
    {
        struct in_addr gwAddr;
        if (inet_aton(gw, &gwAddr) == 0) {
            fprintf(stderr, "Blank next-hop field. Dropping Route\n");
            return 0;
        }
    }

    struct in_addr convmask;
    convmask.s_addr = htonl(~((1 << (32 - rtmsg_ptr->rtm_dst_len)) - 1));
    char mask[INET_ADDRSTRLEN];
    snprintf(mask, sizeof(mask), "%s", inet_ntoa(convmask));

    RouteEntry rentry;

    rentry.address = IPAddress(IPV4, net);
    rentry.gateway = IPAddress(IPV4, gw);
    rentry.netmask = IPAddress(IPV4, mask);
    if (getInterface(intf, "route", &rentry.interface) != 0) {
        return 0;
    }

    // Discard if there's no gateway (IPv4-only)
    if (inet_addr(gw) == INADDR_NONE) {
        fprintf(stderr, "No gateway specified, dropping route entry\n");
        return 0;
    }

    switch (n->nlmsg_type) {
        case RTM_NEWROUTE:
            std::cout << "netlink->RTM_NEWROUTE: net=" << net << ", mask="
                      << mask << ", gw=" << gw << std::endl;
            FlowTable::pendingRoutes.push(PendingRoute(RMT_ADD, rentry));
            break;
        case RTM_DELROUTE:
            std::cout << "netlink->RTM_DELROUTE: net=" << net << ", mask="
                      << mask << ", gw=" << gw << std::endl;
            FlowTable::pendingRoutes.push(PendingRoute(RMT_DELETE, rentry));
            break;
    }

    return 0;
}

void FlowTable::fakeReq(const char *hostAddr, const char *intf) {
    int s;
    struct arpreq req;
    struct hostent *hp;
    struct sockaddr_in *sin;

    memset(&req, 0, sizeof(req));

    sin = (struct sockaddr_in *) &req.arp_pa;
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = inet_addr(hostAddr);

    // Cast to eliminate warning. in_addr.s_addr is uint32_t (netinet/in.h:141)
    if (sin->sin_addr.s_addr == (uint32_t) -1) {
        if (!(hp = gethostbyname(hostAddr))) {
            fprintf(stderr, "ARP: %s ", hostAddr);
            perror(NULL);
            return;
        }
        memcpy(&sin->sin_addr, hp->h_addr, sizeof(sin->sin_addr));
    }

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() failed");
        return;
    }

    connect(s, (struct sockaddr *) sin, sizeof(struct sockaddr));
    close(s);
}

const MACAddress& FlowTable::getGateway(const IPAddress& gateway,
                                        const Interface& iface) {
    if (is_port_down(iface.port)) {
        return FlowTable::MAC_ADDR_NONE;
    }

    // We need to resolve the gateway's IP in order to install a route flow.
    // The MAC address of the next-hop is required as it is used to re-write
    // the layer 2 header before forwarding the packet.
    for (int tries = 0; tries < 50; tries++) {
        boost::lock_guard<boost::mutex> lock(hostTableMutex);
        map<string, HostEntry>::iterator iter;
        iter = FlowTable::hostTable.find(gateway.toString());
        if (iter != FlowTable::hostTable.end()) {
            return (iter->second.hwaddress);
        }

        FlowTable::fakeReq(gateway.toString().c_str(), iface.name.c_str());
        struct timespec sleep = {0, 20000000}; // 20ms
        nanosleep(&sleep, NULL);
    }

    return FlowTable::MAC_ADDR_NONE;
}

bool FlowTable::is_port_down(uint32_t port) {
    vector<uint32_t>::iterator it;
    for (it=down_ports->begin() ; it < down_ports->end(); it++)
        if (*it == port)
            return true;
    return false;
}

int FlowTable::setEthernet(RouteMod& rm, const Interface& local_iface,
                           const MACAddress& gateway) {
    rm.add_match(Match(RFMT_ETHERNET, local_iface.hwaddress));

    if (rm.get_mod() != RMT_DELETE) {
        rm.add_action(Action(RFAT_SET_ETH_SRC, local_iface.hwaddress));
        rm.add_action(Action(RFAT_SET_ETH_DST, gateway));
    }

    return 0;
}

int FlowTable::setIP(RouteMod& rm, const IPAddress& addr,
                     const IPAddress& mask) {
     if (addr.getVersion() == IPV4) {
        rm.add_match(Match(RFMT_IPV4, addr, mask));
    } else if (addr.getVersion() == IPV6) {
        rm.add_match(Match(RFMT_IPV6, addr, mask));
    } else {
        fprintf(stderr, "Cannot send route with unsupported IP version\n");
        return -1;
    }

    uint16_t priority = DEFAULT_PRIORITY;
    priority += static_cast<uint16_t>(mask.toCIDRMask());
    rm.add_option(Option(RFOT_PRIORITY, priority));

    return 0;
}

int FlowTable::sendToHw(RouteModType mod, const RouteEntry& re) {
    const string gateway_str = re.gateway.toString();
    if (mod == RMT_DELETE) {
        return sendToHw(mod, re.address, re.netmask, re.interface,
                        FlowTable::MAC_ADDR_NONE);
    } else if (mod == RMT_ADD) {
        const MACAddress& remoteMac = getGateway(re.gateway, re.interface);
        if (remoteMac == FlowTable::MAC_ADDR_NONE) {
            fprintf(stderr, "Cannot Resolve %s\n", gateway_str.c_str());
            return -1;
        }

        return sendToHw(mod, re.address, re.netmask, re.interface, remoteMac);
    }

    fprintf(stderr, "Unhandled RouteModType (%d)\n", mod);
    return -1;
}

int FlowTable::sendToHw(RouteModType mod, const HostEntry& he) {
    boost::scoped_ptr<IPAddress> mask;

    if (he.address.getVersion() == IPV6) {
        mask.reset(new IPAddress(FULL_IPV6_MASK));
    } else if (he.address.getVersion() == IPV4) {
        mask.reset(new IPAddress(FULL_IPV4_MASK));
    } else {
        fprintf(stderr, "Received HostEntry with unsupported IP version\n");
        return -1;
    }

    return sendToHw(mod, he.address, *mask.get(), he.interface, he.hwaddress);
}

int FlowTable::sendToHw(RouteModType mod, const IPAddress& addr,
                         const IPAddress& mask, const Interface& local_iface,
                         const MACAddress& gateway) {
    if (is_port_down(local_iface.port)) {
        fprintf(stderr, "Cannot send RouteMod for down port\n");
        return -1;
    }

    RouteMod rm;

    rm.set_mod(mod);
    rm.set_id(FlowTable::vm_id);

    if (setEthernet(rm, local_iface, gateway) != 0) {
        return -1;
    }
    if (setIP(rm, addr, mask) != 0) {
        return -1;
    }

    /* Add the output port. Even if we're removing the route, RFServer requires
     * the port to determine which datapath to send to. */
    rm.add_action(Action(RFAT_OUTPUT, local_iface.port));

    FlowTable::ipc->send(RFCLIENT_RFSERVER_CHANNEL, RFSERVER_ID, rm);
    return 0;
}
