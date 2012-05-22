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
 * Low-level thread support.
 *
 * Most code should use the higher-level interfaces in threads/cooperative.hh
 * or threads/native.hh.
 */

#ifndef THREADS_IMPL_HH
#define THREADS_IMPL_HH 1

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <sys/socket.h>
#include <sys/types.h>

struct msghdr;

namespace vigil {

struct co_event;
struct co_group;
struct co_thread;
class Co_completion;

/* One-time initialization.
 *
 * Must be called before any other function in this header. */
void co_init(void);

/* Threads.
 *
 * You should know what a thread is.
 *
 * Both native and cooperative threads may be created.
 */
co_thread* co_thread_create(co_group *, const boost::function<void()>&);
void co_thread_assimilate(void);

/* Finite state machines (FSMs).
 *
 * An FSM is scheduled like a cooperative thread and for most purposes it may
 * be treated in the same way as a cooperative thread.  However, FSMs may not
 * block.  To wait for events, they may call co_fsm_block() and return, to be
 * called again later when the event has occurred.
 *
 * An FSM may also be invoked explicitly from a cooperative thread, using
 * co_fsm_run() or co_fsm_invoke().  In fact, FSMs may even invoke each other
 * this way, as long as a single FSM is not recursively invoked.
 */
co_thread* co_fsm_create(co_group *, const boost::function<void()>&);
void co_fsm_block(void);
void co_fsm_transition(const boost::function<void()>&);
void co_fsm_exit(void);
void co_fsm_yield(void);
void co_fsm_rest(void);
void co_fsm_wake(co_thread *);
void co_fsm_run(co_thread *);
void co_fsm_invoke(co_thread *, const boost::function<void()>&);
void co_fsm_kill(co_thread *);

/* Thread groups.
 *
 *      - Threads within a single group, called "cooperative threads", are
 *        scheduled cooperatively with respect to each other, but not with
 *        respect to threads in other groups.
 *
 *      - Threads not in any group, called "native threads", are scheduled
 *        concurrently.
 *
 * Threads may migrate into and out of and across groups (see co_migrate()).
 *
 * co_group_coop is an initial thread group for general-purpose use.
 */
extern co_group co_group_coop;
void co_group_create(co_group **);
void co_group_destroy(co_group *);

/* Critical sections.
 *
 * A critical section is customarily a region of code that can only be executed
 * by one thread of execution at a time.  With cooperative threads, every
 * section of code between calls to blocking functions is a critical section.
 * Thus, there's a reduced need for the synchronization primitives used to
 * define critical sections.
 *
 * It's easy, however, to accidentally insert a call to a blocking function
 * into code that must execute as a critical section.  Thus, these functions,
 * which allow for assertions that a region of code will not block.  Call
 * co_enter_critical_section() to increase the "critical section count",
 * co_exit_critical_section() to decrease it, and co_might_yield() to assert
 * that the count is 0.
 */
#ifndef NDEBUG
void co_enter_critical_section(void);
void co_exit_critical_section(void);
void co_might_yield(void);
void co_might_yield_if(bool block);
#else
#define co_enter_critical_section() do { } while (0)
#define co_exit_critical_section() do { } while (0)
#define co_might_yield() do { } while (0)
#define co_might_yield_if(BLOCK) do { } while (0)
#endif

/* Running thread and its thread group. */
co_thread *co_self(void);
co_group *co_group_self(void);

/* Allowing other threads to run. */
void co_yield(void);
void co_poll(void);

/* Changing to another thread group. */
co_group *co_migrate(co_group *);

/* Waiting for threads to die. 
 *
 * There is no 'join' function, but:
 *
 *   - A native thread can wait for another native thread to complete by
 *     downing a Native_sema passing into the joined thread's initial function.
 *
 *   - A native thread can wait for a cooperative thread or FSM to complete by
 *     migrating into the thread or FSM's group, then using
 *     co_join_completion(), and waiting on the completion.
 *
 *   - A cooperative thread or FSM can wait for another cooperative thread or
 *     FSM by using co_join_completion() and waiting on the completion.
 *
 *   - A cooperative thread can wait for a native thread to complete by
 *     migrating itself out of its thread group (becoming a native thread) and
 *     downing a Native_sema passing into the joined thread's initial function.
 *
 * - An FSM cannot (easily) wait for a native thread to complete.
 *
 * Don't use pthread_join() to wait for native threads: there is a race
 * between obtaining the thread's pthread_t and the destruction of the
 * co_thread following the thread's death.
 */
void co_join_completion(co_thread *, Co_completion*);

/* Hooking blocking calls. */
typedef boost::function<void()> co_preblock_hook;
co_preblock_hook co_get_preblock_hook(void);
void co_set_preblock_hook(const co_preblock_hook&);
void co_unset_preblock_hook();

/* Blocking.
 *
 * Before blocking, call one or more "wait functions" that register an event to
 * wait one.  By convention, wait function names end in "_wait".  co_block()
 * will return when at least one of the registered events has occurred.  (If
 * co_block() is called without any events being registered, the thread will
 * block forever.)
 *
 * Calling a wait function enters a critical section.  co_block() in turn exits
 * one critical section for each per registered event.  Thus, between calling a
 * wait function and calling co_block(), no blocking functions may be called.
 */
void co_block(void);

/* File descriptor waiters. */
enum co_fd_wait_type {
    CO_FD_WAIT_READ,
    CO_FD_WAIT_WRITE,
    CO_N_FD_WAIT_TYPES
};
int co_fd_block(int fd, enum co_fd_wait_type);
int co_fd_read_block(int fd);
int co_fd_write_block(int fd);
void co_fd_wait(int fd, enum co_fd_wait_type, int *revents);
void co_fd_read_wait(int fd, int *revents);
void co_fd_write_wait(int fd, int *revents);
void co_fd_closed(int fd);

/* Timers. */
void co_timer_wait(timeval abs_time, int *expired);
void co_sleep(timeval duration);

void co_immediate_wake(int retval, int *retvalp);

/* Waitqueue.
 *
 * A waitqueue is a low-level data structure used as a building block for
 * synchronization primitives and other forms of events.  If you are not
 * building your own synchronization primitives, you do not need to understand
 * them.
 *
 * A waitqueue is simply a singly linked list of events used as a FIFO queue.
 * Each event contains a linked list node, so no dynamic allocation is needed.
 *
 * The implementation of waitqueues is too clever: it avoids initializing the
 * 'next' field of the new event to NULL in co_wq_add_event() or the 'head'
 * field of co_waitqueue to NULL in co_wq_init() by making the *next* call to
 * co_wq_wake_all() do it.  Anything that traverses a waitqueue looking for a
 * null pointer must thus terminate the list with a null pointer by setting
 * *wq->tail to NULL. */
class Co_waitqueue
    : boost::noncopyable
{
public:
    Co_waitqueue();
    bool empty() const;
    void wake_all(int retval);
    size_t wake_n(int reval, size_t max_wake);
    co_thread* wake_1(int retval);
    void wait(int* retvalp);
    void dequeue(co_event*);
    void requeue(co_event*);
private:
    co_event *head;             /* First event; not necessarily initialized. */
    co_event **tail;            /* Address of 'next' field of last event, or in
                                 * empty waitqueue address of 'head'.  */
};

/*
 * Simple system call wrappers for use by cooperative threads.  These are
 * pretty cheap, especially if no blocking is required.  They require that file
 * descriptors be in non-blocking mode.  (They will set non-blocking mode
 * automatically in any new file descriptors that they return.)
 */
int co_socket(int name_space, int style, int protocol);
ssize_t co_read(int fd, void *, size_t);
ssize_t co_recv(int fd, void *, size_t, int flags);
ssize_t co_recvfrom(int fd, void *, size_t, int flags,
                    sockaddr *, socklen_t *);
ssize_t co_recvmsg(int fd, msghdr *, int flags);
ssize_t co_write(int fd, const void *, size_t);
ssize_t co_send(int fd, const void *, size_t, int flags);
ssize_t co_sendto(int fd, const void *, size_t, int flags,
                  sockaddr *, socklen_t);
ssize_t co_sendmsg(int fd, const msghdr *, int flags);
int co_connect(int fd, sockaddr *, socklen_t);
int co_accept(int fd, sockaddr *, socklen_t *);

/*
 * More system call wrappers for use by cooperative threads.
 *
 * These are relatively expensive, even if no blocking is required, because
 * they are implemented by migrating the thread away from its group, and then
 * back to it, to avoid blocking other cooperative threads.
 *
 * To do more than one of these operations consecutively, consider using
 * Co_scoped_native_section directly, to avoid doing migration many times.
 */
int co_async_open(const char *, int flags, mode_t);
ssize_t co_async_read(int fd, void *, size_t);
ssize_t co_async_pread(int fd, void *, size_t, off_t);
ssize_t co_async_write(int fd, const void *, size_t);
ssize_t co_async_pwrite(int fd, const void *, size_t, off_t);
int co_async_ftruncate(int fd, off_t);
int co_async_fchmod(int fd, mode_t);
int co_async_fchown(int fd, uid_t, gid_t);
int co_async_fsync(int fd);
int co_async_fdatasync(int fd);
int co_async_rename(const char* old_path, const char* new_path);

} // namespace vigil

#endif /* threads/impl.hh */
