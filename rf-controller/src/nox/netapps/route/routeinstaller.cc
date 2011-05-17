#include "routeinstaller.hh"
#include "openflow-default.hh"
#include "vlog.hh"
#include "assert.hh"
#include <boost/bind.hpp>

using namespace std;

namespace vigil 
{
  using namespace vigil::container;
  static Vlog_module lg("routeinstaller");
  
  void routeinstaller::configure(const Configuration*) 
  {
    resolve(routing);
  }
  
  void routeinstaller::install() 
  {
  }

  bool routeinstaller::get_shortest_path(std::list<network::termination> dst,
					 network::route& route)
  {
    std::list<network::termination>::iterator i = dst.begin();
    if (i == dst.end())
      return false;
 
    if (!get_shortest_path(*i, route))
    {
      return false;
    }

    i++;
    while (i != dst.end())
    {
      network::route r2(route);
      if (!get_shortest_path(*i, r2))
	return false;

      merge_route(&route, &r2);
      i++;
    }

    return true;
  }


  bool routeinstaller::get_shortest_path(network::termination dst,
					 network::route& route)
  {
    Routing_module::RoutePtr sroute;
    Routing_module::RouteId id;
    id.src = route.in_switch_port.dpid;
    id.dst = dst.dpid;

    if (!routing->get_route(id, sroute))
      return false;

    route2tree(dst, sroute, route);
    return true;
  }

  void routeinstaller::route2tree(network::termination dst,
				  Routing_module::RoutePtr sroute,
				  network::route& route)
  {
    route.next_hops.clear();
    network::hop* currHop = &route;
    std::list<Routing_module::Link>::iterator i = sroute->path.begin();
    while (i != sroute->path.end())
    {
      network::hop* nhop = new network::hop(i->dst, i->inport);
      currHop->next_hops.push_front(std::make_pair(i->outport, nhop));
      VLOG_DBG(lg, "Pushing hop from %"PRIx64" from in port %"PRIx16" to out port %"PRIx16"",
      	       i->dst.as_host(), i->inport, i->outport);
      currHop = nhop;
      i++;
    }
    network::hop* nhop = new network::hop(datapathid(), 0);
    currHop->next_hops.push_front(std::make_pair(dst.port, nhop));
    VLOG_DBG(lg, "Pushing last hop to out port %"PRIx16"", dst.port);
    
  }

  void routeinstaller::install_route(const Flow& flow, network::route route, 
				     uint32_t buffer_id, uint32_t wildcards,
				     uint16_t idletime, uint16_t hardtime)
  {
    hash_map<datapathid,ofp_action_list> act;
    list<datapathid> dplist;
    install_route(flow, route, buffer_id, act, dplist, wildcards,
		  idletime, hardtime);
  }

  void routeinstaller::install_route(const Flow& flow, network::route route, 
				     uint32_t buffer_id,
				     hash_map<datapathid,ofp_action_list>& actions,
				     list<datapathid>& skipoutput,
				     uint32_t wildcards,
				     uint16_t idletime, uint16_t hardtime)
  {
    real_install_route(flow, route, buffer_id, actions, skipoutput, wildcards, 
		       idletime, hardtime);  
  }

  void routeinstaller::real_install_route(const Flow& flow, network::route route, 
					  uint32_t buffer_id,
					  hash_map<datapathid,ofp_action_list>& actions,
					  list<datapathid>& skipoutput,
					  uint32_t wildcards,
					  uint16_t idletime, uint16_t hardtime)
  {
    if (route.in_switch_port.dpid.empty())
      return;
    
    //Recursively install 
    network::nextHops::iterator i = route.next_hops.begin();
    while (i != route.next_hops.end())
    {
      if (!(i->second->in_switch_port.dpid.empty()))
	real_install_route(flow, *(i->second), buffer_id, actions, skipoutput,
			   wildcards, idletime, hardtime);
      i++;
    }

    //Check for auxiliary actions
    hash_map<datapathid,ofp_action_list>::iterator j = \
      actions.find(route.in_switch_port.dpid);
    ofp_action_list oalobj;
    ofp_action_list* oal;
    if (j == actions.end())
      oal = &oalobj;
    else
      oal = &(j->second);

    //Add forwarding actions
    list<datapathid>::iterator k = skipoutput.begin();
    while (k != skipoutput.end())
    {
      if (*k == route.in_switch_port.dpid)
	break;
      k++;
    }
    if (k == skipoutput.end())
    {
      i = route.next_hops.begin();
      while (i != route.next_hops.end())
      {
	ofp_action* ofpa = new ofp_action();
	ofpa->set_action_output(i->first, 0);
	oal->action_list.push_back(*ofpa);
	i++;
      }
    }

    //Install flow entry
    install_flow_entry(route.in_switch_port.dpid, flow, buffer_id, 
		       route.in_switch_port.port, *oal,
		       wildcards, idletime, hardtime);
  }
  
  void routeinstaller::install_flow_entry(const datapathid& dpid,
					  const Flow& flow, uint32_t buffer_id, uint16_t in_port,
					  ofp_action_list act_list, uint32_t wildcards_,
					  uint16_t idletime, uint16_t hardtime, uint64_t cookie)
  {
    ssize_t size = sizeof(ofp_flow_mod)+act_list.mem_size();
    of_raw.reset(new uint8_t[size]);
    of_flow_mod ofm;
    ofm.header = openflow_pack::header(OFPT_FLOW_MOD, size);
    ofm.match = flow.get_exact_match();
    ofm.match.in_port = in_port;
    ofm.match.wildcards = wildcards_;
    ofm.cookie = cookie;
    ofm.command = OFPFC_ADD;
    ofm.flags = ofd_flow_mod_flags();
    ofm.idle_timeout = idletime;
    ofm.hard_timeout = hardtime;
    ofm.buffer_id = buffer_id;
    ofm.out_port = OFPP_NONE;
    ofm.pack((ofp_flow_mod*) openflow_pack::get_pointer(of_raw));
    act_list.pack(openflow_pack::get_pointer(of_raw,sizeof(ofp_flow_mod)));
    VLOG_DBG(lg,"Install flow entry %s with %zu actions", 
	     flow.to_string().c_str(), act_list.action_list.size());
    send_openflow_command(dpid, of_raw, false);

    ofm.command = OFPFC_MODIFY_STRICT;
    ofm.pack((ofp_flow_mod*) openflow_pack::get_pointer(of_raw));
    send_openflow_command(dpid, of_raw, false);
  }

  void routeinstaller::merge_route(network::route* tree, network::route* route)
  {
    network::nextHops::iterator routeOut = route->next_hops.begin();
    network::nextHops::iterator i = tree->next_hops.begin();
    while (i != tree->next_hops.end())
    {
      if ((i->first == routeOut->first) &&
	  (i->second->in_switch_port.dpid == 
	   routeOut->second->in_switch_port.dpid))
      {
	merge_route(i->second, routeOut->second);
	return;
      }
      i++;
    }
    
    if (routeOut != route->next_hops.end())
      tree->next_hops.push_front(*routeOut);
  }

  void routeinstaller::getInstance(const container::Context* ctxt, 
				   vigil::routeinstaller*& scpa)
  {
    scpa = dynamic_cast<routeinstaller*>
      (ctxt->get_by_interface(container::Interface_description
			      (typeid(routeinstaller).name())));
  }
  
} // namespace vigil

namespace {
  REGISTER_COMPONENT(vigil::container::Simple_component_factory<vigil::routeinstaller>, 
		     vigil::routeinstaller);
} // unnamed namespace
