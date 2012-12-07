#ifndef __ACTION_HH__
#define __ACTION_HH__

#include "TLV.hh"

enum ActionType {
    RFAT_OUTPUT = 1,        /* Output port */
    RFAT_SET_ETH_SRC = 2,   /* Ethernet source address */
    RFAT_SET_ETH_DST = 3,   /* Ethernet destination address */
    RFAT_PUSH_MPLS = 4,     /* Push MPLS label */
    RFAT_POP_MPLS = 5,      /* Pop MPLS label */
    RFAT_SWAP_MPLS = 6,     /* Swap MPLS label */
    /* MSB = 1; Indicates optional feature. */
    RFAT_DROP = 254,        /* Drop packet (Unimplemented) */
    RFAT_SFLOW = 255,       /* Generate SFlow messages (Unimplemented) */
};

class Action : public TLV {
    public:
        Action(const Action& other);
        Action(ActionType, boost::shared_array<uint8_t> value);
        Action(ActionType, const uint8_t* value);
        Action(ActionType, const uint32_t value);
        Action(ActionType, const MACAddress&);
        Action(ActionType, const IPAddress& addr, const IPAddress& mask);

        Action& operator=(const Action& other);
        bool operator==(const Action& other);
        virtual std::string type_to_string() const;
        virtual mongo::BSONObj to_BSON() const;

        static Action* from_BSON(mongo::BSONObj);
    private:
        static size_t type_to_length(uint8_t);
        static byte_order type_to_byte_order(uint8_t);
};

namespace ActionList {
    mongo::BSONArray to_BSON(const std::vector<Action> list);
    std::vector<Action> to_vector(std::vector<mongo::BSONElement> array);
}

#endif /* __ACTION_HH__ */
