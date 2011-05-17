#ifndef MESSENGER_CORE_HH__
#define MESSENGER_CORE_HH__

/** Special Debug output for messenger_core (print byte by byte)
 */
#define MESSENGER_BYTE_DUMP false

/** Amount of buffer for each read in \ref vigil::messenger.
 */
#define MESSENGER_BUFFER_SIZE 512
/** Maximum length of a message in \ref vigil::messenger.
 */
#define MESSENGER_MAX_MSG_SIZE 3072
/** Maximum number of connections allowed in \ref vigil::messenger.
 */
#define MESSENGER_MAX_CONNECTION 10

#include "component.hh"
#include "ssl-socket.hh"
#include "tcp-socket.hh"
#include "threads/cooperative.hh"
#include <sys/time.h>
#include <boost/bind.hpp>
#include <boost/shared_array.hpp>

namespace vigil
{
  using namespace vigil::container; 

  /** \brief Async_stream for messenger.
   *
   * Include sending function for stream.
   * 
   * Copyright (C) Stanford University, 2009.
   * @author ykk
   * @date May 2009
   * @see Async_stream
   */
  class Msg_stream
  {
  public:
    /** Constructor.
     * @param stream_ reference to Async_stream
     */
    Msg_stream(Async_stream* stream_);

    /** Constructor.
     * @param stream_ reference to Msg_stream
     */
    Msg_stream(Msg_stream& stream_);

    /** Constructor.
     * @param stream_ reference to Async_stream
     * @param isSSL_ indicate if Async_stream is SSL
     */
    Msg_stream(Async_stream* stream_, bool isSSL_);

    /** Destructor.
     */
    ~Msg_stream()
    { ; }

    /** Initialize packet with length and type.
     * @param msg_raw message buffer reference
     * @param size size of buffer to allocate
     */
    void init(boost::shared_array<uint8_t>& msg_raw, 
		     ssize_t size) const;

    /** Initialize JSON string with type and length.
     * @param msg_raw message buffer reference
     * @param str string to contain
     * @param size size of buffer to allocate (if 0, use min to contain string)
     * @param addbraces indidcate if braces should be added
     */
    void init(boost::shared_array<uint8_t>& msg_raw, const char* str, 
		     ssize_t size=0, bool addbraces=true) const;

    /** Send packet on given socket.
     * @param msg message buffer reference
     * @param size size of messages (if 0, assume length-type)
     */
    void send(boost::shared_array<uint8_t>& msg, 
		     ssize_t size=0) const;

    /** Send packet on given socket.
     * @param str string to send
     */
    void send(const std::string& str) const;

    /** Reference to Async
     */
    Async_stream* stream;
    /** Indicate if socket is SSL, else is TCP.
     */
    bool isSSL;
    /** Reference to magic item tagged with stream
     */
    void* magic;
  private:
  };

  /** \brief Structure holding message to and from messenger_core.
   *
   * Copyright (C) Stanford University, 2010.
   * @author ykk
   * @date May 2010
   * @see messenger_core
   */
  struct core_message
  {
    /** Constructor.
     * Allocate memory for message.
     * @param message message
     * @param socket socket message is received with
     * @param size length of message received
     */
    core_message(uint8_t* message, Msg_stream* socket, ssize_t size);

    /** Empty Constructor.
     * Allocate memory for message.
     * @param socket socket message is received with
     */
    core_message(Msg_stream* socket);

    /** Destructor.
     */
    ~core_message();

    /** Print array of bytes for debug.
     */
    void dumpBytes();

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
  
  /** Messenger processing class.
   *
   * Can be inherited by multiple classes to have different
   * server sockets listening on different ports.
   * 
   * Copyright (C) Stanford University, 2009.
   * @author ykk
   * @date May 2009
   * @see messenger
   */
  class message_processor: public Component
  {
  public:
    /** \brief Code for special event
     */
    enum message_code
    {
      msg_code_normal,
      msg_code_new_connection,
      msg_code_disconnection,
    };

    /** Constructor.
     * @param c context as required by Component
     * @param node JSON object
     */
    message_processor(const Context* c, const json_object* node): 
      Component(c)
    { };

