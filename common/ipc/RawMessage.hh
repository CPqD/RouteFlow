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

#ifndef RAWMESSAGE_H_
#define RAWMESSAGE_H_

#include "IPCMessage.hh"
#include <iostream>
using namespace std;

/**
 * @addtogroup Messages
 * @{
 *
 * @class RawMessage
 * This class defines a special type of message: it has no header, its payload
 * is not composed by data structures, but only by bytes.
 */
class RawMessage : public IPCMessage {
protected:

public:

	/**
	 * Default constructor
	 */
	RawMessage();

	/**
	 * Destructor
	 */
	virtual ~RawMessage();

	/**
	 * Returns the header total size
	 */
	uint32_t getHeaderSize() { return 0; };

	/**
	 * Interprets a vector of bytes representing a message header.
	 */
	t_result parseHeaderBytes(uint8_t data[]) { return 0; };

	/**
	 * Interprets a vector of bytes representing a payload.
	 * @param data Vector containing the data to be processed
	 */
	t_result parsePayloadBytes(uint8_t data[]) { return 0; };

	/**
	 * Interprets a vector of bytes (whole message).
	 * @param data Vector containing the data to be processed
	 * @param size Data size to be considered
	 */
	t_result parseBytes(uint8_t data[], uint32_t size);

	bool hasHeader() { return false; };

	/**
	 * Transforms a message represented by this object into a vector of bytes.
	 * @param data Vector in which the data will be copied to.
	 * @param size Total size of the data
	 */
	void getBytes(uint8_t data[], uint32_t & size);

	/**
	 * Return the size (in bytes) of payload.
	 */
	uint32_t getPayloadSize() { return 0; };

	/**
	 * Return the size (in bytes) of this message (header + payload).
	 */
	uint32_t getSize();


	friend ostream & operator<<(ostream & out, RawMessage & msg);
};

/**
 * @}
 */

#endif /* RAWMESSAGE_H_ */
