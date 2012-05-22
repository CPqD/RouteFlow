#include "queue-stats-in.hh"

namespace vigil
{
  Queue_stats::Queue_stats(const ofp_queue_stats* oqs)
  {
    port_no = ntohs(oqs->port_no);
    queue_id = ntohl(oqs->queue_id);
    tx_bytes = ntohll(oqs->tx_bytes);
    tx_packets = ntohll(oqs->tx_packets);
    tx_errors = ntohll(oqs->tx_errors);
  }

  Queue_stats_in_event::Queue_stats_in_event(const datapathid& dpid,
					     const ofp_stats_reply *osr,
					     std::auto_ptr<Buffer> buf)
    : Event(static_get_name()),
      Ofp_msg_event(&osr->header, buf)
  {
    datapath_id = dpid;
    
    size_t queue_len = (htons(osr->header.length) - sizeof *osr)/
      sizeof(ofp_queue_stats);
    const ofp_queue_stats* oqs = (ofp_queue_stats*) osr->body;
    for (int i = 0; i < queue_len; i++)
    {
      queues.push_back(Queue_stats(oqs));
      oqs++;
    } 
  }

}
