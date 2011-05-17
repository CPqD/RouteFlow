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
#include <config.h>
#include "threads/signals.hh"
#include <assert.h>
#include <boost/static_assert.hpp>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include "sigset.hh"
#include "threads/impl.hh"

namespace vigil {

struct Signal_group_impl {
    Sigset signals;
    int in_fd;
    int out_fd;
    bool read(Sigset&);
    void write(Sigset);
};

/* Estimate the number of signals.  The estimate is likely to be on the high
 * side, but for safety's sake we verify that all of the signals anyone is
 * likley to be interested in fall inside the range. */
static const int MAX_SIGNAL = sizeof(sigset_t) * CHAR_BIT + 1;
BOOST_STATIC_ASSERT(SIGABRT < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGALRM < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGBUS < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGCHLD < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGCONT < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGFPE < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGHUP < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGILL < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGINT < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGKILL < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGPIPE < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGQUIT < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGSEGV < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGSTOP < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGTERM < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGTSTP < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGTTIN < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGTTOU < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGUSR1 < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGUSR2 < MAX_SIGNAL);
#ifdef SIGPOLL
BOOST_STATIC_ASSERT(SIGPOLL < MAX_SIGNAL);
#endif
BOOST_STATIC_ASSERT(SIGPROF < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGSYS < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGTRAP < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGURG < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGVTALRM < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGXCPU < MAX_SIGNAL);
BOOST_STATIC_ASSERT(SIGXFSZ < MAX_SIGNAL);

/* Associates each signal with the Signal_group_impl that should hook it.
 * (This is a global variable because signals do not allow for user data.) */
static Signal_group_impl* group_by_signal[MAX_SIGNAL];

/* Tries to read a sig_set_t from 'in_fd' and adds the signals read to 'ss'.
 * Return true if successful, false if 'in_fd' was empty. */
bool
Signal_group_impl::read(Sigset& ss)
{
    sigset_t raw_ss;
    ssize_t nbytes = ::read(in_fd, &raw_ss, sizeof raw_ss);
    if (nbytes != sizeof raw_ss) {
        int error = nbytes < 0 ? errno : 0;
        if (error != EAGAIN) {
            ::fprintf(stderr, "Signal_group_impl::read");
            ::fprintf(stderr, " (%s)", strerror(error));
            ::exit(EXIT_FAILURE);
        }
        return false;
    }
    ss |= raw_ss;
    return true;
}

/* Writes 'ss' to 'out_fd'.  Cannot fail even if 'out_fd' is full: in that
 * case, it reads a signal set from 'in_fd', adds the signal numbers read from
 * it to the signals passed in, and writes them back.  */
void
Signal_group_impl::write(Sigset ss)
{
    for (;;) {
        ssize_t nbytes = ::write(out_fd, &ss.sigset(), sizeof ss.sigset());
        if (nbytes == sizeof ss.sigset()) {
            return;
        }
        int error = nbytes < 0 ? errno : 0;
        if (error != EAGAIN) {
            ::fprintf(stderr, "Signal_group_impl::write");
            ::fprintf(stderr, " (%s)", strerror(error));
            ::exit(EXIT_FAILURE);
        }

        /* Write failed because the pipe was full, so drain the pipe.  Ignore
         * the return value because we can race with other readers. */
        read(ss);
    }
}

/* Signal handler. */
static void
co_signal_group_handler(int sig_nr)
{
    Signal_group_impl* group = group_by_signal[sig_nr];
    if (group) {
        Sigset sigset;
        sigset.add(sig_nr);
        group->write(sigset);
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

/* Initializes the Signal_group.  Initially the group contains no
 * signals. */
Signal_group::Signal_group()
{
    /* Make a pipe. */
    int fds[2];
    if (pipe(fds)) {
        fprintf(stderr, "pipe: %s\n", strerror(errno));
        abort();
    }

    /* Make it nonblocking on both ends. */
    int error = set_nonblocking(fds[0]);
    if (!error) {
        error = set_nonblocking(fds[1]);
    }
    if (error) {
        fprintf(stderr, "fcntl: %s\n", strerror(errno));
        abort();
    }

    /* Set up our data. */
    pimpl = new Signal_group_impl;
    pimpl->in_fd = fds[0];
    pimpl->out_fd = fds[1];
}

/* Adds 'sig_nr' to the signal group.  Afterward, this signal can be caught
 * using the dequeue() or try_dequeue() member function.
 *
 * The signal must have the default handling if this function is to have any
 * effect.  If the signal is set up to be ignored, then this function takes no
 * action, because this indicates that the program that invoked us wants us to
 * ignore this signal.  If the signal already has a handler, then this function
 * assert-fails, because indicates that someone else is already handling this
 * signal, and we'd only be interfering with them. */
void
Signal_group::add_signal(int sig_nr)
{
    assert(sig_nr > 0 && sig_nr < MAX_SIGNAL);
    if (!pimpl->signals.contains(sig_nr)) {
        struct sigaction sa;

        /* Get current handler. */
        if (sigaction(sig_nr, NULL, &sa)) {
            abort();
        }
        assert(sa.sa_handler == SIG_DFL || sa.sa_handler == SIG_IGN);
        if (sa.sa_handler != SIG_DFL) {
            return;
        }

        /* Let the handler know about us. */
        group_by_signal[sig_nr] = pimpl;

        /* Set our handler. */
        sa.sa_handler = co_signal_group_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        if (sigaction(sig_nr, &sa, NULL)) {
            abort();
        }

        /* Add to our signal set. */
        pimpl->signals.add(sig_nr);
    }
}

/* Removes 'sig_nr' from the signal group.  The signal reverts to its default
 * handling. */
void
Signal_group::remove_signal(int sig_nr)
{
    assert(sig_nr > 0 && sig_nr < MAX_SIGNAL);
    if (pimpl->signals.contains(sig_nr)) {
        /* Withdraw the handler's knowledge of us. */
        group_by_signal[sig_nr] = NULL;

        /* Remove from our signal set. */
        pimpl->signals.remove(sig_nr);

        /* Restore default handler. */
        if (signal(sig_nr, SIG_DFL) == SIG_ERR) {
            abort();
        }
    }
}

/* Removes all signals from the signal group. */
void
Signal_group::remove_all()
{
    while (int sig_nr = pimpl->signals.scan()) {
        remove_signal(sig_nr);
    }
}

/* Blocks until one of the signals in this signal group is delivered, then
 * removes the lowest-numbered of the delivered signals from the queue and
 * returns it. */
int
Signal_group::dequeue()
{
    int sig_nr;
    while (!(sig_nr = try_dequeue())) {
        wait();
        co_block();
    }
    return sig_nr;
}

/* If one of the signals in this signal group has been delivered, removes the
 * lowest-numbered of the delivered signals from the queue and returns it.
 * Otherwise, if no signal has been delivered, returns 0. */
int
Signal_group::try_dequeue()
{
    Sigset ss;
    while (pimpl->read(ss)) {
        while (int sig_nr = ss.scan()) {
            ss.remove(sig_nr);
            if (!pimpl->signals.contains(sig_nr)) {
                continue;
            }

            if (ss.scan(sig_nr + 1)) {
                pimpl->write(ss);
            }
            return sig_nr;
        }
    }
    return 0;
}

/* Initializes 'event' as an event that waits for one of the signals in this
 * signal group to be delivered. */
void
Signal_group::wait()
{
    co_fd_read_wait(pimpl->in_fd, NULL);
}

} // namespace vigil
