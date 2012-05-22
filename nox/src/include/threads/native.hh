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
#ifndef THREADS_NATIVE_HH
#define THREADS_NATIVE_HH 1

#include <boost/noncopyable.hpp>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include "threads/task.hh"

// Currently unused and not supported on NetBSD
#ifdef HAVE_TIMED_LOCKS
#undef HAVE_TIMED_LOCKS 
#endif

namespace vigil {

/* A native thread, not part of any thread group. */
class Native_thread
    : public Task
{
public:
    Native_thread();
    Native_thread(const boost::function<void()>& run);
    Native_thread(co_thread*);

    void start(const boost::function<void()>& run);

    /* Changing into a thread group.
     *
     * If a nonnull 'group' is passed in, this function migrates the current
     * thread into a thread group, becoming a cooperative thread.  If that is
     * done, calling the other Co_thread member functions is no longer
     * appropriate.  Further thread manipulation should be done using a
     * Co_thread obtained via, e.g., Co_thread::self(). */
    static void migrate(co_group* group);

    /* There is no 'join' function.
     *
     *   - A native thread can wait for another native thread to complete by
     *     downing a Native_sema passing into the joined thread's initial
     *     function.
     *
     *   - A native thread can wait for a cooperative thread or FSM to complete
     *     by migrating into the thread or FSM's group, then using
     *     co_join_completion(), and waiting on the completion.
     *
     *   - A cooperative thread or FSM can wait for another cooperative thread
     *     or FSM by using co_join_completion and waiting on the completion.
     *
     *   - A cooperative thread can wait for a native thread to complete by
     *     migrating itself out of its thread group (becoming a native thread)
     *     and downing a Native_sema passing into the joined thread's initial
     *     function.
     *
     *   - An FSM cannot (easily) wait for a native thread to complete.
     *
     * Don't use pthread_join() to wait for native threads: there is a race
     * between obtaining the thread's pthread_t and the destruction of the
     * co_thread following the thread's death.
     */
};

/* A native, nonrecursive mutex. */
class Native_mutex
    : boost::noncopyable
{
public:
    Native_mutex();
    ~Native_mutex();
    void lock();
    void unlock();
    bool try_lock();
#ifdef HAVE_TIMED_LOCKS    
     bool timed_lock(const timespec& abs_time);
#endif
private:
    friend class Native_cond;
    pthread_mutex_t mutex;
};

/* Takes and releases a nonrecursive mutex within a scope. */
class Scoped_native_mutex
    : boost::noncopyable
{
public:
    Scoped_native_mutex(Native_mutex* mutex);
    ~Scoped_native_mutex();
    void unlock();
private:
    friend class Native_cond;
    Native_mutex* mutex;
};

/* A native, recursive mutex. */
class Native_recursive_mutex
    : boost::noncopyable
{
public:
    Native_recursive_mutex();
    ~Native_recursive_mutex();
    void lock();
    void unlock();
    bool try_lock();
#ifdef HAVE_TIMED_LOCKS    
    bool timed_lock(const timespec& abs_time);
#endif
private:
    friend class Native_cond;
    pthread_mutex_t mutex;
};

/* Takes and releases a recursive mutex within a scope. */
class Scoped_native_recursive_mutex
    : boost::noncopyable
{
public:
    Scoped_native_recursive_mutex(Native_recursive_mutex*);
    ~Scoped_native_recursive_mutex();
    void unlock();
private:
    friend class Native_cond;
    Native_recursive_mutex* mutex;
};

/* A native condition variable.
 *
 * Must be paired with a Native_mutex or Native_recursive_mutex. */
class Native_cond
    : boost::noncopyable
{
public:
    Native_cond();
    ~Native_cond();
    void signal();
    void broadcast();
    void wait(Scoped_native_mutex&);
    void wait(Scoped_native_recursive_mutex&);
    bool timed_wait(Scoped_native_mutex&, const timespec& abs_time);
    bool timed_wait(Scoped_native_recursive_mutex&, const timespec& abs_time);
private:
    pthread_cond_t cond;
};

/* A native counting semaphore, initially 0. */
class Native_sema
    : boost::noncopyable
{
public:
    Native_sema();
    ~Native_sema();
    void up();
    void down();
    bool try_down();
#ifdef HAVE_TIMED_LOCKS
    bool timed_down(const timespec& abs_time);
#endif
private:
    sem_t sema;
};

/* A native reader-writer lock.
 *
 * The read side is recursive, the write side is not. */
class Native_rwlock
    : boost::noncopyable
{
public:
    Native_rwlock();
    ~Native_rwlock();

    void read_lock();
    bool read_try_lock();
    bool read_timed_lock(const timespec& abs_time);
    void read_unlock();

    void write_lock();
    bool write_try_lock();
    bool write_timed_lock(const timespec& abs_time);
    void write_unlock();

    /* Returns true if successful, false on lock contention. */

    /* Returns true if successful, false on timeout. */
private:
    pthread_rwlock_t rwlock;
};

/* A native barrier. */
class Native_barrier
    : boost::noncopyable
{
public:
    Native_barrier(unsigned int count);
    ~Native_barrier();
    bool wait();
private:
    pthread_barrier_t barrier;
};

} // namespace vigil

#endif /* native-thread.hh */
