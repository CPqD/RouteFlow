#include <net/if.h>
#include <boost/scoped_array.hpp>
#include "Match.hh"

Match::Match(const Match& other) : TLV(other) { }

Match::Match(MatchType type, boost::shared_array<uint8_t> value)
    : TLV(type, type_to_length(type), value) { }

Match::Match(MatchType type, const uint8_t* value)
    : TLV(type, type_to_length(type), value) { }

Match::Match(MatchType type, const uint8_t value)
    : TLV(type, type_to_length(type), value) { }

Match::Match(MatchType type, const uint16_t value)
    : TLV(type, type_to_length(type), value) { }

Match::Match(MatchType type, const uint32_t value)
    : TLV(type, type_to_length(type), value) { }

Match::Match(MatchType type, const MACAddress& addr) : TLV(type, addr) { }

Match::Match(MatchType type, const IPAddress& addr, const IPAddress& mask)
    : TLV(type, addr, mask) { }

Match& Match::operator=(const Match& other) {
    if (this != &other) {
        this->init(other.getType(), other.getLength(), other.getValue());
    }
    return *this;
}

bool Match::operator==(const Match& other) {
    return (this->getType() == other.getType() and
            (memcmp(other.getValue(), this->getValue(), this->length) == 0));
}

std::string Match::type_to_string() const {
    switch (this->type) {
        case RFMT_IPV4:         return "RFMT_IPV4";
        case RFMT_IPV6:         return "RFMT_IPV6";
        case RFMT_ETHERNET:     return "RFMT_ETHERNET";
        case RFMT_MPLS:         return "RFMT_MPLS";
        case RFMT_ETHERTYPE:    return "RFMT_ETHERTYPE";
        case RFMT_NW_PROTO:     return "RFMT_NW_PROTO";
        case RFMT_TP_SRC:       return "RFMT_TP_SRC";
        case RFMT_TP_DST:       return "RFMT_TP_DST";
        case RFMT_IN_PORT:      return "RFMT_IN_PORT";
        case RFMT_VLAN:         return "RFMT_VLAN";
        default:                return "UNKNOWN_MATCH";
    }
}

size_t Match::type_to_length(uint8_t type) {
    switch (type) {
        case RFMT_IPV4:
            return sizeof(struct ip_match);
        case RFMT_IPV6:
            return sizeof(struct ip6_match);
        case RFMT_ETHERNET:
            return IFHWADDRLEN;
        case RFMT_NW_PROTO:
            return sizeof(uint8_t);
        case RFMT_ETHERTYPE:
        case RFMT_TP_SRC:
        case RFMT_TP_DST:
        case RFMT_VLAN:
            return sizeof(uint16_t);
        case RFMT_MPLS:
        case RFMT_IN_PORT:
            return sizeof(uint32_t);
        default:                return 0;
    }
}

/**
 * Determine what byte-order the type is stored in internally
 */
byte_order Match::type_to_byte_order(uint8_t type) {
    switch (type) {
        case RFMT_IPV4:
        case RFMT_IPV6:
        case RFMT_ETHERNET:
            return ORDER_NETWORK;
        default:
            return ORDER_HOST;
    }
}

mongo::BSONObj Match::to_BSON() const {
    byte_order order = type_to_byte_order(type);
    return TLV::TLV_to_BSON(this, order);
}

/**
 * Constructs a new TLV object based on the given BSONObj. Converts values
 * formatted in network byte-order to host byte-order.
 *
 * It is the caller's responsibility to free the returned object. If the given
 * BSONObj is not a valid TLV, this method returns NULL.
 */
Match* Match::from_BSON(const mongo::BSONObj bson) {
    MatchType type = (MatchType)TLV::type_from_BSON(bson);
    if (type == 0)
        return NULL;

    byte_order order = type_to_byte_order(type);
    boost::shared_array<uint8_t> value = TLV::value_from_BSON(bson, order);

    if (value.get() == NULL)
        return NULL;

    return new Match(type, value);
}

namespace MatchList {
    mongo::BSONArray to_BSON(const std::vector<Match> list) {
        std::vector<Match>::const_iterator iter;
        mongo::BSONArrayBuilder builder;

        for (iter = list.begin(); iter != list.end(); ++iter) {
            builder.append(iter->to_BSON());
        }

        return builder.arr();
    }

    /**
     * Returns a vector of Matches extracted from 'bson'. 'bson' should be an
     * array of bson-encoded Match objects formatted as follows:
     * [{
     *   "type": (int),
     *   "value": (binary)
     * },
     * ...]
     *
     * If the given 'bson' is not an array, the returned vector will be empty.
     * If any matches in the array are invalid, they will not be added to the
     * vector.
     */
    std::vector<Match> to_vector(std::vector<mongo::BSONElement> array) {
        std::vector<mongo::BSONElement>::iterator iter;
        std::vector<Match> list;

        for (iter = array.begin(); iter != array.end(); ++iter) {
            Match* match = Match::from_BSON(iter->Obj());

            if (match != NULL) {
                list.push_back(*match);
            }
        }

        return list;
    }
}
