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

#ifndef __ENCODER_LISTENER_H__
#define __ENCODER_LISTENER_H__
#include "cnm_app.h"
#include "misc/bw_monitor.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct EncListenerContext {
    Uint32      coreIdx;
    Comparator  es;
    BOOL        match;
    BOOL        matchOtherInfo;
    EndianMode  streamEndian;
    Uint32      reconWidth;
    Uint32      reconHeight;

    /* performance & bandwidth */
    BOOL        performance;
    Uint32      bandwidth;
    PFCtx       pfCtx;
    BWCtx*      bwCtx;
    Uint32      fps;
    Uint32      pfClock;
    char        cfgFileName[MAX_FILE_PATH];
    BOOL        headerEncDone[MAX_NUM_INSTANCE];
    BOOL        ringBufferEnable;
    BOOL        ringBufferWrapEnable;
} EncListenerContext;


extern int EncoderListener(
    Component   com,
    Uint64      event,
    void*       data,
    void*       context
    );

void Coda9HandleEncGetOutputEvent(
    Component com, 
    CNMComListenerEncDone* param, 
    EncListenerContext* ctx
    );

void Coda9HandleEncMakeHeaderEvent(
    Component com,
    CNMComListenerEncMakeHeader* param,
    EncListenerContext* ctx
    );

void HandleEncCompHeaderEvent(
    Component com,
    BitstreamReader pBr,
    EncListenerContext* ctx
    );

extern BOOL SetupEncListenerContext(
    EncListenerContext* ctx,
    CNMComponentConfig* config
    );

extern void ClearEncListenerContext(
    EncListenerContext* ctx
    );

extern void HandleEncHandlingIntEvent(
    Component                   com,
    CNMComListenerHandlingInt*  param,
    EncListenerContext*         ctx
    );

extern void HandleEncFullEvent(
    Component com,
    CNMComListenerEncFull* param,
    EncListenerContext* ctx
    );

extern void HandleEncGetOutputEvent(
    Component               com,
    CNMComListenerEncDone*  param,
    EncListenerContext*     ctx
    );

extern void HandleEncCompleteSeqEvent(
    Component                       com,
    CNMComListenerEncCompleteSeq*   param,
    EncListenerContext*             ctx
    );

extern void HandleEncGetEncCloseEvent(
    Component               com,
    CNMComListenerEncClose* param,
    EncListenerContext*     ctx
    );
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif

