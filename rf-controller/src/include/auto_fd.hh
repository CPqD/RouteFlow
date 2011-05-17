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
#ifndef AUTO_FD_HH
#define AUTO_FD_HH 1

namespace vigil {

/* A single-owner file descriptor suitable for use with RAII. */
class Auto_fd
{
public:
    explicit Auto_fd(int fd_ = -1) : fd(fd_) { }
    ~Auto_fd() { close(); }
    void reset(int fd_ = -1) { close(); fd = fd_; }
    int release() { int tmp = fd; fd = -1; return tmp; }
    int close();
    int get() { return fd; }
    operator int() { return fd; }
private:
    int fd;

    Auto_fd(const Auto_fd&);
    Auto_fd& operator=(const Auto_fd&);
};

} // namespace vigil

#endif /* auto_fd.hh */
