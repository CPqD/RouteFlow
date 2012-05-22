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
//
// utf8_string - pass python unicode string parameters to C++ as UTF-8
//               encoded std::string
//
#ifndef SWIG_UTF8_STRING
#define SWIG_UTF8_STRING

%{
#include <string>
%}

%typemap(in) std::string {
    if (PyUnicode_Check($input)) {
        PyObject* strobj = PyUnicode_AsUTF8String($input);
        if (!strobj) {
            PyErr_SetString(PyExc_ValueError,
                    "Failed to decode string as utf-8");
            return NULL;
        }
        $1 = string(PyString_AsString(strobj));
        Py_DECREF(strobj);
    }
    else if PyString_Check($input) {
        $1 = string(PyString_AsString($input));
    }
    else {
        PyErr_SetString(PyExc_ValueError,"Expected a string");
        return NULL;
    }
}

#endif /* ndef SWIG_UTF8_STRING */
