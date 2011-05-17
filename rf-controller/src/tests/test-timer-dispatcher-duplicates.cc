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
/* Tests that the timer dispatcher properly handles multiple timers set to
 * expire at the same time.
 *
 * Also tests that multiple timers for a single time are executed in FIFO
 * order.
 *
 * This tests for regression against commit
 * a61f85ab17d3c9fb03948d622d620d6145d2d339 (When dispatching timers, delete
 * only the executed timer from the timer queue.)
 */

#include "timer-dispatcher.hh"
#include <boost/bind.hpp>
#include <unistd.h>
#include "threads/cooperative.hh"
#include <cstdio>

#undef NDEBUG
#include <assert.h>

using namespace vigil;

static const int N_TIMERS = 5;

void
fire(int id)
{
    printf("Timer %d fired\n", id);
    if (id == N_TIMERS - 1) {
        exit(0);
    }
}

int
main(int argc, char *argv[])
{
    co_init();
    co_thread_assimilate();
    co_migrate(&co_group_coop);

    Poll_loop loop(1);

    Timer_dispatcher timer_dispatcher;
    loop.add_pollable(&timer_dispatcher);

    Timer timers[N_TIMERS];
    for (int i = 0; i < N_TIMERS; i++) {
        timers[i] = timer_dispatcher.post(boost::bind(fire, i),
                                          make_timeval(0, 1000));

        /* Verify that the timers are actually scheduled for the same time.  */
        assert(i == 0 || timers[i - 1].get_time() == timers[i].get_time());
    }

    /* If this regresses, we'll hang. */
    alarm(3);

    loop.run();
}
