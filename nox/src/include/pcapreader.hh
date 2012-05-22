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
#ifndef PCAPREADER_HH
#define PCAPREADER_HH

#include <pcap.h>
#include <string>
#include "openflow.hh"
#include "timeval.hh" 

namespace vigil {

class Pcapreader
    : public Openflow_connection
{
public:
    explicit Pcapreader(const std::string& in_file, const std::string& outfile = "", bool use_delay_ = false);
    ~Pcapreader();
    int close();
    std::string to_string();
    Openflow_connection::Connection_type get_conn_type(); 
private:
    std::string in_file;
    std::string out_file;
    pcap_t* pc;
    pcap_dumper_t* pd;
    bool hs_done;
    bool hello_rdy;
    std::auto_ptr<Buffer> next;
    
    // used for pcap time delay 
    bool use_delay; 
    timeval read_start_time, pcap_start_time, next_pcap_time; 
    const u_char *delayed_pcap_data;    
    struct pcap_pkthdr delayed_pcap_header;

    virtual int do_connect();
    virtual int do_send_openflow(const ofp_header*);
    virtual std::auto_ptr<Buffer> do_recv_openflow(int& error);
    virtual void do_connect_wait();
    virtual void do_send_openflow_wait();
    virtual void do_recv_openflow_wait();
};

class Pcapreader_factory
    : public Openflow_connection_factory
{
public:
    explicit Pcapreader_factory(std::string in_file, std::string out_file = "", 
                                            bool delayed = false);
    Openflow_connection* connect(int& error);
    void connect_wait();
    std::string to_string();
    bool passive() { return true; }

private:
    std::string in_file;
    std::string out_file;
    bool opened_once;
    bool delayed; 
};

} // namespace vigil

#endif // PCAPREADER_HH
