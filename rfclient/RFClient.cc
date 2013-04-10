#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <ifaddrs.h>
#include <syslog.h>
#include <cstdlib>
#include <boost/thread.hpp>
#include <iomanip>

#include "RFClient.hh"
#include "converter.h"
#include "defs.h"
#include "FlowTable.h"

#define BUFFER_SIZE 23 /* Mapping packet size. */

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
uint64_t get_interface_id(const char *ifname) {
    if (ifname == NULL)
        return 0;

    uint8_t mac[6];
    uint64_t id;
    stringstream hexmac;

    if (get_hwaddr_byname(ifname, mac) == -1)
        return 0;

    for (int i = 0; i < 6; i++)
        hexmac << std::hex << setfill ('0') << setw (2) << (int) mac[i];
    hexmac >> id;
    return id;
}

RFClient::RFClient(uint64_t id, const string &address) {
    this->id = id;
    syslog(LOG_INFO, "Starting RFClient (vm_id=%s)", to_string<uint64_t>(this->id).c_str());
    ipc = (IPCMessageService*) new MongoIPCMessageService(address, MONGO_DB_NAME, to_string<uint64_t>(this->id));

    this->init_ports = 0;
    this->load_interfaces();

    for (map<int, Interface>::iterator it = this->interfaces.begin() ; it != this->interfaces.end(); it++) {
        Interface i = it->second;
        ifacesMap[i.name] = i;

        PortRegister msg(this->id, i.port, i.hwaddress);
        this->ipc->send(RFCLIENT_RFSERVER_CHANNEL, RFSERVER_ID, msg);
        syslog(LOG_INFO, "Registering client port (vm_port=%d)", i.port);
    }

    this->startFlowTable();

    ipc->listen(RFCLIENT_RFSERVER_CHANNEL, this, this, true);
}

void RFClient::startFlowTable() {
    boost::thread t(&FlowTable::start, this->id, this->ifacesMap, this->ipc, &(this->down_ports));
    t.detach();
}

bool RFClient::process(const string &, const string &, const string &, IPCMessage& msg) {
    int type = msg.get_type();
    if (type == PORT_CONFIG) {
        PortConfig *config = dynamic_cast<PortConfig*>(&msg);
        uint32_t vm_port = config->get_vm_port();
        uint32_t operation_id = config->get_operation_id();

        if (operation_id == 0) {
            syslog(LOG_INFO,
                   "Received port configuration (vm_port=%d)",
                   vm_port);
            vector<uint32_t>::iterator it;
            for (it=down_ports.begin(); it < down_ports.end(); it++)
                if (*it == vm_port)
                    down_ports.erase(it);
            send_port_map(vm_port);
        }
        else if (operation_id == 1) {
            syslog(LOG_INFO,
                   "Received port reset (vm_port=%d)",
                   vm_port);
            down_ports.push_back(vm_port);
        }
    }
    else
        return false;

    return true;
}

int RFClient::send_packet(const char ethName[], uint64_t vm_id, uint8_t port) {
    char buffer[BUFFER_SIZE];
    uint16_t ethType;
    struct ifreq req;
    struct sockaddr_ll sll;
    uint8_t srcAddress[IFHWADDRLEN];
    uint8_t dstAddress[IFHWADDRLEN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    int SockFd = socket(PF_PACKET, SOCK_RAW, htons(RF_ETH_PROTO));

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
    ethType = htons(RF_ETH_PROTO);
    memcpy((void *) (buffer + 2 * IFHWADDRLEN), (void *) &ethType, sizeof(uint16_t));
    memcpy((void *) (buffer + 14), (void *) &vm_id, sizeof(uint64_t));
    memcpy((void *) (buffer + 22), (void *) &port, sizeof(uint8_t));
    return (sendto(SockFd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &sll, (socklen_t) addrLen));

}

/* Set the MAC address of the interface. */
int RFClient::set_hwaddr_byname(const char * ifname, uint8_t hwaddr[], int16_t flags) {
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

/* Get all names of the interfaces in the system. */
void RFClient::load_interfaces() {
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
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        family = ifa->ifa_addr->sa_family;

        if (family == AF_PACKET && strcmp(ifa->ifa_name, "eth0") != 0 && strcmp(ifa->ifa_name, "lo") != 0) {
	        get_hwaddr_byname(ifa->ifa_name, hwaddress);
	        string ifaceName = ifa->ifa_name;
	        size_t pos = ifaceName.find_first_of("123456789");
	        string port_num = ifaceName.substr(pos, ifaceName.length() - pos + 1);
	        uint32_t port_id = atoi(port_num.c_str());

	        Interface interface;
	        interface.port = port_id;
	        interface.name = ifaceName;
	        interface.hwaddress = MACAddress(hwaddress);
	        interface.active = true;

	        printf("Loaded interface %s\n", interface.name.c_str());

	        this->interfaces[interface.port] = interface;
	        intfNum++;
        }
    }

    /* Free list. */
    freeifaddrs(ifaddr);
}

void RFClient::send_port_map(uint32_t port) {
    Interface i = this->interfaces[port];
    if (send_packet(i.name.c_str(), this->id, i.port) == -1)
        syslog(LOG_INFO, "Error sending mapping packet (vm_port=%d)",
               i.port);
    else
        syslog(LOG_INFO, "Mapping packet was sent to RFVS (vm_port=%d)",
               i.port);
}

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
                    fprintf(stderr, "-n is already defined");
                    exit(EXIT_FAILURE);
                }
                id = to_string<uint64_t>(get_interface_id(optarg));
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


    openlog("rfclient", LOG_NDELAY | LOG_NOWAIT | LOG_PID, SYSLOGFACILITY);
    RFClient s(get_interface_id(DEFAULT_RFCLIENT_INTERFACE), address);

    return 0;
}
