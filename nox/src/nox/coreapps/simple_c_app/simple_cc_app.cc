#include "simple_cc_app.hh"

namespace vigil
{
  static Vlog_module lg("simple_cc_app");
  
  void simple_cc_app::configure(const Configuration* c) 
  {
    lg.dbg(" Configure called ");
  }
  
  void simple_cc_app::install()
  {
    lg.dbg(" Install called ");
  }

  void simple_cc_app::getInstance(const Context* c,
				  simple_cc_app*& component)
  {
    component = dynamic_cast<simple_cc_app*>
      (c->get_by_interface(container::Interface_description
			      (typeid(simple_cc_app).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<simple_cc_app>,
		     simple_cc_app);
} // vigil namespace
