#include "flowlog.hh"
#include "flow-removed.hh"
#include "netinet++/ethernetaddr.hh"
#include "shutdown-event.hh"
#include "assert.hh"
#include <boost/bind.hpp>

namespace vigil
{
  static Vlog_module lg("flowlog");
  
  flowlog::flowlog(const Context* c, const json_object* node)
    : Component(c)
  {
    filename = string(FLOWLOG_DEFAULT_FILE);    
    threshold = FLOWLOG_DEFAULT_THRESHOLD;
  }

  void flowlog::configure(const Configuration* c) 
  {
    resolve(tlog);

    register_handler<Flow_removed_event>
      (boost::bind(&flowlog::handle_flow_removed, this, _1));
    register_handler<Shutdown_event>
      (boost::bind(&flowlog::handle_shutdown, this, _1));

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
  
  void flowlog::install()
  {
    //Create table
    tablog::type_name_list flowfield;

    flowfield.push_back(make_pair(tablog::TYPE_UINT64_T, "sec"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT64_T, "usec"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT64_T, "dpid"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT32_T, "duration_sec"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT32_T, "duration_nsec"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT16_T, "idle_timeout"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT64_T, "packet_count"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT64_T, "byte_count"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT64_T, "cookie"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT32_T, "wildcards"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT16_T, "in_port"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT64_T, "dl_src"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT64_T, "dl_dst"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT16_T, "dl_vlan"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT8_T, "dl_vlan_pcp"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT16_T, "dl_type"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT16_T, "nw_tos"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT16_T, "nw_proto"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT32_T, "nw_src"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT32_T, "nw_dst"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT16_T, "tp_src"));
    flowfield.push_back(make_pair(tablog::TYPE_UINT16_T, "tp_dst"));
    tlog->create_table(FLOWLOG_TABLE,flowfield);

    //Prepare file
    tlog->prep_dump_file(FLOWLOG_TABLE, filename.c_str(), true);
    dump_id = tlog->set_auto_dump(FLOWLOG_TABLE, threshold,
				  tablog::DUMP_FILE);
    
  }

  Disposition flowlog::handle_flow_removed(const Event& e)
  {
    const Flow_removed_event& fre = assert_cast<const Flow_removed_event&>(e);

    //Log values
    timeval now;
    gettimeofday(&now, NULL);
    list<void*> values;
    void* value;
    values.clear();
    
    value = new uint64_t(now.tv_sec);
    values.push_back(value);
    value = new uint64_t(now.tv_usec);
    values.push_back(value);
    value = new uint64_t(fre.datapath_id.as_host());
    values.push_back(value);
    value = new uint32_t(fre.duration_sec);
    values.push_back(value);
    value = new uint32_t(fre.duration_nsec);
    values.push_back(value);
    value = new uint16_t(fre.idle_timeout);
    values.push_back(value);
    value = new uint64_t(fre.packet_count);
    values.push_back(value);
    value = new uint64_t(fre.byte_count);
    values.push_back(value);
    value = new uint64_t(fre.cookie);
    values.push_back(value);

    const ofp_match* ofpm = fre.get_flow();
    value = new uint32_t(ntohl(ofpm->wildcards));
    values.push_back(value);
    value = new uint16_t(ntohs(ofpm->in_port));
    values.push_back(value);
    value = new uint64_t(ethernetaddr(ofpm->dl_src).hb_long());
    values.push_back(value);
    value = new uint64_t(ethernetaddr(ofpm->dl_dst).hb_long());
    values.push_back(value);
    value = new uint16_t(ntohs(ofpm->dl_vlan));
    values.push_back(value);
    value = new uint8_t(ntohs(ofpm->dl_vlan_pcp));
    values.push_back(value);
    value = new uint16_t(ntohs(ofpm->dl_type));
    values.push_back(value);
    value = new uint8_t(ntohs(ofpm->nw_tos));
    values.push_back(value);
    value = new uint8_t(ntohs(ofpm->nw_proto));
    values.push_back(value);
    value = new uint32_t(ntohs(ofpm->nw_src));
    values.push_back(value);
    value = new uint32_t(ntohs(ofpm->nw_dst));
    values.push_back(value);
    value = new uint16_t(ntohs(ofpm->tp_src));
    values.push_back(value);
    value = new uint16_t(ntohs(ofpm->tp_dst));
    values.push_back(value);

    tlog->insert(dump_id, values);

    return CONTINUE;
  }

  Disposition flowlog::handle_shutdown(const Event& e)
  {
    //Dump remaining values
    tlog->dump_file(FLOWLOG_TABLE, true);
    
    return CONTINUE;
  }

  void flowlog::getInstance(const Context* c,
				  flowlog*& component)
  {
    component = dynamic_cast<flowlog*>
      (c->get_by_interface(container::Interface_description
			      (typeid(flowlog).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<flowlog>,
		     flowlog);
} // vigil namespace
