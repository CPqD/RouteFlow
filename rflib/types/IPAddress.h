#ifndef __IPADDRESS_H__
#define __IPADDRESS_H__

#include <stdint.h>
#include <string.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sstream>
#include <string>

enum { IPV4 = 4, IPV6 = 6 };

using namespace std;

class IPAddress {
    public:
        IPAddress();
        IPAddress(const int version);
        IPAddress(const int version, const char* address);
        IPAddress(const int version, const string &address);
        IPAddress(const uint32_t data);
        IPAddress(const IPAddress &other);
        IPAddress(const int version, const uint8_t* data);
        IPAddress(const struct in_addr* data);
        IPAddress(const struct in6_addr* data);
        IPAddress(const int version, int prefix_len);
        ~IPAddress();

        IPAddress& operator=(const IPAddress& other);
        bool operator==(const IPAddress& other) const;
        void* toInAddr() const;
        void toArray(uint8_t* array) const;
        uint32_t toUint32() const;
        string toString() const;
        int toPrefixLen() const;
        int toCIDRMask() const;
        int getVersion() const;
        size_t getLength() const;

    private:
        int version;
        size_t length;
        uint8_t* data;
        void init(const int version);
        void data_from_string(const string &address);
};

#endif /* __IPADDRESS_H__ */
