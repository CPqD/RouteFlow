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
#ifndef OPENFLOW_HH
#define OPENFLOW_HH 1

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <inttypes.h>
#include <list>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>
#include "netinet++/datapathid.hh"
#include "netinet++/ipaddr.hh"
#include "netinet++/ethernetaddr.hh"
#include "threads/cooperative.hh"
#include "openflow/openflow.h"
#include "tcp-socket.hh"
#include "ssl-socket.hh"

#define OFP_EXPERIMENTAL_VERSION 0x80

namespace vigil {

class Buffer;
class Async_stream;
class Async_datagram;
class Openflow_connection_factory;

/* Abstract class for a connection that carries OpenFlow traffic. */
class Openflow_connection
{
public:
    Openflow_connection();
    virtual ~Openflow_connection() { }

    /* Versioning. */
    int get_min_version();
    void set_min_version(int min_version);
    int get_version();
    
    enum Connection_type { TYPE_UNKNOWN, TYPE_TCP, TYPE_SSL, TYPE_PACKET_GEN,
                          TYPE_DATAPATH, TYPE_PCAPREADER, TYPE_RELIABLE}; 
    virtual Connection_type get_conn_type() = 0;
    virtual std::string get_ssl_fingerprint() { 
        return "non-SSL connection, no fingerprint"; 
    } 
    virtual uint32_t get_local_ip() { 
        return 0; // 0.0.0.0 indicates failure
    }
    virtual uint32_t get_remote_ip() { 
        return 0; // 0.0.0.0 indicates failure
    }
    /* Core functionality. */
    int connect(bool block);
    int send_openflow(const ofp_header*, bool block);
    std::auto_ptr<Buffer> recv_openflow(int& error, bool block);

    // Buffer to handle OFMP extended data messages
    std::auto_ptr<Buffer> ext_data_buffer;
    uint32_t ext_data_xid;

    void connect_wait();
    void send_openflow_wait();
    void recv_openflow_wait();

    /* Convenience functions. */
    int send_packet(const Buffer& packet, uint16_t out_port,
                    uint16_t in_port, bool block);
    int send_packet(const Buffer& packet, const ofp_action_header actions[],
                    size_t actions_len, uint16_t in_port, bool block);
    int send_packet(uint32_t buffer_id, uint16_t out_port,
                    uint16_t in_port, bool block);
    int send_packet(uint32_t buffer_id, const ofp_action_header actions[],
                    size_t actions_len, uint16_t in_port, bool block);
    int send_features_request();
    int send_switch_config();
    int send_stats_request(ofp_stats_types type);
    int send_echo_request();
    int send_echo_reply(const ofp_header* request);
    int send_barrier_request();
    int send_add_snat(uint16_t port, 
                    uint32_t ip_addr_start, uint32_t ip_addr_end,
                    uint16_t tcp_start, uint16_t tcp_end,
                    uint16_t udp_start, uint16_t udp_end,
                    ethernetaddr mac_addr, 
                    uint16_t mac_timeout=0);
    int send_add_snat(uint16_t port, uint32_t ip_addr);
    int send_del_snat(uint16_t port);
    int send_remote_command(const std::string& command,
                            const std::vector<std::string>& args);

    int send_ofmp_capability_request();
    int send_ofmp_resources_request();
    int send_ofmp_config_request();

    /* Naming. */
    virtual std::string to_string() = 0;

    virtual int close() = 0;

    /* Allow events to be associated with a particular datapath */
    void set_datapath_id(datapathid id_) { datapath_id = id_; }
    datapathid get_datapath_id() const { return datapath_id; }

    /* Allow events to be associated with a particular datapath */
    void set_mgmt_id(datapathid id_) { mgmt_id = id_; }
    datapathid get_mgmt_id() const { return mgmt_id; }

protected:
    virtual int do_connect() = 0;
    virtual int do_send_openflow(const ofp_header*) = 0;
    virtual std::auto_ptr<Buffer> do_recv_openflow(int& error) = 0;
    virtual void do_connect_wait() = 0;
    virtual void do_send_openflow_wait() = 0;
    virtual void do_recv_openflow_wait() = 0;

private:
    datapathid datapath_id;
    datapathid mgmt_id;
    enum State {
        /* This is the ordinary progression of states. */
        S_CONNECTING,           /* Underlying vconn is not connected. */
        S_SEND_HELLO,           /* Waiting to send OFPT_HELLO message. */
        S_RECV_HELLO,           /* Waiting to receive OFPT_HELLO message. */
        S_CONNECTED,            /* Connection established. */
        S_IDLE,                 /* Nothing received in a while. */

        /* These states are entered only when something goes wrong. */
        S_SEND_ERROR,           /* Sending OFPT_ERROR message. */
        S_DISCONNECTED          /* Connection failed or connection closed. */
    } state;
    int error;                  /* Valid only in S_DISCONNECTED. */
    int version;                /* Valid only in S_CONNECTED. */
    int min_version;            /* Minimum acceptable version. */
    timeval timeout;            /* S_CONNECTED or S_IDLE receive timeout. */
    Auto_fsm fsm;

