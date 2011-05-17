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

#include <sys/types.h>
#include <sys/socket.h>
#include <cstring>
#include <arpa/inet.h>
#include <errno.h>
#include <sstream>
#include <syslog.h>

#include <TCPSocket.hh>

TCPSocket::TCPSocket() {
	uiSockFD = -1;

}

TCPSocket::~TCPSocket() {
	int32_t ret = 0;

	if (uiSockFD != 0) {
		ret = checkSocket(uiSockFD);

		if (ret != SUCCESS) {
			syslog(LOG_ERR, "TCPSocket::<~>: Error while closing socket %d",
					ret);
		}

		if (uiSockFD != 0) {
			ret = ::close(uiSockFD);
		}

		if (ret != 0) {
			syslog(LOG_ERR, "TCPSocket::<~>: Error while closing socket %d",
					ret);
		}
	}

}

t_result TCPSocket::open() {
	int32_t ret = 0;

	ret = ::socket(AF_INET, SOCK_STREAM, 0);
	if (ret >= 0) {
		uiSockFD = ret;
		syslog(LOG_INFO, "[TCPSOCK] new uiSockFD: %d ", uiSockFD);
		ret = SUCCESS; // EXIT: Success
	} else {
		ret = -1; // EXIT: Cannot create socket
	}
	return ret;
}

t_result TCPSocket::close() {
	int8_t ret = 0;

	ret = ::close(uiSockFD);

	if (ret != 0) {
		syslog(LOG_ERR, "TCPSocket::close: Cannot close socket");
	}

	return ret;
}

t_result TCPSocket::reopen() {
	int8_t ret = 0;

	ret = close();

	if (ret != SUCCESS) {
		syslog(LOG_ERR, "TCPSocket::reopen: Cannot close socket");
	}

	ret = open();

	if (ret != SUCCESS) {
		syslog(LOG_ERR, "TCPSocket::reopen: Cannot open socket");
	}

	return ret;
}

t_result TCPSocket::setFD(uint32_t fd) {
	int32_t ret = 0;

	ret = checkSocket(fd);
	if (ret == SUCCESS) {
		uiSockFD = fd;
	} else {
		syslog(LOG_ERR, "TCPSocket::setFD: Cannot set FD for TCP socket");
	}
	return ret;
}

t_result TCPSocket::connect(ttag_Address address, uint32_t port) {
	int8_t ret = 0;


	string strAddr;

	::bzero(&sock, sizeof(sockaddr_in));

	sock.sin_family = AF_INET;
	sock.sin_port = htons(port);

	connectedIP.ulTotal = address.ulTotal;
	addressToStr(address, strAddr);

	syslog(LOG_INFO, "[TCPSOCK] Connecting to controller at %s ,port %d",
			strAddr.c_str(), port);

	::inet_pton(AF_INET, strAddr.c_str(), &sock.sin_addr);

	ret = ::connect(uiSockFD, ((sockaddr*) &sock), sizeof(sockaddr_in));

	if (ret != 0) {
		syslog(LOG_ERR, "TCPSocket::connect: Cannot connect");
	} else {
		syslog(LOG_INFO, "[TCPSOCK] Connected successfully");
	}
	return ret;
}

t_result TCPSocket::sendMessage(IPCMessage *msg) {
	t_result ret = 0;

	uint8_t data[MAX_VECTOR_SIZE];
	uint32_t size = 0;

	::bzero(data, MAX_VECTOR_SIZE);

	/** >> DEBUG */
	uint8_t temp[MAX_VECTOR_SIZE];

	if (msg == NULL) {
		return -1;
	}

	stringstream ss;
	string tempText;
	msg->getBytes(temp, size);

	if (uiSockFD < 0) {
		ret = ERROR_2;
		return ret;
	}

	msg->getBytes(data, size);

	ret = ::send(uiSockFD, data, size, 0);

	if (ret > 0) {
		ret = SUCCESS; // EXIT: Success
	} else {

		ret = ERROR_2;

	}

	return ret;
}

t_result TCPSocket::recvMessage(IPCMessage *msg) {
	int32_t ret = 0;

	uint8_t data[MAX_VECTOR_SIZE];

	::bzero(data, MAX_VECTOR_SIZE);

	if (msg == NULL) {
		ret = ERROR_2;
		return ret;
	}

	if (uiSockFD < 0) {
		ret = ERROR_2;
		return ret;
	}

	ret = ::recv(uiSockFD, data, MAX_VECTOR_SIZE, 0);

	if (ret > 0) {

		ret = msg->parseBytes(data, ret);
		if (ret != SUCCESS) {
		}

	} else if (ret == 0) {
		syslog(LOG_INFO, "Message total bytes == 0");
		uint32_t i = 0;
		ret = reopen();
		do {
			ret = reconnect();
			i++;
			syslog(LOG_INFO, "We are trying to reconnect, attempt number: %d ",
					i);
			if (ret != SUCCESS) {
				sleep(5);
			}
		} while (ret != SUCCESS);
	} else if (ret < 0) {
		ret = errno;
		syslog(LOG_INFO, "Ret < 0");

	}

	return ret;
}

t_result TCPSocket::checkSocket(uint32_t fd) {
	sockaddr_in test;
	socklen_t testlen = sizeof(sockaddr_in);
	int32_t ret = 0;

	::bzero(&test, sizeof(sockaddr_in));

	if (fd <= 0) {
		ret = ERROR_2;
		return ret;
	}

	ret = ::getsockname(fd, (sockaddr *) &test, &testlen);
	if (ret == SUCCESS) {
		ret = ::getpeername(fd, (sockaddr *) &test, &testlen);
		if (ret == SUCCESS) {

			ret = SUCCESS;
		} else {
			ret = ERROR_2;
		}
	} else {
		ret = ERROR_2;
	}

	return ret;
}

t_result TCPSocket::reconnect() {

	string strAddr;
	t_result ret = SUCCESS;

	addressToStr(connectedIP, strAddr);

	syslog(LOG_INFO, "[TCPSOCK] Re-Connecting to controller at %s",
			strAddr.c_str());

	::inet_pton(AF_INET, strAddr.c_str(), &sock.sin_addr);

	ret = ::connect(uiSockFD, ((sockaddr*) &sock), sizeof(sockaddr_in));
	if (ret != 0) {
		syslog(LOG_ERR, "TCPSocket::connect: Cannot connect");
	} else {
		syslog(LOG_INFO, "[TCPSOCK] Connected successfully");
	}
	return ret;
}
