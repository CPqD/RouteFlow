#include <stdio.h>
#include <arpa/inet.h>
#include <iostream>
#include <boost/shared_array.hpp>
#include <cstring>
#include <linux/if_ether.h>

#include "openflow.h"
#include "rfofmsg.h"
#include "defs.h"
#include "MACAddress.h"

// TODO: this code is almost pure C, apart from the use of Boost and a few language constructs. Make it pure C.

MSG msg_new(uint8_t* src, size_t size) {
	MSG msg = (uint8_t*) malloc(size);
	memcpy(msg, src, size);
	return msg;
}

size_t msg_size(MSG msg) {
    return ntohs(((uint16_t*) msg)[1]);
}

void msg_save(MSG msg, const char* fname) {
    FILE *f = fopen(fname, "w");
    fwrite(msg, sizeof(uint8_t), msg_size(msg), f);
    fclose(f);
}

void msg_delete(MSG msg) {
    free(msg);
}

void ofm_init(ofp_flow_mod* ofm, size_t size) {
	/* Open Flow Header. */
	ofm->header.version = OFP_VERSION;
	ofm->header.type = OFPT_FLOW_MOD;
	ofm->header.length = htons(size);
	ofm->header.xid = 0;

	std::memset(&(ofm->match), 0, sizeof(struct ofp_match));
	ofm->match.wildcards = htonl(OFPFW_ALL);

	ofm->cookie = htonl(0);
	ofm->priority = htons(OFP_DEFAULT_PRIORITY);
	ofm->flags = htons(0);
}

void ofm_match_in(ofp_flow_mod* ofm, uint16_t in) {

	ofm->match.wildcards &= htonl(~OFPFW_IN_PORT);
	ofm->match.in_port = htons(in);
}

void ofm_match_dl(ofp_flow_mod* ofm, uint32_t match, uint16_t type, const uint8_t src[], const uint8_t dst[]) {

    ofm->match.wildcards &= htonl(~match);

    if (match & OFPFW_DL_TYPE) { /* Ethernet frame type. */
        ofm->match.dl_type = htons(type);
    }
    if (match & OFPFW_DL_SRC) { /* Ethernet source address. */
        std::memcpy(ofm->match.dl_src, src, OFP_ETH_ALEN);
    }
    if (match & OFPFW_DL_DST) { /* Ethernet destination address. */
        std::memcpy(ofm->match.dl_dst, dst, OFP_ETH_ALEN);
    }
}

void ofm_match_vlan(ofp_flow_mod* ofm, uint32_t match, uint16_t id, uint8_t priority) {
    ofm->match.wildcards &= htonl(~match);

    if (match & OFPFW_DL_VLAN) { /* VLAN id. */
        ofm->match.dl_vlan = htons(id);
    }
    if (match & OFPFW_DL_VLAN_PCP) { /* VLAN priority. */
        ofm->match.dl_vlan_pcp = priority;
    }
}

void ofm_match_nw(ofp_flow_mod* ofm, uint32_t match, uint8_t proto, uint8_t tos, uint32_t src, uint32_t dst) {
    ofm->match.wildcards &= htonl(~match);

    if (match & OFPFW_NW_PROTO) { /* IP protocol. */
        ofm->match.nw_proto = proto;
    }
    if (match & OFPFW_NW_TOS) { /* IP ToS (DSCP field, 6 bits). */
        ofm->match.nw_tos = tos;
    }
    if ((match & OFPFW_NW_SRC_MASK) > 0) { /* IP source address. */
        ofm->match.nw_src = src;
    }
    if ((match & OFPFW_NW_DST_MASK) > 0) { /* IP destination address. */
        ofm->match.nw_dst = dst;
    }
}

void ofm_match_tp(ofp_flow_mod* ofm, uint32_t match, uint16_t src, uint16_t dst) {
    ofm->match.wildcards &= htonl(~match);

    if (match & OFPFW_TP_SRC) { /* TCP/UDP source port. */
        ofm->match.tp_src = htons(src);
    }
    if (match & OFPFW_TP_DST) { /* TCP/UDP destination port. */
        ofm->match.tp_dst = htons(dst);
    }
}

