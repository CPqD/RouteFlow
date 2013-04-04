#ifndef ROUTEENTRY_HH
#define ROUTEENTRY_HH

#include "types/IPAddress.h"
#include "Interface.hh"

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

#endif /* ROUTEENTRY_HH */
