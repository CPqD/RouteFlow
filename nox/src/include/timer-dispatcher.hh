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
#ifndef TIMER_DISPATCHER_HH
#define TIMER_DISPATCHER_HH 1

#include <boost/function.hpp>
#include "poll-loop.hh"
#include "timeval.hh"

namespace vigil {

class Timer;
class Timer_impl;
class Timer_dispatcher;
struct Timer_dispatcher_impl;

typedef boost::function<void()> Callback;

/* Timer dispatcher.
 *
 * Allows timers to be associated with callbacks.  One convenient feature of
 * this timer dispatcher is that timer callbacks may block without holding up
 * processing of further timers: they will be dispatched by the Poll_loop in
 * another thread.
 */
class Timer_dispatcher
    : public Pollable
{
public:
    Timer_dispatcher();
    ~Timer_dispatcher();

    /* Posts 'callback' to be called after the given 'duration' elapses.  The
     * caller must not destroy the returned Timer, but may use it to cancel or
     * reschedule the timer, up until the point where the timer is actually
     * invoked. */
    Timer post(const Callback& callback, const timeval& duration);

    /* Posts 'callback' to be called immediately.  The caller must not destroy
     * the returned Timer, but may use it to cancel or reschedule the timer, up
     * until the point where the timer is actually invoked. */
    Timer post(const Callback& callback);

    /* Logs the size of the internal data structures. */
    void debug() const;

private:
    friend class Timer;
    friend class Timer_impl;

    /* Returns true if the timer is known to the dispatcher */
    bool check_validity(Timer_impl*, const unsigned int generation) const;

    Timer_dispatcher_impl* p;
    unsigned int next_generation;

    /* Pollable implementation. */
    bool poll();
    void wait();
};

/* A timer facade returned by Timer_dispatcher::post(). Facade
   contains a pointer to the actual Timer implementation and takes
   care of its proper management.  Eventually the facade should become
   a part of the component interface itself. */
class Timer
{
public:
    Timer();
    Timer(Timer_dispatcher*, Timer_impl*, const unsigned int);
    void cancel();
    void delay(bool neg, long sec, long usec);
    void reset(long sec, long usec);
    timeval get_time() const;

private:
    Timer_dispatcher* dispatcher;
    Timer_impl* impl;
    unsigned int generation;
};

} // namespace vigil

#endif /* timer-dispatcher.hh */
