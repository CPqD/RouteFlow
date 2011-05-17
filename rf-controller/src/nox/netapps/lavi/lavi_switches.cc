#include "lavi_switches.hh"
#include "assert.hh"
#include "datapath-leave.hh"

namespace vigil
{
  static Vlog_module lg("lavi_switches");
  
  void lavi_switches::configure(const Configuration* c) 
  {
    nodetype = "switch";

    resolve(dpm);

    register_handler<JSONMsg_event>
      (boost::bind(&lavi_switches::handle_req, this, _1));
    register_handler<Datapath_join_event>
      (boost::bind(&lavi_switches::handle_switch_add, this, _1));
    register_handler<Datapath_leave_event>
      (boost::bind(&lavi_switches::handle_switch_del, this, _1));
  }
  
  void lavi_switches::install()
  {
  }

  Disposition lavi_switches::handle_switch_add(const Event& e)
  {
    const Datapath_join_event& dje = assert_cast<const Datapath_join_event&>(e);

    list<uint64_t> dpidlist;
    dpidlist.push_back(dje.datapath_id.as_host());
    
    list<Msg_stream*>::iterator i = interested.begin();
    while(i != interested.end())
    {
      send_swlist(*(*i), dpidlist);
      i++;
    }

    return CONTINUE;
  }

  Disposition lavi_switches::handle_switch_del(const Event& e)
  {
    const Datapath_leave_event& dle = assert_cast<const Datapath_leave_event&>(e);

    list<uint64_t> dpidlist;
    dpidlist.push_back(dle.datapath_id.as_host());
    
    list<Msg_stream*>::iterator i = interested.begin();
    while(i != interested.end())
    {
      send_swlist(*(*i), dpidlist, false);
      i++;
    }

    return CONTINUE;
  }

  void lavi_switches::send_list(const Msg_stream& stream)
  {
    VLOG_DBG(lg, "Sending list of switches to %zu clients", interested.size());
    
    //Build list
    list<uint64_t> dpidlist;
    hash_map<uint64_t,Datapath_join_event>::iterator i = dpm->dp_events.begin();
    while (i != dpm->dp_events.end())
    {
      dpidlist.push_back(i->second.datapath_id.as_host());
      i++;
    }
    
    send_swlist(stream, dpidlist);
  }

  void lavi_switches::send_swlist(const Msg_stream& stream, list<uint64_t> dpid_list,
				  bool add)
  {
    VLOG_DBG(lg, "Sending switch list of %zu to %p", dpid_list.size(), &stream);

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

    //Add string of datapathid
    jo = new json_object(json_object::JSONT_ARRAY);
    json_array* ja = new json_array();
    for (list<uint64_t>::iterator i = dpid_list.begin();
	 i != dpid_list.end(); i++)
    {
      jv = new json_object(json_object::JSONT_STRING);
      sprintf(buf,"%"PRIx64"", *i);
      jv->object = new string(buf);
      ja->push_back(jv);
    }
    jo->object = ja;
    VLOG_DBG(lg, "Size %zu: %s",  ja->size(), jo->get_string().c_str());
    jd->insert(make_pair("node_id", jo));

    //Send
    VLOG_DBG(lg, "Sending reply: %s", jm.get_string().c_str());
    stream.send(jm.get_string());
  }

  void lavi_switches::getInstance(const Context* c,
				  lavi_switches*& component)
  {
    component = dynamic_cast<lavi_switches*>
      (c->get_by_interface(container::Interface_description
			      (typeid(lavi_switches).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<lavi_switches>,
		     lavi_switches);
} // vigil namespace
