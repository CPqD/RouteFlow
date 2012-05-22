/* Copyright 2008,2009 (C) Nicira, Inc.
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
#include <config.h>
#include "threads/impl.hh"
#include <assert.h>
#include <boost/foreach.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <errno.h>
#include <fcntl.h>
#include <list>
#include <poll.h>
#include <pthread.h>
#include <queue>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include "threads/cooperative.hh"
#include "timeval.hh"
#include "fault.hh"
#include "ppoll.hh"
#include "vlog.hh"

#define UNUSED __attribute__((__unused__))
#define NOT_REACHED() abort()

namespace vigil {

static Vlog_module lg("threads");

struct co_event {
    Co_waitqueue *wq;           /* Waitqueue that contains the event. */
    struct co_event *next;      /* Next in waitqueue. */
    struct co_thread *thread;   /* Thread to wake. */
    int *retvalp;               /* If non-null, *retvalp set to 0 initially,
                                   to nonzero upon wake. */
};

struct Co_fd_waiter {
    int pollfd_idx;
    Co_waitqueue wq[CO_N_FD_WAIT_TYPES];
    Co_fd_waiter() : pollfd_idx(-1) { }
};

struct Co_timer {
    struct timeval when;
    Co_waitqueue* wq;
};
bool operator<(const Co_timer& a, const Co_timer& b) {
    return b.when < a.when;
}

struct co_group {
    pthread_mutex_t mutex;

    struct co_thread *polling;
    std::list<co_thread*> ready_list, block_list;
    size_t n_threads;

    boost::ptr_vector<Co_fd_waiter> fd_waiters;
    std::vector<pollfd> pollfds;

    std::priority_queue<Co_timer> timers;

    bool fsm_thread;

    co_group();
};

enum co_thread_flags {
    COTF_FSM = 1 << 2,          /* Thread is an FSM? */
    COTF_READY = 1 << 3,        /* Thread is on group's ready_list. */
    COTF_BLOCKING = 1 << 4,     /* Thread is on group's block_list. */
    COTF_NEED_SCHEDULE = 1 << 5, /* Thread awakened to be group scheduler. */
    COTF_PREJOINED = 1 << 6,    /* Thread is already in its initial group. */
#ifndef NDEBUG
    COTF_FSM_ACTION = 1 << 16,  /* FSM called an action function already. */
    COTF_FSM_RUNNING = 1 << 17  /* FSM is currently running. */
#endif
};

/* A thread or finite state machine (FSM). */
struct co_thread
{
    std::list<co_thread*>::iterator iter;
    co_group *group;
    unsigned int flags;
    boost::function<void()> run;

    /* group != NULL only. */
    pthread_t pthread;
    Co_completion* completion;
    sem_t sched_sem;
#ifndef NDEBUG
    int critical_section;
#endif
    co_preblock_hook preblock_hook;
    std::vector<co_event> events;

    co_thread(co_group *group_, const boost::function<void()>& run_);
    ~co_thread();
};

/* Standard groups. */
struct co_group co_group_coop;

/* Portable implementation of an interruptible poll operation. */
static Ppoll* ppoll;

static void *thread_main(void *);
static void dont_call_pthread_exit_directly(void *UNUSED);
static void fsm_thread();
static void fsm_action(void);
static void lock_groups(struct co_group *, struct co_group *);
static void join_coop_group(struct co_group *);
static void leave_coop_group(struct co_group *);
static co_event* add_event(void);
static void schedule();
static void reschedule_while_needed();
static void do_schedule();
static void process_poll_results(int n_events);
static void remove_pollfd(Co_fd_waiter *);
static void do_event_wake(struct co_event *, int retval);
static void cancel_events(struct co_thread *);
static void dequeue_event(struct co_event *);
static void requeue_event(struct co_event *);
static int wakeup_timers(struct timespec *, struct timespec **);
static int set_nonblocking(int fd);
static int mk_nonblocking(int fd);
static bool remove_from_lists(struct co_thread *, unsigned int flags);
static void run_fsm(struct co_thread *);
static void make_ready(struct co_thread *);
static void make_blocking(struct co_thread *);
static void wait_sem(sem_t *);
static void init_self();
static co_thread* get_self();
static void set_self(co_thread*);

/* Initialize the cooperative thread scheduler.  This function should be called
 * before any other function in the cooperative scheduling package.  It must be
 * called exactly once.
 *
 * A co_thread is created for the currently running thread.  Initially it is
 * not in any thread group (i.e. it is a native thread), but it can be
 * migrated into a thread group with co_migrate().
 *
 * There is no corresponding function to disable the cooperative threading
 * package. */
void
co_init(void)
{
    ppoll = new Ppoll(SIGUSR2);
    init_self();

    /* Ensure that operations on disconnected sockets produce EPIPE, not
     * SIGPIPE. */
    signal(SIGPIPE, SIG_IGN);

    /* Don't ignore SIGCHLD here, to allow graceful use of fork. */
}

/* Creates a new thread to run 'start'.  Initially the thread is in thread
 * group 'group', which may be null to make it a native thread (but the new
 * thread can use co_migrate() to change thread groups).
 *
 * If 'group' is the group of the running thread, then the new thread is added
 * to the group synchronously, before co_thread_create() returns.  This fact
 * can be useful for reproducibility of round-robin scheduling.  Otherwise the
 * new thread becomes a member of 'group' asynchronously.
 *
 * Returns a pointer to the new thread.
 *
 * Creating an thread does not yield to the new thread or any other thread. */
co_thread*
co_thread_create(struct co_group *group, const boost::function<void()>& start)
{
    struct co_thread *thread;
    pthread_attr_t attr;

    thread = new co_thread(group, start);
    if (group == co_group_self()) {
        /* This is essentially the same as calling co_migrate() within the new
         * thread, but it never has to use pthread_kill() to wake up the group
         * scheduler, which is at least a small win. */
        thread->flags |= COTF_PREJOINED;
        pthread_mutex_lock(&group->mutex);
        ++group->n_threads;
        make_ready(thread);
        pthread_mutex_unlock(&group->mutex);
    }

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
#ifndef NDEBUG
    pthread_attr_setstacksize(&attr, 1024 * 256);
#endif
    pthread_create(&thread->pthread, &attr, thread_main, thread);
    pthread_attr_destroy(&attr);

    return thread;
}

/* Creates a cooperative thread structure for the running thread, which must
 * not be a thread created by co_thread_create().  After calling this function,
 * the running thread can be migrated into and out of thread groups and
 * otherwise treated like any other cooperative thread. */
void
co_thread_assimilate(void)
{
    assert(!get_self());
    create_signal_stack();
    co_thread* thread = new co_thread(NULL, boost::function<void()>());
    set_self(thread);
    thread->pthread = pthread_self();
    ppoll->block();
}

