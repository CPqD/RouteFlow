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

#ifndef CMD_MSG_H_
#define CMD_MSG_H_

#include "base_msg.h"
#include "openflow.h"

struct flow {
	uint64_t datapath_id;
	uint8_t flow_mod[MAX_PAYLOAD_SIZE - sizeof(datapath_id)];
};

struct send_packet {
	uint64_t datapath_id;
	uint16_t port_out;
	uint64_t pkt_id;
};

#endif /* CMD_MSG_H_ */

