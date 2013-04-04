#ifndef FLOWTABLE_HH_
#define FLOWTABLE_HH_

#include <list>
#include <map>
#include <stdint.h>
#include <boost/thread.hpp>
#include "libnetlink.hh"
#include "SyncQueue.h"

#include "ipc/IPC.h"
#include "types/IPAddress.h"
#include "types/MACAddress.h"
#include "defs.h"

using namespace std;

class Interface {
    public:
        uint32_t port;
        string name;
        IPAddress address;
        IPAddress netmask;
        MACAddress hwaddress;
        bool active;

        Interface() {
            this->active = false;
        }

        Interface& operator=(const Interface& other) {
            if (this != &other) {
                this->port = other.port;
                this->name = other.name;
                this->address = other.address;
                this->netmask = other.netmask;
                this->hwaddress = other.hwaddress;
                this->active = other.active;
            }
            return *this;
        }

        bool operator==(const Interface& other) const {
            return
                (this->port == other.port) and
                (this->name == other.name) and
                (this->address == other.address) and
                (this->netmask == other.netmask) and
                (this->hwaddress == other.hwaddress) and
                (this->active == other.active);
        }
};

class RouteEntry {
    public:
        IPAddress address;
        IPAddress gateway;
        IPAddress netmask;
        Interface interface;

        bool operator==(const RouteEntry& other) const {
            return (this->address == other.address) and
                (this->gateway == other.gateway) and
                (this->netmask == other.netmask) and
                (this->interface == other.interface);
        }
};

class HostEntry {
    public:
        IPAddress address;
        MACAddress hwaddress;
        Interface interface;

        bool operator==(const HostEntry& other) const {
            return (this->address == other.address) and
                (this->hwaddress == other.hwaddress) and
                (this->interface == other.interface);
        }
};

// TODO: recreate this module from scratch without all the static stuff.
// It is a little bit challenging to devise a decent API due to netlink
class FlowTable {
    public:
        static void RTPollingCb();
        static void HTPollingCb();
        static void GWResolverCb();
        static void fakeReq(const char *hostAddr, const char *intf);
        static const MACAddress& getGateway(const IPAddress&, const Interface&);

        static void clear();
        static void interrupt();
        static void start(uint64_t vm_id, map<string, Interface> interfaces, IPCMessageService* ipc, vector<uint32_t>* down_ports);
        static void print_test();

        static int updateHostTable(const struct sockaddr_nl *who, struct nlmsghdr *n, void *arg);
        static int updateRouteTable(const struct sockaddr_nl *who, struct nlmsghdr *n, void *arg);

    private:
        static struct rtnl_handle rth;
        static struct rtnl_handle rthNeigh;
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

        static boost::thread HTPolling;
        static boost::thread RTPolling;
        static boost::thread GWResolver;

        static SyncQueue< std::pair<RouteModType,RouteEntry> > pendingRoutes;
        static list<RouteEntry> routeTable;
        static list<HostEntry> hostTable;

        static bool is_port_down(uint32_t port);
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
