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
%{
#include "oxidereactor.cc"
%}
class delayedcall {
    public:
        void cancel();
        void delay(bool, long, long);
        void reset(long, long);
};

class oxidereactor
{
    public:    
        oxidereactor(PyObject*, PyObject*);
        ~oxidereactor();

        PyObject* addReader(int, PyObject*, PyObject*);
        PyObject* addWriter(int, PyObject*, PyObject*);
        PyObject* removeReader(int);
        PyObject* removeWriter(int);
        
        PyObject* callLater(long, long, PyObject*);
        PyObject* resolve(PyObject*, PyObject*);
};

class vigillog
{
    public:
        PyObject* fatal(int, PyObject*);
        PyObject* warn(int, PyObject*);
        PyObject* err(int, PyObject*);
        PyObject* info(int, PyObject*);
        PyObject* dbg(int, PyObject*);
        PyObject* mod_init(PyObject*);
        bool is_emer_enabled(int);
        bool is_err_enabled(int);
        bool is_warn_enabled(int);
        bool is_info_enabled(int);
        bool is_dbg_enabled(int);
};