    /** Function to do processing for block received.
     * @param buf pointer to block received
     * @param dataSize size of block
     * @param data pointer to current message data (block not added)
     * @param currSize size of current message
     * @param sock reference to socket
     * @return length to copy to current message data
     */
    virtual ssize_t processBlock(uint8_t* buf, ssize_t& dataSize,
				 uint8_t* data, ssize_t currSize, 
				 Msg_stream* sock)
    { return 0; };

    /** Function to determine message is completed.
     * @param data pointer to current message data (block not added)
     * @param currSize size of current message
     * @param sock reference to message stream
     * @return if message is completed (i.e., can be posted)
     */
    virtual bool msg_complete(uint8_t* data, ssize_t currSize,
			      Msg_stream* sock)
    { return false; };

    /** Function to do processing for messages received.
     *
     * @see #message_code
     * @param msg message event for message received
     * @param code code for special events
     */
    virtual void process(const core_message* msg, int code=0)
    {};

    /** Send echo request.
     * @param sock socket to send echo request over
     */
    virtual void send_echo(Msg_stream* sock)
    {};

    /** Periodic check interval (in terms idle time, in sec)
     * Zero indicate, do not check.
     */
    uint16_t idleInterval;
    /** Threshold echo missed before terminated connection.
     */
    uint8_t thresholdEchoMissed;
    /** Send message event upon new connection.
     */
    bool newConnectionMsg;
  };

  /** \brief Core of message interaction with NOX.
   *
   * The class allows messages to be sent and receive on the datapath
   * in NOX.  TCP and SSL sockets can be used.  Binary messages with
   * messenger_msg as the header are used in this communication.
   *
   * All messenger or communication component must not use type 0x00 to 0x09
   * as there are reserved for messenger_core's internal use.  Further,
   * echo response (type 0x02) has to be implemented for end-to-end check
   * with the component.
   *
   * Copyright (C) Stanford University, 2008.
   * @author ykk
   * @date December 2008
   * @see messenger_server
   */
  class messenger_core : public Component
  {
  public:
    /** Constructor.
     * Start server socket.
     * @param c context as required by Component
     * @param node JSON object
     */
    messenger_core(const Context* c, const json_object* node): 
      Component(c)
    { };

    /** Destructor.
     * Close server socket.
     */
    virtual ~messenger_core() 
    { };
    
    /** Configure component
     * Register events..
     * @param config configuration
     */
    void configure(const Configuration* config);

    /** Start component.
     */
    void install()
    { };

    /** Start TCP server.
     * @param messenger reference to message processor
     * @param portNo port number to listen to
     * @see messenger_server
     */
    void start_tcp(message_processor* messenger, uint16_t portNo);

    /** Start SSL server.
     * @param messenger reference to message processor
     * @param config SSL configuration
     * @param portNo port number to listen to
     * @see messenger_ssl_server
     */
    void start_ssl(message_processor* messenger, uint16_t portNo, 
		   boost::shared_ptr<Ssl_config>& config);

    /** Get instance of messenger_core (for python)
     * @param ctxt context
     * @param scpa reference to return instance with
     */
    static void getInstance(const container::Context* ctxt, 
			    vigil::messenger_core*& scpa);

  private:
  }; 

  /** \brief Class to accept TCP connections to the messenger.
   *
   * Copyright (C) Stanford University, 2008.
   * @author ykk
   * @date December 2008
   * @see messenger_tcp_connection
   */
  class messenger_server: Co_thread
  {
  public:
    /** Constructor.
     * Open server socket for messages from messenger.
     * @param messenger reference to messenger
     * @param portNo port number to connect to
     */
    messenger_server(message_processor* messenger, uint16_t portNo);

    /** Destructor.
     */
    ~messenger_server();

    /** Main function to run socket server as a thread.
     */
    void run();

  private:
    /** TCP server socket accepting connection.
     */
    Tcp_socket server_sock;
    /** Reference to messenger.
     */
    message_processor* msger;
  };

  /** \brief Class to accept SSL connections to the messenger.
   *
   * Copyright (C) Stanford University, 2008.
   * @author ykk
   * @date December 2008
   * @see messenger_ssl_connection
   */
  class messenger_ssl_server: Co_thread
  {
  public:
    /** Constructor.
     * Open server socket for messages from messenger.
     * @param messenger reference to messenger
     * @param portNo port number to connect to
     * @param config SSL configuration
     */
    messenger_ssl_server(message_processor* messenger, uint16_t portNo,
			 boost::shared_ptr<Ssl_config>& config);

