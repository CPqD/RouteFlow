/*
 * Copyright 2009 (C) Nicira, Inc.
 */
#ifndef SWITCH_MANAGEMENT_HH
#define SWITCH_MANAGEMENT_HH

#include <string>

#include "config.h"
#include "component.hh"
#include "switch-mgr.hh"
#include "boost/foreach.hpp"

namespace vigil {
namespace applications {

class Switch_management
    : public container::Component {
public:

    Switch_management(const container::Context*, const json_object*);

    void configure(const container::Configuration*);
    void install();
    static void getInstance(const container::Context*, Switch_management *&);

    //TODO: move mgmt methods out of core and put here
    
private:

};


} // namespace applications
} // namespace vigil


#endif /* SWITCH_MANAGEMENT_HH */
