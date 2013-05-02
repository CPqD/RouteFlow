#ifndef FLOWTABLE_HH_
#define FLOWTABLE_HH_

#include <list>
#include <map>
#include <stdint.h>
#include <boost/thread.hpp>
#include "libnetlink.hh"
#include "SyncQueue.h"

#include "fpm.h"
#include "fpm_lsp.h"

#include "ipc/IPC.h"
#include "ipc/RFProtocol.h"
#include "types/IPAddress.h"
#include "types/MACAddress.h"
#include "defs.h"

#include "Interface.hh"
#include "RouteEntry.hh"
#include "HostEntry.hh"

using namespace std;

// TODO: recreate this module from scratch without all the static stuff.
// It is a little bit challenging to devise a decent API due to netlink
class FlowTable {
    public:
        static void HTPollingCb();
        static void GWResolverCb();

        static void clear();
        static void interrupt();
        static void start(uint64_t vm_id, map<string, Interface> interfaces, IPCMessageService* ipc, vector<uint32_t>* down_ports);
        static void print_test();

        static int updateHostTable(const struct sockaddr_nl*,
                                   struct nlmsghdr*, void*);
        static int updateRouteTable(struct nlmsghdr *n);

#ifdef FPM_ENABLED
        static void updateNHLFE(nhlfe_msg_t *nhlfe_msg);
#else
        static void RTPollingCb();
        static int updateRouteTable(const struct sockaddr_nl*,
                                    struct nlmsghdr*, void*);
#endif /* FPM_ENABLED */

    private:
        static int family;
        static unsigned groups;
        static int llink;
        static int laddr;
        static int lroute;

        static const MACAddress MAC_ADDR_NONE;
        static map<string, Interface> interfaces;
        static vector<uint32_t>* down_ports;
        static IPCMessageService* ipc;
        static uint64_t vm_id;

        static boost::thread GWResolver;
        static boost::thread HTPolling;
        static struct rtnl_handle rthNeigh;

#ifdef FPM_ENABLED
        static boost::thread FPMClient;
#else
        static boost::thread RTPolling;
        static struct rtnl_handle rth;
#endif /* FPM_ENABLED */

        static SyncQueue< std::pair<RouteModType,RouteEntry> > pendingRoutes;
        static list<RouteEntry> routeTable;
        static map<string, HostEntry> hostTable;
        static map<string, int> pendingNeighbours;

        static bool is_port_down(uint32_t port);
        static int getInterface(const char *intf, const char *type,
                                Interface& iface);

        static int initiateND(const char *hostAddr);
        static int resolveGateway(const IPAddress&, const Interface&);
        static const MACAddress& findHost(const IPAddress& host);

        static int setEthernet(RouteMod& rm, const Interface& local_iface,
                               const MACAddress& gateway);
        static int setIP(RouteMod& rm, const IPAddress& addr,
                         const IPAddress& mask);
        static int sendToHw(RouteModType, const RouteEntry&);
        static int sendToHw(RouteModType, const HostEntry&);
        static int sendToHw(RouteModType, const IPAddress& addr,
                            const IPAddress& mask, const Interface&,
                            const MACAddress& gateway);
};

#endif /* FLOWTABLE_HH_ */
