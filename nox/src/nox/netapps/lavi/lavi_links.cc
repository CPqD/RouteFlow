#include "lavi_links.hh"
#include "assert.hh"

namespace vigil
{
  static Vlog_module lg("lavi_links");
  
  lavi_links::link_filter::link_filter(const json_object& jo)
  {
    //Check dictionary
    if (jo.type != json_object::JSONT_DICT)
    {
      node_type = "";
      node_id = "";
      return;
    }

    json_dict* jd = (json_dict*) jo.object;  
    json_dict::iterator i = jd->find("node_type");
    json_dict::iterator j = jd->find("node_id");
    if (i != jd->end() && j != jd->end() &&
	i->second->type == json_object::JSONT_STRING &&
	j->second->type == json_object::JSONT_STRING)
    {
      node_type = *((string *) i->second->object);
      node_id = *((string *) j->second->object);
    }
  }

  bool lavi_links::link_filter::operator==(const link_filter& other) const
  {
    return ((other.node_id == node_id) && (other.node_type == node_type));
  }

  bool lavi_links::link_filter::match(string nodetype, string nodeid) const
  {
    return ((nodetype == node_type) && (nodeid== node_id));
  }

  void lavi_links::configure(const Configuration* c) 
  {
    linktype = "all";

    register_handler<JSONMsg_event>
      (boost::bind(&lavi_links::handle_req, this, _1));
  }
  
  void lavi_links::install()
  {
  }

  void lavi_links::send_list(const Msg_stream& stream, const link_filters& filters)
  {
    VLOG_ERR(lg, "Should not be called!");
  }

  Disposition lavi_links::handle_req(const Event& e)
  {
    const JSONMsg_event& jme = assert_cast<const JSONMsg_event&>(e);
    json_dict::iterator i;
    json_dict::iterator j;
    json_dict::iterator k;
    link_filters filters;

    if (linktype == "all")
      return CONTINUE;

    //Filter all weird messages
    if (jme.jsonobj->type != json_object::JSONT_DICT)
      return STOP;

    json_dict* jodict = (json_dict*) jme.jsonobj->object;
    i = jodict->find("type");
    if (i != jodict->end() &&
	i->second->type == json_object::JSONT_STRING)
    {
      if (*((string *) i->second->object) == "disconnect" )
	unsubscribe(jme.sock, filters);

      if (*((string *) i->second->object) == "lavi" )
      {
	i = jodict->find("command");
	j = jodict->find("link_type");
	if (i != jodict->end() &&
	    j != jodict->end() &&
	    i->second->type == json_object::JSONT_STRING &&
	    j->second->type == json_object::JSONT_STRING &&
	    (*((string *) j->second->object) == "all" ||
	     *((string *) j->second->object) == linktype))
	{
	  //Parse filters
	  k = jodict->find("filter");
	  if (k != jodict->end() &&
	      k->second->type == json_object::JSONT_ARRAY)
	  {
	    json_array* ja = (json_array*) k->second->object;
	    json_array::iterator l = ja->begin();
	    while (l != ja->end())
	    {
	      filters.push_back(*(new link_filter(*(*l))));
	      l++;
	    }
	  }

	  //Process requests
	  if (*((string *) i->second->object) == "request")
	  {
	    VLOG_DBG(lg, "Sending list of links of type %s",
		     ((string *) j->second->object)->c_str());
	    send_list(*jme.sock, filters);
	  }
	  else if (*((string *) i->second->object) == "subscribe" )
	  { 
	    VLOG_DBG(lg, "Adding %p to interested %s list", 
		     jme.sock->stream, linktype.c_str());
	    interested.push_back(make_pair(jme.sock, filters));
	  }
	  else if (*((string *) i->second->object) == "unsubscribe" )
	    unsubscribe(jme.sock, filters);
	}
      }
    }
    
    filters.clear();
    return CONTINUE;
  }

  void lavi_links::unsubscribe(Msg_stream* sock, const link_filters& filters)
  {
    list<pair<Msg_stream*, link_filters> >::iterator k = interested.begin();
    while (k != interested.end())
    {
      if (filters.size() == 0)
      {
	//Remove all associated with socket
	if ((*k).first == sock)
	{
	  VLOG_DBG(lg, "Removing %p from interested %s list", 
		   sock->stream, linktype.c_str());
	  k = interested.erase(k);
	}
	else
	  k++;
      }
      else
      {
	//Remove all associated with socket and filter
	if ((*k).first == sock)
	{
	  VLOG_DBG(lg, "Removing %p from interested %s list", 
		   sock->stream, linktype.c_str());
	  k = interested.erase(k);
	}
	else
	  k++;
      }
    }
  }

  bool lavi_links::compare(const link_filters filter1, link_filters filter2)
  {
    if (filter1.size() != filter2.size())
      return false;

    link_filters::const_iterator i = filter1.begin();
    link_filters::const_iterator j = filter2.begin();
    while (i != filter1.end() && j != filter2.end())
    {
      if (*i == *j)
      {
	i++;
	j++;
      }
      else
	return false;
    }

    return true;
  }

  
  bool lavi_links::match(const link_filters filter, string nodetype1, string nodeid1,
			 string nodetype2, string nodeid2)
  {
    link_filters::const_iterator i = filter.begin();
    while (i != filter.end())
    {
      if (i->match(nodetype1, nodeid1) ||
	  i->match(nodetype2, nodeid2))
	i++;
      else
	return false;
    }

    return true;
  }

  void lavi_links::json_add_host(json_dict* jd, bool src, 
				 const ethernetaddr host)
  {
    char buf[20];
    sprintf(buf, "%"PRIx64"",host.hb_long());
    string host_(buf);
    json_add_host(jd, src, host_);
  }

  void lavi_links::json_add_host(json_dict* jd, bool src, 
				 const string& host)
  {
    json_object* jv;

    jv = new json_object(json_object::JSONT_STRING);
    jv->object = new string("host");
    if (src)
      jd->insert(make_pair("src type", jv));
    else
      jd->insert(make_pair("dst type", jv));

    jv = new json_object(json_object::JSONT_STRING);
    jv->object = new string(host);
    if (src)
      jd->insert(make_pair("src id",jv));
    else
      jd->insert(make_pair("dst id",jv));
  }

  void lavi_links::json_add_switch(json_dict* jd, bool src, 
				   const datapathid dpid, const uint16_t port)
  {
    char buf[20];
    sprintf(buf, "%"PRIx16"",port);
    string dpid_(buf);
    json_add_switch(jd, src, dpid.string(), dpid_);
  }

  void lavi_links::json_add_switch(json_dict* jd, bool src, 
				   const string& dpid, const string& port)
  {
    json_object* jv;

    jv = new json_object(json_object::JSONT_STRING);
    jv->object = new string("switch");
    if (src)
      jd->insert(make_pair("src type", jv));
    else
      jd->insert(make_pair("dst type", jv));

    jv = new json_object(json_object::JSONT_STRING);
    jv->object = new string(dpid);
    if (src)
      jd->insert(make_pair("src id",jv));
    else
      jd->insert(make_pair("dst id",jv));

    jv = new json_object(json_object::JSONT_STRING);
    jv->object = new string(port);
    if (src)
      jd->insert(make_pair("src port",jv));
    else
      jd->insert(make_pair("dst port",jv));
  }

  void lavi_links::getInstance(const Context* c,
				  lavi_links*& component)
  {
    component = dynamic_cast<lavi_links*>
      (c->get_by_interface(container::Interface_description
			      (typeid(lavi_links).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<lavi_links>,
		     lavi_links);
} // vigil namespace
