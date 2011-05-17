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

#ifndef IPCMESSAGE_H_
#define IPCMESSAGE_H_

#include <stdint.h>
#include "cpqd.hh"

/**
 * @defgroup Messages Messages
 * @{
 *
 * @class IPCMessage
 *
 * Class for message abstraction.
 * This class abstracts a generic message used by any protocol. There are some
 * details that should be understood:
 * -# This is a generic message. While specifying it, many aspects of messages
 * were taken into account, such as: should this class represents only protocol
 * messages (including header, etc.) or it should consider also plain messages
 * (such as text messages, with no header)? The latter option was chosen, once
 * there are some parts of ONCS that use headerless messages (such as commands
 * sent to Marben's stack through StackOperator class - check out the
 * documentation of mpa_oncs project).
 * -# All messages should contain its own data, i.e., there should not be any
 * pointers to external data. Hence, this class can handle its own memory
 * operations and verifications. Also, no referece from internal data should be
 * passed to external elements, in order to avoid memory corruption.
 * -# The message class should know how to convert a byte vector to its format
 * and vice-versa.
 *
 * The usage flow for this message is:
 * - Building a new message from stratch:
 *   - Call the constructor ( IPCMessage() ). This should be done while building
 *     a new message from a child class;
 *   - Fill the message header - if the message has a header :D (this is
 *     meaningful for child classes);
 *   - Add content to message payload;
 *   - Transform the message into a byte vector.
 *   - And finally, send the byte vector through a communication channel.
 */
class IPCMessage {
protected:

	/**
	 * Message raw data.
	 * All data - and I mean *every* bit - of this message should be copied to
	 * this vector. No reference to it should be passed as a reference to other
	 * elements, in order to prevent memory corruption.
	 */
	uint8_t vuiRawData [MAX_VECTOR_SIZE];

	/**
	 * Current used size of raw data.
	 */
	uint32_t uiRawDataSize;

	/**
	 * Copies a vector to message raw data.
	 * This method copies a vector to the raw data byte vector, starting at the
	 * beginning (there's no offset).
	 * @param data The vector to be copied
	 * @param size How many bytes from data[] should be copied to raw data
	 * byte vector
	 * @return Error code (this value can be decoded using ErrorHandler::dumpError())
	 */
	int8_t copyToRawData(uint8_t data[], uint32_t size);
public:

	/**
	 * Default constructor.
	 * This contructor zeroes vuiRawData vector.
	 */
	IPCMessage();

	/**
	 * Destructor.
	 * This contructor zeroes vuiRawData vector. So, if there's a reference to
	 * some data inside this vector, which should be avoided, it will be invalid
	 * after destroying the message.
	 */
	virtual ~IPCMessage() = 0;

	/**
	 * Returns the header total size.
	 * This method should return a value different than zero iff the message has
	 * a header.
	 * @return Header size, 0 if the message has no header.
	 */
	virtual uint32_t getHeaderSize() = 0;

	/**
	 * Interprets a vector of bytes representing a message header.
	 */
	virtual t_result parseHeaderBytes(uint8_t data []) = 0;

	/**
	 * Interprets a vector of bytes representing a payload
	 * @param data Vector containing the data to be processed.
	 */
	virtual t_result parsePayloadBytes(uint8_t data []) = 0;

	/**
	 * Interprets a vector of bytes (whole message)
	 * @param data Vector containing the data to be processed
	 * @param size Data size to be considered
	 */
	virtual t_result parseBytes(uint8_t data [], uint32_t size) = 0;

	/**
	 * Check if the message has a header.
	 * @return true if the message has a header (protocol messages) or not (text
	 * messages)
	 */
	virtual bool hasHeader() = 0;

	/**
	 * Transforms a message represented by this object into a vector of bytes
	 * @param data Vector in which the data will be copied to.
	 * @param size Total size of the data
	 */
	virtual void getBytes(uint8_t data[], uint32_t & size) = 0;

	/**
	 * Return the size (in bytes) of payload
	 */
	virtual uint32_t getPayloadSize() = 0;

	/**
	 * Return the size (in bytes) of this message (header + payload)
	 */
	virtual uint32_t getSize() = 0;

};

/**
 * @}
 */

#endif /* IPCMESSAGE_H_ */
