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
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_array.hpp>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <stdint.h>

#include "openflow-default.hh"
#include "assert.hh"
#include "component.hh"
#include "flow.hh"
#include "fnv_hash.hh"
#include "hash_set.hh"
#include "packet-in.hh"
#include "vlog.hh"

#include "netinet++/ethernetaddr.hh"
#include "netinet++/ethernet.hh"

using namespace vigil;
using namespace vigil::container;

namespace {

struct Mac_source
{
    /* Key. */
    datapathid datapath_id;     /* Switch. */
    ethernetaddr mac;           /* Source MAC. */

    /* Value. */
    mutable int port;           /* Port where packets from 'mac' were seen. */

    Mac_source() : port(-1) { }
    Mac_source(datapathid datapath_id_, ethernetaddr mac_)
        : datapath_id(datapath_id_), mac(mac_), port(-1)
        { }
};

bool operator==(const Mac_source& a, const Mac_source& b)
{
    return a.datapath_id == b.datapath_id && a.mac == b.mac;
}

bool operator!=(const Mac_source& a, const Mac_source& b) 
{
    return !(a == b);
}

struct Hash_mac_source
{
    std::size_t operator()(const Mac_source& val) const {
        uint32_t x;
        x = vigil::fnv_hash(&val.datapath_id, sizeof val.datapath_id);
        x = vigil::fnv_hash(val.mac.octet, sizeof val.mac.octet, x);
        return x;
    }
};

Vlog_module log("switch");

class Switch
    : public Component 
{
public:
    Switch(const Context* c,
           const json_object*) 
        : Component(c) { }

    void configure(const Configuration*);

    void install();

    Disposition handle(const Event&);

private:
    typedef hash_set<Mac_source, Hash_mac_source> Source_table;
    Source_table sources;

    /* Set up a flow when we know the destination of a packet?  This should
     * ordinarily be true; it is only usefully false for debugging purposes. */
    bool setup_flows;
};

void 
Switch::configure(const Configuration* conf) {
    setup_flows = true; // default value
    BOOST_FOREACH (const std::string& arg, conf->get_arguments()) {
        if (arg == "noflow") {
            setup_flows = false;
        } else {
            VLOG_WARN(log, "argument \"%s\" not supported", arg.c_str());
        }
    }
    
    register_handler<Packet_in_event>
        (boost::bind(&Switch::handle, this, _1));
}

void
Switch::install() {

}

Disposition
Switch::handle(const Event& e)
{
    const Packet_in_event& pi = assert_cast<const Packet_in_event&>(e);
    uint32_t buffer_id = pi.buffer_id;
    Flow flow(pi.in_port, *pi.get_buffer());

    /* drop all LLDP packets */
    if (flow.dl_type == ethernet::LLDP){
        return CONTINUE;
    }

    /* Learn the source. */
    if (!flow.dl_src.is_multicast()) {
        Mac_source src(pi.datapath_id, flow.dl_src);
        Source_table::iterator i = sources.insert(src).first;
        if (i->port != pi.in_port) {
            i->port = pi.in_port;
            VLOG_DBG(log, "learned that "EA_FMT" is on datapath %s port %d",
                     EA_ARGS(&flow.dl_src), pi.datapath_id.string().c_str(),
                     (int) pi.in_port);
        }
    } else {
        VLOG_DBG(log, "multicast packet source "EA_FMT, EA_ARGS(&flow.dl_src));
    }

    /* Figure out the destination. */
    int out_port = -1;        /* Flood by default. */
    if (!flow.dl_dst.is_multicast()) {
        Mac_source dst(pi.datapath_id, flow.dl_dst);
        Source_table::iterator i(sources.find(dst));
        if (i != sources.end()) {
            out_port = i->port;
        }
    }

    /* Set up a flow if the output port is known. */
    if (setup_flows && out_port != -1) {
        ofp_flow_mod* ofm;
        size_t size = sizeof *ofm + sizeof(ofp_action_output);
        boost::shared_array<char> raw_of(new char[size]);
        ofm = (ofp_flow_mod*) raw_of.get();

        ofm->header.version = OFP_VERSION;
        ofm->header.type = OFPT_FLOW_MOD;
        ofm->header.length = htons(size);
        ofm->match.wildcards = htonl(0);
        ofm->match.in_port = htons(flow.in_port);
        ofm->match.dl_vlan = flow.dl_vlan;
        ofm->match.dl_vlan_pcp = flow.dl_vlan_pcp;
        memcpy(ofm->match.dl_src, flow.dl_src.octet, sizeof ofm->match.dl_src);
        memcpy(ofm->match.dl_dst, flow.dl_dst.octet, sizeof ofm->match.dl_dst);
        ofm->match.dl_type = flow.dl_type;
        ofm->match.nw_src = flow.nw_src;
        ofm->match.nw_dst = flow.nw_dst;
        ofm->match.nw_proto = flow.nw_proto;
        ofm->match.nw_tos = flow.nw_tos;
        ofm->match.tp_src = flow.tp_src;
        ofm->match.tp_dst = flow.tp_dst;
        ofm->cookie = htonl(0);
        ofm->command = htons(OFPFC_ADD);
        ofm->buffer_id = htonl(buffer_id);
        ofm->idle_timeout = htons(5);
        ofm->hard_timeout = htons(OFP_FLOW_PERMANENT);
        ofm->priority = htons(OFP_DEFAULT_PRIORITY);
        ofm->flags = htons(ofd_flow_mod_flags());
        ofp_action_output& action = *((ofp_action_output*)ofm->actions);
        memset(&action, 0, sizeof(ofp_action_output));
        action.type = htons(OFPAT_OUTPUT);
        action.len = htons(sizeof(ofp_action_output));
        action.max_len = htons(0);
        action.port = htons(out_port);
        send_openflow_command(pi.datapath_id, &ofm->header, true);
    }

    /* Send out packet if necessary. */
    if (!setup_flows || out_port == -1 || buffer_id == UINT32_MAX) {
        if (buffer_id == UINT32_MAX) {
            if (pi.total_len != pi.get_buffer()->size()) {
                /* Control path didn't buffer the packet and didn't send us
                 * the whole thing--what gives? */
                VLOG_DBG(log, "total_len=%zu data_len=%zu\n",
                         pi.total_len, pi.get_buffer()->size());
                return CONTINUE;
            }
            send_openflow_packet(pi.datapath_id, *pi.get_buffer(),
                                 out_port == -1 ? OFPP_FLOOD : out_port,
                                 pi.in_port, true);
        } else {
            send_openflow_packet(pi.datapath_id, buffer_id,
                                 out_port == -1 ? OFPP_FLOOD : out_port,
                                 pi.in_port, true);
        }
    }
    return CONTINUE;
}

REGISTER_COMPONENT(container::Simple_component_factory<Switch>, Switch);

} // unnamed namespace