/* Creates a new finite state machine in thread group 'group', which must not
 * be null.  (FSMs may not migrate among thread groups.)
 *
 * The FSM is scheduled like a cooperative thread and for most purposes it may
 * be treated in the same way as a cooperative thread.  However, FSMs are
 * stackless: they may not block.  Instead, the FSM's 'start' function is
 * invoked anew each time it is scheduled.  Before the start function returns,
 * it must do exactly one of the following:
 *
 *      - Call one or more wait functions (e.g. co_fd_wait(), Co_sema::wait()),
 *        then co_fsm_block().  The FSM will be invoked again after one or more
 *        of the events occurs.
 *
 *      - Call co_fsm_transition() with a replacement 'start'.  The FSM will be
 *        invoked again with the new function and argument the next time it is
 *        scheduled.
 *
 *      - Call co_fsm_exit().  The FSM terminates.
 *
 *      - Call co_fsm_yield().  The FSM will be rescheduled as soon as
 *        possible.
 *
 *      - Call co_fsm_rest().  The FSM will go dormant: it will only be
 *        scheduled again by an explicit call to co_fsm_invoke() or
 *        co_fsm_run().  (This is equivalent to calling co_fsm_block() without
 *        calling any wait functions.)
 *
 * Returns a pointer to the new thread.
 *
 * Creating an FSM does not yield to the new FSM or to any other thread. */
co_thread*
co_fsm_create(struct co_group *group, const boost::function<void()>& start)
{
    struct co_group *old;
    struct co_thread *thread;

    assert(group);
    old = co_migrate(group);
    thread = new co_thread(group, start);

    thread->flags |= COTF_FSM;
    make_ready(thread);
    if (!group->fsm_thread) {
        group->fsm_thread = true;
        co_thread_create(group, fsm_thread);
    }
    co_migrate(old);

    return thread;
}

/* Called within a finite state machine, this causes the FSM to be invoked
 * again after one or more of the registered events has occurred.  (If no
 * events have been registered, then the FSM will block forever, or until the
 * next call to co_fsm_invoke() or co_fsm_run() naming 'fsm'.) */
void
co_fsm_block(void)
{
    struct co_thread *thread = co_self();
    fsm_action();
    if (!(thread->flags & COTF_READY)) {
#ifndef NDEBUG
        /* One critical section for each event, one for running in an FSM. */
        assert(thread->critical_section >= thread->events.size() + 1);
        thread->critical_section -= thread->events.size();
#endif
        make_blocking(thread);
    }
}

/* Called within a finite state machine, this causes the FSM to be invoked as
 * the 'start' function the next time that it is scheduled. */
void
co_fsm_transition(const boost::function<void()>& start)
{
    struct co_thread *thread = co_self();
    fsm_action();
    assert(!start.empty());
    thread->run = start;
}

/* Called within a finite state machine, this causes the FSM to terminate (as
 * soon as it returns). */
void
co_fsm_exit(void)
{
    struct co_thread *thread = co_self();
    fsm_action();
    thread->run = NULL;
}

/* Called within a finite state machine, causes the FSM to be re-invoked with
 * the same function and argument the next time it is scheduled.
 *
 * Calling this function to wait for an event to happen wastes CPU time.  Use
 * co_fsm_block() instead. */
void
co_fsm_yield(void)
{
    struct co_thread *thread = co_self();
    fsm_action();
    make_ready(thread);
}

/* Called within a finite state machine, this causes the FSM to go dormant: it
 * will only be scheduled again by an explicit call to co_fsm_invoke() or
 * co_fsm_run(). */
void
co_fsm_rest(void)
{
    assert(co_self()->events.empty());
    co_fsm_block();
}

/* Wakes up 'fsm', causing it to be scheduled to run asynchronously.  'fsm' may
 * be blocking or it may already be scheduled to be run, but it must not
 * currently be running.
 *
 * 'fsm' must be a finite state machine in the running thread group.  The
 * caller may be a cooperative thread or a finite state machine.  */
void
co_fsm_wake(co_thread *fsm)
{
    assert(fsm->group);
    assert(fsm->flags & COTF_FSM);
    assert(co_group_self() == fsm->group);
    assert(!(fsm->flags & COTF_FSM_RUNNING));
    if (!(fsm->flags & COTF_READY)) {
        remove_from_lists(fsm, COTF_BLOCKING);
        make_ready(fsm);
    }
}

/* Run 'fsm' synchronously.
 *
 * 'fsm' must be a finite state machine in the running thread group.  The
 * caller may be a cooperative thread or a finite state machine.  Any level of
 * nesting of finite state machine execution is permitted (even if it is not
 * advisable), except that recursively re-invoking an FSM when it is already
 * running yields undefined behavior.  (It will trigger an assertion failure if
 * NDEBUG is not defined.) */
void
co_fsm_run(struct co_thread *fsm)
{
    remove_from_lists(fsm, COTF_READY | COTF_BLOCKING);
    run_fsm(fsm);
}

/* Run 'fsm' synchronously, but first change the FSM's function to 'run'.
 *
 * 'fsm' must be a finite state machine in the running thread group.  The
 * caller may be a cooperative thread or a finite state machine.  Any level of
 * nesting of finite state machine execution is permitted (even if it is not
 * advisable), except that recursively re-invoking an FSM when it is already
 * running yields undefined behavior.  (It will trigger an assertion failure if
 * NDEBUG is not defined.) */
void
co_fsm_invoke(struct co_thread *fsm, const boost::function<void()>& run)
{
    fsm->run = run;
    co_fsm_run(fsm);
}

/* Kill 'fsm', which must be a finite state machine in the running thread group
 * that is not currently running.  The caller may be a cooperative thread or a
 * finite state machine. */
void
co_fsm_kill(struct co_thread *fsm)
{
    assert(fsm->group);
    assert(fsm->flags & COTF_FSM);
    assert(co_group_self() == fsm->group);
    assert(!(fsm->flags & COTF_FSM_RUNNING));

    cancel_events(fsm);
    remove_from_lists(fsm, COTF_READY | COTF_BLOCKING);

    if (fsm->completion) {
        fsm->completion->release();
    }
    delete fsm; 
}

/* Creates a new thread group and stores a pointer to it in '*groupp'.  Threads
 * created within or migrated to the new group will be scheduled cooperatively
 * with respect to each other. */
void
co_group_create(struct co_group **groupp)
{
    *groupp = new co_group;
}

/* Destroys thread group 'group'.  There must not be any remaining threads in
 * 'group'.
 *
 * It's probably better not to destroy groups at all. */
void
co_group_destroy(struct co_group *group)
{
    delete group;
}

#ifndef NDEBUG
/* Increment the critical section count.  When the critical section count is
 * nonzero, blocking is not allowed. */
void
co_enter_critical_section(void)
{
    co_self()->critical_section++;
}

/* Decrement the critical section count.  When the critical section count is
 * nonzero, blocking is not allowed. */
