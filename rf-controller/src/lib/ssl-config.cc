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
#include "ssl-config.hh"
#include <boost/foreach.hpp>
#include <openssl/err.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "dhparams.h"
#include "errno_exception.hh"
#include "vlog.hh"

namespace vigil {

static Vlog_module lg("ssl-config");

static DH* tmp_dh_callback(SSL*, int, int keylength);

static bool init_Ssl()
{
    ::SSL_load_error_strings();
    ::SSL_library_init();
    return true;
}


/*
 * Sets up a configuration object to be used by an Ssl_socket.  Holds any
 * private keys or certificates a socket will need to perform handshakes with
 * remote peers.  Can be shared by multiple Ssl_sockets that want the same
 * config parameters.
 *
 * 'version' indicates which SSL/TLS version(s) the socket should use/support.
 * Should be a bitmask of possible versions defined by Ssl_config::Ssl_version.
 * To support multiple versions, just OR the Ssl_version values, or pass in
 * ALL_VERSIONS to support all possible versions.
 *
 * 'server_auth_' and 'client_auth_' signal the type of authentication that will be
 * required of created sessions communicating with remote servers and clients
 * respectively. 'CAfile' should be a PEM file containing the accepted certificate
 * authorities' certs to be used for authenticating peers.  If created sessions
 * will not be authenticating remote peers, 'CAfile' can be NULL.
 *
 * Meanwhile 'key' and 'cert' are PEM files with a private key and certificate
 * respectively to be used by servers/clients that need to authenticate
 * themselves to remote hosts.  Both 'key' and 'cert' are required of all
 * servers, regardless of whether or not remote clients will require server
 * authentication.  Additionally, clients communicating with servers requiring
 * client authentication will also need a 'key' and 'cert' set in order to
 * communicate.
 */

Ssl_config::Ssl_config(Ssl_version version,
                       Server_auth_type server_auth_, Client_auth_type client_auth_,
                       const char *key, const char *cert, const char *CAfile)
    : server_auth(server_auth_), client_auth(client_auth_)
{
    int error = init(version, key, cert, CAfile);
    if (error) {
        throw errno_exception(error, "Ssl_config initialization failed");
    }
}

int
Ssl_config::init(Ssl_version version,
                 const char* key, const char* cert, 
                 const char* controllerCAfile)
{
    static bool init = init_Ssl();
    assert(init);

    SSL_CONST ::SSL_METHOD* method = ::SSLv23_method();
    if (method == NULL) {
        lg.err("SSLv23_method: %s", ::ERR_error_string(::ERR_get_error(),
                                                       NULL));
        return ENOPROTOOPT;
    }

    ctx = ::SSL_CTX_new(method);
    if (ctx == NULL) {
        lg.err("SSL_CTX_new: %s", ::ERR_error_string(::ERR_get_error(),
                                                       NULL));
        return ENOPROTOOPT;
    }

    if (version != ALL_VERSIONS) {
        if ((version & (~ALL_VERSIONS)) != 0
            || version == 0)
        {
            lg.err("bad SSL version mask");
            return EINVAL;
        }
        long mask = 0;
        if ((version & SSLv2) == 0) {
            mask |= SSL_OP_NO_SSLv2;
        }
        if ((version & SSLv3) == 0) {
            mask |= SSL_OP_NO_SSLv3;
        }
        if ((version & TLSv1) == 0) {
            mask |= SSL_OP_NO_TLSv1;
        }
        ::SSL_CTX_set_options(ctx, mask);
    }

    ::SSL_CTX_set_tmp_dh_callback(ctx, tmp_dh_callback);
    ::SSL_CTX_set_mode(ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
    ::SSL_CTX_set_mode(ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    ::SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_OFF);

    uint32_t id = time(0);
    if (::SSL_CTX_set_session_id_context(ctx, (unsigned char*)(&id), sizeof(id)) != 1) {
        ::SSL_CTX_free(ctx);
        lg.err("SSL_CTX_set_session_id_context: %s",
               ::ERR_error_string(::ERR_get_error(), NULL));
        return ENOPROTOOPT;
    }

    if (key != NULL
        && ::SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM) != 1)
    {
        ::SSL_CTX_free(ctx);
        lg.err("SSL_CTX_use_PrivateKey_file: %s",
               ::ERR_error_string(::ERR_get_error(), NULL));
        return ENOPROTOOPT;
    }


    if (cert != NULL 
        && ::SSL_CTX_use_certificate_file(ctx,cert,SSL_FILETYPE_PEM) != 1)
    {
        ::SSL_CTX_free(ctx);
        lg.err("SSL_CTX_use_certificate_file: %s",
               ::ERR_error_string(::ERR_get_error(), NULL));
        return ENOPROTOOPT;
    }
    
    if (controllerCAfile != NULL && strlen(controllerCAfile) > 0 
        && set_cert_chain(controllerCAfile))
    {
        ::SSL_CTX_free(ctx);
        lg.err("set_cert_chain: %s",
               ::ERR_error_string(::ERR_get_error(), NULL));
        return ENOPROTOOPT;
    }


    return 0;
}
  
/*
 * Not currently used by NOX. This allows us to specify a CA file such
 * that NOX only accepts connections from switches using client certs
 * signed by this CA.  However, the current switch authentication model 
 * does not use this, because we want to allow swithces to be manually
 * approved even when they use self-signed certs.  
 */
int Ssl_config::set_switch_CA(char *CAfile) { 

    if (CAfile && strlen(CAfile)) {

        /* Set up list of CAs that the server will accept from the client.*/ 
        STACK_OF(X509_NAME)* ca_list = ::SSL_load_client_CA_file(CAfile);
        if (ca_list == NULL) {
            ::SSL_CTX_free(ctx);
            lg.err("SSL_load_client_CA_file: %s",
                   ::ERR_error_string(::ERR_get_error(), NULL));
            return ENOPROTOOPT;
        }
        SSL_CTX_set_client_CA_list(ctx, ca_list);
    
        /* Set up CAs for OpenSSL to trust in verifying the peer's
         * certificate. */ 
        if (SSL_CTX_load_verify_locations(ctx, CAfile, NULL) != 1) {
            ::SSL_CTX_free(ctx);
            lg.err("SSL_CTX_load_verify_locations: %s",
                   ::ERR_error_string(::ERR_get_error(), NULL));
            return ENOPROTOOPT;
        }
    }
    return 0; 
} 

// This method is borrowed from Ben's Openflow code
/* Reads the X509 certificate or certificates in file 'file_name'.  On success,
 * stores the address of the first element in an array of pointers to
 * certificates in '*certs' and the number of certificates in the array in
 * '*n_certs', and returns 0.  On failure, stores a null pointer in '*certs', 0
 * in '*n_certs', and returns a positive errno value.
 *
 * The caller is responsible for freeing '*certs'. */
int
read_cert_file(const char *file_name, X509 ***certs, size_t *n_certs)
{
    FILE *file;
    size_t allocated_certs = 0;

    *certs = NULL;
    *n_certs = 0;

    file = fopen(file_name, "r");
    if (!file) {
        lg.err("failed to open %s for reading: %s",
            file_name, strerror(errno));
        return errno;
    }

    for (;;) {
        X509 *certificate;
        int c;

        /* Read certificate from file. */
        certificate = PEM_read_X509(file, NULL, NULL, NULL);
        if (!certificate) {
            size_t i;

            lg.err("PEM_read_X509 failed reading %s: %s",
                     file_name, ERR_error_string(ERR_get_error(), NULL));
            for (i = 0; i < *n_certs; i++) {
                X509_free((*certs)[i]);
            }
            free(*certs);
            *certs = NULL;
            *n_certs = 0;
            return EIO;
        }

        /* Add certificate to array. */
        if (*n_certs >= allocated_certs) {
            allocated_certs = 1 + 2 * allocated_certs;
            *certs = (X509**)realloc(*certs, sizeof *certs * allocated_certs);
        }
        (*certs)[(*n_certs)++] = certificate;

        /* Are there additional certificates in the file? */
        do {
            c = getc(file);
        } while (isspace(c));
        if (c == EOF) {
            break;
        }
        ungetc(c, file);
    }
    fclose(file);
    return 0;
}


/* Sets 'file_name' as the name of a file containing one or more X509
 * certificates to send to the peer.  Typical use in OpenFlow is to send the CA
 * certificate to the peer, which enables a switch to pick up the controller's
 * CA certificate on its first connection. */
int Ssl_config::set_cert_chain(const char *file_name)
{
    X509 **certs;
    size_t n_certs;
    size_t i;
    int error = read_cert_file(file_name, &certs, &n_certs);
    if(error)
      return error; 

    for (i = 0; i < n_certs; i++) {
            error = SSL_CTX_add_extra_chain_cert(ctx, certs[i]);
            if(error != 1) {
                lg.err("SSL_CTX_add_extra_chain_cert: %s",
                         ERR_error_string(ERR_get_error(), NULL));
                return EINVAL; 
            }
    }
    free(certs);
    return 0; 
}


/*
 * Frees the associated SSL_CTX object.
 */

Ssl_config::~Ssl_config()
{
    ::SSL_CTX_free(ctx);
}

/*
 * The server always verifies the client at the SSL layer, since 
 * we check if the fingerprint is valid later (during Openflow setup). 
 *
 * Note: We cannot just use SSL_VERIFY_NONE because that will cause
 * the server to skip sending the certificate request to the client
 * and we need the client cert to verify that the switch is approved.
 */
static int always_verify(int preverify_ok, X509_STORE_CTX *ctx) {
  return 1; 
} 

/*
 * Returns a new SSL instance.  Should be freed when finished using SSL_free().
 */

SSL*
Ssl_config::new_ssl(Session_type session)
{
    SSL* ssl = ::SSL_new(ctx);

    if (ssl == NULL) {
        ::fprintf(stdout, "%s", ::ERR_error_string(::ERR_get_error(), NULL));
        ::fprintf(stderr, " (%s)", strerror(ENOPROTOOPT));
    } else if (session == CLIENT) {
        // FIXME: NOX no longer really uses client SSL functionality, 
        // except in a test.  Because there is not a command-line option
        // to pass in a CA cert for a remote server, we use the 
        // always_verify callback here so that the test passes.  
        ::SSL_set_verify(ssl, server_auth, always_verify);
    } else {
        // FIXME: we will want to be able to also support
        // standard client authentication via a NULL callback
        ::SSL_set_verify(ssl, client_auth, always_verify);
    }

    return ssl;
}

struct dhparams {
    int keylength;
    DH* dh;
    DH* (*constructor)(void);
};

static dhparams dh_table[] = {
    {1024, NULL, get_dh1024},
    {2048, NULL, get_dh2048},
    {4096, NULL, get_dh4096},
};

static DH*
tmp_dh_callback(SSL* ssl, int, int keylength)
{
    BOOST_FOREACH (dhparams& dh, dh_table) {
        if (dh.keylength == keylength) {
            if (!dh.dh) {
                dh.dh = dh.constructor();
                if (!dh.dh) {
                    lg.emer("out of memory constructing "
                            "Diffie-Hellman parameters");
                    return NULL;
                }
            }
            return dh.dh;
        }
    }
    lg.err("no Diffie-Hellman parameters for key length %d", keylength);
    return NULL;
}

} // namespace vigil
