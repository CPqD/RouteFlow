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
#ifndef LEAK_CHECKER_HH
#define LEAK_CHECKER_HH 1

#include <sys/types.h>

namespace vigil {
void leak_checker_start(const char *file_name);
void leak_checker_set_limit(off_t limit);
void leak_checker_stop();
void leak_checker_claim(const void *);
void leak_checker_usage();
}

#endif /* leak-checker.hh */
