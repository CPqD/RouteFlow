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
#include "netlink.hh"
#include <algorithm>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/static_assert.hpp>
#include <cerrno>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/socket.h>
#include <linux/socket.h>
#include <linux/netlink.h>
#include <linux/genetlink.h>
#include <vector>
#include "vlog.hh"

#define NOT_REACHED() abort()

#ifndef SOL_NETLINK
#define SOL_NETLINK 270
#endif

/* Netlink PID.
 *
 * Every Netlink socket must be bound to a unique 32-bit PID.  By convention,
 * programs that have a single Netlink socket use their Unix process ID as PID,
 * and programs with multiple Netlink sockets add a unique per-socket
 * identifier in the bits above the Unix process ID.
 *
 * The kernel has Netlink PID 0.
 */
namespace {

/* Parameters for how many bits in the PID should come from the Unix process ID
 * and how many unique per-socket. */
const int socket_bits = 10;
const int process_bits = 22;
const uint32_t process_mask = (1u << process_bits) - 1;
BOOST_STATIC_ASSERT(socket_bits + process_bits == 32);

/* Bit vector of unused socket identifiers. */
std::vector<bool> avail_sockets(1u << socket_bits, 0);

/* Allocates and returns a new Netlink PID. */
uint32_t alloc_pid()
{
    std::vector<bool>& bm = avail_sockets;
    std::vector<bool>::iterator i = std::find(bm.begin(), bm.end(), 0);
    if (i == bm.end()) {
        throw std::runtime_error("out of sockets in allocate_pid");
    }
    *i = 1;
    return (::getpid () & process_mask) | ((i - bm.begin()) << process_bits);
}

/* Makes the specified 'pid' available for reuse. */
void free_pid(uint32_t pid)
{
    assert(avail_sockets[pid >> process_bits]);
    avail_sockets[pid >> process_bits] = 0;
}

} // null namespace