void
co_exit_critical_section(void)
{
    assert(co_self()->critical_section > 0);
    co_self()->critical_section--;
}

/* Asserts that the critical section count is zero. */
void
co_might_yield(void)
{
    assert(!co_self()->critical_section);
}

/* Asserts that the critical section count is zero or that 'block' is false. */
void
co_might_yield_if(bool block)
{
    assert(!block || !co_self()->critical_section);
}
#endif

/* Returns the running thread.  This is null if the running thread was not
 * created by co_thread_create() or assimilated with co_thread_assimilate(). */
struct co_thread *
co_self(void)
{
    return get_self();
}

/* Returns the running thread's thread group.  The return value is null if the
 * running thread is not in any thread group, i.e. it is a native thread.
 *
 * This function may be called only in a thread created by co_thread_create()
 * or assimilated with co_thread_assimilate(). */
struct co_group *
co_group_self(void)
{
    return co_self()->group;
}

/* Let other threads in the same thread group run, if any are ready.
 *
 * This function does *not* check for file descriptor readiness or timer
 * expiration.  Call co_poll() first if you want to do that.
 */
void
co_yield(void)
{
    struct co_thread *thread = co_self();
    struct co_group *group = thread->group;

    co_might_yield();
    if (group && !group->ready_list.empty()) {
        if(!(thread->flags & COTF_READY)) { 
          make_ready(thread);
        } 
        schedule();
    }
}

/* Checks the state of file descriptors and timers that some thread is waiting
 * on, and wakes up any threads that are waiting on them.  Does not block. */
void
co_poll(void)
{
    struct co_thread *thread = co_self();
    struct co_group *group = thread->group;
    struct timespec timeout, *timeoutp;
    size_t n_events;

    if (group) {
        wakeup_timers(&timeout, &timeoutp);
        if (!group->pollfds.empty()) {
            n_events = poll(&group->pollfds[0], group->pollfds.size(), 0);
            process_poll_results(n_events);
        }
    }
    do_gettimeofday(true);
}

/* Migrates the running thread to the specified 'new' thread group.  If 'new'
 * is null, then the running thread becomes a native thread.  Returns the
 * previous group (null if the thread was native).
 *
 * Finite state machines may not be migrated. */
struct co_group *
co_migrate(struct co_group *new_)
{
    struct co_thread *thread = co_self();
    struct co_group *old = thread->group;

    if (old != new_) {
        assert(!(thread->flags & COTF_FSM));
        lock_groups(old, new_);
        thread->group = new_;
        if (old) {
            old->n_threads--;
            leave_coop_group(old);
            pthread_mutex_unlock(&old->mutex);
        }
        if (new_) {
            new_->n_threads++;
            join_coop_group(new_);
        }
    }
    return old;
}

/* Sets up 'thread' to release 'completion' when it terminates.  'thread' may
 * be a thread or FSM in the same thread group as any thread that may wait on
 * 'completion'.  'thread' must not already have a completion to release (but
 * any number of threads may wait on a single Co_completion object).
 *
 * This function should be called before 'thread' has an opportunity to exit:
 * normally, by the thread's creator immediately after calling
 * co_thread_create() or co_fsm_create(). */
void
co_join_completion(struct co_thread *thread, Co_completion* completion)
{
    assert(!thread->completion);
    thread->completion = completion;
}

/* Returns the pre-block hook function for the current thread, or a null
 * pointer if no pre-block hook function is set.
 *
 * The pre-block hook function is called just before the running thread blocks,
 * at which point it is reset to null.  The pre-block hook function may do
 * anything desired, except that it may not itself block. */
co_preblock_hook
co_get_preblock_hook(void)
{
    return co_self()->preblock_hook;
}

/* Sets 'hook' as the pre-block hook function for the current thread.  It may
 * be an empty boost::function to disable the pre-block hook.
 *
 * The pre-block hook function is called just before the running thread blocks,
 * at which point it is reset to null.  The pre-block hook function may do
 * anything desired, except that it may not itself block. */
void
co_set_preblock_hook(const co_preblock_hook& hook)
{
    co_self()->preblock_hook = hook;
}

/* Clears the pre-block hook. */
void
co_unset_preblock_hook()
{
    co_set_preblock_hook(co_preblock_hook());
}

/* Blocks the calling thread, which must be in a thread group, until at least
 * one of the events registered by calling a wait function has occurred.  If
 * no events have been registered, the thread will block forever. */
void
co_block(void)
{
    struct co_thread *thread = co_self();

#ifndef NDEBUG
    assert(thread->critical_section >= thread->events.size());
    thread->critical_section -= thread->events.size();
    co_might_yield();
#endif
    if (!remove_from_lists(thread, COTF_READY)) {
        make_blocking(thread);
        schedule();
    } else {
        co_poll();
        co_yield();
    }
    cancel_events(thread);

    /* We're definitely not blocking.
     * We're running, which is different from ready. */
    assert(!(thread->flags & (COTF_BLOCKING | COTF_READY)));
}

/* Blocks until 'fd' becomes ready for the given 'type' of I/O: reading if
 * 'type' is CO_FD_WAIT_READ, writing if 'type' is CO_FD_WAIT_WRITE.
 *
 * Returns the revents output by poll() for input or output (according to
 * 'type').  For reading, these bits are POLLIN, POLLERR, and POLLHUP; for
 * writing, POLLOUT, POLLERR, and POLLHUP.  POLLNVAL will also be set if wakeup
 * is due to a call to co_fd_closed() or if 'fd' is closed and poll() notices
 * this (a usage bug that will cause an assertion failure unless assertions are
 * disabled). */
int
co_fd_block(int fd, enum co_fd_wait_type type)
{
    int revents;
    co_might_yield();
    co_fd_wait(fd, type, &revents);
    co_block();
    assert(revents);
    return revents;
}

/* Blocks until 'fd' becomes ready for reading.
 *
 * Returns the revents output by poll() for input: some combination of POLLIN,
 * POLLERR, and POLLHUP.  POLLNVAL will also be set if wakeup is due to a call
 * to co_fd_closed() or if 'fd' is closed and poll() notices this (a usage bug
 * that will cause an assertion failure unless assertions are disabled). */
int
co_fd_read_block(int fd)
{
    return co_fd_block(fd, CO_FD_WAIT_READ);
}


/* Blocks until 'fd' becomes ready for writing.
 *
 * Returns the revents output by poll() for output: some combination of
 * POLLOUT, POLLERR, and POLLHUP.  POLLNVAL will also be set if wakeup is due
 * to a call to co_fd_closed() or if 'fd' is closed and poll() notices this (a
 * usage bug that will cause an assertion failure unless assertions are
 * disabled). */
int
co_fd_write_block(int fd)
{
    return co_fd_block(fd, CO_FD_WAIT_WRITE);
}

