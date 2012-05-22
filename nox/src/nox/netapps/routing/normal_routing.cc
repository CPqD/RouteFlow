/* Copyright 2009 (C) Nicira, Inc.
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
#include "normal_routing.hh"
#include "openflow-default.hh"
#include "openflow-pack.hh"

#include <boost/bind.hpp>
#include <sstream>

#include "assert.hh"
#include "vlog.hh"

#define FLOW_TIMEOUT         5

namespace vigil {
namespace applications {

static Vlog_module lg("normal_routing");

Normal_routing::Normal_routing(const container::Context *c, const json_object* d)
    : container::Component(c)
{
    uint16_t size = sizeof(*ofm) + sizeof(ofp_action_output);
    raw_of.reset(new uint8_t[size]);
    ofm = (ofp_flow_mod*) raw_of.get();
    ofm->header.version = OFP_VERSION;
    ofm->header.type = OFPT_FLOW_MOD;
    ofm->header.length = htons(size);
    ofm->header.xid = openflow_pack::get_xid();
    ofm->match.wildcards = 0;
    ofm->cookie = 0;
    ofm->command = htons(OFPFC_ADD);
    ofm->idle_timeout = htons(FLOW_TIMEOUT);
    ofm->hard_timeout = htons(OFP_FLOW_PERMANENT);
    ofm->priority = htons(OFP_DEFAULT_PRIORITY);
    ofm->out_port = htons(OFPP_NONE);
    ofm->flags = htons(ofd_flow_mod_flags());

    ofp_action_output *action = (ofp_action_output*) (ofm->actions);
    action->type = htons(OFPAT_OUTPUT);
    action->len = htons(sizeof(ofp_action_output));
    action->port = htons(OFPP_NORMAL); // should be normal
    action->max_len = 0;
}

void
Normal_routing::getInstance(const container::Context * ctxt,
                            Normal_routing*& n)
{
    n = dynamic_cast<Normal_routing*>
        (ctxt->get_by_interface(container::Interface_description
                                (typeid(Normal_routing).name())));
}


void
Normal_routing::configure(const container::Configuration*)
{
    register_handler<Flow_in_event>
        (boost::bind(&Normal_routing::handle_flow_in, this, _1));
}

void
Normal_routing::install()
{}

Disposition
Normal_routing::handle_flow_in(const Event& e)
{
    const Flow_in_event& fi = assert_cast<const Flow_in_event&>(e);

    // set up deny flow
    if (fi.active || !fi.fn_applied) {
        ofm->buffer_id = htonl(fi.buffer_id);
        ofp_match& match = ofm->match;
        const Flow& flow = fi.flow;
        match.in_port = flow.in_port;
        memcpy(match.dl_src, flow.dl_src.octet, ethernetaddr::LEN);
        memcpy(match.dl_dst, flow.dl_dst.octet, ethernetaddr::LEN);
        match.dl_vlan = flow.dl_vlan;
        match.dl_vlan_pcp = flow.dl_vlan_pcp;
        match.dl_type = flow.dl_type;
        match.nw_src = flow.nw_src;
        match.nw_dst = flow.nw_dst;
        match.nw_proto = flow.nw_proto;
        match.tp_src = flow.tp_src;
        match.tp_dst = flow.tp_dst;
        ofm->header.length = htons(sizeof(*ofm)
                                   + (fi.active ?
                                      sizeof(ofp_action_output) : 0));
        if (lg.is_dbg_enabled()) {
            std::ostringstream os;
            os << flow;
            VLOG_DBG(lg, "%s %s on dp %"PRIx64".",
                     fi.active ? "allowing" : "denying",
                     os.str().c_str(),
                     fi.datapath_id.as_host());
        }
        int error = send_openflow_command(fi.datapath_id, &ofm->header, false);
        bool buf_sent = false;
        if (!error && fi.buffer_id == UINT32_MAX && fi.active) {
            buf_sent = true;
            error = send_openflow_packet(fi.datapath_id, *fi.buf, ofm->actions,
                                         sizeof(ofp_action_output),
                                         ntohs(flow.in_port), false);
        }
        if (error) {
            if (error == EAGAIN) {
                VLOG_DBG(lg, "Send openflow %s to dp:%"PRIx64" failed with EAGAIN.",
                         buf_sent ? "packet" : "command", fi.datapath_id.as_host());
            } else {
                VLOG_ERR(lg, "Send openflow %s to dp:%"PRIx64" failed with %d:%s.",
                         buf_sent ? "packet" : "command", fi.datapath_id.as_host(),
                         error, strerror(error));
            }
        }

    }

    return CONTINUE;
}

}
}

REGISTER_COMPONENT(vigil::container::Simple_component_factory
                   <vigil::applications::Normal_routing>,
                   vigil::applications::Normal_routing);
