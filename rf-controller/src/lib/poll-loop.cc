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
#include "poll-loop.hh"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <queue>
#include <vector>
#include "fault.hh"
#include "threads/cooperative.hh"

namespace vigil {

namespace {

struct Poll_thread
{
    Co_sema wakeup;
    co_thread* thread;
};

} // null namespace

class Poll_loop_impl
{
public:
    Poll_loop_impl(unsigned int n_threads);
    ~Poll_loop_impl() { }
    void add_pollable(Pollable*);
    void remove_pollable(Pollable*);
    void run();
private:
    boost::ptr_vector<Poll_thread> threads;
    Poll_thread* polling_thread;
    std::deque<Poll_thread*> dormant_threads;

    Co_cond new_pollable;
    Co_sema started;
    typedef std::list<Pollable_ref> Pollable_list;
    Pollable_list pollables;

    void poll_thread_main(Poll_thread*);
    bool service_pollables(Poll_thread*);
    void appoint_polling_thread(Pollable_list::iterator i);
};

Poll_loop_impl::Poll_loop_impl(unsigned int n_threads)
{
    assert(n_threads > 0);

    polling_thread = NULL;
    threads.reserve(n_threads);
    for (int i = 0; i < n_threads - 1; i++) {
        Poll_thread* pt = new Poll_thread;
        threads.push_back(pt);
        pt->thread = co_thread_create(
            &co_group_coop,
            boost::bind(&Poll_loop_impl::poll_thread_main, this, pt));
        started.down();
    }
}

void
Poll_loop_impl::add_pollable(Pollable* p)
{
    Pollable_ref ref = {p, 0};
    p->pollables_pos = pollables.insert(pollables.end(), ref);
    new_pollable.broadcast();
}

void
Poll_loop_impl::remove_pollable(Pollable* p)
{
    Pollable_ref& ref = *p->pollables_pos;
    assert(ref.pollable);
    ref.pollable = NULL;
    if (ref.ref_cnt == 0) {
        pollables.erase(p->pollables_pos);
    }
}

void
Poll_loop_impl::run()
{
    Poll_thread* pt = new Poll_thread;
    threads.push_back(pt);
    pt->thread = co_self();
    polling_thread = pt;
    poll_thread_main(pt);
    threads.pop_back();
}

#define NEXT_WITH_REFERENCE(CODE) \
    do {                                        \
        Pollable_ref& ref = *i;                 \
        ++ref.ref_cnt;                          \
        if (ref.pollable) {                     \
            CODE;                               \
        }                                       \
        if (!--ref.ref_cnt && !ref.pollable) {  \
            i = pollables.erase(i);             \
        } else {                                \
            ++i;                                \
        }                                       \
    } while (0)
void
Poll_loop_impl::poll_thread_main(Poll_thread* pt)
{
    started.up();
    for (;;) {
        /* Wait until we become the polling thread. */
        if (pt != polling_thread) {
            dormant_threads.push_front(pt);
            pt->wakeup.down();
            assert(pt == polling_thread);
        }

        /* Service pollables. */
        while (service_pollables(pt)) {
            co_poll();
            co_yield();
        }

        if (pt == polling_thread) {
            /* We are still the polling thread, but nobody has any work. */
            co_might_yield();

            Pollable_list::iterator end = pollables.end();
            for (Pollable_list::iterator i = pollables.begin(); i != end; ) {
                NEXT_WITH_REFERENCE(ref.pollable->wait());
            }
            new_pollable.wait();
            co_block();
        } else {
            /* Some other thread is now the polling thread. */
        }
    }
}

bool
Poll_loop_impl::service_pollables(Poll_thread* pt)
{
    assert(pt == polling_thread);

    bool progress = false;
    Pollable_list::iterator end = pollables.end();
    for (Pollable_list::iterator i = pollables.begin(); i != end; ) {
        NEXT_WITH_REFERENCE(
            co_set_preblock_hook(
                boost::bind(&Poll_loop_impl::appoint_polling_thread, this, i));
            if (ref.pollable->poll()) {
                progress = true;
            }
            co_unset_preblock_hook();
        );

        if (polling_thread != pt) {
            /* p.poll() blocked somewhere, so we got removed as the polling
             * thread. */
            if (polling_thread) {
                /* Someone else is the polling thread now. */
                return false;
            } else {
                /* Everyone was busy, so we might as well reclaim the position.
                 * But there's no sense in retaining our current notion of
                 * progress, so we might as well return and let the caller
                 * start another loop.  This has the cost of a call to
                 * co_poll(), but we need to make sure that that gets called
                 * sometime anyhow; it should have a relatively small cost
                 * relative to the cost of blocking and switching threads. */
                polling_thread = pt;
                return true;
            }
        }
    }
    return progress;
}

void
Poll_loop_impl::appoint_polling_thread(Pollable_list::iterator i)
{
    /* Rotate the pollables list in-place, so that the next dispatcher we wake
     * up will resume from where we left off.  Otherwise pollables toward the
     * end of the list will get starved if earlier ones tend to block. */
    pollables.splice(pollables.begin(), pollables, ++i, pollables.end());

    if (!dormant_threads.empty()) {
        polling_thread = dormant_threads.front();
        dormant_threads.pop_front();
        polling_thread->wakeup.up();
    } else {
        polling_thread = NULL;
    }
}

/* Implement Poll_loop_impl in terms of Poll_loop. */

Poll_loop::Poll_loop(unsigned int n_threads)
    : pimpl(new Poll_loop_impl(n_threads))
{
}

Poll_loop::~Poll_loop()
{
    delete pimpl;
}

void
Poll_loop::add_pollable(Pollable* p)
{
    pimpl->add_pollable(p);
}

void
Poll_loop::remove_pollable(Pollable* p)
{
    pimpl->remove_pollable(p);
}

void
Poll_loop::run()
{
    pimpl->run();
}

} // namespace vigil
