#include <net/ethernet.h>

#include "defs.h"
#include "rfofmsg.hh"
#include "OFInterface.hh"

static vigil::Vlog_module lg("rfproxy");

/**
 * Convert the RouteFlow Match to an OpenFlow match
 *
 * Returns -1 on failure.
 */
int add_match(ofp_flow_mod *ofm, const Match& match) {
    int error = 0;

    switch (match.getType()) {
        case RFMT_IPV4: {
            const in_addr* address = static_cast<const in_addr*>
                                     (match.getIPAddress());
            const in_addr* mask = static_cast<const in_addr*>
                                  (match.getIPMask());

            if (address == NULL || mask == NULL) {
                error = -1;
                break;
            }

            uint32_t ofp_mask = ofp_get_mask(*mask, OFPFW_NW_DST_SHIFT);
            ofm_match_dl(ofm, OFPFW_DL_TYPE, ETHERTYPE_IP, 0, 0);
            ofm_match_nw(ofm, ofp_mask, 0, 0, 0, address->s_addr);
            break;
        }
        case RFMT_ETHERNET:
            ofm_match_dl(ofm, OFPFW_DL_DST, 0, 0, match.getValue());
            break;
        case RFMT_ETHERTYPE:
            ofm_match_dl(ofm, OFPFW_DL_TYPE, match.getUint16(), 0, 0);
            break;
        case RFMT_NW_PROTO:
            ofm_match_nw(ofm, OFPFW_NW_PROTO, match.getUint8(), 0, 0, 0);
            break;
        case RFMT_TP_SRC:
            ofm_match_tp(ofm, OFPFW_TP_SRC, match.getUint16(), 0);
            break;
        case RFMT_TP_DST:
            ofm_match_tp(ofm, OFPFW_TP_DST, 0, match.getUint16());
            break;
        case RFMT_IPV6:
        case RFMT_MPLS:
            /* Not implemented in OpenFlow 1.0. */
        default:
            error = -1;
            break;
    }

    return error;
}

/**
 * Convert the RouteFlow Action to an OpenFlow Action
 *
 * Returns -1 on failure.
 */
int add_action(uint8_t *buf, const Action& action) {
    int error = 0;
    ofp_action_header *oah = reinterpret_cast<ofp_action_header*>(buf);

    switch (action.getType()) {
        case RFAT_OUTPUT:
            ofm_set_action(oah, OFPAT_OUTPUT, action.getUint16(), 0);
            break;
        case RFAT_SET_ETH_SRC:
            ofm_set_action(oah, OFPAT_SET_DL_SRC, 0, action.getValue());
            break;
        case RFAT_SET_ETH_DST:
            ofm_set_action(oah, OFPAT_SET_DL_DST, 0, action.getValue());
            break;
        case RFAT_PUSH_MPLS:
        case RFAT_POP_MPLS:
        case RFAT_SWAP_MPLS:
            /* Not implemented in OpenFlow 1.0. */
        default:
            error = -1;
            break;
    }

    return error;
}

/**
 * Convert the RouteFlow Option to an OpenFlow field
 *
 * Returns -1 on failure.
 */
int add_option(ofp_flow_mod *ofm, const Option& option) {
    int error = 0;

    switch (option.getType()) {
        case RFOT_PRIORITY:
            ofm->priority = htons(option.getUint16());
            break;
        case RFOT_IDLE_TIMEOUT:
            ofm->idle_timeout = htons(option.getUint16());
            break;
        case RFOT_HARD_TIMEOUT:
            ofm->hard_timeout = htons(option.getUint16());
            break;
        case RFOT_CT_ID:
            /* Ignore. */
            break;
        default:
            /* Unsupported Option. */
            error = -1;
            break;
    }

    return error;
}

/**
 * Return the size of the OpenFlow struct to hold the given matches.
 */
size_t ofp_match_len(std::vector<Match> matches) {
    /* In OpenFlow 1.0, it's a fixed structure included in ofp_flow_mod */
    return 0;
}

/**
 * Return the size of the OpenFlow struct to hold the given action.
 */
size_t ofp_len(const Action& action) {
    size_t len = 0;

    switch (action.getType()) {
        case RFAT_OUTPUT:
            len = sizeof(struct ofp_action_output);
            break;
        case RFAT_SET_ETH_SRC:
        case RFAT_SET_ETH_DST:
            len = sizeof(struct ofp_action_dl_addr);
            break;
        case RFAT_PUSH_MPLS:
        case RFAT_POP_MPLS:
        case RFAT_SWAP_MPLS:
            /* Not implemented in OpenFlow 1.0. */
        default:
            break;
    }

    return len;
}

/**
 * Return the size of the OpenFlow struct to hold the given actions.
 */
size_t ofp_action_len(std::vector<Action> actions) {
    std::vector<Action>::iterator iter;
    size_t len = 0;

    for (iter = actions.begin(); iter != actions.end(); ++iter) {
        len += ofp_len(*iter);
    }

    return len;
}

void log_error(std::string type, bool optional) {
    if (optional) {
        VLOG_DBG(lg, "Dropping unsupported TLV (type: %s)", type.c_str());
    } else {
        VLOG_ERR(lg, "Failed to serialise TLV (type: %s)", type.c_str());
    }
}

/**
 * Create an OpenFlow FlowMod based on the given matches and actions
 *
 * mod: Specify whether to add/remove/modify a flow (RMT_*)
 *
 * Returns a shared_array pointing to NULL on failure (Unsupported feature)
 */
boost::shared_array<uint8_t> create_flow_mod(uint8_t mod,
            std::vector<Match> matches, std::vector<Action> actions,
            std::vector<Option> options) {
    int error = 0;
    ofp_flow_mod *ofm;
    size_t size = sizeof *ofm + ofp_match_len(matches)
                  + ofp_action_len(actions);

    boost::shared_array<uint8_t> raw_of(new uint8_t[size]);
    ofm = reinterpret_cast<ofp_flow_mod*>(raw_of.get());
    ofm_init(ofm, size);

    uint8_t* oah = reinterpret_cast<uint8_t*>(ofm->actions);

    std::vector<Match>::iterator iter_mat;
    for (iter_mat = matches.begin(); iter_mat != matches.end(); ++iter_mat) {
        if (add_match(ofm, *iter_mat) != 0) {
            log_error(iter_mat->type_to_string(), iter_mat->optional());
            if (!iter_mat->optional()) {
                error = -1;
                break;
            }
        }
    }

    std::vector<Action>::iterator iter_act;
    for (iter_act = actions.begin(); iter_act != actions.end(); ++iter_act) {
        if (add_action(oah, *iter_act) != 0) {
            log_error(iter_act->type_to_string(), iter_act->optional());
            if (!iter_act->optional()) {
                error = -1;
                break;
            }
        }
        oah += ofp_len(*iter_act);
    }

    std::vector<Option>::iterator iter_opt;
    for (iter_opt = options.begin(); iter_opt != options.end(); ++iter_opt) {
        if (add_option(ofm, *iter_opt) != 0) {
            log_error(iter_opt->type_to_string(), iter_opt->optional());
            if (!iter_opt->optional()) {
                error = -1;
                break;
            }
        }
    }

    switch (mod) {
        case RMT_ADD:
            ofm_set_command(ofm, OFPFC_ADD);
            break;
        case RMT_DELETE:
            ofm_set_command(ofm, OFPFC_DELETE_STRICT);
            break;
        default:
            VLOG_ERR(lg, "Unrecognised RouteModType (type: %d)", mod);
            error = -1;
            break;
    }

    if (error != 0) {
        raw_of.reset(NULL);
    }

    return raw_of;
}
