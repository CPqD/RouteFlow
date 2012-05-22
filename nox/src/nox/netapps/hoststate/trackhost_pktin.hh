/* Copyright 2010 (C) Stanford University.
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
#ifndef trackhost_pktin_HH
#define trackhost_pktin_HH

#include "component.hh"
#include "config.h"
#include "hosttracker.hh"
#include "topology/topology.hh"

#ifdef LOG4CXX_ENABLED
#include <boost/format.hpp>
#include "log4cxx/logger.h"
#else
#include "vlog.hh"
#endif

namespace vigil
{
  using namespace std;
  using namespace vigil::container;
  using namespace vigil::applications;

  /** \brief trackhost_pktin: Tracks host location using packet in
   * \ingroup noxcomponents
   * 
   * @author ykk
   * @date February 2010
   */
  class trackhost_pktin
    : public Component 
  {
  public:
    /** \brief Constructor of trackhost_pktin.
     *
     * @param c context
     * @param node configuration (JSON object)
     */
    trackhost_pktin(const Context* c, const json_object* node)
      : Component(c)
    {}
    
    /** \brief Handle packet in to register host location
     * @param e packet in event
     * @return CONTINUE
     */
    Disposition handle_pkt_in(const Event& e);

    /** \brief Configure trackhost_pktin.
     * 
     * Parse the configuration, register event handlers, and
     * resolve any dependencies.
     *
     * @param c configuration
     */
    void configure(const Configuration* c);

    /** \brief Start trackhost_pktin.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Get instance of trackhost_pktin.
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    trackhost_pktin*& component);

  private:
    /** Reference to topology
     */
    Topology* topo;
    /** Reference to hosttracker
     */
    hosttracker* ht;
  };
}

#endif
