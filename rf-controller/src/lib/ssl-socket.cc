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
#include "ssl-socket.hh"
#include <boost/bind.hpp>
#include <openssl/err.h>
#include <openssl/evp.h> 
#include <sstream> 
#include <poll.h>
#include <sys/socket.h>
#include "errno_exception.hh"
#include "packets.h"
#include "socket-util.hh"
#include "string.hh"
#include "threads/cooperative.hh"
#include "vlog.hh"


#define NOT_REACHED() abort()

namespace vigil {

static Vlog_module log("ssl");

/*
 * Returns true is OpenSSL error is WANT_READ or WANT_WRITE.
 */

static bool
ssl_wants_io(int ssl_error)
{
    return (ssl_error == SSL_ERROR_WANT_WRITE
            || ssl_error == SSL_ERROR_WANT_READ);
}

enum { 
    STATE_NOT_CONNECTED,
    STATE_TCP_ACCEPTING,
    STATE_TCP_CONNECTING,
    STATE_TCP_CONNECTED,
    STATE_SSL_CONNECTING,
    STATE_CONNECTED
};



/*
 * Constructs an Ssl_socket with an Ssl_config object defining
 * the certificates, key etc. to be used by the socket.
 */

Ssl_socket::Ssl_socket(boost::shared_ptr<Ssl_config>& config_)
    : config(config_),
      fd(::socket(AF_INET, SOCK_STREAM, 0)),
      ssl(NULL),
      type(Ssl_config::CLIENT),
      state(STATE_NOT_CONNECTED),
      session(NULL),
      connect_error(0),
      name("unconnected")
{
    if (fd < 0) {
        throw errno_exception(errno, "socket");
    }
    unsigned int yes = 1;
    if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes, sizeof(yes)) < 0) { 
        // non-fatal error
        log.err("Failed to set sockopt SO_REUSEADDR\n");
    } 

    set_nonblocking(fd);
}


/*
 * Constructs an Ssl_socket from an allocated file descriptor (used on fd
 * returned by accept() call).
 */

Ssl_socket::Ssl_socket(boost::shared_ptr<Ssl_config>& config_, Auto_fd& fd_,
                       sockaddr_in sin)
    : config(config_),
      ssl(NULL),
      type(Ssl_config::SERVER),
      state(STATE_TCP_CONNECTED),
      session(NULL),
      connect_error(0),
      name(string_format(IP_FMT":%"PRIu16,
                         IP_ARGS(&sin.sin_addr.s_addr), ntohs(sin.sin_port))),
      rx_want(SSL_NOTHING),
      tx_want(SSL_NOTHING)
{
    set_nonblocking(fd_);
    fd.reset(fd_.release());
    state_machine();
}

/*
 * Returns a pointer to the session used by the current socket, which can be
 * used to reconnect to a peer without requiring a rehandshake.  Will return
 * NULL if a session cannot be retrieved.
 *
 * Once an Ssl_session has been obtained through this method, it remains valid
 * even after the Ssl_socket has been destructed, and can be use by a client
 * socket attempting to a connect to a server using Ssl_socket::reconnect().
 * The returned Ssl_session object is dynamically allocated, meaning it is the
 * caller's responsibility to free it when finished.  As soon as a socket has
 * been reconnected, the caller can delete their Ssl_session instance (unless
 * they want to preserve it for future use).
 *
 * Server sockets do not need to do anything special to reuse sessions because
 * it is the client's responsibilty to tell the server the ID of the session to
 * resume.  A server then just looks up the ID in the session cache (all taken
 * care of by the OpenSSL library) and resumes it if possible.  In other words,
 * the Ssl_session object is at a high level just an ID holder a client needs
 * to resume a session.
 *
 * NOTE: Caller should only reconnect sessions on sockets using the same
 * Ssl_config objects.
 */

Ssl_session *
Ssl_socket::get_session()
{
    if (ssl == NULL) {
        return NULL;
    }

    SSL_SESSION *session = SSL_get1_session(ssl);
    if (session == NULL) {
        return NULL;
    }

    Ssl_session *wrapper;

    try {
        wrapper = new Ssl_session(session);
    } catch (...) {
        SSL_SESSION_free(session);
        throw;
    }

    return wrapper;
}

void
Ssl_socket::connect_completed(int error) 
{
    state = STATE_CONNECTED;
    connect_error = error;
}

