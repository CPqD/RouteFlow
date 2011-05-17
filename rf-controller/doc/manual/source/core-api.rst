.. _core_api:

NOX Network API
=================

The previous section described the component and event interfaces to
NOX, which provide the basic scaffolding around which to extend NOX.
This section provides more coverage of the internal API available to
applications for interfacing with the network.

Filtering input traffic 
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Often a component will only be interested in a subset of the traffic
received by NOX.  For example, the discovery component only operates
over LLDP traffic.  NOX provides an efficient mechanism for a component
to specify the type of traffic it is interested in.  This is done
through the **register_handler_on_match** method found in
*src/include/nox.hh*.   

This method must be called with a **Packet_expr** (defined in
*src/include/expr.hh*) which defines the packet fields to math on before
calling the handler.  The handler (Pexpr_action) differs from the
standard handler in that it does not return a Disposition value.  When
NOX receives a packet, only matching handler(s) with the lowest priority
value will be invoked.

For example, registering only for TCP packets looks like::

    // only call on TCP packets 
    Packet_expr expr;
    uint32_t val = ethernet::IP;
    expr.set_field(Packet_expr::DL_TYPE,  &val); 
    val = ip_::proto::TCP;
    expr.set_field(Packet_expr::NW_PROTO, &val); 
    register_handler_on_match(100,
                              expr,            
                              boost::bind(&Example::tcp_packet_handler, this, _1));

Managing switch flow tables
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Applications manage forwarding on the network by adding and deleting
flow entries in switches.  NOX exposes the full functionality
supported by the OpenFlow management protocol. The general function
doing this is::

    int send_openflow_command(const datapathid&, const ofp_header*, 
                              bool block) const;


OpenFlow message formats and definitions are contained in *src/include/openflow.hh*.

.. warning::

    Unfortunately this method is quite low level at the moment and
    requires callers to construct their own OpenFlow messages (yuck).
    We will provide convenience functions for the common OpenFlow
    operations in a future release.  Until then, you can use
    *src/nox/apps/switch/switch.cc* as a guide to setting up a flow.

Sending packets
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Applications can send packets on the network using one the following
methods::

    // Instruct a switch to send a packet it has buffered
    int send_openflow_packet(const datapathid&, uint32_t buffer_id, 
                             uint16_t out_port, uint16_t in_port, 
                             bool block) const;
    
    // Send a packet in a Buffer object out on the network
    int send_openflow_packet(const datapathid&, const Buffer&, 
                             uint16_t out_port, uint16_t in_port, 
                             bool block) const;

The first method accepts a **buffer_id** and assumes that the switch
is already buffering the packet.  Unless otherwise instructed,
switches buffer incoming packets and only send the first 128 bytes to
the controller.  The **buffer_id** is then passed to the application
in a **Packet_in_event**.  Use the first method to send this packet
out again (see *src/nox/coreapps/hub/hub.cc* for an example).

The second method allows an application to construct and send an
arbitrary packet out on the network.

Accessing switch table and port statistics
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Switch statistics can be collected by NOX components by issuing
statistics request messages.

