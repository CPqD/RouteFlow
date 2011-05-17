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

#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <iosfwd>
#include <arpa/inet.h>
#include <stdio.h>
#include <bits/stringfwd.h>

#include "cpqd_types.hh"
#include "cpqd.hh"

int8_t bytesToInteger(uint8_t bValue[], uint32_t * iValue) {
	int8_t ret = 0;

	if (iValue == NULL)
	{
		return 0;
	}

	bzero(iValue, sizeof(uint32_t));

#ifdef ONCS_LITTLE_ENDIAN
	(*iValue) |= bValue[3] << 24;
	(*iValue) |= bValue[2] << 16;
	(*iValue) |= bValue[1] << 8;
	(*iValue) |= bValue[0];
#else
	(*iValue) |= bValue[0] << 24;
	(*iValue) |= bValue[1] << 16;
	(*iValue) |= bValue[2] << 8;
	(*iValue) |= bValue[3];
#endif
	return ret;
}

int8_t bytesToShort(uint8_t bValue[], uint16_t * sValue, uint8_t isBigEndian) {
	int8_t ret = 0;

	if (sValue == NULL)
	{
		return 0;
	}

	bzero(sValue, sizeof(uint16_t));

	if (isBigEndian == 0) {
		(*sValue) |= bValue[1] << 8;
		(*sValue) |= bValue[0];
	} else {
		(*sValue) |= bValue[0] << 8;
		(*sValue) |= bValue[1];
	}

	return ret;
}

int8_t integerToBytes(uint32_t iValue, uint8_t ** bValue) {
	int8_t ret = 0;
	if (bValue == NULL)
	{
		return -1;
	}

#ifdef ONCS_LITTLE_ENDIAN
	(*bValue)[3] = (iValue & 0xff000000) >> 24;
	(*bValue)[2] = (iValue & 0x00ff0000) >> 16;
	(*bValue)[1] = (iValue & 0x0000ff00) >> 8;
	(*bValue)[0] = (iValue & 0x000000ff);
#else
	(*bValue)[0] = (iValue & 0xff000000) >> 24;
	(*bValue)[1] = (iValue & 0x00ff0000) >> 16;
	(*bValue)[2] = (iValue & 0x0000ff00) >> 8;
	(*bValue)[3] = (iValue & 0x000000ff);
#endif
	return ret;
}

void printAddress(ttag_Address addr) {
	printf("%u.%u.%u.%u", addr.vecucOctects[0], addr.vecucOctects[1],
			addr.vecucOctects[2], addr.vecucOctects[3]);
}

int8_t addressToStr(ttag_Address addr, string & strAddr) {
	int8_t ret = 0;

	stringstream ss;
	ss << addr;
	ss >> strAddr;

	return ret;
}

int8_t strToAddress(const string str, ttag_Address & addr) {
	int8_t ret = 0;

	::inet_pton(AF_INET, str.c_str(), & addr);

	return ret;
}

bool operator==(ttag_Address & addr1, ttag_Address & addr2) {
	return (addr1.ulTotal == addr2.ulTotal);
}

ostream & operator<<(ostream & out, ttag_Address & addr)
{
	out << "" << (int)addr.vecucOctects[0];
	out << "." << (int)addr.vecucOctects[1];
	out << "." << (int)addr.vecucOctects[2];
	out << "." << (int)addr.vecucOctects[3];
	return out;
}


istream & operator>>(istream & in, ttag_Address & addr) {
	uint32_t octect;
    char c;


    in >> octect;
    addr.vecucOctects[0] = (uint8_t) octect;
    in >> c;

    in >> octect;
    addr.vecucOctects[1] = (uint8_t) octect;
    in >> c;

    in >> octect;
    addr.vecucOctects[2] = (uint8_t) octect;
    in >> c;

    in >> octect;
    addr.vecucOctects[3] = (uint8_t) octect;

	return in;
}
