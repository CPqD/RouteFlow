# Welcome
This version of RouteFlow is a beta developers' release intended to evaluate RouteFlow for providing virtualized IP routing services on one or more OpenFlow switches.

You can learn more about RouteFlow in our [main page in GitHub](http://cpqd.github.com/RouteFlow/) and in our [website](https://sites.google.com/site/routeflow/).

Please be aware of POX, OpenFlow, Open vSwitch, Quagga, MongoDB, jQuery, JIT and RouteFlow licenses and terms.

# Distribution overview
RouteFlow is a distribution composed by three basic applications: RFClient, RFServer and RFProxy.

* RFClient runs as a daemon in the Virtual Machine (VM), detecting changes in the Linux ARP and routing tables. Routing information is sent to the RFServer when there's an update.

* RFServer is a standalone application that manages the VMs running the RFClient daemons. The RFServer keeps the mapping between the RFClient VM instances and interfaces and the corresponding switches and ports. It connects to RFProxy to instruct it about when to configure flows and also to configure the Open vSwitch to maintain the connectivity in the virtual environment formed by the set of VMs.

* RFProxy is an application (for POX and other controllers) responsible for the interactions with the OpenFlow switches (identified by datapaths) via the OpenFlow protocol. It listens to instructions from the RFServer and notifies it about events in the network. We recommend running POX when you are experimenting and testing your network. Other implementations in different controllers will be available soon.

There is also a library of common functions (rflib). It has implementations of the IPC, utilities like custom types for IP and MAC addresses manipulation and OpenFlow message creation.

Additionally, there's `rfweb`, an extra module that provides an web interface for RouteFlow.

# RouteFlow architecture
```
+--------VM---------+
| Quagga | RFClient |
+-------------------+
         \
M:1      \ RFProtocol
         \
+-------------------+
|     RFServer      |
+-------------------+
         \
1:1      \ RFProtocol
         \
+-------------------+
|      RFProxy      |
|-------------------|
|    Controller     |
+-------------------+
         \
1:N      \ OpenFlow Protocol
         \
+-------------------+
|  OpenFlow Switch  |
+-------------------+
```

# Building

RouteFlow runs on Ubuntu 12.04.

1. Install the dependencies:
```
sudo apt-get install build-essential git libboost-dev \
  libboost-program-options-dev libboost-thread-dev \
  libboost-filesystem-dev iproute-dev openvswitch-switch \
  mongodb python-pymongo
```

2. Clone RouteFlow's repository on GitHub:
```
$ git clone git://github.com/CPqD/RouteFlow.git
```

3. Build `rfclient`
```
make rfclient
```

That's it! Now you can run tests 1 and 2. The setup to run them is described in the "Running" section.

# Running
The folder rftest contains all that is needed to create and run two test cases.

## Virtual environment
First, create the default LXC containers that will run as virtual machines:
```
$ sudo ./create
```
The containers will have a default root/root user/password combination. **You should change that if you plan to deploy RouteFlow**.

By default, the tests below will use the LXC containers created  by the `create` script. You can use other virtualization technologies. If you have experience with or questions about setting up RouteFlow on a particular technology, contact us! See the "Support" section.

## Test cases

Default configuration files are provided for these tests in the `rftest` directory (you don't need to change anything).
You can stops them at any time by pressing CTRL+C.

### rftest1
1. Run:
```
$ sudo ./rftest1
```

2. You can then log in to the LXC container b1 and try to ping b2:
```
$ sudo lxc-console -n b1
```

3. Inside b1, run:
```
# ping 172.31.2.2
```

For more details on this test, see its [explanation](https://github.com/CPqD/RouteFlow/wiki/Tutorial-1:-rftest1).

### rftest2
This test should be run with a [Mininet](http://yuba.stanford.edu/foswiki/bin/view/OpenFlow/Mininet) simulated network.

1. Run:
```
$ sudo ./rftest2 --pox
```

2. Once you have a Mininet VM up and running, copy the network topology files in rftest to the VM:
```
$ scp topo-4sw-4host.py openflow@[Mininet address]:/home/openflow/mininet/custom
$ scp ipconf openflow@[Mininet address]:/home/openflow/
```

3. Then start the network:
```
$ sudo mn --custom ~/mininet/custom/topo-4sw-4host.py --topo=rftopo" \
   --controller=remote --ip=[Controller address] --port=6633"
```
Inside Mininet, load the address configuration:
```
mininet> source ipconf
```
Wait for the network to converge (it should take a few seconds), and try to ping:
```
mininet> pingall
...
mininet> h2 ping h3
```

For more details on this test, see its [explanation](http://sites.google.com/site/routeflow/documents/tutorial2-four-routers-with-ospf) (it's a bit dated).


## Now what?
If you want to use the web interface to inspect RouteFlow behavior, see the wiki page on [rfweb](https://github.com/CPqD/RouteFlow/wiki/The-web-interface).

If you want to create your custom configurations schemes for a given setup, check out the [configuration section of the first tutorial](https://github.com/CPqD/RouteFlow/wiki/Tutorial-1:-rftest1#configuration-file) and the guide on [how to create your virtual environment](https://github.com/CPqD/RouteFlow/wiki/Virtual-environment-creation).

# Support
If you want to know more or need to contact us regarding the project for anything (questions, suggestions, bug reports, discussions about RouteFlow and SDN in general) you can use the following resources:
* RouteFlow repository [wiki](https://github.com/CPqD/RouteFlow/wiki) and [issues](https://github.com/CPqD/RouteFlow/issues) in GitHub

* Google Groups [mailing list](http://groups.google.com/group/routeflow-discuss?hl=en_US)


# Known Bugs
* RouteFlow: when all datapaths go down and come back after a short while, 
  bogus routes are installed on them caused by delayed updates from RFClients.

* rfweb: an update cycle is needed to show the images in the network view in some browsers

* See also: [Issues](https://github.com/CPqD/RouteFlow/issues) in Github


# TODO (and features expected in upcoming versions)
* Tests and instructions for other virtualization environments

* Hooks into Quagga Zebra to reflect link up/down events and extract additional route / label information

* Let the RFServer order the RFClient to set the VM's non-administrative interfaces to the same MAC Address

* Create a verbose mode for RFServer

* Configuration arguments for RFServer

* Port RFProxy to Trema, Floodlight

* Add TTL-decrement action (if supported by the datapath devices)

* Explore integration opportunities with FlowVisor

* Libvirt: Virtualization-independence to accomodate alternative virtualization environments via unified virtualization API provided by libvirt (provide on-demand start-up of VMs via libvirt upon interactions (e.g. CLI) with RFServer)

Routing Protocol Optimizations
* Separate topology discovery and maintenance from state distribution
* Dynamic virtual topology maintenance, with selective routing protocol messages delivery to the Datapath (e.g., HELLOs).
* Improve the scenario where routing protocol messages are kept in the virtual domain and topology is mantained through a  Topology Discovery controller application.
* Hooks into Quagga Zebra to reflect link up/down events

Resiliency & Scalability:
* Physically distribute the virtualization environment via mutliple OVS providing the connectivity of the virtual control network
* Improve resiliency: Have a "stand-by" environment to take over in case of failure of the master RFServer / Virtualized Control Plane

* For smaller changes, see TODO markings in the files.


_RouteFlow - Copyright (c) 2012 CPqD_
