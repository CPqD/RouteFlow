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
/* Tests for Co_sema. */

#include "threads/cooperative.hh"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <unistd.h>
#include <cstdio>

using namespace vigil;

static Co_sema sema1, sema2;

static void
thread1()
{
    for (int i = 0; i < 10; ++i) {
        sema1.down();
        printf("down\n");
    }
}

static void
thread2()
{
    for (int i = 0; i < 10; ++i) {
        int wake1, wake2;
        sema1.wait(&wake1);
        sema2.wait(&wake2);
        co_block();
        printf("block wake1=%d wake2=%d\n", wake1, wake2);
    }
}

static void
thread3(int id)
{
    printf("start id=%d\n", id);
    sema1.down();
    printf("wake id=%d\n", id);
    sema2.up();
}

int
main()
{
    co_init();
    co_thread_assimilate();
    co_migrate(&co_group_coop);

    co_thread* thread;
    Co_completion join;

    /* These tests tend to hang if something goes wrong. */
    alarm(3);

    printf("Up without yield\n");
    thread = co_thread_create(&co_group_coop, thread1);
    co_join_completion(thread, &join);
    for (int i = 0; i < 10; ++i) {
        sema1.up();
        printf("up\n");
    }
    join.block();
    join.latch();

    printf("\nUp with yield\n");
    thread = co_thread_create(&co_group_coop, thread1);
    co_join_completion(thread, &join);
    for (int i = 0; i < 10; ++i) {
        sema1.up();
        printf("up\n");
        co_yield();
    }
    join.block();
    join.latch();

    printf("\nAlternating wakers\n");
    thread = co_thread_create(&co_group_coop, thread2);
    co_join_completion(thread, &join);
    for (int i = 0; i < 5; ++i) {
        co_yield();
        sema1.up();
        printf("up sema1\n");

        co_yield();
        sema2.up();
        printf("up sema2\n");
    }
    join.block();
    join.latch();

    printf("\nFIFO queuing\n");
    const int n_threads = 10;
    co_thread* threads[n_threads];
    Co_completion joins[n_threads];
    for (int i = 0; i < 10; ++i) {
        threads[i] = co_thread_create(&co_group_coop, boost::bind(thread3, i));
        co_join_completion(threads[i], &joins[i]);
    }
    co_yield();
    for (int i = 0; i < 10; ++i) {
        printf("up id=%d\n", i);
        sema1.up();
        sema2.down();
    }
    for (int i = 0; i < 10; ++i) {
        joins[i].block();
    }

    return 0;
}
