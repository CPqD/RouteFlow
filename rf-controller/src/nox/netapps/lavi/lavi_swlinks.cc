#include "lavi_swlinks.hh"
#include "assert.hh"
#include "discovery/link-event.hh"

namespace vigil
{
  static Vlog_module lg("lavi_swlinks");
  
  lavi_swlinks::swlink::swlink(string nt1, const datapathid  ni1, uint16_t np1,
			       string nt2, const datapathid  ni2, uint16_t np2)
  {
    char buf[10];

    src_nodetype = nt1;
    dst_nodetype = nt2;
    src_nodeid = ni1.string();
    dst_nodeid = ni2.string();
    sprintf(buf, "%"PRIx16"", np1);
    src_port = buf;
    sprintf(buf, "%"PRIx16"", np2);
    dst_port = buf;
  }

  void lavi_swlinks::swlink::get_json(json_object* jo)
  {
    jo->type = json_object::JSONT_DICT;
    json_dict* jd = new json_dict();
    jo->object = jd;

    json_add_switch(jd, true, src_nodeid, src_port);
    json_add_switch(jd, false, dst_nodeid, dst_port);
  }

  void lavi_swlinks::configure(const Configuration* c) 
  {
    linktype = "sw2sw";

    resolve(dpm);
    resolve(topo);

    register_handler<JSONMsg_event>
      (boost::bind(&lavi_swlinks::handle_req, this, _1));
    register_handler<Link_event>
      (boost::bind(&lavi_swlinks::handle_link, this, _1));
  }
  
  void lavi_swlinks::install()
  {
  }

  Disposition lavi_swlinks::handle_link(const Event& e)
  {
    const Link_event& le = assert_cast<const Link_event&>(e);

    swlink link("switch",le.dpsrc,le.sport,"switch",le.dpdst,le.dport);
    list<swlink> linklist;
    linklist.push_back(link);

    list<pair<Msg_stream*, link_filters> >::iterator i = interested.begin();
    while (i != interested.end())
    {
      if (match(i->second, link))
	send_linklist(*(i->first), linklist, le.action == Link_event::ADD);
      i++;
    }

    return CONTINUE;
  }

  void lavi_swlinks::send_list(const Msg_stream& stream, const link_filters& filters)
  {
    swlink* link;
    list<swlink> linklist;

    hash_map<uint64_t, Datapath_join_event>::const_iterator i = dpm->dp_events.begin();
    //Loop through switches
    while (i != dpm->dp_events.end())
    {
      const Topology::DatapathLinkMap dlm = topo->get_outlinks(i->second.datapath_id);
      Topology::DatapathLinkMap::const_iterator j = dlm.begin();
      //Loop through switch on the other end
      while (j != dlm.end())
      {
	Topology::LinkSet::const_iterator k = j->second.begin();
	//Loop through links between switches
	while( k != j->second.end())
	{
	  link = new swlink("switch", i->second.datapath_id, k->src, 
			    "switch", j->first, k->dst);
	  if (match(filters, *link))
	    linklist.push_back(*link);
	  k++;
	}
	j++;
      }
      i++;
    }

    send_linklist(stream, linklist);
  }

  void lavi_swlinks::send_linklist(const Msg_stream& stream, 
				   list<swlink> link_list,
				   bool add)
  {
    VLOG_DBG(lg, "Sending link list of %zu to %p", link_list.size(), &stream);

    json_object jm(json_object::JSONT_DICT);
    json_dict* jd = new json_dict();
    jm.object = jd;
    json_object* jo;
    json_object* jv;

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

    //Add node_type
    jo = new json_object(json_object::JSONT_STRING);
    jo->object = new string(linktype);
    jd->insert(make_pair("link_type", jo));

    //Add list of links
    jo = new json_object(json_object::JSONT_ARRAY);
    json_array* ja = new json_array();
    for(list<swlink>::iterator i = link_list.begin();
	i != link_list.end(); i++)
    {
      jv = new json_object(json_object::JSONT_DICT);
      i->get_json(jv);
      ja->push_back(jv);
    }
    jo->object = ja;
    jd->insert(make_pair("links", jo));
    
    //Send
    stream.send(jm.get_string());
  }

  bool lavi_swlinks::match(const link_filters filter, swlink link)
  {
    return lavi_links::match(filter, 
			     link.src_nodetype, link.src_nodeid,
			     link.dst_nodetype, link.dst_nodeid);
  }

  void lavi_swlinks::getInstance(const Context* c,
				  lavi_swlinks*& component)
  {
    component = dynamic_cast<lavi_swlinks*>
      (c->get_by_interface(container::Interface_description
			      (typeid(lavi_swlinks).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<lavi_swlinks>,
		     lavi_swlinks);
} // vigil namespace
