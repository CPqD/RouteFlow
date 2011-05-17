#include "flow-stats-in.hh"

namespace vigil {

Flow_stats::Flow_stats(const ofp_flow_stats* ofs)
{
    *(ofp_flow_stats*) this = *ofs;
    const ofp_action_header* headers = ofs->actions;
    size_t n_actions = (ntohs(ofs->length) - sizeof *ofs) / sizeof *headers;
    v_actions.assign(headers, headers + n_actions);
}

Flow_stats_in_event::Flow_stats_in_event(const datapathid& dpid,
                                         const ofp_stats_reply *osr,
                                         std::auto_ptr<Buffer> buf)
    : Event(static_get_name()),
      Ofp_msg_event(&osr->header, buf),
      more((osr->flags & htons(OFPSF_REPLY_MORE)) != 0)
{
    datapath_id  = dpid;

    size_t flow_len = htons(osr->header.length) - sizeof *osr;
    const ofp_flow_stats* ofs = (ofp_flow_stats*) osr->body;
    while (flow_len >= sizeof *ofs) {
        size_t length = ntohs(ofs->length);
        if (length > flow_len) {
            break;
        }
        flows.push_back(Flow_stats(ofs));
        ofs = (const ofp_flow_stats*)((const char*) ofs + length);
        flow_len -= length;
    }
}

} // namespace vigil
