#include "lavi_hostflow.hh"
#include "assert.hh"

namespace vigil
{
  static Vlog_module lg("lavi_hostflow");
  
  void lavi_hostflow::configure(const Configuration* c) 
  {
    resolve(frr);
    resolve(lh2s);

    show_ongoing = false;
    flowtype = "host";
    send_remove_msg = LAVI_HOSTFLOW_SEND_REMOVE;

    register_handler<JSONMsg_event>
      (boost::bind(&lavi_hostflow::handle_req, this, _1));
    register_handler<Flow_route_event>
      (boost::bind(&lavi_hostflow::handle_flow_route, this, _1));
  }
  
  void lavi_hostflow::send_list(const Msg_stream& stream)
  {
    //Send list of flows
    hash_map<Flow, network::route>::iterator i = frr->routeMap.begin();
    while (i != frr->routeMap.end())
    {
      send_flow_list(i->first, i->second, stream);
      i++;
    }
  }

  Disposition lavi_hostflow::handle_flow_route(const Event& e)
  {
    const Flow_route_event& fre = assert_cast<const Flow_route_event&>(e);

    VLOG_ERR(lg, "Got flow %s", fre.flow.to_string().c_str());

    list<Msg_stream*>::iterator i = interested.begin();
    while (i != interested.end())
    {
      if ((fre.eventType == Flow_route_event::REMOVE ||
	   fre.eventType == Flow_route_event::MODIFY) &&
	  send_remove_msg)
      {
	VLOG_DBG(lg, "Send remove flow %s", fre.flow.to_string().c_str());
	send_flow_list(fre.flow, fre.rte, *(*i), false);
      }
      if (fre.eventType == Flow_route_event::ADD ||
	  fre.eventType == Flow_route_event::MODIFY)
      {
	VLOG_DBG(lg, "Send add flow %s", fre.flow.to_string().c_str());
	send_flow_list(fre.flow, fre.rte, *(*i), true);
      }
      i++;
    }
    
    return CONTINUE;
  }
  
  void lavi_hostflow::send_flow_list(const Flow& flow, 
				     const network::route& rte,
				     const Msg_stream& stream, 
				     bool add)
  {
    json_object jm(json_object::JSONT_DICT);
    json_dict* jd = new json_dict();
    jm.object = jd;
    json_object* jo;
    char buf[20];

    //Add type
    jo = new json_object(json_object::JSONT_STRING);
    jo->object = new string("lavi");
    jd->insert(make_pair("type", jo));

    //Add command
    jo = new json_object(json_object::JSONT_STRING);
    if (add)
      jo->object = new string("add");
    else
      jo->object = new string("delete");      
    jd->insert(make_pair("command", jo));

    //Add flow_type
    jo = new json_object(json_object::JSONT_STRING);
    jo->object = new string(flowtype);
    jd->insert(make_pair("flow_type", jo));

    //Add flow_type
    jo = new json_object(json_object::JSONT_STRING);
    sprintf(buf,"%"PRIx64"", get_id(flow));
    jo->object = new string(buf);
    jd->insert(make_pair("flow_id", jo));

    //Add list of hops/route
    if (add)
    {
      jo = new json_object(json_object::JSONT_ARRAY);
      json_array* ja = new json_array();
      get_host_route(ja, flow, rte);
      jo->object = ja;
      jd->insert(make_pair("flows", jo));
    }

    //Send
    VLOG_DBG(lg, "Sending reply: %s", jm.get_string().c_str());
    stream.send(jm.get_string());
  }

  void lavi_hostflow::get_host_route(json_array* ja, 
				     const Flow& flow,
				     const network::route& rte,
				     bool first)
  {
    if (rte.in_switch_port.dpid.empty())
      return;
    
    json_object* jv;
    list<pair< uint16_t, 
      network::hop*> >::const_iterator i = rte.next_hops.begin();

    if (first)
    {
      jv = new json_object(json_object::JSONT_DICT);
      json_dict* jd = new json_dict();
      jv->object = jd;
      lh2s->json_add_host(jd, true, flow.dl_src);
      lh2s->json_add_switch(jd, false, rte.in_switch_port.dpid,
			    rte.in_switch_port.port);
      ja->push_back(jv);
    }

    while (i != rte.next_hops.end())
    {
      if (i->second->in_switch_port.dpid.empty())
      {
	jv = new json_object(json_object::JSONT_DICT);
	json_dict* jd = new json_dict();
	jv->object = jd;
	lh2s->json_add_host(jd, false, flow.dl_dst);
	lh2s->json_add_switch(jd, true, rte.in_switch_port.dpid,
			      rte.in_switch_port.port);
	ja->push_back(jv);
      }
      else
	get_host_route(ja, flow, *(i->second), false);
      i++;
    } 
  }

  void lavi_hostflow::install()
  {
  }

  void lavi_hostflow::getInstance(const Context* c,
				  lavi_hostflow*& component)
  {
    component = dynamic_cast<lavi_hostflow*>
      (c->get_by_interface(container::Interface_description
			      (typeid(lavi_hostflow).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<lavi_hostflow>,
		     lavi_hostflow);
} // vigil namespace
