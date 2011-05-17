#include "queue-config-in.hh"

namespace vigil
{
  void Packet_queue_property::set_queue_property_header(ofp_queue_prop_header* obj,
							const ofp_queue_prop_header* oqph)
  {
    obj->property = ntohs(oqph->property);
    obj->len = ntohs(oqph->len);
  }

  Packet_queue_min_rate_property::Packet_queue_min_rate_property(const ofp_queue_prop_min_rate* oqpmr)
  {
    set_queue_property_header(&prop_header,
			      (const ofp_queue_prop_header*) oqpmr);
    rate = ntohs(oqpmr->rate);
  }    

  Packet_queue::Packet_queue(const ofp_packet_queue* opq)
  {
    queue_id = ntohll(opq->queue_id);
    len = ntohs(opq->len);
    //Parse each property
    size_t len = ntohs(opq->len) - sizeof *opq;
    const ofp_queue_prop_header* oqph = (ofp_queue_prop_header*) opq->properties;
    while (len >- sizeof *oqph)
    {
      size_t length = ntohs(oqph->len);
      if (length > len)
	break;
      switch (ntohs(oqph->property))
      {
      case OFPQT_NONE:
	break;
      case OFPQT_MIN_RATE:
	properties.push_back(Packet_queue_min_rate_property((const ofp_queue_prop_min_rate*) oqph));
	break;
      }

      oqph = (const ofp_queue_prop_header*)((const char*) oqph + length);
      len -= length;
    }
  }

  Queue_config_in_event::Queue_config_in_event(const datapathid& dpid, 
					       const ofp_queue_get_config_reply *oqgcr,
					       std::auto_ptr<Buffer> buf)
    :Event(static_get_name()),
     Ofp_msg_event(&oqgcr->header, buf)
  {
    datapath_id = dpid;
    port = ntohs(oqgcr->port);
    //Parse each queue
    //std::vector<Packet_queue> queues;
    size_t len = ntohs(oqgcr->header.length) - sizeof *oqgcr;
    const ofp_packet_queue* opq = (ofp_packet_queue*) oqgcr->queues;
    while (len >= sizeof *opq)
    {
      size_t length = ntohs(opq->len);
      if (length > len)
	break;
      queues.push_back(Packet_queue(opq));
      opq = (const ofp_packet_queue*)((const char*) opq + length);
      len -= length;
    }
  }
  
}
