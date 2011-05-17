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

#ifndef MESSAGEPROCESSOR_H_
#define MESSAGEPROCESSOR_H_

#include <stdint.h>
#include "IPCMessage.hh"
#include "cpqd_types.hh"
#include "../sys/Semaphore.hh"
#include <map>

//#include "../../qf-server/qfserver.hh"
using namespace std;

/**
 * Function executed when some asynchronous event occurs
 */
typedef void * (*processResult)(void *);

/**
 * Structure for callback procedures
 */
typedef struct tag_Callback
{
	processResult callbackOK; ///< Function to be executed when an event occurs
	processResult callbackError; ///< Function to be executed when an event occurs
	uint16_t resultCallback; ///< Result of the execution of callback function
	uint16_t errorCode; ///< Error code for a request.
	Semaphore * semaphore; ///< Semaphore related to the event waiter (can be NULL)
} ttag_Callback, * ptag_Callback;


/**
 * Abstract class for message processors.
 * Child classes should implement these methods in order to process messages
 * (check which type a message is and take the proper action). Remember that
 * message reception, message checking, and actual actions taken are not
 * supposed to be done by this message.
 */
class MessageProcessor {
protected:
	/**
	 * This static member holds information about scheduled asynchronous
	 * callbacks. For example, think about when a command should be sent to an
	 * equipment, but the answer comes in through other communication channel.
	 * The message sender cannot just sit and wait for an answer to be received
	 * by his socket, once all commmunication is received by other
	 * IPCCommunicator object. Instead, it sets ptag_Callback.fnc (stands for
	 * 'function') member structure to process the answer, adds a new entry
	 * to this attribute and goes on. When the message is received, it should
	 * be recognized as an 'answer' by MessageProcessor child class, and the
	 * function previously defined is called in order to complete the message
	 * processing.
	 * If this operation is blocking (the answer is really important to the
	 * remaining operations), the message sender could set the
	 * ttag_Callback.lock attribute, and, obviously, acquire the lock ( using
	 * lock->lock() method) before going on.
	 */
	static map<uint16_t, ptag_Callback> resultFunctions;

public:
	virtual ~MessageProcessor() {};

	/**
	 * Process a message received by some communicator.
	 * This is the main method for this class. It's supposed to know how to
	 * process a message received by a communicator and start the appropriate
	 * procedures in order to do that.
	 * @param msg Reference to the received message
	 * @return Should return an error code that can be decoded by ErrorHandler::dumpError()
	 */
	virtual t_result processMessage(IPCMessage * msg, void * arg) = 0;

	/**
	 * Initializes the message processor.
	 * This method is needed if some procedure must be performed before starting
	 * the message reception and processing. Otherwise, the implementation of
	 * this method can be empty.
	 * @return Should return an error code that can be decoded by ErrorHandler::dumpError()
	 */
	virtual t_result init() = 0;

	/**
	 * Adds a new callback handler
	 */
	static void addResultCallback(ptag_Callback callback, uint16_t msgID);



	/**
	 * Removes an existing callback handler.
	 * Once the message processing is done, this method SHOULD be called.
	 * Afterwards, the lock attribute should be deallocated!.
	 */
	static void removeResultCallback(uint16_t msgID);

};

#endif /* MESSAGEPROCESSOR_H_ */
