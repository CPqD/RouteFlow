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

#include <stdlib.h>
#include <syslog.h>
#include <stdint.h>

#include "ControllerConnection.hh"
#include "../common/ipc/cpqd.hh"
#include "rfserver.hh"

ControllerConnection::ControllerConnection() {
	syslog(LOG_INFO, "[CLT] Starting controller connection...");
	answerComm = NULL;
	srand(time(NULL));

}

ControllerConnection::~ControllerConnection() {
	syslog(LOG_INFO, "[CLT] Closing communications...");
	if (controllerComm != NULL) {
		delete controllerComm;
	}
	if (answerComm != NULL) {
		delete answerComm;
	}
	controllerComm = NULL;
}

int32_t ControllerConnection::run(void * arg) {
	int8_t ret = 0;
	t_result ProcessRet = 0;

	RouteFlowServer * pQFSrv = (RouteFlowServer *) arg;

	while (ret == SUCCESS) {
		/*Receiving message from Controller Communicator.*/
		ret = controllerComm->recvMessage(message);

		if (ret == SUCCESS) {
			/*Processing the received message.*/
			ProcessRet = messageProcessor->processMessage(message, pQFSrv);
			if (ProcessRet == SUCCESS) {
				syslog(LOG_INFO,
						"[CONTROLLERCON] Message processed successfully");
			} else if (ProcessRet == ERROR_1) {
				/*Some error here. */
				syslog(LOG_ERR, "[CONTROLLERCON] Cannot identify message type");
			}
			/*Else if (ret == ERROR_UNKNOWN_COMMAND).*/
			else {
				syslog(LOG_ERR, "[CONTROLLERCON] Error while processing "
					"message");
			}
		} else if (ret != ERROR_2) {
			syslog(LOG_ERR, "[CONTROLLER] Error receiving/checking message %d",
					ret);

		} else {
			/* ret is a TCP socket timeout. Ignore it.*/
			ret = SUCCESS;
			continue;
		}
		usleep(100);
	}
	return 0;
}

t_result ControllerConnection::setMessageProcessor(MessageProcessor * proc) {
	t_result ret = 0;
	messageProcessor = proc;
	ret = messageProcessor->init();

	if (ret != SUCCESS) {
		syslog(LOG_ERR,
				"[CONTROLLERCON] Can't initialize message processor %d", ret);
	}

	return ret;
}
