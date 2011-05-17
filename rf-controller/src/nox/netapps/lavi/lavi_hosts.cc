#include "lavi_hosts.hh"
#include "assert.hh"

namespace vigil
{
  static Vlog_module lg("lavi_hosts");
  
  void lavi_hosts::configure(const Configuration* c) 
  {
    nodetype = "host";

    resolve(ht);

    register_handler<JSONMsg_event>
      (boost::bind(&lavi_hosts::handle_req, this, _1));
    register_handler<Host_location_event>
       (boost::bind(&lavi_hosts::handle_host_loc, this, _1));
  }
  
  void lavi_hosts::install()
  {
  }
  
  Disposition lavi_hosts::handle_host_loc(const Event& e)
  {
    const Host_location_event& hle = assert_cast<const Host_location_event&>(e);

    VLOG_DBG(lg, "host");
    
    if (hle.eventType == Host_location_event::MODIFY)
      return CONTINUE;

    VLOG_DBG(lg, "Host %"PRIx64" %s",
	     hle.host.hb_long(), 
	     (hle.eventType == Host_location_event::ADD)? "added":"removed");

    list<ethernetaddr> hlist;
    hlist.push_back(hle.host);

    list<Msg_stream*>::iterator i = interested.begin();
    while(i != interested.end())
    {
      send_hostlist(*(*i), hlist, 
		    (hle.eventType == Host_location_event::ADD));
      i++;
    }
    
    return CONTINUE;
  }
  
  void lavi_hosts::send_list(const Msg_stream& stream)
  {
    send_hostlist(stream, ht->get_hosts());
  }

  void lavi_hosts::send_hostlist(const Msg_stream& stream, 
				 const list<ethernetaddr> host_list,
				 bool add)
  {
    VLOG_DBG(lg, "Sending host list of %zu to %p", host_list.size(), &stream);

    json_object jm(json_object::JSONT_DICT);
    json_dict* jd = new json_dict();
    jm.object = jd;
    json_object* jo;
    json_object* jv;
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

    //Add node_type
    jo = new json_object(json_object::JSONT_STRING);
    jo->object = new string(nodetype);
    jd->insert(make_pair("node_type", jo));

    //Add string of host mac
    jo = new json_object(json_object::JSONT_ARRAY);
    json_array* ja = new json_array();
    for (list<ethernetaddr>::const_iterator i = host_list.begin();
	 i != host_list.end(); i++)
    {
      jv = new json_object(json_object::JSONT_STRING);
      sprintf(buf,"%"PRIx64"", i->hb_long());
      jv->object = new string(buf);
      ja->push_back(jv);
    }
    jo->object = ja;
    jd->insert(make_pair("node_id", jo));

    //Send
    stream.send(jm.get_string());
  }

  void lavi_hosts::getInstance(const Context* c,
				  lavi_hosts*& component)
  {
    component = dynamic_cast<lavi_hosts*>
      (c->get_by_interface(container::Interface_description
			      (typeid(lavi_hosts).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<lavi_hosts>,
		     lavi_hosts);
} // vigil namespace
