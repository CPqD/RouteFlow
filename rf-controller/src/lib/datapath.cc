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
/* datapath interface for Netlink transport layer. */

#include "datapath.hh"
#include <sys/socket.h>
#include <linux/socket.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/netlink.h>
#include <errno.h>
#include <map>
#include <netinet/in.h>
#include <string.h>
#include "buffer.hh"
#include "openflow.hh"
#include "auto_free.hh"
#include "netlink.hh"
#include "vlog.hh"
#include "netinet++/ethernetaddr.hh"
#include "openflow/openflow-netlink.h"
#include "openflow/openflow.h"

namespace vigil {

static Vlog_module log("datapath");

Genl_family Datapath::openflow_family(DP_GENL_FAMILY_NAME);

/* Construct a datapath for the given 'dp_idx' (usually 0, as it is uncommon
 * to want more than a single datapath on any given machine).
 *
 * The datapath with the given index need not actually exist, but if it
 * doesn't then you'll only be able to do add_dp(). */
Datapath::Datapath(int dp_idx_)
    : dp_idx(dp_idx_),
      connect_status(CONN_NOT_STARTED)
{
}

/* Binds the Netlink socket.  Returns 0 if successful, otherwise a positive
 * Unix error code.
 *
 * A Datapath bound this way will not receive "packet in" events from the data
 * path.  Use subscribe() instead if you need to receive those. */
int Datapath::bind()
{
    connect_status = sock.bind(NETLINK_GENERIC);
    return connect_status;
}

/* Policy for querying the multicast group associated with a datapath.
 * We need to subscribe to the multicast group if we want to receive "packet
 * in" packets from the datapath. */
static std::tr1::array<Nl_attr, 3> dp_policy_elems = { {
        Nl_attr::u32(DP_GENL_A_DP_IDX).require(),
        Nl_attr::u32(DP_GENL_A_MC_GROUP).require(),
        Nl_attr::string(DP_GENL_A_PORTNAME, 1, IFNAMSIZ - 1),
} };

static Nl_policy dp_policy(dp_policy_elems);

/* Binds the Netlink socket.  Returns 0 on success or a positive errno value.
 *
 * If 'block' is false, returns EAGAIN if the connection cannot complete
 * immediately.  Use get_connect_error() and connect_wait() to determine the
 * connection status.
 *
 * A Datapath bound this way will receive all the "packet in" events from the
 * datapath.  Use bind() instead if you do not need to receive them. */
int Datapath::subscribe(bool block) 
{
    co_might_yield_if(block);
    if (bind()) {
        return connect_status;
    }
    connect_status = CONN_GET_FAMILY;
    connect_state_machine();
    if (block) {
        while (connect_status < 0) {
            co_might_yield();
            connect_wait();
            co_block();
        }
    }
    return connect(block);
}

/* Returns the status of the datapath's connection: ENOTCONN if connection has
 * not even been attempted (via subscribe() or bind(), EAGAIN if connection is
 * in progress, 0 if the connection is complete, otherwise a positive errno
 * value. */
int Datapath::do_connect() 
{
    connect_state_machine();
    return (connect_status == CONN_NOT_STARTED ? ENOTCONN
            : connect_status < 0 ? EAGAIN
            : connect_status);
}

void Datapath::do_connect_wait() 
{
    connect_state_machine();

    if (connect_status == CONN_GET_FAMILY) {
        openflow_family.lookup_wait();
    } else if (connect_status == CONN_GET_MULTICAST_GROUP) {
        sock.transact_wait(*transaction);
    } else {
        co_immediate_wake(1, NULL);
    }
}

void Datapath::connect_state_machine() 
{
    if (connect_status == CONN_NOT_STARTED) {
        return;
    }
    
    if (connect_status == CONN_GET_FAMILY) {
        int family;
        int error = openflow_family.lookup(family, false);
        if (error) {
            if (error != EAGAIN) {
                connect_status = error;
            }
            return;
        }

        request.reset(new Genl_message(family, NLM_F_REQUEST,
                                       DP_GENL_C_QUERY_DP, 1, sock));
        request->put_u32(DP_GENL_A_DP_IDX, dp_idx);
        transaction.reset(new Nl_transaction(sock, *request));
        connect_status = CONN_GET_MULTICAST_GROUP;
    }

    if (connect_status == CONN_GET_MULTICAST_GROUP) {
        /* Request the multicast group associated with datapath 'dp_idx'. */
        int error;
        std::auto_ptr<Buffer> reply_buf = sock.transact(*transaction, error,
                                                        false);
        if (error) {
            if (error != EAGAIN) {
                connect_status = error;
            }
            return;
        }

        Genl_message reply(reply_buf);
        if (!reply.ok()) {
            log.warn("%s: recv: Genl_message too short", to_string().c_str());
            connect_status = EPROTO;
            return;
        }

        std::tr1::array<nlattr*, DP_GENL_A_MAX + 1> attrs;
        if (!dp_policy.parse(reply.rest(), attrs)) {
            connect_status = EPROTO;
            return;
        }

        /* Re-bind to multicast group. */
        Nl_socket::Parameters p(NETLINK_GENERIC);
        p.join_multicast_group(get_u32(attrs[DP_GENL_A_MC_GROUP]));
        connect_status = sock.bind(p);
    }
}

/* Sends the Generic Netlink 'command' in the OpenFlow family to datapath
 * 'dp_idx'.  If 'netdev' is nonempty then it is sent as the DP_GENL_A_PORTNAME
 * attribute.  Calls 'callback' with the send status when the command has been
 * sent. */
int Datapath::send_mgmt_command(int command, const std::string* netdev)
{
    int family;
    int error = openflow_family.lookup(family, true);
    if (error) {
        return error;
    }

    Genl_message request(family, NLM_F_REQUEST, command, 1, sock);
    request.put_u32(DP_GENL_A_DP_IDX, dp_idx);
    if (netdev) {
        request.put_string(DP_GENL_A_PORTNAME, *netdev);
    }

    sock.transact(request, error);
    return error;
}

/* Creates the datapath represented by this object.  Executes 'callback' when
 * complete. */
int Datapath::add_dp()
{
    return send_mgmt_command(DP_GENL_C_ADD_DP, NULL);
}

/* Destroys the datapath represented by this object.  Executes 'callback' when
 * complete. */
int Datapath::del_dp()
{
    return send_mgmt_command(DP_GENL_C_DEL_DP, NULL);
}

/* Adds the Ethernet device named 'netdev' to this datapath.  Executes
 * 'callback' when complete. */
int Datapath::add_port(const std::string& netdev)
{
    return send_mgmt_command(DP_GENL_C_ADD_PORT, &netdev);
}

/* Removes the Ethernet device named 'netdev' from this datapath.  Executes
 * 'callback' when complete. */
int Datapath::del_port(const std::string& netdev)
{
    return send_mgmt_command(DP_GENL_C_DEL_PORT, &netdev);
}

int Datapath::do_send_openflow(const ofp_header* oh)
{
    int family;
    int error = openflow_family.lookup(family, true);
    if (error) {
        return error;
    }

    /* FIXME: copies whole message */
    Genl_message msg(family, NLM_F_REQUEST, DP_GENL_C_OPENFLOW,
                     1, sock, ntohs(oh->length) + 64 /* FIXME */);
    msg.put_u32(DP_GENL_A_DP_IDX, dp_idx);
    msg.put_unspec(DP_GENL_A_OPENFLOW, oh, ntohs(oh->length));
    return sock.send(msg.all(), false);
}

void Datapath::do_send_openflow_wait() 
{
    sock.send_wait();
}

void Datapath::do_recv_openflow_wait()
{
    sock.recv_wait();
}

/* Policy for parsing OpenFlow messages sent to us by the kernel. */
static std::tr1::array<Nl_attr, 2> openflow_policy_elems = { {
        Nl_attr::u32(DP_GENL_A_DP_IDX).require(),
        Nl_attr::unspec(DP_GENL_A_OPENFLOW, sizeof(ofp_header),
                        UINT16_MAX).require(),
} };

static Nl_policy openflow_policy(openflow_policy_elems);

std::auto_ptr<Buffer>
Datapath::do_recv_openflow(int& error)
{
again:
    std::auto_ptr<Buffer> buffer(sock.recv(error, false));
    if (error) {
        return std::auto_ptr<Buffer>(0);
    }

    Genl_message m(std::auto_ptr<Buffer>(new Nonowning_buffer(*buffer)));
    if (!m.ok()) {
        log.warn("%s: recv: Genl_message too short", to_string().c_str());
        error = EPROTO;
        return std::auto_ptr<Buffer>(0);
    }

    if (m.nlhdr().nlmsg_type == NLMSG_ERROR) {
        log.dbg("%s: recv: netlink error: %s",
                to_string().c_str(), m.to_string().c_str());
        goto again;
    }

    int family;
    error = openflow_family.lookup(family, true);
    if (error) {
        return std::auto_ptr<Buffer>(0);
    }

    if (m.nlhdr().nlmsg_type != family) {
        log.warn("%s: recv: type (%"PRIu16") != openflow family (%d): %s",
                 to_string().c_str(), m.nlhdr().nlmsg_type, family,
                 m.to_string().c_str());
        error = EPROTO;
        return std::auto_ptr<Buffer>(0);
    }

    std::tr1::array<nlattr*, DP_GENL_A_MAX + 1> attrs;
    if (!openflow_policy.parse(m.rest(), attrs)) {
        error = EPROTO;
        return std::auto_ptr<Buffer>(0);
    }
    if (get_u32(attrs[DP_GENL_A_DP_IDX]) != dp_idx) {
        log.warn("%s: recv: dp_idx (%"PRIu32") differs from expected (%d)",
                 to_string().c_str(),
                 get_u32(attrs[DP_GENL_A_DP_IDX]), dp_idx);
        error = EPROTO;
        return std::auto_ptr<Buffer>(0);
    }

    /* Kluge! */
    buffer->pull((uint8_t*) get_data(attrs[DP_GENL_A_OPENFLOW])
                 - buffer->data());
    buffer->trim(attrs[DP_GENL_A_OPENFLOW]->nla_len - NLA_HDRLEN);
    error = 0;
    return buffer;
}

std::string Datapath::to_string()
{
    char s[32];
    sprintf(s, "nl:%d", dp_idx);
    return s;
}

int Datapath::close()
{
    return sock.close();
}

Openflow_connection::Connection_type 
Datapath::get_conn_type() { 
  return Openflow_connection::TYPE_DATAPATH; 
} 

Datapath_factory::Datapath_factory(int dp_idx_)
    : dp_idx(dp_idx_) 
{
}

Openflow_connection* Datapath_factory::connect(int& error) 
{
    std::auto_ptr<Datapath> dp(new Datapath(dp_idx));
    error = dp->subscribe(false);
    if (error && error != EAGAIN) {
        return NULL;
    } else {
        error = 0;
        return dp.release();
    }
}

void
Datapath_factory::connect_wait()
{
    co_immediate_wake(1, NULL);
}

std::string Datapath_factory::to_string() 
{
    char s[32];
    ::sprintf(s, "nl:%d", dp_idx);
    return s;
}

} // namespace vigil
