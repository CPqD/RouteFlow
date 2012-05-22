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
#include "timeval.hh"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define MUST_SUCCEED(EXPRESSION)                    \
    if (!(EXPRESSION)) {                            \
        fprintf(stderr, "%s:%d: %s failed\n",       \
                __FILE__, __LINE__, #EXPRESSION);   \
        exit(EXIT_FAILURE);                         \
    }

int
main (void)
{
    /* First check that equality and relational operators work, so that we can
     * rely on them later. */
    timeval tv1, tv2;
    tv1.tv_sec = 1;
    tv1.tv_usec = 2;
    tv2.tv_sec = 3;
    tv2.tv_usec = 4;

    MUST_SUCCEED(tv1 == tv1);
    MUST_SUCCEED(!(tv1 != tv1));
    MUST_SUCCEED(!(tv1 < tv1));
    MUST_SUCCEED(tv1 <= tv1);
    MUST_SUCCEED(!(tv1 > tv1));
    MUST_SUCCEED(tv1 >= tv1);

    MUST_SUCCEED(tv2 == tv2);
    MUST_SUCCEED(!(tv2 != tv2));
    MUST_SUCCEED(!(tv2 < tv2));
    MUST_SUCCEED(tv2 <= tv2);
    MUST_SUCCEED(!(tv2 > tv2));
    MUST_SUCCEED(tv2 >= tv2);

    MUST_SUCCEED(!(tv1 == tv2));
    MUST_SUCCEED(tv1 != tv2);
    MUST_SUCCEED(tv1 < tv2);
    MUST_SUCCEED(!(tv1 > tv2));
    MUST_SUCCEED(tv1 <= tv2);
    MUST_SUCCEED(!(tv1 >= tv2));

    MUST_SUCCEED(!(tv2 == tv1));
    MUST_SUCCEED(tv2 != tv1);
    MUST_SUCCEED(!(tv2 < tv1));
    MUST_SUCCEED(tv2 > tv1);
    MUST_SUCCEED(!(tv2 <= tv1));
    MUST_SUCCEED(tv2 >= tv1);

    /* Now check that the constructors work. */
    MUST_SUCCEED(make_timeval(1, 2) == tv1);
    MUST_SUCCEED(make_timeval(3, 4) == tv2);
    MUST_SUCCEED(make_timeval(1, 2) != tv2);
    MUST_SUCCEED(make_timeval(3, 4) != tv1);

    /* Now check that arithmetic works. */
    timeval tv3 = tv1;
    tv3 += tv2;
    MUST_SUCCEED(tv3 == make_timeval(4, 6));

    tv3 = tv2;
    tv3 -= tv1;
    MUST_SUCCEED(tv3 == make_timeval(2, 2));

    MUST_SUCCEED(tv1 + tv2 == make_timeval(4, 6));
    MUST_SUCCEED(tv2 - tv1 == make_timeval(2, 2));
    MUST_SUCCEED(make_timeval(0, 1) + make_timeval(0, 999999)
                 == make_timeval(1, 0));
    MUST_SUCCEED(make_timeval(1, 0) - make_timeval(0, 999999)
                 == make_timeval(0, 1));
    MUST_SUCCEED(make_timeval(1, 0) - make_timeval(1, 0)
                 == make_timeval(0, 0));

    /* Check conversions. */
    MUST_SUCCEED(timeval_to_ms(make_timeval(1, 2)) == 1000);
    MUST_SUCCEED(timeval_to_ms(make_timeval(3, 4)) == 3000);
    MUST_SUCCEED(timeval_to_ms(make_timeval(LONG_MAX, 999999)) == LONG_MAX);
    MUST_SUCCEED(timeval_to_ms(make_timeval(LONG_MIN, 999999)) == LONG_MIN);
    MUST_SUCCEED(timeval_to_ms(make_timeval(123, 499499)) == 123499);
    MUST_SUCCEED(timeval_to_ms(make_timeval(123, 499501)) == 123500);
    MUST_SUCCEED(timeval_to_ms(make_timeval(123, 500000)) == 123500);
    MUST_SUCCEED(timeval_to_ms(make_timeval(-123, 499499)) == -123499);
    MUST_SUCCEED(timeval_to_ms(make_timeval(-123, 499501)) == -123500);
    MUST_SUCCEED(timeval_to_ms(make_timeval(-123, 500000)) == -123500);

    MUST_SUCCEED(timeval_from_ms(1000) == make_timeval(1, 0));
    MUST_SUCCEED(timeval_from_ms(3000) == make_timeval(3, 0));
    MUST_SUCCEED(timeval_from_ms(LONG_MAX)
                 == make_timeval(LONG_MAX / 1000, LONG_MAX % 1000 * 1000));
    MUST_SUCCEED(timeval_from_ms(LONG_MIN)
                 == make_timeval(LONG_MIN / 1000, (-(LONG_MIN % 1000)) * 1000));
    MUST_SUCCEED(timeval_from_ms(2147483647L) == make_timeval(2147483, 647000));
    MUST_SUCCEED(timeval_from_ms(-2147483647 - 1)
                 == make_timeval(-2147483, 648000));
    MUST_SUCCEED(timeval_from_ms(123499) == make_timeval(123, 499000));
    MUST_SUCCEED(timeval_from_ms(123500) == make_timeval(123, 500000));
    MUST_SUCCEED(timeval_from_ms(-123499) == make_timeval(-123, 499000));
    MUST_SUCCEED(timeval_from_ms(-123500) == make_timeval(-123, 500000));
    

    return 0;
}
