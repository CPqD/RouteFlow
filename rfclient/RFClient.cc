#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <ifaddrs.h>
#include <syslog.h>
#include <cstdlib>
#include <map>
#include <vector>
#include <boost/thread.hpp>

#include "ipc/IPC.h"
#include "ipc/MongoIPC.h"
#include "ipc/RFProtocol.h"
#include "ipc/RFProtocolFactory.h"
#include "defs.h"
#include "converter.h"

#include "FlowTable.h"

#define BUFFER_SIZE 23 /* Mapping packet size. */
#define ETH_P_RFP 0x0A0A /* RF protocol. */

using namespace std;

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

/* Get the interface associated VM identification number. */
uint64_t get_vmId(const char *ifname) {
    if (ifname == NULL)
        return 0;
        
    uint8_t mac[6];
    get_hwaddr_byname(ifname, mac);
    stringstream vmId;
    vmId << (int) mac[0] << (int) mac[1] << (int) mac[2] << (int) mac[3] << (int) mac[4] << (int) mac[5];
    int64_t dVmId = atoll(vmId.str().c_str());

    return (dVmId > 0) ? dVmId : 0;
}

class RFClient : private RFProtocolFactory, private IPCMessageProcessor {
    public:
        RFClient(const string &id, const string &address) {
            this->id = id;
            this->gVmId = string_to<uint64_t>(this->id); // TODO: turn into id
            this->is_init = false;
            
            ipc = (IPCMessageService*) new MongoIPCMessageService(address, MONGO_DB_NAME, this->id);
            syslog(LOG_INFO, "Creating client id=%s", this->id.c_str());
        }

        void start() {
            syslog(LOG_INFO, "Attemping to register with server");
            VMRegisterRequest request(this->gVmId);
            ipc->send(RFCLIENT_RFSERVER_CHANNEL, RFSERVER_ID, request);
            syslog(LOG_INFO, "Listening for messages from server...");
            ipc->listen(RFCLIENT_RFSERVER_CHANNEL, this, this, true);
        }

        void startFlowTable() {
            boost::thread t(&FlowTable::start, this->gVmId, this->ifacesMap, this->ipc);
            t.detach();
        }

    private:
        FlowTable* flowTable;
        IPCMessageService* ipc;
        string id;

        map<string, Interface> ifacesMap;
        uint64_t gVmId;
        uint8_t gVmMAC[IFHWADDRLEN];
        bool is_init;
        
        bool process(const string &from, const string &to, const string &channel, IPCMessage& msg) {
            int type = msg.get_type();
            if (type == VM_REGISTER_RESPONSE) {
                VMRegisterResponse *response = dynamic_cast<VMRegisterResponse*>(&msg);
                if (response->get_accept()) {
            		syslog(LOG_INFO, "VM is now registered");
                }
                else {
                    syslog(LOG_ERR, "VM registration denied");
                    exit(EXIT_FAILURE);
                }
            }
            else if (type == VM_CONFIG) {
                VMConfig *config = dynamic_cast<VMConfig*>(&msg);
                this->ifacesMap.clear();
                this->flowTable->clear();
                init_Interfaces();
                if (not this->is_init) {
                    syslog(LOG_INFO, "Received config message from server");
                    // TODO: add support for number of ports
                    this->startFlowTable();
                    this->is_init = true;
                }
            }
            else
                // This message is not processed by this processor
                return false;
                
            // The message was successfully processed
            return true;
        }

