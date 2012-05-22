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
/* Tests Pollable insertion and removal. */
#include "event-dispatcher.hh"
#include "threads/cooperative.hh"
#include <cstdio>

using namespace vigil;

class My_pollable
    : public Pollable
{
public:
    Poll_loop& loop;
    int id, count;
    My_pollable(Poll_loop& loop_, int id_) : loop(loop_), id(id_), count(0) { }
    bool poll();
    void wait() { }
};

bool
My_pollable::poll() 
{
    printf("Polled My_pollable(%d)\n", id);
    if (id == 1) {
        loop.remove_pollable(this);
    } else {
        if (++count >= 3) {
            exit(0);
        }
    }
    return true;
}

int
main(int argc, char *argv[])
{
    co_init();
    co_thread_assimilate();
    co_migrate(&co_group_coop);

    Poll_loop loop(4);

    /* First make sure that the trivial case is OK. */
    My_pollable p1(loop, 1);
    loop.add_pollable(&p1);
    loop.remove_pollable(&p1);

    /* Then check the slightly less trivial case that we can remove a Pollable
     * while it's running. */
    My_pollable p2(loop, 2);
    loop.add_pollable(&p1);
    loop.add_pollable(&p2);
    loop.run();
}
