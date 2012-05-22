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
#include "tcp-socket.hh"
#include <boost/bind.hpp>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include "buffer.hh"
#include "errno_exception.hh"
#include "netinet++/ipaddr.hh"
#include "socket-util.hh"
#include "threads/cooperative.hh"

#define NOT_REACHED() abort()

namespace vigil {

Tcp_socket::Tcp_socket()
    : fd(::socket(AF_INET, SOCK_STREAM, 0)),
      connect_status(ENOTCONN)
{
    if (fd < 0) {
	throw errno_exception(errno, "socket");
    }

    set_nonblocking(fd);
}

Tcp_socket::Tcp_socket(Auto_fd& fd_)
    : connect_status(0)
{
    set_nonblocking(fd_);
    fd.reset(fd_.release());
}

Tcp_socket::~Tcp_socket()
{
    close();
}

int Tcp_socket::connect(ipaddr ip, uint16_t port, bool block)
{
    co_might_yield_if(block);
    struct sockaddr_in sin = make_sin(ip, port);
    if (block) {
        connect_status = co_connect(fd, (sockaddr*) &sin, sizeof sin);
    } else if (::connect(fd, (sockaddr*) &sin, sizeof sin) < 0) {
        connect_status = errno == EINPROGRESS ? EAGAIN : errno;
    } else {
        connect_status = 0;
    }
    return connect_status;
}

int Tcp_socket::bind(ipaddr ip, uint16_t port)
{
    struct sockaddr_in sin = make_sin(ip, port);
    return ::bind(fd, (sockaddr*) &sin, sizeof sin) < 0 ? errno : 0;
}

int Tcp_socket::listen(int backlog)
{
    return ::listen(fd, backlog) < 0 ? errno : 0;
}

static int
do_getname(int (*getname)(int fd, sockaddr*, socklen_t*), int fd,
           ipaddr* ip, uint16_t* port)
{
    sockaddr_in sin;
    socklen_t sin_len = sizeof sin;
    if (!getname(fd, (sockaddr*) &sin, &sin_len)) {
        if (ip) {
            ip->addr = sin.sin_addr.s_addr;
        }
        if (port) {
            *port = ntohs(sin.sin_port);
        }
        return 0;
    } else {
        if (ip) {
            ip->addr = htons(INADDR_ANY);
        }
        if (port) {
            *port = 0;
        }
        return errno;
    }
}

int Tcp_socket::getsockname(ipaddr* ip, uint16_t* port)
{
    return do_getname(::getsockname, fd, ip, port);

}

int Tcp_socket::getpeername(ipaddr* ip, uint16_t* port)
{
    return do_getname(::getpeername, fd, ip, port);

}

std::auto_ptr<Tcp_socket> Tcp_socket::accept(int& error, bool block)
{
    co_might_yield_if(block);

    Auto_fd new_fd;
    if (block) {
        new_fd.reset(co_accept(fd, NULL, NULL));
        error = new_fd < 0 ? -error : 0;
    } else {
        new_fd.reset(::accept(fd, NULL, NULL));
        error = new_fd < 0 ? errno : 0; 
    }
    return std::auto_ptr<Tcp_socket>(error ? 0 : new Tcp_socket(new_fd));
}

int Tcp_socket::close()
{
    return fd.close();
}

ssize_t Tcp_socket::do_read(Buffer& buffer)
{
    ssize_t retval;
    do {
        retval = ::read(fd, buffer.data(), buffer.size()); 
    } while (retval < 0 && errno == EINTR);
    if (connect_status == EAGAIN) {
        connect_status = retval < 0 ? errno : 0;
    }
    return retval >= 0 ? retval : -errno;
}

ssize_t Tcp_socket::do_write(const Buffer& buffer)
{
    ssize_t retval;
    do {
        retval = ::write(fd, buffer.data(), buffer.size()); 
    } while (retval < 0 && errno == EINTR);
    if (connect_status == EAGAIN) {
        connect_status = retval < 0 ? errno : 0;
    }
    return retval >= 0 ? retval : -errno;;
}

void Tcp_socket::read_wait() 
{
    co_fd_read_wait(fd, NULL);
}

void Tcp_socket::write_wait()
{
    co_fd_write_wait(fd, NULL);
}

void Tcp_socket::accept_wait()
{
    co_fd_read_wait(fd, NULL);
}

void Tcp_socket::connect_wait()
{
    co_fd_write_wait(fd, NULL);
}

int Tcp_socket::get_connect_error()
{
    if (connect_status == EAGAIN) {
        pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLOUT;

        int retval;
        do {
            retval = ::poll(&pfd, 1, 0);
        } while (retval < 0 && errno == EINTR);
        if (retval < 0) {
            NOT_REACHED();
        }

        if (retval && pfd.revents) {
            int error;
            socklen_t len = sizeof(error);
            if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
                NOT_REACHED();
            }
            connect_status = error;
        }
    }
    return connect_status;
}

int Tcp_socket::shutdown(int how)
{
    return ::shutdown(fd, how) < 0 ? errno : 0;
}

uint32_t Tcp_socket::get_local_ip() { 
  struct sockaddr_in addr; 
  socklen_t len = sizeof(addr); 
  int res = ::getsockname(fd.get(),(struct sockaddr*)&addr,&len); 
  if(res)
    return 0; // 0.0.0.0 indicates error
  return (uint32_t) addr.sin_addr.s_addr; 
} 

uint32_t Tcp_socket::get_remote_ip() { 
  struct sockaddr_in addr; 
  socklen_t len = sizeof(addr); 
  int res = ::getpeername(fd.get(),(struct sockaddr*)&addr,&len); 
  if(res)
    return 0; // 0.0.0.0 indicates error
  return (uint32_t) addr.sin_addr.s_addr; 
} 

void Tcp_socket::set_reuseaddr(bool on)
{
    if (setsockopt(SOL_SOCKET, SO_REUSEADDR, on)) {
	/* Should never fail. */
	abort();
    }
}

void Tcp_socket::set_nodelay(bool on)
{
    if (setsockopt(IPPROTO_TCP, TCP_NODELAY, on)) {
	/* Should never fail. */
	abort();
    }
}

/* This is convenient for the options we want to use most frequently.  It would
 * make sense to overload it for other types of values. */
int Tcp_socket::setsockopt(int level, int option, bool value_)
{
    int value = value_;
    return (::setsockopt(fd, level, option, &value, sizeof value) < 0
	    ? errno
	    : 0);
}


} // namespace vigil
