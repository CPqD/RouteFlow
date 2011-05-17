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
/* Interface to datapath. */

#ifndef DATAPATH_HH
#define DATAPATH_HH 1

#include <string>
#include "netlink.hh"
#include "openflow.hh"

namespace vigil {

class Buffer;

class Datapath
    : public Openflow_connection
{
public:
    /* Construct a datapath for the given 'dp_idx' (usually 0).
     * The datapath with the given index need not actually exist, but if it
     * doesn't then you'll only be able to do add_dp(). */
    Datapath(int dp_idx_);

    /* One or the other of these must be called before any other member
     * function will work.  Use subscribe() if you want to receive all the
     * packet_in traffic, bind() otherwise. */
    int bind();
    int subscribe(bool block);

    /* datapath manipulation.
     *
     * These act on the datapath specified in the constructor. */
    int add_dp();
    int del_dp();
    int add_port(const std::string& netdev);
    int del_port(const std::string& netdev);

    int close();

    virtual std::string to_string();
    Openflow_connection::Connection_type get_conn_type(); 

private:
    static Genl_family openflow_family;

    Nl_socket sock;
    int dp_idx;

    /* These members are all for handling non-blocking subscribe calls. */
    enum {
        CONN_NOT_STARTED = -1,
        CONN_GET_FAMILY = -2,
        CONN_GET_MULTICAST_GROUP = -3
    };
    int connect_status;
    std::auto_ptr<Genl_message> request;
    std::auto_ptr<Nl_transaction> transaction;

    Datapath(const Datapath&);
    Datapath& operator=(const Datapath&);

    virtual int do_connect();
    virtual int do_send_openflow(const ofp_header*);
    virtual std::auto_ptr<Buffer> do_recv_openflow(int& error);
    virtual void do_connect_wait();
    virtual void do_send_openflow_wait();
    virtual void do_recv_openflow_wait();

    void start_subscribe();
    void connect_state_machine();

    int send_mgmt_command(int command, const std::string* netdev);
};

class Datapath_factory
    : public Openflow_connection_factory
{
public:
    Datapath_factory(int dp_idx);
    Openflow_connection* connect(int& error);
    void connect_wait();
    std::string to_string();
private:
    int dp_idx;
};

} // namespace vigil

#endif /* datapath.hh */
