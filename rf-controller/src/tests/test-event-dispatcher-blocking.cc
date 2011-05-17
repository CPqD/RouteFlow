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
/* Tests that events continue to be dispatched in additional threads when an
 * existing thread blocks, and that the event dispatcher does not continue
 * dispatching events after the blocking thread resumes. */

#include "event-dispatcher.hh"
#include <boost/bind.hpp>
#include "assert.hh"
#include "threads/cooperative.hh"
#include <cstdio>

using namespace vigil;

static Co_sema wait[3];

class My_event
    : public Event
{
public:
    My_event(int event_data_) : Event("My_event"), event_data(event_data_) { }
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
    if (data < 3) {
        printf("  Blocking...\n");
        wait[data].down();
        printf("Event %d blocker woke up...\n", data);
    } else if (data < 6) {
        printf("  Waking event %d blocker\n", data - 3);
        wait[data - 3].up();
        printf("  Posting event %d\n", data + 3);
        event_dispatcher.post(new My_event(data + 3));
    } else if (data == 8) {
        exit(0);
    }
    return CONTINUE;
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

    Poll_loop loop(4);

    Event_dispatcher event_dispatcher;
    loop.add_pollable(&event_dispatcher);
    event_dispatcher.add_handler(My_event::static_get_name(),
                                 boost::bind(handle_my_event, _1,
                                             boost::ref(event_dispatcher)), 0);
    for (int i = 0; i < 6; ++i) {
        event_dispatcher.post(new My_event(i));
    }

    My_pollable my_pollable;
    loop.add_pollable(&my_pollable);

    loop.run();
}