    /** Destructor.
     */
    ~messenger_ssl_server();

    /** Main function to run socket server as a thread.
     */
    void run();

  private:
    /** SSL server socket accepting connection.
     */
    Ssl_socket server_sock;
    /** Reference to messenger.
     */
    message_processor* msger;
  };

  /** \brief Class to handle a connection from the messenger.
   *
   * Copyright (C) Stanford University, 2008.
   * @author ykk
   * @date December 2008
   */
  class messenger_connection
  {
  public:
    /** Constructor
     * @param messenger reference to message processor
     */
    messenger_connection(message_processor* messenger);

  protected:
    /** Function to processing block of data received.
     * @param buf reference to buffer used to receive block
     * @param dataSize size of data received
     * @param sock socket reference
     */
    void processBlock(Array_buffer& buf, ssize_t& dataSize, Msg_stream* sock);

    /** Function to check for disconnect messages.
     * @param msg message event for message received
     * @param code code for special event
     */
    void process(const core_message* msg, int code=0);

    /** Post disconnection message
     * @param sock socket reference
     */
    void post_disconnect(Msg_stream* sock);

    /** Check idle time.
     */
    void check_idle();

    /** Send message for new connection.
     * @param sock socket reference
     */
    void send_new_connection_msg(Msg_stream* sock);

    /** Internal buffer for message.
     * @see MESSENGER_MAX_MSG_SIZE
     */
    uint8_t internalrecvbuf[MESSENGER_MAX_MSG_SIZE];
    /** Pointer to current end message buffer.
     * @see internalrecvbuf
     */
    uint8_t* endpointer;
    /** Current size of message.
     */
    ssize_t currSize;
    /** Reference to messenger.
     */
    message_processor* msger;
    /** Reference to Msg_stream
     */
    Msg_stream* msgstream;
    /** Indicate if thread should continue to run.
     */
    bool running;
    /** Last active time of connection.
     */
    time_t lastActiveTime;
    /** Echo missed.
     */
    uint8_t echoMissed;
  };

  /** \brief Class to handle a TCP connection from the messenger.
   *
   * Copyright (C) Stanford University, 2008.
   * @author ykk
   * @date December 2008
   */
  class messenger_tcp_connection: messenger_connection, Co_thread
  {
  public:
    /** Constructor.
     * Starts thread that handles messages from new socket accepted.
     * @param messenger reference to messenger
     * @param new_socket new socket accepted.
     */
    messenger_tcp_connection(message_processor* messenger, 
			     std::auto_ptr<Tcp_socket> new_socket);
    /** Destructor.
     */
    ~messenger_tcp_connection()
    { 
      delete msgstream; 
    }

    /** Thread that listens to socket for packets.
     * Sends each message for processing.
     * Closes socket once terminated via message.
     * @see process(const uint8_t* msg, int code)
     */
    void run();

  private:
    /** Reference to accepted socket.
     */
    std::auto_ptr<Tcp_socket> sock;
  };

  /** \brief Class to handle a SSL connection from the messenger.
   *
   * Copyright (C) Stanford University, 2008.
   * @author ykk
   * @date December 2008
   */
  class messenger_ssl_connection: messenger_connection, Co_thread
  {
  public:
    /** Constructor.
     * Starts thread that handles messages from new socket accepted.
     * @param messenger reference to messenger
     * @param new_socket new socket accepted.
     */
    messenger_ssl_connection(message_processor* messenger, 
			     std::auto_ptr<Ssl_socket> new_socket);
    /** Destructor.
     */
    ~messenger_ssl_connection()
    { 
      delete msgstream;
    }
    /** Thread that listens to socket for packets.
     * Sends each message for processing.
     * Closes socket once terminated via message.
     * @see process(const uint8_t* msg, int code)
     */
    void run();

  private:
    /** Reference to accepted socket.
     */
    std::auto_ptr<Ssl_socket> sock;
  };

} // namespace vigil

#endif 
