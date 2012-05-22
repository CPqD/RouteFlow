#include "hostip.hh"
#include "assert.hh"
#include "flow.hh"
#include "packet-in.hh"
#include "netinet++/ethernet.hh"
#include "netinet++/ipaddr.hh"
#include <boost/bind.hpp>

namespace vigil
{
  static Vlog_module lg("hostip");
  
  void hostip::configure(const Configuration* c) 
  {
    register_handler<Packet_in_event>
      (boost::bind(&hostip::handle_pkt_in, this, _1));
  }
  
  void hostip::install()
  {
  }

  ethernetaddr hostip::get_mac(uint32_t ip)
  {
    hash_map<uint32_t, ethernetaddr>::iterator i = ip_map.find(ip);
    if (i == ip_map.end())
      return ethernetaddr();
    else
      return ethernetaddr(i->second);
  }

  Disposition hostip::handle_pkt_in(const Event& e)
  {
    const Packet_in_event& pie = assert_cast<const Packet_in_event&>(e);
 
    Flow flow(htons(pie.in_port), *(pie.buf));
    if (flow.dl_type == ethernet::IP &&
	!flow.dl_src.is_multicast() && !flow.dl_src.is_broadcast() &&
	!ipaddr(flow.nw_src).isMulticast())
    {
      //Add map
      hash_map<uint32_t,ethernetaddr>::iterator i = \
	ip_map.find(flow.nw_src);
      if (i == ip_map.end())
	ip_map.insert(make_pair(flow.nw_src, flow.dl_src));
      else
	i->second=flow.dl_src;
    }

    return CONTINUE;
  }

  void hostip::getInstance(const Context* c,
				  hostip*& component)
  {
    component = dynamic_cast<hostip*>
      (c->get_by_interface(container::Interface_description
			      (typeid(hostip).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<hostip>,
		     hostip);
} // vigil namespace
