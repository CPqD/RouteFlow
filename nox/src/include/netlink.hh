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
/* Netlink interface.
 *
 * Netlink is a datagram-based network protocol primarily for communication
 * between user processes and the kernel, and mainly on Linux.  Netlink is
 * specified in RFC 3549, "Linux Netlink as an IP Services Protocol".
 *
 * Netlink is not suitable for use in physical networks of heterogeneous
 * machines because host byte order is used throughout. */

#ifndef NETLINK_HH
#define NETLINK_HH 1

#include <boost/noncopyable.hpp>
#include <memory>
#include <queue>
#include <stdint.h>
#include <tr1/array>
#include "async_io.hh"
#include "auto_fd.hh"
#include "buffer.hh"
#include "threads/cooperative.hh"
#include "hash_map.hh"

struct nlattr;
struct nlmsghdr;
struct genlmsghdr;

namespace vigil {

class Nl_message;
class Nl_transaction;

/* A Netlink socket. */
class Nl_socket
    : public Async_datagram
{
public:
    /* Parameters for binding a Netlink socket. */
    class Parameters
    {
    public:
        Parameters(int protocol_);
        void join_multicast_group(int group);
        void set_send_buffer(size_t);
        void set_receive_buffer(size_t);
    private:
        friend class Nl_socket;
        int protocol;
        size_t send_buffer;
        size_t receive_buffer;
        int group;
    };

    /* Socket setup. */
    Nl_socket();
    ~Nl_socket();
    int bind(const Parameters&);
    int close();

    /* Sending and receiving individual datagrams.
     *
     * For sending a request and receiving a reply, transact() may be more
     * convenient. */
    void recv_wait();
    void send_wait();

    /* Transactions.
     *
     * Send a request and receive a reply, with retransmission as necessary to
     * ensure reliability. */
    std::auto_ptr<Buffer> transact(Nl_transaction&, int& error, bool block);
    void transact_wait(Nl_transaction&);

    std::auto_ptr<Buffer> transact(Nl_message&, int& error);

    uint32_t pid() { return m_pid; }
    uint32_t next_seq();

private:
    friend class Nl_transaction;
    
    Auto_fd fd;                 /* Underlying file descriptor. */
    uint32_t m_pid;             /* Netlink PID. */

    typedef hash_map<uint32_t, Nl_transaction*> Transaction_map;
    Transaction_map transactions;

    std::auto_ptr<Buffer> do_recv(int& error);
    ssize_t call_recv(void*, size_t nbytes, int flags);

    int do_send(const Buffer&);

    std::auto_ptr<Buffer> do_transact(Nl_transaction&, int& error);

    void retransmit_all();

    Nl_socket(const Nl_socket&);
    Nl_socket& operator=(const Nl_socket&);
};

class Nl_transaction {
public:
    Nl_transaction(Nl_socket& socket, Nl_message& request);
    ~Nl_transaction();
private:
    friend class Nl_socket;

    enum {
        NEED_RETRANSMIT = -2,
        NEED_REPLY = -1
    };

    /* status == NEED_RETRANSMIT: Need to (re)transmit request.
     * status == NEED_REPLY: Waiting for reply.
     * status == 0: Success (reply must be nonnull).
     * status > 0: Positive errno value describing failure. */
    int status;

    Nl_socket& socket;
    Nl_message& request;
    std::auto_ptr<Buffer> reply;
    Co_cond wakeup;

    int retransmit();
    uint32_t seq();
};

/* A Netlink message.
 *
 * This is a wrapper for Buffer that is specialized for composing and parsing
 * Netlink messages.  Probably, much of its functionality should be generalized
 * and integrated back into Buffer, but for now it seems to work OK.
 *
 * It is unclear whether a single class should be used for both composing and
 * parsing messages, since these are fairly different usage patterns.
 *
 * The extensions beyond Buffer have these purposes:
 *
 *      - Provide the ability to "pull" possibly missing headers.  If they are
 *        missing, ok() will return false.  (A bare Buffer has no "invalid"
 *        state, at the moment.)
 *
 *      - Handle Netlink attributes.
 *
 *      - Pad all pulls and puts with NLMSG_ALIGN (in practice, 4-byte
 *        alignment).
 */
class Nl_message
{
public:
    Nl_message(std::auto_ptr<Buffer> buffer_);
    Nl_message(uint32_t type, uint32_t flags, Nl_socket&,
               size_t expected_payload = 0);

