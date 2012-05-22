/* Copyright 2008 (C) Nicira, Inc.
 *
 * This file is part of NOX.
 *
 * NOX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NOX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NOX.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef SHA1_HH
#define SHA1_HH

#include <stdint.h>
#include <string>

namespace vigil {

/* Secure Hashing Standard as defined FIPS PUB 180-1 published April
 * 17, 1995.
 *
 * Copyright (C) 1998
 * Paul E. Jones <paulej@arid.us>
 * All Rights Reserved.
 *
 * This software is licensed as "freeware."  Permission to distribute
 * this software in source and binary forms is hereby granted without
 * a fee.  THIS SOFTWARE IS PROVIDED 'AS IS' AND WITHOUT ANY EXPRESSED
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * THE AUTHOR SHALL NOT BE HELD LIABLE FOR ANY DAMAGES RESULTING FROM
 * THE USE OF THIS SOFTWARE, EITHER DIRECTLY OR INDIRECTLY, INCLUDING,
 * BUT NOT LIMITED TO, LOSS OF DATA OR DATA BEING RENDERED INACCURATE.
 *
 * The class is a stripped version of the original version by Paul
 * E. Jones and exists here merely to prevent introduction of
 * unnecessary external dependencies (like OpenSSL) in the storage
 * implementation.
 */
class SHA1
{
public:
    SHA1();
    virtual ~SHA1();

    /* Size of hash in bytes */
    static const int hash_size = 20;

    /* Re-initialize the class. */
    void reset();
    
    /* Returns the message digest. */
    bool digest(uint8_t *message_digest_array);
    
    /* Provide input. */
    void input(const void* message, int len);

    /* Provide input. */
    inline void input(const std::string& message);
    
private:
    /* Process the next 512 bits of the message */
    void process_message_block();
    
    /* Pads the current message block to 512 bits */
    void pad_message();
    
    /* Performs a circular left shift operation */
    inline uint32_t circular_shift(int bits, unsigned word);
    
    /* Message digest buffers */
    uint32_t H[5];
    
    /* Message length in bits */
    uint32_t length_low;				

    /* Message length in bits */
    uint32_t length_high;				
    
    /* 512-bit message blocks */
    uint8_t message_block[64];	

    /* Index into message block array */
    int message_block_index;

    /* Is the digest computed? */
    bool computed;

    /* Is the message digest corrupted? */
    bool corrupted;
};

inline void
SHA1::input(const std::string &message) {
    input(message.c_str(), message.length());
}

} // namespace vigil

#endif
