#include "lavi_networkflow.hh"
#include "lavi_swlinks.hh"
#include "assert.hh"

namespace vigil
{
  static Vlog_module lg("lavi_networkflow");
  
  void lavi_networkflow::configure(const Configuration* c) 
  {
    resolve(frr);

    show_ongoing = false;
    flowtype = "network";

    register_handler<JSONMsg_event>
      (boost::bind(&lavi_networkflow::handle_req, this, _1));
    register_handler<Flow_route_event>
      (boost::bind(&lavi_networkflow::handle_flow_route, this, _1));
  }
  
  void lavi_networkflow::send_list(const Msg_stream& stream)
  {
    //Send list of flows
    hash_map<Flow, network::route>::iterator i = frr->routeMap.begin();
    while (i != frr->routeMap.end())
    {
      send_flow_list(i->first, i->second, stream);
      i++;
    }
  }
  
  Disposition lavi_networkflow::handle_flow_route(const Event& e)
  {
    const Flow_route_event& fre = assert_cast<const Flow_route_event&>(e);

    VLOG_ERR(lg, "Got flow %s", fre.flow.to_string().c_str());

    list<Msg_stream*>::iterator i = interested.begin();
    while (i != interested.end())
    {
      if (fre.eventType == Flow_route_event::REMOVE ||
	  fre.eventType == Flow_route_event::MODIFY)
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

  void lavi_networkflow::send_flow_list(const Flow& flow, 
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
      serialize_route(ja, rte);
      jo->object = ja;
      jd->insert(make_pair("flows", jo));
    }

    //Send
    VLOG_DBG(lg, "Sending reply: %s", jm.get_string().c_str());
    stream.send(jm.get_string());
  }
 
  void lavi_networkflow::serialize_route(json_array* ja, 
					 const network::route& rte)
  {
    if (rte.in_switch_port.dpid.empty())
      return;
    
    json_object* jv;
    list<pair< uint16_t, 
      network::hop*> >::const_iterator i = rte.next_hops.begin();

    while (i != rte.next_hops.end())
    {
      jv = new json_object(json_object::JSONT_DICT);
      if (!i->second->in_switch_port.dpid.empty())
      {
	lavi_swlinks::swlink sl("switch",
				rte.in_switch_port.dpid,
				i->first,
				"switch",
				i->second->in_switch_port.dpid,
				i->second->in_switch_port.port);
	sl.get_json(jv);
	ja->push_back(jv);
      }

      serialize_route(ja, *(i->second));
      i++;
    }
  }

  void lavi_networkflow::install()
  {
  }

  void lavi_networkflow::getInstance(const Context* c,
				     lavi_networkflow*& component)
  {
    component = dynamic_cast<lavi_networkflow*>
      (c->get_by_interface(container::Interface_description
			      (typeid(lavi_networkflow).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<lavi_networkflow>,
		     lavi_networkflow);
} // vigil namespace