void ofm_set_action(ofp_action_header* hdr, uint16_t type, uint16_t len, uint16_t port, uint16_t max_len, const uint8_t addr[]) {
    std::memset((uint8_t *)hdr, 0, len);
    hdr->type = htons(type);
    hdr->len = htons(len);

    if (type == OFPAT_OUTPUT) {
        ofp_action_output* action = (ofp_action_output*)hdr;
        action->port = htons(port);
        action->max_len = htons(max_len);
    } else if (type == OFPAT_SET_DL_SRC || type == OFPAT_SET_DL_DST) {
        ofp_action_dl_addr* action = (ofp_action_dl_addr*)hdr;
        std::memcpy(&action->dl_addr, addr, OFP_ETH_ALEN);
    }
}

void ofm_set_command(ofp_flow_mod* ofm, uint16_t cmd, uint32_t id, uint16_t idle_to, uint16_t hard_to, uint16_t port) {
    ofm->command = htons(cmd);
    ofm->buffer_id = htonl(id);
    ofm->idle_timeout = htons(idle_to);
    ofm->hard_timeout = htons(hard_to);
    ofm->out_port = htons(port);
}

MSG create_config_msg(DATAPATH_CONFIG_OPERATION operation) {
	ofp_flow_mod* ofm;
	size_t size = sizeof *ofm;

	if (operation != DC_CLEAR_FLOW_TABLE && operation != DC_DROP_ALL) {
		size = sizeof *ofm + sizeof(ofp_action_output);
	}

	boost::shared_array<char> raw_of(new char[size]);
	ofm = (ofp_flow_mod*) raw_of.get();

	ofm_init(ofm, size);

	if (operation == DC_RIPV2) {
		ofm_match_dl(ofm, OFPFW_DL_TYPE, 0x0800, 0, 0);
		ofm_match_nw(ofm, (OFPFW_NW_PROTO | OFPFW_NW_DST_MASK), 0x11, 0, 0, inet_addr("224.0.0.9"));
	} else if (operation == DC_OSPF) {
		ofm_match_dl(ofm, OFPFW_DL_TYPE, 0x0800, 0, 0);
		ofm_match_nw(ofm, OFPFW_NW_PROTO, 0x59, 0, 0, 0);
	} else if (operation == DC_ARP) {
		ofm_match_dl(ofm, OFPFW_DL_TYPE, 0x0806, 0, 0);
	} else if (operation == DC_ICMP) {
		ofm_match_dl(ofm, OFPFW_DL_TYPE, 0x0800, 0, 0);
		ofm_match_nw(ofm, OFPFW_NW_PROTO, 0x01, 0, 0, 0);
	} else if (operation == DC_BGP) {
		ofm_match_dl(ofm, OFPFW_DL_TYPE, 0x0800, 0, 0);
		ofm_match_nw(ofm, OFPFW_NW_PROTO, 0x06, 0, 0, 0);
		ofm_match_tp(ofm, OFPFW_TP_DST, 0, 0x00B3);
	} else if (operation == DC_VM_INFO) {
		ofm_match_dl(ofm, OFPFW_DL_TYPE, 0x0A0A, 0, 0);
	} else if (operation == DC_DROP_ALL) {
		ofm->priority = htons(1);
	}

	if (operation == DC_CLEAR_FLOW_TABLE) {
		ofm_set_command(ofm, OFPFC_DELETE, 0, 0, 0, OFPP_NONE);
		ofm->priority = htons(0);
	} else {
		ofm_set_command(ofm, OFPFC_ADD, UINT32_MAX, OFP_FLOW_PERMANENT, OFP_FLOW_PERMANENT, OFPP_NONE);
		ofm_set_action(ofm->actions, OFPAT_OUTPUT, sizeof(ofp_action_output), OFPP_CONTROLLER, ETH_DATA_LEN, 0);
	}
    
    return msg_new((uint8_t*) &ofm->header, size);
}

