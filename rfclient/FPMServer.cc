/*
 * Copyright (C) 2012  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 *
 * This file uses code from fpm_stub.c
 */

#ifdef FPM_ENABLED

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <errno.h>
#include <assert.h>

#include "fpm_lsp.h"

#include "FPMServer.hh"
#include "FlowTable.h"

typedef struct glob_t_ {
    int server_sock;
    int sock;
} glob_t;

glob_t glob_space;
glob_t *glob = &glob_space;

/* TODO: Integrate logging with RFClient */
int log_level = 1;

#define log(level, format...)     \
if (level <= log_level) {     \
    fprintf(stderr, format);      \
    fprintf(stderr, "\n");      \
}

#define info_msg(format...) log(1, format)
#define warn_msg(format...) log(0, format)
#define err_msg(format...) log(-1, format)
#define trace log

/*
 * create_listen_sock
 */
int FPMServer::create_listen_sock(int port, int *sock_p) {
    int sock;
    struct sockaddr_in addr;
    int reuse;

    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        err_msg( "Failed to create socket: %s", strerror(errno));
        return 0;
    }

    reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse,
                     sizeof(reuse)) < 0) {
        warn_msg("Failed to set reuse addr option: %s", strerror(errno));
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY );
    addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        err_msg("Failed to bind to port %d: %s", port, strerror(errno));
        close(sock);
        return 0;
    }

    if (listen(sock, 5)) {
        err_msg("Failed to listen on socket: %s", strerror(errno));
        close(sock);
        return 0;
    }

    *sock_p = sock;
    return 1;
}

/*
 * accept_conn
 */
int FPMServer::accept_conn(int listen_sock) {
    int sock;
    struct sockaddr_in client_addr;
    unsigned int client_len;

    while (1) {
        trace(1, "Waiting for client connection...");
        client_len = sizeof(client_addr);
        sock = accept(listen_sock, (struct sockaddr *) &client_addr,
                        &client_len);

        if (sock >= 0) {
            trace(1, "Accepted client %s", inet_ntoa(client_addr.sin_addr));
            return sock;
        }

        err_msg("Failed to accept socket: %s", strerror(errno));
    }
}

/*
 * read_fpm_msg
 */
fpm_msg_hdr_t *
FPMServer::read_fpm_msg(char *buf, size_t buf_len) {
    char *cur, *end;
    int need_len, bytes_read, have_len;
    fpm_msg_hdr_t *hdr;
    int reading_full_msg;

    end = buf + buf_len;
    cur = buf;
    hdr = (fpm_msg_hdr_t *) buf;

    while (1) {
        reading_full_msg = 0;

        have_len = cur - buf;

        if (have_len < FPM_MSG_HDR_LEN) {
            need_len = FPM_MSG_HDR_LEN - have_len;
        } else {
            need_len = fpm_msg_len(hdr) - have_len;
            assert(need_len >= 0 && need_len < (end - cur));

            if (!need_len) {
                return hdr;
            }

            reading_full_msg = 1;
        }

        trace(3, "Looking to read %d bytes", need_len);
        bytes_read = read(glob->sock, cur, need_len);

        if (bytes_read <= 0) {
            err_msg("Error reading from socket: %s", strerror(errno));
            return NULL;
        }

        trace(3, "Read %d bytes", bytes_read);
        cur += bytes_read;

        if (bytes_read < need_len) {
            continue;
        }

        assert(bytes_read == need_len);

        if (reading_full_msg) {
            return hdr;
        }

        if (!fpm_msg_ok(hdr, buf_len)) {
            err_msg("Malformed fpm message");
            return NULL;
        }
    }
}

void FPMServer::print_nhlfe(const nhlfe_msg_t *msg) {
    const char *op = (msg->table_operation == ADD_LSP)? "ADD_NHLFE" :
                     (msg->table_operation == REMOVE_LSP)? "REMOVE_NHLFE" :
                     "UNKNOWN";
    const char *type = (msg->nhlfe_operation == PUSH)? "PUSH" :
                       (msg->nhlfe_operation == POP)? "POP" :
                       (msg->nhlfe_operation == SWAP)? "SWAP" :
                       "UNKNOWN";
    const uint8_t *data = reinterpret_cast<const uint8_t*>(&msg->next_hop_ip);
    IPAddress ip(msg->ip_version, data);

    info_msg("fpm->%s %s %s %d %d", op, ip.toString().c_str(), type,
             ntohl(msg->in_label), ntohl(msg->out_label));
}

/*
 * process_fpm_msg
 */
void FPMServer::process_fpm_msg(fpm_msg_hdr_t *hdr) {
    trace(1, "FPM message - Type: %d, Length %d", hdr->msg_type,
            ntohs(hdr->msg_len));

    /**
     * Note: NHLFE and FTN are not standardised in Quagga 0.9.22. These are
     * subject to change, and correspond to the FIMSIM application found here:
     *
     * http://github.com/ofisher/FIMSIM
     */
    if (hdr->msg_type == FPM_MSG_TYPE_NETLINK) {
        struct nlmsghdr *n = (nlmsghdr *) fpm_msg_data(hdr);

        if (n->nlmsg_type == RTM_NEWROUTE || n->nlmsg_type == RTM_DELROUTE) {
            FlowTable::updateRouteTable(n);
        }
    } else if (hdr->msg_type == FPM_MSG_TYPE_NHLFE) {
        nhlfe_msg_t *lsp_msg = (nhlfe_msg_t *) fpm_msg_data(hdr);
        print_nhlfe(lsp_msg);
        FlowTable::updateNHLFE(lsp_msg);
    } else if (hdr->msg_type == FPM_MSG_TYPE_FTN) {
        warn_msg("FTN not yet implemented");
    } else {
        warn_msg("Unknown fpm message type %u", hdr->msg_type);
    }
}

/*
 * fpm_serve
 */
void FPMServer::fpm_serve() {
    char buf[FPM_MAX_MSG_LEN];
    fpm_msg_hdr_t *hdr;
    while (1) {
        hdr = FPMServer::read_fpm_msg(buf, sizeof(buf));
        if (!hdr) {
            return;
        }
        FPMServer::process_fpm_msg(hdr);
    }
}

void FPMServer::start() {
    memset(glob, 0, sizeof(*glob));
    if (!FPMServer::create_listen_sock(FPM_DEFAULT_PORT, &glob->server_sock)) {
        exit(1);
    }

    /*
     * Server forever.
     */
    while (1) {
        glob->sock = FPMServer::accept_conn(glob->server_sock);
        FPMServer::fpm_serve();
        trace(1, "Done serving client");
    }
}

#endif /* FPM_ENABLED */
