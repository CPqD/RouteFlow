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

#ifndef TESTDEFS_HH__
#define TESTDEFS_HH__

#include <string>
#include <iostream>

#define NOX_ASSERT_PASS "+assert_pass: "
#define NOX_ASSERT_FAIL "+assert_fail: "

inline void NOX_TEST_ASSERT(bool expr, const std::string& name = (const char*)"")
{
    if (expr){
        std::cout << NOX_ASSERT_PASS << name << std::endl;
    }else{
        std::cout << NOX_ASSERT_FAIL << name << std::endl;
    }
}

#endif  // TESTDEFS_HH__
