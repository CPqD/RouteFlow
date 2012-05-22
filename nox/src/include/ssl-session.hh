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
#ifndef SSL_SESSION_HH
#define SSL_SESSION_HH 1

#include <openssl/ssl.h>

/*
 * SSL_SESSION wrapper for client use of the session cache
 */

namespace vigil {

class Ssl_session {

public:
    friend class Ssl_socket;

    ~Ssl_session() {
        if (session != NULL) {
            SSL_SESSION_free(session);
        }
    }

private:
    SSL_SESSION * const session;

    Ssl_session(SSL_SESSION *session_)
        : session(session_) {}

    Ssl_session();
    Ssl_session(const Ssl_session&);
    Ssl_session& operator=(const Ssl_session&);
};

} // namespace vigil

#endif /* ssl-session.hh */
