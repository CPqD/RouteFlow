#ifndef __MACADDRESS_H__
#define __MACADDRESS_H__

#include <stdint.h>
#include <string.h>
#include <net/if.h>

#include <sstream>
#include <iomanip>
#include <string>

using namespace std;

class MACAddress {
    public:
        MACAddress();
        MACAddress(const char* address);
        MACAddress(const string &address);
        MACAddress(const MACAddress &other);
        MACAddress(const uint8_t* data);
        
        MACAddress& operator=(const MACAddress &other);
        bool operator==(const MACAddress &other) const;
        void toArray(uint8_t* array) const;
        string toString() const;
        
    private:    
        uint8_t data[IFHWADDRLEN];
        void data_from_string(const string &address);
};

#endif /* __MACADDRESS_H__ */



