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
/*
 * High-level cooperative thread support.
 *
 * This builds a more object-oriented layer on top of the low-level thread
 * support in threads/impl.hh.
 */

#ifndef THREADS_COOPERATIVE_HH
#define THREADS_COOPERATIVE_HH 1

#include <assert.h>
#include <boost/noncopyable.hpp>
#include "threads/task.hh"

namespace vigil {

/* Common behavior for cooperative threads (Co_thread objects) and FSMs (Co_fsm
 * objects).
 *
 * A cooperative thread can become a native thread, and vice versa, but this is
 * not reflected in the class hierarchy.  See threads/task.hh for more
 * information.
 *
 * Destroying a Co_task has no effect on the underlying cooperative thread or
 * FSM.  On the other hand, when a cooperative thread or FSM terminates, any
 * Co_thread or Co_fsm that represents it immediately becomes invalidated.  To
 * avoid accessing deallocated memory, any code that references a thread that
 * might terminate should check whether that thread has terminated using a
 * Co_completion and the join_completion() function below.  The cooperative
 * scheduler ensures that a thread cannot terminate asynchronously; use
 * critical section (see Co_critical_section below) to provide higher
 * assurance.
 */
class Co_task
    : public Task
{
public:
    Co_task();
    Co_task(co_thread*);

    /* Checking other threads for readiness. */
    static void poll(void);

    /* File descriptors.  */
    static void fd_wait(int fd, enum co_fd_wait_type, int *revents = 0);
    static void fd_read_wait(int fd, int *revents = 0);
    static void fd_write_wait(int fd, int *revents = 0);
    static void fd_closed(int fd);

    /* Timers. */
    static void timer_wait(timeval abs_time, int *expired = 0);

    /* Waiting for threads to die.
     *
     * A cooperative thread or FSM can wait for another cooperative thread or
     * FSM to complete by using this function and waiting on the completion.
     */
    void join_completion(Co_completion*);
};

/* Cooperative threads.
 *
 * Cooperative threads within a single group are scheduled cooperatively with
 * respect to each other, but preemptively with respect to cooperative threads
 * in other groups and with respect to native threads.
 *
 * A cooperative thread must not block, because that also blocks the execution
 * of other threads in the same thread group.  Instead, it should use the
 * "wait" and "block" functions in Co_thread and Co_task to allow other threads
 * to run when it would otherwise block.
 *
 * Destroying a Co_thread has no effect on the underlying cooperative thread.
 * The converse is not true; see the comment on Co_task above for more
 * information.
 *
 * This class is suitable for use as a class member or as a (likely protected
 * or private) base class.
 */
class Co_thread
    : public Co_task
{
public:
    Co_thread();
    Co_thread(const boost::function<void()>& run,
              co_group& group = co_group_coop);
    Co_thread(co_thread*);

    /* Return a Co_thread that represents the running thread, which should be
     * a cooperative thread. */
    static Co_thread self();

    /* Start a new thread represented by this Co_thread, which must have been
     * created with the default constructor. */
    void start(const boost::function<void()>& run,
               co_group& group = co_group_coop);

    /* Changing to another thread group or becoming a native thread. */
    static co_group* migrate(co_group*);

    /* Blocking.
     *
     * Before blocking, call one or more "wait functions" that register an
     * event to wait on.  By convention, wait function names end in "wait".
     * block() will return when at least one of the registered events has
     * occurred.  (If block() is called without any events being registered,
     * the thread will block forever.)
     *
     * Calling a wait function enters a critical section.  block() in turn
     * exits one critical section for each per registered event.  Thus, between
     * calling a wait function and calling block(), no blocking functions may
     * be called.
     */
    static void block();
    static void yield();

    /* File descriptors. */
    static int fd_block(int fd, enum co_fd_wait_type);
    static int fd_read_block(int fd);
    static int fd_write_block(int fd);

    /* Timers. */
    static void sleep(timeval duration);

    /*
     * Simple system call wrappers for use by cooperative threads.  These are
     * pretty cheap, especially if no blocking is required.  They require that
     * file descriptors be in non-blocking mode.  (They will set non-blocking
     * mode automatically in any new file descriptors that they return.)
     */
    static int socket(int name_space, int style, int protocol);
    static ssize_t read(int fd, void *, size_t);
    static ssize_t recv(int fd, void *, size_t, int flags);
    static ssize_t recvfrom(int fd, void *, size_t, int flags,
                            sockaddr *, socklen_t *);
    static ssize_t recvmsg(int fd, msghdr *, int flags);
    static ssize_t write(int fd, const void *, size_t);
    static ssize_t send(int fd, const void *, size_t, int flags);
    static ssize_t sendto(int fd, const void *, size_t, int flags,
                          sockaddr *, socklen_t);
    static ssize_t sendmsg(int fd, const msghdr *, int flags);
    static int connect(int fd, sockaddr *, socklen_t);
    static int accept(int fd, sockaddr *, socklen_t *);

    /*
     * More system call wrappers for use by cooperative threads.
     *
     * These are relatively expensive, even if no blocking is required, because
     * they are implemented by migrating the thread away from its group, and
     * then back to it, to avoid blocking other cooperative threads.
     *
     * To do more than one of these operations consecutively, consider using
     * Co_scoped_native_section directly, to avoid doing migration many times.
     */
    int async_open(const char *, int flags, mode_t);
    ssize_t async_read(int fd, void *, size_t);
    ssize_t async_pread(int fd, void *, size_t, off_t);
    ssize_t async_write(int fd, const void *, size_t);
    ssize_t async_pwrite(int fd, const void *, size_t, off_t);
    int async_ftruncate(int fd, off_t);
    int async_fchmod(int fd, mode_t);
    int async_fchown(int fd, uid_t, gid_t);
    int async_fsync(int fd);
    int async_fdatasync(int fd);
};

/* Finite state machines (FSMs).
 *
 * An FSM is scheduled like a cooperative thread and for most purposes it may
 * be treated in the same way as a cooperative thread.  However, FSMs are
 * stackless: they may not block.  Instead, the FSM's 'start' function is
 * invoked anew each time it is scheduled.  Before the start function returns,
 * it must do exactly one of the following:
 *
 *      - Call one or more wait functions (e.g. co_fd_wait(), Co_sema::wait()),
 *        then block().  The FSM will be invoked again after one or more of the
 *        events occurs.
 *
 *      - Call transition() with a replacement 'start'.  The FSM will be
 *        invoked again with the new function and argument the next time it is
 *        scheduled.
 *
 *      - Call exit().  The FSM terminates.
 *
 *      - Call yield().  The FSM will be rescheduled as soon as possible.
 *
 *      - Call rest().  The FSM will go dormant: it will only be scheduled
 *        again by an explicit call to invoke() or co_fsm_run().  (This is
 *        equivalent to calling block() without calling any wait functions.)
 *
 * An FSM may also be invoked explicitly from a cooperative thread, using its
 * run() or invoke() member function.  In fact, FSMs may even invoke each other
 * this way, as long as a single FSM is not recursively invoked.
 *
 * Destroying a Co_fsm has no effect on the underlying cooperative thread.
 * The converse is not true; see the comment on Co_task above for more
 * information.
 *
 * This class is suitable for use as a class member or as a (likely protected
 * or private) base class.
 */
class Co_fsm
    : public Co_task
{
public:
    Co_fsm();
    Co_fsm(const boost::function<void()>& run, co_group& group);
    Co_fsm(co_thread*);

    /* Returns a Co_fsm that represents the running thread, which should be a
     * finite state machine. */
    static Co_fsm self();

    /* Start a new finite state machine represented by this Co_fsm, which must
     * have been created with the default constructor. */
    void start(const boost::function<void()>& run,
               co_group& group = co_group_coop);

    /* Functions that may be called externally. */
    void wake();
    void run();
    void invoke(const boost::function<void()>&);
    void kill();

    /* Functions for use only within the FSM's run function. */
    static void block();
    static void transition(const boost::function<void()>&);
    static void exit();
    static void yield();
    static void rest();
};

/* A Co_fsm that kills the underlying Fsm (if any) when it is destroyed. */
class Auto_fsm
    : public Co_fsm,
      private boost::noncopyable
{
public:
    Auto_fsm() { }
    Auto_fsm(const boost::function<void()>& run,
             co_group& group = co_group_coop)
        { start(run, group); }
    ~Auto_fsm() { kill(); }
};

/*
 * Cooperative synchronization primitives.
 *
 * Compared to native threads, there is less need for synchronization among
 * cooperative threads, because of the reduced level of potential concurrency,
 * but we provide a complete set of primitives anyhow.
 *
 * These synchronization primitives may only be used among threads within a
 * single thread group.  They may not be used by native threads.
 */

/* Scoped critical section.
 *
 * A critical section is customarily a region of code that can only be executed
 * by one thread of execution at a time.  With cooperative threads, every
 * section of code between calls to blocking functions is a critical section.
 * Thus, there's a reduced need for the synchronization primitives used to
 * define critical sections.
 *
 * It's easy, however, to accidentally insert a call to a blocking function
 * into code that must execute as a critical section.  Thus, this class, which
 * allows for assertions that the region of code that executes within the
 * critical section's scope will not block.
 *
 * Call co_might_yield(), declared in threads/impl.hh, to assert that the
 * program is not executing inside a critical section.
 */
class Co_critical_section
{
public:
    Co_critical_section() { co_enter_critical_section(); }
    ~Co_critical_section() { co_exit_critical_section(); }
};

/* Native section.
 *
 * Migrates to a native thread in the constructor, then back to the original
 * thread group in the destructor.  May also be used in a native thread (with
 * no effect).
 *
 * Some system calls do not have non-blocking version.  Any system call that
 * takes a file name, e.g. open or rename, usually falls into this category.
 * Some other system calls do have non-blocking versions, but those
 * non-blocking versions have inconvenient semantics, e.g. aio_read, aio_write.
 *
 * To make these operations in these two categories appear non-blocking
 * relative to other cooperative threads in the same thread group, the thread
 * can migrate itself to a native thread, do the operation, and then migrate
 * itself back into its original thread group.  Co_scoped_native_section
 * provides an easy way to do exactly that.
 *
 * Commonly used system calls have convenient wrappers in Co_thread that use
 * Co_scoped_native_section (or its equivalent) internally.  You might want to
 * use Co_scoped_native_section directly for dealing with other system calls,
 * or for doing more than one operation at a time natively, since each
 * migration out of and back into a thread group is relatively expensive.
 */
class Co_native_section
{
public:
    Co_native_section() { co_might_yield(); old_group = co_migrate(NULL); }
    ~Co_native_section() { co_migrate(old_group); }
private:
    co_group* old_group;
};

/* Condition variables. */
class Co_cond {
public:
    void signal();
    void broadcast();
    void block();
    void wait(int* signaled = 0);
private:
    Co_waitqueue wq;
};

/* Mutex (non-recursive). */
class Co_mutex {
public:
    Co_mutex();
    co_thread* get_owner() const;
    void lock();
    bool try_lock();
    void unlock();
    void wait(int* acquired = 0);
private:
    co_thread *owner;
    Co_waitqueue wq;
};

/* Scoped mutex (non-recursive). */
class Co_scoped_mutex {
public:
    Co_scoped_mutex(Co_mutex* m_) : m(m_) { if (m) m->lock(); }
    ~Co_scoped_mutex() { if (m) m->unlock(); }
    void unlock() { if (m) { m->unlock(); m = NULL; } }
private:
    Co_mutex* m;
};

/* Reader-writer lock.
 *
 * The read side is recursive, the write side is not. */
class Co_rwlock {
public:
    Co_rwlock();

