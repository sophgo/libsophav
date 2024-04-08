//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
// 
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
// 
// The entire notice above must be reproduced on all authorized copies.
// 
// Description  : 
//-----------------------------------------------------------------------------

#ifndef __CNM_APP_INTERNAL_H__
#define __CNM_APP_INTERNAL_H__

#include "vputypes.h"

#define MAX_COMPONENTS_IN_TASK      6

extern BOOL supportThread;
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct CNMTaskContext {
    Uint32          numComponents;
    void*           componentList[MAX_COMPONENTS_IN_TASK];
    BOOL            stop;
    BOOL            oneTimeRun;
    BOOL            released;
    BOOL            terminate;
    BOOL            componentsConnected;
} CNMTaskContext;

typedef struct {
    Uint32  numTasks;
    CNMTask taskList[MAX_NUM_INSTANCE];
} CNMAppContext;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CNM_APP_INTERNAL_H__ */

