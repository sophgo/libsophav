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

#include <string.h>
#include "component.h"

typedef struct {
    Uint32  dummy;
} ExampleContext;

static CNMComponentParamRet GetParameterExample(ComponentImpl* from, ComponentImpl* my, GetParameterCMD commandType, void* data) 
{
    BOOL            result  = TRUE;
    PortContainer* container;

    switch(commandType) {
    case GET_PARAM_COM_IS_CONTAINER_CONUSUMED:
        container = (PortContainer*)data;
        container->consumed = TRUE;
        break;
    default:
        result = FALSE;
        break;
    }

    if (result == TRUE) return CNM_COMPONENT_PARAM_SUCCESS;
    else                return CNM_COMPONENT_PARAM_FAILURE;
}

static CNMComponentParamRet SetParameterExample(ComponentImpl* from, ComponentImpl* my, SetParameterCMD commandType, void* data) 
{
    BOOL        result  = TRUE;

    switch(commandType) {
    default:
        result = FALSE;
        break;
    }

    if (result == TRUE) return CNM_COMPONENT_PARAM_SUCCESS;
    else                return CNM_COMPONENT_PARAM_FAILURE;
}

static BOOL PrepareExample(ComponentImpl* com, BOOL* done)
{
    *done = TRUE;
    return TRUE;
}

static BOOL ExecuteExample(ComponentImpl* com, PortContainer* in, PortContainer* out) 
{
    return TRUE;
}

static BOOL DestroyExample(ComponentImpl* com) 
{
    ExampleContext* myParam = (ExampleContext*)com->context;

    osal_free(myParam);

    return TRUE;
}

static Component CreateExample(ComponentImpl* com, CNMComponentConfig* componentParam) 
{
    com->context = (ExampleContext*)osal_malloc(sizeof(ExampleContext));
    osal_memset(com->context, 0, sizeof(ExampleContext));

    return (Component)com;
}

static void ReleaseExample(ComponentImpl* com)
{
}

ComponentImpl exampleComponentImpl = {
    "example",
    NULL,
    {0,},
    {0,},
    sizeof(PortContainer),
    5,
    CreateExample,
    GetParameterExample,
    SetParameterExample,
    PrepareExample,
    ExecuteExample,
    ReleaseExample,
    DestroyExample
};

