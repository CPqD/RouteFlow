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
#ifndef PACKET_GEN_HH
#define PACKET_GEN_HH

#include "buffer.hh"
#include "openflow.hh"

namespace vigil {

class Packetgen
    : public Openflow_connection
{
public:
    Packetgen(int num);
    ~Packetgen() {} ;
    int  close();
    std::string to_string();
    Openflow_connection::Connection_type get_conn_type(); 

private:
    bool hs_done;   /* Handshake complete with controller */
    bool hello_rdy;   /* Ready to send hello to the controller */
    Array_buffer data;
    std::auto_ptr<Buffer> next;

    int packets_left;

    virtual int do_connect();
    virtual int do_send_openflow(const ofp_header*);
    virtual std::auto_ptr<Buffer> do_recv_openflow(int& error);
    virtual void do_connect_wait();
    virtual void do_send_openflow_wait();
    virtual void do_recv_openflow_wait();
};

class Packetgen_factory
    : public Openflow_connection_factory
{
public:
    Packetgen_factory(int num = -1);
    Openflow_connection* connect(int& error);
    void connect_wait();
    std::string to_string();
    bool passive() { return true; }
private:    
    int num_packets;
    bool opened_once;
};

} // namespace vigil

#endif // PACKET_GEN_HH
