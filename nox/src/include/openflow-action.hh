/** Copyright 2009 (C) Stanford University
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
#ifndef OPENFLOW_ACTION_H
#define OPENFLOW_ACTION_H

#include "openflow-pack.hh"
#include "openflow-default.hh"
#include "netinet++/ethernetaddr.hh"
#include <stdlib.h>
#include <list>
#include <boost/shared_array.hpp>

namespace vigil
{  
  /** \brief Structure to hold OpenFlow action.
   * \ingroup noxapi
   * 
   * Copyright (C) Stanford University, 2009.
   * @author ykk
   * @date February 2009
   */
  struct ofp_action
  { 
    /** Pointer to memory for OpenFlow messages.
     */
    boost::shared_array<uint8_t> action_raw;
    
    /** Initialize action.
     */
    ofp_action()
    {}

    /** Set enqueue action, i.e., ofp_action_enqueue.
     * @param port port number of send to
     * @param queueid queue id
     */
    void set_action_enqueue(uint16_t port, uint32_t queueid);
    
    /** Set output action, i.e., ofp_action_output.
     * @param port port number of send to
     * @param max_len maximum length to send to controller
     */
    void set_action_output(uint16_t port, uint16_t max_len);

    /** Set source/destination mac address.
     * @param type OFPAT_SET_DL_SRC or OFPAT_SET_DL_DST
     * @param mac mac address to set to
     */ 
    void set_action_dl_addr(uint16_t type, ethernetaddr mac);

    /** Set source/destination IP address.
     * @param type OFPAT_SET_NW_SRC or OFPAT_SET_NW_DST
     * @param ip ip address to set to
     */ 
    void set_action_nw_addr(uint16_t type, uint32_t ip);

    /** \brief Retrieve of_action_header.
     *
     * @return of_action_header
     */
    of_action_header get_header();
  };
  
  /** \brief List of actions for switches.   
   * \ingroup noxapi
   *
   * Copyright (C) Stanford University, 2009.
   * @author ykk
   * @date February 2009
   */
  struct ofp_action_list
  {
    /** List of actions.
     */
    std::list<ofp_action> action_list;

    /** Give total length of action list, i.e.,
     * memory needed.
     * @return memory length needed for list
     */
    uint16_t mem_size();

    /** Destructor.
     * Clear list of actions.
     */
    ~ofp_action_list()
    {
      action_list.clear();
    }

    /** \brief Pack action list
     *
     * @param pointer to start packing
     */
    void pack(uint8_t* actions);
    
  };
}
#endif
