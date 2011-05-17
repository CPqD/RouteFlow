#include "lavi_host2sw.hh"
#include "assert.hh"

namespace vigil
{
  static Vlog_module lg("lavi_host2sw");
  
  void lavi_host2sw::configure(const Configuration* c) 
  {
    linktype = "host2sw";

    resolve(ht);

    register_handler<JSONMsg_event>
      (boost::bind(&lavi_host2sw::handle_req, this, _1));
    register_handler<Host_location_event>
      (boost::bind(&lavi_host2sw::handle_hostloc, this, _1));
  }
  
  void lavi_host2sw::send_list(const Msg_stream& stream, const link_filters& filters)
  {
    const list<ethernetaddr> hosts = ht->get_hosts();
    for (list<ethernetaddr>::const_iterator i = hosts.begin();
	 i != hosts.end(); i++)
    {
      const list<hosttracker::location> hostloc = ht->get_locations(*i);
      for (list<hosttracker::location>::const_iterator j = hostloc.begin();
	   j != hostloc.end(); j++)
	if (match(filters, *i, *j))
	  send_link(stream, *i, *j);   
    }
  }
  
  Disposition lavi_host2sw::handle_hostloc(const Event& e)
  {
    const Host_location_event& hle = assert_cast<const Host_location_event&>(e);

    if (hle.eventType == Host_location_event::MODIFY)
      return CONTINUE;

    list<pair<Msg_stream*, link_filters> >::iterator i = interested.begin();
    while (i != interested.end())
    {
      for (list<hosttracker::location>::const_iterator j = hle.loc.begin();
	   j != hle.loc.end(); j++)
	if (match(i->second, hle.host, *j))
	  send_link(*(i->first), hle.host, *j,
		    (hle.eventType == Host_location_event::ADD));
      i++;
    }

    return CONTINUE;
  }
  
  void lavi_host2sw::send_link(const Msg_stream& stream, 
			       const ethernetaddr host,
			       const hosttracker::location link, 
			       bool add)
  {
    VLOG_DBG(lg, "Sending host to switch link to %p", &stream);

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
    jv = new json_object(json_object::JSONT_DICT);
    get_json(jv, host, link);
    ja->push_back(jv);
    jo->object = ja;
    jd->insert(make_pair("links", jo));
    
    //Send
    stream.send(jm.get_string());
  }
  
  void lavi_host2sw::get_json(json_object* jo, const ethernetaddr host,
			      const hosttracker::location link)
  {
    jo->type = json_object::JSONT_DICT;
    json_dict* jd = new json_dict();
    jo->object = jd;
    
    json_add_host(jd, true, host);
    json_add_switch(jd, false, link.dpid, link.port);
  }			     

  bool lavi_host2sw::match(const link_filters filter,
			   ethernetaddr host,
			   hosttracker::location loc)
  {
    return lavi_links::match(filter,
			     "host", host.string(),
			     "switch", loc.dpid.string());
  }

  void lavi_host2sw::install()
  {
  }

  void lavi_host2sw::getInstance(const Context* c,
				  lavi_host2sw*& component)
  {
    component = dynamic_cast<lavi_host2sw*>
      (c->get_by_interface(container::Interface_description
			      (typeid(lavi_host2sw).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<lavi_host2sw>,
		     lavi_host2sw);
} // vigil namespace
