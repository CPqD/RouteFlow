#include "lavi_flows.hh"
#include "assert.hh"

namespace vigil
{
  static Vlog_module lg("lavi_flows");
  
  void lavi_flows::configure(const Configuration* c) 
  {
    show_ongoing = false;
    flowtype = "all";

   register_handler<JSONMsg_event>
     (boost::bind(&lavi_flows::handle_req, this, _1));
  }
  
  void lavi_flows::install()
  {
  }

  void lavi_flows::send_list(const Msg_stream& stream)
  {
    VLOG_ERR(lg, "Should not be called!");
  }

  Disposition lavi_flows::handle_req(const Event& e)
  {
    const JSONMsg_event& jme = assert_cast<const JSONMsg_event&>(e);
    json_dict::iterator i;
    json_dict::iterator j;

    if (flowtype == "all")
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
	unsubscribe(jme.sock);

      if (*((string *) i->second->object) == "lavi" )
      {
	i = jodict->find("command");
	j = jodict->find("flow_type");
	if (i != jodict->end() &&
	    j != jodict->end() &&
	    i->second->type == json_object::JSONT_STRING &&
	    j->second->type == json_object::JSONT_STRING &&
	    (*((string *) j->second->object) == "all" ||
	     *((string *) j->second->object) == flowtype))
	{
	  if (*((string *) i->second->object) == "request" &&
	      show_ongoing)
	  {
	    VLOG_DBG(lg, "Sending list of flows of type %s %s",
		     ((string *) j->second->object)->c_str(),
		     show_ongoing?"true":"false");
	    send_list(*jme.sock);
	  }
	  else if (*((string *) i->second->object) == "subscribe" )
	  { 
	    VLOG_DBG(lg, "Adding %p to interested %s list", 
		     jme.sock->stream, flowtype.c_str());
	    interested.push_back(jme.sock);
	  }
	  else if (*((string *) i->second->object) == "unsubscribe" )
	    unsubscribe(jme.sock);
	  
	}
      }
    }
    
    return CONTINUE;
  }

  void lavi_flows::unsubscribe(Msg_stream* sock)
  {
    list<Msg_stream*>::iterator k = interested.begin();
    while (k != interested.end())
    {
      if (*k == sock)
      {
	VLOG_DBG(lg, "Removing %p from interested %s list", 
		 sock->stream, flowtype.c_str());
	k = interested.erase(k);
      }
      else
	k++;
    }
  }

  uint64_t lavi_flows::get_id(const Flow& flow)
  {
   if (flow.cookie == 0)
      return flow.hash_code();
    else
      return flow.cookie;
  }

  void lavi_flows::getInstance(const Context* c,
				  lavi_flows*& component)
  {
    component = dynamic_cast<lavi_flows*>
      (c->get_by_interface(container::Interface_description
			      (typeid(lavi_flows).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<lavi_flows>,
		     lavi_flows);
} // vigil namespace
