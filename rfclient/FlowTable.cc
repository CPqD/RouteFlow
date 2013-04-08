#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ether.h>
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

#define FULL_IPV4_PREFIX 32
#define FULL_IPV6_PREFIX 128

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

boost::mutex ndMutex;
map<string, int> FlowTable::pendingNeighbours;

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

        PendingRoute pr;
        FlowTable::pendingRoutes.wait_and_pop(pr);

        RouteEntry* existingEntry = NULL;
        std::list<RouteEntry>::iterator iter = FlowTable::routeTable.begin();
        for (; iter != FlowTable::routeTable.end(); iter++) {
            if (pr.second == (*iter)) {
                existingEntry = &(*iter);
                break;
            }
        }

        if (existingEntry != NULL && pr.first == RMT_ADD) {
            fprintf(stdout, "Received duplicate route addition for route %s\n",
                    existingEntry->address.toString().c_str());
            continue;
        }

        if (existingEntry == NULL && pr.first == RMT_DELETE) {
            fprintf(stdout, "Received route removal for %s but route %s.\n",
                    pr.second.address.toString().c_str(), "cannot be found");
            continue;
        }

        /* If we can't resolve the gateway, put it to the end of the queue.
         * Routes with unresolvable gateways will constantly trigger this code,
         * popping and re-pushing. */
        const RouteEntry& re = pr.second;
        if (resolveGateway(re.gateway, re.interface) < 0) {
            fprintf(stderr, "An error occurred while %s %s/%s.\n",
                    "attempting to resolve", re.address.toString().c_str(),
                    re.netmask.toString().c_str());
            FlowTable::pendingRoutes.push(pr);
            continue;
        }

        if (FlowTable::sendToHw(pr.first, pr.second) < 0) {
            fprintf(stderr, "An error occurred while pushing route %s/%s.\n",
                    re.address.toString().c_str(),
                    re.netmask.toString().c_str());
            FlowTable::pendingRoutes.push(pr);
            continue;
        }

        if (pr.first == RMT_ADD) {
            FlowTable::routeTable.push_back(pr.second);
        } else if (pr.first == RMT_DELETE) {
            FlowTable::routeTable.remove(pr.second);
        } else {
            fprintf(stderr, "Received unexpected RouteModType (%d)\n", pr.first);
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

            string host = hentry.address.toString();
            {
                // Add to host table
                boost::lock_guard<boost::mutex> lock(hostTableMutex);
                FlowTable::hostTable[host] = hentry;
            }
            {
                // If we have been attempting neighbour discovery for this
                // host, then we can close the associated socket.
                boost::lock_guard<boost::mutex> lock(ndMutex);
                map<string, int>::iterator iter = pendingNeighbours.find(host);
                if (iter != pendingNeighbours.end()) {
                    if (close(iter->second) == -1) {
                        perror("pendingNeighbours");
                    }
                    pendingNeighbours.erase(host);
                }
            }

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

/**
 * Begins the neighbour discovery process to the specified host.
 *
 * Returns an open socket on success, or -1 on error.
 */
int FlowTable::initiateND(const char *hostAddr) {
    int s, flags;
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(sin));

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(hostAddr);

    if (sin.sin_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "Invalid IP address for resolution. Dropping\n");
    }

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() failed");
        return -1;
    }

    // Prevent the connect() call from blocking
    flags = fcntl(s, F_GETFL, 0);
    if (fcntl(s, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl() failed");
        close(s);
        return -1;
    }

    connect(s, (struct sockaddr *)&sin, sizeof(struct sockaddr));
    return s;
}

/**
 * Initiates the gateway resolution process for the given host.
 *
 * Returns:
 *  0 if address resolution is currently being performed
 * -1 on error (usually an issue with the socket)
 */
int FlowTable::resolveGateway(const IPAddress& gateway,
                              const Interface& iface) {
    if (is_port_down(iface.port)) {
        return -1;
    }

    string gateway_str = gateway.toString();

    // If we already initiated neighbour discovery for this gateway, return.
    boost::lock_guard<boost::mutex> lock(ndMutex);
    if (pendingNeighbours.find(gateway_str) != pendingNeighbours.end()) {
        return 0;
    }

    // Otherwise, we should go ahead and begin the process.
    int sock = initiateND(gateway_str.c_str());
    if (sock == -1) {
        return -1;
    }
    FlowTable::pendingNeighbours[gateway_str] = sock;

    return 0;
}

/**
 * Find the MAC Address for the given host in a thread-safe manner.
 *
 * This searches the internal hostTable structure for the given host, and
 * returns its MAC Address. If the host is unresolved, this will return
 * FlowTable::MAC_ADDR_NONE. Neighbour Discovery is not performed by this
 * function.
 */
const MACAddress& FlowTable::findHost(const IPAddress& host) {
    boost::lock_guard<boost::mutex> lock(hostTableMutex);
    map<string, HostEntry>::iterator iter;
    iter = FlowTable::hostTable.find(host.toString());
    if (iter != FlowTable::hostTable.end()) {
        return iter->second.hwaddress;
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
    /* Theoretically we would need N*M rules in OpenFlow 1.0 to correctly
     * match on all switch MAC addresses (N) and L3 routes (M). To avoid this
     * issue, we simply don't match on MAC addresses. This reduces the number
     * of flows required on a switch to ~M. */
    //rm.add_match(Match(RFMT_ETHERNET, local_iface.hwaddress));

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
        const MACAddress& remoteMac = findHost(re.gateway);
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
        mask.reset(new IPAddress(IPV6, FULL_IPV6_PREFIX));
    } else if (he.address.getVersion() == IPV4) {
        mask.reset(new IPAddress(IPV4, FULL_IPV4_PREFIX));
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