/* Causes the next call to co_block() or co_fsm_block() to wake up when 'fd'
 * becomes ready for the given 'type' of I/O: reading if 'type' is
 * CO_FD_WAIT_READ, writing if 'type' is CO_FD_WAIT_WRITE.
 *
 * When 'fd' becomes ready, '*revents' is set to the revents returned by poll()
 * for input or output, according to 'type', if 'revents' is non-null.  For
 * reading, these bits are POLLIN, POLLERR, and POLLHUP; for writing, POLLOUT,
 * POLLERR, and POLLHUP.  POLLNVAL will also be set if wakeup is due to a call
 * to co_fd_closed() or if 'fd' is closed and poll() notices this (a usage bug
 * that will cause an assertion failure unless assertions are disabled).
 *
 * co_block() must be called to actually wait for the event to occur.  No
 * blocking function may be called between calling this function and calling
 * co_block(). */
void
co_fd_wait(int fd, enum co_fd_wait_type type, int *revents)
{
    struct co_group *group = co_group_self();

    if (fd >= group->fd_waiters.size()) {
        group->fd_waiters.reserve(fd + 1);
        while (fd >= group->fd_waiters.size()) {
            group->fd_waiters.push_back(new Co_fd_waiter);
        }
    }

    Co_fd_waiter* fdw = &group->fd_waiters[fd];
    struct pollfd *pfd;
    if (fdw->pollfd_idx >= 0) {
        pfd = &group->pollfds[fdw->pollfd_idx];
    } else {
        fdw->pollfd_idx = group->pollfds.size();
        group->pollfds.push_back(pollfd());
        pfd = &group->pollfds.back();
        pfd->fd = fd;
    }

    if (type == CO_FD_WAIT_READ) {
        pfd->events |= POLLIN;
    } else if (type == CO_FD_WAIT_WRITE) {
        pfd->events |= POLLOUT;
    } else {
        NOT_REACHED();
    }
    fdw->wq[type].wait(revents);
}

/* Sets up to wait for 'fd' to become ready for reading.
 *
 * If 'revents' is non-null, then when the event occurs, '*revents' is set to
 * the revents returned by poll() for input (according to 'type'): some
 * combination of POLLIN, POLLERR, and POLLHUP.  POLLNVAL will also be set if
 * wakeup is due to a call to co_fd_closed() or if 'fd' is closed and poll()
 * notices this (a usage bug that will cause an assertion failure unless
 * assertions are disabled).
 *
 * co_block() must be called to actually wait for the event to occur.  No
 * blocking function may be called between calling this function and calling
 * co_block(). */
void
co_fd_read_wait(int fd, int *revents)
{
    co_fd_wait(fd, CO_FD_WAIT_READ, revents);
}

/* Sets up to wait for 'fd' to become ready for writing.
 *
 * If 'revents' is non-null, then when the event occurs, '*revents' is set to
 * the revents returned by poll() for output (according to 'type'): some
 * combination of POLLOUT, POLLERR, and POLLHUP.  POLLNVAL will also be set if
 * wakeup is due to a call to co_fd_closed() or if 'fd' is closed and poll()
 * notices this (a usage bug that will cause an assertion failure unless
 * assertions are disabled).
 *
 * co_block() must be called to actually wait for the event to occur.  No
 * blocking function may be called between calling this function and calling
 * co_block(). */
void
co_fd_write_wait(int fd, int *revents)
{
    co_fd_wait(fd, CO_FD_WAIT_WRITE, revents);
}

/* Wakes up all threads blocked on 'fd' in this thread group, typically because
 * 'fd' has been closed (or soon will be closed).  The is set to POLLIN or
 * POLLOUT, plus POLLHUP and POLLNVAL.  (POLLNVAL can be used as an indication
 * that 'fd' was closed.)
 *
 * Doesn't do anything about threads or FSMs in any other thread group or
 * native threads that might be blocked on 'fd'.  */
void
co_fd_closed(int fd)
{
    struct co_group *group = co_group_self();
    if (fd < 0 || fd >= group->fd_waiters.size()) {
        return;
    }

    Co_fd_waiter* fdw = &group->fd_waiters[fd];
    if (fdw->pollfd_idx < 0) {
        return;
    }

    fdw->wq[CO_FD_WAIT_READ].wake_all(POLLHUP | POLLNVAL | POLLIN);
    fdw->wq[CO_FD_WAIT_WRITE].wake_all(POLLHUP | POLLNVAL | POLLOUT);
    remove_pollfd(fdw);
}

/* Causes the next call to co_block() or co_fsm_block() to wake up when the
 * current time reaches 'abs_time'.
 *
 * co_block() must be called to actually wait for the event to occur.  No
 * blocking function may be called between calling this function and calling
 * co_block().
 *
 * When co_block() returns, '*expired' is set to zero if the timer did not
 * expire or to a nonzero value if it did. */
void
co_timer_wait(struct timeval abs_time, int *expired)
{
    struct co_group *group = co_group_self();

    Co_timer timer;
    timer.when = abs_time;
    timer.wq = new Co_waitqueue;
    timer.wq->wait(expired);
    group->timers.push(timer);
}

/* Blocks for at least the period given in 'duration'. */
void
co_sleep(struct timeval duration)
{
    co_might_yield();
    co_timer_wait(do_gettimeofday(false) + duration, NULL);
    co_block();
}

/* Set up the running thread to wake up immediately, without blocking, when
 * co_block() is called.  (This is useful when a caller wants to wait for some
 * event that has already occurred.)
 *
 * co_block() must be called to actually wait for the event to occur.  No
 * blocking function may be called between calling this function and calling
 * co_block().
 *
 * If 'retvalp' is non-null, then '*retvalp' is set to 'retval', which must be
 * nonzero. */
void
co_immediate_wake(int retval, int *retvalp)
{
    struct co_thread *thread = co_self();
    co_enter_critical_section();

    /* Note the event is added only to maintain the critical_section
       counter and n_events synchronized. */
    co_event* event = add_event();
    event->wq = NULL;
    event->thread = thread;
    event->retvalp = retvalp;

    assert(retval);
    if (retvalp) {
        *retvalp = retval;
    }

    if (!(thread->flags & COTF_READY)) {
        make_ready(thread);
    }
}

/* Initializes 'this' as an empty waitqueue.
 *
 * (wq->head is intentionally not initialized; see the big comment on
 * waitqueues in coop.h.) */
Co_waitqueue::Co_waitqueue()
{
    tail = &head;
}

/* Returns true if 'this' contains no events, false otherwise. */
bool
Co_waitqueue::empty() const
{
    return tail == &head;
}

/* Wakes all of the threads waiting for an event on 'this'.  If a nonnull
 * 'retvalp' was passed into co_wq_wait() when each event was registered, then
 * each '*retvalp' is set to 'retval', which must be nonzero.
 *
 * All of the awakened threads must be in the current thread group. */
