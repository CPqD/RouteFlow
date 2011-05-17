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

#include "nox.hh"

#include <boost/bind.hpp>
#include <boost/shared_array.hpp>
#include <errno.h>
#include <map>
#include <signal.h>
#include "kernel.hh" 
#include "assert.hh"
#include "buffer.hh"
#include "cfg.hh"
#include "datapath-join.hh"
#include "datapath-leave.hh"
#include "echo-request.hh"
#include "event-dispatcher.hh"
#include "flow-mod-event.hh"
#include "ofmp-config-update.hh"
#include "ofmp-config-update-ack.hh"
#include "ofmp-resources-update.hh"
#include "openflow.hh"
#include "openflow/nicira-ext.h"
#include "openflow/openflow-mgmt.h"
#include "openflow-event.hh"
#include "poll-loop.hh"
#include "shutdown-event.hh"
#include "string.hh"
#include "switch-mgr.hh"
#include "switch-mgr-join.hh"
#include "switch-mgr-leave.hh"
#include "threads/signals.hh"
#include "timeval.hh"
#include "vlog.hh"
#include "switch_auth.hh"

namespace vigil {
namespace nox {

static Vlog_module lg("nox");

static Packet_classifier classifier;

static const int N_THREADS = 8;
static Poll_loop* main_loop;
static Event_dispatcher event_dispatcher;
static Timer_dispatcher timer_dispatcher;
static Switch_Auth *switch_authenticator = NULL; 

class Conn
    : public Pollable {
public:
    boost::shared_ptr<Openflow_connection> oconn;
    Co_sema* disconnected;

    Conn(boost::shared_ptr<Openflow_connection> oconn_,
         Co_sema* disconnected_);
    ~Conn();

    bool poll();
    void wait();
    
    void close();
private:
    bool closing;
    int poll_cnt;

    bool do_poll();
};

// DPID to connection mappings 
typedef std::map<datapathid, Conn*> chashmap;
static chashmap connection_map;

// DPID to management ID mappings 
typedef std::map<datapathid, datapathid> mhashmap;
static mhashmap mgmt_map;

// Management ID to switch monitor object mappings 
typedef std::map<datapathid, boost::shared_ptr<Switch_mgr> > swmhashmap;
static swmhashmap swm_map;


Conn::Conn(boost::shared_ptr<Openflow_connection> oconn_,
           Co_sema* disconnected_)
    : oconn(oconn_),
      disconnected(disconnected_),
      closing(false),
      poll_cnt(0)
{
    main_loop->add_pollable(this);
}

Conn::~Conn()
{
    close();
}

static boost::shared_ptr<Openflow_connection>
dpid_to_oconn(datapathid dpid)
{
    chashmap::iterator iter = connection_map.find(dpid);
    if (iter == connection_map.end()) {
        lg.err("no datapath with id %s registered at nox",
               dpid.string().c_str());
        return boost::shared_ptr<Openflow_connection>();
    }
    return iter->second->oconn;
}

boost::shared_ptr<Switch_mgr>
mgmtid_to_swm(datapathid mgmt_id)
{
    swmhashmap::iterator iter = swm_map.find(mgmt_id);
    if (iter == swm_map.end()) {
        // this happens legitimately due to a race between
        // the switch connection and the management connection
        lg.dbg("no manager with id %s registered at nox",
               mgmt_id.string().c_str());
        return boost::shared_ptr<Switch_mgr>();
    }
    return iter->second;
}

// Given a datapath id, returns a management id.  If one cannot be
// found, returns a 'datapathid' object with value of 0.
datapathid
dpid_to_mgmtid(datapathid dpid)
{
    mhashmap::iterator iter = mgmt_map.find(dpid);
    if (iter == mgmt_map.end()) {
        lg.err("no manager found for datapath id %s", dpid.string().c_str());
        return datapathid();
    }
    return iter->second;
}

bool
active_mgmt(const datapathid& mgmtid)
{
    for (mhashmap::const_iterator it = mgmt_map.begin();
         it != mgmt_map.end(); ++it)
    {
        if (it->second == mgmtid) {
            return true;
        }
    }
    return false;
}

Disposition
handle_ofmp_config(const Event &e)
{
    const Ofmp_config_update_event& ocu
            = assert_cast<const Ofmp_config_update_event&>(e);

    if (boost::shared_ptr<Switch_mgr> swm = nox::mgmtid_to_swm(ocu.mgmt_id)) {
        swm->set_config(ocu.new_config);
    } else {
        lg.warn("got config update for unknown switch mgr %s", 
                ocu.mgmt_id.string().c_str());
    }

    return CONTINUE;
}

Disposition
handle_ofmp_config_ack(const Event &e)
{
    const Ofmp_config_update_ack_event& ocua
            = assert_cast<const Ofmp_config_update_ack_event&>(e);

    if (boost::shared_ptr<Switch_mgr> swm = nox::mgmtid_to_swm(ocua.mgmt_id)) {
        swm->config_ack_handler(ocua);
    } else {
        lg.warn("got config ack for unknown switch mgr %s", 
                ocua.mgmt_id.string().c_str());
    }

    return CONTINUE;
}

Disposition
handle_ofmp_resources_update(const Event &e)
{
    const Ofmp_resources_update_event& orm
            = assert_cast<const Ofmp_resources_update_event&>(e);

    if (boost::shared_ptr<Switch_mgr> swm = nox::mgmtid_to_swm(orm.mgmt_id)) {
        swm->resources_update_handler(orm);
    } else {
        lg.warn("got resource update for unknown switch mgr %s", 
                orm.mgmt_id.string().c_str());
    }

    return CONTINUE;
}

bool
Conn::poll()
{
    ++poll_cnt;
    bool retval = do_poll();
    int p = --poll_cnt;
    if (!p && closing) {
        delete this;
    }
    return retval;
}

void
Conn::close()
{
    if (!closing) {
        closing = true;
        datapathid dp_id = oconn->get_datapath_id();
        datapathid mgmt_id = oconn->get_mgmt_id();
        if (dp_id == mgmt_id) {
            Switch_mgr_leave_event* swmle = new Switch_mgr_leave_event(mgmt_id);
            swm_map.erase(mgmt_id);
            post_event(swmle);
        } else {
            Datapath_leave_event* dple = new Datapath_leave_event(dp_id);
            post_event(dple);
        }
        connection_map.erase(dp_id);
        mgmt_map.erase(dp_id);
        main_loop->remove_pollable(this);
        if (!poll_cnt) {
            delete this;
        }
    }
}

bool
Conn::do_poll()
{
    int error;
    std::auto_ptr<Buffer> b(oconn->recv_openflow(error, false));
    switch (error) {
    case 0: {
        std::auto_ptr<Buffer> msgB(new Array_buffer(b.get()->size()));
        memcpy(msgB.get()->data(), b.get()->data(), b.get()->size());

        std::auto_ptr<Event> event(openflow_packet_to_event(oconn, b));
        if (event.get()) {
            event_dispatcher.dispatch(*event);
        }

        event.reset(openflow_msg_to_event(oconn, msgB));
	if (event.get())
	    event_dispatcher.dispatch(*event);

        return true;
    }

    case EAGAIN:
        return false;

    case EOF:
        lg.warn("%s: connection closed by peer", oconn->to_string().c_str());
        close();
        return true;
 
    default:
        lg.warn("%s: disconnected (%s)",
                oconn->to_string().c_str(), strerror(error));
        close();
        return true;
   }
}

void
Conn::wait()
{
    oconn->recv_openflow_wait();
}

class Signal_handler {
public:
    Signal_handler();
    ~Signal_handler();
private:
    Co_fsm fsm;
    Signal_group* sig_group;
    bool shutdown_in_progress;

