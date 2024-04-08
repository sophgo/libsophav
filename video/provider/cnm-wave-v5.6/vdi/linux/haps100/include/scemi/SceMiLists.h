/******************************************************************************
 Copyright © 2009 Synopsys, Inc. This Example and the associated documentation
 are confidential and proprietary to Synopsys, Inc. Your use or disclosure of
 this example is subject to the terms and conditions of a written license
 agreement between you, or your company, and Synopsys, Inc.
 *******************************************************************************
 Title      : Commanly used macros
 Project    : SCE-MI 2 Implementation
 ******************************************************************************/
/**
 * @file   SceMiLists.h
 * @author  Roshan Antony Tauro
 * @date   2006/10/17
 *
 * @brief  Macros used to implement lists.
 *
 *
 */
/******************************************************************************
 $Id: //chipit/chipit/main/release/tools/scemi2/sw/include/SceMiLists.h#1 $ $Author: mlange $ $DateTime: 2014/06/05 03:13:44 $

 $Log: SceMiLists.h,v $
 Revision 1.6  2009/07/03 09:25:11  rtauro
 Minor modifications.

 Revision 1.5  2009/07/02 15:11:23  rtauro
 added new scope related functions and pipe related functions

 Revision 1.4  2009/02/20 13:20:03  rtauro
 Added copyright info.

 Revision 1.3  2007/11/05 15:08:08  rtauro
 First Revision



 ******************************************************************************/
#ifndef __SceMiLists_h__
#define __SceMiLists_h__

#ifdef __cplusplus
namespace SceMiC {
#endif
/* Declaration of Lists */
DECLARE_LIST_FUNCTIONS(UMRBusHandleList, UMR_HANDLE)
DECLARE_LIST_FUNCTIONS(SceMiU32List, SceMiU32)
DECLARE_LIST_FUNCTIONS(SceMiMessageInPortProxyList, SceMiMessageInPortProxy)
DECLARE_LIST_FUNCTIONS(SceMiMessageOutPortProxyList, SceMiMessageOutPortProxy)
DECLARE_LIST_FUNCTIONS(SceMiDPIFunctionList, SceMiDPIFunction)
DECLARE_LIST_FUNCTIONS(SceMiPipeList, SceMiPipe)
DECLARE_LIST_FUNCTIONS(SceMiPipeNotifierList, SceMiPipe*)
DECLARE_LIST_FUNCTIONS(SceMiParamAttributeList, SceMiParamAttribute)
DECLARE_LIST_FUNCTIONS(SceMiParamInstanceList, SceMiParamInstance)
DECLARE_LIST_FUNCTIONS(SceMiParamObjectKindList, SceMiParamObjectKind)
DECLARE_LIST_FUNCTIONS(SceMiUserDataList, scemiUserData)
DECLARE_LIST_FUNCTIONS(SceMiScopeList, scemiScope)
#ifdef __cplusplus
}
#endif

#endif
