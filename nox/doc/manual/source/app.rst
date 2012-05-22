.. _sec_application:

Developing within NOX
===================================

Overview
----------

This section provides a brief primer to developing within NOX.
Unfortunately there currently isn't extensive API documentation so you
should expect to get comfortable with the source. 

There are really only two basic concepts one has to learn to work
effectively with NOX, the component system, and the event system.

A component is really just an encapsulation of functionality with some
additional frills to allow declaration of dependencies and dynamic loading.
For example, routing could be (and is) implemented as a component within NOX.
Any function which requires routing must declare it as a dependency to ensure
it will be available when running.

NOX components can be written in either C++ or Python (or both).  At the time
of this writing, the Python API to the network is more mature and therefore
more friendly for new NOX developers.  We recommend that unless the component
being developed has serious performance requirements, that developers start
with Python.

Events instigate all execution in the system.  An event roughly correlates to
something which happens on the network that may be of interest to a NOX
component.  For example, NOX has a number of built-in components which
correspond directly to OpenFlow messages. These include, datapath join events
(a new switch has joined the network), packet-in events (a packet was received
by a switch and forwarded to the controller), datapath leave events (a switch
has left the network), flow expiration events (a flow in the network has
expired) and many more.  In addition to OpenFlow events, components may define
and throw their own events.  Events are the primary method of communication
between components.

Components
------------

