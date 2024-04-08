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

CNMTask CNMTaskCreate(void)
{
    CNMTaskContext* ctx = (CNMTaskContext*)osal_malloc(sizeof(CNMTaskContext));

    if (ctx == NULL) {
        VLOG(ERR, "%s:%d Failed to allocate a memory\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    osal_memset((void*)ctx, 0x00, sizeof(CNMTaskContext));
    ctx->oneTimeRun = supportThread;

    return (CNMTask)ctx;
}

void CNMTaskRelease(CNMTask task)
{
    CNMTaskContext* ctx = (CNMTaskContext*)task;
    Uint32          i;

    if (task == NULL) {
        VLOG(WARN, "%s:%d HANDLE is NULL\n", __FUNCTION__, __LINE__);
        return;
    }

    if (TRUE == ctx->released) {
        return;
    }

    for (i=0; i< ctx->numComponents; i++) {
        ComponentRelease(ctx->componentList[i]);
    }

    ctx->released = TRUE;

    return;
}

BOOL CNMTaskDestroy(CNMTask task)
{
    CNMTaskContext* ctx = (CNMTaskContext*)task;
    Uint32          i;
    Uint32          cnt = 0;

    if (task == NULL) {
        VLOG(WARN, "%s:%d HANDLE is NULL\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    for (i=0; i<ctx->numComponents; i++) {
        if (TRUE == ComponentDestroy(ctx->componentList[i], NULL)) {
            ctx->componentList[i] = NULL;
            cnt++;
        } else {
            //Close components in in sequence
            break;
        }
    }

    if(cnt != ctx->numComponents) {
        return FALSE;
    }

    osal_free(task);

    return TRUE;
}

BOOL CNMTaskAdd(CNMTask task, Component component)
{
    CNMTaskContext* ctx = (CNMTaskContext*)task;
    Uint32 num = ctx->numComponents;

    if (ctx->numComponents == MAX_COMPONENTS_IN_TASK) {
        return FALSE;
    }

    ctx->componentList[num++] = (void*)component;
    ctx->numComponents = num;

    return TRUE;
}

BOOL CNMTaskRun(CNMTask task)
{
    CNMTaskContext* ctx = (CNMTaskContext*)task;
    Uint32          i;
    ComponentImpl*  com;
    BOOL            terminate      = FALSE;
    BOOL            stopComponents = FALSE;

    if (ctx->componentsConnected == FALSE) {
        for (i=0; i<ctx->numComponents-1; i++) {
            Component from = (Component)ctx->componentList[i];
            Component to   = (Component)ctx->componentList[i+1];
            if (ComponentSetupTunnel(from, to) == FALSE) {
                return FALSE;
            }
        }
        ctx->componentsConnected = TRUE;
    }

    while (terminate == FALSE) {
        terminate = TRUE;
        for (i=0; i<ctx->numComponents; i++) {
            if ((com=ctx->componentList[i]) == NULL) {
                ctx->stop = TRUE;
                break;
            }
            if (stopComponents == TRUE) {
                /* Failure! stop all components */
                ComponentStop(com);
            }
            else {
                if (com->terminate == FALSE) {
                    if (ComponentExecute(com) == COMPONENT_STATE_TERMINATED) {
                        stopComponents = (com->success == FALSE);
                        terminate     &= TRUE;
                    }
                    else {
                        terminate     &= FALSE;
                    }
                } else if (com->terminate == TRUE){
                    com->state = COMPONENT_STATE_TERMINATED;
                }
            }
        }
        if (ctx->oneTimeRun == TRUE) break;
        osal_msleep(1); // For cross-debugging on non-thread environment.
    }
    ctx->terminate = (terminate || stopComponents);

    return TRUE;
}

CNMTaskWaitState CNMTaskWait(CNMTask task)
{
    Uint32              i;
    Uint32              doneCount = 0;
    Int32               ret = CNM_TASK_ERROR;
    ComponentImpl*      com;
    CNMTaskContext*     ctx = (CNMTaskContext*)task;
    BOOL                stopComponents = FALSE;

    for (i=0; i<ctx->numComponents; i++) {
        if ((com=(Component)ctx->componentList[i]) != NULL) {
            if (stopComponents == TRUE) {
                ComponentStop(com);
                ret = ComponentWait(com);
            }
            else {
                if ((ret=ComponentWait(com)) == 0) {
                    stopComponents = (com->success == FALSE);
                }
            }
        }
        else {
            ret = 0;    // A component might be terminated in the creating step.
        }

        if (ret == 0) doneCount++;
    }

    return (doneCount == ctx->numComponents) ? CNM_TASK_DONE : CNM_TASK_RUNNING;
}

BOOL CNMTaskStop(CNMTask task)
{
    CNMTaskContext* ctx = (CNMTaskContext*)task;
    ComponentImpl*  com;
    Uint32          i;
    BOOL            success;

    if (task == NULL) 
        return TRUE;
    
    for (i=0; i<ctx->numComponents; i++) {
        if (ctx->componentList[i]) ComponentStop(ctx->componentList[i]);
    }

    success = TRUE;
    for (i=0; i<ctx->numComponents; i++) {
        if ((com=(ComponentImpl*)ctx->componentList[i]) != NULL) {
            if (com->success == FALSE) {
                VLOG(WARN, "%s:%d <%s> returns FALSE\n", __FUNCTION__, __LINE__, com->name);
            }
            success &= com->success;
        }
    }

    return success;
}

BOOL CNMTaskIsTerminated(CNMTask task)
{
    CNMTaskContext* ctx = (CNMTaskContext*)task;
    
    return ctx->terminate;
}

