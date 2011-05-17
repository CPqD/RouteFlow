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
#include "threads/native.hh"
#include <errno.h>
#include <stdio.h>

namespace vigil {

/* Native_thread. */

/* Constructs a Native_thread that does not yet represent any thread. */
Native_thread::Native_thread()
{}

/* Constructs a new Native_thread that represents a newly created native thread
 * that calls 'run'. */
Native_thread::Native_thread(const boost::function<void()>& run)
{
    start(run);
}

/* Constructs a new Native_thread that represents 'thread'. */
Native_thread::Native_thread(co_thread* thread_)
    : Task(thread_)
{}

/* Starts a new thread represented by this Native_thread, which must have been
 * created with the default constructor.  The new thread calls the 'run'
 * function. */
void
Native_thread::start(const boost::function<void()>& run)
{
    assert(empty());
    thread = co_thread_create(NULL, run);
}

/* Migrates the running thread to 'group'.  If 'group' is non-null, then the
 * running thread becomes a cooperative thread. */
void
Native_thread::migrate(co_group* group)
{
    co_migrate(group);
}

/* Helper macros. */

/* If EXPRESSION is nonzero, aborts the program with a message referring to
 * the error value to which EXPRESSION evaluates.
 *
 * Has no effect when NDEBUG is defined; should only be used for errors that
 * indicate a program bug. */
#ifndef NDEBUG
#define MUST_SUCCEED(EXPRESSION)                                \
    do {                                                        \
        int error = (EXPRESSION);                               \
        if (error) {                                            \
            fprintf(stderr, "%s in %s", #EXPRESSION, __func__); \
            fprintf(stderr, " (%s)", strerror(error));          \
            exit(EXIT_FAILURE);                                 \
        }                                                       \
    } while (0)
#else
#define MUST_SUCCEED(EXPRESSION) ((void) (EXPRESSION))
#endif

/* If EXPRESSION is nonzero, aborts the program with a message referring to the
 * error value in errno.
 *
 * Has no effect when NDEBUG is defined; should only be used for errors that
 * indicate a program bug. */
#ifndef NDEBUG
#define MUST_SUCCEED_ERRNO(EXPRESSION)                          \
    do {                                                        \
        if (EXPRESSION) {                                       \
            fprintf(stderr, "%s in %s", #EXPRESSION, __func__); \
            fprintf(stderr, " (%s)", strerror(errno));          \
            exit(EXIT_FAILURE);                                 \
        }                                                       \
    } while (0)
#else
#define MUST_SUCCEED_ERRNO(EXPRESSION) ((void) (EXPRESSION))
#endif

/* If EXPRESSION is nonzero and different from ALLOWED, aborts the program with
 * a message referring to the error value to which EXPRESSION evaluates.
 *
 * Has no effect when NDEBUG is defined; should only be used for errors that
 * indicate a program bug. */
#ifndef NDEBUG
#define CHECK_ERROR(ERROR, ALLOWED)             \
    do {                                        \
        if ((ERROR) && (ERROR) != (ALLOWED)) {  \
            fprintf(stderr, "%s", __func__);     \
            fprintf(stderr, " (%s)", strerror((ERROR)));          \
            exit(EXIT_FAILURE);                                 \
        }                                       \
    } while (0)
#else
#define CHECK_ERROR(ERROR, ALLOWED) ((void) 0)
#endif

/* Native_mutex. */

Native_mutex::Native_mutex()
{
#ifndef NDEBUG
    pthread_mutexattr_t attr;
    MUST_SUCCEED(pthread_mutexattr_init(&attr));
    MUST_SUCCEED(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK));
    MUST_SUCCEED(pthread_mutex_init(&mutex, &attr));
    MUST_SUCCEED(pthread_mutexattr_destroy(&attr));
#else
    MUST_SUCCEED(pthread_mutex_init(&mutex, NULL));
#endif
}

Native_mutex::~Native_mutex()
{
    MUST_SUCCEED(pthread_mutex_destroy(&mutex));
}

/* Locks the mutex, which must not be locked by the current thread. */
void
Native_mutex::lock()
{
    MUST_SUCCEED(pthread_mutex_lock(&mutex));
}

/* Unlocks the mutex, which must be locked by the current thread. */
void
Native_mutex::unlock()
{
    MUST_SUCCEED(pthread_mutex_unlock(&mutex));
}

/* Attempts to lock the mutex.  Returns true if successful, false on lock
 * contention. */
bool
Native_mutex::try_lock()
{
    int error = pthread_mutex_trylock(&mutex);
    CHECK_ERROR(error, EBUSY);
    return error == 0;
}

#ifdef HAVE_TIMED_LOCKS
/* Attempts to lock the mutex until 'abs_time'.  Returns true if successful,
 * false on timeout. */
bool
Native_mutex::timed_lock(const timespec& abs_time)
{
    int error = pthread_mutex_timedlock(&mutex, &abs_time);
    CHECK_ERROR(error, ETIMEDOUT);
    return error == 0;
}
#endif // HAVE_TIMED_LOCKS

/* Scoped_native_mutex. */

/* Acquires 'mutex', if 'mutex' is nonnull. */
Scoped_native_mutex::Scoped_native_mutex(Native_mutex* mutex_)
    : mutex(mutex_)
{
    if (mutex) {
        mutex->lock();
    }
}

/* Release the mutex, if one was acquired and not yet released. */
Scoped_native_mutex::~Scoped_native_mutex()
{
    if (mutex) {
        mutex->unlock();
    }
}

/* Releases the mutex early, if one was acquired and not yet released.  Calling
 * multiple times has no additional effect.
 * Marking an instance of Scoped_native_mutex 'const' disables this
 * function. */
void
Scoped_native_mutex::unlock()
{
    if (mutex) {
        mutex->unlock();
        mutex = NULL;
    }
}

/* Native_recursive_mutex. */

Native_recursive_mutex::Native_recursive_mutex()
{
    pthread_mutexattr_t attr;
    MUST_SUCCEED(pthread_mutexattr_init(&attr));
    MUST_SUCCEED(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE));
    MUST_SUCCEED(pthread_mutex_init(&mutex, &attr));
    MUST_SUCCEED(pthread_mutexattr_destroy(&attr));
}

Native_recursive_mutex::~Native_recursive_mutex()
{
    MUST_SUCCEED(pthread_mutex_destroy(&mutex));
}

/* Locks the mutex, blocking as necessary. */
void
Native_recursive_mutex::lock()
{
    MUST_SUCCEED(pthread_mutex_lock(&mutex));
}

/* Unlocks the mutex, which must have been locked by this thread without a
 * corresponding unlock(). */
void
Native_recursive_mutex::unlock()
{
    MUST_SUCCEED(pthread_mutex_unlock(&mutex));
}

/* Attempts to lock the mutex.  Returns true if successful, false on lock
 * contention. */
bool
Native_recursive_mutex::try_lock()
{
    int error = pthread_mutex_trylock(&mutex);
    CHECK_ERROR(error, EBUSY);
    return error == 0;
}

#ifdef HAVE_TIMED_LOCKS
/* Attempts to lock the mutex until 'abs_time'.  Returns true if successful,
 * false on timeout. */
bool
Native_recursive_mutex::timed_lock(const timespec& abs_time)
{
    int error = pthread_mutex_timedlock(&mutex, &abs_time);
    CHECK_ERROR(error, ETIMEDOUT);
    return error == 0;
}
#endif // HAVE_TIMED_LOCKS

/* Scoped_native_recursive_mutex. */

/* Acquires 'mutex', if 'mutex' is nonnull. */
Scoped_native_recursive_mutex::Scoped_native_recursive_mutex(
    Native_recursive_mutex* mutex_)
    : mutex(mutex_)
{
    if (mutex) {
        mutex->lock();
    }
}

/* Releases the mutex, if one was acquired and not yet released. */
Scoped_native_recursive_mutex::~Scoped_native_recursive_mutex()
{
    if (mutex) {
        mutex->unlock();
    }
}

/* Releases the mutex early, if one was acquired and not yet released.  Calling
 * multiple times has no additional effect.
 * Marking an instance of Scoped_native_mutex 'const' disables this
 * function. */
void
Scoped_native_recursive_mutex::unlock()
{
    if (mutex) {
        mutex->unlock();
        mutex = NULL;
    }
}

/* Native_cond. */

Native_cond::Native_cond()
{
    MUST_SUCCEED(pthread_cond_init(&cond, NULL));
}

Native_cond::~Native_cond()
{
    MUST_SUCCEED(pthread_cond_destroy(&cond));
}

/* Wakes up one thread waiting on the condition variable (if any are
 * waiting). */
void
Native_cond::signal()
{
    MUST_SUCCEED(pthread_cond_signal(&cond));
}

/* Wakes up all threads waiting on the condition variable (if any are
 * waiting). */
void
Native_cond::broadcast()
{
    MUST_SUCCEED(pthread_cond_broadcast(&cond));
}

/* Atomically releases 'mutex', waits for the condition variable to be
 * signaled, and re-acquires 'mutex'. */
void
Native_cond::wait(Scoped_native_mutex& mutex)
{
    MUST_SUCCEED(pthread_cond_wait(&cond, &mutex.mutex->mutex));
}

/* Atomically releases 'mutex', waits for the condition variable to be
 * signaled, and re-acquires 'mutex'. */
void
Native_cond::wait(Scoped_native_recursive_mutex& mutex)
{
    MUST_SUCCEED(pthread_cond_wait(&cond, &mutex.mutex->mutex));
}

/* Atomically releases 'mutex', waits for the condition variable to be signaled
 * or until 'abs_time' (whichever comes first), and re-acquires 'mutex'.
 *
 * Returns true if 'mutex' was successfully acquired, false on timeout. */
bool
Native_cond::timed_wait(Scoped_native_mutex& mutex, const timespec& abs_time)
{
    int error = pthread_cond_timedwait(&cond, &mutex.mutex->mutex, &abs_time);
    CHECK_ERROR(error, ETIMEDOUT);
    return error == 0;
}

/* Atomically releases 'mutex', waits for the condition variable to be signaled
 * or until 'abs_time' (whichever comes first), and re-acquires 'mutex'.
 *
 * Returns true if 'mutex' was successfully acquired, false on timeout. */
bool
Native_cond::timed_wait(Scoped_native_recursive_mutex& mutex,
                        const timespec& abs_time)
{
    int error = pthread_cond_timedwait(&cond, &mutex.mutex->mutex, &abs_time);
    CHECK_ERROR(error, ETIMEDOUT);
    return error == 0;
}

/* Native_sema. */

Native_sema::Native_sema()
{
    MUST_SUCCEED_ERRNO(sem_init(&sema, false, 0));
}

Native_sema::~Native_sema()
{
    MUST_SUCCEED_ERRNO(sem_destroy(&sema));
}

/* Increments the semaphore's value. */
void
Native_sema::up()
{
    MUST_SUCCEED_ERRNO(sem_post(&sema));
}

/* Decrements the semaphore's value, first waiting for it to become positive
 * if necessary. */
void
Native_sema::down()
{
    /* Unlike the pthread_*() functions, sem_wait() is interruptible by
     * signals. */
    while (sem_wait(&sema) < 0) {
        if (errno != EINTR) {
#ifndef NDEBUG
            fprintf(stderr, "sem_wait failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
#else
            break;
#endif
        }
    }
}

/* Attempts to decrement the semaphore's value.
 * Returns true if successful, false if the semaphore's value was 0. */
bool
Native_sema::try_down()
{
    if (!sem_trywait(&sema)) {
        return true;
    } else {
        CHECK_ERROR(errno, EAGAIN);
        return false;
    }
}

#ifdef HAVE_TIMED_LOCKS
/* Attempts to decrement the semaphore's value, waiting until no later than
 * 'abs_time' to do so.
 * Returns true if successful, false on timeout. */
bool
Native_sema::timed_down(const timespec& abs_time)
{
    if (!sem_timedwait(&sema, &abs_time)) {
        return true;
    } else {
        CHECK_ERROR(errno, ETIMEDOUT);
        return false;
    }
}
#endif // HAVE_TIMED_LOCKS

/* Native_rwlock. */

Native_rwlock::Native_rwlock() 
{
    MUST_SUCCEED(pthread_rwlock_init(&rwlock, NULL));
}

Native_rwlock::~Native_rwlock() 
{
    MUST_SUCCEED(pthread_rwlock_destroy(&rwlock));
}

/* Acquires a lock for reading, blocking as necessary. */
void
Native_rwlock::read_lock() 
{
    MUST_SUCCEED(pthread_rwlock_rdlock(&rwlock));
}

/* Attempts to obtain a read lock, blocking until no later that 'abs_time'.
 * Returns true if successful, false on timeout. */
bool
Native_rwlock::read_timed_lock(const timespec& abs_time)
{
    int error = pthread_rwlock_timedrdlock(&rwlock, &abs_time);
    CHECK_ERROR(error, ETIMEDOUT);
    return error == 0;
}

/* Attempts to obtain a read lock, without blocking.
 * Returns true if successful, false if the lock could not be acquired
 * immediately. */
bool
Native_rwlock::read_try_lock()
{
    int error = pthread_rwlock_tryrdlock(&rwlock);
    CHECK_ERROR(error, EBUSY);
    return error == 0;
}

/* Releases a read lock. */
void
Native_rwlock::read_unlock() 
{
    MUST_SUCCEED(pthread_rwlock_unlock(&rwlock));
}

/* Acquires a lock for writing, blocking as necessary. */
void
Native_rwlock::write_lock() 
{
    MUST_SUCCEED(pthread_rwlock_wrlock(&rwlock));
}

/* Attempts to obtain a write lock, without blocking.
 * Returns true if successful, false if the lock could not be acquired
 * immediately. */
bool
Native_rwlock::write_try_lock()
{
    int error = pthread_rwlock_trywrlock(&rwlock);
    CHECK_ERROR(error, EBUSY);
    return error == 0;
}

/* Attempts to obtain a write lock, blocking until no later that 'abs_time'.
 * Returns true if successful, false on timeout. */
bool
Native_rwlock::write_timed_lock(const timespec& abs_time)
{
    int error = pthread_rwlock_timedwrlock(&rwlock, &abs_time);
    CHECK_ERROR(error, ETIMEDOUT);
    return error == 0;
}

/* Releases a write lock. */
void
Native_rwlock::write_unlock() 
{
    MUST_SUCCEED(pthread_rwlock_unlock(&rwlock));
}

/* Native_barrier. */

/* Initializes the barrier to be passable when 'count' threads call the wait()
 * function. */
Native_barrier::Native_barrier(unsigned int count)
{
    MUST_SUCCEED(pthread_barrier_init(&barrier, NULL, count));
}

Native_barrier::~Native_barrier() 
{
    MUST_SUCCEED(pthread_barrier_destroy(&barrier));
}

/* Waits until 'count' threads are all waiting in wait().
 * Returns true in one arbitrarily selected thread, false in all others. */
bool
Native_barrier::wait() 
{
    int error = pthread_barrier_wait(&barrier);
    CHECK_ERROR(error, PTHREAD_BARRIER_SERIAL_THREAD);
    return error == PTHREAD_BARRIER_SERIAL_THREAD;
}

} // namespace vigil