void
Co_waitqueue::wake_all(int retval)
{
    struct co_event *e;
    assert(retval);
    *tail = NULL;
    for (e = head; e != NULL; e = e->next) {
        do_event_wake(e, retval);
    }

    /* Reinitialize 'this' as an empty waitqueue. */
    tail = &head;
}

/* Dequeues the first 'max_wake' events in 'this', or fewer if 'this' does not
 * contain that many events, and wakes the associated threads.  Returns the
 * number of events removed from 'this'.
 *
 * If a nonnull 'retvalp' was passed into co_wq_wait() when each event was
 * registered, then '*retvalp' for each awakened event is set to 'retval',
 * which must be nonzero.
 *
 * Wakes threads in FIFO order.
 *
 * All of the awakened threads must be in the current thread group. */
size_t
Co_waitqueue::wake_n(int retval, size_t max_wake)
{
    size_t n;
    struct co_event *e;

    assert(retval);
    *tail = NULL;
    for (e = head, n = 0; e != NULL && n < max_wake; e = e->next, n++) {
        do_event_wake(e, retval);
    }
    if (e) {
        head = e;
    } else {
        tail = &head;
    }

    return n;
}

/* Dequeues the first event in 'this' and wakes the associated thread.  If an
 * event was dequeued, returns the thread that was awakened; otherwise, in the
 * case where 'this' was empty, returns a null pointer.
 *
 * If a nonnull 'retvalp' was passed into co_wq_wait() when the event was
 * registered, then '*retvalp' is set to 'retval', which must be nonzero.
 *
 * Wakes threads in FIFO order.
 *
 * The awakened thread must be in the current thread group. */
struct co_thread *
Co_waitqueue::wake_1(int retval)
{
    struct co_event *e;

    assert(retval);
    *tail = NULL;
    e = head;
    if (e) {
        struct co_event *next = e->next;
        do_event_wake(e, retval);
        if (next) {
            head = next;
        } else {
            tail = &head;
        }
        return e->thread;
    } else {
        return NULL;
    }
}

/* Appends a new to the list of events waiting on 'this'.  If 'retvalp' is
 * nonnull, then '*retvalp' is set to 0 immediately and it will be set to a
 * nonzero value if and when the event is awakened.
 *
 * Also increments the critical section count.  The thread must not block
 * between adding the event and passing it to co_block(). */
void
Co_waitqueue::wait(int *retvalp)
{
    struct co_thread *thread = co_self();

    co_enter_critical_section();
    co_event* event = add_event();
    event->wq = this;
    event->thread = thread;
    event->retvalp = retvalp;
    if (retvalp) {
        *retvalp = 0;
    }
    *tail = event;
    tail = &event->next;
}

void
Co_waitqueue::dequeue(co_event* event)
{
    assert(event->wq == this);

    struct co_event **prev;
    struct co_event *e2;
    *tail = NULL;
    for (prev = &head; (e2 = *prev) != event; prev = &e2->next) {
        assert(e2 != NULL);
    }
    if (event->next) {
        *prev = event->next;
    } else {
        tail = prev;
    }
}

void
Co_waitqueue::requeue(co_event* event)
{
    assert(event->wq == this);
    *tail = event;
    tail = &event->next;
}

/* Creates a new socket with the given 'namespace', 'style', and 'protocol'.
 * Returns a nonnegative file descriptor for a socket in non-blocking mode if
 * successful, otherwise a negative errno value.
 *
 * Does not block. */
int
co_socket(int namespace_, int style, int protocol)
{
    int fd = socket(namespace_, style, protocol);
    return fd >= 0 ? mk_nonblocking(fd) : -errno;
}

#define CO_LOOP_EAGAIN(CALL, WAIT_TYPE)             \
    co_might_yield();                               \
    for (;;) {                                      \
        ssize_t retval = (CALL);                    \
        if (retval >= 0) {                          \
            return retval;                          \
        } else if (errno == EAGAIN) {               \
            co_fd_block(fd, WAIT_TYPE);             \
        } else if (errno == EINTR) {                \
            /* Nothing to do. */                    \
        } else {                                    \
            return -errno;                          \
        }                                           \
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
ssize_t
co_read(int fd, void *data, size_t nbytes)
{
    CO_LOOP_EAGAIN(read(fd, data, nbytes), CO_FD_WAIT_READ);
}

/* Calls 'recv(fd, data, nbytes, flags)', blocking if necessary.
 * Returns the number of bytes read if successful, otherwise a negative errno
 * value. */
ssize_t
co_recv(int fd, void *data, size_t nbytes, int flags)
{
    CO_LOOP_EAGAIN(recv(fd, data, nbytes, flags), CO_FD_WAIT_READ);
}

/* Calls 'recvfrom(fd, data, nbytes, flags, sockaddr, socklenp)', blocking if
 * necessary.
 *
 * Returns the number of bytes read if successful, otherwise a negative errno
 * value. */
ssize_t
co_recvfrom(int fd, void *data, size_t nbytes, int flags,
            struct sockaddr *sockaddr, socklen_t *socklenp)
{
    CO_LOOP_EAGAIN(recvfrom(fd, data, nbytes, flags, sockaddr, socklenp),
                   CO_FD_WAIT_READ);
}

/* Calls 'recvmsg(fd, msg, flags)', blocking if necessary.
 * Returns the number of bytes read if successful, otherwise a negative errno
 * value. */
ssize_t
co_recvmsg(int fd, struct msghdr *msg, int flags)
{
    CO_LOOP_EAGAIN(recvmsg(fd, msg, flags), CO_FD_WAIT_READ);
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
ssize_t
co_write(int fd, const void *data, size_t nbytes)
{
    CO_LOOP_EAGAIN(write(fd, data, nbytes), CO_FD_WAIT_WRITE);
}

/* Calls 'send(fd, data, nbytes, flags)', blocking if necessary.
 * Returns the number of bytes written if successful, otherwise a negative
 * errno value. */
ssize_t
co_send(int fd, const void *data, size_t nbytes, int flags)
{
    CO_LOOP_EAGAIN(send(fd, data, nbytes, flags), CO_FD_WAIT_WRITE);
}

/* Calls 'sendto(fd, data, nbytes, flags, sockaddr, socklen)', blocking if
 * necessary.
 *
 * Returns the number of bytes written if successful, otherwise a negative
 * errno value. */
ssize_t
co_sendto(int fd, const void *data, size_t nbytes, int flags,
          struct sockaddr *sockaddr, socklen_t socklen)
{
    CO_LOOP_EAGAIN(sendto(fd, data, nbytes, flags, sockaddr, socklen),
                   CO_FD_WAIT_WRITE);
}

/* Calls 'sendmsg(fd, msg, flags)', blocking if necessary.
 * Returns the number of bytes written if successful, otherwise a negative
 * errno value. */
ssize_t
co_sendmsg(int fd, const struct msghdr *msg, int flags)
{
    CO_LOOP_EAGAIN(sendmsg(fd, msg, flags), CO_FD_WAIT_WRITE);
}

/* Calls 'connect(fd, sockaddr, socklen)' and blocks, if necessary, until the
 * connection is complete or an error occurs.
 * Returns 0 if successful, otherwise a positive errno value. */
int
co_connect(int fd, struct sockaddr *sockaddr, socklen_t socklen)
{
    int retval;

    co_might_yield();
    retval = connect(fd, sockaddr, socklen);
    if (retval >= 0) {
        return 0;
    } else if (errno == EINPROGRESS || errno == EINTR) {
        int error;
        socklen_t error_len = sizeof error;

        co_fd_write_block(fd);

        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &error_len) < 0) {
            NOT_REACHED();
        }
        return error;
    } else {
        return errno;
    }
}

