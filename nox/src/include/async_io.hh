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
#ifndef ASYNC_IO_HH
#define ASYNC_IO_HH 1

#include <memory>
#include <unistd.h>

namespace vigil {

class Buffer;

class Async_io
{
public:
    virtual ~Async_io() { }
    virtual int close() = 0;
protected:
    Async_io() { }
};

class Async_stream
    : public Async_io
{
public:
    virtual ~Async_stream() { }
    ssize_t read(Buffer&, bool block);
    ssize_t write(const Buffer&, bool block);
    int read_fully(Buffer&, ssize_t *bytes_read, bool block);
    int write_fully(const Buffer&, ssize_t *bytes_written, bool block);
    virtual void read_wait() = 0;
    virtual void write_wait() = 0;
    virtual void connect_wait() = 0;
    virtual int get_connect_error() = 0;
protected:
    virtual ssize_t do_read(Buffer&) = 0;
    virtual ssize_t do_write(const Buffer&) = 0;
};

class Async_datagram
    : public Async_io
{
public:
    std::auto_ptr<Buffer> recv(int& error, bool block);
    int send(const Buffer&, bool block);
    virtual void recv_wait() = 0;
    virtual void send_wait() = 0;

protected:
    virtual std::auto_ptr<Buffer> do_recv(int& error) = 0;
    virtual int do_send(const Buffer&) = 0;
};

} // namespace vigil

#endif /* async_io.hh */
