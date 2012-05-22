#include "jsonmessenger.hh"
#include "vlog.hh"
#include "assert.hh"
#include "hash_map.hh"

namespace vigil
{
  using namespace vigil::container;    
  using namespace std;

  static Vlog_module lg("jsonmessenger");
  static const std::string app_name("jsonmessenger");

  JSONMsg_event::JSONMsg_event(const core_message* cmsg):
    Event(static_get_name())
  {
    set_name(static_get_name());
    sock = cmsg->sock;
    jsonobj.reset(new json_object((const uint8_t*)cmsg->raw_msg.get(), (ssize_t&) cmsg->len));
    VLOG_DBG(lg, "JSON message of length %zu", cmsg->len); 
  }

  jsonmessenger::jsonmessenger(const Context* c, const json_object* node): 
    message_processor(c,node)
  { 
    idleInterval = JSONMESSENGER_ECHO_IDLE_THRESHOLD;
    thresholdEchoMissed = JSONMESSENGER_ECHO_THRESHOLD;
    newConnectionMsg = JSONMSG_ON_NEW_CONNECTION;
  };
  
  void jsonmessenger::configure(const Configuration* config)
  {
    resolve(msg_core);

    register_event(JSONMsg_event::static_get_name());
    register_handler<JSONMsg_event>
      (boost::bind(&jsonmessenger::handle_message, this, _1));

    //Get default port configuration
    if (ENABLE_TCP_JSONMESSENGER)
      tcpport = JSONMESSENGER_PORT;
    else
      tcpport = 0;
    if (ENABLE_SSL_JSONMESSENGER)
      sslport = JSONMESSENGER_SSL_PORT;
    else
      sslport = 0;

    //Get commandline arguments
    const hash_map<string, string> argmap = \
      config->get_arguments_list();
    hash_map<string, string>::const_iterator i;
    i = argmap.find("tcpport");
    if (i != argmap.end())
      tcpport = (uint16_t) atoi(i->second.c_str());
    i = argmap.find("sslport");
    if (i != argmap.end())
      sslport = (uint16_t) atoi(i->second.c_str());
  }

  void jsonmessenger::install() 
  {
    if (tcpport != 0)
      msg_core->start_tcp(this, tcpport);
    
    if (sslport != 0)
    {
      boost::shared_ptr<Ssl_config> config(new Ssl_config(Ssl_config::ALL_VERSIONS,
							  Ssl_config::NO_SERVER_AUTH,
							  Ssl_config::NO_CLIENT_AUTH,
							  "nox/coreapps/messenger/serverkey.pem",
							  "nox/coreapps/messenger/servercert.pem",
							  "nox/coreapps/messenger/cacert.pem"));
      msg_core->start_ssl(this, sslport, config);
    }
  }
  
  ssize_t jsonmessenger::processBlock(uint8_t* ptr, ssize_t& dataSize,
				      uint8_t* data, ssize_t currSize,
				      Msg_stream* sock)
  {
    hash_map<Msg_stream*,counters>::iterator i = sock_counters.find(sock);

    //Counters for new connection
    if (i == sock_counters.end())
    {
      sock_counters.insert(std::make_pair(sock,counters()));
      i = sock_counters.find(sock);
      i->second.reset();
    }

    //Check block
    char prev = '\0';
    if (currSize != 0)
      prev = *(data+currSize-1);

    for (int j = 0; j < dataSize; j++)
    {
      //Check brackets and braces
      if (prev != '\\')
      {
	switch (*ptr)
	{
	case '[':
	  i->second.brackets++;
	  break;
	case ']':
	  if (i->second.brackets == 0)
	    VLOG_ERR(lg, "%p sending crap JSON data (too many ])",
		     sock->stream);
	  else
	    i->second.brackets--;
	  break;
	case '{':
	  i->second.braces++;
	  break;
	case '}':
	  if (i->second.braces == 0)
	    VLOG_ERR(lg, "%p sending crap JSON data (too many })",
		     sock->stream);
	  else
	    i->second.braces--;
	  break;
	}
      }
      //Completed message
      if ((currSize+j) > 2  && i->second.balanced())
	return (j+1);

      //Move on
      prev=*ptr;
      ptr++;
    }
    
    return dataSize;
  }

