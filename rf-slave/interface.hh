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

#ifndef INTERFACE_HH_
#define INTERFACE_HH_

#include <string>
#include <net/if.h>
#include <sys/types.h>

using std::string;

class Interface
{
	uint32_t m_portId;	/* Interface port number. */
	string m_iName;		/* Interface name. */
	uint32_t m_ipAddr;	/* Interface IP address. */
	uint32_t m_netmask;	/* Interface network mask */
	uint8_t m_hwAddr[IFHWADDRLEN]; /* Interface MAC address */

public:

	Interface();
	Interface(uint32_t port, string name) throw (string);
	~Interface();

	/*
	 * Getters
	 */
	
	uint32_t getPortId() const;
	uint8_t * getHwAddr();
	uint32_t getIpAddr() const;
	string getName() const;
	uint32_t getNetmask() const;
	uint64_t getPciBusAddr()const;
	string getDriverName()const;

	/*
	 * Setters
	 */
	void setHwAddr(uint8_t hwAddr[]);
	void setIpAddr(uint32_t ipAddr);
	void setNetmask(uint32_t netmask);
	
};

#endif /* INTERFACE_HH_ */
