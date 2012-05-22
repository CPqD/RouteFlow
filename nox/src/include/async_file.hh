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
#ifndef ASYNC_FILE_HH
#define ASYNC_FILE_HH 1

#include <boost/shared_ptr.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include "async_io.hh"
#include "auto_fd.hh"

namespace vigil {

class Dispatcher;

/*
 * Asynchronous file I/O.
 *
 * This is asynchronous only in the sense that it doesn't block progress of
 * other (cooperative or native) threads.  All operations complete
 * synchronously from the caller's point of view.  (Thus, read_wait and
 * write_wait always wake up the thread immediately.)
 */
class Async_file
    : public Async_stream
{
public:
    Async_file();

    int open(const std::string& file_name, int flags);
    int open(const std::string& file_name, int flags, mode_t mode);
    int close();

    off_t lseek(off_t, int whence = SEEK_SET);
    off_t tell();

    void connect_wait();
    void read_wait();
    void write_wait();
    int get_connect_error();

    ssize_t pread(off_t, Buffer& buffer);
    ssize_t pwrite(off_t, const Buffer& buffer);

    int fstat(struct stat&);
    off_t size();
    int ftruncate(off_t);
    int fchmod(mode_t);
    int fchown(uid_t, gid_t);
    int fsync();
    int fdatasync();

    static int rename(const std::string& old_path, const std::string& new_path);

private:
    Auto_fd fd;

    ssize_t do_read(Buffer&);
    ssize_t do_write(const Buffer&);

    Async_file(const Async_file&);
    Async_file& operator=(const Async_file&);
};

} // namespace vigil

#endif /* async_file.hh */
