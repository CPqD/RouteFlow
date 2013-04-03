#ifndef __TLV_HH__
#define __TLV_HH__

#include <stdint.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <vector>
#include <boost/shared_array.hpp>
#include <mongo/client/dbclient.h>

#include "types/MACAddress.h"
#include "types/IPAddress.h"

enum byte_order {
    ORDER_HOST = 0,
    ORDER_NETWORK = 1,
};

struct ip_match {
    struct in_addr addr;
    struct in_addr mask;
};

struct ip6_match {
    struct in6_addr addr;
    struct in6_addr mask;
};

class TLV {
    public:
        TLV(const TLV& other);
        TLV(uint8_t, size_t, boost::shared_array<uint8_t> value);
        TLV(uint8_t, size_t, const uint8_t* value);
        TLV(uint8_t, size_t, uint8_t value);
        TLV(uint8_t, size_t, uint16_t value);
        TLV(uint8_t, size_t, uint32_t value);
        TLV(uint8_t, size_t, uint64_t value);
        TLV(uint8_t, const MACAddress&);
        TLV(uint8_t, const IPAddress& addr, const IPAddress& mask);

        TLV& operator=(const TLV& other);
        bool operator==(const TLV& other);
        uint8_t getType() const;
        size_t getLength() const;
        uint8_t getUint8() const;
        uint16_t getUint16() const;
        uint32_t getUint32() const;
        uint64_t getUint64() const;
        const uint8_t* getValue() const;
        const void* getIPAddress() const;
        const void* getIPMask() const;

        virtual std::string toString() const;
        virtual std::string type_to_string() const;
        virtual bool optional() const;
        virtual mongo::BSONObj to_BSON();

    protected:
        uint8_t type;
        size_t length;
        boost::shared_array<uint8_t> value;

        void init(uint8_t type, size_t, const uint8_t* value);
        static mongo::BSONObj TLV_to_BSON(const TLV*, byte_order);
        static uint8_t type_from_BSON(mongo::BSONObj bson);
        static boost::shared_array<uint8_t> value_from_BSON(mongo::BSONObj,
                                                            byte_order);

    private:
        void init(uint8_t type, size_t, boost::shared_array<uint8_t> value);
};

#endif /* __TLV_HH__ */
