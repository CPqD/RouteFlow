#include "lavitest_showflow.hh"
#include "assert.hh"
#include "flow.hh"
#include "route/flowroute_record.hh"
#include <boost/bind.hpp>

namespace vigil
{
  static Vlog_module lg("lavitest_showflow");
  
  void lavitest_showflow::configure(const Configuration* c) 
  {
    resolve(sr);

    register_handler<Flow_route_event>
      (boost::bind(&lavitest_showflow::handle_flow_route, this, _1));
  }
  
  void lavitest_showflow::install()
  {
    sr->post_flow_record = true;
  }

  Disposition lavitest_showflow::handle_flow_route(const Event& e)
  {
    const Flow_route_event& fre = assert_cast<const Flow_route_event&>(e);

    //Check for ICMP type
    if (ntohs(fre.flow.dl_type) == 0x0800 &&  fre.flow.nw_proto == 1 &&
	ntohl(fre.flow.nw_src) < ntohl(fre.flow.nw_dst)) 
    {
      VLOG_DBG(lg, "Got ICMP flow %s", fre.flow.to_string().c_str());
      return CONTINUE;
    }

    return STOP;
  } 

  void lavitest_showflow::getInstance(const Context* c,
				  lavitest_showflow*& component)
  {
    component = dynamic_cast<lavitest_showflow*>
      (c->get_by_interface(container::Interface_description
			      (typeid(lavitest_showflow).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<lavitest_showflow>,
		     lavitest_showflow);
} // vigil namespace