    void run();
    static Disposition exit(const Event&) { ::exit(0); }
};

Signal_handler::Signal_handler()
    : sig_group(new Signal_group),
      shutdown_in_progress(false)
{
    sig_group->add_signal(SIGTERM);
    sig_group->add_signal(SIGINT);
    sig_group->add_signal(SIGHUP);
    fsm.start(boost::bind(&Signal_handler::run, this));
    register_handler(Shutdown_event::static_get_name(), exit, 9999);
}

Signal_handler::~Signal_handler()
{
    sig_group->remove_all();
}

void
Signal_handler::run()
{
    if (sig_group->try_dequeue() && !shutdown_in_progress) {
        shutdown_in_progress = true;
        post_event(new Shutdown_event);
    }
    if (!shutdown_in_progress) {
        sig_group->wait();
    }
    co_fsm_block();
}

static Disposition
handle_echo_request(const Event& e)
{
    const Echo_request_event& echo = assert_cast<const Echo_request_event&>(e);
    if (boost::shared_ptr<Openflow_connection> oconn
        = dpid_to_oconn(echo.datapath_id)) {
        oconn->send_echo_reply(echo.get_ofp_msg());
    }

    return CONTINUE;
}

void
init()
{
    classifier.register_packet_in();
    register_handler(Echo_request_event::static_get_name(),
                     handle_echo_request, 100);
    register_handler(Ofmp_config_update_event::static_get_name(),
                     handle_ofmp_config, 100);
    register_handler(Ofmp_config_update_ack_event::static_get_name(),
                     handle_ofmp_config_ack, 100);
    register_handler(Ofmp_resources_update_event::static_get_name(),
                     handle_ofmp_resources_update, 100);
    main_loop = new Poll_loop(N_THREADS);
    main_loop->add_pollable(&event_dispatcher);
    main_loop->add_pollable(&timer_dispatcher);
    new Signal_handler;
}

Poll_loop*
get_poll_loop() {
    return main_loop;
}

void
register_handler(const Event_name& name,
                 boost::function<Disposition(const Event&)> handler, int order)
{
    event_dispatcher.add_handler(name, handler, order);
}

/* Returns a nonzero OpenFlow transaction ID that has not been used for some
 * time.  Transaction IDs are per-datapath (actually, per connection to a given
 * datapath), so this is more uniqueness than needed, but the available space
 * is large enough that there is no disadvantage to it.  */
uint32_t
allocate_openflow_xid()
{
    static uint32_t xid;
    if (!xid) {
        xid = 1;
    }
    return xid++;
}

/* Attempts to send OpenFlow command 'oh' to switch 'datapath_id'.
 *
 * Returns 0 if successful or a positive errno value.  Returns ESRCH if switch
 * 'datapath_id' is unknown.  If 'block' is true, blocks as necessary;
 * otherwise, returns EAGAIN if blocking is needed.  */
int send_openflow_command(const datapathid& datapath_id, const ofp_header* oh,
                          bool block)
{
    co_might_yield_if(block);
    boost::shared_ptr<Openflow_connection> oconn = dpid_to_oconn(datapath_id);
    if (!oconn) {
        return ESRCH;
    }

    int error = oconn->send_openflow(oh, block);
    if (!error && oh->type == OFPT_FLOW_MOD) {
        /* Notify flowtracker than we're modifying the switch's flow table. */
        /* NOTE: only okay to pass in NULL because synchronously dispatching */
        Flow_mod_event fme(datapath_id,
                           reinterpret_cast<const ofp_flow_mod*>(oh),
                           std::auto_ptr<Buffer>(NULL));
        event_dispatcher.dispatch(fme);
    }
    return error;
}

/* Attempts to send 'packet' on 'out_port' of switch 'datapath_id'.  If
 * 'out_port' is OFPP_FLOOD, then 'packet' will not be sent on 'in_port'.
 *
 * Returns 0 if successful or a positive errno value.  Returns ESRCH if switch
 * 'datapath_id' is unknown.  If 'block' is true, blocks as necessary;
 * otherwise, returns EAGAIN if blocking is needed.  */
int send_openflow_packet_out(const datapathid& datapath_id, 
                             const Buffer& packet, uint16_t out_port,
                             uint16_t in_port, bool block)
{
    co_might_yield_if(block);
    boost::shared_ptr<Openflow_connection> oconn = dpid_to_oconn(datapath_id);
    if (!oconn) {
        return ESRCH;
    }

    return oconn->send_packet(packet, out_port, in_port, block);
}

int close_openflow_connection(const datapathid& dpid)
{
    chashmap::iterator iter = connection_map.find(dpid);
    if (iter == connection_map.end()) {
        lg.err("request to close connection to unknown dpid '%s'\n",
               dpid.string().c_str());
        return ESRCH;
    }
    iter->second->close();
    return 0; // success
}

/* Attempts to send set nat parameters for switch port 'datapath_id':'port num'
 *
 * Returns 0 if successful or a positive errno value.  Returns ESRCH if switch
 * 'datapath_id' is unknown.   */
int send_add_snat(const datapathid &datapath_id, uint16_t port, 
                    uint32_t ip_addr_start, uint32_t ip_addr_end,
                    uint16_t tcp_start, uint16_t tcp_end,
                    uint16_t udp_start, uint16_t udp_end,
                    ethernetaddr mac_addr, 
                    uint16_t mac_timeout) {
    boost::shared_ptr<Openflow_connection> oconn = dpid_to_oconn(datapath_id);
    if (!oconn) {
        return ESRCH;
    }
    return oconn->send_add_snat(port,ip_addr_start,ip_addr_end, 
                  tcp_start,tcp_end,udp_start,udp_end, mac_addr, mac_timeout); 
}

int send_del_snat(const datapathid &datapath_id, uint16_t port){
    boost::shared_ptr<Openflow_connection> oconn = dpid_to_oconn(datapath_id);
    if (!oconn) {
        return ESRCH;
    }
    return oconn->send_del_snat(port);
}

uint32_t get_switch_controller_ip(const datapathid &datapath_id){
    boost::shared_ptr<Openflow_connection> oconn = dpid_to_oconn(datapath_id);
    if (!oconn) {
        return 0; //indicates invalid IP 0.0.0.0
    }
    return oconn->get_local_ip();
}

uint32_t get_switch_ip(const datapathid &datapath_id){
    boost::shared_ptr<Openflow_connection> oconn = dpid_to_oconn(datapath_id);
    if (!oconn) {
        return 0; //indicates invalid IP 0.0.0.0
    }
    return oconn->get_remote_ip();
}

class Log_fetcher {
public:
    Log_fetcher(boost::shared_ptr<Tcp_socket>& tcp,
                const std::string& output_name_,
                const boost::function<void(int error,
                                           const std::string& msg)>& cb_)
        : listener(tcp),
          cb(cb_),
          output_name(output_name_),
          output(NULL),
          fsm(boost::bind(&Log_fetcher::run_listen, this)) { }
    ~Log_fetcher() {
        if (output) {
            fclose(output);
        }
    }
private:
    boost::shared_ptr<Tcp_socket> listener;
    boost::shared_ptr<Tcp_socket> receiver;
    boost::function<void(int error, const std::string& msg)> cb;
    const std::string& output_name;
    FILE *output;
    Auto_fsm fsm;

