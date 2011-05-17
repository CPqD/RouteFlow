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
/* Proxy class to expose flow_fetcher.hh to Python.
 * This file is only to be included from the SWIG interface file
 * (flow_fetcher_proxy.i)
 */

#ifndef FLOW_FETCHER_PROXY_HH__
#define FLOW_FETCHER_PROXY_HH__

#include <Python.h>

#include "flow_fetcher.hh"
#include "pyrt/pyglue.hh"

using namespace std;

namespace vigil {
namespace applications {

class Flow_fetcher_proxy {
public:
    Flow_fetcher_proxy(PyObject* ctxt, PyObject* dpid, PyObject* request,
                       PyObject* cb);
    ~Flow_fetcher_proxy();

    void cancel();
    int get_status() const;
    PyObject* get_flows() const;

protected:
    PyObject* cb;
    boost::shared_ptr<Flow_fetcher> ff;

    void thunk();
}; // class Flow_fetcher_proxy

} // namespace applications
} // namespace vigil

#endif //  FLOW_FETCHER_PROXY_HH__
