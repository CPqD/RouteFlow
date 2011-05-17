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
#include <boost/shared_array.hpp>
#include "assert.hh"
#include "component.hh"
#include "flow.hh"
#include "packet-in.hh"
#include "vlog.hh"

#include "netinet++/ethernet.hh"

namespace {

using namespace vigil;
using namespace vigil::container;

Vlog_module lg("hub");

class Hub 
    : public Component 
{
public:
     Hub(const Context* c,
         const json_object*) 
         : Component(c) { }
    
    void configure(const Configuration*) {
    }

    Disposition handler(const Event& e)
    {
        const Packet_in_event& pi = assert_cast<const Packet_in_event&>(e);
        uint32_t buffer_id = pi.buffer_id;
        Flow flow(pi.in_port, *(pi.get_buffer()));

        /* drop all LLDP packets */
        if (flow.dl_type == ethernet::LLDP){
            return CONTINUE;
        }

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
        ofm->match.tp_src = flow.tp_src;
        ofm->match.tp_dst = flow.tp_dst;
        ofm->cookie = htonl(0);
        ofm->command = htons(OFPFC_ADD);
        ofm->buffer_id = htonl(buffer_id);
        ofm->idle_timeout = htons(5);
        ofm->hard_timeout = htons(5);
        ofm->priority = htons(OFP_DEFAULT_PRIORITY);
        ofm->flags = htons(0);
        ofp_action_output& action = *((ofp_action_output*)ofm->actions);
        memset(&action, 0, sizeof(ofp_action_output));
        action.type = htons(OFPAT_OUTPUT);
        action.len = htons(sizeof(ofp_action_output));
        action.port = htons(OFPP_FLOOD);
        action.max_len = htons(0);
        send_openflow_command(pi.datapath_id, &ofm->header, true);
        // free(ofm);

        if (buffer_id == UINT32_MAX) {
            size_t data_len = pi.get_buffer()->size();
            size_t total_len = pi.total_len;
            if (total_len == data_len) {
                send_openflow_packet(pi.datapath_id, *pi.get_buffer(), 
                        OFPP_FLOOD, pi.in_port, true);
            } else {
                /* Control path didn't buffer the packet and didn't send us
                 * the whole thing--what gives? */
                lg.dbg("total_len=%zu data_len=%zu\n", total_len, data_len); 
            }
        }

        return CONTINUE;
    }

    void install()
    {
        register_handler<Packet_in_event>(boost::bind(&Hub::handler, this, _1));
    }
};

REGISTER_COMPONENT(container::Simple_component_factory<Hub>, Hub);

} // unnamed namespace