    void run_listen();
    void run_receive();
    void die(int error, const std::string& msg);
};

void
Log_fetcher::run_listen()
{
    int error;
    std::auto_ptr<Tcp_socket> sock(listener->accept(error, false));
    if (error == EAGAIN) {
        sock->accept_wait();
        fsm.block();
    } else if (error) {
        die(error, "accept failed");
    } else {
        listener->close();
        output = fopen(output_name.c_str(), "w");
        if (output) {
            receiver = boost::shared_ptr<Tcp_socket>(sock.release());
            fsm.transition(boost::bind(&Log_fetcher::run_receive, this));
        } else {
            die(errno, string_format("could not create output file %s: %s",
                                     output_name.c_str(), strerror(errno)));
        }
    }
}

void
Log_fetcher::run_receive()
{
    Array_buffer b(4096);
    int error = receiver->read(b, false);
    if (error == -EAGAIN || error > 0) {
        if (error > 0) {
            fwrite(b.data(), 1, b.size(), output);
        }
        receiver->read_wait();
        fsm.block();
    } else if (!error) {
        die(0, "success");
    } else {
        die(-error, string_format("error reading socket: %s",
                                  strerror(error)));
    }
}

void
Log_fetcher::die(int error, const std::string& msg)
{
    lg.warn("log file retrieval complete: %s", msg.c_str());

    /* Close the file before invoking the callback, to ensure that it is
     * flushed to disk.  */
    if (output) {
        fclose(output);
        output = NULL;
    }

    cb(error, msg);
    fsm.exit();
    delete this;
}

/* Fetches logs from switch 'dpid' into 'output_file' (which will be
 * replaced and overwritten if it already exists).  Returns 0 if the operation
 * can be initiated, otherwise a positive errno value.
 *
 * If this function returns 0, then 'cb' will eventually be invoked with an
 * 'error' of 0 if log file retrieval is successful, otherwise with a positive
 * errno value, as well as an explanatory 'msg' in either case. */
int
fetch_switch_logs(datapathid dpid, const std::string& output_file,
                  const boost::function<void(int error,
                                             const std::string& msg)>& cb)
{
    boost::shared_ptr<Openflow_connection> oconn = dpid_to_oconn(dpid);
    if (!oconn) {
        return ESRCH;
    }

    boost::shared_ptr<Tcp_socket> tcp;
    /* Call to tcp.bind() is intentionally omitted here, because we don't care
     * about the local port number. */
    int error = tcp->listen(1);
    if (error) {
        lg.warn("could not listen on tcp socket: %s", strerror(error));
        return error;
    }
    ipaddr ip;
    uint16_t port;
    error = tcp->getsockname(&ip, &port);
    if (error) {
        lg.warn("could not get local socket address: %s", strerror(error));
        return error;
    }

    std::vector<std::string> args;
    args.push_back(ip.string());
    args.push_back(string_format("%"PRIu16, port));
    error = oconn->send_remote_command("get-logs", args);
    if (error) {
        lg.warn("could not send remote command: %s", strerror(error));
        return error;
    }
    /* XXX we should register a callback to be fired when we receive a reply
     * from the switch to our remote command, so that we can detect and report
     * errors from the switch.  But we don't have any framework for doing that
     * yet. */

    new Log_fetcher(tcp, output_file, cb);
    return 0;
}

int
send_switch_command(datapathid dpid, const std::string& command, const std::vector<std::string> args)
{
    boost::shared_ptr<Openflow_connection> oconn = dpid_to_oconn(dpid);
    if (!oconn) {
        return ESRCH;
    }

    int error = oconn->send_remote_command(command, args);
    if (error) {
        lg.warn("could not send remote command: %s", strerror(error));
        return error;
    }
    /* XXX we should register a callback to be fired when we receive a reply
     * from the switch to our remote command, so that we can detect and report
     * errors from the switch.  But we don't have any framework for doing that
     * yet. */

    return 0;
}

int 
switch_reset(datapathid dpid)
{
    std::vector<std::string> args;
    return send_switch_command(dpid, "reboot", args);
}

int 
switch_update(datapathid dpid)
{
    std::vector<std::string> args;
    return send_switch_command(dpid, "update", args);
}


/* Attempts to send 'packet' with 'actions'.  
 *
 * Returns 0 if successful or a positive errno value.  Returns ESRCH if switch
 * 'datapath_id' is unknown.  If 'block' is true, blocks as necessary;
 * otherwise, returns EAGAIN if blocking is needed.  */
int send_openflow_packet_out(const datapathid& datapath_id, 
                             const Buffer& packet, 
                             const ofp_action_header actions[], 
                             uint16_t actions_len,
                             uint16_t in_port, bool block)
{
    co_might_yield_if(block);
    boost::shared_ptr<Openflow_connection> oconn = dpid_to_oconn(datapath_id);
    if (!oconn) {
        return ESRCH;
    }

    return oconn->send_packet(packet, actions, actions_len, in_port, block);
}

/* Attempts to send buffered packet 'buffer_id' on 'out_port' of switch
 * 'datapath_id'.  If 'out_port' is OFPP_FLOOD, then 'packet' will not be sent
 * on 'in_port'.
 *
 * Returns 0 if successful or a positive errno value.  Returns ESRCH if switch
 * 'datapath_id' is unknown.  If 'block' is true, blocks as necessary;
 * otherwise, returns EAGAIN if blocking is needed.  */
int
send_openflow_packet_out(const datapathid& datapath_id, 
                         const uint32_t buffer_id, uint16_t out_port, 
                         uint16_t in_port, bool block)
{
    co_might_yield_if(block);
    boost::shared_ptr<Openflow_connection> oconn = dpid_to_oconn(datapath_id);
    if (!oconn) {
        return ESRCH;
    }

    return oconn->send_packet(buffer_id, out_port, in_port, block);
}

/* Attempts to send 'packet' with 'actions' through switch 'datapath_id'.  
 *
 * Returns 0 if successful or a positive errno value.  Returns ESRCH if switch
 * 'datapath_id' is unknown.  If 'block' is true, blocks as necessary;
 * otherwise, returns EAGAIN if blocking is needed.  */
int
send_openflow_packet_out(const datapathid& datapath_id, 
                         const uint32_t buffer_id,
                         const ofp_action_header actions[], 
                         uint16_t actions_len,
                         uint16_t in_port, bool block)
{
    co_might_yield_if(block);
    boost::shared_ptr<Openflow_connection> oconn = dpid_to_oconn(datapath_id);
    if (!oconn) {
        return ESRCH;
    }

    return oconn->send_packet(buffer_id, actions, actions_len, in_port, block);
}

void
post_event(Event* event)
{
    event_dispatcher.post(event);
}

Timer
post_timer(const Callback& callback, const timeval& duration)
{
    return timer_dispatcher.post(callback, duration);
}

Timer
post_timer(const Callback& callback)
{
    return timer_dispatcher.post(callback);
}

void
timer_debug() {
    timer_dispatcher.debug();
}

//-----------------------------------------------------------------------------

uint32_t 
register_handler_on_match(uint32_t priority, const Packet_expr &expr,
                          Pexpr_action callback)
{
    return classifier.add_rule(priority, expr, callback);
}

bool 
unregister_handler(uint32_t rule_id)
{
    return classifier.delete_rule(rule_id);
}

void register_switch_auth(Switch_Auth* auth) { 
  if(switch_authenticator) { 
    lg.err("Switch Auth already set, ignoring register_switch_auth\n");
    return; 
  } 
  switch_authenticator = auth; 
} 

Switch_Auth* get_switch_auth() { 
  return switch_authenticator; 
} 

/* FSM responsible for Openflow_connections on which we have not yet received 
 * a features reply message.  It transmits (and retransmits, if necessary) 
 * features request messages until it receives a reply, at which point it 
 * registers the connection with nox under its datapath id and kills
 * itself.
 *
 * Kind of a mess, really.  Needs refactoring. */
class Handshake_fsm {
public:
    Handshake_fsm(std::auto_ptr<Openflow_connection>, 
              Co_sema* disconnected, int *errorp, int timeout_secs_);
private:
    enum Handshake_state { SEND_FEATURES_REQ = 0, 
                           SEND_CONFIG,
                           RECV_FEATURES_REPLY,
                           SEND_MGMT_CAPABILITY_REQ,
                           RECV_MGMT_CAPABILITY_REPLY,
                           SEND_MGMT_RESOURCES_REQ,
                           RECV_MGMT_RESOURCES_UPDATE,
                           SEND_MGMT_CONFIG_REQ,
                           RECV_MGMT_CONFIG_UPDATE,
                           CHECK_SWITCH_AUTH, 
                           CHECK_MGMT_AUTH, 
                           REGISTER_SWITCH, 
                           REGISTER_MGMT, 
                           NUM_STATES };

