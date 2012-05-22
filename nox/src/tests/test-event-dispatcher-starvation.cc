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
/* Tests that the event dispatcher can't starve the other pollable tasks in the
 * system, by ensuring that an event posted during the dispatching of another
 * event will not be dispatched until the event dispatcher is polled again
 * later. */

#include "event.hh"
#include "event-dispatcher.hh"
#include <boost/bind.hpp>
#include "assert.hh"
#include "threads/cooperative.hh"
#include <cstdio>

using namespace vigil;

class My_event
    : public Event 
{
public:
    My_event(int event_data_) : Event(static_get_name()), event_data(event_data_) { }
    int get_event_data() const { return event_data; }

    static const Event_name static_get_name() {
        return "My_event";
    }
private:
    int event_data;
};

static Disposition
handle_my_event(const Event& e_, Event_dispatcher& event_dispatcher)
{
    const My_event& e = assert_cast<const My_event&>(e_);

    int data = e.get_event_data();
    printf("Handling event %d\n", data);
    if (data < 5) {
        printf("  Handler posting event %d\n", data + 5);
        event_dispatcher.post(new My_event(data + 5));
    } else if (data == 14) {
        exit(0);
    }
    return CONTINUE;
}

class My_pollable
    : public Pollable
{
public:
    My_pollable(Event_dispatcher& event_dispatcher_)
        : event_dispatcher(event_dispatcher_) { }
    bool poll();
    void wait();
private:
    Event_dispatcher& event_dispatcher;
};

bool
My_pollable::poll() 
{
    printf("Polled My_pollable\n");
    static int n = 0;
    if (++n < 5) {
        printf("  My_pollable posting event %d\n", n + 10);
        event_dispatcher.post(new My_event(n + 10));
    }
    return true;
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

    Event_dispatcher event_dispatcher;
    loop.add_pollable(&event_dispatcher);
    event_dispatcher.add_handler(My_event::static_get_name(),
                                 boost::bind(handle_my_event, _1,
                                             boost::ref(event_dispatcher)), 0);
    event_dispatcher.post(new My_event(1));
    event_dispatcher.post(new My_event(2));

    My_pollable my_pollable(event_dispatcher);
    loop.add_pollable(&my_pollable);

    loop.run();
}
