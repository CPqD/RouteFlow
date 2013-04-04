#ifndef INTERFACE_HH
#define INTERFACE_HH

#include "types/IPAddress.h"
#include "types/MACAddress.h"

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

#endif /* INTERFACE_HH */
