#ifndef JSONMESSENGER_HH__
#define JSONMESSENGER_HH__

/** Server port number for \ref vigil::jsonmessenger.
 */
#define JSONMESSENGER_PORT 2703
/** Server port number for SSL connection in \ref vigil::jsonmessenger.
 */
#define JSONMESSENGER_SSL_PORT 1303
/** Enable TCP connection.
 */
#define ENABLE_TCP_JSONMESSENGER true
/** Enable SSL connection.
 */
#define ENABLE_SSL_JSONMESSENGER false
/** Send message when new connections.
 */
#define JSONMSG_ON_NEW_CONNECTION true
/** Idle interval to trigger echo request (in s).
 */
#define JSONMESSENGER_ECHO_IDLE_THRESHOLD 0
/** Echo request non reply threshold to drop connection.
 */
#define JSONMESSENGER_ECHO_THRESHOLD 3

#include "json_object.hh"
#include "messenger_core.hh"
#include <boost/shared_ptr.hpp>

namespace vigil
{
  using namespace vigil::container; 

  /** \ingroup noxevents
   * \brief Structure hold JSON message
   *
   * Copyright (C) Stanford University, 2010.
   * @author ykk
   * @date February 2010
   * @see jsonmessenger
   */
  struct JSONMsg_event : public Event
  {
    /** Constructor.
     * @param cmsg core message use to initialize JSONMsg event
     */
    JSONMsg_event(const core_message* cmsg);

    /** Destructor.
     */
    ~JSONMsg_event()
    {}

    /** For use within python.
     */
    JSONMsg_event() : Event(static_get_name()) 
    { }

    /** Static name required in NOX.
     */
    static const Event_name static_get_name() 
    {
      return "JSONMsg_event";
    }

    /** Reference to socket.
     */
    Msg_stream* sock;

    /** Reference to JSON object
     */
    boost::shared_ptr<json_object> jsonobj;
  };

  /** \ingroup noxcomponents
   * \brief Class through which to interact with NOX using JSON.
   *
   * TCP and SSL port can be changed at commandline using
   * tcpport and sslport arguments for jsonmessenger respectively. 
   * port 0 is interpreted as disabling the server socket.  
   * E.g.,
   * ./nox_core -i ptcp:6633 jsonmessenger=tcpport=11222,sslport=0
   * will run TCP server on port 11222 and SSL server will be disabled.
   *
   * Received messages are sent to other components via the JSONMsg_event.
   * This allows extension of the messaging system, with no changes of
   * jsonmessenger.
   *
   * The expected basic message format is a dictionary as follows.
   * <PRE>
   * {
   *  "type": <string | connect, disconnect, ping, echo>
   * }
   * </PRE>
   * The rest of the dictionary can be application specific.
   *
   * Copyright (C) Stanford University, 2009.
   * @author ykk
   * @date Feburary 2010
   * @see messenger_core
   * @see json_object
   */
  class jsonmessenger : public message_processor
  {
  public:
    /** \brief Constructor.
     * Start server socket.
     * @param c context as required by Component
     * @param node JSON object
     */
    jsonmessenger(const Context* c, const json_object* node);

    /** Destructor.
     * Close server socket.
     */
    virtual ~jsonmessenger() 
    { };
    
    /** Configure component
     * Register events..
     * @param config configuration
     */
    void configure(const Configuration* config);

    /** Start component.
     * Create jsonmessenger_server and starts server thread.
     */
    void install();

    /** Get instance of jsonmessenger (for python)
     * @param ctxt context
     * @param scpa reference to return instance with
     */
    static void getInstance(const container::Context* ctxt, 
			    vigil::jsonmessenger*& scpa);

    /** Function to do processing for messages received.
     * @param msg message event for message received
     * @param code code for special events
     */
    void process(const core_message* msg, int code=0);

    /** Send echo request.
     * @param sock socket to send echo request over
     */
    void send_echo(Msg_stream* sock);

    /** Function to do processing for block received.
     * @param buf pointer toblock received
     * @param dataSize size of block
     * @param data pointer to current message data (block not added)
     * @param currSize size of current message
     * @param sock reference to socket
     * @return length to copy
     */
    ssize_t processBlock(uint8_t* buf, ssize_t& dataSize,
			 uint8_t* data, ssize_t currSize, 
			 Msg_stream* sock);

    /** Function to determine message is completed.
     * @param data pointer to current message data (block not added)
     * @param currSize size of current message
     * @param sock reference to message stream
     * @return if message is completed (i.e., can be posted)
     */
    bool msg_complete(uint8_t* data, ssize_t currSize, Msg_stream* sock);

    /** Process string type, i.e., print in debug logs.
     * @param e event to be handled
     * @return CONTINUE always
     */
    Disposition handle_message(const Event& e);

    /** Reply to echo request.
     * @param echoreq echo request
     */
    void reply_echo(const JSONMsg_event& echoreq);

  private:
    /** \brief Object to count braces for JSON
     */
    struct counters
    {
      /** \brief Reset values
       */
      void reset();

      /** \brief Check if counter are balanced
       * @return if balanced
       */
      bool balanced();

      /** Number of outstanding left square brackets
       */
      ssize_t brackets;
      /** Number of outstanding left braces
       */
      ssize_t braces;
    };

    /** Reference to messenger_core.
     */
    messenger_core* msg_core;
    /** Memory allocated for \ref vigil::bookman messages.
     */
    boost::shared_array<uint8_t> raw_msg;
    /** Counters for connections
     */
    hash_map<Msg_stream*, counters> sock_counters;
    /** TCP port number.
     */
    uint16_t tcpport;
    /** SSL port number.
     */
    uint16_t sslport;
  };
}

#endif