        int send_packet(char ethName[], uint8_t srcAddress[IFHWADDRLEN], uint8_t port, uint64_t VMid) {
	        char buffer[BUFFER_SIZE];
	        uint16_t ethType;
	        struct sockaddr_ll sll;
	        struct ifreq req;
	        uint8_t dstAddress[IFHWADDRLEN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

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
	        for (i = 0; i < IFHWADDRLEN; i++)
		        srcAddress[i] = (uint8_t) req.ifr_hwaddr.sa_data[i];

	        memset(&sll, 0, sizeof(struct sockaddr_ll));
	        sll.sll_family = PF_PACKET;
	        sll.sll_ifindex = ifindex;

	        if (bind(SockFd, (struct sockaddr *) &sll, addrLen) < 0) {
		        fprintf(stderr, "ERROR! bind() call has failed: %s\n", strerror(errno));
		        exit(1);
	        }

	        memset(buffer, 0, BUFFER_SIZE);

	        memcpy((void *) buffer, (void *) dstAddress, IFHWADDRLEN);
	        memcpy((void *) (buffer + IFHWADDRLEN), (void *) srcAddress, IFHWADDRLEN);
	        ethType = htons(ETH_P_RFP);
	        memcpy((void *) (buffer + 2 * IFHWADDRLEN), (void *) &ethType,
			        sizeof(uint16_t));
	        memcpy((void *) (buffer + 14), (void *) &VMid, sizeof(uint64_t));
	        memcpy((void *) (buffer + 22), (void *) &port, sizeof(uint8_t));
            syslog(LOG_INFO, "Sending mapping packet for interface name=%s", ethName);
	        return (sendto(SockFd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &sll, (socklen_t) addrLen));

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

        /* Get all names of the interfaces in the system. */
        std::vector<Interface> get_Interfaces() {

	        struct ifaddrs *ifaddr, *ifa;
	        int family;
	        int intfNum;

	        if (getifaddrs(&ifaddr) == -1) {
		        perror("getifaddrs");
		        exit( EXIT_FAILURE);
	        }

	        /* Walk through linked list, maintaining head pointer so we
	         can free list later. */
	        intfNum = 0;
	        std::vector<Interface> interfaces;
	        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		        family = ifa->ifa_addr->sa_family;

		        if (family == AF_PACKET && strcmp(ifa->ifa_name, "eth0") != 0 && strcmp(ifa->ifa_name, "lo") != 0) {
			        if (0 == intfNum) {
				        get_hwaddr_byname(ifa->ifa_name, gVmMAC);
			        } else {
				        set_hwaddr_byname(ifa->ifa_name, gVmMAC, ifa->ifa_flags);
				        syslog(LOG_INFO, "Setting MAC Addr (%s)", ifa->ifa_name);
			        }
			        string ifaceName = ifa->ifa_name;
			        size_t pos = ifaceName.find_first_of("123456789");
			        string port_num = ifaceName.substr(pos, ifaceName.length() - pos + 1);
			        uint32_t port_id = atoi(port_num.c_str());

			        Interface interface;
			        interface.port = port_id;
			        interface.name = ifaceName;
			        interface.hwaddress = MACAddress(gVmMAC);
			        interface.active = true;

			        printf("Interface %s\n", interface.name.c_str());

			        interfaces.push_back(interface);
			        intfNum++;

			        if (send_packet(ifa->ifa_name, gVmMAC, port_id, gVmId) == -1)
				        syslog(LOG_INFO, "send_packet failed");
		        }
	        }

	        /* Free list. */
	        freeifaddrs(ifaddr);
	        return interfaces;
        }

        int init_Interfaces() {
	        vector<Interface> interfaces = get_Interfaces();
	        unsigned int i;
	        for (i = 0; i < interfaces.size(); i++) {
		        ifacesMap[interfaces.at(i).name] = (Interface) interfaces.at(i);
	        }

	        return 0;
        }
};

// -----------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    char c;
    stringstream ss;
    string id;
    string address = MONGO_ADDRESS;
    while ((c = getopt (argc, argv, "n:i:a:")) != -1)
        switch (c) {
            case 'n':
                fprintf (stderr, "Custom naming not supported yet.");
                exit(EXIT_FAILURE);
                /* TODO: support custom naming for VMs.
                if (!id.empty()) {
                    fprintf (stderr, "-i is already defined");
                    exit(EXIT_FAILURE);
                }
                id = optarg;
                */
                break;
            case 'i':
                if (!id.empty()) {
                    fprintf (stderr, "-n is already defined");
                    exit(EXIT_FAILURE);
                }
                id = to_string<uint64_t>(get_vmId(optarg));
                break;
            case 'a':
                address = optarg;
                break;
            case '?':
                if (optopt == 'n' || optopt == 'i' || optopt == 'a')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                return EXIT_FAILURE;
            default:
                abort();
        }

    // If no ID is given, get it from the default NIC MAC address.
    if (id.empty()) {
        ss << get_vmId(DEFAULT_RFCLIENT_INTERFACE);
        id = ss.str();
    }

    openlog("rfclient", LOG_NDELAY | LOG_NOWAIT | LOG_PID, SYSLOGFACILITY);

    RFClient s(id, address);
    s.start();

    return 0;
}
