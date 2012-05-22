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
#include "event.hh"
#include <typeinfo>
#include <vector>
#include <string>

/* Following are for Event::get_name below.
 * Not portable outside GCC's C++ ABI.
 */
#include <cstdlib>
#include <cxxabi.h>

namespace vigil {

Event::Event(const Event_name& name_) : 
    name(name_) { 

}

Event::~Event() { 

}

void
Event::set_name(const Event_name& name_) {
    name = name_;
}

Event_name 
Event::get_name() const { 
    return name; 
}

std::string Event::get_class_name() const
{
    const char* mangled_name = typeid(*this).name();

    int status;
    char* demangled_name = __cxxabiv1::__cxa_demangle(mangled_name,
						      0, 0, &status);
    if (status)
	return mangled_name;

    std::string s(demangled_name);
    free(demangled_name); /* Yes, free not delete. */
    return s;
}

} // namespace vigil
