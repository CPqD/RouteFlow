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

#ifndef CONTROLLERCONNECTION_H_
#define CONTROLLERCONNECTION_H_

#include <list>
#include <stdint.h>

#include "../common/sys/Thread.hh"
#include "../common/sys/Mutex.hh"
#include "../common/ipc/MessageProcessor.hh"
#include "../common/ipc/IPCCommunicator.hh"
#include "../common/ipc/IPCMessage.hh"
#include "../common/ipc/RawMessage.hh"
#include "cpqd_types.hh"

class ClientConnectionHelper;

#define CLNCNX_THREAD_POOL_SIZE 2

/**
 * Class which handles controller connections.
 */
class ControllerConnection: public Thread {
protected:
	/**
	 * Communicator used for messages exchange
	 * Once set, this attribute must not be deallocated, except when destroying
	 * this object (from ClientConnection).
	 */
	IPCCommunicator * controllerComm;

	/**
	 * Communicator used to send answer messages.
	 */
	IPCCommunicator * answerComm;

	/**
	 * Message used for parsing data
	 */
	RawMessage * message;

	/**
	 * Message processor used.
	 * This processor is called whenever a message is received.
	 */
	MessageProcessor * messageProcessor;

public:

	/**
	 * Default constructor.
	 * For proper usage of this class, clientComm attribute must be set.
	 */
	ControllerConnection();

	/**
	 * Destructor.
	 * This destructor deallocates clientComm object.
	 */
	virtual ~ControllerConnection();

	/**
	 * Configures the communication method used for messages exchange.
	 */
	void setCommunicator(IPCCommunicator * comm) {
		controllerComm = comm;
	}

	/*
	 * Returns
	 */
	void * getCommunicator() {
		return controllerComm;
	}

	/**
	 * Configures communicator for answers
	 * This method is useful when the communicator used to receive the request
	 * is not the same as the one to send the answers.
	 * @param answ Reference to a communicator which will send all the answers
	 */
	void setAnswerCommunicator(IPCCommunicator * answ) {
		answerComm = answ;
	}

	/**
	 * Sets the default message.
	 * The default message is used to parse received bytes.
	 * @param msg Reference to an empty message to be set as default
	 */
	void setMessage(IPCMessage * msg) {
		message = dynamic_cast<RawMessage *> (msg);
	}
	;

	/**
	 * Sets the default message processor.
	 * The message processor will read the contents of the default message (set
	 * by setMessage(IPCMessage *)) method and then process it.
	 * @param proc Reference to an object of MessageProcessor's child class.
	 */
	t_result setMessageProcessor(MessageProcessor * proc);

	/**
	 * Entry point.
	 * This is the main method of this class, and should only be used within a
	 * thread.
	 */
	int32_t run(void * arg);

};

#endif /* CONTROLLERCONNECTION_H_ */
