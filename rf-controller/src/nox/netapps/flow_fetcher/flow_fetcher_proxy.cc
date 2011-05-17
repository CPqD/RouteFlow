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
*/

#include "flow_fetcher_proxy.hh"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include "pyrt/pycontext.hh"
#include "swigpyrun.h"
#include "vlog.hh"
#include "pyrt/pyglue.hh"

using namespace std;
using namespace vigil;
using namespace vigil::applications;

namespace {
Vlog_module lg("flow_fetcher_proxy");
}

namespace vigil {
namespace applications {

Flow_fetcher_proxy::Flow_fetcher_proxy(PyObject* ctxt_, PyObject* dpid_,
                                       PyObject* request_, PyObject* cb_)
{
    const container::Context* ctxt
        = from_python<const container::Context*>(ctxt_);
    datapathid dpid = from_python<datapathid>(dpid_);
    ofp_flow_stats_request request
        = from_python<ofp_flow_stats_request>(request_);
    if (!cb_ || !PyCallable_Check(cb_)) {
        PyErr_SetString(PyExc_TypeError, "'f' object is not callable");
    }
    cb = cb_;
    Py_XINCREF(cb);
    ff = Flow_fetcher::fetch(ctxt, dpid, request,
                             boost::bind(&Flow_fetcher_proxy::thunk, this));
}

Flow_fetcher_proxy::~Flow_fetcher_proxy()
{
    ff->cancel();
    Py_CLEAR(cb);
}

void
Flow_fetcher_proxy::thunk()
{
    /* Don't combine these statements: Py_XDECREF is a macro that evaluates its
     * argument twice. */
    PyObject* retval = PyObject_CallObject(cb, NULL);
    Py_XDECREF(retval);
}

void
Flow_fetcher_proxy::cancel()
{
    ff->cancel();
}

int
Flow_fetcher_proxy::get_status() const
{
    return ff->get_status();
}

PyObject*
Flow_fetcher_proxy::get_flows() const
{
    const std::vector<Flow_stats>& flows = ff->get_flows();
    PyObject* pyflows = PyTuple_New(flows.size());
    Py_ssize_t i = 0;
    BOOST_FOREACH (const Flow_stats& fs, flows) {
        PyTuple_SET_ITEM(pyflows, i, to_python(fs));
        ++i;
    }
    return pyflows;
}

} // namespace applications
} // namespace vigil