namespace vigil {

static Vlog_module log("netlink");

/* Construct a set of socket parameters for connecting to the given Netlink
 * 'protocol' (NETLINK_ROUTE, NETLINK_GENERIC, ...). */
Nl_socket::Parameters::Parameters(int protocol_)
    : protocol(protocol_), send_buffer(0), receive_buffer(0), group(0)
{ }

/* Sets the socket parameters to join the given multicast 'group'.
 *
 * In theory a netlink socket could join any number of multicast groups, but we
 * only need support for a single group, so that's all we allow for now. */
void Nl_socket::Parameters::join_multicast_group(int group_)
{
    assert(group == 0);
    group = group_;
}

/* Sets the socket parameters to use the given kernel 'send_buffer' size, in
 * bytes.  It is not clear why this is useful. */
void Nl_socket::Parameters::set_send_buffer(size_t send_buffer_)
{
    send_buffer = send_buffer_;
}

/* Sets the socket parameters to use the given kernel 'receive_buffer' size, in
 * bytes.  This is useful for querying flow tables, since large numbers of
 * flows can easily overflow the kernel's default socket buffer size. */
void Nl_socket::Parameters::set_receive_buffer(size_t receive_buffer_)
{
    receive_buffer = receive_buffer_;
}

/* Constructs a new Netlink socket.
 * The socket must be bound with a call to bind() before it can be used to send
 * or receive packets. */
Nl_socket::Nl_socket()
{
    m_pid = alloc_pid();
}

/* Destructs the socket. */
Nl_socket::~Nl_socket()
{
    free_pid(m_pid);
}

/* Binds the Netlink socket according to 'p', and connects the remote address
 * as the kernel.
 * Returns 0 if successful, otherwise a Unix error number.
 *
 * A bound Netlink socket may be rebound at any time if desired. */
int Nl_socket::bind(const Parameters& p)
{
    fd.reset(::socket(AF_NETLINK, SOCK_RAW, p.protocol));
    if (fd < 0) {
        return errno;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        log.err("fcntl: %s", strerror(errno));
        return errno;
    }

    if (p.send_buffer != 0
        && ::setsockopt(fd, SOL_SOCKET, SO_SNDBUF,
                        &p.send_buffer, sizeof p.send_buffer) < 0) {
        log.err("setsockopt(SO_SNDBUF,%zu): %s",
                p.send_buffer, strerror(errno));
        return errno;
    }

    if (p.receive_buffer != 0
        && ::setsockopt(fd, SOL_SOCKET, SO_RCVBUF,
                        &p.receive_buffer, sizeof p.receive_buffer) < 0) {
        log.err("setsockopt(SO_RCVBUF,%zu): %s",
                p.send_buffer, strerror(errno));
        return errno;
    }

    /* Bind local address as our selected pid. */
    sockaddr_nl local;
    memset(&local, 0, sizeof local);
    local.nl_family = AF_NETLINK;
    local.nl_pid = m_pid;
    if (p.group > 0 && p.group <= 32) {
        local.nl_groups |= 1ul << (p.group - 1);
    }
    if (::bind(fd, (sockaddr*) &local, sizeof local) < 0) {
        log.err("bind(%"PRIu32"): %s", m_pid, strerror(errno));
        return errno;
    }

    /* Bind remote address as the kernel (pid 0). */
    sockaddr_nl remote;
    memset(&remote, 0, sizeof remote);
    remote.nl_family = AF_NETLINK;
    remote.nl_pid = 0;
    if (::connect(fd, (sockaddr*) &remote, sizeof remote) < 0) {
        log.err("connect(0): %s", strerror(errno));
        return errno;
    }

    if (p.group > 32
        && ::setsockopt(fd, SOL_NETLINK, NETLINK_ADD_MEMBERSHIP,
                        &p.group, sizeof p.group) < 0) {
        log.err("setsockopt(NETLINK_ADD_MEMBERSHIP,%d): %s",
                p.group, strerror(errno));
        return errno;
    }

    return 0;
}

/* Closes the Netlink socket. */
int Nl_socket::close()
{
    return fd.close();
}

void Nl_socket::recv_wait()
{
    co_fd_read_wait(fd, NULL);
}

void Nl_socket::send_wait()
{
    co_fd_write_wait(fd, NULL);
}

/* Calls ::recv on fd with the specified arguments, with basic handling for
 * error conditions, and passes along ::recv's return value. */
ssize_t
Nl_socket::call_recv(void* buf, size_t nbytes, int flags)
{
    for (;;) {
        ssize_t retval = ::recv(fd, buf, nbytes, flags);
        if (retval < 0) {
            if (errno == ENOBUFS) {
                log.warn("lost packet (high data rate?)");
                retransmit_all();
                continue;
            } else {
                if (errno != EAGAIN) {
                    log.err("recv: %s", strerror(errno)); 
                }
                return -errno;
            }
        }
        return retval;
    }
}

std::auto_ptr<Buffer>
Nl_socket::do_recv(int& error)
{
    for (;;) {
        /* First find out the length of the message. */
        uint8_t tmp;
        error = call_recv(&tmp, 1, MSG_PEEK | MSG_TRUNC);
        if (error <= 0) {
            error = -error;
            return std::auto_ptr<Buffer>(0);
        }

        /* Then read the whole message. */
        std::auto_ptr<Buffer> buffer(new Array_buffer(error));
        error = call_recv(buffer->data(), error, 0);
        if (error < 0) {
            error = -error;
            return std::auto_ptr<Buffer>(0);
        }
        buffer->trim(error);

        Nl_message msg(buffer);
        if (!msg.ok()) {
            log.warn("received invalid Netlink message");
            error = EPROTO;
            return std::auto_ptr<Buffer>(0);
        }

        uint32_t seq = msg.nlhdr().nlmsg_seq;
        Transaction_map::iterator i(transactions.find(seq));
        if (i == transactions.end()) {
            error = 0;
            return msg.steal();
        }

        Nl_transaction& t = *i->second;
        if (t.status < 0) {
            t.reply = msg.steal();
            t.status = 0;
            t.wakeup.broadcast();
        }
    }
}

int Nl_socket::do_send(const Buffer& msg)
{
    int retval = ::write(fd, msg.data(), msg.size());
    return retval < 0 ? errno : 0;
}

std::auto_ptr<Buffer>
Nl_socket::transact(Nl_transaction& t, int& error, bool block)
{
    co_might_yield_if(block);
    for (;;) {
        std::auto_ptr<Buffer> b(do_transact(t, error));
        if (error == EAGAIN && block) {
            co_might_yield();
            transact_wait(t);
            co_block();
        } else {
            return b;
        }
    }
}

std::auto_ptr<Buffer>
Nl_socket::transact(Nl_message& request, int& error)
{
    Nl_transaction t(*this, request);
    std::auto_ptr<Buffer> b(transact(t, error, true));
    return b;
}

/* Sends 'msg' to the kernel and waits for a response.
 *
 * This function is reliable even though Netlink is an unreliable datagram
 * transport.  The message send to the kernel is reliable: the kernel will tell
 * us if the message cannot be queued (and we will in that case put it on the
 * transmit queue and wait until it can be delivered).  Receiving the reply is
 * the real problem: if the socket buffer is full when the kernel tries to send
 * the reply, the reply will be dropped.  However, the kernel lets us know that
 * a reply was dropped by returning ENOBUFS from ::recv.  We deal with this by
 * resending all outstanding requests.
 *
 * This retransmission mechanism only functions properly for idempotent
 * requests.
 *
 * It's best not to mix transactional and non-transactional messages on a
 * single socket, as non-transactional data can get dropped even *after* it is
 * received because we're waiting for the reply to a transaction.
 */
std::auto_ptr<Buffer>
Nl_socket::do_transact(Nl_transaction& t, int& error)
{
    for (;;) {
        switch (t.status) {
        case Nl_transaction::NEED_RETRANSMIT:
            error = t.retransmit();
            if (error) {
                return std::auto_ptr<Buffer>(0);
            }
            break;

        case Nl_transaction::NEED_REPLY:
            /* Receive a message.
             *
             * If the message is not part of a transaction, it will get thrown
             * away.  We could queue a few received non-transactional messages,
             * but we'd still have to start throwing them away after a while.
             * It's probably a better idea to use separate Netlink sockets for
             * transactional and non-transactional traffic.
             *
             * Also, we can get duplicate responses to transactional messages,
             * in the case where we retransmit a request that doesn't need to
             * be, and those shouldn't be queued at all. */
            do_recv(error);
            if (error) {
                return std::auto_ptr<Buffer>(0);
            }
            break;

        case 0: {
            Nl_message m(t.reply);
            if (!m.ok()) {
                log.err("Nl_message too short");
                error = EPROTO;
                return std::auto_ptr<Buffer>(0);
            }

            if (m.nlhdr().nlmsg_type == NLMSG_ERROR) {
                const nlmsgerr* nlerr = m.pull<nlmsgerr>();
                if (!nlerr) {
                    log.err("NLMSG_ERROR packet too short");
                    error = EPROTO;
                    return std::auto_ptr<Buffer>(0);
                } else if (nlerr->error < 0) {
                    error = -nlerr->error;
                    if (error == EAGAIN) {
                        error = EPROTO;
                    }
                    log.err("NLMSG_ERROR received from kernel: %d (%s)",
                            error, strerror(error));
                    return std::auto_ptr<Buffer>(0);
                }
            }

            error = 0;
            return m.steal();
        }

        default:
            assert(t.status > 0);
            error = t.status;
            return std::auto_ptr<Buffer>(0);
        }
    }
}

void
Nl_socket::retransmit_all()
{
    BOOST_FOREACH (Transaction_map::value_type& i, transactions) {
        Nl_transaction& t = *i.second;
        if (t.status == Nl_transaction::NEED_REPLY) {
            t.status = Nl_transaction::NEED_RETRANSMIT;
            t.retransmit();

            /* Transaction may be waiting to receive a message, but now it
             * needs to wait to send a message, so wake it up. */
            t.wakeup.broadcast();
        } else {
            /* Transaction is already waiting to retransmit.  Nothing to do. */
        }
    }
}

void
Nl_socket::transact_wait(Nl_transaction& t)
{
    /* Need to retransmit? */
    int error = t.retransmit();
    if (error) {
        if (error == EAGAIN) {
            co_fd_write_wait(fd, NULL);
        } else {
            co_immediate_wake(1, NULL);
        }
        return;
    }

    /* Need to receive? */
    if (t.status == Nl_transaction::NEED_REPLY) {
        /* Need to wait for messages to arrive. */
        co_fd_read_wait(fd, NULL);

        /* Also need to wait for someone else to get our message and
         * complete the transaction for us.  We won't necessarily be
         * awakened by the FD becoming ready, because another thread can
         * read from the FD without ever coming through the poll loop. */
        t.wakeup.wait();
        return;
    }

    /* Transaction complete! */
    co_immediate_wake(1, NULL);
}

Nl_transaction::Nl_transaction(Nl_socket& socket, Nl_message& request)
    : status(NEED_RETRANSMIT),
      socket(socket),
      request(request),
      reply(0)
{
    request.nlhdr().nlmsg_flags |= NLM_F_ACK;
    if (!socket.transactions.insert(std::make_pair(seq(), this)).second) {
        NOT_REACHED();
    }
}

Nl_transaction::~Nl_transaction() 
{
    if (socket.transactions.erase(seq()) != 1) {
        NOT_REACHED();
    }
}

uint32_t
Nl_transaction::seq() 
{
    return request.nlhdr().nlmsg_seq;
}

int
Nl_transaction::retransmit()
{
    int error = 0;
    if (status == NEED_RETRANSMIT) {
        error = socket.send(request.all(), false);
        if (!error) {
            status = NEED_REPLY;
        } else if (error != EAGAIN) {
            status = error;
        }
    }
    return error;
}

/* Returns a new sequence number for sending a request.
 *
 * This implementation uses sequence numbers that are unique process-wide, to
 * avoid a hypothetical race: send request, close socket, open new socket that
 * reuses the old socket's PID value, send request on new socket, receive reply
 * from kernel to old socket but with same PID and sequence number.  This race
 * could be avoided other ways, so process-wide uniqueness is not a guarantee
 * of the interface and thus next_seq() is a non-static member function. */
uint32_t Nl_socket::next_seq()
{
    static uint32_t seq = time(0);
    return seq++;
}

/* Nl_message. */

/* Constructs an Nl_message for parsing the message in 'buffer'.
 *
 * Pulls the nlmsghdr from 'buffer'.  If 'buffer' is shorter than an nlmsghdr,
 * ok() will return false. */
Nl_message::Nl_message(std::auto_ptr<Buffer> buffer_)
    : buffer(buffer_), offset(0)
{
    pull<nlmsghdr>();
}

/* Constructs an Nl_message for composing a Netlink message with the given
 * 'type' and 'flags'.  'sock' is used to obtain a PID and sequence number for
 * proper routing of replies.  'expected_payload' may be specified as an
 * estimate of the number of payload bytes to be supplied; if the size of the
 * payload is unknown the default of 0 is acceptable.
 *
 * 'type' is ordinarily an enumerated value specific to the Netlink protocol
 * (e.g. RTM_NEWLINK, for NETLINK_ROUTE protocol).  For Generic Netlink, 'type'
 * is the family number obtained via Genl_family::number().
 *
 * 'flags' is a bit-mask that indicates what kind of request is being made.  It
 * is often NLM_F_REQUEST indicating that a request is being made, commonly
 * or'd with NLM_F_ACK to request an acknowledgement.
 *
 * Genl_message::Genl_message() provides a more convenient interface for
 * composing a Generic Netlink message. */
Nl_message::Nl_message(uint32_t type, uint32_t flags, Nl_socket& sock,
                       size_t expected_payload)
{
    size_t size = NLMSG_HDRLEN + NLMSG_ALIGN(expected_payload);
    buffer.reset(new Array_buffer(size));
    buffer->trim(0);
    offset = 0;

    nlmsghdr* h = put<nlmsghdr>();
    h->nlmsg_len = buffer->size();
    h->nlmsg_type = type;
    h->nlmsg_flags = flags;
    h->nlmsg_seq = sock.next_seq();
    h->nlmsg_pid = sock.pid();
}

/* Attempts to pull 'nbytes' of data (rounded up to Netlink alignment) off the
 * front of the message.  Returns a pointer to the data if successful, a null
 * pointer if the message is too small. */
uint8_t* Nl_message::pull(size_t nbytes)
{
    size_t start = offset;
    offset += NLMSG_ALIGN(nbytes);
    return ok() ? buffer->data() + start : NULL;
}

/* Returns true if everything is OK, false if more bytes have been pulled from
 * the message than were available (i.e. the message is invalid). */
bool Nl_message::ok() const
{
    return offset <= buffer->size();
}

/* Returns the message's nlmsghdr.  May be used only if ok(). */
nlmsghdr& Nl_message::nlhdr() const
{
    assert(buffer->size() >= NLMSG_HDRLEN);
    return buffer->at<nlmsghdr>(0);
}

static void nlmsghdr_to_string(const nlmsghdr& h, char *p)
{
    sprintf(p, "nl(len:%"PRIu32", type=%"PRIu16,
            h.nlmsg_len, h.nlmsg_type);
    if (h.nlmsg_type == NLMSG_NOOP) {
        strcat(p, "(no-op)");
    } else if (h.nlmsg_type == NLMSG_ERROR) {
        strcat(p, "(error)");
    } else if (h.nlmsg_type == NLMSG_DONE) {
        strcat(p, "(done)");
    } else if (h.nlmsg_type == NLMSG_OVERRUN) {
        strcat(p, "(overrun)");
    } else if (h.nlmsg_type < NLMSG_MIN_TYPE) {
        strcat(p, "(reserved)");
    } else {
        strcat(p, "(family-defined)");
    }
    sprintf(strchr(p, '\0'), ", flags=%"PRIu16, h.nlmsg_flags);
    if (h.nlmsg_flags & NLM_F_REQUEST) {
        strcat(p, "[REQUEST]");
    }
    if (h.nlmsg_flags & NLM_F_MULTI) {
        strcat(p, "[MULTI]");
    }
    if (h.nlmsg_flags & NLM_F_ACK) {
        strcat(p, "[ACK]");
    }
    if (h.nlmsg_flags & NLM_F_ECHO) {
        strcat(p, "[ECHO]");
    }
    if (h.nlmsg_flags & ~(NLM_F_REQUEST | NLM_F_MULTI
                          | NLM_F_ACK | NLM_F_ECHO)) {
        strcat(p, "[OTHER]");
    }
    if (h.nlmsg_flags == 0) {
        strcat(p, "0");
    }
    sprintf(strchr(p, '\0'), ", seq=%"PRIx32", pid=%"PRIu32"(%d:%d))",
            h.nlmsg_seq, h.nlmsg_pid, (int) (h.nlmsg_pid & process_mask),
            (int) (h.nlmsg_pid >> process_bits));
}

std::string Nl_message::to_string() const 
{
    if (buffer->size() < NLMSG_HDRLEN) {
        return "nl(truncated)";
    } else {
        char buf[4096];
        nlmsghdr& h = nlhdr();
        nlmsghdr_to_string(h, buf);
        if (h.nlmsg_type == NLMSG_ERROR) {
            if (all().size()
                < NLMSG_HDRLEN + NLMSG_ALIGN(sizeof(struct nlmsgerr))) {
                strcat(buf, " error(truncated)");
            } else {
                const nlmsgerr& e = all().at<nlmsgerr>(NLMSG_HDRLEN);
                sprintf(strchr(buf, '\0'), " error(%d", e.error);
                if (e.error < 0) {
                    sprintf(strchr(buf, '\0'), "(%s)", strerror(-e.error));
                }
                strcat(buf, ", in-reply-to(");
                nlmsghdr_to_string(e.msg, strchr(buf, '\0'));
                strcat(buf, "))");
            }
        }
        return buf;
    }
}

/* Returns the part of the message that has not been pulled off as headers.
 * May be used only if ok(). */
Nonowning_buffer Nl_message::rest() const
{
    assert(ok());
    return Nonowning_buffer(*buffer, offset);
}

/* Returns the entire message, including data that has been pulled off as
 * headers. */
Buffer& Nl_message::all()
{
    return *buffer;
}
const Buffer& Nl_message::all() const
{
    return *buffer;
}

/* Appends room for 'size' bytes (plus padding for Netlink) alignment to the
 * message and returns the first byte.  Any appended padding is cleared to
 * zeros. */
uint8_t* Nl_message::put(size_t size)
{
    uint8_t* p = buffer->put(NLMSG_ALIGN(size));
    memset((char*) p + size, 0, NLMSG_ALIGN(size) - size);
    nlhdr().nlmsg_len = buffer->size();
    return p;
}

/* Transfers ownership of the buffer to the caller.  After calling this
 * function no other member function that accesses messages data may be
 * called. */
std::auto_ptr<Buffer> Nl_message::steal()
{
    return buffer;
}

/* Appends a Netlink attribute with the given 'type' and 'size' bytes of
 * 'content' to the message, together with appropriate padding. */
void Nl_message::put_unspec(uint16_t type, const void* content, size_t size)
{
    if (size > UINT16_MAX) {
        throw std::runtime_error("attribute too large");
    }

    nlattr* nla = put<nlattr>();
    nla->nla_type = type;
    nla->nla_len = size + NLA_HDRLEN;

    memcpy(put(size), content, size);
}

/* Constructs a Genl_message for parsing the message in 'buffer'.
 *
 * Pulls the nlmsghdr and genlmsghdr from 'buffer'.  If 'buffer' is shorter
 * than the concatenation of these headers, ok() will return false. */
Genl_message::Genl_message(std::auto_ptr<Buffer> buffer)
    : Nl_message(buffer)
{
    pull<genlmsghdr>();
}

/* Constructs a Genl_message for composing a Generic Netlink message with the
 * given 'family', 'flags', 'cmd', and 'version'.  'sock' is used to obtain a
 * PID and sequence number for proper routing of replies.  'expected_payload'
 * may be specified as an estimate of the number of payload bytes to be
 * supplied; if the size of the payload is unknown the default of 0 is
 * acceptable.
 *
 * 'family' is the family number obtained via Genl_family::number().
 *
 * 'flags' is a bit-mask that indicates what kind of request is being made.  It
 * is often NLM_F_REQUEST indicating that a request is being made, commonly
 * or'd with NLM_F_ACK to request an acknowledgement.
 *
 * 'cmd' is an enumerated value specific to the Generic Netlink family
 * (e.g. CTRL_CMD_NEWFAMILY for the GENL_ID_CTRL family).
 *
 * 'version' is a version number specific to the family and command (often 1).
 *
 * Nl_message::Nl_message() should be used to compose Netlink messages that are
 * not Generic Netlink messages. */
Genl_message::Genl_message(int family, uint32_t flags,
                           uint8_t cmd, uint8_t version,
                           Nl_socket& sock, size_t expected_payload)
    : Nl_message(family, flags, sock, GENL_HDRLEN + expected_payload)
{
    genlmsghdr* h = put<genlmsghdr>();
    h->cmd = cmd;
    h->version = version;
    h->reserved = 0;
}

genlmsghdr& Genl_message::genlhdr() const 
{
    assert(all().size() >= NLMSG_HDRLEN + GENL_HDRLEN);
    return all().at<genlmsghdr>(NLMSG_HDRLEN);
}

std::string Genl_message::to_string() const
{
    std::string base = Nl_message::to_string();
    if (all().size() < NLMSG_HDRLEN + GENL_HDRLEN) {
        return base + " genl(truncated)";
    } else if (nlhdr().nlmsg_type >= GENL_MIN_ID
               && nlhdr().nlmsg_type <= GENL_MAX_ID) {
        genlmsghdr& h = genlhdr();
        char buf[256];
        sprintf(buf, "genl(cmd:%"PRIu8", version:%"PRIu8, h.cmd, h.version);
        if (h.reserved) {
            sprintf(strchr(buf, '\0'), ", reserved=%"PRIx16, h.reserved);
        }
        strcat(buf, ")");
        return base + " " + buf;
    } else {
        return base;
    }
}

namespace {

/* Returns the address of the payload for nlattr 'nla' as type T*. */
template <class T>
const T* nla_data(const nlattr* nla)
{
    assert(nla->nla_len >= NLA_HDRLEN + sizeof(T));
    return (const T*) (((const char*) nla) + NLA_HDRLEN);
}

} // null namespace

/* Returns the payload of nlattr 'nla' as type 'const void*'. */
const void* get_data(const nlattr* nla)
{
    assert(nla->nla_len >= NLA_HDRLEN);
    return (const void*) (((const char*) nla) + NLA_HDRLEN);
}

/* Returns the payload of nlattr 'nla' as type 'void*'. */
void* get_data(nlattr* nla)
{
    assert(nla->nla_len >= NLA_HDRLEN);
    return (void*) (((char*) nla) + NLA_HDRLEN);
}

/* Returns true if 'nla' is a "true" flag attribute.
 *
 * Flag attributes are different from all the other kinds of attributes in that
 * they carry no data.  Instead, the very presence of a flag attribute
 * indicates that the flag is set.  (Thus, a "required" flag attribute carries
 * no information.) */
bool get_flag(const nlattr* nla)
{
    return nla != NULL;
}

/* Returns the value of u8 attribute 'nla'. */
uint8_t get_u8(const nlattr* nla)
{
    return *nla_data<uint8_t>(nla);
}

/* Returns the value of u16 attribute 'nla'. */
uint16_t get_u16(const nlattr* nla)
{
    return *nla_data<uint16_t>(nla);
}

/* Returns the value of u32 attribute 'nla'. */
uint32_t get_u32(const nlattr* nla)
{
    return *nla_data<uint32_t>(nla);
}

/* Returns the value of u64 attribute 'nla'. */
uint64_t get_u64(const nlattr* nla)
{
    return *nla_data<uint64_t>(nla);
}

/* Returns the value of string attribute 'nla'.
 *
 * FIXME?  Quite possibly we should drop the first null byte and any bytes
 * following it. */
std::string get_string(const nlattr* nla)
{
    return std::string(nla_data<char>(nla), nla->nla_len - NLA_HDRLEN);
}

/* Returns the value of null-terminated string attribute 'nla'.
 *
 * FIXME?  Quite possibly we should drop the first null byte and any bytes
 * following it. */
std::string get_nul_string(const nlattr* nla)
{
    return std::string(nla_data<char>(nla), nla->nla_len - NLA_HDRLEN - 1);
}

/* Implementation of template<> Nl_policy::Nl_policy. */
void Nl_policy::init(Nl_attr elements_[], size_t n)
{
    n_required = 0;
    for (size_t i = 0; i < n; ++i) {
        uint16_t attr_type = elements_[i].attr_type;

        if (elements.size() <= attr_type) {
            elements.resize(attr_type + 1);
        }
        assert(elements[attr_type] == NULL);
        elements[attr_type] = &elements_[i];
        if (elements[attr_type]->required) {
            ++n_required;
        }
    }
}

/* Implementation of template<> Nl_policy::parse. */
bool Nl_policy::parse(const Buffer& b_, nlattr* attrs[], size_t n)
{
    assert(n >= elements.size());
    for (size_t i = 0; i < n; i++) {
        attrs[i] = NULL;
    }

    size_t still_required = n_required;
    Nonowning_buffer b = b_;
    while (b.size() > 0) {
        nlattr* nla = b.try_pull<nlattr>();
        if (!nla) {
            log.warn("attribute shorter than NLA_HDRLEN (%zu)",
                     b.size());
            return false;
        }

        /* Make sure its claimed length is plausible. */
        if (nla->nla_len < NLA_HDRLEN) {
            log.warn("attribute shorter than NLA_HDRLEN (%"PRIu16")",
                    nla->nla_len);
            return false;
        }
        size_t len = nla->nla_len - NLA_HDRLEN;
        size_t aligned_len = NLA_ALIGN(len);
        if (aligned_len > b.size()) {
            log.warn("attribute %"PRIu16" aligned data len (%zu) "
                    "> bytes left (%zu)",
                    nla->nla_type, aligned_len, b.size());
            return false;
        }

        uint16_t type = nla->nla_type;
        if (type < elements.size() && elements[type] != NULL) {
            const Nl_attr& e = *elements[type];

            /* Validate length and content. */
            if (len < e.min_len || len > e.max_len) {
                log.warn("attribute %"PRIu16" length %zu not in allowed range "
                        "%zu...%zu", type, len, e.min_len, e.max_len);
                return false;
            }
            if (e.value_type == Nl_attr::NUL_STRING
                && ((char*) nla)[nla->nla_len - 1] != '\0') {
                log.warn("attribute %"PRIu16" lacks null terminator", type);
                return false;
            }

            if (e.required && attrs[type] == NULL) {
                --still_required;
            }
            attrs[type] = nla;
        } else {
            /* Skip attribute type that we don't care about. */
        }

        b.pull(aligned_len);
    }
    if (still_required) {
        log.warn("%zu required attributes missing", still_required);
        return false;
    }
    return true;
}

static std::tr1::array<Nl_attr, 1> family_policy_elems = { {
        Nl_attr::u16(CTRL_ATTR_FAMILY_ID).require(),
    } };

static Nl_policy family_policy(family_policy_elems);

class Genl_family_transactor 
{
public:
    Genl_family_transactor(int* number, Co_cond* wakeup,
                           const std::string& name);
    ~Genl_family_transactor() { co_fsm_kill(fsm); }
    void completion_wait();
private:
    int* number;
    Co_cond* wakeup;
    Nl_socket socket;
    Genl_message request;
    Nl_transaction transaction;
    co_thread* fsm;

