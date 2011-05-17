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
#ifndef SSL_SOCKET_HH
#define SSL_SOCKET_HH 1

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <openssl/ssl.h>
#include <deque>
#include <string>

#include "async_io.hh"
#include "auto_fd.hh"
#include "buffer.hh"
#include "netinet++/ipaddr.hh"
#include "ssl-config.hh"
#include "ssl-session.hh"

/*
 * SSL socket class.
 */

namespace vigil {

class Ssl_socket
    : public Async_stream
{
public:
    Ssl_socket(boost::shared_ptr<Ssl_config>&);
    ~Ssl_socket();

    int close();

    void read_wait();
    void write_wait();
    void connect_wait();
    void accept_wait();

    int connect(ipaddr ip, uint16_t port, Ssl_session* session, bool block);
    int get_connect_error();
    int bind(ipaddr ip, uint16_t port);
    int listen(int backlog);
    std::auto_ptr<Ssl_socket> accept(int& error, bool block);
    int shutdown();

    Ssl_session *get_session();

    std::string get_peer_cert_fingerprint(); 
    uint32_t get_local_ip(); 
    uint32_t get_remote_ip(); 

private:
    boost::shared_ptr<Ssl_config> config;
    Auto_fd fd;
    SSL *ssl;
    Ssl_config::Session_type type;
    int state;
    Ssl_session* session;
    int connect_error;
    std::string name;

    /* rx_want and tx_want record the result of the last call to ::SSL_read()
     * and ::SSL_write(), respectively:
     *
     *    - If the call reported that data needed to be read from the file
     *      descriptor, the corresponding member is set to SSL_READING.
     *
     *    - If the call reported that data needed to be written to the file
     *      descriptor, the corresponding member is set to SSL_WRITING.
     *
     *    - Otherwise, the member is set to SSL_NOTHING, indicating that the
     *      call completed successfully (or with an error) and that there is no
     *      need to block.
     *
     * These are needed because there is no way to ask OpenSSL what a data read
     * or write would require without giving it a buffer to receive into or
     * data to send, respectively.  The latter is particularly awkward since in
     * write_wait() we have no idea what our client wants to send next.  (Note
     * that the ::SSL_want() status is overwritten by each ::SSL_read() or
     * ::SSL_write() call, so we can't rely on its value.)
     *
     * A single call to ::SSL_read() or ::SSL_write() can perform both reading
     * and writing and thus invalidate not one of these values but actually
     * both.  Consider this situation, for example:
     *
     *    - ::SSL_write() blocks on a read, so tx_want gets SSL_READING.
     *
     *    - ::SSL_read() laters succeeds reading from 'fd' and clears out the
     *      whole receive buffer, so rx_want gets SSL_READING.
     *
     *    - Client calls read_wait() and write_wait() and blocks.
     *
     *    - Now we're stuck blocking until the peer sends us data, even though
     *      ::SSL_write() could now succeed, which could easily be a deadlock
     *      condition.
     *
     * On the other hand, we can't reset both tx_want and rx_want on every call
     * to ::SSL_read() or ::SSL_write(), because that would produce livelock,
     * e.g. in this situation:
     *
     *    - ::SSL_write() blocks, so tx_want gets SSL_READING or SSL_WRITING.
     *
     *    - ::SSL_read() blocks, so rx_want gets SSL_READING or SSL_WRITING,
     *      but tx_want gets reset to SSL_NOTHING.
     *
     *    - Client calls read_wait() and write_wait() and blocks.
     *
     *    - Client wakes up immediately since SSL_NOTHING in tx_want indicates
     *      that no blocking is necessary.
     *
     * The solution we adopt here is to set tx_want to SSL_NOTHING after
     * calling ::SSL_read() only if the SSL state of the connection changed,
     * which indicates that an SSL-level renegotiation made some progress, and
     * similarly for rx_want and ::SSL_write().  This prevents both the
     * deadlock and livelock situations above.
     */
    int rx_want, tx_want;

    Ssl_socket(boost::shared_ptr<Ssl_config>&, Auto_fd& fd, sockaddr_in);

    ssize_t do_read(Buffer&);
    ssize_t do_write(const Buffer&);

    void state_machine();
    bool finish_connecting();
    bool wait_to_finish_connecting();
    void ssl_wait(int want);
    void connect_completed(int error);
    bool set_ssl();

    Ssl_socket();
    Ssl_socket(const Ssl_socket&);
    Ssl_socket& operator=(const Ssl_socket&);
};

} // namespace vigil

#endif /* ssl-socket.hh */