uint32_t Ssl_socket::get_local_ip() { 
  struct sockaddr_in addr; 
  socklen_t len = sizeof(addr); 
  int res = ::getsockname(fd.get(),(struct sockaddr*)&addr,&len); 
  if(res)
    return 0; // 0.0.0.0 indicates error
  return (uint32_t) addr.sin_addr.s_addr; 
} 

uint32_t Ssl_socket::get_remote_ip() { 
  struct sockaddr_in addr; 
  socklen_t len = sizeof(addr); 
  int res = ::getpeername(fd.get(),(struct sockaddr*)&addr,&len); 
  if(res)
    return 0; // 0.0.0.0 indicates error
  return (uint32_t) addr.sin_addr.s_addr; 
} 

std::string Ssl_socket::get_peer_cert_fingerprint() { 
  unsigned char buf[EVP_MAX_MD_SIZE]; 
  unsigned int len = 0; 
  X509 *cert = (X509*)SSL_get_peer_certificate(ssl);
  if(!cert) { 
    log.err("Could not retrieve peer certificate to create fingerprint\n"); 
    return ""; 
  }   
  if(!X509_digest(cert,EVP_sha1(),buf, &len)) {  
    log.err("Error calculating cert digest\n");
    return ""; 
  } 
  std::stringstream ss; 
  for(int i=0; i < (int)len; i++) { 
    char tmp[3]; 
    snprintf(tmp,3,"%02x",buf[i]); 
    ss << tmp; 
  } 
  X509_free(cert); // decrement reference count
  return ss.str(); 
} 

static void
interpret_ssl_error(const char *name, const char *function, int ret, int error)
{
    int queued_error = ERR_get_error();

    switch (error) {
    case SSL_ERROR_NONE:
        VLOG_ERR(log, "%s: unexpected SSL_ERROR_NONE from %s", name, function);
        break;

    case SSL_ERROR_ZERO_RETURN:
        VLOG_ERR(log, "%s: unexpected SSL_ERROR_ZERO_RETURN from %s",
                 name, function);
        break;

    case SSL_ERROR_WANT_READ:
        VLOG_ERR(log, "%s: unexpected SSL_ERROR_WANT_READ from %s",
                 name, function);
        break;

    case SSL_ERROR_WANT_WRITE:
        VLOG_ERR(log, "%s: unexpected SSL_ERROR_WANT_WRITE from %s",
                 name, function);
        break;

    case SSL_ERROR_WANT_CONNECT:
        VLOG_ERR(log, "%s: unexpected SSL_ERROR_WANT_CONNECT from %s",
                 name, function);
        break;

    case SSL_ERROR_WANT_ACCEPT:
        VLOG_ERR(log, "%s: unexpected SSL_ERROR_WANT_ACCEPT from %s",
                 name, function);
        break;

    case SSL_ERROR_WANT_X509_LOOKUP:
        VLOG_ERR(log, "%s: unexpected SSL_ERROR_WANT_X509_LOOKUP from %s",
                 name, function);
        break;

    case SSL_ERROR_SYSCALL:
        if (!queued_error) {
            if (ret < 0) {
                int status = errno;
                VLOG_WARN(log, "%s: system error (%s) from %s",
                          name, strerror(status), function);
            } else {
                VLOG_WARN(log, "%s: unexpected SSL connection close from %s",
                          name, function);
            }
        } else {
            VLOG_WARN(log, "%s: %s from %s",
                      name, ERR_error_string(queued_error, NULL), function);
        }
        break;

    case SSL_ERROR_SSL:
        if (queued_error) {
            VLOG_WARN(log, "%s: %s from %s",
                      name, ERR_error_string(queued_error, NULL), function);
        } else {
            VLOG_ERR(log, "%s: SSL_ERROR_SSL without queued error from %s",
                     name, function);
        }
        break;

    default:
        VLOG_ERR(log, "%s: bad SSL error code %d from %s",
                 name, error, function);
        break;
    }
}

