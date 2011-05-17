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

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <iostream>
#include <cstring>

#include "interface.hh"

using std::memset;
using std::memcpy;
using std::cout;
using std::endl;
using std::stringstream;
using std::hex;


Interface::Interface(){}

Interface::Interface(uint32_t port, string name) throw (string)
{
	this->m_iName = name;
	this->m_portId = port;

}

Interface::~Interface(){
}


uint32_t Interface::getPortId() const
{
	return m_portId;
}

uint8_t * Interface::getHwAddr()
{
	return m_hwAddr;
}

uint32_t Interface::getIpAddr() const
{
	return m_ipAddr;
}

string Interface::getName() const
{
	return m_iName;
}

uint32_t Interface::getNetmask() const
{
	return m_netmask;
}

void Interface::setHwAddr(uint8_t hwAddr[])
{
	for (int i = 0; i < IFHWADDRLEN; i++)
	{
		this->m_hwAddr[i] = hwAddr[i];
	}

}

void Interface::setIpAddr(uint32_t ipAddr)
{
	this->m_ipAddr = ipAddr;

}

void Interface::setNetmask(uint32_t netmask)
{
	this->m_netmask = netmask;

	
}
