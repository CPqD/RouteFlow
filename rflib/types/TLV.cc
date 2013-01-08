#include <net/if.h>
#include <boost/scoped_array.hpp>

#include "TLV.hh"
#include "endian.hh"

// Most Significant Bit in the type field indicates the type is optional.
#define OPTIONAL_MASK (1 << 7)

/**
 * Makes a deep copy of the given TLV.
 */
TLV::TLV(const TLV& other) {
    this->init(other.getType(), other.getLength(), other.getValue());
}

/**
 * Constructs a new TLV object by storing the given shared_array.
 */
TLV::TLV(uint8_t type, size_t len, boost::shared_array<uint8_t> value) {
    this->init(type, len, value);
}

/**
 * Constructs a new TLV object by copying the value from the given pointer.
 */
TLV::TLV(uint8_t type, size_t len, const uint8_t* value) {
    this->init(type, len, value);
}

TLV::TLV(uint8_t type, size_t len, uint8_t value) {
    this->init(type, len, reinterpret_cast<uint8_t*>(&value));
}

TLV::TLV(uint8_t type, size_t len, uint16_t value) {
    this->init(type, len, reinterpret_cast<uint8_t*>(&value));
}

TLV::TLV(uint8_t type, size_t len, uint32_t value) {
    this->init(type, len, reinterpret_cast<uint8_t*>(&value));
}

TLV::TLV(uint8_t type, size_t len, uint64_t value) {
    this->init(type, len, reinterpret_cast<uint8_t*>(&value));
}

TLV::TLV(uint8_t type, const MACAddress& addr) {
    boost::shared_array<uint8_t> buf(new uint8_t[IFHWADDRLEN]);
    addr.toArray(buf.get());
    init(type, IFHWADDRLEN, buf);
}

TLV::TLV(uint8_t type, const IPAddress& addr, const IPAddress& mask) {
    boost::shared_array<uint8_t> buf;
    uint8_t *buf_mask = NULL;
    size_t length;

    if (addr.getVersion() == IPV6) {
        length = sizeof(ip6_match);
        buf.reset(new uint8_t[length]);
        buf_mask = buf.get() + (length / 2);
    } else if (addr.getVersion() == IPV4) {
        length = sizeof(ip_match);
        buf.reset(new uint8_t[length]);
        buf_mask = buf.get() + (length / 2);
    } else {
        throw "Invalid IP version";
    }

    addr.toArray(buf.get());
    mask.toArray(buf_mask);

    init(type, length, buf);
}

TLV& TLV::operator=(const TLV& other) {
    if (this != &other) {
        this->init(other.getType(), other.getLength(), other.getValue());
    }
    return *this;
}

bool TLV::operator==(const TLV& other) {
    return (this->getType() == other.getType() and
            (memcmp(other.getValue(), this->getValue(), this->length) == 0));
}

uint8_t TLV::getType() const {
    return this->type;
}

uint8_t TLV::getUint8() const {
    if (this->getLength() < sizeof(uint8_t)) {
        return 0;
    }
    return *reinterpret_cast<const uint8_t*>(this->getValue());
}

uint16_t TLV::getUint16() const {
    if (this->getLength() < sizeof(uint16_t)) {
        return 0;
    }
    return *reinterpret_cast<const uint16_t*>(this->getValue());
}

uint32_t TLV::getUint32() const {
    if (this->getLength() < sizeof(uint32_t)) {
        return 0;
    }
    return *reinterpret_cast<const uint32_t*>(this->getValue());
}

uint64_t TLV::getUint64() const {
    if (this->getLength() < sizeof(uint64_t)) {
        return 0;
    }
    return *reinterpret_cast<const uint64_t*>(this->getValue());
}

size_t TLV::getLength() const {
    return this->length;
}

const uint8_t* TLV::getValue() const {
    return this->value.get();
}

/**
 * Returns the IP address held internally, in network byte-order.
 */
const void* TLV::getIPAddress() const {
    return this->getValue();
}

/**
 * Returns the IP mask held internally, in network byte-order.
 */
const void* TLV::getIPMask() const {
    if (this->length == sizeof(ip_match)) {
        return &reinterpret_cast<const ip_match*>(this->getValue())->mask;
    } else if (this->length == sizeof(ip6_match)) {
        return &reinterpret_cast<const ip6_match*>(this->getValue())->mask;
    }
    return NULL;
}

