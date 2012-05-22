#include "rttlog.hh"
#include <boost/bind.hpp>
#include "hash_map.hh"
#include "shutdown-event.hh"
#include <time.h>

namespace vigil
{
  static Vlog_module lg("rttlog");

  rttlog::rttlog(const Context* c, const json_object* node)
    : Component(c)
  {
    filename = string(RTTLOG_DEFAULT_FILE);    
    threshold = RTTLOG_DEFAULT_THRESHOLD;
  }
  
  void rttlog::configure(const Configuration* c) 
  {
    resolve(srtt);
    resolve(tlog);

    register_handler<Shutdown_event>
      (boost::bind(&rttlog::handle_shutdown, this, _1));

    //Get commandline arguments
    const hash_map<string, string> argmap = c->get_arguments_list();
    hash_map<string, string>::const_iterator i;
    i = argmap.find("file");
    if (i != argmap.end())
      filename = i->second;
    i = argmap.find("threshold");
    if (i != argmap.end())
      threshold = (uint16_t) atoi(i->second.c_str());
    
    if (threshold < 1)
      threshold = 1;
  }
  
  void rttlog::install()
  {
    //Create table
    tablog::type_name_list rttfield;

    rttfield.push_back(make_pair(tablog::TYPE_UINT64_T, "sec"));
    rttfield.push_back(make_pair(tablog::TYPE_UINT64_T, "usec"));
    rttfield.push_back(make_pair(tablog::TYPE_UINT64_T, "dpid"));
    rttfield.push_back(make_pair(tablog::TYPE_UINT64_T, "rtt_sec"));
    rttfield.push_back(make_pair(tablog::TYPE_UINT64_T, "rtt_usec"));
    tlog->create_table(RTTLOG_TABLE,rttfield);

    //Prepare file
    tlog->prep_dump_file(RTTLOG_TABLE, filename.c_str(), true);
    dump_id = tlog->set_auto_dump(RTTLOG_TABLE, threshold,
				  tablog::DUMP_FILE);
    
    //Schedule log
    timeval tv;
    tv.tv_sec = srtt->interval;
    post(boost::bind(&rttlog::periodic_log, this), tv);
  }

  Disposition rttlog::handle_shutdown(const Event& e)
  {
    //Dump remaining values
    tlog->dump_file(RTTLOG_TABLE, true);
    
    return CONTINUE;
  }

  void rttlog::periodic_log()
  { 
    //Log values
    timeval now;
    gettimeofday(&now, NULL);
    list<void*> values;
    uint64_t* value;
    hash_map<uint64_t,timeval>::const_iterator i = srtt->rtt.begin();
    while (i != srtt->rtt.end())
    {
      values.clear();
      value = new uint64_t(now.tv_sec);
      values.push_back(value);
      value = new uint64_t(now.tv_usec);
      values.push_back(value);      
      value = new uint64_t(i->first);
      values.push_back(value);
      value = new uint64_t(i->second.tv_sec);
      values.push_back(value);
      value = new uint64_t(i->second.tv_usec);
      values.push_back(value);
      tlog->insert(dump_id, values);
      i++;
    }

    //Schedule log
    timeval tv;
    tv.tv_sec = srtt->interval;
    post(boost::bind(&rttlog::periodic_log, this), tv);
  }

  void rttlog::getInstance(const Context* c,
			   rttlog*& component)
  {
    component = dynamic_cast<rttlog*>
      (c->get_by_interface(container::Interface_description
			      (typeid(rttlog).name())));
  }



  REGISTER_COMPONENT(Simple_component_factory<rttlog>,
		     rttlog);
} // vigil namespace
