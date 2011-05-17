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
#ifndef DEFERREDCALLBACK_HH
#define DEFERREDCALLBACK_HH 1

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <Python.h>

namespace vigil {
namespace applications {

class DeferredCallback
    : boost::noncopyable
{
public:
    typedef void Callback_signature(PyObject*);
    typedef boost::function<Callback_signature> Callback;

    DeferredCallback(); // For Python only
    ~DeferredCallback();

    void *operator()(PyObject*);

    static PyObject *get_instance(const Callback&);
    static bool add_callbacks(PyObject *deferred, PyObject *cb, PyObject *ecb);
    static bool add_callback(PyObject* deferred, PyObject* cb);
    static bool add_errback(PyObject* deferred, PyObject* cb);

private:
    DeferredCallback(const Callback&);

    Callback cb;
};

} /* namespace applications */
} /* namespace vigil */

#endif /* DEFERREDCALLBACK_HH */
