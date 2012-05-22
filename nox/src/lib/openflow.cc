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
#include <config.h>
#include <algorithm>
#include "openflow.hh"
#include "openflow/openflow-mgmt.h"
#include "openflow/nicira-ext.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include "assert.hh"
#include <cerrno>
#include <inttypes.h>
#include <netinet/in.h>
#include "async_io.hh"
#include "buffer.hh"
#include "datapath.hh"
#include "errno_exception.hh"
#include "netinet++/ipaddr.hh"
#include "packetgen.hh"
#ifdef HAVE_PCAP
#include "pcapreader.hh"
#endif
#include "resolver.hh"
#include "tcp-socket.hh"
#include "ssl-socket.hh"
#include "ssl-config.hh"
#include "timeval.hh"
#include "vlog.hh"
#include "openflow-pack.hh"

namespace vigil {

static Vlog_module log("openflow");

const int Reliable_openflow_connection::backoff_limit = 60;
const int Openflow_connection::probe_interval = 15;

Openflow_connection::Openflow_connection()
    : ext_data_xid(UINT32_MAX),
      datapath_id(),
      state(S_CONNECTING),
      min_version(OFP_VERSION),
      fsm(boost::bind(&Openflow_connection::run, this))
{
    ext_data_buffer.reset(new Array_buffer(0));
}

void
Openflow_connection::s_connecting()
{
    int retval = do_connect();
    assert(retval != EINPROGRESS);
    if (!retval) {
        state = S_SEND_HELLO;
    } else if (retval != EAGAIN) {
        state = S_DISCONNECTED;
        error = retval;
    }
}

void
Openflow_connection::s_send_hello()
{
    ofp_header hello;
    hello.version = OFP_VERSION;
    hello.type = OFPT_HELLO;
    hello.length = htons(sizeof hello);
    hello.xid = openflow_pack::get_xid();

    int retval = call_send_openflow(&hello);
    if (!retval) {
        state = S_RECV_HELLO;
    } else if (retval != EAGAIN) {
        state = S_DISCONNECTED;
        error = retval;
    }
}

void
Openflow_connection::s_recv_hello()
{
    int retval;
    std::auto_ptr<Buffer> b(call_recv_openflow(retval));
    if (!retval) {
        const struct ofp_header *oh = &b->at<ofp_header>(0);
        if (oh->type == OFPT_HELLO) {
            if (b->size() > sizeof *oh) {
                VLOG_WARN(log, "%s: extra-long hello (%zu extra bytes)",
                          to_string().c_str(), b->size() - sizeof *oh);
            }

            version = std::min(OFP_VERSION, int(oh->version));
            if (version < min_version) {
                VLOG_WARN(log, "%s: version negotiation failed: we support "
                          "versions 0x%02x to 0x%02x inclusive but peer "
                          "supports no later than version 0x%02"PRIx8,
                          to_string().c_str(), min_version, OFP_VERSION,
                          oh->version);
                state = S_SEND_ERROR;
            } else {
                VLOG_DBG(log, "%s: negotiated OpenFlow version 0x%02x "
                         "(we support versions 0x%02x to 0x%02x inclusive, "
                         "peer no later than version 0x%02"PRIx8")",
                         to_string().c_str(), version, min_version,
                         OFP_VERSION, oh->version);
		if (oh->version > OFP_EXPERIMENTAL_VERSION)
		  state = S_SEND_ERROR;
		else
		  state = S_CONNECTED;
                timeout = do_gettimeofday() + make_timeval(probe_interval, 0);
                fsm.wake();
            }
        } else {
            VLOG_WARN(log, "%s: unexpected message (type 0x%02"PRIx8") "
                      "waiting for hello", to_string().c_str(), oh->type);
            retval = EPROTO;
        }
    }

    if (retval && retval != EAGAIN) {
        state = S_DISCONNECTED;
        error = retval;
    }
}

void
Openflow_connection::s_send_error()
{
    char s[128];
    snprintf(s, sizeof s, "We support versions 0x%02x to 0x%02x inclusive but "
             "you support no later than version 0x%02"PRIx8".",
             min_version, OFP_VERSION, version);
    size_t s_len = strlen(s);

    size_t size = sizeof(ofp_error_msg) + s_len;
    Array_buffer b(size);
    ofp_error_msg& oem = b.at<ofp_error_msg>(0);
    oem.header.version = OFP_VERSION;
    oem.header.type = OFPT_ERROR;
    oem.header.length = htons(size);
    oem.type = htons(OFPET_HELLO_FAILED);
    oem.code = htons(OFPHFC_INCOMPATIBLE);
    memcpy(oem.data, s, s_len);

    int retval = call_send_openflow(&oem.header);
    if (retval != EAGAIN) {
        if (retval) {
            log.warn("%s: openflow send failed: %s",
                     to_string().c_str(), strerror(retval));
        }
        state = S_DISCONNECTED;
        error = retval ? retval : EPROTO;
    }
}

/* Tries to complete the OpenFlow connection.  If successful, returns 0.
 *
 * Returns a positive errno value on failure.
 *
 * If 'block' is false, returns EAGAIN if the connection cannot be completed
 * immediately. */
int Openflow_connection::connect(bool block)
{
    co_might_yield_if(block);

    State last_state;
    do {
        last_state = state;
        switch (state) {
        case S_CONNECTING:
            s_connecting();
            break;

        case S_SEND_HELLO:
            s_send_hello();
            break;

        case S_RECV_HELLO:
            s_recv_hello();
            break;

        case S_CONNECTED:
        case S_IDLE:
            return 0;

        case S_SEND_ERROR:
            s_send_error();
            break;

        case S_DISCONNECTED:
            return error;

        default:
            ::abort();
        }
    } while (state != last_state);

    return EAGAIN;
}

/* Tries to queue 'oh' for transmission.  If successful, returns 0.  The caller
 * retains ownership of 'oh'.  Success does not guarantee that 'oh' has been or
 * ever will be delivered to the peer, only that it has been queued for
 * transmission.
 *
 * Returns a positive errno value on failure.
 *
 * If 'block' is false, returns EAGAIN if 'oh' cannot be immediately accepted
 * for transmission. */
int Openflow_connection::send_openflow(const ofp_header* oh, bool block)
{
    co_might_yield_if(block);
    for (;;) {
        int error = connect(block);
        if (!error) {
            error = call_send_openflow(oh);
        }
        if (block && error == EAGAIN) {
            co_might_yield();
            send_openflow_wait();
            co_block();
        } else if (error != EINTR) {
            return error;
        }
    }
}

/* Call do_send_openflow() and log any error. */
int
Openflow_connection::call_send_openflow(const ofp_header* oh)
{
    int error = do_send_openflow(oh);
    if (error && error != EAGAIN) {
        log.warn("%s: send error: %s", to_string().c_str(), strerror(error));
    }
    return error;
}

/* Call do_recv_openflow() and log any error. */
std::auto_ptr<Buffer>
Openflow_connection::call_recv_openflow(int& error)
{
    std::auto_ptr<Buffer> b(do_recv_openflow(error));
    if (error && error != EAGAIN) {
        if (error == EOF) {
            log.warn("%s: connection closed by peer", to_string().c_str());
        } else {
            log.warn("%s: receive error: %s",
                     to_string().c_str(), strerror(error));
        }
    }
    return b;
}

void
Openflow_connection::run()
{
    if (state == S_CONNECTED || state == S_IDLE) {
        if (do_gettimeofday() >= timeout) {
            if (state == S_CONNECTED) {
                int retval = send_echo_request();
                if (!retval) {
                    log.dbg("%s: idle %d seconds, sending inactivity probe",
                            to_string().c_str(), probe_interval);
                    state = S_IDLE;
                    timeout = (do_gettimeofday()
                               + make_timeval(probe_interval, 0));
                    co_timer_wait(timeout, NULL);
                } else if (retval == EAGAIN) {
                    send_openflow_wait();
                } else {
                    log.warn("%s: sending inactivity probe failed, "
                             "disconnected: %s",
                             to_string().c_str(), strerror(retval));
                    state = S_DISCONNECTED;
                    error = retval;
                    close();
                }
            } else {
                log.warn("%s: no response to inactivity probe after %d "
                         "seconds, disconnecting",
                         to_string().c_str(), probe_interval);
                state = S_DISCONNECTED;
                error = EPROTO;
                close();
            }
        } else {
            co_timer_wait(timeout, NULL);

        }
    }
    co_fsm_block();
}

/* Tries to receive an OpenFlow message.  If successful, returns the received
 * message and sets 'error' to 0.  The caller is responsible for destroying the
 * message.  On failure, returns null and stores a positive errno value into
 * 'error'.
 *
 * If the connection has been closed in the normal fashion, sets 'error' to
 * EOF (a negative value).
 *
 * If 'block' is false, returns EAGAIN immediately if no packets have been
 * received. */
std::auto_ptr<Buffer>
Openflow_connection::recv_openflow(int& error, bool block)
{
    co_might_yield_if(block);
    for (;;) {
        error = connect(false);
        if (error) {
            return std::auto_ptr<Buffer>();
        }
        std::auto_ptr<Buffer> b(call_recv_openflow(error));
        if (block && error == EAGAIN) {
            co_might_yield();
            recv_openflow_wait();
            co_block();
        } else if (error != EINTR) {
            if (!error) {
                if (state == S_IDLE) {
                    log.dbg("%s: message received, entering CONNECTED",
                            to_string().c_str());
                    state = S_CONNECTED;
                }
                timeout = do_gettimeofday() + make_timeval(probe_interval, 0);
            }
            assert((error == 0) == (b.get() != NULL));
            return b;
        }
    }
}

bool Openflow_connection::need_to_wait_for_connect()
{
    switch (state) {
    case S_CONNECTING:
        do_connect_wait();
        return true;

    case S_SEND_HELLO:
    case S_SEND_ERROR:
        do_send_openflow_wait();
        return true;

    case S_RECV_HELLO:
        do_recv_openflow_wait();
        return true;

    case S_CONNECTED:
    case S_IDLE:
    case S_DISCONNECTED:
        return false;
    }
    abort();
}

void Openflow_connection::connect_wait()
{
    if (!need_to_wait_for_connect()) {
        co_immediate_wake(1, NULL);
    }
}

void Openflow_connection::send_openflow_wait()
{
    if (!need_to_wait_for_connect()) {
        do_send_openflow_wait();
    }
}

void Openflow_connection::recv_openflow_wait()
{
    if (!need_to_wait_for_connect()) {
        do_recv_openflow_wait();
    }
}

/* Sends 'packet' out on 'out_port'.  If 'out_port' is OFPP_FLOOD, then the
 * packet is not sent on 'in_port'.
 *
 * Returns 0 on success, positive errno value on failure.
 *
 * If 'block' is false, returns EAGAIN if the packet cannot be immediately
 * accepted for transmission. */
int Openflow_connection::send_packet(const Buffer& packet, uint16_t out_port,
                                     uint16_t in_port, bool block)
{
    co_might_yield_if(block);
    ofp_action_output oa;
    memset(&oa, 0, sizeof oa);
    oa.type = htons(OFPAT_OUTPUT);
    oa.len = htons(sizeof oa);
    oa.port = htons(out_port);
    oa.max_len = htons(0);
    return send_packet(packet, (ofp_action_header *)&oa, sizeof oa, 
            in_port, block);
}

/* Sends 'packet' and its 'actions'.  If an action is OFPP_FLOOD, then 
 * the packet is not sent on 'in_port'.
 *
 * Returns 0 on success, positive errno value on failure.
 *
 * If 'block' is false, returns EAGAIN if the packet cannot be immediately
 * accepted for transmission. */
int Openflow_connection::send_packet(const Buffer& packet, 
                                     const ofp_action_header actions[],
                                     size_t actions_len,
                                     uint16_t in_port, bool block)
{
    co_might_yield_if(block);

    size_t opo_size = sizeof(ofp_packet_out) + actions_len + packet.size();
    assert(opo_size <= UINT16_MAX);

    Array_buffer b(opo_size);
    ofp_packet_out& opo = b.at<ofp_packet_out>(0);
    memset(&opo, 0, sizeof opo);
    opo.header.version = OFP_VERSION;
    opo.header.type = OFPT_PACKET_OUT;
    opo.header.length = htons(opo_size);
    opo.buffer_id = htonl(UINT32_MAX);
    opo.in_port = htons(in_port);
    opo.actions_len = htons(actions_len);
    memcpy(opo.actions, actions, actions_len);
    memcpy((uint8_t *)opo.actions + actions_len, packet.data(), packet.size());

    return send_openflow(&opo.header, block);
}


/* Sends the packet with id 'buffer_id' out on 'out_port'.  If 'out_port' is
 * OFPP_FLOOD, then the packet is not sent on 'in_port'.
 *
 * Returns 0 on success, positive errno value on failure.
 *
 * If 'block' is false, returns EAGAIN if the packet cannot be immediately
 * accepted for transmission. */
int Openflow_connection::send_packet(uint32_t buffer_id, uint16_t out_port,
                                     uint16_t in_port, bool block)
{
    co_might_yield_if(block);
    ofp_action_output oa;
    memset(&oa, 0, sizeof oa);
    oa.type = htons(OFPAT_OUTPUT);
    oa.len = htons(sizeof oa);
    oa.port = htons(out_port);
    oa.max_len = htons(0);
    return send_packet(buffer_id, (ofp_action_header *)&oa, sizeof oa, 
            in_port, block);
}

/* Sends the packet with id 'buffer_id' and its 'actions'.  If an action is 
 * OFPP_FLOOD, then the packet is not sent on 'in_port'.
 *
 * Returns 0 on success, positive errno value on failure.
 *
 * If 'block' is false, returns EAGAIN if the packet cannot be immediately
 * accepted for transmission. */
int Openflow_connection::send_packet(uint32_t buffer_id,
                                       const ofp_action_header actions[],
                                       size_t actions_len,
                                       uint16_t in_port, bool block)
{
    co_might_yield_if(block);

    size_t opo_size = sizeof(ofp_packet_out) + actions_len;
    assert(opo_size <= UINT16_MAX);
    Array_buffer b(opo_size);

    ::memset(b.data(), 0, opo_size);

    ofp_packet_out& opo = b.at<ofp_packet_out>(0);
    opo.header.version = OFP_VERSION;
    opo.header.type = OFPT_PACKET_OUT;
    opo.header.length = htons(opo_size);
    opo.buffer_id = htonl(buffer_id);
    opo.in_port = htons(in_port);
    opo.actions_len = htons(actions_len);
    memcpy(opo.actions, actions, actions_len);
    return send_openflow(&opo.header, block);
}

/* Sends an ofp_features_request message.
 * Does not block: returns EAGAIN if the messages cannot be immediately
 * accepted for transmission. */
int Openflow_connection::send_features_request()
{
    struct ofp_header ofr;

    /* Send OFPT_FEATURES_REQUEST. */
    ofr.type = OFPT_FEATURES_REQUEST;
    ofr.version = OFP_VERSION;
    ofr.length = htons(sizeof ofr);
    ofr.xid = openflow_pack::get_xid();
    return send_openflow(&ofr, false);
} 

/* xxx Should these ofmp messages be a different class? */
/* xxx Shouldn't these be setting xid? */
int Openflow_connection::send_ofmp_capability_request()
{
    struct ofmp_capability_request ocr;

    memset(&ocr, '\0', sizeof(ocr));

    /* OpenFlow header */
    ocr.header.header.header.type = OFPT_VENDOR;
    ocr.header.header.header.version = OFP_VERSION;
    ocr.header.header.header.length = htons(sizeof ocr);
    ocr.header.header.header.xid = openflow_pack::get_xid();

    /* Nicira header */
    ocr.header.header.vendor = htonl(NX_VENDOR_ID);
    ocr.header.header.subtype = htonl(NXT_MGMT);

    /* OFMP header */
    ocr.header.type = htons(OFMPT_CAPABILITY_REQUEST);

    ocr.format = htonl(OFMPCAF_SIMPLE);

    return send_openflow(&ocr.header.header.header, false);
} 

int Openflow_connection::send_ofmp_resources_request()
{
    struct ofmp_resources_request orr;

    memset(&orr, '\0', sizeof(orr));

    /* OpenFlow header */
    orr.header.header.header.type = OFPT_VENDOR;
    orr.header.header.header.version = OFP_VERSION;
    orr.header.header.header.length = htons(sizeof orr);
    orr.header.header.header.xid = openflow_pack::get_xid();

    /* Nicira header */
    orr.header.header.vendor = htonl(NX_VENDOR_ID);
    orr.header.header.subtype = htonl(NXT_MGMT);

    /* OFMP header */
    orr.header.type = htons(OFMPT_RESOURCES_REQUEST);

    return send_openflow(&orr.header.header.header, false);
} 

int Openflow_connection::send_ofmp_config_request()
{
    struct ofmp_config_request ocr;

    memset(&ocr, '\0', sizeof(ocr));

    /* OpenFlow header */
    ocr.header.header.header.type = OFPT_VENDOR;
    ocr.header.header.header.version = OFP_VERSION;
    ocr.header.header.header.length = htons(sizeof ocr);
    ocr.header.header.header.xid = openflow_pack::get_xid();

    /* Nicira header */
    ocr.header.header.vendor = htonl(NX_VENDOR_ID);
    ocr.header.header.subtype = htonl(NXT_MGMT);

    /* OFMP header */
    ocr.header.type = htons(OFMPT_CONFIG_REQUEST);

    ocr.format = htonl(OFMPCOF_SIMPLE);

    return send_openflow(&ocr.header.header.header, false);
} 

/* Sends a ofp_switch_config message.
 * Does not block: returns EAGAIN if the messages cannot be immediately
 * accepted for transmission. */
int Openflow_connection::send_switch_config() { 
    
    struct ofp_switch_config osc;
    
    /* Send OFPT_SET_CONFIG. */
    osc.header.type = OFPT_SET_CONFIG;
    osc.header.version = OFP_VERSION;
    osc.header.length = htons(sizeof osc);
    osc.header.xid = openflow_pack::get_xid();
    osc.flags =  0;
    osc.miss_send_len = htons(OFP_DEFAULT_MISS_SEND_LEN);

    return send_openflow(&osc.header, false);
}

/* Sends an ofp_stats_request message  
 *
 * Does not block: returns EAGAIN if the messages cannot be immediately
 * accepted for transmission. */
int Openflow_connection::send_stats_request(ofp_stats_types type)
{
    struct ofp_stats_request osr;

    /* Send OFPT_STATS_REQUEST. */
    osr.header.type    = OFPT_STATS_REQUEST;
    osr.header.version = OFP_VERSION;
    osr.header.length  = htons(sizeof osr);
    osr.header.xid     = openflow_pack::get_xid();
    osr.type           = type;
    osr.flags          = htons(0); /* CURRENTLY NONE DEFINED */ 

    return send_openflow(&osr.header, false);
}

/* Sends an OFPT_ECHO_REQUEST message.
 *
 * Does not block: returns EAGAIN if the message cannot be immediately accepted
 * for transmission. */
int Openflow_connection::send_echo_request()
{
    ofp_header oer;
    oer.type = OFPT_ECHO_REQUEST;
    oer.version = OFP_VERSION;
    oer.length = htons(sizeof oer);
    oer.xid = openflow_pack::get_xid();
    return send_openflow(&oer, false);
}


/* Sends an OFPT_ECHO_REPLY message matching the OFPT_ECHO_REQUEST message in
 * 'rq'.
 *
 * Does not block: returns EAGAIN if the message cannot be immediately accepted
 * for transmission. */
int Openflow_connection::send_echo_reply(const struct ofp_header *rq)
{
    Array_buffer out(0);
    size_t size = ntohs(rq->length);
    memcpy(out.put(size), rq, size);
    ofp_header* reply = &out.at<ofp_header>(0);
    reply->type = OFPT_ECHO_REPLY;
    return send_openflow(reply, false);
}


/* Sends an OFPT_BARRIER_REQUEST message.
 *
 * Does not block: returns EAGAIN if the message cannot be immediately accepted
 * for transmission. */
int Openflow_connection::send_barrier_request()
{
    ofp_header obr;
    obr.type = OFPT_BARRIER_REQUEST;
    obr.version = OFP_VERSION;
    obr.length = htons(sizeof obr);
    obr.xid = openflow_pack::get_xid();
    return send_openflow(&obr, false);
}


/* Sends a delete SNAT configuration message.
 *
 * Does not block: returns EAGAIN if the messages cannot be immediately
 * accepted for transmission. */
int Openflow_connection::send_add_snat(uint16_t port, 
        uint32_t ip_addr_start, uint32_t ip_addr_end,
        uint16_t tcp_start, uint16_t tcp_end,
        uint16_t udp_start, uint16_t udp_end,
        ethernetaddr mac_addr, 
        uint16_t mac_timeout)
{
    size_t nac_size = sizeof (nx_act_config) + sizeof (nx_snat_config);
    assert(nac_size <= UINT16_MAX);
    Array_buffer b(nac_size);

    ::memset(b.data(), 0, nac_size);
    nx_act_config& nac = b.at<nx_act_config>(0);

    nac.header.header.type    = OFPT_VENDOR;
    nac.header.header.version = OFP_VERSION;
    nac.header.header.length  = htons(nac_size);
    nac.header.header.xid     = openflow_pack::get_xid();

    nac.header.vendor         = htonl(NX_VENDOR_ID);
    nac.header.subtype        = htonl(NXT_ACT_SET_CONFIG);
    nac.type                  = htons(NXAST_SNAT);

    nac.snat[0].command       = NXSC_ADD;
    nac.snat[0].port          = htons(port);
    nac.snat[0].mac_timeout   = htons(mac_timeout);
    nac.snat[0].ip_addr_start = htonl(ip_addr_start);
    nac.snat[0].ip_addr_end   = htonl(ip_addr_end);
    nac.snat[0].tcp_start     = htons(tcp_start);
    nac.snat[0].tcp_end       = htons(tcp_end);
    nac.snat[0].udp_start     = htons(udp_start);
    nac.snat[0].udp_end       = htons(udp_end);
    memcpy(nac.snat[0].mac_addr, (const uint16_t*)mac_addr, OFP_ETH_ALEN);
    
    return send_openflow(&nac.header.header, false);
}

int Openflow_connection::send_add_snat(uint16_t port, uint32_t ip_addr)
{
    ethernetaddr eaddr = ethernetaddr("00:00:00:00:00:00");
    return send_add_snat(port, ip_addr, ip_addr, 0, 0, 0, 0, eaddr, 0);
}

/* Sends a delete SNAT configuration message.
 *
 * Does not block: returns EAGAIN if the messages cannot be immediately
 * accepted for transmission. */
int Openflow_connection::send_del_snat(uint16_t port)
{
    size_t nac_size = sizeof (nx_act_config) + sizeof (nx_snat_config);
    assert(nac_size <= UINT16_MAX);
    Array_buffer b(nac_size);

    ::memset(b.data(), 0, nac_size);
    nx_act_config& nac = b.at<nx_act_config>(0);

    nac.header.header.type    = OFPT_VENDOR;
    nac.header.header.version = OFP_VERSION;
    nac.header.header.length  = htons(nac_size);
    nac.header.header.xid     = openflow_pack::get_xid();

    nac.header.vendor         = htonl(NX_VENDOR_ID);
    nac.header.subtype        = htonl(NXT_ACT_SET_CONFIG);
    nac.type                  = htons(NXAST_SNAT);

    nac.snat[0].command       = NXSC_DELETE;
    nac.snat[0].port          = htons(port);

    return send_openflow(&nac.header.header, false);
}

/* Sends a remote command across the OpenFlow connection, using an Nicira
 * extension.  The command sent is 'command', along with the set of optional
 * command arguments in 'args'. */
int
Openflow_connection::send_remote_command(const std::string& command,
                                         const std::vector<std::string>& args)
{
    Array_buffer b(sizeof(nicira_header));

    nicira_header* nh = &b.at<nicira_header>(0);
    nh->header.type = OFPT_VENDOR;
    nh->header.version = OFP_VERSION;
    nh->header.xid = openflow_pack::get_xid();
    nh->vendor = htonl(NX_VENDOR_ID);
    nh->subtype = htonl(NXT_COMMAND_REQUEST);

    memcpy(b.put(command.size()), command.data(), command.size());
    BOOST_FOREACH (const std::string& arg, args) {
        b.put(1)[0] = '\0';
        memcpy(b.put(arg.size()), arg.data(), arg.size());
    }

    nh = &b.at<nicira_header>(0);
    nh->header.length = htons(b.size());

    return send_openflow(&nh->header, false);
}


/* Constructs a Openflow connection that takes over ownership of 'stream'. */
Openflow_stream_connection::Openflow_stream_connection(
    std::auto_ptr<Async_stream> stream_,Connection_type t)
    : tx_fsm(boost::bind(&Openflow_stream_connection::tx_run, this)),
      stream(stream_), rx_bytes(0), conn_type(t)
{
}

/* Close the stream associated with this connection */
int
Openflow_stream_connection::close()
{
    return stream->close();
}

int
Openflow_stream_connection::do_connect()
{
    return stream->get_connect_error();
}

void
Openflow_stream_connection::do_connect_wait()
{
    stream->connect_wait();
}

void
Openflow_stream_connection::tx_run()
{
    if (tx_buf.get() && send_tx_buf() == EAGAIN) {
        do_send_openflow_wait();
    }
    co_fsm_block();
}

int Openflow_stream_connection::send_tx_buf()
{
    ssize_t bytes_written;
    int error = stream->write_fully(*tx_buf, &bytes_written, false);
    tx_buf->pull(bytes_written);

    if (error) {
        if (error != EAGAIN) {
            stream->close(); 
        }
        return error;
    }

    if (!tx_buf->size()) {
        tx_buf.reset();
        return 0;
    } else {
        return EAGAIN;
    }
}

int Openflow_stream_connection::do_send_openflow(const ofp_header* oh)
{
    if (tx_buf.get()) {
        int error = send_tx_buf();
        if (error) {
            return error;
        }
    }

    /* FIXME: shouldn't need a copy here in most cases */
    tx_buf.reset(new Array_buffer(ntohs(oh->length)));
    memcpy(tx_buf->data(), oh, ntohs(oh->length));
    int error = send_tx_buf();
    if (error == EAGAIN) {
        tx_fsm.wake();
        return 0;
    } else {
        return error;
    }
}

int Openflow_stream_connection::do_read(void *p, size_t need_bytes)
{
    Nonowning_buffer b(p, need_bytes);
    ssize_t n = stream->read(b, false);
    if (n > 0) {
        rx_bytes += n;
        return n < need_bytes ? EAGAIN : 0;
    } else if (n == -EAGAIN) {
        return EAGAIN;
    } else {
        stream->close();
        if (n == 0) {
            if (!rx_bytes) {
                return EOF;
            } else {
                log.warn("%s: unexpected connection drop in middle "
                         "of a message", to_string().c_str());
                return EPROTO;
            }
        } else {
            return -n;
        }
    }
}

std::auto_ptr<Buffer> Openflow_stream_connection::do_recv_openflow(int& error)
{
    if (rx_bytes < sizeof rx_header) {
        error = do_read((char *) &rx_header + rx_bytes,
                        sizeof rx_header - rx_bytes);
        if (error) {
            return std::auto_ptr<Buffer>(0);
        }

        size_t length = ntohs(rx_header.length);
        if (length < sizeof rx_header) {
            log.warn("%s: received length (%zu) claims to be shorter than "
                     "header", to_string().c_str(), length);
            stream->close();
            error = EPROTO;
            return std::auto_ptr<Buffer>(0);
        }
        rx_buf.reset(new Array_buffer(length));
        memcpy(rx_buf->data(), &rx_header, sizeof rx_header);
    }

    if (size_t need_bytes = ntohs(rx_header.length) - rx_bytes) {
        error = do_read(rx_buf->data() + rx_bytes, need_bytes);
        if (error) {
            return std::auto_ptr<Buffer>(0);
        }
    } else {
        /* OpenFlow packet is exactly sizeof(ofp_header) bytes long. */
        error = 0;
    }
    rx_bytes = 0;
    return rx_buf;
}

void
Openflow_stream_connection::do_send_openflow_wait()
{
    stream->write_wait();
}

void
Openflow_stream_connection::do_recv_openflow_wait()
{
    stream->read_wait();
}

std::string Openflow_stream_connection::to_string()
{
    // FIXME
    return "stream";
}

Openflow_connection::Connection_type 
Openflow_stream_connection::get_conn_type() { 
  return conn_type; 
} 

std::string Openflow_stream_connection::get_ssl_fingerprint() { 
  if(get_conn_type() == TYPE_SSL) { 
      Ssl_socket* sock = assert_cast< Ssl_socket* >( &(*stream));
      return sock->get_peer_cert_fingerprint().c_str(); 
  } 
  return "non-SSL connection, no fingerprint"; 
} 

uint32_t Openflow_stream_connection::get_local_ip() { 
  if(get_conn_type() == TYPE_SSL) { 
      Ssl_socket* sock = assert_cast< Ssl_socket* >( &(*stream));
      return sock->get_local_ip(); 
  } else if(get_conn_type() == TYPE_TCP) { 
      Tcp_socket* sock = assert_cast< Tcp_socket* >( &(*stream));
      return sock->get_local_ip(); 
  } 
  return 0; // 0.0.0.0 indiates failure 
} 

uint32_t Openflow_stream_connection::get_remote_ip() { 
  if(get_conn_type() == TYPE_SSL) { 
      Ssl_socket* sock = assert_cast< Ssl_socket* >( &(*stream));
      return sock->get_remote_ip(); 
  } else if(get_conn_type() == TYPE_TCP) { 
      Tcp_socket* sock = assert_cast< Tcp_socket* >( &(*stream));
      return sock->get_remote_ip(); 
  } 
  return 0; // 0.0.0.0 indiates failure 
} 



Reliable_openflow_connection::Reliable_openflow_connection(
    std::auto_ptr<Openflow_connection_factory> f_)
    : f(f_),
      c(NULL),
      backoff(0),
      status(CONN_SLEEPING)
{
    fsm = co_fsm_create(&co_group_coop,
                  boost::bind(&Reliable_openflow_connection::run, this));
    co_fsm_run(fsm);
}

Reliable_openflow_connection::~Reliable_openflow_connection()
{
    co_fsm_kill(fsm);
}

int
Reliable_openflow_connection::do_connect()
{
    return 0;
}

void
Reliable_openflow_connection::do_connect_wait()
{
    co_immediate_wake(1, NULL);
}

int
Reliable_openflow_connection::do_send_openflow(const ofp_header* oh)
{
    if (status == CONN_CONNECTED) {
        int error = c->send_openflow(oh, false);
        if (error == 0 || error == EAGAIN) {
            return error;
        } else {
            reconnect(error);
            co_fsm_run(fsm);
        }
    }
    return 0;
}

void
Reliable_openflow_connection::do_send_openflow_wait()
{
    if (status == CONN_CONNECTED) {
        c->send_openflow_wait();
    } else {
        connected.wait();
    }
}

std::auto_ptr<Buffer>
Reliable_openflow_connection::do_recv_openflow(int& error)
{
    if (status == CONN_CONNECTED) {
        std::auto_ptr<Buffer> buffer = c->recv_openflow(error, false);
        if (!error || error == EAGAIN) {
            if (!error) {
                backoff = 0;
            }
            return buffer;
        }
        reconnect(error);
        co_fsm_run(fsm);
    }
    error = EAGAIN;
    return std::auto_ptr<Buffer>(0);
}

void
Reliable_openflow_connection::do_recv_openflow_wait()
{
    if (status == CONN_CONNECTED) {
        c->recv_openflow_wait();
    } else {
        connected.wait();
    }
}

void
Reliable_openflow_connection::reconnect(int error)
{
    c.reset();
    backoff = std::min(backoff_limit, std::max(1, backoff * 2));
    status = CONN_SLEEPING;
    log.warn("%s: connection dropped (%s), waiting %d seconds...",
             f->to_string().c_str(), ::strerror(error), backoff);
}

void
Reliable_openflow_connection::run() 
{
    if (status == CONN_SLEEPING) {
        status = CONN_WAKING;
        if (backoff) {
            co_timer_wait(do_gettimeofday() + make_timeval(backoff, 0), NULL);
            co_fsm_block();
            return;
        }
    }
    if (status == CONN_WAKING) {
        if (backoff) {
            log.warn("%s: reconnecting", f->to_string().c_str()); 
        }

        int error;
        c.reset(f->connect(error));
        if (error) {
            reconnect(error);
            co_fsm_yield();
            return;
        }
        status = CONN_CONNECTING;
    }
    if (status == CONN_CONNECTING) {
        int error = c->connect(false);
        if (!error) { 
            status = CONN_CONNECTED;
            connected.broadcast();
            co_fsm_rest();
        } else if (error != EAGAIN) {
            reconnect(error);
            co_fsm_yield();
        } else {
            c->connect_wait();
            co_fsm_block();
        }
        return;
    }
}

int
Reliable_openflow_connection::close()
{
    c->close();
    return 0;
}

std::string
Reliable_openflow_connection::to_string()
{
    return f->to_string() + " (reliable)";
}

Openflow_connection::Connection_type 
Reliable_openflow_connection::get_conn_type() { 
    return TYPE_RELIABLE; 
} 


Openflow_connection_factory* Openflow_connection_factory::create(
    const std::string& s)
{
    boost::char_separator<char> separator(":", "", boost::keep_empty_tokens);
    boost::tokenizer<boost::char_separator<char> > tokenizer(s, separator);
    std::vector<std::string> tokens(tokenizer.begin(), tokenizer.end());

    if (tokens.size() < 2) {
        log.err("openflow connection name must contain `:'");
        exit(EXIT_FAILURE);
    }

    if (tokens[0] == "tcp") {
        uint16_t port = tokens.size() >= 3 
                ? atoi(tokens[2].c_str()) : OFP_TCP_PORT;
        return new Tcp_openflow_connection_factory(tokens[1], htons(port));
    } else if (tokens[0] == "ptcp") {
        uint16_t port = atoi(tokens[1].c_str());
        if (!port) {
            port = OFP_TCP_PORT;
        }
        return new Passive_tcp_openflow_connection_factory(htons(port));
    } else if (tokens[0] == "ssl") {
        if (tokens.size() != 6) {
            log.err("ssl connection name not in the form ssl:HOST:[PORT]:KEY:CERT:CAFILE");
            exit(EXIT_FAILURE);
        }
        uint16_t port = atoi(tokens[2].c_str());
        if (!port) {
            port = OFP_SSL_PORT;
        }
        return new Ssl_openflow_connection_factory(
            tokens[1], htons(port), tokens[3].c_str(),
            tokens[4].c_str(), tokens[5].c_str());
    } else if (tokens[0] == "pssl") {
        if (tokens.size() != 5) {
            log.err("pssl connection name not in the form pssl:[PORT]:KEY:CERT:CAFILE");
            exit(EXIT_FAILURE);
        }
        uint16_t port = atoi(tokens[1].c_str());
        if (!port) {
            port = OFP_SSL_PORT;
        }
        return new Passive_ssl_openflow_connection_factory(
            htons(port), tokens[2].c_str(), tokens[3].c_str(),
            tokens[4].c_str());
    } else if (tokens[0] == "pcap") {
#ifndef HAVE_PCAP        
            log.err("pcap support not built in.  Ensure you have pcap installed and rebuild");
            exit(EXIT_FAILURE);
#else            
        if (tokens.size() == 2) {
            return new Pcapreader_factory(tokens[1]);
        }else if (tokens.size() == 3){
            return new Pcapreader_factory(tokens[1], tokens[2]);
        }else{
            log.err("Invalid format for pcap reader\"%s\"", s.c_str());
            exit(EXIT_FAILURE);
        }
#endif            
    } else if(tokens[0] == "pcapt") {
#ifndef HAVE_PCAP        
            log.err("pcap support not built in.  Ensure you have pcap installed and rebuild");
            exit(EXIT_FAILURE);
#else            
        if (tokens.size() == 2) {
            return new Pcapreader_factory(tokens[1],"",true);
        }else if (tokens.size() == 3){
            return new Pcapreader_factory(tokens[1], tokens[2],true);
        }else{
            log.err("Invalid format for pcapt reader\"%s\"", s.c_str());
            exit(EXIT_FAILURE);
        }
#endif            

    } else if (tokens[0] == "pgen") {
        if (tokens.size() == 2) {
            return new Packetgen_factory(atoi(tokens[1].c_str()));
        } else{
            log.err("Invalid format for packet generator \"%s\"", s.c_str());
            exit(EXIT_FAILURE);
        }
#ifdef HAVE_NETLINK
    } else if (tokens[0] == "nl") {
        return new Datapath_factory(atoi(tokens[1].c_str()));
#endif
    } else {
        log.err("unsupported openflow connection type \"%s\"",
              tokens[0].c_str());
        exit(EXIT_FAILURE);
    }
    abort();
}

Tcp_openflow_connection_factory
::Tcp_openflow_connection_factory(const std::string& host_, uint16_t port_)
    : host(host_), port(port_)
{
}

Openflow_connection*
Tcp_openflow_connection_factory::connect(int& error)
{
    std::auto_ptr<Tcp_socket> socket(new Tcp_socket);
    error = socket->connect(host, port, false);
    if (error && error != EAGAIN) {
        return NULL;
    }

    error = 0;
    return new Openflow_stream_connection(
        std::auto_ptr<Async_stream>(socket.release()),
        Openflow_stream_connection::TYPE_TCP);
}

void
Tcp_openflow_connection_factory::connect_wait()
{
    co_immediate_wake(1, NULL);
}

std::string
Tcp_openflow_connection_factory::to_string()
{
    char name[128];
    snprintf(name, sizeof name, "tcp:%s:%"PRIu16, host.string().c_str(), 
             ntohs(port));
    return std::string(name);
}

Passive_tcp_openflow_connection_factory
::Passive_tcp_openflow_connection_factory(uint16_t port_)
    : port(port_)
{
    socket.set_reuseaddr();
    int error = socket.bind(htonl(INADDR_ANY), port);
    if (error) {
        throw errno_exception(error, "bind");
    }


    error = socket.listen(SOMAXCONN);
    if (error) {
        throw errno_exception(error, "listen");
    }
    log.dbg("Passive tcp interface bound to port %d", htons(port));
}

Openflow_connection*
Passive_tcp_openflow_connection_factory::connect(int& error)
{
    std::auto_ptr<Tcp_socket> tsoc = socket.accept(error, false);
    Openflow_connection* c = NULL;
    if (!error) {
        tsoc->set_nodelay(true);
        log.dbg("Passive tcp interface received connection ");
        std::auto_ptr<Async_stream> new_socket(tsoc);
        c = new Openflow_stream_connection(new_socket, 
                Openflow_stream_connection::TYPE_TCP);
    }
    return c;
}

void
Passive_tcp_openflow_connection_factory::connect_wait()
{
    socket.accept_wait();
}

std::string Passive_tcp_openflow_connection_factory::to_string()
{
    char s[16];
    sprintf(s, "ptcp:%d", ntohs(port));
    return s;
}

Ssl_openflow_connection_factory
::Ssl_openflow_connection_factory(const std::string& host_, uint16_t port_,
                                  const char *key, const char *cert,
                                  const char *CAfile)
    : config(new Ssl_config(Ssl_config::SSLv3 | Ssl_config::TLSv1,
                            Ssl_config::AUTHENTICATE_SERVER,
                            Ssl_config::REQUIRE_CLIENT_CERT,
                            key, cert, CAfile)),
      host(host_), port(port_)
{
}

Openflow_connection*
Ssl_openflow_connection_factory::connect(int& error)
{
    std::auto_ptr<Ssl_socket> socket(new Ssl_socket(config));
    error = socket->connect(host, port, NULL, false);
    if (error && error != EAGAIN) {
        return NULL;
    }

    error = 0;
    return new Openflow_stream_connection(
        std::auto_ptr<Async_stream>(socket.release()), 
        Openflow_stream_connection::TYPE_SSL);
}

void
Ssl_openflow_connection_factory::connect_wait()
{
    co_immediate_wake(1, NULL);
}

std::string
Ssl_openflow_connection_factory::to_string()
{
    char name[128];
    snprintf(name, sizeof name, "ssl:%s:%"PRIu16, host.string().c_str(), 
             ntohs(port));
    return std::string(name);
}

Passive_ssl_openflow_connection_factory
::Passive_ssl_openflow_connection_factory(uint16_t port_,
                                          const char *key, const char *cert,
                                          const char *CAfile)
    : config(new Ssl_config(Ssl_config::SSLv3 | Ssl_config::TLSv1,
                            Ssl_config::AUTHENTICATE_SERVER,
                            Ssl_config::REQUIRE_CLIENT_CERT,
                            key, cert, CAfile)),
      socket(config),
      port(port_)
{
    int error = socket.bind(htonl(INADDR_ANY), port);
    if (error) {
        throw errno_exception(error, "bind");
    }


    error = socket.listen(SOMAXCONN);
    if (error) {
        throw errno_exception(error, "listen");
    }
    log.dbg("Passive ssl interface bound to port %d", htons(port));
}

Openflow_connection*
Passive_ssl_openflow_connection_factory::connect(int& error)
{
    std::auto_ptr<Async_stream> new_socket(socket.accept(error, false));
    Openflow_connection* c = NULL;
    if (!error) {
        log.dbg("Passive ssl interface received connection ");
        c = new Openflow_stream_connection(new_socket,
                Openflow_stream_connection::TYPE_SSL);
    }
    return c;
}

void
Passive_ssl_openflow_connection_factory::connect_wait()
{
    socket.accept_wait();
}

std::string Passive_ssl_openflow_connection_factory::to_string()
{
    char s[16];
    sprintf(s, "pssl:%d", ntohs(port));
    return s;
}


} // namespace vigil