    void run();
};

Genl_family_transactor::Genl_family_transactor(int* number_, Co_cond* wakeup_,
                                               const std::string& name)
    : number(number_), wakeup(wakeup_), socket(),
      request(GENL_ID_CTRL, NLM_F_REQUEST, CTRL_CMD_GETFAMILY, 2, socket),
      transaction(socket, request)
{
    int error = socket.bind(NETLINK_GENERIC);
    if (error) {
        *number = -error;
        return;
    }

    request.put_string(CTRL_ATTR_FAMILY_NAME, name);

    fsm = co_fsm_create(&co_group_coop,
                        boost::bind(&Genl_family_transactor::run, this));
}

void
Genl_family_transactor::run()
{
    int error;
    std::auto_ptr<Buffer> reply_buf(socket.transact(transaction, error,
                                                    false));
    if (error != EAGAIN) {
        if (!error) {
            Genl_message reply(reply_buf);
            std::tr1::array<nlattr*, CTRL_ATTR_MAX + 1> attrs;
            if (family_policy.parse(reply.rest(), attrs)) {
                *number = get_u16(attrs[CTRL_ATTR_FAMILY_ID]);
            } else {
                *number = -EPROTO;
            }
        } else {
            *number = -error;
        }
        wakeup->broadcast();
        co_fsm_exit();
        fsm = NULL;
    } else {
        socket.transact_wait(transaction);
        co_fsm_block();
    }
}

void
Genl_family_transactor::completion_wait()
{
    socket.transact_wait(transaction);
}

Genl_family::Genl_family(const std::string& name_)
    : transactor(NULL), name(name_), number(0)
{
}

Genl_family::~Genl_family()
{
    delete transactor;
}

/* Looks up the binding for the Generic Netlink family name.  Returns a
 * positive family number if successful, otherwise a negative errno value.
 *
 * The binding is cached: after the first resolution, no additional netlink
 * traffic will take place. */
int Genl_family::lookup(int& family, bool block)
{
    if (number == 0) {
        if (!transactor) {
            transactor = new Genl_family_transactor(&number, &wakeup, name);
        }
        if (!number) {
            if (!block) {
                return EAGAIN;
            }
            wakeup.block();
        } else {
            /* Don't wait: transaction completed in constructor. */
        }
        assert(number);
        delete transactor;
        transactor = NULL;
    }
    family = number;
    return number > 0 ? 0 : -number;
}

void Genl_family::lookup_wait()
{
    if (!transactor) {
        co_immediate_wake(1, NULL);
    } else {
        transactor->completion_wait();
    }
}

} // namespace vigil
