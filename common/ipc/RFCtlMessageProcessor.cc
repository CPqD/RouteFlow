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

#include <syslog.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "RFCtlMessageProcessor.hh"
#include "IPCMessage.hh"
#include "RawMessage.hh"
#include "../../include/cmd_msg.h"
#include "../../include/event_msg.h"
#include "../../include/openflow.h"
#include "../../rf-server/rfserver.hh"

t_result RFCtlMessageProcessor::processMessage(IPCMessage * msg, void * arg) {

	syslog(LOG_INFO, "[RFMP] Processing Message");

	t_result ret = SUCCESS;
	base_msg BaseMessage;

	msg = (RawMessage *) msg;

	uint32_t size = sizeof(base_msg);
	bzero(&BaseMessage, size);
	msg->getBytes(&BaseMessage.group, size);

	RouteFlowServer * pQFSrv = (RouteFlowServer *) arg;
	if (BaseMessage.group == EVENT) {

		if (BaseMessage.type == PACKET_IN) {
			packet_in MsgEvent;
			::memcpy(&MsgEvent, &BaseMessage.payload, sizeof(MsgEvent));
			pQFSrv->packet_in_event(MsgEvent.datapath_id, MsgEvent.port_in,
					MsgEvent.pkt_id, 0, MsgEvent.type);
		}
		else if (BaseMessage.type == DATAPATH_JOIN) {
			datapath_join MsgEvent;
			::memcpy(&MsgEvent, &BaseMessage.payload, BaseMessage.pay_size);
			pQFSrv->datapath_join_event(MsgEvent.datapath_id, MsgEvent.hw_desc,
					MsgEvent.no_ports);

		}
		else if (BaseMessage.type == DATAPATH_LEAVE) {
			datapath_leave MsgEvent;
			::memcpy(&MsgEvent, &BaseMessage.payload, sizeof(MsgEvent));
			pQFSrv->datapath_leave_event(MsgEvent.datapath_id);
		}
		else if (BaseMessage.type == LINK_EVENT) {
			link_event MsgEvent;
			::memcpy(&MsgEvent, &BaseMessage.payload, BaseMessage.pay_size);
			pQFSrv->handle_link_event(MsgEvent.reason, MsgEvent.dp1,
					MsgEvent.port_1, MsgEvent.dp2, MsgEvent.port_2);

		}
		else if (BaseMessage.type == MAP_EVENT) {
			VmtoOvs MsgMapEvent;
			::memcpy(&MsgMapEvent, &BaseMessage.payload, sizeof(MsgMapEvent));
			pQFSrv->VmToOvsMapping(MsgMapEvent.vmId, MsgMapEvent.VmPort,
					MsgMapEvent.OvsPort);
		}
	} else
		ret = ERROR_1;

	return ret;
}

t_result RFCtlMessageProcessor::init() {

	syslog(LOG_INFO, "[RFMP] Initializing Message Processor");

	t_result ret = SUCCESS;

	return ret;
}
