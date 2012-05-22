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
#ifndef TCP_SOCKET_HH
#define TCP_SOCKET_HH 1

#include "async_io.hh"
#include "auto_fd.hh"
#include "netinet++/ipaddr.hh"

namespace vigil {

class Tcp_socket
    : public Async_stream
{
public:
    explicit Tcp_socket();
    explicit Tcp_socket(Auto_fd& fd_);
    ~Tcp_socket();

    int close();

    void set_reuseaddr(bool on = true);
    void set_nodelay(bool on = true);

    void read_wait();
    void write_wait();
    void connect_wait();
    void accept_wait();

    int connect(ipaddr ip, uint16_t port, bool block);
    int get_connect_error();
    int bind(ipaddr ip, uint16_t port);
    int getsockname(ipaddr* ip, uint16_t* port);
    int getpeername(ipaddr* ip, uint16_t* port);
    int listen(int backlog);
    std::auto_ptr<Tcp_socket> accept(int& error, bool block);
    int shutdown(int how);
    uint32_t get_local_ip(); 
    uint32_t get_remote_ip(); 

private:
    Auto_fd fd;
    int connect_status;

    ssize_t do_read(Buffer&);
    ssize_t do_write(const Buffer&);

    int setsockopt(int level, int option, bool value);

    Tcp_socket(const Tcp_socket&);
    Tcp_socket& operator=(const Tcp_socket&);
};

} // namespace vigil

#endif	/* tcp-socket.hh */
