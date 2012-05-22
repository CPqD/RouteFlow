#ifndef FLOWTABLE_HH_
#define FLOWTABLE_HH_

#include <list>
#include <map>
#include <stdint.h>
#include <boost/thread.hpp>
#include "libnetlink.hh"

#include "ipc/IPC.h"
#include "types/IPAddress.h"
#include "types/MACAddress.h"

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
                
        bool operator==(const Interface& other) {
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
        
        bool operator==(const RouteEntry& other) {
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
        bool operator==(const HostEntry& other) {
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
        static void fakeReq(const char *hostAddr, const char *intf);
        static void clear();
        
        static void start(uint64_t vm_id, map<string, Interface> interfaces, IPCMessageService* ipc);
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

        static map<string, Interface> interfaces;
        static IPCMessageService* ipc;
        static uint64_t vm_id;
        
        static boost::thread HTPolling;
        static boost::thread RTPolling;

        static list<RouteEntry> routeTable;
        static list<HostEntry> hostTable;

        static int32_t addFlowToHw(const RouteEntry& route);
        static int32_t addFlowToHw(const HostEntry& host);
        static int32_t delFlowFromHw(const RouteEntry& route);
        static int32_t delFlowFromHw(const HostEntry& host);
};

#endif /* FLOWTABLE_HH_ */
