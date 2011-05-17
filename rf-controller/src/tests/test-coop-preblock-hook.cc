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
/* Trivial test to ensure that preblocking hooks set with
 * co_set_preblock_hook() actually fire when they should. */

#include "threads/cooperative.hh"
#include <boost/bind.hpp>
#include <cstdio>

using namespace vigil;

static Co_sema start;
static Co_sema done;

static void
thread1_hook() 
{
    printf("hook\n");
}

static void
thread1()
{
    start.up();
    for (int i = 0; i < 10; i++) {
        printf("thread1\n");
        co_set_preblock_hook(thread1_hook);
        co_yield();
    }
    done.up();
}

static void
thread2() 
{
    start.down();
    for (int i = 0; i < 10; i++) {
        printf("thread2\n");
        co_yield();
    }
    done.up();
}

int
main()
{
    co_init();
    co_thread_assimilate();
    co_migrate(&co_group_coop);
    co_thread_create(&co_group_coop, thread1);
    co_thread_create(&co_group_coop, thread2);
    done.down();
    done.down();
}
