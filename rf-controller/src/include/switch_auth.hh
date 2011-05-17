/* Copyright 2008 (C) Nicira, Inc.
 *
 * This file is part of NOX.
 *
 * NOX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NOX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NOX.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef  SWITCH_AUTH_HH
#define  SWITCH_AUTH_HH

/**
 * An abstract class representing the functionality 
 * required to determine if a switch is allowed to connect
 * to Nox.  As described in nox.hh, only a single Switch_Auth
 * component can be registered with Nox.  
 */ 

#include "openflow/openflow.h"
#include "openflow.hh"
#include <boost/function.hpp> 


namespace vigil {

class Switch_Auth {

public:

    typedef boost::function<void(bool)> Auth_callback; 

    // note: the callback 'cb' MUST be called using the dispatcher
    // (i.e. a post() call), not called directly. 
    virtual void check_switch_auth(
        std::auto_ptr<Openflow_connection> &oconn, 
        ofp_switch_features* features, Auth_callback cb) = 0; 

};


} // namespace vigil

#endif
