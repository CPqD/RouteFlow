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

#include <stdint.h>
#include <cpqd_types.hh>
#include <cpqd.hh>
#include <MessageProcessor.hh>

map<uint16_t, ptag_Callback> MessageProcessor::resultFunctions;

void MessageProcessor::addResultCallback(ptag_Callback callback, uint16_t msgID)
{
	resultFunctions[msgID] = callback;
}

void MessageProcessor::removeResultCallback(uint16_t msgID)
{
	ptag_Callback ptagCurrCbk = NULL;
	map<uint16_t, ptag_Callback>::iterator it;
	if ((it = resultFunctions.find(msgID)) != resultFunctions.end())
	{
		ptagCurrCbk = (*it).second;
		resultFunctions.erase(it);
	}
}


