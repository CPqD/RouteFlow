/*
 * Copyright 2009 (C) Nicira, Inc.
 */

#include "switch_management.hh"

#include "vlog.hh"

using namespace std;
using namespace vigil;
using namespace vigil::applications;

namespace {
static Vlog_module lg("switch_management");
}

namespace vigil {
namespace applications {

Switch_management::Switch_management(const container::Context* c,
        const json_object*) 
    : Component(c)
{
    //NOP
}

void
Switch_management::configure(const container::Configuration* conf) {
    //NOP
}

void
Switch_management::install() {
    //NOP
}

void
Switch_management::getInstance(const container::Context* ctxt,
        Switch_management*& h)
{
    h = dynamic_cast<Switch_management*>
        (ctxt->get_by_interface(container::Interface_description
                              (typeid(Switch_management).name())));
}


} // namespace applications
} // namespace vigil

REGISTER_COMPONENT(container::Simple_component_factory<Switch_management>, 
                   Switch_management);


