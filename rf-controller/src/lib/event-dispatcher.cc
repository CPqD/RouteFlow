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
#include "event-dispatcher.hh"

#include <map>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/ptr_container/ptr_list.hpp>

#include "hash_map.hh"
#include "threads/cooperative.hh"
#include "vlog.hh"

namespace vigil {

static Vlog_module lg("event-dispatcher");

typedef std::multimap<int, Event_dispatcher::Handler> Signal;

struct Event_dispatcher_impl
{
    hash_map<Event_name, Signal> table;
    boost::ptr_list<Event> queue;
    Co_cond nonempty_queue;
    unsigned int serial;
};

Event_dispatcher::Event_dispatcher()
    : p(new Event_dispatcher_impl())
{
    p->serial = 0;
}

Event_dispatcher::~Event_dispatcher()
{
    delete p;
}

void
Event_dispatcher::add_handler(const Event_name& name, 
                              const Handler& handler,
                              int order)
{
    p->table[name].insert(Signal::value_type(order, handler));
}

void
Event_dispatcher::post(Event* event)
{
    if (p->queue.empty()) {
        p->nonempty_queue.broadcast();
    }
    p->queue.push_back(event);
}

void
Event_dispatcher::dispatch(const Event& e)
{
    const Event_name& name = e.get_name();
    if (p->table.find(name) != p->table.end()) {
        BOOST_FOREACH (Signal::value_type& i, p->table[name]) {
            try {
                if (i.second(e) == STOP) {
                    break;
                }
            } catch (const std::exception& e) {
                lg.err("Event %s processing leaked an exception: %s", 
		       name.c_str(), e.what());
                break;
            }
        }
    }
}

bool
Event_dispatcher::poll()
{
    /* Dispatch all the events initially in the queue, but not any events
     * queued by processing those events, to avoid starving other Pollables.
     *
     * XXX We need a scheme for prioritizing event dispatch above other
     * Pollables.
     */
    size_t max = p->queue.size();
    unsigned int serial = ++p->serial;
    for (size_t i = 0; i < max; ++i) {
        std::auto_ptr<Event> event(p->queue.pop_front().release());
        dispatch(*event);

        if (serial != p->serial) {
            /* dispatch(*event) blocked and Event_dispatcher::poll() was
             * eventually re-entered in another thread.  That other call
             * already dispatched our events, so we are done. */
            break;
        }
    }
    return max > 0;
}

void
Event_dispatcher::wait()
{
    if (!p->queue.empty()) {
        co_immediate_wake(1, NULL);
    } else {
        p->nonempty_queue.wait();
    }
}

} // namespace vigil
