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

#include <string.h>

#include <stdint.h>
#include <cpqd.hh>
#include <cpqd_types.hh>
#include <RawMessage.hh>

RawMessage::RawMessage()
{

}
RawMessage::~RawMessage()
{

}

/**
 * Interprets a vector of bytes (whole message)
 * @param data Vector containing the data to be processed
 * @param size Data size to be considered
 */
t_result RawMessage::parseBytes(uint8_t data[], uint32_t size)
{
	int8_t ret = 0;

	ret = copyToRawData(data, size);

	return ret;
}

/**
 * Transforms a message represented by this object into a vector of bytes
 * @param data Vector in which the data will be copied to.
 * @param size Total size of the data
 */
void RawMessage::getBytes(uint8_t data[], uint32_t & size)
{
	::memcpy(data, vuiRawData, uiRawDataSize);
	size = uiRawDataSize;
}

/**
 * Return the size (in bytes) of this message (header + payload)
 */
uint32_t RawMessage::getSize()
{
	return uiRawDataSize;
}

ostream & operator<<(ostream & out, RawMessage & msg)
{
	uint8_t data[MAX_VECTOR_SIZE];
	uint32_t size = 0;
	memset(data, 0, MAX_VECTOR_SIZE);

	msg.getBytes(data, size);

	for (uint32_t i = 0; i < size; i++)
	{
		out << "[" << (data[i] < 16 ? "0" : "") << hex << (uint32_t) data[i] << "] ";
		if (((i % 16) == 0) && (i!=0))
		{
			out << endl;
		}
	}
	return out;
}
