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

#ifndef __CNM_APP_H__
#define __CNM_APP_H__

#include "vputypes.h"
#include "component.h"

typedef void*       CNMTask;

typedef enum {
    CNM_TASK_DONE,
    CNM_TASK_RUNNING,
    CNM_TASK_ERROR
} CNMTaskWaitState;

typedef void (*CNMTaskListener)(CNMTask task, void* context);

typedef struct CNMAppConfig {
    Uint32          task_interval;
} CNMAppConfig;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void    CNMAppInit(void);
extern BOOL    CNMAppAdd(CNMTask task);
extern BOOL    CNMAppRun(CNMAppConfig cfg);
extern void    CNMAppStop(void);
extern CNMTask CNMTaskCreate(void);
extern void    CNMTaskRelease(CNMTask task);
extern BOOL    CNMTaskDestroy(CNMTask task);
extern BOOL    CNMTaskAdd(CNMTask task, Component component);
extern BOOL    CNMTaskRun(CNMTask task);
extern CNMTaskWaitState CNMTaskWait(CNMTask task);
extern BOOL    CNMTaskStop(CNMTask task);
extern BOOL    CNMTaskIsTerminated(CNMTask task);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CNM_APP_H__ */

