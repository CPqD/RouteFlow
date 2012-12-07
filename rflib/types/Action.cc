#include <net/if.h>
#include <boost/scoped_array.hpp>

#include "Action.hh"

Action::Action(const Action& other) : TLV(other) { }

Action::Action(ActionType type, boost::shared_array<uint8_t> value)
    : TLV(type, type_to_length(type), value) { }

Action::Action(ActionType type, const uint8_t* value)
    : TLV(type, type_to_length(type), value) { }

Action::Action(ActionType type, const uint32_t value)
    : TLV(type, type_to_length(type), value) { }

Action::Action(ActionType type, const MACAddress& addr) : TLV(type, addr) { }

Action::Action(ActionType type, const IPAddress& addr, const IPAddress& mask)
    : TLV(type, addr, mask) { }

Action& Action::operator=(const Action& other) {
    if (this != &other) {
        this->init(other.getType(), other.getLength(), other.getValue());
    }
    return *this;
}

bool Action::operator==(const Action& other) {
    return (this->getType() == other.getType() and
            (memcmp(other.getValue(), this->getValue(), this->length) == 0));
}

std::string Action::type_to_string() const {
    switch (this->type) {
        case RFAT_OUTPUT:           return "RFAT_OUTPUT";
        case RFAT_PUSH_MPLS:        return "RFAT_PUSH_MPLS";
        case RFAT_SWAP_MPLS:        return "RFAT_SWAP_MPLS";
        case RFAT_SET_ETH_SRC:      return "RFAT_SET_ETH_SRC";
        case RFAT_SET_ETH_DST:      return "RFAT_SET_ETH_DST";
        case RFAT_POP_MPLS:         return "RFAT_POP_MPLS";
        case RFAT_DROP:             return "RFAT_DROP";
        case RFAT_SFLOW:            return "RFAT_SFLOW";
        default:                    return "UNKNOWN_ACTION";
    }
}

size_t Action::type_to_length(uint8_t type) {
    switch (type) {
        case RFAT_OUTPUT:
        case RFAT_PUSH_MPLS:
        case RFAT_SWAP_MPLS:
            return sizeof(uint32_t);
        case RFAT_SET_ETH_SRC:
        case RFAT_SET_ETH_DST:
            return IFHWADDRLEN;
        case RFAT_POP_MPLS: /* len = 0 */
        case RFAT_DROP:
        case RFAT_SFLOW:
        default:
            return 0;
    }
}

/**
 * Determine what byte-order the type is stored in internally
 */
byte_order Action::type_to_byte_order(uint8_t type) {
    switch (type) {
        case RFAT_SET_ETH_SRC:
        case RFAT_SET_ETH_DST:
            return ORDER_NETWORK;
        default:
            return ORDER_HOST;
    }
}

mongo::BSONObj Action::to_BSON() const {
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
Action* Action::from_BSON(const mongo::BSONObj bson) {
    ActionType type = (ActionType)TLV::type_from_BSON(bson);
    if (type == 0)
        return NULL;

    byte_order order = type_to_byte_order(type);
    boost::shared_array<uint8_t> value = TLV::value_from_BSON(bson, order);

    if (value.get() == NULL)
        return NULL;

    return new Action(type, value);
}

namespace ActionList {
    mongo::BSONArray to_BSON(const std::vector<Action> list) {
        std::vector<Action>::const_iterator iter;
        mongo::BSONArrayBuilder builder;

        for (iter = list.begin(); iter != list.end(); ++iter) {
            builder.append(iter->to_BSON());
        }

        return builder.arr();
    }

    /**
     * Returns a vector of Actions extracted from 'bson'. 'bson' should be an
     * array of bson-encoded Action objects formatted as follows:
     * [{
     *   "type": (int),
     *   "value": (binary)
     * },
     * ...]
     *
     * If the given 'bson' is not an array, the returned vector will be empty.
     * If any actions in the array are invalid, they will not be added to the
     * vector.
     */
    std::vector<Action> to_vector(std::vector<mongo::BSONElement> array) {
        std::vector<mongo::BSONElement>::iterator iter;
        std::vector<Action> list;

        for (iter = array.begin(); iter != array.end(); ++iter) {
            Action* action = Action::from_BSON(iter->Obj());

            if (action != NULL) {
                list.push_back(*action);
            }
        }

        return list;
    }
}
