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

#include "rfprotocol.hh"
#include <cstring>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

/*
 * RFMessage
 */

RFMessage::RFMessage() {
	this->m_length = sizeof(RFMessage);
}

/* Getters */
uint32_t RFMessage::getDstIP() const {
	return this->m_dstIP;
}

string RFMessage::getStrDstIP() const {
	struct in_addr addr;
	addr.s_addr = this->m_dstIP;
	string strIP(inet_ntoa(addr));

	return strIP;
}

uint32_t RFMessage::getSrcIP() const {
	return this->m_srcIP;
}

string RFMessage::getStrSrcIP() const {
	struct in_addr addr;
	addr.s_addr = this->m_srcIP;
	string strIP(inet_ntoa(addr));

	return strIP;
}

RFMsgType RFMessage::getType() const {
	return this->m_type;
}

uint64_t RFMessage::getVMId() const {
	return this->m_vmId;
}

uint32_t RFMessage::length() const {
	return this->m_length;
}

/* Setters */
int32_t RFMessage::setDstIP(uint32_t dstIP) {
	this->m_dstIP = dstIP;

	return 0;
}

int32_t RFMessage::setDstIP(string dstIP) {
	struct in_addr inp;

	if (!inet_aton(dstIP.c_str(), &inp)) {
		return -1;
	}

	this->m_dstIP = inp.s_addr;

	return 0;
}

int32_t RFMessage::setSrcIP(uint32_t srcIP) {
	this->m_srcIP = srcIP;

	return 0;
}

int32_t RFMessage::setSrcIP(string srcIP) {
	struct in_addr inp;

	if (!inet_aton(srcIP.c_str(), &inp)) {
		return -1;
	}

	this->m_srcIP = inp.s_addr;

	return 0;
}

int32_t RFMessage::setType(RFMsgType type) {
	this->m_type = type;

	return 0;
}

int32_t RFMessage::setVMId(uint64_t id) {
	this->m_vmId = id;

	return 0;
}

/*
 * RFVMMsg
 */

RFVMMsg::RFVMMsg() {
	this->m_length = sizeof(RFVMMsg);
}

/* Getters */
uint8_t RFVMMsg::getWildcard() const {
	return this->m_wildcard;
}

uint64_t RFVMMsg::getDpId() const {
	return this->m_dpId;
}

RFVMMode RFVMMsg::getMode() const {
	return this->m_mode;
}

uint8_t RFVMMsg::getNPorts() const {
	return this->m_nports;
}

/* Setters */
int32_t RFVMMsg::setWildcard(uint8_t wildcard) {
	this->m_wildcard = wildcard & RFP_VM_ALL;

	return 0;
}

int32_t RFVMMsg::setDpId(uint64_t dpId) {
	this->m_dpId = dpId;

	return 0;
}

int32_t RFVMMsg::setMode(RFVMMode mode) {
	this->m_mode = mode;

	return 0;
}

uint8_t RFVMMsg::setNPorts(uint8_t nports) {
	this->m_nports = nports;

	return 0;
}

/*
 * RFPacketMsg
 */

RFPacketMsg::RFPacketMsg(const uint8_t * buffer, uint32_t size) {
	std::memcpy(this->m_packet, buffer, size);
	this->m_pktSize = size;
	this->m_length = sizeof(RFPacketMsg) - sizeof(this->m_packet) + size;
}

const uint8_t * RFPacketMsg::pktData() const {
	return (const uint8_t *) this->m_packet;
}

uint32_t RFPacketMsg::pktSize() const {
	return this->m_pktSize;
}

uint32_t RFPacketMsg::getInPort() const {
	return this->m_inPort;
}

int32_t RFPacketMsg::setInPort(uint32_t port) {
	this->m_inPort = port;

	return 0;
}

/*
 *  RFFlowMsg
 */

RFFlowMsg::RFFlowMsg() {
	this->m_length = sizeof(RFFlowMsg);
}

/* Getters */
const FlowRule& RFFlowMsg::getRule() const {
	return this->m_rule;
}
const FlowAction& RFFlowMsg::getAction() const {
	return this->m_action;
}

/* Setters */
int32_t RFFlowMsg::setRule(uint32_t ip, uint32_t mask) {
	this->m_rule.ip = ip;
	this->m_rule.mask = mask;

	return 0;
}

