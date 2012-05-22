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
/* Tests that the timer dispatcher supports delaying and canceling timers.
 *
 * We keep three timers active at any given time, with successive ids starting
 * from n:
 *
 *      - The timer with id n fires.
 *
 *      - The timer with id n + 1 is delayed and its id is reassigned as n + 3.
 *
 *      - The timer with id n + 2 is canceled.
 *
 * Then the timer that was delayed is the next one to fire.
 */

#include "timer-dispatcher.hh"
#include <boost/bind.hpp>
#include "threads/cooperative.hh"
#include <cstdio>

using namespace vigil;

struct Timer_handler
{
    Timer_dispatcher& timer_dispatcher;
    Timer timer;
    int id;

    Timer_handler(Timer_dispatcher& timer_dispatcher_, int id_,
                  unsigned long int usec);
    void fire();
};

static Timer_handler* timer_to_delay;
static Timer_handler* timer_to_cancel;

Timer_handler::Timer_handler(Timer_dispatcher& timer_dispatcher_, int id_,
                             unsigned long int usec)
  : timer_dispatcher(timer_dispatcher_),
    id(id_)
{
    timer = timer_dispatcher.post(boost::bind(&Timer_handler::fire, this),
                                  make_timeval(0, usec));
}

void
Timer_handler::fire()
{
    printf("Timer %d fired\n", id);
    if (id < 15) {
        timer_to_delay->timer.delay(false, 0, 2);
        timer_to_delay->id += 2;

        timer_to_cancel->timer.cancel();
        delete timer_to_cancel;

        timer_to_delay = new Timer_handler(timer_dispatcher, id + 4, 10000);
        timer_to_cancel = new Timer_handler(timer_dispatcher, id + 5, 20000);
    } else if (id == 18) {
        exit(0);
    }
    delete this;
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
    new Timer_handler(timer_dispatcher, 1, 1000);
    timer_to_delay = new Timer_handler(timer_dispatcher, 2, 2000);
    timer_to_cancel = new Timer_handler(timer_dispatcher, 3, 3000);

    My_pollable my_pollable;
    loop.add_pollable(&my_pollable);

    loop.run();
}
