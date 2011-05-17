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

#include "rfvm.hh"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

/* Getters */
uint64_t RFVirtualMachine::getVmId() const
{
	return this->m_vmId;
}

const Datapath_t & RFVirtualMachine::getDpId() const
{
	return (const Datapath_t &)this->m_dpId;
}

RFVMMode RFVirtualMachine::getMode() const
{
	return this->m_mode;
}

uint32_t RFVirtualMachine::getIpAddr() const
{
	return this->m_ipAddr;
}

string RFVirtualMachine::getStrIpAddr() const
{
	struct in_addr addr;

	addr.s_addr = this->m_ipAddr;
	string strIP(inet_ntoa(addr));

	return strIP;
}

uint32_t RFVirtualMachine::getTcpPort() const
{
	return this->m_tcpPort;
}

uint32_t RFVirtualMachine::getSockFd() const
{
	return this->m_sockFd;
}

const struct sockaddr_in * RFVirtualMachine::getVMAddr() const
{
	return (const struct sockaddr_in *) &this->m_vmAddr;
}

/* Setters */
int32_t RFVirtualMachine::setVmId(uint64_t vmId)
{
	this->m_vmId = vmId;

	return 0;
}

int32_t RFVirtualMachine::setDpId(const Datapath_t & dpId)
{
	this->m_dpId = dpId;

	return 0;
}

int32_t RFVirtualMachine::setMode(RFVMMode mode)
{
	this->m_mode = mode;

	return 0;
}

int32_t RFVirtualMachine::setIpAddr(uint32_t ipAddr)
{
	this->m_ipAddr = ipAddr;

	return 0;
}

int32_t RFVirtualMachine::setIpAddr(string ipAddr)
{
	struct in_addr inp;

	if (!inet_aton(ipAddr.c_str(), &inp))
	{
		return -1;
	}

	this->m_ipAddr = inp.s_addr;

	return 0;
}

int32_t RFVirtualMachine::setTcpPort(uint32_t tcpPort)
{
	this->m_tcpPort = tcpPort;

	return 0;
}

int32_t RFVirtualMachine::setSockFd(uint32_t sockFd)
{
	this->m_sockFd = sockFd;

	return 0;
}

int32_t RFVirtualMachine::setVMAddr(const struct sockaddr_in * addr)
{
	std::memcpy(&this->m_vmAddr, addr, sizeof(this->m_vmAddr));

	return 0;
}
