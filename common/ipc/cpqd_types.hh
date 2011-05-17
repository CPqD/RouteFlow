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

#ifndef TYPESCARD_H_
#define TYPESCARD_H_

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Definicoes utilizadas no modulo *********************************************
 * ****************************************************************************/

#ifdef __GNUC__
#define PACKED __attribute__((packed))
#else
#error "Atributo PACKED disponivel no GCC."
#endif

/*******************************************************************************
 * Macros utilizadas no modulo *************************************************
 * ****************************************************************************/

/* Macro para criacao da struct de acordo com a especificacao de codificacao
 * do CPqD.
 */
#define STRUCT(__newStruct) struct PACKED tag_ ## __newStruct
/* Macro para definicao da struct de acordo com a especificacao de codificacao
 * do CPqD.
 */
#define TYPEDEF_STRUCT(__newTypeStruct) typedef STRUCT(__newTypeStruct) ttag_ ## __newTypeStruct
/* Macro para criacao da union de acordo com a especificacao de codificacao
 * do CPqD.
 */
#define UNION(__newUnion) union PACKED uni_ ## __newUnion
/* Macro para definicao da union de acordo com a especificacao de codificacao
 * do CPqD.
 */
#define TYPEDEF_UNION(__newTypeUnion) typedef UNION(__newTypeUnion) tuni_ ## __newTypeUnion
/* Macro para criacao da enumeracao de acordo com a especificacao de codificacao
 * do CPqD.
 */
#define ENUM(__newEnum) enum PACKED enum_ ## __newEnum
/* Macro para definicao da enumeracao de acordo com a especificacao de
 * codificacao do CPqD.
 */
#define TYPEDEF_ENUM(__newTypeEnum) typedef ENUM(__newTypeEnum) tenum_ ## __newTypeEnum

/*******************************************************************************
 * Definicoes de variaveis no modulo *******************************************
 * ****************************************************************************/

#define  TRUE8  1
#define  FALSE8 0

/* Definicoes de variaveis de acordo com a especificacao de codificacao
 * do CPqD.
 */

typedef       unsigned    t_bitfield;
typedef            int      t_result;
typedef unsigned  char          Bool8;

	typedef unsigned char 		uInt8; ///< uc variable prefix
typedef unsigned short int 	uInt16; ///< ui variable prefix
typedef unsigned int 		uInt32; ///< ul variable prefix
typedef unsigned  long        uInt64; ///< ull variable prefix
typedef signed char 		Int8; ///< c variable prefix
typedef signed short int 	Int16; ///< i variable prefix
typedef signed int			Int32; ///< l variable prefix
typedef signed long int 	Int64; ///< ll variable prefix

typedef unsigned char*			puInt8;
typedef unsigned short int*		puInt16;
typedef unsigned long int*		puInt32;
typedef unsigned long int*	puInt64;

typedef char*					pInt8;
typedef short int*				pInt16;
typedef long int*				pInt32;
typedef long int*      	pInt64;


/*******************************************************************************
 * Prototipo Funcoes Externas **************************************************
 * ****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* TYPESCARD_H_ */