/* Calls 'accept(fd, sockaddr, socklen)' and blocks, if necessary, until a
 * connection is accepted or an error occurs.
 * Returns a nonnegative file descriptor for a socket in non-blocking mode if
 * successful, otherwise a negative errno value. */
int
co_accept(int fd, struct sockaddr *sockaddr, socklen_t *socklen)
{
    co_might_yield();
    for (;;) {
        int retval = accept(fd, sockaddr, socklen);
        if (retval >= 0) {
            return mk_nonblocking(retval);
        } else if (errno == EAGAIN) {
            co_fd_read_block(fd);
        } else if (errno == EINTR) {
            /* Nothing to do. */
        } else {
            return -errno;
        }
    }
}

/* Calls 'open(name, flags, mode)' from a native thread, which avoids blocking
 * other cooperative threads in the thread group.
 * Returns a nonnegative file descriptor if successful, otherwise a negative
 * errno value.
 *
 * The returned file descriptor is not changed to non-blocking mode.
 * Generally, doing so would have no useful effect, because system calls on
 * regular files always wait for I/O to complete. */
int
co_async_open(const char *name, int flags, mode_t mode)
{
    Co_native_section as_native;
    int retval = open(name, flags, mode);
    return retval < 0 ? -errno : retval;
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
ssize_t
co_async_read(int fd, void *data, size_t size)
{
    Co_native_section as_native;
    ssize_t retval = read(fd, data, size);
    return retval < 0 ? -errno : retval;
}

/* Calls 'pread(fd, data, size, offset)' from a native thread, which avoids
 * blocking other cooperative threads in the thread group.
 * Returns the number of bytes read if successful, otherwise a negative errno
 * value. */
ssize_t
co_async_pread(int fd, void *data, size_t size, off_t offset)
{
    Co_native_section as_native;
    ssize_t retval = pread(fd, data, size, offset);
    return retval < 0 ? -errno : retval;
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
ssize_t
co_async_write(int fd, const void *data, size_t size)
{
    Co_native_section as_native;
    ssize_t retval = write(fd, data, size);
    return retval < 0 ? -errno : retval;
}

/* Calls 'pwrite(fd, data, size, offset)' from a native thread, which avoids
 * blocking other cooperative threads in the thread group.
 * Returns the number of bytes written if successful, otherwise a negative
 * errno value. */
ssize_t
co_async_pwrite(int fd, const void *data, size_t size, off_t offset)
{
    Co_native_section as_native;
    ssize_t retval = pwrite(fd, data, size, offset);
    return retval < 0 ? -errno : retval;
}

/* Calls 'ftruncate(fd, offset)' from a native thread, which avoids blocking
 * other cooperative threads in the thread group.
 * Returns 0 if successful, otherwise a positive errno value. */
int
co_async_ftruncate(int fd, off_t offset)
{
    Co_native_section as_native;
    int retval = ftruncate(fd, offset);
    return retval < 0 ? errno : 0;
}

/* Calls 'fchmod(fd, mode)' from a native thread, which avoids blocking other
 * cooperative threads in the thread group.
 * Returns 0 if successful, otherwise a positive errno value. */
int
co_async_fchmod(int fd, mode_t mode)
{
    Co_native_section as_native;
    int retval = fchmod(fd, mode);
    return retval < 0 ? errno : 0;
}

/* Calls 'fchown(fd, user, group)' from a native thread, which avoids blocking
 * other cooperative threads in the thread group.
 * Returns 0 if successful, otherwise a positive errno value. */
int
co_async_fchown(int fd, uid_t user, gid_t group)
{
    Co_native_section as_native;
    int retval = fchown(fd, user, group);
    return retval < 0 ? errno : 0;
}

/* Calls 'fsync(fd)' from a native thread, which avoids blocking other
 * cooperative threads in the thread group.
 * Returns 0 if successful, otherwise a positive errno value. */
int
co_async_fsync(int fd)
{
    Co_native_section as_native;
    int retval = fsync(fd);
    return retval < 0 ? errno : 0;
}

/* Calls 'fdatasync(fd)' from a native thread, which avoids blocking other
 * cooperative threads in the thread group.
 * Returns 0 if successful, otherwise a positive errno value. */
int
co_async_fdatasync(int fd)
{
    Co_native_section as_native;
#ifdef HAVE_FDATASYNC
    int retval = fdatasync(fd);
#else
    int retval = fsync(fd);
#endif
    return retval < 0 ? errno : 0;
}

/* Calls 'rename(oldpath, newpath)' from a native thread, which avoids blocking other
 * cooperative threads in the thread group.
 * Returns 0 if successful, otherwise a positive errno value. */
int
co_async_rename(const char* old_path, const char* new_path) {
    Co_native_section as_native;
    int retval = rename(old_path, new_path);
    return retval < 0 ? errno : 0;
}


co_thread::co_thread(co_group *group_, const boost::function<void()>& run_)
    : group(group_),
      flags(0),
      run(run_),
      completion(NULL)
#ifndef NDEBUG
    , critical_section(0)
#endif
{
    sem_init(&sched_sem, 0, 0);
}

co_thread::~co_thread()
{
    if (!(flags & COTF_FSM)) {
        sem_destroy(&sched_sem);
    }
}

static void *
thread_main(void *thread_)
{
    struct co_thread *thread = static_cast<co_thread*>(thread_);
    struct co_group *group;

    set_self(thread);

    create_signal_stack();
#ifndef NDEBUG
    pthread_cleanup_push(dont_call_pthread_exit_directly, NULL);
#endif

    if (thread->flags & COTF_PREJOINED) {
        wait_sem(&thread->sched_sem);
        reschedule_while_needed();
    } else {
        ppoll->block();
        group = thread->group;
        if (group) {
            thread->group = NULL;
            co_migrate(group);
        }
    }

    do_gettimeofday(true);
    thread->run();
    if (thread->completion) {
        thread->completion->release();
    }
    co_migrate(NULL);
    delete thread;

#ifndef NDEBUG
    pthread_cleanup_pop(0);
#endif
    free_signal_stack();

    return NULL;
}

static void
dont_call_pthread_exit_directly(void *unused UNUSED)
{
    NOT_REACHED();
}

static void
fsm_thread()
{
    co_block();
    NOT_REACHED();
}

static void
fsm_action(void)
{
#ifndef NDEBUG
    struct co_thread *thread = co_self();
    assert(thread->flags & COTF_FSM);
    assert(thread->flags & COTF_FSM_RUNNING);
    assert(!(thread->flags & COTF_FSM_ACTION));
    thread->flags |= COTF_FSM_ACTION;
#endif
}

co_group::co_group()
{
    pthread_mutex_init(&mutex, NULL);

    polling = NULL;
    n_threads = 0;

    fsm_thread = false;
}

static void
lock_groups(struct co_group *a, struct co_group *b)
{
    assert(a != b);
    if (a) {
        if (b) {
            if (a < b) {
                pthread_mutex_lock(&a->mutex);
                pthread_mutex_lock(&b->mutex);
            } else {
                pthread_mutex_lock(&b->mutex);
                pthread_mutex_lock(&a->mutex);
            }
        } else {
            pthread_mutex_lock(&a->mutex);
        }
    } else if (b) {
        pthread_mutex_lock(&b->mutex);
    }
}

static void
join_coop_group(struct co_group *group)
{
    struct co_thread *thread = co_self();
    co_might_yield();
    if (group->n_threads == 1) {
        /* We're the only thread in 'group'.  Start running right away. */
        assert(group->ready_list.empty());
        pthread_mutex_unlock(&group->mutex);
    } else {
        /* Wait to be scheduled.*/
        make_ready(thread);
        if (group->polling) {
            /* Wake up the scheduler if it's sitting in poll(). */
            ppoll->interrupt(group->polling->pthread);
        }
        pthread_mutex_unlock(&group->mutex);
        wait_sem(&thread->sched_sem);
        reschedule_while_needed();
        do_gettimeofday(true);
    }
}

static bool
wake_thread_on_list(const std::list<co_thread*>& list)
{
    BOOST_FOREACH (co_thread* thread, list) {
        if (!(thread->flags & COTF_FSM)) {
            assert(thread != co_self());
            thread->flags |= COTF_NEED_SCHEDULE;
            sem_post(&thread->sched_sem);
            return true;
        }
    }
    return false;
}

static void
leave_coop_group(struct co_group *group)
{
    struct co_thread *thread = co_self();
    remove_from_lists(thread, COTF_READY);
    if (!wake_thread_on_list(group->ready_list)) {
        wake_thread_on_list(group->block_list);
    }
}

static void
do_event_wake(struct co_event *event, int retval)
{
    struct co_thread *thread = event->thread;
    assert(retval);
    event->wq = NULL;
    if (event->retvalp) {
        *event->retvalp = retval;
    }
    assert(thread->group == co_group_self());
    remove_from_lists(thread, COTF_BLOCKING);
    if (!(thread->flags & COTF_READY)) {
        make_ready(thread);
    }
}

static void
cancel_events(struct co_thread *thread)
{
    BOOST_FOREACH (co_event& event, thread->events) {
        dequeue_event(&event);
    }
    thread->events.clear();
}

static void
dequeue_event(struct co_event *event)
{
    /* This function is O(n) in the number of events in 'event''s waitqueue.
     * This is not efficient, but should be rarely noticeable, because most of
     * the time waitqueues are short and this function is rarely called.  If
     * this function does show up on a profile we can switch to a doubly linked
     * list. */
    Co_waitqueue *wq = event->wq;
    if (wq) {
        wq->dequeue(event);
    }
}

static void
requeue_event(struct co_event *event)
{
    Co_waitqueue *wq = event->wq;
    if (wq) {
        wq->requeue(event);
    }
}

static co_event*
add_event()
{
    struct co_thread *thread = co_self();
    if (thread->events.size() >= thread->events.capacity()) {
        BOOST_FOREACH (co_event& event, thread->events) {
            dequeue_event(&event);
        }

        size_t capacity = thread->events.capacity();
        thread->events.reserve(capacity ? 2 * capacity : 1);

        BOOST_FOREACH (co_event& event, thread->events) {
            requeue_event(&event);
        }
    }
    thread->events.push_back(co_event());
    return &thread->events.back();
}

static void
schedule()
{
    do_schedule();
    reschedule_while_needed();
}

static void
reschedule_while_needed()
{
    struct co_thread *thread = co_self();

    while (thread->flags & COTF_NEED_SCHEDULE) {
        thread->flags &= ~COTF_NEED_SCHEDULE;
        do_schedule();
    }
}

static void
do_schedule()
{
    struct co_thread *thread = co_self();
    struct co_group *group = thread->group;
    struct co_thread *next;

    if (!thread->preblock_hook.empty()) {
        co_enter_critical_section();
        thread->preblock_hook();
        co_exit_critical_section();

        co_unset_preblock_hook();
    }

    for (;;) {
        co_might_yield();

        pthread_mutex_lock(&group->mutex);
        while (group->ready_list.empty()) {
            struct timespec timeout, *timeoutp;
            int n_events;

            if (wakeup_timers(&timeout, &timeoutp)) {
                size_t n_pollfds = group->pollfds.size();
                pollfd* pollfds = n_pollfds ? &group->pollfds[0] : NULL;
                n_events = poll(pollfds, n_pollfds, 0);
            } else {
                group->polling = thread;
                pthread_mutex_unlock(&group->mutex);

                /* We use a signal to wake up, thus no loop on EINTR here. */
                n_events = ppoll->poll(group->pollfds, timeoutp);
                if (n_events == 0) {
                    wakeup_timers(&timeout, &timeoutp);
                }

                /* Handle events by waking up threads. */
                pthread_mutex_lock(&group->mutex);
                group->polling = NULL;
            }
            process_poll_results(n_events);
        }

        /* Mark the next thread as running. */
        next = group->ready_list.front();
        group->ready_list.pop_front();
        next->flags &= ~COTF_READY;
        if (next == thread) {
            pthread_mutex_unlock(&group->mutex);
            break;
        }

        if (!(next->flags & COTF_FSM)) {
            /* Start the next thread running. */
            sem_post(&next->sched_sem);
            pthread_mutex_unlock(&group->mutex);

            /* Wait until we're scheduled again. */
            wait_sem(&thread->sched_sem);
            break;
        } else {
            pthread_mutex_unlock(&group->mutex);
            co_fsm_run(next);
        }
    }
    do_gettimeofday(true);
}

static void
run_fsm(struct co_thread *fsm)
{
    struct co_thread *thread = co_self();
    assert(fsm->group);
    assert(fsm->flags & COTF_FSM);
    assert(co_group_self() == fsm->group);

#ifndef NDEBUG
    assert(!(fsm->flags & COTF_FSM_RUNNING));
    fsm->flags |= COTF_FSM_RUNNING;
    fsm->flags &= ~COTF_FSM_ACTION;
#endif

    cancel_events(fsm);

    do_gettimeofday(true);

    set_self(fsm);
    co_enter_critical_section();
    int save_errno = errno;
    errno = 0;
    fsm->run();
    errno = save_errno;
    co_exit_critical_section();
    set_self(thread);

#ifndef NDEBUG
    fsm->flags &= ~COTF_FSM_RUNNING;
#endif

    assert(fsm->flags & COTF_FSM_ACTION);
    if (fsm->run == NULL) {
        assert(!(fsm->flags & (COTF_READY | COTF_BLOCKING)));
        assert(fsm->events.empty());
        if (fsm->completion) {
            fsm->completion->release();
        }
        delete fsm;
    } else {
        assert(fsm->flags & (COTF_READY | COTF_BLOCKING));
    }
}

static void
process_poll_results(int n_events)
{
    struct co_group *group = co_group_self();
    size_t i;

    for (i = 0; n_events > 0 && i < group->pollfds.size(); ) {
        struct pollfd *pfd = &group->pollfds[i];

        if (pfd->revents) {
            const unsigned int err_mask = POLLERR | POLLHUP | POLLNVAL;
            const unsigned int in_mask = POLLIN | err_mask;
            const unsigned int out_mask = POLLOUT | err_mask;

            if (pfd->revents & POLLNVAL) {
                // XXX Periodically we get an invalid fd from twisted.
                // Its unclear whether it is expected for the reactor to
                // handle this correctly or this in an error condition.
                // For now, print an erro and ignore.
                lg.dbg("invalid fd in poll loop: %d", pfd->fd);
                // NOT_REACHED();
            }
            n_events--;

            Co_fd_waiter* fdw = &group->fd_waiters[pfd->fd];
            if (pfd->revents & in_mask) {
                fdw->wq[CO_FD_WAIT_READ].wake_all(pfd->revents & in_mask);
                pfd->events &= ~POLLIN;
            }
            if (pfd->revents & out_mask) {
                fdw->wq[CO_FD_WAIT_WRITE].wake_all(pfd->revents & out_mask);
                pfd->events &= ~POLLOUT;
            }

            if (!pfd->events) {
                remove_pollfd(fdw);
                continue;   /* Don't increment 'i'. */
            }
        }
        i++;
    }
}

static void
remove_pollfd(Co_fd_waiter *fdw)
{
    struct co_group *group = co_group_self();
    size_t i = fdw->pollfd_idx;
    struct pollfd *pfd = &group->pollfds[i];
    struct pollfd *back = &group->pollfds.back();
    if (pfd != back) {
        *pfd = *back;
        group->fd_waiters[pfd->fd].pollfd_idx = i;
    }
    group->pollfds.pop_back();
    fdw->pollfd_idx = -1;
}

/* Wakes up all the timers that have expired and returns the number of expired
 * timers.  Stores in '*timeoutp' a timeout value to pass to ppoll(); if this
 * is non-null, then '*timeout' is used for storage. */
static int
wakeup_timers(struct timespec *timeout, struct timespec **timeoutp)
{
    struct co_group *group = co_group_self();
    int n_woke = 0;
    if (!group->timers.empty()) {
        struct timeval now = do_gettimeofday(true);
        struct timeval when;
        do {
            Co_timer& top = const_cast<Co_timer&>(group->timers.top());
            when = top.when;
            if (now < when) {
                break;
            }
            top.wq->wake_all(1);
            delete top.wq;
            group->timers.pop();
            n_woke++;
        } while (!group->timers.empty());
        if (!n_woke) {
            /* Add a 1-ms "fudge factor" to the timeout; otherwise, we tend to
             * wake up shortly before the timeout expires.  Not sure why. */
            struct timeval d = when - now + make_timeval(0, 1000);
            timeout->tv_sec = d.tv_sec;
            timeout->tv_nsec = d.tv_usec * 1000;
        } else {
            timeout->tv_sec = 0;
            timeout->tv_nsec = 0;
        }
        *timeoutp = timeout;
        return n_woke;
    } else {
        *timeoutp = NULL;
        return 0;
    }
}

/* Sets 'fd' to non-blocking mode.  Returns 0 if successful, otherwise a
 * positive errno value. */
static int
set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags != -1) {
        return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1 ? 0 : errno;
    } else {
        return errno;
    }
}

