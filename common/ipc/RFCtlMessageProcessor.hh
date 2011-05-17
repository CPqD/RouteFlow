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

#ifndef RFMESSAGEPROCESSOR_HH_
#define RFMESSAGEPROCESSOR_HH_

#include "MessageProcessor.hh"
#include "../../rf-server/rfserver.hh"


class RFCtlMessageProcessor : public MessageProcessor {

public:
	/**
	 * Process a message received by some communicator.
	 * This is the main method for this class. It's supposed to know how to
	 * process a message received by a communicator and start the appropriate
	 * procedures in order to do that.
	 * @param msg Reference to the received message
	 * @return Should return an error code that can be decoded by ErrorHandler::dumpError()
	 */
	t_result processMessage(IPCMessage * msg, void * arg);

	/**
	 * Initializes the message processor.
	 * This method is needed if some procedure must be performed before starting
	 * the message reception and processing. Otherwise, the implementation of
	 * this method can be empty.
	 * @return Should return an error code that can be decoded by ErrorHandler::dumpError()
	 */
	t_result init();
};


#endif /* RFMESSAGEPROCESSOR_HH_ */