void
Ssl_socket::state_machine()
{
    switch (state) {
    case STATE_NOT_CONNECTED:
    case STATE_TCP_ACCEPTING:
    case STATE_CONNECTED:
        return;
        
    case STATE_TCP_CONNECTING: {
        /* Check whether the connection completed. */
        pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLOUT;
        if (poll(&pfd, 1, 0) <= 0) {
            return;
        }

        /* Check how it completed. */
        int error;
        socklen_t len = sizeof connect_error;
        if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
            error = errno;
            log.warn("%s: getsockopt(SO_ERROR) failed: %s",
                     name.c_str(), strerror(error));
            connect_completed(error);
            return;
        }
        if (error) {
            log.warn("%s: connect failed: %s", name.c_str(), strerror(error));
            connect_completed(error);
            return;
        }
        state = STATE_TCP_CONNECTED;
        /* Fall through. */
    }
    
    case STATE_TCP_CONNECTED: {
        if (!set_ssl()) {
            log.warn("%s: set_ssl() failed", name.c_str());
            connect_completed(ENOPROTOOPT);
            return;
        }
        if (session) {
            int ret = SSL_set_session(ssl, session->session);
            if (ret != 1) {
                interpret_ssl_error(name.c_str(), "SSL_set_session",
                                    ret, SSL_get_error(ssl, ret));
                connect_completed(ENOPROTOOPT);
                return;
            }
        }
        state = STATE_SSL_CONNECTING;
        /* Fall through. */
    }

    case STATE_SSL_CONNECTING: {
        int ret = (type == Ssl_config::CLIENT ? ::SSL_connect(ssl)
                   : ::SSL_accept(ssl));
        if (ret != 1) {
            int ssl_error = ::SSL_get_error(ssl, ret);
            if (ret < 0 && ssl_wants_io(ssl_error)) {
                /* Stay in this state to repeat the SSL_connect later. */
                return;
            } else {
                if (SSL_get_verify_result(ssl) != X509_V_OK){
                    log.err("switch provided invalid X509 certificate");
                }
                interpret_ssl_error(name.c_str(),
                                    (type == Ssl_config::CLIENT
                                     ? "SSL_connect" : "SSL_accept"),
                                    ret, ssl_error);
                ::shutdown(fd, SHUT_RDWR);
                connect_completed(EPROTO);
                return;
            }
        }
        connect_completed(0);
        return;
    }

    default:
        NOT_REACHED();
    }
}

bool
Ssl_socket::finish_connecting() 
{
    state_machine();
    return state == STATE_CONNECTED;
}

/*
 * Helper function for connect() and reconnect() in which 'session' is either
 * NULL or points to an OpenSSL SSL_SESSION struct.
 */
int
Ssl_socket::connect(ipaddr ip, uint16_t port, Ssl_session *session_,
                    bool block)
{
    co_might_yield_if(block);

    session = session_;

    struct sockaddr_in sin = make_sin(ip, port);
    if (!::connect(fd, (sockaddr*) &sin, sizeof sin)) {
        state = STATE_TCP_CONNECTED;
    } else if (errno == EINPROGRESS) {
        state = STATE_TCP_CONNECTING;
    } else {
        state = STATE_CONNECTED;
        connect_error = errno;
    }
    name = string_format(IP_FMT":%"PRIu16, IP_ARGS(&sin.sin_addr.s_addr),
                         ntohs(sin.sin_port));

    while (block && get_connect_error() == EAGAIN) {
        co_might_yield();
        connect_wait();
        co_block();
    }

    int ret = get_connect_error();
    if (ret && ret != EAGAIN) {
        log.warn("%s: connection failed: %s", name.c_str(), strerror(ret));
    }
    return ret;
}

bool
Ssl_socket::wait_to_finish_connecting()
{
    state_machine();
    
    switch (state) {
    case STATE_NOT_CONNECTED:
        co_immediate_wake(1, NULL);
        break;

    case STATE_TCP_ACCEPTING:
        co_fd_read_wait(fd, NULL);
        break;
        
    case STATE_TCP_CONNECTING:
        co_fd_write_wait(fd, NULL);
        break;
        
    case STATE_TCP_CONNECTED:
        /* Not reached: state_machine() should have transitioned us away to
         * another state. */
        NOT_REACHED();

    case STATE_SSL_CONNECTING:
        /* state_machine() called SSL_accept() or SSL_connect(), which set up
         * the status that we test here. */
        if (SSL_want_read(ssl)) {
            co_fd_read_wait(fd, NULL);
        } else if (SSL_want_write(ssl)) {
            co_fd_write_wait(fd, NULL);
        } else {
            NOT_REACHED();
        }
        break;

    case STATE_CONNECTED:
        return false;

    default:
        NOT_REACHED();
    }
    
    return true;
}

void
Ssl_socket::ssl_wait(int want)
{
    switch (want) {
    case SSL_NOTHING:
        co_immediate_wake(1, NULL);
        break;

    case SSL_READING:
        co_fd_read_wait(fd, NULL);
        break;

    case SSL_WRITING:
        co_fd_write_wait(fd, NULL);
        break;

    default:
        NOT_REACHED();
    }
}

