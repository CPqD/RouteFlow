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
#ifndef POLL_LOOP_HH
#define POLL_LOOP_HH 1

#include <boost/noncopyable.hpp>
#include <list>
#include <cassert>

namespace vigil {

class Pollable;
class Poll_loop;
class Poll_loop_impl;

/* Implementation detail not of interest to client code. */
struct Pollable_ref
{
    Pollable* pollable;
    int ref_cnt;
};

/* Represents an activity that needs periodic attention. */
class Pollable
    : boost::noncopyable
{
public:
    virtual ~Pollable() { }

    /* Called by the poll loop from any of the threads in its pool of threads.
     * May block.  If it blocks, then it may be called reentrantly from another
     * thread, and so on if that thread blocks.  It may prevent being called
     * again by removing itself from the Poll_loop's set of Pollables with
     * remove_pollable().
     *
     * While the Pollable runs, no other Pollables can run.  Thus, it should
     * limit the amount of work it can do without either blocking or returning,
     * both of which allow other Pollables to run.  Yielding with co_yield()
     * does allow other threads to run, including other Pollables that have
     * blocked, but it does not allow other Pollables to be polled.
     *
     * Should return true if some work was accomplished or if the Pollable
     * wants to be called again without waiting, false if the Pollable had no
     * work to do and has no need to be called again for now. */
    virtual bool poll() = 0;

    /* Called by the poll loop when all Pollables have returned false.  Should
     * call a wait function (such as co_fd_read_wait()) that will allow the
     * thread to wake up when there is some work to do.  If there is already
     * work to do, should call co_immediate_wake().  Must not block. */
    virtual void wait() = 0;
private:
    friend class Poll_loop_impl;

    /* Position in owning Poll_loop's list of Pollables. */
    std::list<Pollable_ref>::iterator pollables_pos;
};

/* A loop that polls a set of Pollables in round-robin fashion, forever.  */
class Poll_loop
{
public:
    Poll_loop(unsigned int n_threads);
    ~Poll_loop();

    /* Add 'p' to the set of Pollables to poll.  'p' must not currently be in
     * this Poll_loop's set of Pollables (or any other Poll_loop's set). */
    void add_pollable(Pollable* p);

    /* Remove 'p' from the set of Pollables to poll.  'p' must be currently in
     * the Poll_loop's set of Pollables.  Afterward, 'p' will not be polled or
     * otherwise referenced again.  'p' may be currently running any number of
     * poll operations, which may complete normally.  */
    void remove_pollable(Pollable* p);
    void run();    
private:
    Poll_loop_impl* pimpl;
};

} // namespace vigil

#endif /* poll-loop.hh */
