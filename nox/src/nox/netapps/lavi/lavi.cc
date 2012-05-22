#include "lavi.hh"

namespace vigil
{
  static Vlog_module lg("lavi");
  
  void lavi::configure(const Configuration* c) 
  {
  }
  
  void lavi::install()
  {
  }

  void lavi::getInstance(const Context* c,
				  lavi*& component)
  {
    component = dynamic_cast<lavi*>
      (c->get_by_interface(container::Interface_description
			      (typeid(lavi).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<lavi>,
		     lavi);
} // vigil namespace
