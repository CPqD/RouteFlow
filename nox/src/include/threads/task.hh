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
/*
 * Base class for cooperative and native threads and FSMs.
 *
 * The overall class hierarchy looks like this:
 *
 *                              Task
 *                             /    \
 *                            /      \
 *                 Native_thread    Co_task
 *                                   /    \
 *                                  /      \
 *                             Co_thread   Co_fsm
 *
 * The Task at the root of this hierarchy is a simple wrapper around the
 * lowest-level implementation abstraction of a co_thread, which is the
 * underlying data structure that all of these represent.
 */

#ifndef THREADS_TASK_HH
#define THREADS_TASK_HH 1

#include "threads/impl.hh"

namespace vigil {

/* Base class for cooperative and native threads and FSMs. */
class Task
{
public:
    bool empty() const;
    bool operator==(const Task&) const;
    bool operator!=(const Task&) const;

protected:
    co_thread* thread;

    Task();
    Task(co_thread*);
};

/* Constructs an empty Task that does not yet represent any underlying task. */
inline
Task::Task()
    : thread(NULL)
{}

/* Constructs a Task that represents 'thread', which may be a cooperative or
 * native thread or FSM. */
inline
Task::Task(co_thread* thread_)
    : thread(thread_)
{}

/* Returns true if the Task has no underlying task,
 * false if it represents some cooperative or native thread or FSM. */
inline bool
Task::empty() const
{
    return thread == NULL;
}

/* Returns true if 'this' and 'rhs' represent the same underlying task, or if
 * they are both empty, false otherwise.*/
inline bool
Task::operator==(const Task& rhs) const
{
    return thread == rhs.thread;
}

/* Returns true if 'this' and 'rhs' represent different underlying tasks, or if
 * one represents a task and the other is empty, false otherwise.*/
inline bool
Task::operator!=(const Task& rhs) const
{
    return thread != rhs.thread;
}

} // namespace vigil

#endif /* threads/task.hh */
