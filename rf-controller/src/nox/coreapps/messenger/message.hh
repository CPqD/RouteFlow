/** \page console NOX Console/Messaging
 * 
 * NOX console is powered by vigil::jsonmessenger, which also allows other 
 * components to receive commands from external sources using vigil::JSONMsg_event.
 *
 * The basic structure of the JSON message is
 * <PRE>
 * {  
 *   "type" : <string | connect, disconnect, ping, echo>
 *   <content>
 * }
 * </PRE>
 * Content is application specific and the types connect, disconnect, ping and 
 * echo are reserved for use by vigil::jsonmessenger.
 * 
 * A user can can the TCP and SSL port for vigil::jsonmessenger at commandline using
 * tcpport and sslport arguments for jsonmessenger respectively. 
 * port 0 is interpreted as disabling the server socket.  
 * E.g.,
 * <PRE>
 * ./nox_core -i ptcp:6633 jsonmessenger=tcpport=11222,sslport=0
 * </PRE>
 * will run TCP server on port 11222 and SSL server will be disabled.
 *
 * The client side script are the following.
 * <UL>
 * <LI>nox_console</LI>
 * </UL>
 * Server side components of the console includes the following.
 * <UL>
 * <LI>vigil::lavi_switches</LI>
 * <LI>vigil::lavi_swlinks</LI>
 * </UL>
 * 
 */
