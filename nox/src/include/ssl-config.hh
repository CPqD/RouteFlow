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
#ifndef SSL_CONFIG_HH
#define SSL_CONFIG_HH 1

#include <openssl/ssl.h>

/*
 * Configuration class for SSL sessions.
 * Holds key and certificates.
 */

namespace vigil {

class Ssl_config {

public:
    enum Ssl_version {
        SSLv2 = 0x1 << 0,
        SSLv3 = 0x1 << 1,
        TLSv1 = 0x1 << 2,
        ALL_VERSIONS = (0x1 << 3) - 1
    };

    enum Server_auth_type {
        NO_SERVER_AUTH = SSL_VERIFY_NONE,
        AUTHENTICATE_SERVER = SSL_VERIFY_PEER
    };

    enum Client_auth_type {
        NO_CLIENT_AUTH = SSL_VERIFY_NONE,
        REQUEST_CLIENT_CERT = SSL_VERIFY_PEER,
        REQUIRE_CLIENT_CERT = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT
    };

    enum Session_type {
        CLIENT,
        SERVER
    };

    Ssl_config(Ssl_version, Server_auth_type, Client_auth_type,
               const char *, const char *, const char *);
    ~Ssl_config();
    SSL *new_ssl(Session_type t);
    int set_switch_CA(char *CAfile);  


private:
    SSL_CTX *ctx;
    Server_auth_type server_auth;
    Client_auth_type client_auth;

    int init(Ssl_version, const char *, const char *, const char *);
    int set_cert_chain(const char *file_name); 
};

static inline Ssl_config::Ssl_version operator|(Ssl_config::Ssl_version a,
                                                Ssl_config::Ssl_version b)
{
    return Ssl_config::Ssl_version((int)a | (int)b);
}

} // namespace vigil

#endif /* ssl-config.hh */
