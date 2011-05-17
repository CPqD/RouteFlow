/*
 *	Copyright 2011 Fundação CPqD
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *	 you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *	 Unless required by applicable law or agreed to in writing, software
 *	 distributed under the License is distributed on an "AS IS" BASIS,
 *	 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#ifndef TCPSOCKET_H_
#define TCPSOCKET_H_

#include "IPCCommunicator.hh"

/**
 * @addtogroup Communicators
 * @{
 *
 * @class TCPSocket
 * Class used for interprocess communication using TCP.
 * The usage flow is:
 * - call the constructor (TCPSocket())
 * - set the file descriptor by using setFD(uint32_t) or open() methods.
 * - if the socket was opened through open(), it must be connected to some
 * endpoint. This can be done by connect(char[], uint32_t) method;
 * -
 *
 */
class TCPSocket : public IPCCommunicator {
protected:

	/**
	 * File descriptor of the opened socket
	 */
	int32_t uiSockFD;

	/**
	 * Check whether the socket is still valid
	 */
	static t_result checkSocket(uint32_t fd);

	ttag_Address connectedIP;

	/*
	 * Socket used to connect
	 */

	sockaddr_in sock;

public:

	/**
	 * Default constructor.
	 * In this constructor, no attempt is made to build a socket. This must be
	 * done later, using 'openSocket', 'connect', and 'recvMessage' and
	 * 'sendMessage' methods.
	 */
	TCPSocket();

	/**
	 * Destructor.
	 * When deleting an object of this class, the socket, if exists, is closed.
	 */
	virtual ~TCPSocket();

	/**
	 * Creates a socket.
	 * This brand new socket isn't bound to any port or address. This task
	 * is done by the 'connect' method.
	 * @return The error returned by the 'socket' function. (Check your operating
	 * system's manual).
	 */
	t_result open();

	/**
	 * Closes the socket
	 */
	t_result close();

	/**
	 * Reopens the socket, but it doesn't connect to any host (as previously
	 * configured)
	 */
	t_result reopen();

	/**
	 * Configures the file descriptor related to the socket.
	 * Before assigning the new file descriptor, a test is made in order to check
	 * the validity of the file descriptor. If it is valid, then this object will
	 * use the given file descriptor for further communication.
	 * @param fd The new file descriptor
	 * @return The state of the file descriptor
	 */
	t_result setFD(uint32_t fd);

	/**
	 * Connects the socket to a specific remote address and port.
	 * @param address Remote address
	 * @param port Remote port
	 * @return The error returnd by the 'connect' function. (Check your operating
	 * system's manual)
	 */
	t_result connect(ttag_Address address, uint32_t port);


	/**
	 * Receives a message from the opened socket.
	 * @param msg The message which will hold the received data. This pointer
	 * should be already allocated. This pointer doesn't belong to this class,
	 * so it's up to the user to release its memory afterwards.
	 */
	t_result recvMessage(IPCMessage *msg);

	/**
	 * Sends a message through the socket.
	 * @param msg The message which will hold the received data. This pointer
	 * should be already allocated. This pointer doesn't belong to this class,
	 * so it's up to the user to release its memory afterwards.
	 */
	t_result sendMessage(IPCMessage *msg);

	ttag_Address getConnectedIP() { return connectedIP; };

	t_result reconnect();
};

/**
 * @}
 */

#endif /* TCPSOCKET_H_ */
