#include "simplerouting.hh"
#include "assert.hh"
#include "netinet++/ethernet.hh"
#include <boost/bind.hpp>

namespace vigil
{
  static Vlog_module lg("simplerouting");
  
  void simplerouting::configure(const Configuration* c) 
  {
    resolve(ht);
    resolve(ri);
    resolve(frr);

    post_flow_record = SIMPLEROUTING_POST_RECORD_DEFAULT;

    register_handler<Packet_in_event>
      (boost::bind(&simplerouting::handle_pkt_in, this, _1));

    //Get commandline arguments
    const hash_map<string, string> argmap = \
      c->get_arguments_list();
    hash_map<string, string>::const_iterator i = \
      argmap.find("postrecord");
    if (i != argmap.end())
    {
      if (i->second == "true")
	post_flow_record = true;
      else if (i->second == "false")
	post_flow_record = false;
      else
	VLOG_WARN(lg, "Cannot parse argument postrecord=%s", 
		  i->second.c_str());
    }
  }
  
  Disposition simplerouting::handle_pkt_in(const Event& e)
  {
    const Packet_in_event& pie = assert_cast<const Packet_in_event&>(e);

    //Skip LLDP
    if (pie.flow.dl_type == ethernet::LLDP)
      return CONTINUE;

    const hosttracker::location dloc = ht->get_latest_location(pie.flow.dl_dst);
    bool routed = false;

    //Route or at least try
    if (!dloc.dpid.empty())
    {      
      network::route rte(pie.datapath_id, pie.in_port);
      network::termination endpt(dloc.dpid, dloc.port);
      if (ri->get_shortest_path(endpt, rte))
      {
      	ri->install_route(pie.flow, rte, pie.buffer_id);
	if (post_flow_record)
	  frr->set(pie.flow, rte);
	routed = true;
      }
    }

    //Failed to route, flood
    if (!routed)
    {
      //Flood
      VLOG_DBG(lg, "Sending packet of %s via control",
	       pie.flow.to_string().c_str());
      if (pie.buffer_id == ((uint32_t) -1))
	send_openflow_packet(pie.datapath_id, *(pie.buf),
			     OFPP_FLOOD, pie.in_port, false);
      else
	send_openflow_packet(pie.datapath_id, pie.buffer_id,
			     OFPP_FLOOD, pie.in_port, false);	
    }

    return CONTINUE;
  }

  void simplerouting::install()
  {
  }

  void simplerouting::getInstance(const Context* c,
				  simplerouting*& component)
  {
    component = dynamic_cast<simplerouting*>
      (c->get_by_interface(container::Interface_description
			      (typeid(simplerouting).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<simplerouting>,
		     simplerouting);
} // vigil namespace
