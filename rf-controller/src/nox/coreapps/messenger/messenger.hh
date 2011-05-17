#ifndef MESSENGER_HH__
#define MESSENGER_HH__

/** Server port number for \ref vigil::messenger.
 */
#define MESSENGER_PORT 2603
/** Server port number for SSL connection in \ref vigil::messenger.
 */
#define MESSENGER_SSL_PORT 1304
/** Enable TCP connection.
 */
#define ENABLE_TCP_MESSENGER true
/** Enable SSL connection.
 */
#define ENABLE_SSL_MESSENGER false
/** Send message when new connections.
 */
#define MSG_ON_NEW_CONNECTION true
/** Idle interval to trigger echo request (in s).
 */
#define MESSENGER_ECHO_IDLE_THRESHOLD 0
/** Echo request non reply threshold to drop connection.
 */
#define MESSENGER_ECHO_THRESHOLD 3

#include "messenger_core.hh"

namespace vigil
{
  using namespace vigil::container; 

  /** Types of messages.
   * Type value 0x00 to 0x09 are reserved for internal use.
   */
  enum msg_type
  {
    /** Disconnection message.
     * Need to be consistent.
     */
    MSG_DISCONNECT = 0x00,
    /** Echo message.
     * Need to be consistent.
     */
    MSG_ECHO = 0x01,
    /** Response message.
     * Need to be consistent.
     */
    MSG_ECHO_RESPONSE = 0x02,
    /** Authentication.
     * Need to be consistent.
     */
    MSG_AUTH = 0x03,
    /** Authenication response.
     * Need to be consistent.
     */
    MSG_AUTH_RESPONSE = 0x04,
    /** Authentication status.
     * Need to be consistent.
     */
    MSG_AUTH_STATUS = 0x05,
    
    /** Plain string.
     */
    MSG_NOX_STR_CMD = 0x0A,
  };

  /** \brief Basic structure of message in \ref vigil::messenger.
   *
   * Copyright (C) Stanford University, 2008.
   * @author ykk
   * @date December 2008
   * @see messenger
   */
  struct messenger_msg
  {
    /** Length of message, including this header.
     */
    uint16_t length;
    /** Type of message, as defined in \ref msg_type.
     */
    uint8_t type;
    /** Reference to body of message.
     */
    uint8_t body[0];
  } __attribute__ ((packed));
  
  /** \ingroup noxevents
   * \brief Structure holding message to and from messenger.
   *
   * Copyright (C) Stanford University, 2008.
   * @author ykk
   * @date December 2008
   * @see messenger
   */
  struct Msg_event : public Event
  {
    /** Constructor.
     * Allocate memory for message.
     * @param message message
     * @param socket socket message is received with
     * @param size length of message received
     */
    Msg_event(messenger_msg* message, Msg_stream* socket, ssize_t size);

    /** Constructor.
     * Allocate memory for message.
     * @param cmsg core message to construct event from
     */
    Msg_event(const core_message* message);

    /** Destructor.
     */
    ~Msg_event();

    /** Empty constructor.
     * For use within python.
     */
    Msg_event() : Event(static_get_name()) 
    { }

    /** Static name required in NOX.
     */
    static const Event_name static_get_name() 
    {
      return "Msg_event";
    }

    /** Print array of bytes for debug.
     */
    void dumpBytes();

    /** Array reference to hold message.
     */
    messenger_msg* msg;
    /** Length of message.
     */
    ssize_t len;
    /** Memory allocated for message.
     */
    boost::shared_array<uint8_t> raw_msg;
    /** Reference to socket.
     */
    Msg_stream* sock;
  };   

  /** \ingroup noxcomponents
   * \brief Class through which to interact with NOX.
   *
   * In NOX, packets can be packed using Msg_stream.  Note that each 
   * component that sends messages via messenger should have an array
   * for storing the messages.
   *
   * TCP and SSL port can be changed at commandline using
   * tcpport and sslport arguments for messenger respectively. 
   * port 0 is interpreted as disabling the server socket.  
   * E.g.,
   * ./nox_core -i ptcp:6633 messenger=tcpport=11222,sslport=0
   * will run TCP server on port 11222 and SSL server will be disabled.
   *
   * Received messages are sent to other components via the Msg_event.
   * This allows extension of the messaging system, with no changes of
   * messenger.
   *
   * Copyright (C) Stanford University, 2009.
   * @author ykk
   * @date May 2009
   * @see messenger_core
   */
  class messenger : public message_processor
  {
  public:
    /** Constructor.
     * Start server socket.
     * @param c context as required by Component
     * @param node JSON object 
     */
    messenger(const Context* c, const json_object* node);

    /** Destructor.
     * Close server socket.
     */
    virtual ~messenger() 
    { };
    
    /** Configure component
     * Register events..
     * @param config configuration
     */
    void configure(const Configuration* config);

    /** Start component.
     * Create messenger_server and starts server thread.
     */
    void install();

    /** Get instance of messenger (for python)
     * @param ctxt context
     * @param scpa reference to return instance with
     */
    static void getInstance(const container::Context* ctxt, 
			    vigil::messenger*& scpa);

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
     * @param buf pointer to block received
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
    void reply_echo(const Msg_event& echoreq);

  private:
    /** Reference to messenger_core.
     */
    messenger_core* msg_core;
    /** Memory allocated for \ref vigil::bookman messages.
     */
    boost::shared_array<uint8_t> raw_msg;
    /** TCP port number.
     */
    uint16_t tcpport;
    /** SSL port number.
     */
    uint16_t sslport;
  };
}

#endif
