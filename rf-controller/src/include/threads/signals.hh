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
/* Signal handling.
 *
 * A signal group is a set of one or more signals that are handled together.
 * That is, a thread that waits for one of the signals in the group also waits
 * for the rest.  Each signal group consumes a pair of file descriptors, hence
 * the reason for grouping them.
 *
 * A given signal may be in at most one group at a time.
 *
 * A signal group may never be destroyed, because there is an inherent race
 * condition between closing the file descriptors and accessing them from the
 * signal handler.  In a multithreaded environment, there is no way to ensure
 * that the signal handler is not running while the file descriptors are being
 * closed.  Instead of destroying a signal group, just remove all of its
 * signals with remove_all(). */

#ifndef THREADS_SIGNALS_HH
#define THREADS_SIGNALS_HH 1

namespace vigil {

struct Signal_group_impl;
class Signal_group {
public:
    Signal_group();
    void add_signal(int sig_nr);
    void remove_signal(int sig_nr);
    void remove_all();
    int dequeue();
    int try_dequeue();
    void wait();
private:
    Signal_group_impl* pimpl;

    ~Signal_group();
};

} // namespace vigil

#endif /* threads/signals.h */
