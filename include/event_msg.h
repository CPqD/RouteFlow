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

#ifndef EVENT_MSG_H_
#define EVENT_MSG_H_

#include<string>
#include "base_msg.h"
#include <string>

using std::string;

using std::string;

enum action {
	ADD, REMOVE
};

struct packet_in {
	uint64_t datapath_id;
	uint16_t port_in;
	uint64_t pkt_id;
	uint32_t type;
};

struct datapath_join {
	uint64_t datapath_id;
	uint32_t no_ports;
	char hw_desc[100];

};

struct datapath_leave {
	uint64_t datapath_id;
};

struct link_event {
	uint8_t reason;
	uint64_t dp1;
	uint16_t port_1;
	uint64_t dp2;
	uint16_t port_2;
};

struct VmtoOvs {
	uint64_t vmId;
	uint16_t VmPort;
	uint16_t OvsPort;
};

#endif /* EVENT_MSG_H_ */

