#ifndef __OPTION_HH__
#define __OPTION_HH__

#include "TLV.hh"

#define DEFAULT_PRIORITY 0x8000

enum OptionType {
    RFOT_PRIORITY = 1,      /* Route priority */
    RFOT_IDLE_TIMEOUT = 2,  /* Drop route after specified idle time */
    RFOT_HARD_TIMEOUT = 3,  /* Drop route after specified time has passed */
    /* MSB = 1; Indicates optional feature. */
    RFOT_CT_ID = 255,       /* Specify destination controller */
};

class Option : public TLV {
    public:
        Option(const Option& other);
        Option(OptionType, boost::shared_array<uint8_t> value);
        Option(OptionType, const uint8_t* value);
        Option(OptionType, const uint16_t value);
        Option(OptionType, const uint32_t value);
        Option(OptionType, const uint64_t value);

        Option& operator=(const Option& other);
        bool operator==(const Option& other);
        virtual std::string type_to_string() const;
        virtual mongo::BSONObj to_BSON() const;

        static Option* from_BSON(mongo::BSONObj bson);
    private:
        static size_t type_to_length(uint8_t type);
        static byte_order type_to_byte_order(uint8_t);
};

namespace OptionList {
    mongo::BSONArray to_BSON(const std::vector<Option> list);
    std::vector<Option> to_vector(std::vector<mongo::BSONElement> array);
}

#endif /* __OPTION_HH__ */