    /* Extracting data. */
    uint8_t* pull(size_t);
    template <class T> T* pull();
    bool ok() const;
    Nonowning_buffer rest() const;
    Buffer& all();
    const Buffer& all() const;

    nlmsghdr& nlhdr() const;

    std::string to_string() const;

    /* Adding data. */
    uint8_t* put(size_t);
    template <class T> T* put();

    /* Taking possession of the data. */
    std::auto_ptr<Buffer> steal();

    /* Adding attributes. */
    void put_unspec(uint16_t type, const void*, size_t);
    void put_flag(uint16_t type) { put_unspec(type, NULL, 0); }
    void put_u8(uint16_t type, uint8_t value) { put(type, value); }
    void put_u16(uint16_t type, uint16_t value) { put(type, value); }
    void put_u32(uint16_t type, uint32_t value) { put(type, value); }
    void put_u64(uint16_t type, uint64_t value) { put(type, value); }
    void put_string(uint16_t type, const std::string& s)
        { put_unspec(type, s.c_str(), s.size() + 1); }
    void put_nested(uint16_t type, const Nl_message& msg)
        { put_unspec(type, msg.buffer->data(), msg.buffer->size()); }

private:
    std::auto_ptr<Buffer> buffer;
    size_t offset;

    Nl_message();
    Nl_message(const Nl_message&);
    Nl_message& operator=(const Nl_message&);

    /* Appends a Netlink attribute with the given 'type' and 'value' to the
       message, together with appropriate padding. */
    template <class T>
    void put(uint16_t type, T value)
        { put_unspec(type, &value, sizeof value); }
};

template <class T>
T* Nl_message::pull()
{
    return reinterpret_cast<T*>(pull(sizeof(T)));
}

template <class T>
T* Nl_message::put()
{
    return reinterpret_cast<T*>(put(sizeof(T)));
}

/* A Generic Netlink message.
 *
 * This is a Nl_message that handles the Generic Netlink header
 * automatically. */
class Genl_message
    : public Nl_message
{
public:
    Genl_message(std::auto_ptr<Buffer>);
    Genl_message(int family, uint32_t flags, uint8_t cmd, uint8_t version,
                 Nl_socket&, size_t expected_payload = 0);

    genlmsghdr& genlhdr() const;

    std::string to_string() const;

private:
    Genl_message();
    Genl_message(const Genl_message&);
    Genl_message& operator=(const Genl_message&);

    void put_genlhdr(uint8_t cmd, uint8_t version);
};

/* Netlink attribute parsing. */
const void* get_data(const nlattr*);
void* get_data(nlattr*);
bool get_flag(const nlattr*);
uint8_t get_u8(const nlattr*);
uint16_t get_u16(const nlattr*);
uint32_t get_u32(const nlattr*);
uint64_t get_u64(const nlattr*);
std::string get_string(const nlattr*);
std::string get_nul_string(const nlattr*);

/* Netlink attribute policy.
 *
 * Specifies how to parse a single attribute from a Netlink message payload.
 *
 * See Nl_policy for example.
 */
class Nl_attr
{
public:
    enum Type {
        SENTINEL = -1,
        UNSPEC,
        U8,
        U16,
        U32,
        U64,
        STRING,
        FLAG,
        MSECS,
        NESTED,
        NESTED_COMPAT,
        NUL_STRING,
        BINARY
    };

    uint16_t attr_type;
    Type value_type;
    size_t min_len, max_len;
    bool required;