int32_t RFFlowMsg::setAction(uint32_t port, const uint8_t srcmac[],
		const uint8_t dstmac[]) {
	this->m_action.dstPort = port;
	std::memcpy(this->m_action.srcMac, srcmac, IFHWADDRLEN);
	std::memcpy(this->m_action.dstMac, dstmac, IFHWADDRLEN);

	return 0;
}

/*
 * RFSocket
 */

RFMessage * RFSocket::rfRecv() {
	char buffer[ETH_PKT_SIZE];

	std::memset(buffer, 0, ETH_PKT_SIZE);

	int rcount = read(this->m_sockFd, buffer, sizeof(RFMessage));

	if (sizeof(RFMessage) == rcount) {
		RFMessage * recvMsg = (RFMessage *) buffer;
		int32_t length = recvMsg->length() - rcount;
		rcount += read(this->m_sockFd, buffer + sizeof(RFMessage), length);

		char * msg = new char[rcount];
		std::memcpy(msg, buffer, rcount);

		return (RFMessage *) msg;
	}

	return NULL;
}

int32_t RFSocket::rfSend(RFMessage & msg) {
	return write(this->m_sockFd, &msg, msg.length());
}

RFSocket::RFSocket() {

}

RFSocket::RFSocket(const RFSocket& s) {
	this->m_sockFd = s.m_sockFd;
	std::memcpy(&this->m_addr, &s.m_addr, sizeof(this->m_addr));
}

int32_t RFSocket::rfOpen() {
	this->m_sockFd = socket(AF_INET, SOCK_STREAM, 0);

	return this->m_sockFd;
}

int32_t RFSocket::rfAccept(RFSocket & s) {
	socklen_t addrlen = sizeof(struct sockaddr_in);

	s.m_sockFd
			= accept(this->m_sockFd, (struct sockaddr *) &s.m_addr, &addrlen);

	return s.m_sockFd;
}

int32_t RFSocket::rfBind(uint32_t tcpPort) {
	std::memset((char *) &this->m_addr, 0, sizeof(this->m_addr));
	this->m_addr.sin_family = AF_INET;
	this->m_addr.sin_addr.s_addr = INADDR_ANY;
	this->m_addr.sin_port = tcpPort;

	return bind(this->m_sockFd, (struct sockaddr *) &this->m_addr,
			sizeof(this->m_addr));
}

int32_t RFSocket::rfConnect(uint32_t tcpPort, string ipAddr) {
	struct in_addr inp;

	if (!inet_aton(ipAddr.c_str(), &inp)) {
		return -1;
	}

	std::memset((char *) &this->m_addr, 0, sizeof(this->m_addr));
	this->m_addr.sin_family = AF_INET;
	this->m_addr.sin_port = tcpPort;
	this->m_addr.sin_addr.s_addr = inp.s_addr;

	return connect(this->m_sockFd, (struct sockaddr *) &this->m_addr,
			sizeof(this->m_addr));
}

int32_t RFSocket::rfConnect(uint32_t tcpPort, uint32_t ipAddr) {
	std::memset((char *) &this->m_addr, 0, sizeof(this->m_addr));
	this->m_addr.sin_family = AF_INET;
	this->m_addr.sin_port = tcpPort;
	this->m_addr.sin_addr.s_addr = ipAddr;

	return connect(this->m_sockFd, (struct sockaddr *) &this->m_addr,
			sizeof(this->m_addr));
}

int32_t RFSocket::rfListen(uint32_t nConn) {
	return listen(this->m_sockFd, nConn);
}

int32_t RFSocket::rfBlock(bool enable) {
	int flags = fcntl(this->m_sockFd, F_GETFL);

	flags &= ~O_NONBLOCK;

	if (!enable) {
		flags |= O_NONBLOCK;
	}

	return fcntl(this->m_sockFd, F_SETFL, flags);
}

int32_t RFSocket::rfClose() {
	return close(this->m_sockFd);
}

uint32_t RFSocket::getIpAddr() {
	return this->m_addr.sin_addr.s_addr;
}

uint32_t RFSocket::getPort() {
	return this->m_addr.sin_port;
}

int32_t RFSocket::getFd() {
	return this->m_sockFd;
}