void
Ssl_socket::read_wait()
{
    if (!wait_to_finish_connecting()) {
        ssl_wait(rx_want);
    }
}

void
Ssl_socket::write_wait() 
{
    if (!wait_to_finish_connecting()) {
        ssl_wait(tx_want);
    }
}

void
Ssl_socket::connect_wait() 
{
    if (!wait_to_finish_connecting()) {
        co_immediate_wake(1, NULL);
    }
}

void
Ssl_socket::accept_wait() 
{
    co_fd_read_wait(fd, NULL);
}

int
Ssl_socket::get_connect_error() 
{
    state_machine();
    return (state == STATE_NOT_CONNECTED ? ENOTCONN
            : state == STATE_CONNECTED ? connect_error
            : EAGAIN);
}

/*
 * Binds socket to the passed in 'ip' and 'port' where 'port' is in network
 * byte order.  Returns the Linux errno set if ::bind fails, else returns 0.
 */

int
Ssl_socket::bind(ipaddr ip, uint16_t port)
{
    struct sockaddr_in sin = make_sin(ip, port);
    if (!::bind(fd, (sockaddr*) &sin, sizeof sin)) {
        return 0;
    } else {
        log.warn("failed to bind to "IP_FMT":%"PRIu16": %s",
                 IP_ARGS(&sin.sin_addr), ntohs(sin.sin_port),
                 strerror(errno));
        return errno;
    }
}


/*
 * Sets up socket for listening with a maximum 'backlog' of unaccepted
 * connections.  Returns the Linux errno set if listen fails, else 0.
 */

int
Ssl_socket::listen(int backlog)
{
    int retval = ::listen(fd, backlog);

    /* Make a string name of the address we're bound to. */
    sockaddr_in sin;
    socklen_t sin_len = sizeof sin;
    std::string bind_name =
        (!::getsockname(fd, (sockaddr *) &sin, &sin_len)
         ? string_format(IP_FMT":%"PRIu16,
                         IP_ARGS(&sin.sin_addr.s_addr), ntohs(sin.sin_port))
         : "unbound");

    if (!retval) {
        state = STATE_TCP_ACCEPTING;
        name = "listen:" + bind_name;
        log.dbg("listening on %s", bind_name.c_str());
        return 0;
    } else {
        log.warn("failed to listen to %s: %s",
                 bind_name.c_str(), strerror(errno));
        return errno;
    }
}

/*
 * Asynchronously accepts a connection, calling 'callback' with an error
 * signaling if a connection was successfully accepted, and a pointer to the
 * accepted connection's socket.  If error is '0', a connection was
 * successfully accepted and the passed along Ssl_socket * can be used for
 * communication.  Else error can be either an errno set by a failed Linux
 * ::accept call, or 'EPROTO' if the SSL handshake failed.  If error is not
 * '0' and the returned socket is not NULL, the returned socket needs to be
 * deleted as it was dynamically allocated by this method.
 */

std::auto_ptr<Ssl_socket>
Ssl_socket::accept(int& error, bool block)
{
    co_might_yield_if(block);

    sockaddr_in sin;
    socklen_t sin_len = sizeof sin;
    Auto_fd new_fd;
    if (block) {
        new_fd.reset(co_accept(fd, (sockaddr *) &sin, &sin_len));
    } else {
        new_fd.reset(::accept(fd, (sockaddr *) &sin, &sin_len));
    }

    std::auto_ptr<Ssl_socket> sock;
    if (new_fd < 0) {
        error = block ? -new_fd : errno;
    } else {
        error = 0;
        sock.reset(new Ssl_socket(config, new_fd, sin));
    }
    if (error && error != EAGAIN) {
        log.warn("%s: accept failed: %s", name.c_str(), strerror(error));
    }
    return sock;
}

/*
 * Asynchronously shutdowns the socket's SSL connection, calling 'callback'
 * when the shutdown has either successefully completed or definitively failed.
 * 'callback' is called with '0' if the shutdown was successful, else with
 * EPROTO.  Pending reads and writes are called back with
 * ECANCELED.
 *
 * Performs a uni-directional shutdown that will callback immediately after the
 * local host has sent a 'close notify', not waiting for the remote host to
 * reciprocate (which the RFC defines as acceptable behavior).
 */

