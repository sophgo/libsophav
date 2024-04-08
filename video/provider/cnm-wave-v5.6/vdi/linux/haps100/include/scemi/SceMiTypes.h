/******************************************************************************
 Copyright 2009 Synopsys, Inc. This Example and the associated documentation
 are confidential and proprietary to Synopsys, Inc. Your use or disclosure of
 this example is subject to the terms and conditions of a written license
 agreement between you, or your company, and Synopsys, Inc.
 *******************************************************************************
 Title      : SCE-MI Object definitions
 Project    : SCE-MI 2 Implementation
 ******************************************************************************/
/**
 * @file   SceMiConstants.h
 * @author  Roshan Antony Tauro
 * @date   2006/11/30
 *
 * @brief  Type Definitions of Various Data Types used in the Library.
 */
/******************************************************************************
 $Id: //chipit/chipit/main/release/tools/scemi2/sw/include/SceMiTypes.h#2 $ $Author: mlange $ $DateTime: 2017/02/02 05:27:09 $
 ******************************************************************************/

#ifndef __SceMiTypes_h__
#define __SceMiTypes_h__

#ifdef _WIN32
	#include <windows.h>
	#include <BaseTsd.h>
	typedef UINT8  SceMiU8;
	typedef UINT16 SceMiU16;
	typedef UINT32 SceMiU32;
	typedef UINT64 SceMiU64;
#else
	#include <inttypes.h>
	typedef uint8_t SceMiU8;
	typedef uint16_t SceMiU16;
	typedef uint32_t SceMiU32;
	typedef uint64_t SceMiU64;
#endif
#ifndef _WIN32
typedef unsigned char BOOL;
#endif

#ifndef VPI_USER_H
	typedef struct vpi_vecval {
	        SceMiU32 a;
	        SceMiU32 b;
	} s_vpi_vecval, *p_vpi_vecval;
	typedef void *vpiHandle;
#endif

#ifndef INCLUDED_SVDPI
	typedef unsigned char svBit;
	typedef SceMiU32 svBitVecVal;
	typedef SceMiU32 svBitVec32;
	typedef s_vpi_vecval svLogicVecVal;
	typedef struct { SceMiU32 c; SceMiU32 d;} svLogicVec32;
#endif

#endif

