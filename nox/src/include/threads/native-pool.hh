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
#ifndef NATIVE_THREAD_POOL_HH
#define NATIVE_THREAD_POOL_HH

#include <assert.h>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <fcntl.h>
#include <queue>

#include "threads/cooperative.hh"
#include "threads/native.hh"

namespace vigil {

/* A native thread pool implementation for executing tasks in a fixed
   set of native threads. */
template <typename R, typename W>
class Native_thread_pool
    : boost::noncopyable {

public:
    typedef boost::function<R(W*)> T;
    typedef boost::function<void(R)> Callback;

    /**
     * \param batch sets the maximum number of pending tasks to
     * execute in a worker thread per a mutex access.
     */
    Native_thread_pool(const int batch = 1)
        : running(true), max_batch(batch), n_threads(0) {

        // Note, the worker thread may block while writing.
        int pfd[2];
        if (pipe(pfd) == -1) {
            throw std::runtime_error("Unable to create a pipe for "
                                     "a native worker thread.");
        }

        read_fd = pfd[0];
        write_fd = pfd[1];
        int flags = fcntl(read_fd, F_GETFL, 0);
        if (fcntl(read_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            throw std::runtime_error("Unable to set a pipe non-blocking.");
        }

        inject_fsm.start(boost::bind(&Native_thread_pool<R, W>::inject, this));
        fetch_fsm.start(boost::bind(&Native_thread_pool<R,W >::fetch, this));
    }

    /* Destructor blocks until the worker threads finish, if the pool
       has not been shutdown before. */
    ~Native_thread_pool() {
        if (running) {
            running = false;
            wait_threads(false, 0);
        }

        close(write_fd);
        close(read_fd);
        co_fd_closed(read_fd);
    }

private:
    /* Wait for the worker(s) to finish. */
    void wait_threads(bool async, const boost::function<void()>& cb) {
        co_might_yield();
        
        {
            Scoped_native_mutex lock(&mutex);
            available.broadcast();
        }
        
        for (int i = 0; i < n_threads; ++i) {
            dead_workers.down();
        }
       
        if (async) { cb(); }
    }

public:
    /* Asynchronous pool shutdown */
    void shutdown(const boost::function<void()> cb) {
        using namespace boost;
        
        running = false;

        new Co_thread(bind(&Native_thread_pool<R,W>::wait_threads, this,
                           true, cb));

        /* Thread is deleted once it completes */
    }

    /* Add a worker object to the pool. */
    void add_worker(W* w, const boost::function<void()>& init) const {
        Scoped_native_mutex lock(&mutex);

        Native_thread t;
        t.start(boost::bind(&Native_thread_pool<R, W>::run, this, init));
        free_workers.push_back(w);
        ++n_threads;
    }

    /*
     * Executes a task and blocks until the task completes.
     *
     * \param t is the task to execute.
     */
    R execute(const T& t) const {
        boost::shared_ptr<Task>
            task((Task*)new BlockingTask(t));
        to_inject.push_back(task);
        need_injection.broadcast();
        task->wait();

        return task->r;
    }

    /*
     * Executes a task and returns immediately once the task is queued
     * into the pool. Once the task completes, the pool calls the
     * callback.
     *
     * \param t is the task to execute.
     * \param cb is the callback to execute once the task completes.
     */
    void execute(const T& t, const Callback& cb) const {
        boost::shared_ptr<Task>
            task((Task*)new NonblockingTask(t, (Callback)cb));
        to_inject.push_back(task);
        need_injection.broadcast();
    }

private:
    class Task {
    public:
        Task(T t_) : t(t_) { }

        /* Since the pool uses boost::shared_ptr<Task> for the task
           management, proper memory management requires a virtual
           destructor. */
        virtual ~Task() { }

        inline void execute(W* w) { r = t(w); }
        virtual void complete() = 0;
        virtual void wait() = 0;

        const T t;
        R r;
    };

    class BlockingTask
        : public Task
    {
    public:
        BlockingTask(const T& t) : Task(t) { }

        ~BlockingTask() { }
        void complete() { c.release(); }
        void wait() { c.block(); }

    private:
        Co_completion c;
    };

    class NonblockingTask
        : public Task
    {
    public:
        NonblockingTask(const T& t, const Callback& cb_)
            : Task(t), cb(cb_) { };

        ~NonblockingTask() { }
        void complete() { if (cb) { cb(this->r); } }
        void wait() { }
    private:
        const Callback cb;
    };

    /* main() for the worker thread. */
    void run(const boost::function<void()>& init) const {
        boost::shared_ptr<std::deque<boost::shared_ptr<Task> > >
            e(new std::deque<boost::shared_ptr<Task> >());
        std::deque<boost::shared_ptr<Task> > results;
        W *w = 0;

        if (init) {
            init();
        }

        while (running) {
            /* Fetch pending tasks and hand results of previous tasks
               back. */

            {
                Scoped_native_mutex lock(&mutex);
                const bool trigger_poll = to_dispatch.empty() &&
                    !results.empty();

                while (!results.empty()) {
                    to_dispatch.push_back(results.front());
                    results.pop_front();
                }

                if (trigger_poll) {
                    const static char buf = '\0';
                    write(write_fd, &buf, sizeof(buf));
                }

                if (w) {
                    free_workers.push_front(w);
                }

                if (to_execute.empty()) {
                    /* Wait for more. */
                    available.wait(lock);
                }

                if (to_execute.size() > max_batch) {
                    for (int i = 0; i < max_batch; ++i) {
                        e->push_back(to_execute.front());
                        to_execute.pop_front();
                    }

                    /* Trigger another thread, in case there's a free
                       one. */
                    available.signal();

                } else if (to_execute.size() > 0) {
                    to_execute.swap(*e);
                }

                w = free_workers.front();
                free_workers.pop_front();
            }

            /* Execute the tasks */
            while (!e->empty() && running)  {
                boost::shared_ptr<Task> t = e->front();
                t->execute(w);

                results.push_back(t);
                e->pop_front();
            }
        }

        dead_workers.up();
    }

    /* whether the pool is still up and running or not */
    bool running;

    /* Maximum # of requests fetched by a worker thread per a mutex
       access. Default 1. */
    const int max_batch;

    /* Mutex protecting containers passing tasks between caller and
       pool thread(s). */
    mutable Native_mutex mutex;

    /* Condition variable signaling about new tasks to execute. */
    mutable Native_cond available;

    /* Free worker threads */
    mutable std::deque<W*> free_workers;

    /* Pipe for signaling the calling thread from a worker thread
       about results. */
    int read_fd, write_fd;

    /* Injects a task into the the pool. */
    void inject() const {
        {
            Scoped_native_mutex lock(&mutex);

            while (!to_inject.empty()) {
                to_execute.push_back(to_inject.front());
                to_inject.pop_front();
            }

            available.signal();
        }

        need_injection.wait();
        co_fsm_block();
    }

    /* up'd by a worker thread at its death;
       down'd by the destructor to wait for worker threads to die. */
    mutable Native_sema dead_workers;
    mutable size_t n_threads;

    /* Condition variable to ask for a task delivery from the
       main thread to the worker thread.  */
    mutable Co_cond need_injection;

    /* Tasks queued by clients but not yet transferred to 'rqueue'.
       Owned by client end; not synchronized. */
    mutable std::deque<boost::shared_ptr<Task> > to_inject;

    /* Thread injecting tasks into the pool. */
    Auto_fsm inject_fsm;

    /* Tasks queued for worker threads. Protected by 'mutex'. */
    mutable std::deque<boost::shared_ptr<Task> > to_execute;

    /* Retrieves complete tasks from the pool. */
    void fetch() const {
        char buf;
        int err;

        if ((err = read(read_fd, &buf, sizeof(buf)))) {
            Scoped_native_mutex lock(&mutex);

            while (!to_dispatch.empty()) {
                to_dispatch.front()->complete();
                to_dispatch.pop_front();
            }
        } else {
            /* Unable to the read pipe. */
        }

        co_fd_read_wait(read_fd, NULL);
        co_fsm_block();
    }

    /* Completed tasks. Protected by 'mutex'. */
    mutable std::deque<boost::shared_ptr<Task> > to_dispatch;

    /* Thread retreving complete tasks from the pool. */
    Auto_fsm fetch_fsm;
};

} // namespace vigil

#endif /* native-thread-pool.hh */
