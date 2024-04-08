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

#ifndef __DECODER_LISTENER_H__
#define __DECODER_LISTENER_H__
#include "cnm_app.h"
#include "misc/bw_monitor.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct DecListenerContext {
    Component           renderer;
    Comparator          comparator;
    Int32               lastSeqNo;
    DecGetFramebufInfo  fbInfo;
    Uint32              compareType;
    BOOL                match;
    Uint32              notDecodedCount;
    /* performance & bandwidth */
    BOOL                performance;
    Uint32              bandwidth;
    PFCtx               pfCtx;
    BWCtx*              bwCtx;
    Uint32              fps;
    Uint32              pfClock;
    char                inputPath[MAX_FILE_PATH];
    Uint32              numVCores;
    CodStd              bitFormat;
    Int32               productId;
    BOOL                enableScaler;
    Int32               scale_width;
    Int32               scale_height;
} DecListenerContext;

int DecoderListener(Component com, Uint64 event, void* data, void* context);
BOOL SetupDecListenerContext(DecListenerContext* ctx, CNMComponentConfig* config, Component renderer);
void ClearDecListenerContext(DecListenerContext* ctx);
void HandleDecCompleteSeqEvent(Component com, CNMComListenerDecCompleteSeq* param, DecListenerContext* ctx);
void HandleDecRegisterFbEvent(Component com, CNMComListenerDecRegisterFb* param, DecListenerContext* ctx);
void HandleDecGetOutputEvent(Component com, CNMComListenerDecDone* param, DecListenerContext* ctx);
void HandleDecCloseEvent(Component com, CNMComListenerDecClose* param, DecListenerContext* ctx);
void HandleDecInterruptEvent(Component com, CNMComListenerDecInt* param, DecListenerContext* ctx);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif 

