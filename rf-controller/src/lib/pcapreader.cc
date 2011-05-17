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
#include "pcapreader.hh"
#include <boost/bind.hpp>

#include <cerrno>
#include <stdint.h>
#include <netinet/in.h>
#include <stdexcept>
#include <string>

#include "netinet++/ethernetaddr.hh"
#include "buffer.hh"
#include "packets.h"
#include "threads/cooperative.hh"
#include "vlog.hh"

namespace vigil {

static Vlog_module log("pcap");

Pcapreader::Pcapreader(const std::string& in_file_, const std::string& out_file_, bool use_delay_)
    : in_file(in_file_), out_file(out_file_), pc(0), pd(0), hs_done(false), hello_rdy(false), use_delay(use_delay_), delayed_pcap_data(NULL) 
{
    char pcerr[PCAP_ERRBUF_SIZE];

    pc = ::pcap_open_offline(in_file.c_str(), pcerr);
    if (!pc){
        log.err("PCAP Open Failed: %s \n", pcerr);
        throw std::runtime_error(pcerr);
    }
    if (out_file_ != ""){
        pd = ::pcap_dump_open(pc, out_file.c_str());
        if (!pd) {
            throw std::runtime_error("unable to open file "+out_file);
        }
    }
    read_start_time = make_timeval(0,0);
    pcap_start_time = make_timeval(0,0);
}

Pcapreader::~Pcapreader()
{
    if (pc) {
        pcap_close(pc);
    }
    if (pd) {
        pcap_dump_close(pd);
    }
}

int Pcapreader::do_connect() 
{
    return 0;
}

void Pcapreader::do_connect_wait()
{
    co_immediate_wake(1, NULL);
}

void Pcapreader::do_send_openflow_wait()
{
    co_immediate_wake(1, NULL);
}

void Pcapreader::do_recv_openflow_wait()
{
    // if we are delaying packets, and we did 
    // not return a packet last time do_openflow_recv()
    // was called, then tell the thread to wait
    if(use_delay && next_pcap_time.tv_sec != 0) {
      struct timeval wait;
      wait.tv_sec = next_pcap_time.tv_sec;
      wait.tv_usec = next_pcap_time.tv_usec;
      co_timer_wait(wait, NULL); 
    }else // don't wait 
      co_immediate_wake(1, NULL); 
}

int Pcapreader::do_send_openflow(const ofp_header* oh)
{
    if (oh->type == OFPT_FEATURES_REQUEST) {
        log.dbg("received features request packet\n");
        size_t size = sizeof(ofp_switch_features) + sizeof(ofp_phy_port);
        std::auto_ptr<Buffer> b(new Array_buffer(size));
        ofp_switch_features* osf = &b->at<ofp_switch_features>(0);
        memset(osf, 0, size);
        osf->header.type = OFPT_FEATURES_REPLY;
        osf->header.version = OFP_VERSION;
        osf->header.length = htons(size);
        osf->header.xid = oh->xid;
        osf->datapath_id = htonll(UINT64_C(0x0000123456789abc));
        osf->ports[0].port_no = htons(0);
        ::memcpy(osf->ports[0].hw_addr, "\x02\x00\x12\x34\x56\x78", ETH_ADDR_LEN);
        osf->ports[0].config = htonl(0);
        osf->ports[0].state = htonl(0);
        osf->ports[0].curr = htonl(OFPPF_100MB_HD);
        osf->ports[0].advertised = htonl(OFPPF_100MB_HD);
        osf->ports[0].supported = htonl(OFPPF_100MB_HD);
        osf->ports[0].peer = htonl(0);
        next = b;
        hs_done = true;
    } else if (oh->type == OFPT_SET_CONFIG) {
        log.dbg("received SET_CONFIG\n");
        /* We ignore request to set the switch configuration */
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
        if (pd) {
            if (oh->type == OFPT_PACKET_OUT) {
                struct pcap_pkthdr hdr;
                struct ofp_packet_out* opo = (ofp_packet_out*)oh;
                int actions_len = ntohs(opo->actions_len);
                int data_len = ntohs(oh->length) - sizeof *opo - actions_len;

                hdr.len      = data_len;
                hdr.caplen   = data_len;
                // hdr.ts     = 0; /* FIXME */

                pcap_dump((u_char*)pd, &hdr, 
                        (uint8_t *)opo->actions + actions_len);
            } else {
                log.dbg("received non-OFPT_PACKET_OUT, ignoring\n");
            }
        } else {
            log.dbg("received non-feature request but no outfile specified, ignoring\n");
        }
    }
    return 0;
}

std::auto_ptr<Buffer> Pcapreader::do_recv_openflow(int& error)
{
    // -- wait for controller hello before sending
    if(hello_rdy){
        if (next.get()) {
            log.dbg("\'receiving\' HELLO\n");
            error = 0;
            return next;
        }
    }

    // Until the feature request has been received, we don't want to read 
    // from the file (as per the openflow protocol)
    if (! hs_done){
        // FIXME, don't want to loop endlessly
        error = EAGAIN;
        return std::auto_ptr<Buffer>(0);
    }

    // Send out features reply if pending
    if (next.get()) {
        error = 0;
        return next;
    }

    // if we don't already have a packet saved from a past 
    // call to pcap_next, grab one 
    if(delayed_pcap_data == NULL) { 
      delayed_pcap_data = ::pcap_next(pc, &delayed_pcap_header);
      if(!delayed_pcap_data){
        /* End of file. */
        error = EOF;
        return std::auto_ptr<Buffer>(0);
      }
    } 

    if(use_delay) { 
      timeval cur_time = do_gettimeofday(); 
      timeval pcap_time = make_timeval(delayed_pcap_header.ts.tv_sec,
                                      delayed_pcap_header.ts.tv_usec);
      
      // initialize start times that will synch realtime with pcap time
      if(read_start_time.tv_sec == 0) {
        read_start_time = cur_time; 
        pcap_start_time = pcap_time; 
      }

      timeval realtime_diff = cur_time - read_start_time; 
      timeval pcap_diff = pcap_time - pcap_start_time;

      if(pcap_diff > realtime_diff){
        // tell caller to try again later
        // set next_pcap_time so recv_openflow_wait() knows how long to wait
        next_pcap_time = (pcap_diff - realtime_diff) + cur_time; 
        error = EAGAIN;
        return std::auto_ptr<Buffer>(0);
      }else {
        next_pcap_time.tv_sec = next_pcap_time.tv_usec = 0; 
      }
    } 

    std::auto_ptr<Buffer> b(new Array_buffer(sizeof(ofp_packet_in) + delayed_pcap_header.len));
    ofp_packet_in* opi = &b->at<ofp_packet_in>(0);
    opi->header.type = OFPT_PACKET_IN;
    opi->header.version = OFP_VERSION;
    opi->header.length = htons(b->size());
    opi->buffer_id = UINT32_MAX;
    opi->total_len = htons(delayed_pcap_header.len);
    opi->in_port = htons(0);
    opi->reason = OFPR_NO_MATCH;
    ::memcpy(opi->data, delayed_pcap_data, delayed_pcap_header.len);
    delayed_pcap_data = NULL; 
    error = 0;
    return b;
}

int Pcapreader::close()
{
    if (pc) {
        pcap_close(pc);
    }
    if (pd) {
        pcap_dump_close(pd);
    }
    return 0;
}

std::string Pcapreader::to_string()
{

    return "pcap:" + in_file;
}

Openflow_connection::Connection_type 
Pcapreader::get_conn_type() { 
  return Openflow_connection::TYPE_PCAPREADER; 
} 

Pcapreader_factory::Pcapreader_factory(std::string in_file_, std::string out_file_, 
                                bool delayed_)
    : in_file(in_file_), out_file(out_file_), opened_once(false), delayed(delayed_)
{
}

Openflow_connection* Pcapreader_factory::connect(int& error)
{
    try {
        if (opened_once) {
            error = EAGAIN;
            return NULL;
        }

        opened_once = true;
        error = 0;
        return new Pcapreader(in_file, out_file, delayed);
    } catch (std::runtime_error) {
        /* FIXME: Probably a missing file, but... */
        error = ENOENT;
        return NULL;
    }
}

void Pcapreader_factory::connect_wait()
{
    /* Since no event is set here, the subsequent co_block() will
       block indefinitely. */
}

std::string Pcapreader_factory::to_string()
{
    return "pcap:" + in_file;
}

} // namespace vigil