    static std::string state_desc[NUM_STATES]; 
    std::auto_ptr<Openflow_connection> oconn;
    int timeout_secs;
    timeval timeout;            /* Time to give up on the connection. */
    Co_sema* disconnected;
    int *errorp;
    Handshake_state state;
    Co_completion completion; // used to signal switch approval is done
    bool switch_approved;

    datapathid mgmt_id;
    Cfg swm_caps;             // Initial manager capabilities
    Cfg swm_config;           // Initial manager config

    // Buffer to handle OFMP extended data messages
    std::auto_ptr<Buffer> ext_data_buffer; 
    uint32_t ext_data_xid;

    void handle_vendor(std::auto_ptr<Buffer> buf);
    void handle_ofmp_capability(const ofmp_capability_reply *ocr, 
            std::auto_ptr<Buffer> buf);
    void handle_ofmp_config_update(const ofmp_config_update *ocu, 
            std::auto_ptr<Buffer> buf);
    void handle_ofmp_resources(const ofmp_resources_update *oru, 
            std::auto_ptr<Buffer> buf);
    void handle_ofmp_extended_data(const ofmp_extended_data *oed, 
            std::auto_ptr<Buffer> buf);

    /* We need to buffer features reply so that we can create 
     * a Datapath_join_event if asynchronous switch authorization succeeds */ 
    std::auto_ptr<Buffer> features_reply_buf;
    ofp_switch_features *features_reply; 

