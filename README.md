# Welcome
This version of RouteFlow is a beta developers' release intended to evaluate RouteFlow for providing virtualized IP routing services on one or more OpenFlow switches.

You can learn more about RouteFlow in our [main page in GitHub](http://cpqd.github.com/RouteFlow/) and in our [website](https://sites.google.com/site/routeflow/).

Please be aware of NOX, POX, OpenFlow, Open vSwitch, Quagga, MongoDB, jQuery, JIT and RouteFlow licenses and terms.

# Distribution overview
RouteFlow is a distribution composed by three basic applications: RFClient, RFServer and RFProxy.

* RFClient runs as a daemon in the Virtual Machine (VM), detecting changes in the Linux ARP and routing tables. Routing information is sent to the RFServer when there's an update.

* RFServer is a standalone application that manages the VMs running the RFClient daemons. The RFServer keeps the mapping between the RFClient VM instances and interfaces and the corresponding switches and ports. It connects to RFProxy to instruct it about when to configure flows and also to configure the Open vSwitch to maintain the connectivity in the virtual environment formed by the set of VMs.

* RFProxy is an application (for NOX and POX) responsible for the interactions with the OpenFlow switches (identified by datapaths) via the OpenFlow protocol. It listens to instructions from the RFServer and notifies it about events in the network. We recommend running POX when you are experimenting and testing your network. You can also use NOX though if you need or want (for production maybe).

There is also a library of common functions (rflib). It has implementations of the IPC, utilities like custom types for IP and MAC addresses manipulation and OpenFlow message creation.

Additionally, there are two extra modules: rfweb, an web interface for RouteFlow and jsonflowagent, an SNMP agent.

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
|      NOX/POX      |
+-------------------+
         \
1:N      \ OpenFlow Protocol
         \
+-------------------+
|  OpenFlow Switch  |
+-------------------+
```

# Building
These instructions are tested on Ubuntu 11.04.

## Open vSwitch
1. Install the dependencies:
```
$ sudo apt-get install build-essential linux-headers-generic
```

2. Download Open vSwitch 1.4.1, extract it to a folder and browse to it:
```
$ wget http://openvswitch.org/releases/openvswitch-1.4.1.tar.gz
$ tar xzf openvswitch-1.4.1.tar.gz
$ cd openvswitch-1.4.1
```

3. Configure it as a kernel module, then compile and install:
```
$ ./configure --with-linux=/lib/modules/`uname -r`/build
$ make
$ sudo make install
```

4. Install the modules in your system:
```
$ sudo mkdir /lib/modules/`uname -r`/kernel/net/ovs
$ sudo cp datapath/linux/*.ko /lib/modules/`uname -r`/kernel/net/ovs/
$ sudo depmod -a
$ sudo modprobe openvswitch_mod
```

5. Edit /etc/modules to configure the automatic loading of the openvswitch_mod module:
```
$ sudo vi /etc/modules
```
Insert the following line at the end of the file, save and close:
```
openvswitch_mod
```
To be sure that everything is OK, reboot your computer. Log in and try the following command. If the "openvswitch_mod" line is shown like in the example below, you're ready to go.
```
$ sudo lsmod | grep openvswitch_mod
openvswitch_mod        68247  0
```

6. Initialize the configuration database:
```
$ sudo mkdir -p /usr/local/etc/openvswitch
$ sudo ovsdb-tool create /usr/local/etc/openvswitch/conf.db vswitchd/vswitch.ovsschema
```

### Note
If for some reason the bridge module is loaded in your system, you'll run into an invalid module format error in step 4. If you don't need the default bridge support in your system, use the modprobe -r bridge command to unload it and try the modprobe command again. If you need bridge support, you can get some help from the INSTALL.bridge file instructions in the Open vSwitch distribution directory.

To avoid automatic loading of the bridge module (which would conflict with openvswitch_mod), let's blacklist it. Access the /etc/modprobe.d/ directory and create a new bridge.conf file:
```
$ cd /etc/modprobe.d
$ sudo vi bridge.conf
```
Insert the following lines in the editor, save and close:
```
# avoid conflicts with the openvswitch module
blacklist bridge
```

## MongoDB
1. Install the dependencies:
```
$ sudo apt-get install git-core build-essential scons libboost-dev \
  libboost-program-options-dev libboost-thread-dev libboost-filesystem-dev \
  python-pip
```

2. Download and extract MongoDB v2.0.5
```
$ wget http://downloads.mongodb.org/src/mongodb-src-r2.0.5.tar.gz
$ tar zxf mongodb-src-r2.0.5.tar.gz
$ cd mongodb-src-r2.0.5
```

3. There's a conflict with a constant name in NOX and MongoDB. It has been fixed, but is not part of version 2.0.5 yet. So, we need to fix it applying the changes listed in this [commit](https://github.com/mongodb/mongo/commit/a1e68969d48bbb47c893870f6428048a602faf90).

4. Then compile and install MongoDB:
```
$ scons all
$ sudo scons --full install --prefix=/usr --sharedclient
$ sudo pip install pymongo
```

## NOX
These instructions are only necessary if you want to run RouteFlow using NOX. The version of the NOX controller we are using does not compile under newer versions of Ubuntu (11.10, 12.04). You can use POX, which doesn't require compiling.

1. Install the dependencies:
```
$ sudo apt-get install autoconf automake g++ libtool swig make git-core \
  libboost-dev libboost-test-dev libboost-filesystem-dev libssl-dev \
  libpcap-dev python-twisted python-simplejson python-dev
```

2. TwistedPython, one of the dependencies of the NOX controller bundled with the RouteFlow distribution, got an update that made it stop working. To get around this issue, edit the following file:
```
$ sudo vi /usr/lib/python2.6/dist-packages/twisted/internet/base.py
```
Insert the method `_handleSigchld` at the end of the file, at the same level as the `mainLoop` method (be diligent, this is Python code), just before the last statement (the one that reads `__all__ = []`):
```python
        def _handleSigchld(self, signum, frame, _threadSupport=platform.supportsThreads()):
            from twisted.internet.process import reapAllProcesses
            if _threadSupport:
                self.callFromThread(reapAllProcesses)
            else:
                self.callLater(0, reapAllProcesses)
```
Save the file and you're ready to go.

NOX will be compiled with the RouteFlow distribution in the steps ahead.

## RouteFlow
1. Install the dependencies:
```
$ sudo apt-get install build-essential iproute-dev swig1.3
```

2. Checkout the RouteFlow distribution:
```
$ git clone git://github.com/CPqD/RouteFlow.git
```

3. You can compile all RouteFlow applications by running the following command in the project root:
```
$ make all
```
You can also compile them individually:
```
$ make rfclient
$ make nox
```

4. That's it, everything is compiled! After the build, you can run tests 1 and 2. The setup to run them is described in the "Running" section.

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

In the tests below, you can choose to run with either NOX or POX by changing the command line arguments.
Default configuration files are provided for these tests in the `rftest` directory (you don't need to change anything).
You can stops them at any time by pressing CTRL+C.

### rftest1
1. Run:
```
$ sudo ./rftest1 --nox
```

2. You can then log in to the LXC container b1 and try to ping b2:
```
$ sudo lxc-console -n b1
```

3. Inside b1, run:
```
# ping 172.31.2.2
```

For more details on this test, see its [explanation](http://sites.google.com/site/routeflow/documents/first-routeflow) (it's a bit dated).

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


## Web interface
The rfweb module provides an web application to inspect the network, showing topology, status and statistics. The application is written in Python using the [WSGI specification](http://wsgi.readthedocs.org/en/latest/).

The web interface only works when running under POX.

It's possible to run the application in several servers, and a simple server is provided in `rfweb_server.py`. This server is very simple, and you probably don't want to use it for anything more serious than testing and playing around:
```
$ python rfweb_server.py
```

We've also tested the application with [gunicorn](http://gunicorn.org/). You can run rfweb on it using the following command:
```
$ gunicorn -w 4 -b 0.0.0.0:8080 rfweb:application
```
This command runs four workers, listening on all interfaces on port 8080.

Then to access the main page of the web interface (adapt the address to your setup), go to:
```
http://localhost:8080/index.html
```

## Configuration
You can skip this section if you intend to run the default tests mentioned above. Default configuration files are provided in this case. 

**You should read this section if you plan to create a setup for your network.**

RouteFlow configuration files are comma-separated values (CSV) files. The format is exemplified below:
```
vm_id,vm_port,dp_id,dp_port
2D0D0D0D0D0,1,8,1
2A0A0A0A0A0,3,A,2
```
Lines:

1. Contains default column names and should not be changed.

2. Tells RouteFlow to match port `1` of VM with id=`2D0D0D0D0D0` to port `1` of datapath with id=`8`.

3. Tells RouteFlow to match port `3` of VM with id=`2A0A0A0A0A0` to port `2` of datapath with id=`A`.

ID column values are expressed by hexadecimal digits, while port column values use decimal digits.

When RFServer is started, a configuration file should be provided as the first argument. The configuration will be copied to the main database. After RFServer is started, it will only associate ports in the way that is specified by the configuration.

## Build new VMs
You can skip this section if you intend to run the default tests mentioned above. Default VMs are provided in this case. 

**You should read this section if you plan to create a setup for your network.**

We recommend using LXC for the virtual machines (or containers in this case) that will run the RFClient instances. However, you can use any other virtualization technology.

The `create` script in `rftest` reads the `config` subdirectory and builds containers based on these files. These files and folders are:
* `config`: default LXC configuration file. Change the container name and interfaces in this file.
* `fstab`: fstab file. Adapt the mount points to the location of your container (`/var/lib/lxc/CONTAINER_NAME`)
* `rootfs`: represents a subset of the container file system. It will be copied into the generated container, so this is the place to adapt it to your needs: network addresses, Quagga setup and everything else.

Once you are done creating your setup, run:
```
$ sudo ./create
```
This will create the containers where RouteFlow looks for them (`/var/lib/lxc/`).

If you need to install custom software in your container, modify the `create` script where indicated in it.

The containers will have a default root/root user/password combination. **You should change that if you plan to deploy RouteFlow**.


# Support
If you want to know more or need to contact us regarding the project for anything (questions, suggestions, bug reports, discussions about RouteFlow and SDN in general) you can use the following resources:
* RouteFlow repository [wiki](https://github.com/CPqD/RouteFlow/wiki) and [issues](https://github.com/CPqD/RouteFlow/issues) at GitHub

* Google Groups [mailing list](http://groups.google.com/group/routeflow-discuss?hl=en_US)

* OpenFlowHub [forum](http://forums.openflowhub.org/forum.php)


# Known Bugs
* rftest*: When closing the tests, segfaults happen, to no effect.

* RouteFlow: when all datapaths go down and come back after a short while, 
  bogus routes are installed on them caused by delayed updates from RFClients.

* rfweb: an update cycle is needed to show the images in the network view in some browsers

* See also: [Issues](https://github.com/CPqD/RouteFlow/issues) in Github and [RouteFlow bugs](http://bugs.openflowhub.org/browse/ROUTEFLOW) in OpenFlowHub


# TODO (and features expected in upcoming versions)
* Tests and instructions for other virtualization environments

* Hooks into Quagga Zebra to reflect link up/down events and extract additional route / label information

* Create headers for RFClient.cc

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
