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

#ifndef RFPROTOCOL_HH_
#define RFPROTOCOL_HH_

#include <stdint.h>
#include <string>
#include <net/if.h>
#include <sys/types.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/socket.h>
#include <netinet/in.h>

using std::string;

const int ETH_PKT_SIZE = 1600;

typedef enum {
	/* Connection. */
	RFP_CMD_REGISTER, RFP_CMD_ACCPET, RFP_CMD_REJECT, RFP_CMD_UNREGISTER,

	/* VM Configuration. */
	RFP_CMD_VM_CREATE, RFP_CMD_VM_DELETE, RFP_CMD_VM_RESET, RFP_CMD_VM_CONFIG,

	/* Hardware Flow. */
	RFP_CMD_FLOW_INSTALL, RFP_CMD_FLOW_REMOVE,

	/* Packet */
	RFP_MSG_PACKET
} RFMsgType;

typedef enum {
	RFP_VM_MODE_RUNNING, RFP_VM_MODE_STANDBY
} RFVMMode;

typedef enum {
	RFP_VM_NONE = 0,
	RFP_VM_DPID = 1,
	RFP_VM_MODE = 2,
	RFP_VM_NPORTS = 4,
	RFP_VM_ALL = 7
} RFVMConfigEntries;

class RFMessage {
protected:
	uint32_t m_dstIP;
	uint32_t m_srcIP;
	uint64_t m_vmId;
	RFMsgType m_type;
	uint32_t m_length;
public:

	RFMessage();

	uint32_t length() const;

	/* Getters */
	uint32_t getDstIP() const;
	string getStrDstIP() const;
	uint32_t getSrcIP() const;
	string getStrSrcIP() const;
	RFMsgType getType() const;
	uint64_t getVMId() const;

	/* Setters */
	int32_t setDstIP(uint32_t dstIP);
	int32_t setDstIP(string dstIP);
	int32_t setSrcIP(uint32_t srcIP);
	int32_t setSrcIP(string srcIP);
	int32_t setType(RFMsgType cmd);
	int32_t setVMId(uint64_t id);

};

class RFVMMsg: public RFMessage {
private:
	uint8_t m_wildcard;
	uint64_t m_dpId;
	RFVMMode m_mode;
	uint8_t m_nports;
public:

	RFVMMsg();

	/* Getters */
	uint8_t getWildcard() const;
	uint64_t getDpId() const;
	RFVMMode getMode() const;
	uint8_t getNPorts() const;

	/* Setters */
	int32_t setWildcard(uint8_t wildcard);
	int32_t setDpId(uint64_t dpId);
	int32_t setMode(RFVMMode mode);
	uint8_t setNPorts(uint8_t nports);
};

class RFPacketMsg: public RFMessage {
	uint32_t m_pktSize;
	uint32_t m_inPort;
	uint8_t m_packet[ETH_PKT_SIZE];
public:

	RFPacketMsg(const uint8_t * buffer, uint32_t size);

	const uint8_t * pktData() const;
	uint32_t pktSize() const;

	uint32_t getInPort() const;
	int32_t setInPort(uint32_t port);
};

class FlowRule {
public:
	uint32_t ip;
	uint32_t mask;
};

class FlowAction {
public:
	uint32_t dstPort;
	uint8_t srcMac[IFHWADDRLEN];
	uint8_t dstMac[IFHWADDRLEN];
};

class RFFlowMsg: public RFMessage {
	FlowRule m_rule;
	FlowAction m_action;

public:

	RFFlowMsg();

	/* Getters */
	const FlowRule& getRule() const;
	const FlowAction& getAction() const;

	/* Setters */
	int32_t setRule(uint32_t ip, uint32_t mask);
	int32_t setAction(uint32_t port, const uint8_t srcmac[],
			const uint8_t dstmac[]);

};

class RFConnection {
public:
	virtual int32_t rfOpen() = 0;
	virtual int32_t rfClose() = 0;
	virtual int32_t rfSend(RFMessage & msg) = 0;
	virtual RFMessage * rfRecv() = 0;
};

class RFSocket: public RFConnection {
private:
	int m_sockFd;
	struct sockaddr_in m_addr;
public:

	RFSocket();
	RFSocket(const RFSocket & s);

	int32_t rfSend(RFMessage & msg);
	RFMessage * rfRecv();
	int32_t rfAccept(RFSocket & s);
	int32_t rfBind(uint32_t tcpPort);
	int32_t rfConnect(uint32_t tcpPort, uint32_t ipAddr);
	int32_t rfConnect(uint32_t tcpPort, string ipAddr);
	int32_t rfListen(uint32_t nConn);
	int32_t rfBlock(bool enable);
	int32_t rfOpen();
	int32_t rfClose();

	uint32_t getIpAddr();
	uint32_t getPort();
	int32_t getFd();
};

#endif /* RFPROTOCOL_HH_ */
