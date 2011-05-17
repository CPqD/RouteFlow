RouteFlow v0.1.0
-----------------------------------
Copyright (C) 2011 CPqD

Welcome
========

Welcome to the RouteFlow remote virtual routing platform.  This distribution includes all the software you need to build, install, and deploy RouteFlow in your OpenFlow network.

This version of RouteFlow is an alpha developers' release intended to evaluate RouteFlow for providing virtualized
IP routing services on one or more OpenFlow switches.

*Note*

RouteFlow relies on NOX and OpenFlow as the communication protocol for controlling switches.  
RouteFlow uses Open vSwitch to provide the connectivitiy within the virtual environment where Linux virtual machines run the Quagga routing engine.
This build supports Openflow v1.0.0 and NOX v0.6.
Please be aware of OpenFlow, NOX, Open vSwitch, Quagga and RouteFlow licenses.

Contents
========
- Overview
- Install
- Running
- Changes from last version
- Known bugs
- TODO (+ Upcoming features)

Overview
========

RouteFlow is made of four basic modules: rf-slave, rf-server, rf-controller, and the rf-protocol:


- RF-Slave is the module running as a daemon in the Virtual Machine (VM) responsible for detecting changes in the linux ARP table and ROUTE tables. Upon a change is detected (via IP Netlink announcements), the corresponding rf-Protocol message is sent to the rf-Server.

- RF-Server is a standalone application that manages the VM running the RF-Slave daemon and requests the creation of the OpenFlow flow mod messages according to route changes detected by rf-slaves. The RF-Server keeps the mapping between the rf-slave VM instances and the corresponding datapath switches. In addition, the RF-Server configures the Open vSwitch via the OpenFlow protocol to maintain the connectivity in the virtual environment formed by the set of registered VMs;

- RF-Controller is a NOX (OpenFlow controller) application responsible for the interactions with the OpenFlow switches (identified by Datapath ID - DPID) via the OpenFlow protocol and implements the RF-Protocol to send and receives commands from/to the rf-server.

- RF-Protocol defines most of the classes used by the other modules and specifies the protocol messages used for the interactions between the rf-server and the rf-slave and the rf-controller.;



+----- VM ----------+
| Quagga | RF-Slave |
+-------------------+
	\
M:1     \ rf-Protocol
	\
+----------------+
| rf-Server  	 |
+----------------+
	\
1:1     \ rf-Protocol
	\
+|-----------------------------+
| rf-Controller  | Discovery   |
|------------------------------|
| 	NOX                    |
+------------------------------+
	\
1:N     \ OpenFlow Protocol
	\
+-----------------+
| OF Switch (DPID)|      	 
+-----------------+




Install 
=======
See the companion INSTALL file.

Running 
========

Once the VMs are up and assigned to the discovered Datapath switches, routing protocol messages originating at the VMs running Quagga are sent over the rf-Controller to the corresponding port in the Datapah. Conversely, routing protocol messages entering a Datapath port will match the default flow entry and will be forwarded to the rf-Controller, which in turn delivers it to the virtual interface of the corresponding VM.


Changes from last version
========
-


Known Bugs
========
-


ToDo (+ features expected in upcoming versions)
========
- Alternative virtualization environments, e.g., LXC, OpenVZ

- Dynamic virtual topology maintenance, with selective routing protocol messages delivery to the Datapath.

- Flexible mapping (M:1, 1:N and M:N) between Datapath and VMs, allowing for virtual routing approaches and/or physical switch stacking.

- Hooks into Quagga Zebra to reflect link up/down events

- Improve the scenario where routing protocol messages are kept in the virtual domain and topology updates are replicated in response to an OpenFlow-based topology discovery and maintenance approach.