/* Sets 'fd' to non-blocking mode.  Returns 'fd' if successful, otherwise
 * closes 'fd' and returns a negative errno value. */
static int
mk_nonblocking(int fd)
{
    int error = set_nonblocking(fd);
    if (!error) {
        return fd;
    } else {
        close(fd);
        return error;
    }
}

static bool
remove_from_lists(struct co_thread *thread, unsigned int flags)
{
    assert(!(flags & ~(COTF_READY | COTF_BLOCKING)));
    if (thread->flags & flags) {
        if (thread->flags & COTF_READY) {
            thread->group->ready_list.erase(thread->iter);
        } else {
            thread->group->block_list.erase(thread->iter);
        }
        thread->flags &= ~flags;
        return true;
    } else {
        return false;
    }
}

static void
make_ready(struct co_thread *thread)
{
    struct co_group *group = co_group_self();
    assert(!(thread->flags & (COTF_BLOCKING | COTF_READY)));
    thread->iter = group->ready_list.insert(group->ready_list.end(), thread);
    thread->flags |= COTF_READY;
}

static void
make_blocking(struct co_thread *thread)
{
    struct co_group *group = co_group_self();

    assert(!(thread->flags & (COTF_BLOCKING | COTF_READY)));
    thread->iter = group->block_list.insert(group->block_list.end(), thread);
    thread->flags |= COTF_BLOCKING;
}

static void
wait_sem(sem_t *sem)
{
    while (sem_wait(sem) < 0) {
        if (errno != EINTR) {
            lg.warn("sem_wait() failed: %s", strerror(errno));
        }
    }
}

#if HAVE_TLS
static __thread co_thread* self;

static void init_self()
{
}

static co_thread*
get_self()
{
    return self;
}

static void
set_self(co_thread* self_)
{
    self = self_;
}
#else /* !HAVE_TLS */
static pthread_key_t self_key;

static void init_self()
{
    if (int error = pthread_key_create(&self_key, 0)) {
        ::fprintf(stderr, "pthread_key_create");
        ::fprintf(stderr, " (%s)", strerror(error));
        ::exit(EXIT_FAILURE);
    }
}

static co_thread*
get_self()
{
    return static_cast<co_thread*>(pthread_getspecific(self_key));
}

static void
set_self(co_thread* self)
{
    pthread_setspecific(self_key, self);
}
#endif /* !HAVE_TLS */


} // namespace vigil
