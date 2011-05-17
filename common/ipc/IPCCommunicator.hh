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


#ifndef IPCCOMMUNICATOR_H_
#define IPCCOMMUNICATOR_H_

#include "cpqd.hh"
#include "IPCMessage.hh"

/**
 * @defgroup Communicators Communicators
 * This module contains classes for interprocess communications.
 * The design of communicators classes and messages considered the following
 * points:
 * -# A message class could be used by any communicator class.
 * -# The communicator class must not know anything about message encoding.
 * -# The message must not rely on any particularity of a communicator class.
 *
 * But, the most important thing about these communicator messages is:
 * <h1>They are supposed to send and receive messages only! </h1> although it
 * can also handle its file descriptor, for instance.
 * This means:
 *  - <b> No message processing should be done by any of these classes; </b>
 *  - <b> No verifications on message data should be performed, except the presence
 *  or not of a message header. This is necessary in order to check whether the
 *  message should parse its header or not.</b>
 * @{
 *
 * @class IPCCommunicator
 * Abstract class for communications.
 */
class IPCCommunicator {
public:
	/** Default constructor. */
	IPCCommunicator();
	/** Destructor. */
	virtual ~IPCCommunicator() = 0;

	/**
	 * Opens the communication channel.
	 * This class doesn't define what exactly is a communication channel. For
	 * Linux systems, for instance, it can be a file descriptor for a socket.
	 * @return It's up to the child class to define what can be return by open()
	 * method. It's advisable that this error code can be decoded by using
	 * ErrorHandler::dumpError().
	 */
	virtual t_result open() = 0;

	/**
	 * Closes the communication channel.
	 * This class doesn't define what exactly is a communication channel. For
	 * Linux systems, for instance, it can be a file descriptor for a socket.
	 * @return It's up to the child class to define what can be return by open()
	 * method. It's advisable that this error code can be decoded by using
	 * ErrorHandler::dumpError().
	 */
	virtual t_result close() = 0;

	/**
	 * Reopens the communication channel.
	 * This class doesn't define what exactly is a communication channel. For
	 * Linux systems, for instance, it can be a file descriptor for a socket.
	 * This method can close and reopen the communication channel
	 * @return It's up to the child class to define what can be return by open()
	 * method. It's advisable that this error code can be decoded by using
	 * ErrorHandler::dumpError().
	 */
	virtual t_result reopen() = 0;

	/**
	 * Receives a message.
	 * @param msg The message which will hold the received data. This pointer
	 * should be already allocated. This pointer doesn't belong to this class,
	 * so it's up to the user to release its memory afterwards.
	 */
	virtual t_result recvMessage(IPCMessage *msg) = 0;

	/**
	 * Sends a message.
	 * @param msg The message which will hold the received data. This pointer
	 * should be already allocated. This pointer doesn't belong to this class,
	 * so it's up to the user to release its memory afterwards.
	 */
	virtual t_result sendMessage(IPCMessage *msg) = 0;
};

/**
 * @}
 */

#endif /* IPCCOMMUNICATOR_H_ */
