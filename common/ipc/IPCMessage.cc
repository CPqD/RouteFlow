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

#include "IPCMessage.hh"


IPCMessage::IPCMessage() {
	::memset(vuiRawData, 0, MAX_VECTOR_SIZE);
	uiRawDataSize = 0;
}

IPCMessage::~IPCMessage() {
	::memset(vuiRawData, 0, MAX_VECTOR_SIZE);
	uiRawDataSize = 0;
}

int8_t IPCMessage::copyToRawData(uint8_t data[], uint32_t size)
{
	int8_t ret = 0;

	::memset(vuiRawData, 0, MAX_VECTOR_SIZE);
	::memcpy(vuiRawData, data, size);
	uiRawDataSize = size;

	return ret;
}
