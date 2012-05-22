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
#include <stdint.h>
#include "threads/cooperative.hh"

namespace vigil {

/* Condition variables. */

/* Wakes up a single thread waiting on 'this'. */
void
Co_cond::signal()
{
    wq.wake_n(1, 1);
}

/* Wakes up all threads waiting on 'this'. */
void
Co_cond::broadcast()
{
    wq.wake_all(1);
}

/* Blocks the running thread until 'this' is signaled. */
void
Co_cond::block()
{
    co_might_yield();
    wait();
    co_block();
}

/* Sets up to wait for 'this' to be signaled.
 *
 * co_block() must be called to actually wait for the event to occur.  No
 * blocking function may be called between calling this function and calling
 * co_block().
 *
 * If 'signaled' is non-null, then after co_block() returns, '*signaled' will
 * be set to 1 if 'this' is signaled and otherwise to 0. */
void
Co_cond::wait(int *signaled)
{
    wq.wait(signaled);
}

/* Mutex. */

/* Initializes 'this' as a mutex. */
Co_mutex::Co_mutex()
{
    owner = NULL;
}

/* Returns the thread that currently owns 'this', or a null pointer if no
 * thread currently owns 'this'. */
co_thread*
Co_mutex::get_owner() const
{
    return owner;
}

/* Locks 'this', blocking as necessary. */
void
Co_mutex::lock()
{
    co_might_yield();
    assert(owner != co_self());
    if (!try_lock()) {
        wq.wait(NULL);
        co_block();
        assert(owner == co_self());
    }
}

/* Attempts to lock 'this', without blocking.  Returns true if successful,
 * false otherwise. */
bool
Co_mutex::try_lock()
{
    if (owner == NULL) {
        owner = co_self();
        return true;
    } else {
        return false;
    }
}

/* Unlocks 'this' (which must be locked by the current thread). */
void
Co_mutex::unlock()
{
    assert(owner == co_self());
    owner = wq.wake_1(1);
}

/* Registers the current thread as attempting to lock 'this'.  The next call to
 * co_block() or co_fsm_block() will wake up when 'this' is successfully locked
 * by the current thread.
 *
 * co_block() must be called to actually wait for the event to occur.  No
 * blocking function may be called between calling this function and calling
 * co_block().
 *
 * When co_block() returns, if 'acquired' is non-null then '*acquired' will be
 * set to 1 if the mutex was acquired, otherwise to 0. */
void
Co_mutex::wait(int *acquired)
{
    if (!try_lock()) {
        wq.wait(acquired);
    } else {
        co_immediate_wake(1, acquired);
    }
}

/* Reader-writer lock. */

/* Initializes 'this' as a reader-writer lock. */
Co_rwlock::Co_rwlock()
{
    write_owner = NULL;
    read_owners = 0;
}

/* Locks 'this' for reading, blocking as necessary. */
void
Co_rwlock::read_lock()
{
    co_might_yield();
    assert(write_owner != co_self());
    if (!read_try_lock()) {
        int acquired;
        read_wq.wait(&acquired);
        co_block();
        assert(acquired);
        assert(read_owners);
    }
}

/* Attempts to lock 'this' for reading, without blocking.  Returns true if
 * successful, false otherwise. */
bool
Co_rwlock::read_try_lock()
{
    if (!write_owner && write_wq.empty()) {
        read_owners++;
        return true;
    } else {
        return false;
    }
}

/* Unlocks 'this' for reading (which must be locked for reading by the
 * current thread). */
void
Co_rwlock::read_unlock()
{
    assert(read_owners);
    if (!--read_owners) {
        write_owner = write_wq.wake_1(1);
    }
}

/* Registers the current thread as attempting to lock 'this' for reading.  The
 * next call to co_block() or co_fsm_block() will wake up when 'this' is
 * successfully locked for reading by the current thread.
 *
 * co_block() must be called to actually wait for the event to occur.  No
 * blocking function may be called between calling this function and calling
 * co_block().
 *
 * When co_block() returns, if 'acquired' is non-null then '*acquired' will be
 * set to 1 if the read lock was acquired, otherwise to 0. */
void
Co_rwlock::read_wait(int *acquired)
{
    if (!read_try_lock()) {
        read_wq.wait(acquired);
    } else {
        co_immediate_wake(1, acquired);
    }
}

/* Locks 'this' for writing, blocking as necessary. */
void
Co_rwlock::write_lock()
{
    co_might_yield();
    assert(write_owner != co_self());
    if (!write_try_lock()) {
        int acquired;
        write_wq.wait(&acquired);
        co_block();
        assert(acquired);
        assert(write_owner == co_self());
    }
}

/* Attempts to lock 'this' for writing, without blocking.  Returns true if
 * successful, false otherwise. */
bool
Co_rwlock::write_try_lock()
{
    if (!read_owners && !write_owner) {
        write_owner = co_self();
        return true;
    } else {
        return false;
    }
}

/* Unlocks 'this' for writing (which must be locked for writing by the
 * current thread). */
void
Co_rwlock::write_unlock()
{
    assert(write_owner == co_self());
    write_owner = NULL;
    if (!read_wq.empty()) {
        read_owners = read_wq.wake_n(1, SIZE_MAX);
    } else {
        write_wq.wake_n(1, 1);
    }
}

/* Registers the current thread as attempting to lock 'this' for writing.  The
 * next call to co_block() or co_fsm_block() will wake up when 'this' is
 * successfully locked for reading by the current thread.
 *
 * co_block() must be called to actually wait for the event to occur.  No
 * blocking function may be called between calling this function and calling
 * co_block().
 *
 * When co_block() returns, if 'acquired' is non-null then '*acquired' will be
 * set to 1 if the write lock was acquired, otherwise to 0. */
void
Co_rwlock::write_wait(int *acquired)
{
    if (!write_try_lock()) {
        write_wq.wait(acquired);
    } else {
        co_immediate_wake(1, acquired);
    }
}

/* Initializes 'this' as a semaphore with initial value 0.
   (Call up() to increase the initial value.) */
Co_sema::Co_sema()
{
    value = 0;
}

/* Decrements the value of 'this', first blocking until it becomes positive if
 * necessary. */
void
Co_sema::down()
{
    co_might_yield();
    if (!try_down()) {
        int downed;
        wq.wait(&downed);
        co_block();
        assert(downed);
    }
}

/* If 'this' has a positive value, decrements it and returns true.  Otherwise,
 * returns false without modifying the value of 'this'. */
bool
Co_sema::try_down()
{
    if (value > 0) {
        --value;
        return true;
    } else {
        return false;
    }
}

/* Increments the value of 'this'. */
void
Co_sema::up()
{
    if (!wq.empty()) {
        wq.wake_n(1, 1);
    } else {
        ++value;
    }
}

/* Registers the current thread as attempting to decrement 'this'.  The
 * next call to co_block() or co_fsm_block() will wake up when 'this' is
 * successfully decremented by the current thread.
 *
 * co_block() must be called to actually wait for the event to occur.  No
 * blocking function may be called between calling this function and calling
 * co_block().
 *
 * When co_block() returns, if 'downed' is non-null then '*downed' will be set
 * to 1 if 'this' was downed by the current thread, otherwise to 0. */
void
Co_sema::wait(int* downed)
{
    if (value > 0) {
        --value;
        co_immediate_wake(1, downed);
    } else {
        wq.wait(downed);
    }
}

/* Initializes 'this' as a barrier that must be reached by 'count' threads
 * to be broken. */
Co_barrier::Co_barrier(unsigned int count_)
    : count(count_)
{
}

/* Blocks until the expected number of threads (including the current thread)
 * reaches 'this'. */
void
Co_barrier::block()
{
    co_might_yield();
    assert(count > 0);
    if (--count) {
        wq.wait(NULL);
        co_block();
        assert(!count);
    } else {
        wq.wake_all(1);
    }
}

/* Initializes 'this' as an unreleased completion. */
Co_completion::Co_completion()
    : complete(false)
{
}

/* Re-latches 'this'.  Threads that call wait() or block() will have to wait
 * until release() is called. */
void
Co_completion::latch() 
{
    complete = false;
}

/* Releases 'this', awakening any threads waiting for 'this' to complete. */
void
Co_completion::release()
{
    if (!complete) {
        complete = true;
        wq.wake_all(1);
    }
}

/* Returns true if release() has been called for this completion, otherwise
 * false.  */
bool
Co_completion::is_complete()
{
    return complete;
}

/* Causes the next call to co_block() or co_fsm_block() to wake up when
 * release() is called for 'this'.  If release() has already been called, then
 * the call to co_block() will not block at all.
 *
 * co_block() must be called to actually wait for the event to occur.  No
 * blocking function may be called between calling this function and calling
 * co_block().
 *
 * When co_block() returns, if 'released' is non-null then '*released' will be
 * set to 1 if the completion was released, otherwise to 0. */
void
Co_completion::wait(int *released)
{
    if (!complete) {
        wq.wait(released);
    } else {
        co_immediate_wake(1, released);
    }
}

/* Blocks the current thread until 'this' is released. */
void
Co_completion::block()
{
    co_might_yield();
    wait(NULL);
    co_block();
}

} // namespace vigil
