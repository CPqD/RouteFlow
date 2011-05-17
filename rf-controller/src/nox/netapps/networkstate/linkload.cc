#include "assert.hh"
#include "linkload.hh"
#include "port-status.hh"
#include "datapath-leave.hh"
#include "openflow-pack.hh"
#include <boost/bind.hpp>

namespace vigil
{
  static Vlog_module lg("linkload");
  
  void linkload::configure(const Configuration* c) 
  {
    //Get commandline arguments
    const hash_map<string, string> argmap = \
      c->get_arguments_list();
    hash_map<string, string>::const_iterator i;
    i = argmap.find("interval");
    if (i != argmap.end())
      load_interval = atoi(i->second.c_str());
    else
      load_interval = LINKLOAD_DEFAULT_INTERVAL;

    resolve(dpmem);

    register_handler<Port_stats_in_event>
      (boost::bind(&linkload::handle_port_stats, this, _1));
    register_handler<Datapath_leave_event>
      (boost::bind(&linkload::handle_dp_leave, this, _1));
    register_handler<Port_status_event>
      (boost::bind(&linkload::handle_port_event, this, _1));
  }
  
  void linkload::install()
  {
    dpi = dpmem->dp_events.begin();

    post(boost::bind(&linkload::stat_probe, this), get_next_time());
  }

  void linkload::stat_probe()
  {
    if (dpmem->dp_events.size() != 0)
    {
      if (dpi == dpmem->dp_events.end())
	dpi = dpmem->dp_events.begin();
      
      VLOG_DBG(lg, "Send probe to %"PRIx64"",
	       dpi->second.datapath_id.as_host());

      send_stat_req(dpi->second.datapath_id);

      dpi++;
    }

    post(boost::bind(&linkload::stat_probe, this), get_next_time());
  }

  float linkload::get_link_load_ratio(datapathid dpid, uint16_t port, bool tx)
  {
    uint32_t speed = dpmem->get_link_speed(dpid, port);
    linkload::load ll  = get_link_load(dpid,port);
    if (speed == 0 || ll.interval == 0)
      return -1;
 
    if (tx)
      return (float) ((double) ll.txLoad*8)/ \
	(((double) dpmem->get_link_speed(dpid, port))*1000*\
	 ((double) ll.interval));
    else
      return (float) ((double) ll.rxLoad*8)/ \
	(((double) dpmem->get_link_speed(dpid, port))*1000*\
	 ((double) ll.interval));
  }

  linkload::load linkload::get_link_load(datapathid dpid, uint16_t port)
  {
    hash_map<switchport, load>::iterator i = \
      loadmap.find(switchport(dpid, port));
    if (i == loadmap.end())
      return load(0,0,0);
    else
      return i->second;
  }

  void linkload::send_stat_req(const datapathid& dpid, uint16_t port)
  {
    size_t size = sizeof(ofp_stats_request)+sizeof(ofp_port_stats_request);
    of_raw.reset(new uint8_t[size]);

    of_stats_request osr(openflow_pack::header(OFPT_STATS_REQUEST, size),
			 OFPST_PORT, 0); 
    of_port_stats_request opsr;
    opsr.port_no = OFPP_NONE;

    osr.pack((ofp_stats_request*) openflow_pack::get_pointer(of_raw));
    opsr.pack((ofp_port_stats_request*) openflow_pack::get_pointer(of_raw, sizeof(ofp_stats_request)));

    send_openflow_command(dpi->second.datapath_id,of_raw, false);
  }

  timeval linkload::get_next_time()
  {
    timeval tv = {0,0};
    if (dpmem->dp_events.size() == 0)
      tv.tv_sec = load_interval;
    else
      tv.tv_sec = load_interval/dpmem->dp_events.size();

    if (tv.tv_sec == 0)
      tv.tv_sec = 1;

    return tv;
  }

  Disposition linkload::handle_port_event(const Event& e)
  {
    const Port_status_event& pse = assert_cast<const Port_status_event&>(e);

    hash_map<switchport, Port_stats>::iterator swstat = \
      statmap.find(switchport(pse.datapath_id, pse.port.port_no));
    if (swstat != statmap.end() &&
	pse.reason == OFPPR_DELETE)
      statmap.erase(swstat);

    hash_map<switchport, load>::iterator swload = \
      loadmap.find(switchport(pse.datapath_id, pse.port.port_no));
    if (swload != loadmap.end() &&
	pse.reason == OFPPR_DELETE)
      loadmap.erase(swload);

    return CONTINUE;
  }

  Disposition linkload::handle_dp_leave(const Event& e)
  {
    const Datapath_leave_event& dle = assert_cast<const Datapath_leave_event&>(e);

    hash_map<switchport, Port_stats>::iterator swstat = statmap.begin();
    while (swstat != statmap.end())
    {
      if (swstat->first.dpid == dle.datapath_id)
	swstat = statmap.erase(swstat);
      else
	swstat++;
    }

    hash_map<switchport, load>::iterator swload = loadmap.begin();
    while (swload != loadmap.end())
    {
      if (swload->first.dpid == dle.datapath_id)
	swload = loadmap.erase(swload);
      else
	swload++;
    }

    return CONTINUE;
  }


  Disposition linkload::handle_port_stats(const Event& e)
  {
    const Port_stats_in_event& psie = assert_cast<const Port_stats_in_event&>(e);

    vector<Port_stats>::const_iterator i = psie.ports.begin();
    while (i != psie.ports.end())
    {
      hash_map<switchport, Port_stats>::iterator swstat = \
	statmap.find(switchport(psie.datapath_id, i->port_no));
      if (swstat != statmap.end())
      {
	updateLoad(psie.datapath_id, i->port_no,
		   swstat->second, *i);
	statmap.erase(swstat);
      }
      statmap.insert(make_pair(switchport(psie.datapath_id, i->port_no),
			       Port_stats(*i)));
      i++;
    }

    return CONTINUE;
  }


  void linkload::updateLoad(const datapathid& dpid, uint16_t port,
			    const Port_stats& oldstat,
			    const Port_stats& newstat)
  {
    hash_map<switchport, load>::iterator i = \
      loadmap.find(switchport(dpid, port));
    
    if (i != loadmap.end())
      loadmap.erase(i);

    loadmap.insert(make_pair(switchport(dpid, port),
			     load(newstat.tx_bytes-oldstat.tx_bytes,
				  newstat.rx_bytes-oldstat.rx_bytes,
				  load_interval)));
  }

  void linkload::getInstance(const Context* c,
				  linkload*& component)
  {
    component = dynamic_cast<linkload*>
      (c->get_by_interface(container::Interface_description
			      (typeid(linkload).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<linkload>,
		     linkload);
} // vigil namespace
