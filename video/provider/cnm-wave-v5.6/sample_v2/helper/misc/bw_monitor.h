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

#ifndef __BANDWIDTH_MONITOR_H__
#define __BANDWIDTH_MONITOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void*   BWCtx;
#define BW_PARAM_NOT_USED (-10)

BWCtx
BWMonitorSetup(
    CodecInst*  instance,
    BOOL        perFrame,
    Uint32      bwMode,
    char*       strLogDir
    );

/**
 * @brief           Releases all resources allocated on BWMonitorset().
 *                  @ctx is released internally.
 */
void
BWMonitorRelease(
    BWCtx        ctx
    );

void
BWMonitorReset(
    BWCtx        ctx
    );

void
BWMonitorUpdate(
    BWCtx        ctx,
    Uint32       numCores,
    Int32       recon_idx
    );

void 
    BWMonitorUpdatePrint(
    BWCtx       context,
    Uint32      picType,
    Int32       recon_idx,
    Uint32      w6_frame_num
    );


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BANDWIDTH_MONITOR_H__ */
 
