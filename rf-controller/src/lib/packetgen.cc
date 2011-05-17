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
#include "packetgen.hh"
#include <cerrno>
#include <stdint.h>

// #include "packets.h"
#include "netinet++/ethernet.hh"
#include "netinet++/ip.hh"
#include "threads/cooperative.hh"
#include "vlog.hh"

#define DEFAULT_SIZE 64

namespace vigil {

static Vlog_module log("packetgen");

struct udp {
    uint16_t sport;
    uint16_t dport;
    uint16_t len;
    uint16_t csum;
}__attribute__ ((__packed__));

Packetgen::Packetgen(int num)
    : hs_done(false), hello_rdy(false), data(DEFAULT_SIZE), packets_left(num)
{
    ethernet *eh = (ethernet*)data.data();
    try {
        eh->daddr = "12:34:56:78:9a:bc";
        eh->saddr = "ab:cd:ef:12:34:56";
    } 
    catch (const bad_ethernetaddr_cast& e) {
        /* Won't be thrown with the above strings */
        assert(0);
    }
    eh->type = ethernet::IP;
    ip_ *ih = (ip_*)eh->data();
    ih->ihl = 5;
    ih->ver = 4;
    ih->tos = 0;
    ih->tot_len = htons(DEFAULT_SIZE - sizeof(*eh));
    ih->id = htons(1234);
    ih->frag_off = 0;
    ih->ttl = ip_::DEFTTL;
    ih->protocol = ip_::proto::UDP;
    ih->csum = 0;
    ih->saddr = 0x1234dcba;
    ih->daddr = 0x4321abcd;
    ih->csum = htons(ih->calc_csum());
    udp *uh = (udp*)ih->data();
    uh->sport = htons(2345);
    uh->dport = htons(7654);
    uh->len = htons(DEFAULT_SIZE - sizeof(*eh) - sizeof(*ih));
    uh->csum = 0;
}


int
Packetgen::do_send_openflow(const ofp_header* oh)
{
    if (oh->type == OFPT_FEATURES_REQUEST) {
        log.dbg("sending features reply\n");
        size_t size = sizeof(ofp_switch_features) + sizeof(ofp_phy_port);
        next.reset(new Array_buffer(size));
        ofp_switch_features* osf = &next->at<ofp_switch_features>(0);
        memset(osf, 0, size);
        osf->header.type = OFPT_FEATURES_REPLY;
        osf->header.version = OFP_VERSION;
        osf->header.length = htons(size);
        osf->header.xid = oh->xid;
        osf->datapath_id = htonll(UINT64_C(0x0000123456789abc));
        osf->ports[0].port_no = htons(0);
        ::memcpy(osf->ports[0].hw_addr, "\x02\x00\x12\x34\x56\x78", ethernetaddr::LEN);
        osf->ports[0].config = htonl(0);
        osf->ports[0].state = htonl(0);
        osf->ports[0].curr = htonl(OFPPF_100MB_HD);
        osf->ports[0].advertised = htonl(OFPPF_100MB_HD);
        osf->ports[0].supported = htonl(OFPPF_100MB_HD);
        osf->ports[0].peer = htonl(0);
        hs_done = true;
    } else if (oh->type == OFPT_SET_CONFIG) {
        log.dbg("received SET_CONFIG\n");
        /* We ignore requests to set the switch configuration */
    } else if (oh->type == OFPT_HELLO) {
        log.dbg("received HELLO\n");
        next.reset(new Array_buffer(sizeof(ofp_header)));
        ofp_header* hello =  &next->at<ofp_header>(0) ;
        hello->version = OFP_VERSION;
        hello->type    = OFPT_HELLO;
        hello->length  = htons(sizeof(ofp_header));
        hello->xid = 0;
        hello_rdy  = true;
    } else {
        log.dbg("received non-feature request %x, ignoring\n",
                oh->type);
    }
    return 0;
}

std::auto_ptr<Buffer> Packetgen::do_recv_openflow(int& error)
{
    // -- wait for controller hello before sending
    if(hello_rdy){
        if (next.get()) {
            log.dbg("\'receiving\' HELLO\n");
            error = 0;
            return next;
        }
    }

    // wait til features reply has been sent out
    if (!hs_done){
        // FIXME, don't want to loop endlessly
        // log.dbg("waiting for features request\n");
        error = EAGAIN;
        return std::auto_ptr<Buffer>(0);
    }

    // Send out features reply if pending
    if (next.get()) {
        log.dbg("\'receiving\' features request\n");
        error = 0;
        return next;
    }

    if (packets_left == 0){
        error = EOF;
        return std::auto_ptr<Buffer>(0);
    }else if (packets_left != -1){
        packets_left--;
    }

    std::auto_ptr<Buffer> b(new Array_buffer(sizeof(ofp_packet_in)
                                             + DEFAULT_SIZE));
    ofp_packet_in* opi = &b->at<ofp_packet_in>(0);
    opi->header.type = OFPT_PACKET_IN;
    opi->header.version = OFP_VERSION;
    opi->header.length = htons(b->size());
    opi->buffer_id = UINT32_MAX;
    opi->total_len = htons(DEFAULT_SIZE);
    opi->in_port = htons(0);
    opi->reason = OFPR_NO_MATCH;
    ::memcpy(opi->data, data.data(), DEFAULT_SIZE);
    error = 0;
    return b;
}

int Packetgen::do_connect()
{
    return 0;
}

void
Packetgen::do_connect_wait()
{
    co_immediate_wake(1, NULL);
}

void
Packetgen::do_send_openflow_wait()
{
    co_immediate_wake(1, NULL);
}

void
Packetgen::do_recv_openflow_wait()
{
    co_immediate_wake(1, NULL);
}

std::string
Packetgen::to_string()
{
    return "pgen:";
}

int
Packetgen::close()
{
    return 0;
}

Openflow_connection::Connection_type 
Packetgen::get_conn_type() { 
  return Openflow_connection::TYPE_PACKET_GEN; 
} 

Packetgen_factory::Packetgen_factory(int num):
    num_packets(num), opened_once(false)
{
//     Vlog::mod()[Vlog::MOD_TEST].set_level(Vlog::FACILITY_CONSOLE, Vlog::LEVEL_DBG);
}

Openflow_connection* Packetgen_factory::connect(int& error) 
{
    if (opened_once){
        error = EAGAIN;
        return NULL;
    }
    opened_once = true;
    error = 0;
    return new Packetgen(num_packets);
}

void Packetgen_factory::connect_wait() 
{
    /* Since no event is set here, the subsequent co_block() will
       block indefinitely. */
}


std::string
Packetgen_factory::to_string()
{
    return "pgen:";
}


}
