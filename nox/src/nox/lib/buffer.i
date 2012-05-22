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
/* Convert python string to a const Nonowning_buffer& 
 * 
 * */

%typemap(in) const Nonowning_buffer& (Nonowning_buffer temp) {

      if (!PyString_Check($input)){
          fprintf(stderr, " (openflow typemap) py argument to Buffer must be of type str, instead received\n");
          PyObject_Print($input, stdout, 0); printf("\n");
          return SWIG_Py_Void();
      }
      ssize_t str_size = PyString_Size($input);
      temp.init((uint8_t*)PyString_AsString($input), str_size);
      $1 = &temp;
}

%typemap(in) const std::list<Nonowning_buffer>& (std::list<Nonowning_buffer> temp) {
    if (!PyList_Check($input)) {
        fprintf(stderr, " (Buffer list typemap) py argument must be of type list, instead received\n");
        PyObject_Print($input, stderr, 0); printf("\n");
    } else {
        uint32_t size = PyList_GET_SIZE($input);
        for (uint32_t i = 0; i < size; i++) {
            PyObject *str = PyList_GET_ITEM($input, i);
            if (!PyString_Check(str)){
                fprintf(stderr, " (Buffer list typemap) py list must contain type strs, instead received\n");
                PyObject_Print(str, stderr, 0); printf("\n");
                break;
            }
            uint32_t str_size = PyString_GET_SIZE(str);
            temp.push_back(Nonowning_buffer(PyString_AS_STRING(str), str_size));
        }   
    }
    $1 = &temp;
}