    static Nl_attr unspec(uint16_t type, size_t min_len, size_t max_len)
        { return Nl_attr(type, UNSPEC, min_len, max_len); }
    static Nl_attr u8(uint16_t type)
        { return Nl_attr(type, U8, sizeof(uint8_t)); }
    static Nl_attr u16(uint16_t type)
        { return Nl_attr(type, U16, sizeof(uint16_t)); }
    static Nl_attr u32(uint16_t type)
        { return Nl_attr(type, U32, sizeof(uint32_t)); }
    static Nl_attr u64(uint16_t type)
        { return Nl_attr(type, U64, sizeof(uint64_t)); }
    static Nl_attr string(uint16_t type, size_t min_len, size_t max_len)
        { return Nl_attr(type, STRING, min_len, max_len); }
    static Nl_attr nul_string(uint16_t type, size_t min_len,
                              size_t max_len)
        { return Nl_attr(type, NUL_STRING, min_len + 1, max_len + 1); }
    static Nl_attr flag(uint16_t type)
        { return Nl_attr(type, FLAG, 0); }
    static Nl_attr binary(uint16_t type, size_t min_len, size_t max_len)
        { return Nl_attr(type, BINARY, min_len, max_len); }

    Nl_attr require()
        { required = true; return *this; }
private:
    Nl_attr(uint16_t attr_type_, Type value_type_, size_t len)
        : attr_type(attr_type_), value_type(value_type_),
          min_len(len), max_len(len), required(false)
        { }
    Nl_attr(uint16_t attr_type_, Type value_type_, size_t min_len_,
            size_t max_len_)
        : attr_type(attr_type_), value_type(value_type_),
          min_len(min_len_), max_len(max_len_), required(false)
        { }
};

/* A Netlink message parsing policy.
 *
 * Specifies how to parse the collection of attributes that a Netlink message
 * may contain as its payload.
 *
 * Example declaration (should be in namespace scope):
 *
 *     std::tr1::array<Nl_attr, 3> dp_policy_elems = { {
 *             Nl_attr::u32(DP_GENL_A_DP_IDX).require(),
 *             Nl_attr::u32(DP_GENL_A_MC_GROUP).require(),
 *             Nl_attr::string(DP_GENL_A_PORTNAME, 1, IFNAMSIZ - 1),
 *     } };
 *
 *     Nl_policy dp_policy(dp_policy_elems);
 *
 * Example usage:
 *
 *     Genl_message msg(buffer);
 *     if (!msg.ok()) {
 *        ...handle error...
 *     }
 *
 *     std::tr1::array<nlattr*, DP_GENL_A_MAX + 1> attrs;
 *     if (!dp_policy.parse(msg.rest(), attrs)) {
 *        ...handle error...
 *     }
 *
 */
class Nl_policy
{
public:
    /* Constructs an Nl_policy with the N policy attributes in 'elements'.
     *
     * 'elements' must persist for the entire lifetime of the policy (see
     * above for typical usage). */
    template <size_t N>
    Nl_policy(std::tr1::array<Nl_attr, N>& elements) {
        init(&elements[0], N);
    }

    /* Parses 'buffer' according to the policy, filling in each element of
     * 'attrs' with a pointer to the corresponding attribute or (if the
     * attribute is not found) a null pointer.
     *
     * Returns true if the policy is satisfied, false if it is violated. */
    template <size_t N>
    bool parse(const Buffer& buffer, std::tr1::array<nlattr*, N>& attrs) {
        return parse(buffer, &attrs[0], N);
    }
private:
    std::vector<Nl_attr*> elements;
    size_t n_required;

    void init(Nl_attr[], size_t n);
    bool parse(const Buffer&, nlattr*[], size_t n);
};

/* A binding between the name of a Generic Netlink family and the number
 * currently used by the kernel for that family.
 *
 * Example declaration (should be in namespace scope):
 *
 *     Genl_family Datapath::openflow_family(DP_GENL_FAMILY_NAME);
 */
class Genl_family_transactor;
class Genl_family
    : boost::noncopyable
{
public:
    Genl_family(const std::string& name_);
    ~Genl_family();
    int lookup(int& family, bool block);
    void lookup_wait();
private:
    Genl_family_transactor* transactor;
    Co_cond wakeup;
    std::string name;
    int number;

    Genl_family();
    Genl_family(Genl_family&);
    Genl_family& operator=(const Genl_family&);
};

} // namespace vigil

#endif /* netlink.hh */
