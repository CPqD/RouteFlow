#include "lavi_nodes.hh"
#include "assert.hh"

namespace vigil
{
  static Vlog_module lg("lavi_nodes");
  
  void lavi_nodes::configure(const Configuration* c) 
  {
    nodetype = "all";

    register_handler<JSONMsg_event>
      (boost::bind(&lavi_nodes::handle_req, this, _1));
  }
  
  void lavi_nodes::install()
  {
  }

  void lavi_nodes::send_list(const Msg_stream& stream)
  {
    VLOG_ERR(lg, "Should not be called!");
  }

  Disposition lavi_nodes::handle_req(const Event& e)
  {
    const JSONMsg_event& jme = assert_cast<const JSONMsg_event&>(e);
    json_dict::iterator i;
    json_dict::iterator j;

    if (nodetype == "all")
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
	j = jodict->find("node_type");
	if (i != jodict->end() &&
	    j != jodict->end() &&
	    i->second->type == json_object::JSONT_STRING &&
	    j->second->type == json_object::JSONT_STRING &&
	    (*((string *) j->second->object) == "all" ||
	     *((string *) j->second->object) == nodetype))
	{
	  if (*((string *) i->second->object) == "request")
	  {
	    VLOG_DBG(lg, "Sending list of nodes of type %s",
		     ((string *) j->second->object)->c_str());
	    send_list(*jme.sock);
	  }
	  else if (*((string *) i->second->object) == "subscribe" )
	  { 
	    VLOG_DBG(lg, "Adding %p to interested %s list", 
		     jme.sock->stream, nodetype.c_str());
	    interested.push_back(jme.sock);
	  }
	  else if (*((string *) i->second->object) == "unsubscribe" )
	    unsubscribe(jme.sock);
	  
	}
      }
    }
    
    return CONTINUE;
  }

  void lavi_nodes::unsubscribe(Msg_stream* sock)
  {
    list<Msg_stream*>::iterator k = interested.begin();
    while (k != interested.end())
    {
      if (*k == sock)
      {
	VLOG_DBG(lg, "Removing %p from interested %s list", 
		 sock->stream, nodetype.c_str());
	k = interested.erase(k);
      }
      else
	k++;
    }
  }

  void lavi_nodes::getInstance(const Context* c,
				  lavi_nodes*& component)
  {
    component = dynamic_cast<lavi_nodes*>
      (c->get_by_interface(container::Interface_description
			      (typeid(lavi_nodes).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<lavi_nodes>,
		     lavi_nodes);
} // vigil namespace
