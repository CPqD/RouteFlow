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
#ifndef EVENT_DISPATCHER_HH
#define EVENT_DISPATCHER_HH 1

#include <boost/function.hpp>
#include "event.hh"
#include "poll-loop.hh"

namespace vigil {

struct Event_dispatcher_impl;

 /** \brief An event dispatcher.
 *
 * This class serves three purposes that could really be broken up into
 * separate classes, but that seems pointless right now:
 *
 *   - Associates events with handlers and allows events to be dispatched to
 *     the appropriate handlers.
 *
 *   - Maintains a queue of pending events.
 *
 *   - Implements Pollable, which allows it to be added to a Poll_loop for
 *     periodic processing of the event queue.
 *
 * One convenient feature of this event dispatcher is that event handlers may
 * block without holding up processing of further events: they will be
 * dispatched by the Poll_loop in another thread.
 */
class Event_dispatcher
    : public Pollable
{
public:
    Event_dispatcher();
    ~Event_dispatcher();

    /* Registers 'handler' to be called to process each event of the given
     * 'type'.  Multiple handlers may be registered for any 'type', in which
     * case the handlers are called in increasing order of 'order'. */
    typedef Disposition Handler_signature(const Event&);
    typedef boost::function<Handler_signature> Handler;
    void add_handler(const Event_name&, const Handler&, int order);

    /* Appends 'event' to the list of events to be handled in the main loop. */
    void post(Event* event);

    /* Dispatches 'event' immediately, bypassing the event queue. */
    void dispatch(const Event& event);

    /* Pollable implementation.  Processes pending events when polled.  */
    bool poll();
    void wait();

private:
    Event_dispatcher_impl* p;
};

} // namespace vigil

#endif /* event-dispatcher.hh */
