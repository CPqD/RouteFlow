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
#include <boost/bind.hpp>
#include <boost/test/unit_test.hpp>
#include <errno.h>
#include <string>

#include "errno_exception.hh"
#include "ssl-socket.hh"
#include "ssl-session.hh"
#include "timeval.hh"
#include "tests/ssl-test-str.hh"
#include "threads/cooperative.hh"
#include "vlog.hh"

using namespace vigil;
using namespace std;
using namespace boost::unit_test;

namespace {

Vlog_module lg("ssl-test");

Ssl_session* session = NULL;

class Ssl_echo_server
    : Co_thread
{
public:
    Ssl_echo_server(boost::shared_ptr<vigil::Ssl_config>& config);
    ~Ssl_echo_server();

private:
    Ssl_socket socket;

    void run();
};

class Ssl_server_conn
    : Co_thread
{
public:
    Ssl_server_conn(std::auto_ptr<Ssl_socket> socket_);
    ~Ssl_server_conn();

    void run();
    
private:
    std::auto_ptr<Ssl_socket> socket;
};

Ssl_echo_server::Ssl_echo_server(boost::shared_ptr<Ssl_config>& config)
    : socket(config)
{
    int error = socket.bind(INADDR_ANY, htons(1234));
    BOOST_REQUIRE_MESSAGE(error == 0, "Unable to bind server port");

    error = socket.listen(10);
    BOOST_REQUIRE_MESSAGE(error == 0, "Unable to listen server socket.");
    
    start(boost::bind(&Ssl_echo_server::run, this));
}

Ssl_echo_server::~Ssl_echo_server()
{
    int error = socket.close();
    BOOST_REQUIRE_MESSAGE(error == 0, "Unable to close the server socket.");
}

void
Ssl_echo_server::run()
{
    int error;
    while (true) {
        socket.accept_wait();
        co_block();
        std::auto_ptr<Ssl_socket> new_socket = socket.accept(error, true);
        new Ssl_server_conn(new_socket);
        // Ignore the memory leak.
    }
    if (error != EAGAIN) {
        lg.err("SSL server accept: %d", error);
    }

    // TODO: Implement shutdown...
}

Ssl_server_conn::Ssl_server_conn(std::auto_ptr<Ssl_socket> socket_)
    : socket(socket_)
{
    start(boost::bind(&Ssl_server_conn::run, this));
}

void
Ssl_server_conn::run()
{
    Array_buffer buffer(1);

    for (;;) {
        ssize_t bytes_read;
        int error = socket->read_fully(buffer, &bytes_read, true);
        if (error) {
            if (error != EOF) {
                lg.err("SSL server read: %d", error);
            }
            return;
        }
        buffer.trim(bytes_read);

        ssize_t bytes_written;
        error = socket->write_fully(buffer, &bytes_written, true);
        if (error) {
            if (error != EPIPE) {
                error = socket->shutdown(); 
            }
            if (error) {
                lg.err("SSL server connection shutdown: %d", error);
            }
            return;
        }
    }
}

Ssl_server_conn::~Ssl_server_conn()
{
    int error = socket->close();
    if (error) {
        lg.err("SSL server connection close: %d", error);
    }
}

static void
connect_as_client(boost::shared_ptr<Ssl_config>& config, const char* str) 
{
    Ssl_socket socket(config);

    int error = socket.connect(vigil::ipaddr("127.0.0.1"), htons(1234), 
                               session, true);
    BOOST_REQUIRE_MESSAGE(error == 0, "Unable to connect to 127.0.0.1");
    
    Nonowning_buffer out(str, strlen(str));
    ssize_t bytes_written;
    error = socket.write_fully(out, &bytes_written, true);
    if (error) {
        if (error != EPIPE) {
            error = socket.shutdown();
            BOOST_REQUIRE_MESSAGE(error == 0, "Error, but can't shutdown the "
                                  "client socket.");
        }

        BOOST_REQUIRE_MESSAGE(error == 0, "Unable to shutdown?");
    }

    Array_buffer in(strlen(str));
    ssize_t bytes_read;
    error = socket.read_fully(in, &bytes_read, true);
    if (error) {
        BOOST_REQUIRE_MESSAGE(error == EOF, "Unable to properly read.");
        return;
    } else if (bytes_read < bytes_written) {
        BOOST_REQUIRE_MESSAGE(error == EOF, "Read too little.");
        return;
    }

    if (!session) {
        session = socket.get_session();
        BOOST_REQUIRE_MESSAGE(session != 0, "Can't find the SSL session.");
    } else {
        delete session;
        session = 0;
    }
    error = socket.shutdown();
    BOOST_REQUIRE_MESSAGE(error == 0, "Unable to shutdown the client socket.");
}

BOOST_AUTO_TEST_CASE(SSLTest) {
    Ssl_echo_server* server;

    boost::shared_ptr<Ssl_config> config(
        new Ssl_config(Ssl_config::ALL_VERSIONS,
                       Ssl_config::AUTHENTICATE_SERVER,
                       Ssl_config::REQUIRE_CLIENT_CERT,
                       "nox/netapps/tests/serverkey.pem",
                       "nox/netapps/tests/servercert.pem",
                       "nox/netapps/tests/cacert.pem"));
    
    server = new Ssl_echo_server(config);

    BOOST_REQUIRE_MESSAGE(config, "Unable to read the config.");
    BOOST_REQUIRE_MESSAGE(server, "Unable to construct server.");

    connect_as_client(config, test_str);        
    connect_as_client(config, test_str);
    
    //delete server;
    //delete config;
}
}
