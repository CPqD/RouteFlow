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
/* Tests that the timer dispatcher can't starve the other pollable tasks in the
 * system, by ensuring that a timer posted during the dispatching of another
 * timer will not be dispatched until the timer dispatcher is polled again
 * later. */

#include "timer-dispatcher.hh"
#include <boost/bind.hpp>
#include "threads/cooperative.hh"
#include <cstdio>

using namespace vigil;

static void
timer_handler(Timer_dispatcher& timer_dispatcher, int i)
{
    printf("Timer %d fired\n", i);
    if (i < 5) {
        printf("Posting timer %d\n", i + 1);
        timer_dispatcher.post(boost::bind(timer_handler,
                                          boost::ref(timer_dispatcher), i + 1),
                              make_timeval(0, 0));
    } else {
        exit(0);
    }
}

class My_pollable
    : public Pollable
{
public:
    bool poll();
    void wait();
};

bool
My_pollable::poll()
{
    printf("Polled My_pollable\n");
    return false;
}

void
My_pollable::wait()
{
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
    timer_dispatcher.post(boost::bind(timer_handler,
                                      boost::ref(timer_dispatcher), 1),
                          make_timeval(0, 1));

    My_pollable my_pollable;
    loop.add_pollable(&my_pollable);

    loop.run();
}
