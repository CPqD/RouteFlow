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
#ifndef PPOLL_HH
#define PPOLL_HH

#include <config.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <vector>

namespace vigil {

/* An interface to poll() that can be interrupted by another thread.
 * On systems with ppoll(), this is implemented as ppoll() and a signal;
 * on other systems, it uses the "pipe trick" to avoid races.
 *
 * Only a single ongoing call to poll() may be reliably interrupted.
 *
 * Instantiating Ppoll is expensive (it consumes a signal or a pair of file
 * descriptors), so ordinarily only a single global instance is used.
 */
class Ppoll {
public:
    /* Instantiate to use signal 'sig_nr'. */
    Ppoll(int sig_nr);
    ~Ppoll();

    /* Blocks 'sig_nr' in the current thread.  Must be called before any other
     * thread might attempt to interrupt this one. */
    void block();

    /* Unblocks 'sig_nr' in the current thread.  May be called only after any
     * possible interruptions of this thread have already completed. */
    void unblock();

    /* Interrupts any blocking call to ->poll() currently taking place in
     * 'thread'. */
    void interrupt(pthread_t thread);

    /* Invokes the poll() system call passing the specified 'pollfds' with the
     * given 'timeout'.
     *
     * On systems that lack ppoll(), the contents of 'pollfds' are modified
     * temporarily for the duration of the call. */
    int poll(std::vector<pollfd>& pollfds, const struct timespec *timeout);
private:
#ifdef HAVE_PPOLL
    int sig_nr;
    sigset_t unblock_signal;
    struct sigaction old_sigaction;
#else
    int fds[2];
#endif
};

} // namespace vigil

#endif /* ppoll.hh */
