.. highlight:: rest

NOX Manual
===============================

Introduction
---------------

This document provides installation, use, and development instructions
for NOX v0.6.0.  

NOX is a network control platform. [#]_  Its purpose is to provide a
high-level, programmatic interface on top of which network management
and control applications can be built.  NOX is different from standard
network development environments (such as building routers within or on
top of Linux) in that it provides a centralized programming model for an
entire network.

NOX applications have flow-level control of the network.  This means
that they can determine which flows are allowed on the network and the
path they take.  In addition, NOX provides applications with an
abstracted view of the network resources, including the network topology
and the location of all detected hosts.

NOX is designed to support both large enterprise networks of hundreds of
switches (supporting many thousands of hosts) and smaller networks of a
few hosts.

The primary goals of NOX are:

*  To provide a platform which allows developers to innovate within home or enterprise networks using real networking hardware. Developers on NOX can control all connectivity on the network including forwarding, routing, which hosts and users are allowed etc.  In addition, NOX can interpose on any flow. 
* To provide usable network software for operators.  The current release includes central managament for all switches on a network, admission control at the user and host level, and a full policy engine.  

NOX controls the network switches through the OpenFlow protocol
(http://www.openflowswitch.org).  Therefore it requires at least one
switch on the network to support OpenFlow.  However, NOX is able to
support networks composed of OpenFlow switches interconnected with
traditional L2 switches and L3 routers.

For a more detailed description of NOX, see the :ref:`nox_overview`.

This version of NOX is a developers' release.  Applications can be
written in either C++ or Python.  NOX provides a high-level API to
OpenFlow as well as to other network controls functions (see
:ref:`sec_application`).  While intended primarily for developers, 
this release does contain a full policy engine for declaring network
access controls which can be used to manage a network.

.. warning::

    Unfortunately the current documentation lags significantly behind
    development.  While this manual should be helpful in getting the
    system up and running, it currently does not provide adequate
    coverage of the internals. 

Contents:
----------

.. toctree::
   :maxdepth: 2

   nox_overview
   installation
   vm_environment
   using
   app
   core-api
   app-index

.. Indices and tables
.. ==================
.. 
.. * :ref:`genindex`
.. * :ref:`search`

.. rubric:: Footnotes
 
.. [#] NOX is a follow-on to Ethane (http://yuba.stanford.edu/ethane). NOX is currently being developed by Nicira Networks as part of a larger effort to do government trials of the Ethane network architecture.  
