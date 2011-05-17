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
#ifndef PY_STORAGE_HH
#define PY_STORAGE_HH 1

#include <Python.h>

#include "component.hh"
#include "storage.hh"

namespace vigil {
namespace applications {
namespace storage {

/* Python bindings for non-transactional multi key-value pair
 * storage.
 */
class PyStorage {
public:
    PyStorage(PyObject*);
    void configure(PyObject*);
    void install(PyObject*);
    PyObject* create_table(PyObject*, PyObject*, PyObject*, PyObject*);
    PyObject* drop_table(PyObject*, PyObject*);
    PyObject* get(PyObject*, PyObject*, PyObject*);
    PyObject* get_next(PyObject*, PyObject*);
    PyObject* put(PyObject*, PyObject*, PyObject*);
    PyObject* modify(PyObject*, PyObject*, PyObject*);
    PyObject* remove(PyObject*, PyObject*);
    PyObject* put_row_trigger(PyObject*, PyObject*, PyObject*);
    PyObject* put_table_trigger(PyObject*, PyObject*, PyObject*, PyObject*);
    PyObject* remove_trigger(PyObject*, PyObject*);
    
private:
    container::Component* c;
    Async_storage* storage;
};

} // namespace storage
} // namespace applications
} // namespace vigil

#endif
