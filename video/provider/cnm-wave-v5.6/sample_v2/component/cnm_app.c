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

#include "vdi_osal.h"
#include "cnm_app.h"
#include "cnm_app_internal.h"
#include "./misc/debug.h"

STATIC CNMAppContext appCtx;

void CNMAppInit(void)
{
    osal_memset((void*)&appCtx, 0x00, sizeof(CNMAppContext));

    osal_init_keyboard();
}

BOOL CNMAppAdd(CNMTask task)
{
    CNMTaskContext* taskCtx = (CNMTaskContext*)task;
    if (appCtx.numTasks >= MAX_NUM_INSTANCE) {
        return FALSE;
    }

    taskCtx->oneTimeRun = TRUE;
    appCtx.taskList[appCtx.numTasks++] = task;
    return TRUE;
}

BOOL CNMAppRun(CNMAppConfig cfg)
{
    CNMTask             task;
    Uint32              i;
    BOOL                success;
    Uint32              runningTask;

    success     = TRUE;
    runningTask = appCtx.numTasks;
    for (i=0; i<appCtx.numTasks; i++) {
        task=(CNMTask)appCtx.taskList[i];
        if ( FALSE == CNMTaskRun((CNMTask)task))
            return FALSE;
        
        if (cfg.task_interval) 
            osal_msleep(cfg.task_interval);
    }

    while (0 < runningTask) {
        for (i=0; i<appCtx.numTasks; i++) {
            if (NULL == (task=(CNMTask)appCtx.taskList[i])) continue;

            if (FALSE == supportThread) {
                if (FALSE == CNMTaskRun((CNMTask)task)) break;
            }

            if (CNM_TASK_RUNNING != CNMTaskWait(task)) {
                success &= CNMTaskStop(task);
                CNMTaskRelease(task);
                if(TRUE == CNMTaskDestroy(task)) {
                    appCtx.taskList[i] = NULL;
                    runningTask--;
                }
            }
        }
    }

    ReleaseCompMutex();

    osal_memset((void*)&appCtx, 0x00, sizeof(CNMAppContext));

    osal_close_keyboard();

    return (0 == CNMErrorGet()) ? success : FALSE;
}

void CNMAppStop(void)
{
    Uint32 i;

    for (i=0; i<appCtx.numTasks; i++) {
        CNMTaskStop(appCtx.taskList[i]);
    }
}

