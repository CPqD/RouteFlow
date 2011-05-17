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


#include "ssl-test.hh"

#include <boost/bind.hpp>
#include <errno.h>
#include <string>

#include "testharness/testdefs.hh"

#include "errno_exception.hh"
#include "ssl-socket.hh"
#include "ssl-session.hh"
#include "timeval.hh"
#include "ssl-test-str.hh"
#include "threads/cooperative.hh"
#include "vlog.hh"

#include "bootstrap-complete.hh"


using namespace vigil;
using namespace std;


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
    NOX_TEST_ASSERT(error == 0, "Bind to server port");

    error = socket.listen(10);
    NOX_TEST_ASSERT(error == 0, "Listen to server socket.");
    
    start(boost::bind(&Ssl_echo_server::run, this));
}

Ssl_echo_server::~Ssl_echo_server()
{
    int error = socket.close();
    NOX_TEST_ASSERT(error == 0, "Close the server socket.");
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
    NOX_TEST_ASSERT(error == 0, "Connect to 127.0.0.1");
    
    Nonowning_buffer out(str, strlen(str));
    ssize_t bytes_written;
    error = socket.write_fully(out, &bytes_written, true);
    if (error) {
        if (error != EPIPE) {
            error = socket.shutdown();
            NOX_TEST_ASSERT(error == 0, "Shutdown client socket.");
        }

        NOX_TEST_ASSERT(error == 0, "Shutdown?");
    }

    Array_buffer in(strlen(str));
    ssize_t bytes_read;
    error = socket.read_fully(in, &bytes_read, true);
    if (error) {
        NOX_TEST_ASSERT(error == EOF, "Unable to read.");
        return;
    } else if (bytes_read < bytes_written) {
        NOX_TEST_ASSERT(error == EOF, "Read too little.");
        return;
    }

    if (!session) {
        session = socket.get_session();
        NOX_TEST_ASSERT(session != 0, "Find the SSL session.");
    } else {
        delete session;
        session = 0;
    }
    error = socket.shutdown();
    NOX_TEST_ASSERT(error == 0, "Shutdown the client socket.");
}

void 
SSLTest::configure(const Configuration*)
{
    register_handler<Bootstrap_complete_event>
        (boost::bind(&SSLTest::handle_bootstrap, this,
                     _1));

}

void 
SSLTest::install()
{
}

Disposition
SSLTest::handle_bootstrap(const Event& e)
{
    Ssl_echo_server* server;

    boost::shared_ptr<Ssl_config> config(
        new Ssl_config(Ssl_config::ALL_VERSIONS,
                       Ssl_config::AUTHENTICATE_SERVER,
                       Ssl_config::REQUIRE_CLIENT_CERT,
                       "nox/coreapps/coretests/serverkey.pem",
                       "nox/coreapps/coretests/servercert.pem",
                       "nox/coreapps/coretests/cacert.pem"));
    
    server = new Ssl_echo_server(config);

    connect_as_client(config, test_str);        
    connect_as_client(config, test_str);

    ::exit(0);

    return CONTINUE;

}
