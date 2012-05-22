.. _sec_application-index:

Application Index 
======================

NOX itself only provides very low-level methods for interfacing with the
network.  All higher-level functions and events are created by network
applications.  The current release contains applications which
reconstruct the network topology (**discovery** and **topology**),
perform network routing (**routing**), and track the movement of hosts on
the network (**authenticator**).

To run NOX with routing, topology discovery, and host tracking issue the
following command (**note** python must be enabled)::

    ./nox_core -v routing

.. warning::

        This section is largely notes at the moment. 


Discovery
-----------

Discovery sends LLDP packets out of every switch interface.  It uses received
LLDP packets to detect switch links, at which time it generates Link_events. 

Topology
-----------

The topology application provides an in-memory record of all links
currently up in the network.  It does not post any events, but rather
exposes methods to retrieve the links sourced at any given datapath in
the network.

Authenticator
---------------

The authenticator application keeps an in-memory record of all
authenticated hosts and users in the network, indexed on network
location.  A network location is defined by a 3-tuple consisting of
a link layer address, a network layer address, and an access point
(datapath, port pair).  Usually the authenticator will listen for
Auth_events to know when to add a new host entry - however, with the
current setup all packet-sending hosts "authenticate" by default.

Other applications having a handle to the authenticator can ask for a
host's Connector record (defined in "flow-in.hh") by calling
"get_conn" and specifying a location.  Hosts are indexed by network
identifiers in the order dladdr->nwaddr->access point, and so partial
lookup methods "get_dlconns" and "get_nwconns" requiring only the
dladdr and dladdr+nwaddr, respectively, have also been exposed (useful
for finding all network layer addresses on a link layer address etc).

On packet-in events, the authenticator creates a Flow_in_event
containing the source and destination access points, which the routing
application then listens for and uses to set up the flow's route
through the network.

Lastly, the authenticator posts host join and leave events when a new
network location becomes active or times out from inactivity, 
respectively.  However, a join event is not posted when a new network
address for an already active link layer address/access point pair is
seen (as it is not a new host).

Routing
-----------

The routing application keeps track of shortest-path routes between
any two authenticated datapaths in the network.  On link changes, the
set of affected routes is updated.

Routing currently listens for Flow_in_events and routes them by
setting up exact match flow entries in all switches along the path
from the source to destination access points.

Other applications can also ask the routing module for a specific
route using the "get_route" method.

