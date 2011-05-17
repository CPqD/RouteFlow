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
#include "async_io.hh"
#include <cerrno>
#include <stdio.h>
#include "buffer.hh"
#include "threads/cooperative.hh"

namespace vigil {

/* Attempts to read data into 'buffer'.  Returns a positive number of bytes
 * read, zero at end of file, or a negative errno value on error.
 *
 * If 'block' is false, returns -EAGAIN immediately if no data can be read;
 * otherwise, blocks until data is available. */
ssize_t
Async_stream::read(Buffer& buffer, bool block)
{
    co_might_yield_if(block);
    for (;;) {
        ssize_t retval = do_read(buffer);
        if (block && retval == -EAGAIN) {
            read_wait();
            co_block();
        } else if (retval != -EINTR) {
            return retval;
        }
    }
}

/* Attempts to write data from 'buffer' into the stream.  Returns a positive
 * number of bytes written or a negative errno value.
 *
 * If 'block' is false, returns -EAGAIN if no data can be accepted for writing
 * immediately; otherwise, blocks until data can be written. */
ssize_t
Async_stream::write(const Buffer& buffer, bool block)
{
    co_might_yield_if(block);
    for (;;) {
        ssize_t retval = do_write(buffer);
        if (block && retval == -EAGAIN) {
            write_wait();
            co_block();
        } else if (retval != -EINTR) {
            return retval;
        }
    }
}

/* Returns 0 if successful, EOF if no bytes were read at end of file, otherwise
 * a positive errno value.  '*bytes_read' indicates how many bytes were read
 * before end-of-file or the error was reached.  If the return value is 0 and
 * '*bytes_read' is less than the size of 'buffer', then end of file was
 * reached after that many bytes.
 *
 * If 'block' is false, indicates error EAGAIN if no data can be read
 * immediately; otherwise, blocks until data can be read. */
int
Async_stream::read_fully(Buffer& buffer_, ssize_t *bytes_read, bool block)
{
    co_might_yield_if(block);

    Nonowning_buffer buffer(buffer_);
    *bytes_read = 0;
    while (buffer.size()) {
        ssize_t n = read(buffer, block);
        if (n <= 0) {
            if (n == 0) {
                return *bytes_read ? 0 : EOF;
            } else {
                return -n;
            }
        }
        assert(n <= buffer.size());
        buffer.pull(n);
        *bytes_read += n;
    }
    return 0;
}

/* Returns 0 if successful, otherwise a positive errno value.  '*bytes_written'
 * indicates how many bytes were written, regardless.  If the return value is
 * 0, then '*bytes_written' is the size of 'buffer'.
 *
 * If 'block' is false, indicates error EAGAIN if no data can be accepted for
 * writing immediately; otherwise, blocks until data can be written. */
int
Async_stream::write_fully(const Buffer& buffer_, ssize_t *bytes_written,
                          bool block)
{
    co_might_yield_if(block);

    Nonowning_buffer buffer(buffer_);
    *bytes_written = 0;
    while (buffer.size()) {
        ssize_t n = write(buffer, block);
        if (n <= 0) {
            return -n;
        }
        assert(n <= buffer.size());
        buffer.pull(n);
        *bytes_written += n;
    }
    return 0;
}

/* Attempts to receive a datagram.  If successful, returns the datagram and
 * sets error to 0; otherwise, returns a null pointer and sets error to a
 * positive errno value.
 *
 * If 'block' is false, indicates error EAGAIN if no datagram can be read
 * immediately; otherwise, blocks until one is received. */
std::auto_ptr<Buffer>
Async_datagram::recv(int& error, bool block)
{
    co_might_yield_if(block);
    for (;;) {
        std::auto_ptr<Buffer> buffer = do_recv(error);
        if (block && error == EAGAIN) {
            recv_wait();
            co_block();
        } else if (error != EINTR) {
            assert((error == 0) == (buffer.get() != NULL));
            return buffer;
        }
    }
}

/* Attempts to send 'buffer' as a datagram.  Returns 0 if successful,
 * otherwise a positive errno value.
 *
 * If 'block' is false, returns EAGAIN if the datagram cannot be accepted to be
 * sent immediately; otherwise, blocks until the datagram can be written. */
int
Async_datagram::send(const Buffer& buffer, bool block)
{
    co_might_yield_if(block);
    for (;;) {
        int error = do_send(buffer);
        if (block && error == EAGAIN) {
            send_wait();
            co_block();
        } else if (error != EINTR) {
            return error;
        }
    }
}
} // namespace vigil
