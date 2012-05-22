#include "trackhost_pktin.hh"
#include "assert.hh"
#include "packet-in.hh"
#include "netinet++/ethernet.hh"
#include "flow.hh"
#include <boost/bind.hpp>

namespace vigil
{
  static Vlog_module lg("trackhost_pktin");
  
  Disposition trackhost_pktin::handle_pkt_in(const Event& e)
  {
    const Packet_in_event& pie = assert_cast<const Packet_in_event&>(e);
 
    if (pie.flow.dl_type == ethernet::LLDP)
      return CONTINUE;

    if (!topo->is_internal(pie.datapath_id, pie.in_port) &&
	!pie.flow.dl_src.is_multicast() && !pie.flow.dl_src.is_broadcast())
      ht->add_location(pie.flow.dl_src, pie.datapath_id, pie.in_port);
    else
      VLOG_DBG(lg, "Host %"PRIx64" not registered, 'cos %s%s %s",
	       pie.flow.dl_src.hb_long(),
	       pie.flow.dl_src.is_multicast()?"multicast mac":"",
	       pie.flow.dl_src.is_broadcast()?"broadcast mac":"",
	       topo->is_internal(pie.datapath_id, pie.in_port)?"on internal port":"");

    return CONTINUE;
  } 

  void trackhost_pktin::configure(const Configuration* c) 
  {
    resolve(ht);
    resolve(topo);

    register_handler<Packet_in_event>
      (boost::bind(&trackhost_pktin::handle_pkt_in, this, _1));
  }
  
  void trackhost_pktin::install()
  {
  }

  void trackhost_pktin::getInstance(const Context* c,
				  trackhost_pktin*& component)
  {
    component = dynamic_cast<trackhost_pktin*>
      (c->get_by_interface(container::Interface_description
			      (typeid(trackhost_pktin).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<trackhost_pktin>,
		     trackhost_pktin);
} // vigil namespace