    Ofmp_resources_update_event *resources_event;

    void run();
    void send_message();
    void recv_message();
    void check_switch_auth(Handshake_state next); 
    void switch_auth_cb(bool is_valid, Handshake_state next); 
    void register_switch(); 
    void register_mgmt(); 
    void do_exit(int error);
};

std::string Handshake_fsm::state_desc[NUM_STATES] = { 
                                          "sending features request", 
                                          "sending switch config", 
                                          "receiving features reply",
                                          "sending ofmp capability request", 
                                          "receiving ofmp capability reply", 
                                          "sending ofmp resources request", 
                                          "receiving ofmp resources reply", 
                                          "sending ofmp config request", 
                                          "receiving ofmp config update", 
                                          "checking switch auth",
                                          "checking management auth",
                                          "registering switch",
                                          "registering mgmt channel"}; 

Handshake_fsm::Handshake_fsm(std::auto_ptr<Openflow_connection> oconn_,
                    Co_sema* disconnected_, int* errorp_, int timeout_secs_)
    : oconn(oconn_),
      timeout_secs(timeout_secs_),
      timeout(do_gettimeofday() + make_timeval(timeout_secs_,0)),
      disconnected(disconnected_),
      errorp(errorp_),
      state(SEND_FEATURES_REQ),
      switch_approved(false), 
      ext_data_xid(UINT32_MAX),
      features_reply(NULL),
      resources_event(NULL)
{
    ext_data_buffer.reset(new Array_buffer(0));
    co_fsm_create(co_group_self(), boost::bind(&Handshake_fsm::run, this));
}

void Handshake_fsm::send_message() {
    int error; 
    Handshake_state next_state; 
    if(state == SEND_FEATURES_REQ) {
      error = oconn->send_features_request();
      next_state = SEND_CONFIG; 
    } else if (state == SEND_CONFIG) { 
      error = oconn->send_switch_config(); 
      next_state = RECV_FEATURES_REPLY; 
    } else if (state == SEND_MGMT_CAPABILITY_REQ) { 
      error = oconn->send_ofmp_capability_request(); 
      next_state = RECV_MGMT_CAPABILITY_REPLY; 
    } else if (state == SEND_MGMT_RESOURCES_REQ) { 
      error = oconn->send_ofmp_resources_request(); 
      next_state = RECV_MGMT_RESOURCES_UPDATE; 
    } else if (state == SEND_MGMT_CONFIG_REQ) { 
      error = oconn->send_ofmp_config_request(); 
      next_state = RECV_MGMT_CONFIG_UPDATE; 
    } else { 
      lg.err("Invalid state %d in Handshake_fsm::send_openflow()\n", state);
      do_exit(EINVAL);
      return; 
    } 
    
    if (error == EAGAIN) {
        oconn->send_openflow_wait();
        co_timer_wait(timeout, NULL);
        co_fsm_block();
    } else if (error) {
        lg.warn("Error %s: %s", state_desc[state].c_str(), strerror(error));
        do_exit(error);
    } else {
        state = next_state; 
        lg.dbg("Success sending in '%s'", state_desc[state].c_str());
        co_fsm_yield();
    }
}

void Handshake_fsm::handle_ofmp_capability(const ofmp_capability_reply *ocr,
        std::auto_ptr<Buffer> buf)
{
    int data_len = buf->size() - sizeof(*ocr);
    if (data_len < 0) {
        lg.warn("received bad ofmp capability reply");
        return;
    }

    if (ocr->format != htonl(OFMPCAF_SIMPLE)) {
        lg.warn("received unsupported ofmp capability format: %d",
                ntohl(ocr->format));
        return;
    }

    mgmt_id = datapathid::from_net(ocr->mgmt_id);

    swm_caps.load(ocr->data, data_len);
    
    if (!swm_caps.get_bool(0, "com.nicira.mgmt.manager")) {
        // This is not a management connection
        lg.dbg("datapath %012"PRIx64" has manager %s\n", 
                ntohll(features_reply->datapath_id), 
                mgmt_id.string().c_str()); 
        state = CHECK_SWITCH_AUTH; 
        return;
    }

    state = SEND_MGMT_RESOURCES_REQ; 
}

void Handshake_fsm::handle_ofmp_resources(const ofmp_resources_update *oru,
        std::auto_ptr<Buffer> buf)
{
    /* The OFMP resources update event constructor already parses these
     * messages, so even though we don't need an event, it's a useful
     * container.
     */
    resources_event = new Ofmp_resources_update_event(mgmt_id, oru,
            buf->size());
}

void Handshake_fsm::handle_ofmp_config_update(const ofmp_config_update *ocu,
        std::auto_ptr<Buffer> buf)
{
    int data_len = buf->size() - sizeof(*ocu);
    if (data_len < 0) {
        lg.warn("Received bad ofmp config update\n");
        return;
    }
    if (ocu->format != htonl(OFMPCOF_SIMPLE)) {
        lg.warn("unsupported config format: %d\n", ntohl(ocu->format));
        return;
    }

    swm_config.load(ocu->data, data_len);
}

void Handshake_fsm::handle_ofmp_extended_data(const ofmp_extended_data *oed,
        std::auto_ptr<Buffer> buf)
{
    int len = buf->size() - sizeof *oed;
    uint8_t *ptr;

    if (buf->size() <= sizeof *oed) {
        lg.warn("Received too short ofmp extended message: %d\n", len);
        return;
    }

    ext_data_xid = oed->header.header.header.xid;

    ptr = ext_data_buffer->put(len);
    memcpy(ptr, oed->data, len);

    if (!(oed->flags & OFMPEDF_MORE_DATA)) {
        /* An embedded message must be greater than the size of an
         * OpenFlow message. */
        if (ext_data_buffer->size() < 65536) {
            lg.warn("Received short embedded message: %zu\n",
                    ext_data_buffer->size());
            return;
        }

        /* Make sure that this is a management message and that there's
         * not an embedded extended data message. */
        ofmp_header *ofmph = ext_data_buffer->try_at<ofmp_header>(0);
        if ((ofmph->header.vendor != htonl(NX_VENDOR_ID))
                || (ofmph->header.subtype != htonl(NXT_MGMT))
                || (ofmph->type == htonl(OFMPT_EXTENDED_DATA))) {
            lg.warn("Received bad embedded extended message\n");
            return;
        }
        ofmph->header.header.xid = ext_data_xid;
        ofmph->header.header.length = 0;

        handle_vendor(ext_data_buffer);
        ext_data_buffer.reset(new Array_buffer(0));
    }
}

void Handshake_fsm::handle_vendor(std::auto_ptr<Buffer> buf) 
{
    // The only vendor extension we care about are the Nicira management
    // ones, so ignore any others.
    nicira_header *nh = buf->try_at<nicira_header>(0);
    if (nh && (nh->vendor == htonl(NX_VENDOR_ID))
            && (nh->subtype == htonl(NXT_MGMT))) {

        if (ofmp_header *ofmph = buf->try_at<ofmp_header>(0)) {
            /* Reset the extended data buffer if we're certain that this isn't a
             * continuation of an existing extended data message. */
            if (ext_data_xid != ofmph->header.header.xid) {
                if (ext_data_buffer->size() > 0) {
                    ext_data_buffer.reset(new Array_buffer(0));
                }
            }   

            switch (ntohs(ofmph->type)) {
            case OFMPT_CAPABILITY_REPLY: {
                    if(state != RECV_MGMT_CAPABILITY_REPLY) { 
                      lg.warn("Ignoring mgmt capability reply "
                              "received while in state '%s'", 
                              state_desc[state].c_str()); 
                      return; 
                    }
                    ofmp_capability_reply *ocr =
                        buf->try_at<ofmp_capability_reply>(0);
                    handle_ofmp_capability(ocr, buf);
                }
                break;
            case OFMPT_RESOURCES_UPDATE: {
                    if(state != RECV_MGMT_RESOURCES_UPDATE) { 
                      lg.warn("Ignoring mgmt resources update "
                              "received while in state '%s'", 
                              state_desc[state].c_str()); 
                      return; 
                    }
                    ofmp_resources_update *oru =
                        buf->try_at<ofmp_resources_update>(0);
                    handle_ofmp_resources(oru, buf);
                }
                state = SEND_MGMT_CONFIG_REQ; 
                break;
            case OFMPT_CONFIG_UPDATE: {
                    if(state != RECV_MGMT_CONFIG_UPDATE) { 
                      lg.warn("Ignoring mgmt config update "
                              "received while in state '%s'", 
                              state_desc[state].c_str()); 
                      return; 
                    }
                    ofmp_config_update *ocu =
                        buf->try_at<ofmp_config_update>(0);
                    handle_ofmp_config_update(ocu, buf);
                }
                state = CHECK_MGMT_AUTH; 
                break;
            case OFMPT_ERROR: {
                    ofmp_error_msg *oem =
                        buf->try_at<ofmp_error_msg>(0);
                    lg.warn("received ofmp error with type %d and code %d\n",
                            ntohs(oem->type), ntohs(oem->code));
                }
                break;
            case OFMPT_EXTENDED_DATA: {
                    ofmp_extended_data *oed =
                        buf->try_at<ofmp_extended_data>(0);
                    handle_ofmp_extended_data(oed, buf);
                }
                break;
            default:
                lg.warn("received unknown mgmt type: %d", 
                        ntohs(ofmph->type));
                // ignore message, do not change state
                break;
            }
        }
    }
}

void Handshake_fsm::recv_message() {

    int error;
    std::auto_ptr<Buffer> buf = oconn->recv_openflow(error, false);
    if (error == EAGAIN) {
        oconn->recv_openflow_wait();
        co_timer_wait(timeout, NULL);
        co_fsm_block();
        return; 
    } else if (error) {
        lg.warn("Error %s : recv: %s", state_desc[state].c_str(),
                error == EOF ? "connection closed" : strerror(error));
        do_exit(error);
        return; 
    } 
    if (ofp_header* oh = buf->try_at<ofp_header>(0)) {
      lg.dbg("Success receiving in '%s'", state_desc[state].c_str());
      switch (oh->type) {
        case OFPT_FEATURES_REPLY:
          if(state != RECV_FEATURES_REPLY) { 
              lg.warn("Ignoring features reply "
                      "received while in state '%s'", 
                      state_desc[state].c_str()); 
              break; 
          }

          // save features reply for switch auth and registration 
          features_reply_buf = buf;
          features_reply = features_reply_buf->try_at<ofp_switch_features>(0);

          if(features_reply) 
            state = SEND_MGMT_CAPABILITY_REQ;
          break;
        case OFPT_VENDOR: 
          handle_vendor(buf);
          break;
        case OFPT_ECHO_REQUEST:
          oconn->send_echo_reply(oh);
          break;
        case OFPT_ERROR:
          if (state == RECV_MGMT_CAPABILITY_REPLY) {
            lg.dbg("Datapath %012"PRIx64" sent error in response to "
                    "capability reply, assuming no management support",
                    ntohll(features_reply->datapath_id)); 
            state = CHECK_SWITCH_AUTH;
          } else {
            ofp_error_msg *oem = buf->try_at<ofp_error_msg>(0);
            lg.warn("Received error during handshake (%d/%d)",
                    ntohs(oem->type), ntohs(oem->code));
            do_exit(EINVAL);
            return;
          }
          break;
        case OFPT_PACKET_IN:
          /* These messages can be received before nox considers the
           * handshake to be complete, and don't indicate an error. */
          lg.dbg("Dropping packet in message during handshake");
          break;
        default:
          lg.warn("Received unsupported message type during handshake (%d)",
                  oh->type);
          break;
      }
    }
    co_fsm_yield();
}

void Handshake_fsm::check_switch_auth(Handshake_state next) {
 
  Switch_Auth* auth = nox::get_switch_auth(); 
  if(!auth) { 
    lg.dbg("No switch auth module registered, auto-approving switch\n"); 
    switch_approved = true;
    state = next; 
    run();
    return; 
  }  
  assert(features_reply);     
  auth->check_switch_auth(oconn, features_reply, 
      boost::bind(&Handshake_fsm::switch_auth_cb,this,_1,next));  
  completion.wait(NULL); 
  co_fsm_block();
} 

void Handshake_fsm::switch_auth_cb(bool is_approved,Handshake_state next) {
  switch_approved = is_approved;
  state = next; 
  completion.release(); // restart FSM
} 

void Handshake_fsm::register_mgmt() { 
    
    if(!switch_approved) { 
      lg.err("Disconnecting unapproved management channel %"PRIx64" \n", mgmt_id.as_host());
      do_exit(EPERM);
      return; 
    }
    if(mgmt_id.as_host() == 0) { 
      lg.err("0 is not a valid management id. Disconnecting.");
      do_exit(EINVAL);
      return; 
    }

    oconn->set_datapath_id(mgmt_id);
    oconn->set_mgmt_id(mgmt_id);
    register_conn(oconn.release(), disconnected);

    Switch_mgr *swm = new Switch_mgr(mgmt_id);
    swm->set_capabilities(swm_caps);
    swm->set_config(swm_config);
    assert(resources_event); 
    swm->resources_update_handler(*resources_event);
    swm_map.insert(swmhashmap::value_type(mgmt_id, 
                boost::shared_ptr<Switch_mgr>(swm)));

    lg.dbg("Registering mgmt channel with id = %"PRIx64"\n",mgmt_id.as_host()); 
    /* Really we want to just dispatch this event immediately, but that would
     * prevent any handlers for it from blocking, since we're running inside
     * an FSM. */
    // NOTE: event steals auto_ptr to feature
    event_dispatcher.post(new Switch_mgr_join_event(mgmt_id));
    disconnected = NULL;

    do_exit(0);
}

void Handshake_fsm::register_switch() { 
    datapathid dpid = datapathid::from_net(features_reply->datapath_id); 
    if(!switch_approved) { 
      lg.err("Disconnecting unapproved switch %"PRIx64" \n", dpid.as_host());
      do_exit(EPERM);
      return; 
    }
    
    if(dpid.as_host() == 0) { 
      lg.err("0 is not a valid DPID. Disconnecting switch");
      do_exit(EINVAL);
      return; 
    }

    oconn->set_datapath_id(dpid);
    oconn->set_mgmt_id(mgmt_id);
    register_conn(oconn.release(), disconnected);
    mgmt_map.insert(mhashmap::value_type(dpid, mgmt_id));

    /* delete all flows on this switch */
    {
        ofp_flow_mod* ofm;
        size_t size = sizeof *ofm;
        boost::shared_array<char> raw_of(new char[size]);
        ofm = (ofp_flow_mod*) raw_of.get();
        ofm->header.version = OFP_VERSION;
        ofm->header.type   = OFPT_FLOW_MOD;
        ofm->header.length = htons(size);
        ofm->match.wildcards = htonl(0xffffffff);
	ofm->cookie = htonl(0);
        ofm->command = htons(OFPFC_DELETE);
        ofm->buffer_id    = htonl(0);
        ofm->idle_timeout = htons(0);
        ofm->hard_timeout = htons(0);
        ofm->priority     = htons(0);
        ofm->out_port     = htons(OFPP_NONE);
        ofm->flags        = htons(0);
        /* XXX OK to do non-blocking send?  We do so with all other
         * commands on switch join */
        if ( send_openflow_command(dpid, &ofm->header, false) == EAGAIN) {
              lg.err("Error, unable to clear flow table on startup");
        }
    }

    lg.dbg("Registering switch with DPID = %"PRIx64"\n",dpid.as_host()); 
    /* Really we want to just dispatch this event immediately, but that would
     * prevent any handlers for it from blocking, since we're running inside
     * an FSM. */
    // NOTE: event steals auto_ptr to features
    event_dispatcher.post(new Datapath_join_event(features_reply, features_reply_buf));
    disconnected = NULL;

    do_exit(0);
}


void Handshake_fsm::run()
{
    timeval now = do_gettimeofday();
    if (now > timeout) {
        lg.warn("%s: closing connection due to timeout after %d seconds in "
                "%s state ", oconn->to_string().c_str(), timeout_secs,
                state_desc[state].c_str());
        do_exit(ETIMEDOUT);
        return;
    }

    switch (state) { 
      case SEND_FEATURES_REQ: 
        send_message();
        break;  
      case SEND_CONFIG: 
        send_message();
        break;  
      case RECV_FEATURES_REPLY: 
        recv_message(); 
        break; 
      case SEND_MGMT_CAPABILITY_REQ:
        send_message();
        break;
      case RECV_MGMT_CAPABILITY_REPLY:
        recv_message();
        break;
      case SEND_MGMT_RESOURCES_REQ:
        send_message();
        break;
      case RECV_MGMT_RESOURCES_UPDATE:
        recv_message();
        break;
      case SEND_MGMT_CONFIG_REQ:
        send_message();
        break;
      case RECV_MGMT_CONFIG_UPDATE:
        recv_message();
        break;
      case CHECK_SWITCH_AUTH: 
        check_switch_auth(REGISTER_SWITCH);
        break;
      case CHECK_MGMT_AUTH: 
        check_switch_auth(REGISTER_MGMT);
        break;
      case REGISTER_SWITCH: 
        register_switch(); 
        break; 
      case REGISTER_MGMT: 
        register_mgmt(); 
        break; 
      default: 
        lg.err("Invalid Handshake state = %d \n", state); 
    } 
}

void Handshake_fsm::do_exit(int error) 
{
    if (errorp) {
        *errorp = error;
    }
    if (disconnected) {
        disconnected->up();
    }
    if(resources_event){
      delete resources_event; 
    }
    co_fsm_exit();
    delete this; 
}

/* Connector threads.
 *
 * This code is responsible for taking a Openflow_connection_factory and
 * connecting to it, then creating a Handshake_fsm for it.
 *
 * There's no reason that this code has to run as a thread.  It used to have
 * to because some Openflow_connections had connect() functions that blocked.
 * They don't anymore, so eventually the connector threads should probably
 * become FSMs to save the cost of a thread. */

struct Connector_aux {
    Openflow_connection_factory* factory;
};

int do_connect(Connector_aux* aux, int timeout_sec,
               Co_sema *disconnected, int *errorp)
{
    for (;;) {
        int error;
        std::auto_ptr<Openflow_connection> oconn(aux->factory->connect(error));
        if (error != EAGAIN) {
            if (!error) {
                new Handshake_fsm(oconn, disconnected, errorp, timeout_sec);
            }
            return error;
        }
        aux->factory->connect_wait();
        co_block();
    }
}

void unreliable_connector_thread(Connector_aux* aux) 
{
    do_connect(aux, 60, NULL, NULL);
}

void passive_connector_thread(Connector_aux* aux)
{
    for (;;) {
        do_connect(aux, 5, NULL, NULL);
    }
}

void
connect(Openflow_connection_factory* factory, bool reliable)
{
    Connector_aux* aux = new Connector_aux;
    aux->factory = factory;

    if (reliable && !factory->passive()) {
        std::auto_ptr<Openflow_connection_factory> f(factory);
        std::auto_ptr<Openflow_connection> c(
            new Reliable_openflow_connection(f));
        new Handshake_fsm(c, NULL, NULL, 4);
    } else {
        void (*thread_func)(Connector_aux*);
        if (factory->passive()) {
            thread_func = passive_connector_thread; 
        } else {
            thread_func = unreliable_connector_thread;
        }
        co_thread_create(&co_group_coop, boost::bind(thread_func, aux)); 
    }
}

void
register_conn(Openflow_connection* oconn, Co_sema* disconnected) 
{
    datapathid dp_id = oconn->get_datapath_id();

    chashmap::iterator i;
    Conn* conn = new Conn(boost::shared_ptr<Openflow_connection>(oconn),
                          disconnected);
    std::pair<chashmap::iterator, bool> pair
        = connection_map.insert(chashmap::value_type(dp_id, conn));
    if (!pair.second) {
        pair.first->second->close();
        // Conn->close() removes the entry from connection_map, so reinsert
        pair = connection_map.insert(chashmap::value_type(dp_id, conn));
        assert(pair.second); 
    }
}

void run()
{
    main_loop->run();
}

} // namespace vigil::nox
} // namespace vigil
