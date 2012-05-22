#include "hosttracker.hh"
#include <boost/bind.hpp>

namespace vigil
{
  static Vlog_module lg("hosttracker");
  
  Host_location_event::Host_location_event(const ethernetaddr host_,
					   const list<hosttracker::location> loc_,
					   enum type type_):
    Event(static_get_name()), host(host_), eventType(type_)
  {
    for (list<hosttracker::location>::const_iterator i = loc_.begin();
	 i != loc_.end(); i++)
      loc.push_back(*(new hosttracker::location(i->dpid,
						i->port,
						i->lastTime)));
  }

  hosttracker::location::location(datapathid dpid_, uint16_t port_, time_t tv):
    dpid(dpid_), port(port_), lastTime(tv)
  { }

  void hosttracker::location::set(const hosttracker::location& loc)
  {
    dpid = loc.dpid;
    port = loc.port;
    lastTime = loc.lastTime;
  }

  void hosttracker::configure(const Configuration* c) 
  {
    defaultnBindings = DEFAULT_HOST_N_BINDINGS;
    hostTimeout = DEFAULT_HOST_TIMEOUT;

    register_event(Host_location_event::static_get_name());
  }
  
  void hosttracker::install()
  {
  }

  void hosttracker::check_timeout()
  {
    if (hostlocation.size() == 0)
      return;

    time_t timenow;
    time(&timenow);

    hash_map<ethernetaddr,list<hosttracker::location> >::iterator i = \
      hostlocation.find(oldest_host());
    if (i == hostlocation.end())
      return;
    
    list<hosttracker::location>::iterator j = get_oldest(i->second);
    while (j->lastTime+hostTimeout < timenow)
    {
      //Remove old entry
      i->second.erase(j);
      if (i->second.size() == 0)
      	hostlocation.erase(i);

      //Get next oldest
      i = hostlocation.find(oldest_host());
      if (i == hostlocation.end())
	return;
      j = get_oldest(i->second);
    }

    //Post next one
    timeval tv = {get_oldest(i->second)->lastTime+hostTimeout-timenow, 0};
    post(boost::bind(&hosttracker::check_timeout, this), tv);
  }

  const ethernetaddr hosttracker::oldest_host()
  {
    ethernetaddr oldest((uint64_t) 0);
    time_t oldest_time = 0;
    hash_map<ethernetaddr,list<hosttracker::location> >::iterator i = \
      hostlocation.begin();
    while (i != hostlocation.end())
    {
      if (oldest_time == 0 ||
	  get_oldest(i->second)->lastTime < oldest_time)
      {
	oldest_time = get_oldest(i->second)->lastTime;
	oldest = i->first;
      }
      i++;
    }

    return oldest;
  } 

  void hosttracker::add_location(ethernetaddr host, datapathid dpid, 
				 uint16_t port, time_t tv, bool postEvent)
  {
    //Set default time as now
    if (tv == 0)
      time(&tv);

    add_location(host, hosttracker::location(dpid,port,tv), postEvent);
  }

  void hosttracker::add_location(ethernetaddr host, hosttracker::location loc,
				 bool postEvent)
  {
    hash_map<ethernetaddr,list<hosttracker::location> >::iterator i = \
      hostlocation.find(host);
    if (i == hostlocation.end())
    {
      //New host
      list<location> j = list<location>();
      j.push_front(loc);
      hostlocation.insert(make_pair(host,j));

      if (postEvent)
	post(new Host_location_event(host,j,Host_location_event::ADD));

      VLOG_DBG(lg, "New host %"PRIx64" at %"PRIx64":%"PRIx16"",
	       host.hb_long(), loc.dpid.as_host(), loc.port);
    }
    else
    {
      //Existing host
      list<location>::iterator k = i->second.begin();
      while (k->dpid != loc.dpid || k->port != loc.port)
      {
	k++;
	if (k == i->second.end())
	  break;
      }

      if (k == i->second.end())
      {
	//New location
	while (i->second.size() >= getBindingNumber(host))
	  i->second.erase(get_oldest(i->second));
	i->second.push_front(loc);
	VLOG_DBG(lg, "Host %"PRIx64" at new location %"PRIx64":%"PRIx16"",
		 i->first.hb_long(), loc.dpid.as_host(), loc.port);

	if (postEvent)
	  post(new Host_location_event(host,i->second,
				       Host_location_event::MODIFY));
      }
      else
      {
	//Update timeout
	k->lastTime = loc.lastTime;
	VLOG_DBG(lg, "Host %"PRIx64" at old location %"PRIx64":%"PRIx16"",
		 i->first.hb_long(), loc.dpid.as_host(), loc.port);
      }	
    }

    VLOG_DBG(lg, "Added host %"PRIx64" to location %"PRIx64":%"PRIx16"",
	     host.hb_long(), loc.dpid.as_host(), loc.port);

    //Schedule timeout if first entry
    if (hostlocation.size() == 0)
    {
      timeval tv= {hostTimeout,0};
      post(boost::bind(&hosttracker::check_timeout, this), tv);
    }
  }

