#include <net/if.h>
#include <arpa/inet.h>
#include <boost/scoped_array.hpp>

#include "Option.hh"

Option::Option(const Option& other) : TLV(other) { }

Option::Option(OptionType type, boost::shared_array<uint8_t> value)
    : TLV(type, type_to_length(type), value) { }

Option::Option(OptionType type, const uint8_t* value)
    : TLV(type, type_to_length(type), value) { }

Option::Option(OptionType type, const uint16_t value)
    : TLV(type, type_to_length(type), value) { }

Option::Option(OptionType type, const uint32_t value)
    : TLV(type, type_to_length(type), value) { }

Option::Option(OptionType type, const uint64_t value)
    : TLV(type, type_to_length(type), value) { }

Option& Option::operator=(const Option& other) {
    if (this != &other) {
        this->init(other.getType(), other.getLength(), other.getValue());
    }
    return *this;
}

bool Option::operator==(const Option& other) {
    return (this->getType() == other.getType() and
            (memcmp(other.getValue(), this->getValue(), this->length) == 0));
}

std::string Option::type_to_string() const {
    switch (this->type) {
        case RFOT_PRIORITY:         return "RFOT_PRIORITY";
        case RFOT_IDLE_TIMEOUT:     return "RFOT_IDLE_TIMEOUT";
        case RFOT_HARD_TIMEOUT:     return "RFOT_HARD_TIMEOUT";
        case RFOT_CT_ID:            return "RFOT_CT_ID";
        default:                    return "UNKNOWN_OPTION";
    }
}

size_t Option::type_to_length(uint8_t type) {
    switch (type) {
        case RFOT_PRIORITY:
        case RFOT_IDLE_TIMEOUT:
        case RFOT_HARD_TIMEOUT:
            return sizeof(uint16_t);
        case RFOT_CT_ID:
            return sizeof(uint64_t);
        default:
            return 0;
    }
}

/**
 * Determine what byte-order the type is stored in internally
 */
byte_order Option::type_to_byte_order(uint8_t type) {
    switch (type) {
        default:
            return ORDER_HOST;
    }
}

mongo::BSONObj Option::to_BSON() const {
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
Option* Option::from_BSON(const mongo::BSONObj bson) {
    OptionType type = (OptionType)TLV::type_from_BSON(bson);
    if (type == 0)
        return NULL;

    byte_order order = type_to_byte_order(type);
    boost::shared_array<uint8_t> value = TLV::value_from_BSON(bson, order);

    if (value.get() == NULL)
        return NULL;

    return new Option(type, value);
}

namespace OptionList {
    mongo::BSONArray to_BSON(const std::vector<Option> list) {
        std::vector<Option>::const_iterator iter;
        mongo::BSONArrayBuilder builder;

        for (iter = list.begin(); iter != list.end(); ++iter) {
            builder.append(iter->to_BSON());
        }

        return builder.arr();
    }

    /**
     * Returns a vector of Options extracted from 'bson'. 'bson' should be an
     * array of bson-encoded Option objects formatted as follows:
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
    std::vector<Option> to_vector(std::vector<mongo::BSONElement> array) {
        std::vector<mongo::BSONElement>::iterator iter;
        std::vector<Option> list;

        for (iter = array.begin(); iter != array.end(); ++iter) {
            Option* option = Option::from_BSON(iter->Obj());

            if (option != NULL) {
                list.push_back(*option);
            }
        }

        return list;
    }
}
