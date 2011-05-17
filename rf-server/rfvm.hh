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

#ifndef QFVM_HH_
#define QFVM_HH_

#include <netinet/in.h>
#include "rfprotocol.hh"

typedef struct Datapath {
	uint64_t dpId;
	uint8_t nports;
} Datapath_t;

class RFVirtualMachine
{
	uint32_t m_sockFd; /* File descriptor for server connection. */
	uint64_t m_vmId;   /* Number of the VM. */
	Datapath_t m_dpId; /* Id of the datapath associated with the VM. */
	uint32_t m_ipAddr; /* IP address of the VM configuration interface */
	uint32_t m_tcpPort; /* TCP port for server connection */
	RFVMMode m_mode;	/* Operation mode. */
	struct sockaddr_in m_vmAddr; /* For socket connection with server. */

public:
	/* Getters */
	uint64_t getVmId() const;
	const Datapath_t & getDpId() const;
	RFVMMode getMode() const;
	uint32_t getIpAddr() const;
	string getStrIpAddr() const;
	uint32_t getTcpPort() const;
	uint32_t getSockFd() const;
	const struct sockaddr_in * getVMAddr() const;

	/* Setters */
	int32_t setVmId(uint64_t vmId);
	int32_t setDpId(const Datapath_t & dpId);
	int32_t setMode(RFVMMode mode);
	int32_t setIpAddr(uint32_t ipAddr);
	int32_t setIpAddr(string ipAddr);
	int32_t setTcpPort(uint32_t tcpPort);
	int32_t setSockFd(uint32_t sockFd);
	int32_t setVMAddr(const struct sockaddr_in * addr);

};

#endif /* QFVM_HH_ */
