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

#ifndef CPQD_H_
#define CPQD_H_

#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <string>
#include "cpqd_types.hh"

using namespace std;

#ifdef ONCS_BIG_ENDIAN
#define IS_BIG_ENDIAN 1
#else
#define IS_BIG_ENDIAN 0
#endif

#define MPA_ACCESS_PORT 10000
#define MPA_ANSWER_PORT 10001
#define CLI_ACCESS_PORT 5001




/** Maximum size of any vector throughout the code */
#define MAX_VECTOR_SIZE 			2048
/** Maximum size of any CLI text sent back to CLI */
#define MAX_TEXT_SIZE				5000
/** Maximum size of any string throughout the code */
#define MAX_STRING_SIZE 			200
/** Maximum number of characters in an IP ttag_Address */
#define MAX_STR_IP_ADDR_SIZE 		16
#define MAX_CLIENT_ID_SIZE 			16 ///< Maximum size of client IDs

/**
 * This structure holds an IPv4 ttag_Address.
 * This ttag_Address (the integer attribute) should be in big-endian order.
 * @todo Trocar tag por uni em address
 */
typedef union tag_Address {
	uint32_t ulTotal; ///< Total ttag_Address
	uint8_t vecucOctects[4]; ///< ttag_Address separated by octects
} ttag_Address, *ptag_Address;

/**
 * This structure is useful when using template structures with string content
 */
typedef struct tag_String
{
	char szString[MAX_STRING_SIZE];
} ttag_String, * ptag_String;

ostream & operator<<(ostream & out, ttag_Address & addr);
istream & operator>>(istream & out, ttag_Address & addr);

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Print a vector in hexadecimal format.
 */
void printVector(uint8_t vec[], uint32_t size, uint32_t offset);

/**
 * Prints a vector to a string
 */
void sprintVector(char * str, uint8_t vec[], uint32_t size, uint32_t offset);

/**
 * Converts a 4-byte vector into integer value.
 * This function uses the definition of LITTLE_ENDIAN or BIG_ENDIAN
 */
int8_t bytesToInteger(uint8_t bValue [], uint32_t * iValue);

/**
 * Converts a integer value into a 4-byte vector.
 * This function uses the definition of LITTLE_ENDIAN or BIG_ENDIAN
 */
int8_t integerToBytes(uint32_t iValue, uint8_t ** bValue);

/**
 * Converts a 2-byte vector into short value.
 */
int8_t bytesToShort(uint8_t bValue [], uint16_t * sValue, uint8_t isBigEndian);


int8_t addressToStr(ttag_Address addr, string & strAddr);

int8_t strToAddress(const string str, ttag_Address & addr);

enum {
	SUCCESS ,
	ERROR_1 ,
	ERROR_2
};


#ifdef __cplusplus
}
#endif


#endif /* CPQD_H_ */
