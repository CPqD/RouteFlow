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
#ifndef TOPOLOGY_HH
#define TOPOLOGY_HH 1

#include <list>

#include "component.hh"
#include "hash_map.hh"
#include "discovery/link-event.hh"
#include "netinet++/datapathid.hh"
#include "port.hh"

namespace vigil {
namespace applications {

/** \ingroup noxcomponents
 *
 * \brief The current network topology  
 */
class Topology
    : public container::Component {

public:
    /** \brief Structure to hold source and destination port
     */
    struct LinkPorts {
        uint16_t src;
        uint16_t dst;
    };

    typedef std::vector<Port> PortVector;
    typedef hash_map<uint16_t, std::pair<uint16_t, uint32_t> > PortMap;
    typedef std::list<LinkPorts> LinkSet;
    typedef hash_map<datapathid, LinkSet> DatapathLinkMap;

    /** \brief Structure to hold information about datapath
     */
    struct DpInfo {
        /** \brief List of ports for datapath
	 */
        PortVector ports;
        /** \brief Map of internal ports (indexed by port)
         */
        PortMap internal;
        /** \brief Map of outgoing links 
	 * (indexed by datapath id of switch on the other end)
	 */
        DatapathLinkMap outlinks;
        /** \brief Indicate if datapath is active
	 */
        bool active;
    };

    /** \brief Constructor
     */
    Topology(const container::Context*, const json_object*);

    /** \brief Get instance of component
     */
    static void getInstance(const container::Context*, Topology*&);

    /** \brief Configure components
     */
    void configure(const container::Configuration*);

    /** \brief Install components
     */
    void install();

    /** \brief Get information about datapath
     */
    const DpInfo& get_dpinfo(const datapathid& dp) const;
    /** \brief Get outgoing links of datapath
     */
    const DatapathLinkMap& get_outlinks(const datapathid& dpsrc) const;
    /** \brief Get links between two datapaths
     */
    const LinkSet& get_outlinks(const datapathid& dpsrc, const datapathid& dpdst) const;
    /** \brief Check if link is internal (i.e., between switches)
     */
    bool is_internal(const datapathid& dp, uint16_t port) const;

private:
    /** \brief Map of information index by datapath id
     */
    typedef hash_map<datapathid, DpInfo> NetworkLinkMap;
    NetworkLinkMap topology;
    DpInfo empty_dp;
    LinkSet empty_link_set;

    //Topology() { }

    /** \brief Handle datapath join
     */
    Disposition handle_datapath_join(const Event&);
    /** \brief Handle datapath leave
     */
    Disposition handle_datapath_leave(const Event&);
    /** \brief Handle port status changes
     */
    Disposition handle_port_status(const Event&);
    /** \brief Handle link changes
     */
    Disposition handle_link_event(const Event&);

    /** \brief Add new port
     */
    void add_port(const datapathid&, const Port&, bool);
    /** \brief Delete port
     */
    void delete_port(const datapathid&, const Port&);

    /** \brief Add new link
     */
    void add_link(const Link_event&);
    /** \brief Delete link
     */
    void remove_link(const Link_event&);

    /** \brief Add new internal port
     */
    void add_internal(const datapathid&, uint16_t);
    /** \brief Remove internal port
     */
    void remove_internal(const datapathid&, uint16_t);
};

} // namespace applications
} // namespace vigil

#endif
