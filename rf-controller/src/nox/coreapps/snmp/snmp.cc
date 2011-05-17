#include "snmp.hh"

namespace vigil
{
  static Vlog_module lg("snmp");
  
  void snmp::configure(const Configuration* c) 
  {
  }
  
  void snmp::install()
  {
  }

  void snmp::getInstance(const Context* c,
				  snmp*& component)
  {
    component = dynamic_cast<snmp*>
      (c->get_by_interface(container::Interface_description
			      (typeid(snmp).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<snmp>,
		     snmp);
} // vigil namespace
