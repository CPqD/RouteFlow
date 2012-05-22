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
#include "timer-dispatcher.hh"

#include <boost/foreach.hpp>
#include <set>

#include "hash_set.hh"
#include "threads/cooperative.hh"
#include "vlog.hh"

namespace vigil {

static Vlog_module lg("timer-dispatcher");

class Timer_impl {
public:
    Timer_impl(Timer_dispatcher* dispatcher_, timeval time_, 
               const unsigned int generation_, const Callback& f)
        : dispatcher(dispatcher_), func(f), time(time_), admitted(false),
          generation(generation_) {
    }

    void cancel();
    void delay(bool neg, long sec, long usec);
    void reset(long sec, long usec);

    timeval get_time() const { return time; }

private:
    friend class Timer_dispatcher;
    friend struct Compare_timer;

    Timer_dispatcher* dispatcher;
    Callback func;
    timeval time;
    bool admitted;
    unsigned int generation;

    Timer_impl() { }
    void set_timeout(const timeval&);
};

/* Timers. */
struct Compare_timer {
    bool operator()(const Timer_impl* lhs, const Timer_impl* rhs) {
        timeval lhs_time = lhs->get_time();
        timeval rhs_time = rhs->get_time();
        if (lhs_time != rhs_time) {
            return lhs_time < rhs_time;
        } else {
            return lhs->generation < rhs->generation;
        }
    }
};

typedef std::set<Timer_impl*, Compare_timer> Timer_queue;
typedef hash_set<Timer_impl*> Timer_set;

struct Timer_dispatcher_impl
{
    Timer_queue timers;         /* Active timers. */
    Timer_set applicants;       /* Timers waiting for admission to 'timers'. */
    Timer_set all_timers;       /* Every timer instance in either of above. */
    Co_cond new_timers;         /* Signaled to wake up dispatcher. */
    unsigned int serial;        /* Detects timer dispatch that blocked. */
};

Timer_dispatcher::Timer_dispatcher()
    : p(new Timer_dispatcher_impl), next_generation(0)
{
    do_gettimeofday(true);
    p->serial = 0;
}

Timer_dispatcher::~Timer_dispatcher()
{
    delete p;
}

Timer
Timer_dispatcher::post(const Callback& callback, const timeval& duration)
{
    Timer_impl* t = 
        new Timer_impl(this, do_gettimeofday() + duration, ++next_generation,
                       callback);
    p->applicants.insert(t);
    p->all_timers.insert(t);
    p->new_timers.signal();
    return Timer(this, t, t->generation);
}

Timer
Timer_dispatcher::post(const Callback& callback)
{
    timeval duration = {0, 0};
    return post(callback, duration);
}

void
Timer_dispatcher::debug() const {
    lg.dbg("statistics: all timers = %d, applicants = %d, timers = %d", 
           p->all_timers.size(), p->applicants.size(), p->timers.size());
}

bool
Timer_dispatcher::poll()
{
    /* Update the time of day for this iteration. */
    timeval now = do_gettimeofday(true);

    /* Admit new timers. */
    BOOST_FOREACH (Timer_impl* t, p->applicants) {
        t->admitted = true;
    }
    p->timers.insert(p->applicants.begin(), p->applicants.end());
    p->applicants.clear();

    /* Execute all expired timers initially in the queue, but no new or
     * modified timers, to avoid starving other Pollables.
     *
     * XXX We need a scheme for prioritizing timer dispatch above other
     * Pollables.
     */
    bool progress = false;
    unsigned int serial = ++p->serial;
    while (!p->timers.empty()) {
        /* Check whether we should fire this timer. */
        Timer_impl* t = *p->timers.begin();
        if (now <= t->get_time()) {
            break;
        }

        /* Fire it. */
        progress = true;
        p->timers.erase(p->timers.begin());
        p->all_timers.erase(t);
        try {
            t->func();
        } catch (const std::exception& e) {
            lg.err("Timer leaked an exception: %s", e.what());
        }
        delete t;

        if (serial != p->serial) {
            /* t->call() blocked and Timer_dispatcher::poll() was eventually
             * re-entered in another thread.  That other call already admitted
             * and fired our timers, so we are done. */
            break;
        }
    }
    return progress;
}

void
Timer_dispatcher::wait()
{
   /* Figure the amount of time left for a next callback or timer.  Moreover,
    * prepare the time adding condition variable to detect new timers while
    * dispatcher is being blocked. */
    if (!p->timers.empty()) {
        Timer_impl* t = *p->timers.begin();
        co_timer_wait(t->get_time(), NULL);
    }
    p->new_timers.wait();
}

bool
Timer_dispatcher::check_validity(Timer_impl* impl, 
                                 const unsigned int generation) const {
    const Timer_set& all_timers = p->all_timers;
    Timer_set::const_iterator i = all_timers.find(impl);
    return i != all_timers.end() && (*i)->generation == generation;
}


/* Timer implementation. */

void
Timer_impl::cancel()
{
    if (admitted) {
        dispatcher->p->timers.erase(this);
        admitted = false;
    } else {
        dispatcher->p->applicants.erase(this);
    }

    dispatcher->p->all_timers.erase(this);
}

void
Timer_impl::set_timeout(const timeval& when)
{
    cancel();
    time = when;
    dispatcher->p->applicants.insert(this);
    dispatcher->p->all_timers.insert(this);
}

void
Timer_impl::delay(bool neg, long sec, long usec)
{
    timeval delta = {sec, usec};
    set_timeout(neg ? time - delta : time + delta);
}

void
Timer_impl::reset(long sec, long usec)
{
    set_timeout(do_gettimeofday() + make_timeval(sec, usec));
}

Timer::Timer() 
    : dispatcher(0), impl(0), generation(0) {
}

Timer::Timer(Timer_dispatcher* dispatcher_, Timer_impl* impl_, 
             const unsigned int generation_)
    : dispatcher(dispatcher_), impl(impl_), generation(generation_) {
}

void
Timer::cancel() {
    if (dispatcher && dispatcher->check_validity(impl, generation)) {
        impl->cancel();
        delete impl;
        dispatcher = 0; 
        impl = 0;
    }
}

void 
Timer::delay(bool neg, long sec, long usec) {
    if (dispatcher && dispatcher->check_validity(impl, generation)) {
        impl->delay(neg, sec, usec);
    }
}

void 
Timer::reset(long sec, long usec) {
    if (dispatcher && dispatcher->check_validity(impl, generation)) {
        impl->reset(sec, usec);
    }
}

timeval 
Timer::get_time() const {
    if (dispatcher && dispatcher->check_validity(impl, generation)) {
        return impl->get_time();
    }
    return timeval();
}

} // namespace vigil
