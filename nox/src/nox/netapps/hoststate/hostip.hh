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
#ifndef hostip_HH
#define hostip_HH

#include "component.hh"
#include "config.h"
#include "hash_map.hh"
#include "netinet++/ethernetaddr.hh"

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

  /** \brief hostip: store IP address of hosts
   * \ingroup noxcomponents
   * 
   * @author ykk
   * @date February 2010
   */
  class hostip
    : public Component 
  {
  public:
    /** \brief Constructor of hostip.
     *
     * @param c context
     * @param node XML configuration (JSON object)
     */
    hostip(const Context* c, const json_object* node)
      : Component(c)
    {}
    
    /** \brief Handle packet in to register host ip and ethernet address
     * @param e packet in event
     * @return CONTINUE
     */
    Disposition handle_pkt_in(const Event& e);

    /** \brief Configure hostip.
     * 
     * Parse the configuration, register event handlers, and
     * resolve any dependencies.
     *
     * @param c configuration
     */
    void configure(const Configuration* c);

    /** \brief Start hostip.
     * 
     * Start the component. For example, if any threads require
     * starting, do it now.
     */
    void install();

    /** \brief Get ethernet address of host with IP
     * @param ip ip address
     * @return ethernet address
     */
    ethernetaddr get_mac(uint32_t ip);

    /** \brief Get instance of hostip.
     * @param c context
     * @param component reference to component
     */
    static void getInstance(const container::Context* c, 
			    hostip*& component);

  private:
    /** Mapping between host and IP.
     */
    hash_map<uint32_t, ethernetaddr> ip_map;
  };
}

#endif