MSG create_flow_install_msg(uint32_t ip, uint32_t mask, uint8_t srcMac[], uint8_t dstMac[], uint32_t dstPort) {
    ip = htonl(ip);
    ofp_flow_mod* ofm;
    size_t size = sizeof *ofm + (2 * sizeof(ofp_action_dl_addr)) + sizeof(ofp_action_output);
    boost::shared_array<char> raw_of(new char[size]);
    ofm = (ofp_flow_mod*) raw_of.get();

    ofm_init(ofm, size);

    ofm_match_dl(ofm, OFPFW_DL_TYPE , 0x0800, 0, 0);
    if (MATCH_L2)
	    ofm_match_dl(ofm, OFPFW_DL_DST, 0, 0, srcMac);
    ofm_match_nw(ofm, (((uint32_t) 31 + mask) << OFPFW_NW_DST_SHIFT), 0, 0, 0, ip);

    ofm_set_command(ofm, OFPFC_ADD, UINT32_MAX, OFP_FLOW_PERMANENT, OFP_FLOW_PERMANENT, OFPP_NONE);

    if (mask == 32) {
	    ofm->idle_timeout = htons(300);
    }

    ofm->priority = htons((OFP_DEFAULT_PRIORITY + mask));

    uint8_t *pActions = (uint8_t *) ofm->actions;

    /*Action: change the dst MAC address.*/
    ofm_set_action((ofp_action_header*)pActions, OFPAT_SET_DL_DST, sizeof(ofp_action_dl_addr), 0, 0, dstMac);
    pActions += sizeof(ofp_action_dl_addr);

    /*Action: change the src MAC address.*/
    ofm_set_action((ofp_action_header*)pActions, OFPAT_SET_DL_SRC, sizeof(ofp_action_dl_addr), 0, 0, srcMac);
    pActions += sizeof(ofp_action_dl_addr);

    /*Action: forward to port dstPort. */
    ofm_set_action((ofp_action_header*) pActions, OFPAT_OUTPUT, sizeof(ofp_action_output), dstPort, 0, 0);

    return msg_new((uint8_t*) &ofm->header, size);
}

MSG create_flow_remove_msg(uint32_t ip, uint32_t mask, uint8_t srcMac[]) {
    ip = htonl(ip);
    
    ofp_flow_mod* ofm;
    size_t size = sizeof *ofm + sizeof(ofp_action_output);
    boost::shared_array<char> raw_of(new char[size]);
    ofm = (ofp_flow_mod*) raw_of.get();

    ofm_init(ofm, size);

    ofm_match_dl(ofm, OFPFW_DL_TYPE , 0x0800, 0, 0);
    if (MATCH_L2)
	    ofm_match_dl(ofm, OFPFW_DL_DST, 0, 0, srcMac);

    ofm_match_nw(ofm, (((uint32_t) 31 + mask) << OFPFW_NW_DST_SHIFT), 0, 0, 0, ip);

    ofm->priority = htons((OFP_DEFAULT_PRIORITY + mask));
    ofm_set_command(ofm, OFPFC_MODIFY_STRICT, UINT32_MAX, 60, OFP_FLOW_PERMANENT, OFPP_NONE);
    ofm_set_action(ofm->actions, OFPAT_OUTPUT, sizeof(ofp_action_output), 0, 0, 0);

    return msg_new((uint8_t*) &ofm->header, size);
}

/*
int main() {
    // Garbage just to look pretty in the tests
    uint8_t a[6];
    uint8_t b[6];
    
    MSG msg = create_flow_install_msg(0xEEAABBCC, 24, a, b, 123);
    msg_save(msg, "install");
    
    msg = create_config_msg(DC_RIPV2);
    msg_save(msg, "config");
}
*/
