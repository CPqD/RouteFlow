#include "switchrtt.hh"
#include "assert.hh"
#include "openflow-msg-in.hh"
#include "datapath-leave.hh"
#include "openflow-pack.hh"
#include "hash_map.hh"
#include <boost/bind.hpp>
#include <sys/time.h>

namespace vigil
{
  static Vlog_module lg("switchrtt");

  switchrtt::switchrtt(const Context* c, const json_object* node)
    : Component(c)
  {
    of_raw.reset(new uint8_t[sizeof(ofp_header)]);
    openflow_pack::header(OFPT_ECHO_REQUEST, 
			  sizeof(ofp_header)).pack(openflow_pack::get_header(of_raw));

    interval = SWITCHRTT_DEFAULT_PROBE_INTERVAL;
  }
  
  void switchrtt::configure(const Configuration* c) 
  {
    resolve(dpmem);

    register_handler<Openflow_msg_event>
      (boost::bind(&switchrtt::handle_openflow_msg, this, _1));
    register_handler<Datapath_leave_event>
      (boost::bind(&switchrtt::handle_dp_leave, this, _1));

    dpi = dpmem->dp_events.begin();

    const hash_map<string,string> argmap = c->get_arguments_list();
    hash_map<string,string>::const_iterator i = argmap.find("interval");
    if (i != argmap.end())
      interval = atoi(i->second.c_str());
    if (interval < 1)
      interval = 1;
  }

  timeval switchrtt::get_next_time()
  {
    timeval tv = {0,0};
    if (dpmem->dp_events.size() == 0)
      tv.tv_sec = interval;
    else
      tv.tv_sec = interval/dpmem->dp_events.size();

    if (tv.tv_sec == 0)
      tv.tv_sec = 1;

    return tv;
  }
  
  void switchrtt::install()
  {
    dpi = dpmem->dp_events.begin();
    
    post(boost::bind(&switchrtt::periodic_probe, this), get_next_time());
  }

  Disposition switchrtt::handle_dp_leave(const Event& e)
  {
    const Datapath_leave_event& dle = assert_cast<const Datapath_leave_event&>(e);
    
    hash_map<uint64_t,pair<uint32_t,timeval> >::iterator i =\
      echoSent.find(dle.datapath_id.as_host());
    if (i != echoSent.end())
      echoSent.erase(i);
    
    hash_map<uint64_t,timeval>::iterator j = \
      rtt.find(dle.datapath_id.as_host());
    if (j != rtt.end())
      rtt.erase(j);

    return CONTINUE;
  }

  Disposition switchrtt::handle_openflow_msg(const Event& e)
  {
    const Openflow_msg_event& ome = assert_cast<const Openflow_msg_event&>(e);
    of_header ofph;
    ofph.unpack(openflow_pack::get_header(*(ome.buf)));
    if (ofph.type == OFPT_ECHO_REPLY)
    {
      hash_map<uint64_t,pair<uint32_t,timeval> >::iterator i = \
	echoSent.find(ome.datapath_id.as_host());
      if (i == echoSent.end())
	VLOG_WARN(lg, "Received echo reply (xid %"PRIx32\
		   ") from unknown switch %"PRIx64"",
		  ofph.xid, ome.datapath_id.as_host());
      else
      {
	if (ofph.xid != i->second.first)
	  VLOG_WARN(lg, "xid mismatch %"PRIx32" vs %"PRIx32\
		    " from switch %"PRIx64"",
		    ofph.xid, i->second.first,
		    ome.datapath_id.as_host());
	else
	{
	  timeval diff, now;
	  gettimeofday(&now, NULL);
	  timersub(&now, &(i->second.second), &diff);
	  hash_map<uint64_t,timeval>::iterator j =\
	    rtt.find(ome.datapath_id.as_host());
	  if (j == rtt.end())
	    rtt.insert(make_pair(ome.datapath_id.as_host(),diff));
	  else
	    j->second = diff;
	  VLOG_DBG(lg, "Received echo reply (xid %"PRIx32\
		   ") from switch %"PRIx64"",
		   ofph.xid, ome.datapath_id.as_host());
	  echoSent.erase(i);
	}
      }
    }
    return CONTINUE;
  }

  void switchrtt::periodic_probe()
  {
    if (dpmem->dp_events.size() != 0)
    {
      if (dpi == dpmem->dp_events.end())
	dpi = dpmem->dp_events.begin();

      VLOG_DBG(lg, "Send probe to %"PRIx64"",
	       dpi->second.datapath_id.as_host());
      
      //Record send time
      timeval now;
      gettimeofday(&now, NULL);
      uint32_t currxid = openflow_pack::xid(of_raw);
      echoSent.insert(make_pair(dpi->second.datapath_id.as_host(), 
				make_pair(currxid,now)));
      //Send echo request
      send_openflow_command(dpi->second.datapath_id,of_raw, false);
      dpi++;
    }
    
    //Register for next probe
    post(boost::bind(&switchrtt::periodic_probe, this), get_next_time());
  }

  void switchrtt::getInstance(const Context* c,
				  switchrtt*& component)
  {
    component = dynamic_cast<switchrtt*>
      (c->get_by_interface(container::Interface_description
			      (typeid(switchrtt).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<switchrtt>,
		     switchrtt);
} // vigil namespace
