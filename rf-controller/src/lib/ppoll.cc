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
#include "ppoll.hh"
#include <assert.h>
#include <stdlib.h>
#include "socket-util.hh"
#include "timeval.hh"

namespace vigil {

#ifdef HAVE_PPOLL
static void
noop_handler(int)
{
}

Ppoll::Ppoll(int sig_nr_)
    : sig_nr(sig_nr_)
{
    pthread_sigmask(SIG_SETMASK, NULL, &unblock_signal);

    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = noop_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(sig_nr, &sa, &old_sigaction);

    assert(old_sigaction.sa_handler == SIG_DFL);
}

Ppoll::~Ppoll()
{
    sigaction(sig_nr, &old_sigaction, NULL);
}

void
Ppoll::block()
{
    sigset_t sigmask;
    sigemptyset(&sigmask);
    sigaddset(&sigmask, sig_nr);
    int error = pthread_sigmask(SIG_BLOCK, &sigmask, NULL);
    if (error) {
        ::fprintf(stderr, "pthread_sigmask");
        ::fprintf(stderr, " (%s)", strerror(error));
        ::exit(EXIT_FAILURE);
    }
}

void
Ppoll::unblock()
{
    sigset_t sigmask;
    sigemptyset(&sigmask);
    sigaddset(&sigmask, sig_nr);
    int error = pthread_sigmask(SIG_UNBLOCK, &sigmask, NULL);
    if (error) {
        fprintf(stderr, "pthread_sigmask");
        fprintf(stderr, " (%s)", strerror(error));
        exit(EXIT_FAILURE);
    }
}

void
Ppoll::interrupt(pthread_t thread)
{
    pthread_kill(thread, sig_nr);
}

int
Ppoll::poll(std::vector<pollfd>& pollfds, const struct timespec *timeout)
{
    return (!pollfds.empty()
            ? ::ppoll(&pollfds[0], pollfds.size(), timeout, &unblock_signal)
            : ::ppoll(NULL, 0, timeout, &unblock_signal));
}
#else /* !HAVE_PPOLL */
Ppoll::Ppoll(int)
{
    if (pipe(fds)) {
        ::fprintf(stderr, "pipe");
        ::fprintf(stderr, " (%s)", strerror(errno));
        ::exit(EXIT_FAILURE);
    }
    set_nonblocking(fds[0]);
    set_nonblocking(fds[1]);
}


Ppoll::~Ppoll()
{
}

void
Ppoll::block()
{
}

void
Ppoll::unblock()
{
}

void
Ppoll::interrupt(pthread_t thread)
{
    write(fds[1], "", 1);
}

int
Ppoll::poll(std::vector<pollfd>& pollfds, const struct timespec *timeout)
{
    pollfd pfd;
    pfd.fd = fds[0];
    pfd.events = POLLIN;
    pollfds.push_back(pfd);

    long int ms;
    if (timeout) {
        ms = timespec_to_ms(*timeout);
#if INT_MAX < LONG_MAX
        ms = std::min(ms, (long int)INT_MAX);
#endif
    } else {
        ms = -1;
    }

    int retval = ::poll(&pollfds[0], pollfds.size(), ms);
    if (retval > 0 && pollfds.back().revents) {
        /* Drain the pipe. */
        int save_errno = errno;
        char buffer[512];
        while (::read(fds[0], buffer, sizeof buffer) > 0) {
            continue;
        }
        errno = save_errno;

        /* Fix up retval. */
        if (--retval == 0) {
            retval = -1;
            errno = EINTR;
        }
    }
    pollfds.pop_back();

    return retval;
}
#endif /* !HAVE_PPOLL */

} // namespace vigil
