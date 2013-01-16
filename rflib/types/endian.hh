#ifndef ENDIAN_H
#define ENDIAN_H 1

#include <stdint.h>
#include <endian.h>
#include <arpa/inet.h>

#define ntohll htonll
inline uint64_t htonll(uint64_t value)
{
#if  __BYTE_ORDER == __LITTLE_ENDIAN
    const uint32_t high = htonl(static_cast<uint32_t>(value >> 32));
    const uint32_t low = htonl(static_cast<uint32_t>(value & 0xFFFFFFFFLL));

    return (static_cast<uint64_t>(low) << 32) | high;
#elif __BYTE_ORDER == __BIG_ENDIAN
    return value;
#else
    #error "Unrecognised endianness"
#endif
}

#endif /* ENDIAN_H */