    void read_lock();
    bool read_try_lock();
    void read_unlock();
    void read_wait(int* acquired = 0);

    void write_lock();
    bool write_try_lock();
    void write_unlock();
    void write_wait(int* acquired = 0);
private:
    co_thread *write_owner;
    size_t read_owners;
    Co_waitqueue read_wq;
    Co_waitqueue write_wq;
};

/* Counting semaphore. */
class Co_sema {
public:
    Co_sema();
    void down();
    bool try_down();
    void up();
    void wait(int* downed);
private:
    int value;
    Co_waitqueue wq;
};

/* Barrier. */
class Co_barrier {
public:
    Co_barrier(unsigned int count);
    void block();
private:
    unsigned int count;
    Co_waitqueue wq;
};

/* Completion: threads wait until the completion is released, and afterward
 * everything passes through. */
class Co_completion {
public:
    Co_completion();
    void latch();
    void release();
    bool is_complete();
    void wait(int* released);
    void block();
private:
    bool complete;
    Co_waitqueue wq;
};

/* Co_task. */

/* Constructs an empty Co_task that does not (yet) represent any thread or
 * FSM. */
inline
Co_task::Co_task()
{}

/* Constructs a Co_task that represents 'thread'. */
inline
Co_task::Co_task(co_thread* thread_)
    : Task(thread_)
{}

/* Checks the state of file descriptors and timers that some thread in this
 * thread group is waiting on, and wakes up any threads that are waiting on
 * them.  Does not block. */
inline void
Co_task::poll(void)
{
    co_poll();
}

/* Causes the next call to block() to wake up when 'fd' becomes ready for the
 * given 'type' of I/O: reading if 'type' is CO_FD_WAIT_READ, writing if 'type'
 * is CO_FD_WAIT_WRITE.
 *
 * When 'fd' becomes ready, '*revents' is set to the revents returned by poll()
 * for input or output, according to 'type', if 'revents' is non-null.  For
 * reading, these bits are POLLIN, POLLERR, and POLLHUP; for writing, POLLOUT,
 * POLLERR, and POLLHUP.  POLLNVAL will also be set if wakeup is due to a call
 * to fd_closed() or if 'fd' is closed and poll(2) notices this (a usage bug
 * that will cause an assertion failure unless assertions are disabled).
 *
 * block() must be called to actually wait for the event to occur.  No blocking
 * function may be called between calling this function and calling block(). */
inline void
Co_task::fd_wait(int fd, enum co_fd_wait_type type, int *revents)
{
    co_fd_wait(fd, type, revents);
}

/* Sets up to wait for 'fd' to become ready for reading.
 *
 * If 'revents' is non-null, then when the event occurs, '*revents' is set to
 * the revents returned by poll() for input (according to 'type'): some
 * combination of POLLIN, POLLERR, and POLLHUP.  POLLNVAL will also be set if
 * wakeup is due to a call to fd_closed() or if 'fd' is closed and poll(2)
 * notices this (a usage bug that will cause an assertion failure unless
 * assertions are disabled).
 *
 * block() must be called to actually wait for the event to occur.  No blocking
 * function may be called between calling this function and calling block(). */
inline void
Co_task::fd_read_wait(int fd, int *revents)
{
    co_fd_read_wait(fd, revents);
}

/* Sets up to wait for 'fd' to become ready for writing.
 *
 * If 'revents' is non-null, then when the event occurs, '*revents' is set to
 * the revents returned by poll() for output (according to 'type'): some
 * combination of POLLOUT, POLLERR, and POLLHUP.  POLLNVAL will also be set if
 * wakeup is due to a call to fd_closed() or if 'fd' is closed and poll(2)
 * notices this (a usage bug that will cause an assertion failure unless
 * assertions are disabled).
 *
 * block() must be called to actually wait for the event to occur.  No blocking
 * function may be called between calling this function and calling block(). */
inline void
Co_task::fd_write_wait(int fd, int *revents)
{
    co_fd_write_wait(fd, revents);
}

/* Wakes up all cooperative threads or FSMs blocked on 'fd' in this thread
 * group, typically because 'fd' has been closed (or soon will be closed).  The
 * is set to POLLIN or POLLOUT, plus POLLHUP and POLLNVAL.  (POLLNVAL can be
 * used as an indication that 'fd' was closed.)
 *
 * Doesn't do anything about threads or FSMs in any other thread group or
 * native threads that might be blocked on 'fd'.  */
inline void
Co_task::fd_closed(int fd)
{
    co_fd_closed(fd);
}

/* Causes the next call to block() to wake up when the current time reaches
 * 'abs_time'.
 *
 * block() must be called to actually wait for the event to occur.  No blocking
 * function may be called between calling this function and calling block().
 *
 * When block() returns, '*expired' is set to zero if the timer did not expire
 * or to a nonzero value if it did. */
inline void
Co_task::timer_wait(timeval abs_time, int *expired)
{
    co_timer_wait(abs_time, expired);
}

/* Sets up this cooperative thread or FSM to release 'completion' when it
 * terminates.  '*this' must be a thread or FSM in the same thread group as any
 * thread that may wait on 'completion'.  'thread' must not already have a
 * completion to release (but any number of threads may wait on a single
 * Co_completion object).
 *
 * This function should be called before '*this' has an opportunity to exit:
 * normally, by the thread's creator immediately after calling
 * co_thread_create() or co_fsm_create(). */
inline void
Co_task::join_completion(Co_completion* completion)
{
    co_join_completion(thread, completion);
}

/* Co_thread. */

/* Constructs an empty Co_thread that does not (yet) represent any thread. */
inline
Co_thread::Co_thread()
{}

/* Constructs a new thread to run 'start'.  Initially the thread is in thread
 * group 'group' (co_group_coop by default).
 *
 * If 'group' is the group of the running thread, then the new thread is added
 * to the group synchronously, before the constructor returns.  This fact can
 * be useful for reproducibility of round-robin scheduling.  Otherwise the new
 * thread becomes a member of 'group' asynchronously.
 *
 * Creating an thread does not yield to the new thread or any other thread. */
inline
Co_thread::Co_thread(const boost::function<void()>& run, co_group& group)
{
    start(run, group);
}

/* Constructs a Co_thread to represent 'thread'. */
inline
Co_thread::Co_thread(co_thread* thread)
    : Co_task(thread)
{}

/* Returns a Co_thread that represents the running thread, which should be a
 * cooperative thread. */
inline Co_thread
Co_thread::self()
{
    return Co_thread(co_self());
}

/* Starts a new thread represented by this Co_thread, which must have been
 * created with the default constructor.  The new thread calls the 'run'
 * function and runs in 'group' (by default co_group_coop). */
inline void
Co_thread::start(const boost::function<void()>& run, co_group& group)
{
    assert(empty());
    thread = co_thread_create(&group, run);
}

/* Migrates the running thread to 'group'.  If 'group' is null, then the
 * running thread becomes a native thread.  Returns the previous group (null if
 * the thread was native). */
inline co_group*
Co_thread::migrate(co_group* group)
{
    return co_migrate(group);
}

/* Blocks the calling cooperative thread until at least one of the events
 * registered by calling a wait function has occurred.  If no events have been
 * registered, the thread will block forever. */
inline void
Co_thread::block()
{
    co_block();
}

/* Let other threads in the same thread group run, if any are ready.
 *
 * This function does *not* check for file descriptor readiness or timer
 * expiration.  Call poll() first if you want to do that.
 */
inline void
Co_thread::yield()
{
    co_yield();
}

/* Blocks until 'fd' becomes ready for the given 'type' of I/O: reading if
 * 'type' is CO_FD_WAIT_READ, writing if 'type' is CO_FD_WAIT_WRITE.
 *
 * Returns the revents output by ::poll() for input or output (according to
 * 'type').  For reading, these bits are POLLIN, POLLERR, and POLLHUP; for
 * writing, POLLOUT, POLLERR, and POLLHUP.  POLLNVAL will also be set if wakeup
 * is due to a call to fd_closed() or if 'fd' is closed and ::poll() notices
 * this (a usage bug that will cause an assertion failure unless assertions are
 * disabled). */
inline int
Co_thread::fd_block(int fd, enum co_fd_wait_type type)
{
    return co_fd_block(fd, type);
}

/* Blocks until 'fd' becomes ready for reading.
 *
 * Returns the revents output by ::poll() for input: some combination of
 * POLLIN, POLLERR, and POLLHUP.  POLLNVAL will also be set if wakeup is due to
 * a call to fd_closed() or if 'fd' is closed and ::poll() notices this (a
 * usage bug that will cause an assertion failure unless assertions are
 * disabled). */
inline int
Co_thread::fd_read_block(int fd)
{
    return co_fd_read_block(fd);
}

/* Blocks until 'fd' becomes ready for writing.
 *
 * Returns the revents output by poll() for output: some combination of
 * POLLOUT, POLLERR, and POLLHUP.  POLLNVAL will also be set if wakeup is due
 * to a call to co_fd_closed() or if 'fd' is closed and poll() notices this (a
 * usage bug that will cause an assertion failure unless assertions are
 * disabled). */
inline int
Co_thread::fd_write_block(int fd)
{
    return co_fd_write_block(fd);
}

/* Blocks for at least the period given in 'duration'. */
inline void
Co_thread::sleep(timeval duration)
{
    co_sleep(duration);
}

/* Creates a new socket with the given 'namespace', 'style', and 'protocol'.
 * Returns a nonnegative file descriptor for a socket in non-blocking mode if
 * successful, otherwise a negative errno value.
 *
 * Does not block. */
inline int
Co_thread::socket(int name_space, int style, int protocol)
{
    return co_socket(name_space, style, protocol);
}

/* Calls 'read(fd, data, nbytes)', blocking if necessary.
 * Returns the number of bytes read if successful, otherwise a negative errno
 * value.
 *
 * This function is suitable for reading from a file descriptor that usefully
 * supports non-blocking mode, such as a socket or pipe.  It will work with a
 * regular file, but it will not achieve the desired purpose of avoiding
 * blocking other cooperative threads in the thread group, because Unix systems
 * always block for I/O on regular files.  Use co_async_read() instead for
 * reading from regular files. */
inline ssize_t
Co_thread::read(int fd, void *data, size_t size)
{
    return co_read(fd, data, size);
}

/* Calls 'recv(fd, data, nbytes, flags)', blocking if necessary.
 * Returns the number of bytes read if successful, otherwise a negative errno
 * value. */
inline ssize_t
Co_thread::recv(int fd, void *data, size_t size, int flags)
{
    return co_recv(fd, data, size, flags);
}

/* Calls 'recvfrom(fd, data, nbytes, flags, sockaddr, socklenp)', blocking if
 * necessary.
 *
 * Returns the number of bytes read if successful, otherwise a negative errno
 * value. */
inline ssize_t
Co_thread::recvfrom(int fd, void *data, size_t size, int flags,
                 sockaddr *sa, socklen_t *sa_len)
{
    return co_recvfrom(fd, data, size, flags, sa, sa_len);
}

/* Calls 'recvmsg(fd, msg, flags)', blocking if necessary.
 * Returns the number of bytes read if successful, otherwise a negative errno
 * value. */
inline ssize_t
Co_thread::recvmsg(int fd, msghdr *msg, int flags)
{
    return co_recvmsg(fd, msg, flags);
}

/* Calls 'write(fd, data, nbytes)', blocking if necessary.
 * Returns the number of bytes written if successful, otherwise a negative
 * errno value.
 *
 * This function is suitable for writing to a file descriptor that usefully
 * supports non-blocking mode, such as a socket or pipe.  It will work with a
 * regular file, but it will not achieve the desired purpose of avoiding
 * blocking other cooperative threads in the thread group, because Unix systems
 * always block for I/O on regular files.  Use co_async_write() instead for
 * writing to regular files. */
inline ssize_t
Co_thread::write(int fd, const void *data, size_t size)
{
    return co_write(fd, data, size);
}

/* Calls 'send(fd, data, nbytes, flags)', blocking if necessary.
 * Returns the number of bytes written if successful, otherwise a negative
 * errno value. */
inline ssize_t
Co_thread::send(int fd, const void *data, size_t size, int flags)
{
    return co_send(fd, data, size, flags);
}

/* Calls 'sendto(fd, data, nbytes, flags, sockaddr, socklen)', blocking if
 * necessary.
 *
 * Returns the number of bytes written if successful, otherwise a negative
 * errno value. */
inline ssize_t
Co_thread::sendto(int fd, const void *data, size_t size, int flags,
               sockaddr *sa, socklen_t sa_len)
{
    return co_sendto(fd, data, size, flags, sa, sa_len);
}

/* Calls 'sendmsg(fd, msg, flags)', blocking if necessary.
 * Returns the number of bytes written if successful, otherwise a negative
 * errno value. */
inline ssize_t
Co_thread::sendmsg(int fd, const msghdr *msg, int flags)
{
    return co_sendmsg(fd, msg, flags);
}

/* Calls 'connect(fd, sockaddr, socklen)' and blocks, if necessary, until the
 * connection is complete or an error occurs.
 * Returns 0 if successful, otherwise a positive errno value. */
inline int
Co_thread::connect(int fd, sockaddr *sa, socklen_t sa_len)
{
    return co_connect(fd, sa, sa_len);
}

/* Calls 'accept(fd, sockaddr, socklen)' and blocks, if necessary, until a
 * connection is accepted or an error occurs.
 * Returns a nonnegative file descriptor for a socket in non-blocking mode if
 * successful, otherwise a negative errno value. */
inline int
Co_thread::accept(int fd, sockaddr *sa, socklen_t *sa_len)
{
    return co_accept(fd, sa, sa_len);
}

/* Calls 'open(name, flags, mode)' from a native thread, which avoids blocking
 * other cooperative threads in the thread group.
 * Returns a nonnegative file descriptor if successful, otherwise a negative
 * errno value.
 *
 * The returned file descriptor is not changed to non-blocking mode.
 * Generally, doing so would have no useful effect, because system calls on
 * regular files always wait for I/O to complete. */
inline int
Co_thread::async_open(const char *name, int flags, mode_t mode)
{
    return co_async_open(name, flags, mode);
}

/* Calls 'read(fd, data, size)' from a native thread, which avoids blocking
 * other cooperative threads in the thread group.
 * Returns the number of bytes read if successful, otherwise a negative errno
 * value.
 *
 * 'fd' should be a file descriptor in blocking mode.  This function will not
 * loop on EAGAIN.
 *
 * This function is best suited for reading from a regular file.  Although it
 * can read from any kind of file descriptor, reading from sockets, pipes,
 * etc. using co_read() entails less overhead. */
inline ssize_t
Co_thread::async_read(int fd, void *data, size_t size)
{
    return co_async_read(fd, data, size);
}

/* Calls 'pread(fd, data, size, offset)' from a native thread, which avoids
 * blocking other cooperative threads in the thread group.
 * Returns the number of bytes read if successful, otherwise a negative errno
 * value. */
inline ssize_t
Co_thread::async_pread(int fd, void *data, size_t size, off_t offset)
{
    return co_async_pread(fd, data, size, offset);
}

/* Calls 'write(fd, data, size)' from a native thread, which avoids blocking
 * other cooperative threads in the thread group.
 * Returns the number of bytes written if successful, otherwise a negative
 * errno value.
 *
 * 'fd' should be a file descriptor in blocking mode.  This function will not
 * loop on EAGAIN.
 *
 * This function is best suited for writing to a regular file.  Although it can
 * writing to any kind of file descriptor, writing to sockets, pipes,
 * etc. using co_write() entails less overhead. */
inline ssize_t
Co_thread::async_write(int fd, const void *data, size_t size)
{
    return co_async_write(fd, data, size);
}

/* Calls 'pwrite(fd, data, size, offset)' from a native thread, which avoids
 * blocking other cooperative threads in the thread group.
 * Returns the number of bytes written if successful, otherwise a negative
 * errno value. */
inline ssize_t
Co_thread::async_pwrite(int fd, const void *data, size_t size, off_t offset)
{
    return co_async_pwrite(fd, data, size, offset);
}

/* Calls 'ftruncate(fd, offset)' from a native thread, which avoids blocking
 * other cooperative threads in the thread group.
 * Returns 0 if successful, otherwise a positive errno value. */
inline int
Co_thread::async_ftruncate(int fd, off_t offset)
{
    return co_async_ftruncate(fd, offset);
}

/* Calls 'fchmod(fd, mode)' from a native thread, which avoids blocking other
 * cooperative threads in the thread group.
 * Returns 0 if successful, otherwise a positive errno value. */
inline int
Co_thread::async_fchmod(int fd, mode_t mode)
{
    return co_async_fchmod(fd, mode);
}

/* Calls 'fchown(fd, user, group)' from a native thread, which avoids blocking
 * other cooperative threads in the thread group.
 * Returns 0 if successful, otherwise a positive errno value. */
inline int
Co_thread::async_fchown(int fd, uid_t user, gid_t group)
{
    return co_async_fchown(fd, user, group);
}

/* Calls 'fsync(fd)' from a native thread, which avoids blocking other
 * cooperative threads in the thread group.
 * Returns 0 if successful, otherwise a positive errno value. */
inline int
Co_thread::async_fsync(int fd)
{
    return co_async_fsync(fd);
}

/* Calls 'fdatasync(fd)' from a native thread, which avoids blocking other
 * cooperative threads in the thread group.
 * Returns 0 if successful, otherwise a positive errno value. */
inline int
Co_thread::async_fdatasync(int fd)
{
    return co_async_fdatasync(fd);
}

/* Co_fsm. */

/* Constructs an empty Co_thread that does not (yet) represent any thread. */
inline
Co_fsm::Co_fsm()
    : Co_task()
{}

/* Constructs a new finite state machine in thread group 'group', which must
 * not be null.  (FSMs may not migrate among thread groups.)
 *
 * The FSM is scheduled like a cooperative thread and for most purposes it may
 * be treated in the same way as a cooperative thread.  However, FSMs are
 * stackless: they may not block.  Instead, the FSM's 'start' function is
 * invoked anew each time it is scheduled, and it must call one of the
 * functions mentioned in the large comment on Co_fsm before it returns.
 *
 * Creating an FSM does not yield to the new FSM or to any other thread. */
inline
Co_fsm::Co_fsm(const boost::function<void()>& run, co_group& group)
{
    start(run, group);
}

/* Constructs a Co_thread to represent 'fsm'. */
inline
Co_fsm::Co_fsm(co_thread* fsm_)
    : Co_task(fsm_)
{}

/* Return a Co_fsm that represents the running thread, which should be a finite
 * state machine. */
inline Co_fsm
Co_fsm::self()
{
    return Co_fsm(co_self());
}

/* Starts a new FSM represented by this Co_fsm, which must have been created
 * with the default constructor.  The new FSM calls the 'run' function and runs
 * in 'group' (by default co_group_coop). */
inline void
Co_fsm::start(const boost::function<void()>& run, co_group& group)
{
    assert(empty());
    thread = co_fsm_create(&group, run);
}

/* Wakes up this FSM, causing it to be scheduled to run asynchronously.
 * '*this' may be blocking or it may already be scheduled to be run, but it
 * must not currently be running.
 *
 * '*this' must be a finite state machine in the running thread group.  The
 * caller may be a cooperative thread or a finite state machine.  */
inline void
Co_fsm::wake()
{
    co_fsm_wake(thread);
}

/* Run this FSM synchronously.
 *
 * '*this' must be a finite state machine in the running thread group.  The
 * caller may be a cooperative thread or a finite state machine.  Any level of
 * nesting of finite state machine execution is permitted (even if it is not
 * advisable), except that recursively re-invoking an FSM when it is already
 * running yields undefined behavior.  (It will trigger an assertion failure if
 * NDEBUG is not defined.) */
inline void
Co_fsm::run()
{
    co_fsm_run(thread);
}

/* Run this FSM synchronously, but first change the FSM's function to 'run'.
 *
 * '*this' must be a finite state machine in the running thread group.  The
 * caller may be a cooperative thread or a finite state machine.  Any level of
 * nesting of finite state machine execution is permitted (even if it is not
 * advisable), except that recursively re-invoking an FSM when it is already
 * running yields undefined behavior.  (It will trigger an assertion failure if
 * NDEBUG is not defined.) */
inline void
Co_fsm::invoke(const boost::function<void()>& run)
{
    co_fsm_invoke(thread, run);
}

/* Kill '*this', which must be a finite state machine in the running thread
 * group that is not currently running.  The caller may be a cooperative thread
 * or a finite state machine. */
inline void
Co_fsm::kill()
{
    co_fsm_kill(thread);
}

/* Called within a finite state machine's 'run' function, this causes the
 * running FSM to be invoked again after one or more of the registered events
 * has occurred.  (If no events have been registered, then the FSM will block
 * forever, or until the next call to Co_fsm::invoke() or Co_fsm::run() naming
 * '*this'.) */
inline void
Co_fsm::block()
{
    co_fsm_block();
}

/* Called within a finite state machine's 'run' function, this causes the FSM
 * to be invoked as the FSM's 'run' function the next time that it is
 * scheduled. */
inline void
Co_fsm::transition(const boost::function<void()>& run)
{
    co_fsm_transition(run);
}

/* Called within a finite state machine's 'run' function, this causes the FSM
 * to terminate (as soon as it returns). */
inline void
Co_fsm::exit(void)
{
    co_fsm_exit();
}

/* Called within a finite state machine's 'run' function, causes the FSM to be
 * re-invoked with the same function and argument the next time it is
 * scheduled.
 *
 * Calling this function to wait for an event to happen wastes CPU time.  Use
 * co_fsm_block() instead. */
inline void
Co_fsm::yield(void)
{
    co_fsm_yield();
}

/* Called within a finite state machine, this causes the FSM to go dormant: it
 * will only be scheduled again by an explicit call to its invoke() or run()
 * function. */
inline void
Co_fsm::rest(void)
{
    co_fsm_rest();
}

} // namespace vigil

#endif /* threads/cooperative.hh */
