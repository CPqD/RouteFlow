/* Copyright 2008, 2009 (C) Nicira, Inc.
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
#include "file-server.hh"
#include <boost/bind.hpp>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "async_file.hh"
#include "buffer.hh"
#include "controller.hh"
#include "netinet++/ipaddr.hh"
#include "tcp-socket.hh"
#include "threads/cooperative.hh"

using namespace vigil;

namespace {

class File_connection
{
public:
    static const std::size_t piece_size = 1;

    File_connection(std::auto_ptr<Tcp_socket> socket_,
		    const std::string& file_name)
	: socket(socket_) {
        if (int error = file.open(file_name, O_RDONLY)) {
            ::error(error, "%s", ("open \"" + file_name + "\"").c_str());
        }
        co_thread_create(&co_group_coop,
                         boost::bind(&File_connection::run, this));
    }

private:
    std::auto_ptr<Tcp_socket> socket;
    Async_file file;

    void run() {
        Array_buffer buffer(piece_size);
        for (;;) {
            ssize_t bytes_read = file.read(buffer, true);
            if (bytes_read == 0) {
                break;
            } else if (bytes_read < 0) {
                ::error(-bytes_read, "file read");
                break;
            }

            ssize_t bytes_written;
            int error = socket->write_fully(buffer, &bytes_written, true);
            if (error) {
                ::error(error, "socket write");
                break;
            }
        }
    }
};

class File_server
{
public:
    File_server(const std::string& file_name_, uint16_t port)
	: socket(), file_name(file_name_) {
	int error;

	error = socket.bind(INADDR_ANY, port);
	if (error) {
            ::error(error, "bind");
            return;
	}

	error = socket.listen(10);
	if (error) {
            ::error(error, "listen");
            return;
	}

        co_thread_create(&co_group_coop, boost::bind(&File_server::run, this));
    }

private:
    Tcp_socket socket;
    std::string file_name;

    static void run_wrapper(void* object_) {
        std::auto_ptr<File_server> object(static_cast<File_server*>(object_));
        object->run();
    }

    void run() {
        int error;
        for (;;) {
            std::auto_ptr<Tcp_socket> new_socket = socket.accept(error, true);
            if (error) {
                break;
            }
            new File_connection(new_socket, file_name);
        }
        ::error(error, "accept");
    }
};

} // null namespace

namespace vigil {

void register_file_server(const std::string& file_name, uint16_t port)
{
new File_server(file_name, port);
}

void 
FileServerTest::configure(const Configuration*)
{
    register_handler<Bootstrap_complete_event>
        (boost::bind(&FileServerTest::handle_bootstrap, this,
                     _1));

}

void 
FileServerTest::install()
{
}

Disposition
FileServerTest::handle_bootstrap(const Event& e)
{
    new File_server(file_name, port);
}

} // namespace vigil
