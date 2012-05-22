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
/* Tests the parsing and formatting features of ethernetaddr. */

#include "netinet++/ethernetaddr.hh"
#include <stdio.h>
#include <stdlib.h>

#define MUST_SUCCEED(EXPRESSION)                    \
    if (!(EXPRESSION)) {                            \
        fprintf(stderr, "%s:%d: %s failed\n",       \
                __FILE__, __LINE__, #EXPRESSION);   \
        exit(EXIT_FAILURE);                         \
    }

#define MUST_THROW(EXPRESSION)                                  \
    do {                                                        \
        bool caught = false;                                    \
        try {                                                   \
            (EXPRESSION);                                       \
        } catch (bad_ethernetaddr_cast) {                       \
            caught = true;                                      \
        }                                                       \
        if (!caught) {                                          \
            fprintf(stderr, "%s:%d: %s failed to throw\n",      \
                    __FILE__, __LINE__, #EXPRESSION);           \
            exit(EXIT_FAILURE);                                 \
        }                                                       \
    } while (0)

using namespace vigil;

int
main(void) 
{
    /* Successful parsing. */
    MUST_SUCCEED(ethernetaddr("1:2:3:4:5:6").hb_long() == 0x010203040506ULL);
    MUST_SUCCEED(ethernetaddr("01:02:03:04:05:06").hb_long()
                 == 0x010203040506ULL);
    MUST_SUCCEED(ethernetaddr("1a:2b:3c:4d:5e:6f").hb_long()
                 == 0x1a2b3c4d5e6fULL);
    MUST_SUCCEED(ethernetaddr("1A:2B:3C:4D:5E:6F").hb_long()
                 == 0x1a2b3c4d5e6fULL);

    /* Unsuccessful parsing. */
    MUST_THROW(ethernetaddr(""));
    MUST_THROW(ethernetaddr(":1:2:3:4:5"));
    MUST_THROW(ethernetaddr("x:1:2:3:4:5"));
    MUST_THROW(ethernetaddr("1:2:3:4:5:6:12x"));
    MUST_THROW(ethernetaddr("1:2:3:4:5x:6:12"));
    MUST_THROW(ethernetaddr("1:2:3ax:4:5:6:12"));
    MUST_THROW(ethernetaddr("1:2:3:4:5:678"));

    /* Formatting. */
    MUST_SUCCEED(ethernetaddr(0x010203040506ULL).string()
                 == "01:02:03:04:05:06");
    MUST_SUCCEED(ethernetaddr(0x1a2b3c4d5e6fULL).string()
                 == "1a:2b:3c:4d:5e:6f");

    return 0;
}
