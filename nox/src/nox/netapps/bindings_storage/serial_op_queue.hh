/* Copyright 2008, 2009 (C) Nicira, Inc.
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
#ifndef SERIAL_OP_QUEUE
#define SERIAL_OP_QUEUE

#include <list>
#include <boost/function.hpp>
#include "vlog.hh"

namespace vigil {
namespace applications {


typedef boost::function<void()> Serial_Op_fn;

/**
 * Helper class to help with the scenario when
 * multiple types of asynchronous operations must
 * occur in a serial fashion.  For example, adding
 * and removing rows from certain database tables
 * should be performed in serial to avoid inconsistent
 * states
 */
class Serial_Op_Queue {

public:
    Serial_Op_Queue(container::Component *c, std::string n, Vlog_module &log)
        : component(c), serial_op_running(false),
          name(n), error_log(log)
        {}

    // Indicate that a serial operation is ready to run.
    // If another serial operation is already in progress, we
    // enqueue this operation for later dispatch
    void add_serial_op(const Serial_Op_fn &fn) {
        if (!serial_op_running){
            start_serial_op(fn);
            return;
        }

        pending_op_queue.push_back(fn);

        // this code checks that we are not "stuck"
        struct timeval cur_time = do_gettimeofday();
        uint32_t sec_diff = cur_time.tv_sec - last_start_time.tv_sec;
        if (sec_diff > 3) {
            error_log.err("Serial queue '%s' has size = %zu and "
                          " has not completed an operation in %d seconds."
                          " This is likely a bug\n",
                          name.c_str(), pending_op_queue.size(), sec_diff);
        }
    }

    // Called whenever a serial operation completes in
    // order to dispatch a subsequent op if one exists
    void finished_serial_op() {
        if(pending_op_queue.size() == 0) {
            serial_op_running = false;
            return;
        }
        Serial_Op_fn fn = pending_op_queue.front();
        pending_op_queue.pop_front();
        start_serial_op(fn);
    }

    int size() { return pending_op_queue.size(); }

private:
    std::list<Serial_Op_fn > pending_op_queue;
    container::Component *component; // needed to call post();
    bool serial_op_running;

    // for debugging statements
    std::string name;
    Vlog_module &error_log;
    struct timeval last_start_time;

    void start_serial_op(const Serial_Op_fn &fn) {
        last_start_time = do_gettimeofday(); // for debug checks
        serial_op_running = true;
        component->post(fn); // invoke function
    }

};

}
}

#endif
