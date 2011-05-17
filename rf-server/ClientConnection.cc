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
#include <cstring>

#include "ClientConnection.hh"
#include "rfserver.hh"

using std::memset;

int32_t ClientConnection::run(void * arg) {
	timeval t;
	uint8_t buffer[ETH_PKT_SIZE];
	fd_set rfds;
	int nfds;

	RouteFlowServer * pRFSrv = (RouteFlowServer *) arg;

	t.tv_sec = 0;
	t.tv_usec = 1000;
	while (1) {

		if (pRFSrv->m_vmList.size()) {
			FD_ZERO(&rfds);
			nfds = 0;
			std::list<RFVirtualMachine>::iterator iter;

			mutex.lock();
			for (iter = pRFSrv->m_vmList.begin(); iter
					!= pRFSrv->m_vmList.end(); iter++) {
				int fd = iter->getSockFd();
				if (fd > nfds) {
					nfds = fd;
				}

				FD_SET(fd, &rfds);
			}

			timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = 10;

			int rcount = select(nfds + 1, &rfds, NULL, NULL, &timeout);

			if (rcount > 0) {
				syslog(LOG_INFO,"[CLIENT CONNECTION] Found VM Message");
				for (iter = pRFSrv->m_vmList.begin(); iter
						!= pRFSrv->m_vmList.end(); iter++) {
					int fd = iter->getSockFd();
					if (FD_ISSET(fd, &rfds)) {
						std::memset(buffer, 0, ETH_PKT_SIZE);
						rcount = pRFSrv->recv_msg(fd, (RFMessage *) buffer,
								ETH_PKT_SIZE);
						syslog(LOG_INFO, "N entrei");
						if (rcount > 0) {
							syslog(LOG_INFO, "Entrei");
							pRFSrv->process_msg((RFMessage *) buffer);
						} else if (rcount == 0) {
							/* Client disconnected */
							syslog(LOG_INFO,
									"The VM %lld has been disconnected",
									iter->getVmId());
							if (iter->getMode() == RFP_VM_MODE_RUNNING) {
								syslog(LOG_DEBUG, "VM = %lld, DpId = %lld",
										iter->getVmId(), iter->getDpId().dpId);
								pRFSrv->send_flow_msg(iter->getDpId().dpId,
										CLEAR_FLOW_TABLE);
								pRFSrv->m_dpIdleList.push_back(iter->getDpId());
							}
							close(fd);
							iter = pRFSrv->m_vmList.erase(iter);
							if (iter == pRFSrv->m_vmList.end()) {
								break;
							}
						}
					}
				}
			}
			pRFSrv->unlockvmList();
		}
		mutex.unlock();

	}

	return 0;
}
