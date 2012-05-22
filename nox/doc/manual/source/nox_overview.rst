.. _nox_overview:

The What and HOW of NOX
==========================

What? 
------

NOX is an open platform for developing management functions for
enterprise and home networks. NOX runs on commodity hardware and
provides a software environment on top of which programs can control
large networks at Gigabit speeds. More practically, NOX enables the
following:

* NOX provides **sophisticated network functionality** (management, visibility, monitoring, access controls, etc.) on extremely **cheap switches**.

* Developers can add their own control software and, unlike standard \*nix based router environments, NOX provides an interface for managing off the shelf hardware switches at line speeds.

* NOX provides a **central programming model for an entire network** -- one program can control the forwarding decisions on all switches on the network. This makes program development much easier than in the standard distributed fashion.

How? 
------

As is shown in the image below, NOX's control software runs on a single
commodity PC and manages the forwarding tables of multiple switches.

.. image:: nox-overview.jpg

NOX exports a programmatic interface on top of which multiple network
programs (which we call *applications*) can run. These applications can
hook into network events, gain access to traffic, control the switch
forwarding decisions, and generate traffic.

NOX applications control the network by using OpenFlow to configure each
switch's flow table.  This configuration can be set up reactively for
each new flow on the network, or proactively as appropriate.  The former
allows the applications more fine-grained control (per-flow decisions)
the latter is more scalable and reduces per-flow setup times.

Applications are able to determine packet forwarding rules, gather
datapath statistics, specificy header modification policies, and even
redirect full flow through the controller for in-depth inspection.

Applications have already been built on NOX which reconstruct the
network topology, track hosts as they move around the network, provide
fine-grained network access controls, and manage network history
(thereby allowing reconstruction of previous network states).