int
Ssl_socket::shutdown()
{
    if (ssl == NULL || (::SSL_get_shutdown(ssl) & SSL_SENT_SHUTDOWN) != 0) {
        ::shutdown(fd, SHUT_RDWR);
        return 0;
    }

    int ret = ::SSL_shutdown(ssl);
    if (ret >= 0) {
        ::shutdown(fd, SHUT_RDWR);
        return 0;
    } else if (ret == -1) {
        interpret_ssl_error(name.c_str(), "SSL_shutdown", ret,
                            ::SSL_get_error(ssl, ret));
        ::shutdown(fd, SHUT_RDWR);
        return EPROTO;
    } else {
        return ssl_wants_io(::SSL_get_error(ssl, ret)) ? EAGAIN : EPROTO;
    }

}

Ssl_socket::~Ssl_socket()
{
    close();
}

/*
 * Frees the socket's SSL object, closes the socket, and frees any pending
 * reads and writes (i.e. their callbacks will not get called).
 */

int
Ssl_socket::close()
{
    if (ssl != NULL) {
        if ((::SSL_get_shutdown(ssl) & SSL_SENT_SHUTDOWN) == 0) {
            ::SSL_shutdown(ssl);
        }
        ::SSL_free(ssl);
        ssl = NULL;
    }

    /* Ensure that later reads and writes fail without dereferencing 'ssl'. */
    state = STATE_CONNECTED;
    connect_error = EBADF;

    int fd_close_res = fd.close(); 
    return fd_close_res;
}


ssize_t
Ssl_socket::do_write(const Buffer& buffer)
{
    if (!finish_connecting()) {
        return state == STATE_NOT_CONNECTED ? -ECONNRESET : -EAGAIN;
    } else if (connect_error) {
        return -connect_error;
    }

    if (buffer.size() == 0) {
        /* Behavior of zero-byte SSL_write is undefined. */
        return 0;
    }

    int old_state = ::SSL_get_state(ssl);
    int ret = ::SSL_write(ssl, buffer.data(), buffer.size());
    if (old_state != ::SSL_get_state(ssl)) {
        rx_want = SSL_NOTHING;
    }
    tx_want = SSL_NOTHING;

    if (ret > 0) {
        return ret;
    } else {
        int error = ::SSL_get_error(ssl, ret);
        if (error == SSL_ERROR_ZERO_RETURN) {
            /* Connection closed (EOF). */
            return -EPIPE;
        } else if (ssl_wants_io(error)) {
            tx_want = SSL_want(ssl);
            return -EAGAIN;
        } else {
            interpret_ssl_error(name.c_str(), "SSL_write", ret, error);
            return -EPROTO;
        }
    }
}

ssize_t
Ssl_socket::do_read(Buffer& buffer)
{
    if (!finish_connecting()) {
        return state == STATE_NOT_CONNECTED ? -ENOTCONN : -EAGAIN; 
    } else if (connect_error) {
        return -connect_error;
    }

    if (buffer.size() == 0) {
        /* Behavior of zero-byte SSL_read is poorly defined. */
        return 0;
    }

    int old_state = ::SSL_get_state(ssl);
    int ret = ::SSL_read(ssl, buffer.data(), buffer.size());
    if (old_state != ::SSL_get_state(ssl)) {
        tx_want = SSL_NOTHING;
    }
    rx_want = SSL_NOTHING;

    if (ret > 0) {
        return ret;
    } else {
        int error = ::SSL_get_error(ssl, ret);
        if (error == SSL_ERROR_ZERO_RETURN) {
            /* Connection closed (EOF). */
            return 0;
        } else if (ssl_wants_io(error)) {
            rx_want = ::SSL_want(ssl);
            return -EAGAIN;
        } else {
            interpret_ssl_error(name.c_str(), "SSL_read", ret, error);
            return -EPROTO;
        }
    }
}

bool
Ssl_socket::set_ssl()
{
    if (ssl != NULL) {
        if ((::SSL_get_shutdown(ssl) & SSL_SENT_SHUTDOWN) == 0) {
            ::SSL_shutdown(ssl);
        }
        ::SSL_free(ssl);
    }

    ssl = config->new_ssl(type);
    if (ssl == NULL) {
        ::shutdown(fd, SHUT_RDWR);
        return false;
    }
    ::SSL_set_mode(ssl, SSL_MODE_ENABLE_PARTIAL_WRITE);
    ::SSL_set_mode(ssl, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

    if (::SSL_set_fd(ssl, fd) == 0) {
        ::SSL_free(ssl);
        ssl = NULL;
        ::shutdown(fd, SHUT_RDWR);
        return false;
    }

    return true;
}


} // namespace vigil