  void hosttracker::remove_location(ethernetaddr host, datapathid dpid, 
				    uint16_t port, bool postEvent)
  {
    remove_location(host, hosttracker::location(dpid,port,0), postEvent);
  }

  void hosttracker::remove_location(ethernetaddr host, 
				    hosttracker::location loc,
				    bool postEvent)
  {
    hash_map<ethernetaddr,list<hosttracker::location> >::iterator i =	\
      hostlocation.find(host);
    if (i != hostlocation.end())
    {
      bool changed = false;
      list<location>::iterator k = i->second.begin();
      while (k != i->second.end())
      {
	if (k->dpid == loc.dpid || k->port == loc.port)
	{
	  k = i->second.erase(k);
	  changed = true;
	}
	else
	  k++;
      }

      if (postEvent && changed)
	post(new Host_location_event(host,i->second,
				     (i->second.size() == 0)?
				     Host_location_event::REMOVE:
				     Host_location_event::MODIFY));

      if (i->second.size() == 0)
	hostlocation.erase(i);	
    }
    else
      VLOG_DBG(lg, "Host %"PRIx64" has no location, cannot unset.");
  }

  const hosttracker::location hosttracker::get_latest_location(ethernetaddr host)
  {
    list<hosttracker::location> locs = \
      (list<hosttracker::location>) get_locations(host);
    if (locs.size() == 0)
      return hosttracker::location(datapathid(), 0, 0);
    else
      return *(get_newest(locs));
  }

  const list<ethernetaddr> hosttracker::get_hosts()
  {
    list<ethernetaddr> hostlist;
    hash_map<ethernetaddr,list<hosttracker::location> >::iterator i = \
      hostlocation.begin();
    while (i != hostlocation.end())
    {
      hostlist.push_back(i->first);
      i++;
    }

    return hostlist;
  }

  const list<hosttracker::location> hosttracker::get_locations(ethernetaddr host)
  {
    hash_map<ethernetaddr,list<hosttracker::location> >::iterator i = \
      hostlocation.find(host);
    if (i == hostlocation.end())
      return list<hosttracker::location>();
    else
      return i->second;
  }

  list<hosttracker::location>::iterator 
  hosttracker::get_newest(list<hosttracker::location>& loclist)
  {
    list<location>::iterator newest = loclist.begin();
    list<location>::iterator j = loclist.begin();
    while (j != loclist.end())
    {
      if (j->lastTime > newest->lastTime)
	newest = j;
      j++;
    }  
    return newest;
  }

  list<hosttracker::location>::iterator 
  hosttracker::get_oldest(list<hosttracker::location>& loclist)
  {
    list<location>::iterator oldest = loclist.begin();
    list<location>::iterator j = loclist.begin();
    while (j != loclist.end())
    {
      if (j->lastTime < oldest->lastTime)
	oldest = j;
      j++;
    }  
    return oldest;
  }

  uint8_t hosttracker::getBindingNumber(ethernetaddr host)
  {
    hash_map<ethernetaddr,uint8_t>::iterator i = nBindings.find(host);
    if (i != nBindings.end())
      return i->second;

    return defaultnBindings;
  }

  const list<hosttracker::location> hosttracker::getLocations(ethernetaddr host)
  {
    hash_map<ethernetaddr,list<hosttracker::location> >::iterator i = \
      hostlocation.find(host);
    
    if (i == hostlocation.end())
      return list<hosttracker::location>();
    
    return i->second;
  }

  void hosttracker::getInstance(const Context* c,
				  hosttracker*& component)
  {
    component = dynamic_cast<hosttracker*>
      (c->get_by_interface(container::Interface_description
			      (typeid(hosttracker).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<hosttracker>,
		     hosttracker);
} // vigil namespace
