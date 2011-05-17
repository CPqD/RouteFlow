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
/* Tests for Co_signal_group. */

#include "threads/signals.hh"
#include <signal.h>
#include "threads/cooperative.hh"
#include "threads/impl.hh"
#include <cstdio>

using namespace vigil;

static Signal_group& sig_group = *new Signal_group;
static Co_sema sema;

#define MUST_SUCCEED(EXPRESSION)                    \
    if (!(EXPRESSION)) {                            \
        fprintf(stderr, "%s:%d: %s failed\n",       \
                __FILE__, __LINE__, #EXPRESSION);   \
        exit(EXIT_FAILURE);                         \
    }

static void
thread1()
{
    printf("up\n");
    sema.up();
    printf("try_dequeue\n");
    MUST_SUCCEED(!sig_group.try_dequeue());
    printf("dequeue\n");
    MUST_SUCCEED(sig_group.dequeue() == SIGTERM);
}

int
main()
{
    co_init();
    co_thread_assimilate();
    co_migrate(&co_group_coop);

    co_thread* thread;
    Co_completion join;

    thread = co_thread_create(&co_group_coop, thread1);
    co_join_completion(thread, &join);
    sig_group.add_signal(SIGTERM);
    printf("down\n");
    sema.down();
    printf("kill\n");
    kill(getpid(), SIGTERM);
    printf("join\n");
    join.block();

    return 0;
}
