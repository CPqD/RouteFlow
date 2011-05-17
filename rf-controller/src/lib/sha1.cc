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
#include <arpa/inet.h>
#include "sha1.hh"

using namespace vigil;

SHA1::SHA1() {
	reset();
}

SHA1::~SHA1() { }

void 
SHA1::reset() {
    length_low = 0;
    length_high = 0;
    message_block_index = 0;
    
    H[0] = 0x67452301;
    H[1] = 0xEFCDAB89;
    H[2] = 0x98BADCFE;
    H[3] = 0x10325476;
    H[4] = 0xC3D2E1F0;
    
    computed = false;
    corrupted = false;
}

bool SHA1::digest(uint8_t *message_digest_array)
{
    if (corrupted) return false;
    if (!computed) {
        pad_message();
        computed = true;
    }

    uint32_t* array = (uint32_t*)message_digest_array;
    for(int i = 0; i < 5; i++) {
        array[i] = htonl(H[i]);
    }

    return true;
}

void 
SHA1::input(const void *message_array, int length)
{
    if (!length) return;

    if (computed || corrupted) {
        corrupted = true;
        return;
    }

    const uint8_t* array = (const uint8_t*)message_array;

    while(length-- && !corrupted) {
        message_block[message_block_index++] = *array & 0xFF;
        
        length_low += 8;
        length_low &= 0xFFFFFFFF;

        if (length_low == 0) {
            length_high++;
            length_high &= 0xFFFFFFFF;
            if (length_high == 0) {
                /* Message is too long */
                corrupted = true;
            }
        }
        
        if (message_block_index == 64) {
            process_message_block();
        }

        array++;
    }
}

void 
SHA1::process_message_block() {
    const unsigned K[] = {
        /* Constants defined for SHA-1 */
        0x5A827999,
        0x6ED9EBA1,
        0x8F1BBCDC,
        0xCA62C1D6
    };
    int t;			
    uint32_t temp;		
    uint32_t W[80];		
    uint32_t A, B, C, D, E;

    /* Initialize the first 16 words in the array W */
    for(t = 0; t < 16; t++) {
        W[t] = ((uint32_t)message_block[t * 4]) << 24;
        W[t] |= ((uint32_t)message_block[t * 4 + 1]) << 16;
        W[t] |= ((uint32_t)message_block[t * 4 + 2]) << 8;
        W[t] |= ((uint32_t)message_block[t * 4 + 3]);
    }

    for(t = 16; t < 80; t++) {
        W[t] = circular_shift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
    }

    A = H[0];
    B = H[1];
    C = H[2];
    D = H[3];
    E = H[4];
    
    for(t = 0; t < 20; t++) {
        temp = circular_shift(5,A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = circular_shift(30,B);
        B = A;
        A = temp;
    }
    
    for(t = 20; t < 40; t++) {
        temp = circular_shift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = circular_shift(30,B);
        B = A;
        A = temp;
    }

    for(t = 40; t < 60; t++) {
        temp = circular_shift(5,A) +
            ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = circular_shift(30,B);
        B = A;
        A = temp;
    }
    
    for(t = 60; t < 80; t++) {
        temp = circular_shift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = circular_shift(30,B);
        B = A;
        A = temp;
    }
    
    H[0] = (H[0] + A) & 0xFFFFFFFF;
    H[1] = (H[1] + B) & 0xFFFFFFFF;
    H[2] = (H[2] + C) & 0xFFFFFFFF;
    H[3] = (H[3] + D) & 0xFFFFFFFF;
    H[4] = (H[4] + E) & 0xFFFFFFFF;
    
    message_block_index = 0;
}

void 
SHA1::pad_message() {
    /* Check to see if the current message block is too small to hold
     * the initial padding bits and length.  If so, we will pad the
     * block, process it, and then continue padding into a second
     * block. */
    if (message_block_index > 55) {
        message_block[message_block_index++] = 0x80;
        while(message_block_index < 64) {
            message_block[message_block_index++] = 0;
        }
        
        process_message_block();
        
        while(message_block_index < 56) {
            message_block[message_block_index++] = 0;
        }
    } else {
        message_block[message_block_index++] = 0x80;
        while(message_block_index < 56) {
            message_block[message_block_index++] = 0;
        }
    }

    /* Store the message length as the last 8 octets */
    message_block[56] = (length_high >> 24) & 0xFF;
    message_block[57] = (length_high >> 16) & 0xFF;
    message_block[58] = (length_high >> 8) & 0xFF;
    message_block[59] = (length_high) & 0xFF;
    message_block[60] = (length_low >> 24) & 0xFF;
    message_block[61] = (length_low >> 16) & 0xFF;
    message_block[62] = (length_low >> 8) & 0xFF;
    message_block[63] = (length_low) & 0xFF;
    
    process_message_block();
}

uint32_t 
SHA1::circular_shift(int bits, uint32_t word)
{
    return ((word << bits) & 0xFFFFFFFF) | ((word & 0xFFFFFFFF) >> (32-bits));
}