    /* Number of seconds before a connected channel is considered idle. */
    static const int probe_interval;

    void s_connecting();
    void s_send_hello();
    void s_recv_hello();
    void s_send_error();
    bool need_to_wait_for_connect();
    int call_send_openflow(const ofp_header*);
    std::auto_ptr<Buffer> call_recv_openflow(int& error);
    void run();
};

/* Wrapper for carrying OpenFlow traffic over a stream connection, e.g. TCP or
 * SSL. */
class Openflow_stream_connection
    : public Openflow_connection
{
public:
    Openflow_stream_connection(std::auto_ptr<Async_stream>);

    Openflow_stream_connection(std::auto_ptr<Async_stream>, Connection_type t);
    int get_connect_error();
    void connect_wait();
    void send_openflow_wait();
    void recv_openflow_wait();
    int  close();
    std::string to_string();
    Connection_type get_conn_type(); 
    std::string get_ssl_fingerprint();
    uint32_t get_local_ip();  
    uint32_t get_remote_ip();  
private:
    virtual int do_connect();
    virtual int do_send_openflow(const ofp_header*);
    virtual std::auto_ptr<Buffer> do_recv_openflow(int& error);
    virtual void do_connect_wait();
    virtual void do_send_openflow_wait();
    virtual void do_recv_openflow_wait();

    Auto_fsm tx_fsm;
    void tx_run();
    int send_tx_buf();

    int do_read(void *, size_t);

    std::auto_ptr<Async_stream> stream;

    size_t rx_bytes;
    ofp_header rx_header;
    std::auto_ptr<Buffer> rx_buf;

    std::auto_ptr<Buffer> tx_buf;
    Connection_type conn_type; 
};

/* Wrapper class that reconnects, with exponential backoff, when an underlying
 * connection fails.
 *
 * (The actual sending and receiving of packets is *not* any more reliable.  If
 * you're not connected, you can't send or receive packets.  But it'll keep
 * trying to reconnect.)
 */
class Reliable_openflow_connection
    : public Openflow_connection
{
public:
    Reliable_openflow_connection(std::auto_ptr<Openflow_connection_factory>);
    ~Reliable_openflow_connection();
    int close();
    std::string to_string();
    Connection_type get_conn_type(); 
private:
    std::auto_ptr<Openflow_connection_factory> f;
    std::auto_ptr<Openflow_connection> c;
    int backoff;
    co_thread* fsm;
    Co_cond connected;
    enum {
        CONN_SLEEPING,
        CONN_WAKING,
        CONN_CONNECTING,
        CONN_CONNECTED
    } status;

    virtual int do_connect();
    virtual int do_send_openflow(const ofp_header*);
    virtual std::auto_ptr<Buffer> do_recv_openflow(int& error);
    virtual void do_connect_wait();
    virtual void do_send_openflow_wait();
    virtual void do_recv_openflow_wait();

    void reconnect(int error);
    void run();

    static const int backoff_limit;
};

/* To carry OpenFlow traffic over Netlink, see datapath.hh.
 * To receive fake OpenFlow traffic from a Pcap file, see pcapreader.hh. */

/* Factories. */

/* Abstract class for opening a Openflow_connection. */
class Openflow_connection_factory
{
public:
    virtual ~Openflow_connection_factory() { }
    static Openflow_connection_factory* create(const std::string& s);
    virtual Openflow_connection* connect(int& error) = 0;
    virtual void connect_wait() = 0;
    virtual std::string to_string() = 0;
    virtual bool passive() { return false; }
};

class Tcp_openflow_connection_factory
    : public Openflow_connection_factory
{
public:
    Tcp_openflow_connection_factory(const std::string& host, uint16_t port);
    Openflow_connection* connect(int& error);
    void connect_wait();
    std::string to_string();
private:
    ipaddr host;
    uint16_t port;
};

class Passive_tcp_openflow_connection_factory
    : public Openflow_connection_factory
{
public:
    Passive_tcp_openflow_connection_factory(uint16_t port);
    Openflow_connection* connect(int& error);
    void connect_wait();
    std::string to_string();
    bool passive() { return true; }
private:
    Tcp_socket socket;
    uint16_t port;
};

class Ssl_openflow_connection_factory
    : public Openflow_connection_factory
{
public:
    Ssl_openflow_connection_factory(const std::string& host, uint16_t port,
                                    const char* key, const char* cert,
                                    const char* CAfile);
    Openflow_connection* connect(int& error);
    void connect_wait();
    std::string to_string();
private:
    boost::shared_ptr<Ssl_config> config;
    ipaddr host;
    uint16_t port;
};

class Passive_ssl_openflow_connection_factory
    : public Openflow_connection_factory
{
public:
    Passive_ssl_openflow_connection_factory(uint16_t port, const char *key,
                                            const char *cert,
                                            const char *CAfile);
    Openflow_connection* connect(int& error);
    void connect_wait();
    std::string to_string();
    bool passive() { return true; }
private:
    boost::shared_ptr<Ssl_config> config;
    Ssl_socket socket;
    uint16_t port;
};

} // namespace vigil

#endif /* openflow.hh */
