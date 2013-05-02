#ifndef HOSTENTRY_HH
#define HOSTENTRY_HH

#include "types/IPAddress.h"
#include "types/MACAddress.h"
#include "Interface.hh"

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

#endif /* HOSTENTRY_HH */