All NOX component code resides in *src/nox/coreapps*,
*src/nox/netapps/*, or *src/nox/webapps/*.  In general, each NOX
component has its own directory however this isn't strictly necessary
(for example *src/nox/coreapps/examples/* contains multiple apps).  

Building a C++ only component
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Lets use the hub program (in src/nox/coreapps/hub/ ) as an example of how to
extend NOX.  Hub is just that, a dumb network hub which floods packets
out of all ports except the incoming port.  For every packet received by
NOX, the hub floods the packet and then adds a flow entry in the
OpenFlow switch to flood all subsequent packets from the same flow.  

..  highlight:: c
 
Class Hub provides an example of a simple component.  All components must have
the following construction::


   #include "component.hh"

   class Hub 
       : public Component 
   {
   public:
        Hub(const Context* c,
            const xercesc::DOMNode*) 
            : Component(c) { }
   
       void configure(const Configuration*) {
       }
   
       void install()
       { }
   };
   
   REGISTER_COMPONENT(container::Simple_component_factory<Hub>, Hub);

A component must inherit from Cass component, have a constructor matching
hub's, and include a REGISTER_COMPONENT macro with external linkage to aid the
dynamic loader.  The methods "configure" and "install" are called at load time,
configure first before install, and are used to register events and register
event handlers.

Components must also have a meta.xml file residing in the same directory as the
component.  On startup, NOX search the directory tree for meta.xml files and
uses them to determine what components are available on the system and their
dependencies.  For C++ components, the component <library> value must match the
name of the shared library. 

Simple components generally register for events in the install method and
perform their functions in the event handlers.  Take for example hub, which
registers to be called for each packet received by NOX for any switch on the
network.::

    void install()
    {
        register_handler<Packet_in_event>(boost::bind(&Hub::handler, this, _1));
    }

Event handling and registration is described further below.

Components can communicate through events or directly.  In order to access a
component directly, you must have a handle to it.  To do this, use the resolve
method (inherited by your app class from Component). Say you want to get a
pointer to the topology component. Then the call should look something like:: 

    Topology* topology;
    resolve(topology); // resolve inherited from Component


Building a Python only component
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
..  highlight:: python 

Pure python components are much simpler to construct then C++ components.  As
an example, see src/nox/coreapps/examples/pyloop.py.  A Python component must have the
following construction::

    from nox.lib.core import *

    class foo(Component):

        def __init__(self, ctxt):
            Component.__init__(self, ctxt)

        def install(self):
            # register for event here 
            pass

        def getInterface(self):
            return str(foo)


    def getFactory():
        class Factory:
            def instance(self, ctxt):
                return foo(ctxt)

        return Factory()


You may optionally add a configure method which is called in the same order as
for C++ (before install on startup).  The following steps should be all that is
needed to build a bare-bones python component for NOX.

* Add your .py file to src/nox/coreapps/examples/
* Copy code from src/nox/apps/examples/pyloop (you need to mirror everything except for the code under the install method)
* Add your Python file(s) to NOX_RUNTIMEFILES in src/nox/apps/examples/Makefile.am
* Update src/nox/apps/examples/meta.xml to include your new app. Make sure that "python runtime" is a dependency (copying is the best approach). 

**Pointers**

* The core python API is in nox/lib/core.py and nox/lib/util.py.
* To get a handle to another component, use the Component.resolve(..) method on the class or interface to which you want a handle. For example::

   from nox.app.examples.pyswitch import pyswitch
   self.resolve(pyswitch) 

Building an integrated C++/Python component
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Building integrated components (those exposed to both C++ and Python) is rather
complex at the moment.  It will require some familiarity with swig
(http://www.swig.org).  To see how this is done, take a look at
*src/nox/apps/simple-c-py-app/*.

Events
------------

In the abstract components are really just a set of event handlers.
Events are therefore what drive all execution in NOX.  NOX contains a
number of built-in events which map directly to OpenFlow messages, these
include,

* *Datapath_join_event* (src/include/datapath-join.hh) Issued whenever a new switch is detected on the network.

* *datapath_leave_event* (src/include/datapath-leave.hh) Issued whenever a switch has left the network.

* *Packet_in_event* (src/include/packet-in.hh) Called for each new packet received by NOX.  The event includes the switch ID, the incoming port, and the packet buffer.

* *Port_status_event* (src/include/port-status.hh) Indicates a change in port status.  Contains the current port state including whether it is disabled, speed, and the port name.

* *Port_stats* (src/include/port-stats.hh) Sent in response to a Port_stats_request message and includes the current counter values for a given port (such as rx,tx, and errors).

In addition, components themselves may define and throw higher level
events which may be handled by any other events.  The following events
are thrown by existing NOX components.

* *Host_event* (src/nox/apps/authenticator/host-event.hh) Thrown whenever a new host has joined the network or a host leaves the network (generally due to timeout).

* *Link_event* (src/nox/apps/discovery/link-event.hh) Sent for each link discovered on the network.  This can be used to reconstruct the network topology if the discovery application is being run.

There are many more events in NOX that are not described here.  Until
documentation improves, the best place to learn more is to comb over the
source.

Registering for Events 
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Registering for events is simple in both C++ and Python.

..  highlight:: c

In C++, use the Component::register_handler method
(src/builtin/component.cc).  It expects the name of the event, and an
Event_handler (which must be of type
boost::function<Handler_signature>).  Relevant C++ definitions can be
found in src/nox/coponent.hh. NOX's C++ API relies heavily on
boost::bind and boost::fuctions, if you are not familiar with these you
can learn more form the boost library documentation
(http://www.boost.org).
The following example shows how one might register for a packet in event:: 

    Disposition handler(const Event& e)
    {
        return CONTINUE;
    }

    void install()
    {
        register_handler<Packet_in_event>(boost::bind(handler, this, _1));
    }

All handlers must return a Disposition (defined in src/include/event.hh)
which is either **CONTINUE**, meaning to pass the event to the next
listener, or **STOP**, which will stop the event chain.

..  highlight:: python 

Registering a handler in Python is similar to the C++ interface.  To do
so, use the register_handler interface defined in src/nox/lib/core.py.
For example::

    def handler(self):
        return  CONTINUE

    def install(self):
        self.register_handler (Packet_in_event.static_get_name(),
        handler)

Posting events 
^^^^^^^^^^^^^^^^

Any application can create and post events for other applications to
handle using the following method::

    void post(Event*) const;


.. warning::

    Events passed to post() are assumed to be dynamically allocated and
    are freed by NOX once fully dispatched.


Posting an event is simple.  For example::

        post(new Flow_in_event(flow, *src, *dst, src_dl_authed,
                               src_nw_authed, dst_dl_authed, dst_nw_authed, pi));

Posting events from Python is nearl identical.  The following code is
from src/nox/apps/discovery/discovery.py::

        e = Link_event(create_datapathid_from_host(linktuple[0]),
                      create_datapathid_from_host(linktuple[2]),
                      linktuple[1], linktuple[3], action)
        self.post(e)


Posting timers
^^^^^^^^^^^^^^^^^^^^^^

In NOX, all execution is event-driven (this isn't entirely true, but
unless you want to muck around with native threads, it's a reasonable
assumption).  Applications can ask NOX to call a handler after some
amount of time has lapsed, forming the basis for timer creation.  This
functionality is also done using the **post** method::

    Timer post(const Timer_Callback&, const timeval& duration) const;

For example, registering a method to be called every second might look like::

    void timer(){
        using namespace std;
        cout << "One second has passed " << endl;
        timevale tv={1,0}
        post(boost::bind(&Example::timer, this), tv);
    }

    timevale tv={1,0}
    post(boost::bind(&Example::timer, this), tv);

Or in Python::

    def timer():
        print 'one second has passed'
        self.post_callback(1, timer)

    post_callback(1, timer)
