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
/*
 * Asynchronous file I/O implementation.
 *
 * We do asynchronous I/O by migrating ourselves to a native thread, doing the
 * I/O, and then migrating back.
 */

#include "async_file.hh"
#include <cerrno>
#include <fcntl.h>
#include <string>
#include "buffer.hh"
#include "threads/cooperative.hh"

namespace vigil {

/* Constructs an Async_file that is not associated with any file.  Use .open()
 * to open a file. */
Async_file::Async_file()
  : fd()
{
}

/* Opens a new file via ::open(file_name, flags).  Returns 0 if successful,
 * otherwise a system error code representing the error.  Any formerly open
 * file is closed. */
int Async_file::open(const std::string& file_name, int flags)
{
    assert(!(flags & O_CREAT));
    return open(file_name, flags, 0);
}

/* Opens a new file via ::open(file_name, flags, mode).  Returns 0 if
 * successful, otherwise a system error code representing the error.  Any
 * formerly open file is closed. */
int Async_file::open(const std::string& file_name, int flags, mode_t mode)
{
    fd.reset(::open(file_name.c_str(), flags, mode));
    return fd < 0 ? errno : 0;
}

/* Closes the file (if one is open). */
int Async_file::close()
{
    fd.reset();
    return 0;
}

/* Seeks as would ::lseek() with similar arguments.  Returns the new file
 * offset if successful, otherwise a negative system error code.  */
off_t Async_file::lseek(off_t offset, int whence)
{
    return ::lseek(fd, offset, whence);
}

/* Returns the current file offset. */
off_t Async_file::tell()
{
    return ::lseek(fd, SEEK_CUR, 0);
}

ssize_t Async_file::do_read(Buffer& buffer)
{
    return co_async_read(fd, buffer.data(), buffer.size());
}

ssize_t Async_file::do_write(const Buffer& buffer)
{
    return co_async_write(fd, buffer.data(), buffer.size());
}

void Async_file::connect_wait()
{
    co_immediate_wake(1, NULL);
}

void Async_file::read_wait()
{
    co_immediate_wake(1, NULL);
}

void Async_file::write_wait()
{
    co_immediate_wake(1, NULL);
}

int Async_file::get_connect_error()
{
    return 0;
}

ssize_t Async_file::pread(off_t offset, Buffer& buffer)
{
    return co_async_pread(fd, buffer.data(), buffer.size(), offset);
}

ssize_t Async_file::pwrite(off_t offset, const Buffer& buffer)
{
    return co_async_pwrite(fd, buffer.data(), buffer.size(), offset);
}

/* Calls ::fstat() for the file.  Stores the stat information in 's' and
 * returns 0 if successful.  Otherwise returns a system error code without
 * modifying 's'. */
int Async_file::fstat(struct stat& s)
{
    return ::fstat(fd, &s) < 0 ? errno : 0;
}

/* Returns the size of the file in bytes, if successful, otherwise a negative
 * system error code. */
off_t Async_file::size()
{
    struct stat s;
    int error = fstat(s);
    return error ? -error : s.st_size;
}

/* Extends or shortens the file to exactly 'size' bytes in length.  Returns 0
 * if successful, otherwise a system error code. */
int Async_file::ftruncate(off_t size)
{
    return co_async_ftruncate(fd, size);
}

/* Changes the file's permissions to 'mode.  Returns 0 if successful, otherwise
 * a system error code. */
int Async_file::fchmod(mode_t mode)
{
    return co_async_fchmod(fd, mode);
}

/* Changes the file's owner to 'owner' and its group to 'group'.  Returns 0 if
 * successful, otherwise a system error code. */
int Async_file::fchown(uid_t owner, gid_t group)
{
    return co_async_fchown(fd, owner, group);
}

/* Writes the file's data and inode to disk. */
int Async_file::fsync()
{
    return co_async_fsync(fd);
}

/* Writes the file's data to disk. */
int Async_file::fdatasync()
{
    return co_async_fdatasync(fd);
}

/* Change the name or location of a file. */
int Async_file::rename(const std::string& old_path, const std::string& new_path) {
    return co_async_rename(old_path.c_str(), new_path.c_str());
}

} // namespace vigil