mongo::BSONObj TLV::to_BSON() {
    return TLV_to_BSON(this, ORDER_HOST);
}

/**
 * Serialises the TLV object to BSON in the following format:
 * {
 *   "type": (int),
 *   "value": (binary)
 * }
 *
 * Where "value" is converted from "byte_order" to network byte-order.
 */
mongo::BSONObj TLV::TLV_to_BSON(const TLV* tlv, byte_order order) {
    const uint8_t* value = tlv->getValue();

    boost::scoped_array<uint8_t> arr(new uint8_t[tlv->length]);
    if (order == ORDER_HOST) {
        switch (tlv->length) {
            case sizeof(uint16_t): {
                uint16_t new_val = htons(tlv->getUint16());
                memcpy(arr.get(), &new_val, tlv->length);
                value = arr.get();
                break;
            }
            case sizeof(uint32_t): {
                uint32_t new_val = htonl(tlv->getUint32());
                memcpy(arr.get(), &new_val, tlv->length);
                value = arr.get();
                break;
            }
            case sizeof(uint64_t): {
                uint64_t new_val = htonll(tlv->getUint64());
                memcpy(arr.get(), &new_val, tlv->length);
                value = arr.get();
                break;
            }
            default:
                break;
        }
    }

    mongo::BSONObjBuilder builder;
    builder.append("type", tlv->type);
    builder.appendBinData("value", tlv->length, mongo::BinDataGeneral, value);

    return builder.obj();
}

uint8_t TLV::type_from_BSON(mongo::BSONObj bson) {
    const mongo::BSONElement& btype = bson["type"];

    if (btype.type() != mongo::NumberInt)
        return 0;

    return static_cast<uint8_t>(btype.Int());
}

boost::shared_array<uint8_t> TLV::value_from_BSON(mongo::BSONObj bson,
                                                  byte_order order) {
    boost::shared_array<uint8_t> arr(NULL);

    const mongo::BSONElement& bvalue = bson["value"];
    if (bvalue.type() != mongo::BinData)
        return arr;

    int len = bvalue.valuesize();
    const uint8_t* value = reinterpret_cast<const uint8_t*>
                           (bvalue.binData(len));

    arr.reset(new uint8_t[len]);
    if (order == ORDER_HOST) {
        switch (len) {
            case sizeof(uint16_t): {
                uint16_t new_val = ntohs(*reinterpret_cast<const uint16_t*>
                                        (value));
                memcpy(arr.get(), &new_val, len);
                break;
            }
            case sizeof(uint32_t): {
                uint32_t new_val = ntohl(*reinterpret_cast<const uint32_t*>
                                        (value));
                memcpy(arr.get(), &new_val, len);
                break;
            }
            case sizeof(uint64_t): {
                uint64_t new_val = ntohll(*reinterpret_cast<const uint64_t*>
                                         (value));
                memcpy(arr.get(), &new_val, len);
                break;
            }
            default:
                memcpy(arr.get(), value, len);
                break;
        }
    } else {
        memcpy(arr.get(), value, len);
    }

    return arr;
}

std::string TLV::toString() const {
    boost::scoped_array<char> buf(new char[this->length]);
    snprintf(buf.get(), this->length, "%*s", static_cast<int>(this->length),
             this->value.get());

    std::stringstream ss;
    ss << "{\"type\": " << this->type_to_string() << ", \"value\":\"";
    ss.write(buf.get(), this->length);
    ss << "\"}";

    return ss.str();
}

std::string TLV::type_to_string() const {
    std::stringstream ss;
    ss << this->type;
    return ss.str();
}

bool TLV::optional() const {
    if (this->type & OPTIONAL_MASK) {
        return true;
    }
    return false;
}

void TLV::init(uint8_t type, size_t len, const uint8_t* value) {
    boost::shared_array<uint8_t> arr(new uint8_t[len]);
    memcpy(arr.get(), value, len);
    this->init(type, len, arr);
}

void TLV::init(uint8_t type, size_t len, boost::shared_array<uint8_t> value) {
    this->type = type;
    this->length = 0;

    if (len == 0 || value.get() == NULL) {
        return;
    }

    this->value = value;
    this->length = len;
}