  bool jsonmessenger::msg_complete(uint8_t* data, ssize_t currSize,
				   Msg_stream* sock)
  {
    hash_map<Msg_stream*,counters>::iterator i = sock_counters.find(sock);
    return ((currSize > 1) && i->second.balanced());
  }

  void jsonmessenger::process(const core_message* msg, int code)
  {
    core_message cmsg(msg->sock);
 
    switch (code)
    {
    case msg_code_normal:
      post(new JSONMsg_event(msg));
      VLOG_DBG(lg, "Message posted as JSONMsg_event");
      return;
    case msg_code_new_connection:
      //New connection
      cmsg.raw_msg.reset(new uint8_t[19]);
      cmsg.len = 18;
      strcpy((char *) cmsg.raw_msg.get(),"{\"type\":\"connect\"}");
      VLOG_DBG(lg, "JSON message of length %zu (connect)", cmsg.len); 
      break;
    case msg_code_disconnection:
      //Disconnection
      cmsg.raw_msg.reset(new uint8_t[22]);
      cmsg.len = 21;
      strcpy((char *) cmsg.raw_msg.get(),"{\"type\":\"disconnect\"}");
      VLOG_DBG(lg, "JSON message of length %zu (disconnect)", cmsg.len); 
      break;
    }

    post(new JSONMsg_event(&cmsg));
  }

  Disposition jsonmessenger::handle_message(const Event& e)
  {
    const JSONMsg_event& jme = assert_cast<const JSONMsg_event&>(e);
    json_dict::iterator i;

    VLOG_DBG(lg, "JSON: %s", jme.jsonobj->get_string().c_str());
    
    //Filter all weird messages
    if (jme.jsonobj->type != json_object::JSONT_DICT)
      return STOP;

    json_dict* jodict = (json_dict*) jme.jsonobj->object;
    i = jodict->find("type");
    if (i == jodict->end())
      return STOP;

    //Handle disconnect and hello
    if (i->second->type == json_object::JSONT_STRING)
    {
      if ( *((string *) i->second->object) == "disconnect" )
      {
	hash_map<Msg_stream*,counters>::iterator j = sock_counters.find(jme.sock);
	if (j != sock_counters.end())
	  sock_counters.erase(j);
	VLOG_DBG(lg, "Clear connection state for %p", jme.sock->stream);
      }
      else if ( *((string *) i->second->object) == "ping" )
      {
	reply_echo(jme);
	VLOG_DBG(lg, "Echo response to ping");	
      }
    }

    return CONTINUE;
  }

  void jsonmessenger::send_echo(Msg_stream* sock)
  {
    const char* jsonmsg = "\"type\":\"ping\"";

    VLOG_DBG(lg, "Sending ping on idle socket");
    sock->init(raw_msg, jsonmsg);
    sock->send(raw_msg, strlen(jsonmsg)+3);
  }

  void jsonmessenger::reply_echo(const JSONMsg_event& echoreq)
  {
    const char* jsonmsg = "\"type\":\"echo\"";

    VLOG_DBG(lg, "Replying echo");
    echoreq.sock->init(raw_msg, jsonmsg);
    echoreq.sock->send(raw_msg, strlen(jsonmsg)+3);
  }
  
  void jsonmessenger::getInstance(const container::Context* ctxt, 
				  vigil::jsonmessenger*& scpa) 
  {
    scpa = dynamic_cast<jsonmessenger*>
      (ctxt->get_by_interface(container::Interface_description
			      (typeid(jsonmessenger).name())));
  }

  void jsonmessenger::counters::reset()
  {
    brackets=0;
    braces=0;
  }

  bool jsonmessenger::counters::balanced()
  {
    return ((brackets == 0) && (braces == 0));
  }

  REGISTER_COMPONENT(vigil::container::
		     Simple_component_factory<vigil::jsonmessenger>, 
		     vigil::jsonmessenger);
}
