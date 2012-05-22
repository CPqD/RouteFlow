#ifndef NETWORKGRAPH_HH_
#define NETWORKGRAPH_HH_

#include <stdint.h>
#include <stdlib.h>
#include <list>
#include "netinet++/datapathid.hh"
#include "hash_map.hh"
#include "openssl/md5.h"

namespace vigil
{
  /** \brief Class to contain abstraction of network graph.
   * 
   * @author ykk
   * @date February 2009
   */
  class network
  {
  public:

    /** \brief Switch/port pair.
     * 
     * @author ykk
     * @date February 2009
     */    
    struct switch_port
    {
      /** Switch datapath id.
       */
      datapathid dpid;
      /** Port number.
       */
      uint16_t port;
      
      /** Constructor.
       * @param dpid_ datapath id of switch
       * @param port_ port number
       */
      switch_port(datapathid dpid_, uint16_t port_):
	dpid(dpid_), port(port_)
      {}

     /** Copy constructor.
       * @param sp switch port to clone
       */
      switch_port(const switch_port& sp):
	dpid(sp.dpid), port(sp.port)
      {}

      /** Set value of switch_port.
       * @param val value to set to.
       */
      void set(switch_port val)
      {
	dpid=val.dpid;
	port=val.port;
      }

      /** Generate hash code.
       * @return hash code
       */
      uint64_t hash_code() const;

      bool operator==(const switch_port& swp) const
      {
	return (dpid == swp.dpid) && 
	  (port == swp.port);
      }
    };

    /** Network termination, classified by switch port pair.
     * 
     * @author ykk
     * @date February 2009
     */
    typedef switch_port termination;

    /** \brief Hop in route.
     *
     * @author ykk
     * @date February 2009
     */
    struct hop
    {
      /** Switch/port pair, i.e., in port to switch.
       */
      switch_port in_switch_port;
      /** List of next hop.
       */
      std::list<std::pair<uint16_t, hop*> > next_hops;

      /** Empty constructor.
       */
      hop();

      /** Copy constructor
       * @param h hop to be cloned
       */
      hop(const hop& h);

      /** Constructor.
       * @param dpid_ switch datapathid
       * @param port_ port number
       */
      hop(datapathid dpid_, uint16_t port_):
	in_switch_port(dpid_, port_)
      { }

      /** Destructor.
       * Delete std::list of pointers.
       */
      ~hop()
      {
	if (!in_switch_port.dpid.empty())
	  next_hops.clear();
      }      
    };

    /** List of next hops.
     *
     * @author ykk
     * @date February 2009
     */
    typedef std::list<std::pair<uint16_t, hop*> > nextHops;

    /** Route in network, i.e., a tree.
     * Reoute terminate when next_hop has empty datapathid, i.e., 0.
     *
     * @author ykk
     * @date February 2009
     */
    typedef hop route;  
  };
}

ENTER_HASH_NAMESPACE
template <>
struct hash<vigil::network::switch_port> {
    std::size_t operator() (const vigil::network::switch_port& sw_p) const {
        return HASH_NAMESPACE::hash<uint64_t>()(sw_p.hash_code());
    }
};
EXIT_HASH_NAMESPACE

#endif
