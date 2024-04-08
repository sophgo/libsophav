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
#include <stdarg.h>
#include "main_helper.h"

extern YuvFeederImpl loaderYuvFeederImpl;
extern YuvFeederImpl cfbcYuvFeederImpl;

BOOL loaderYuvFeeder_Create(
    YuvFeederImpl *impl,
    const char* path,
    Uint32   packed,
    Uint32   fbStride,
    Uint32   fbHeight
);

BOOL loaderYuvFeeder_Destroy(
    YuvFeederImpl *impl
);

BOOL loaderYuvFeeder_Feed(
    YuvFeederImpl*  impl,
    Int32           coreIdx,
    FrameBuffer*    fb,
    size_t          picWidth,
    size_t          picHeight,
    Uint32          srcFbIndex,
    ENC_subFrameSyncCfg *subFrameSyncConfig,
    void*           arg
);

BOOL loaderYuvFeeder_Configure(
    YuvFeederImpl* impl,
    Uint32         cmd,
    YuvInfo        yuv
);

BOOL cfbcYuvFeeder_Create(
    YuvFeederImpl *impl,
    const char* path,
    Uint32   packed,
    Uint32   fbStride,
    Uint32   fbHeight
);

BOOL cfbcYuvFeeder_Destroy(
    YuvFeederImpl *impl
);

BOOL cfbcYuvFeeder_Feed(
    YuvFeederImpl*  impl,
    Int32           coreIdx,
    FrameBuffer*    fb,
    size_t          picWidth,
    size_t          picHeight,
    Uint32          srcFbIndex,
    ENC_subFrameSyncCfg *subFrameSyncConfig,
    void*           arg
);

BOOL cfbcYuvFeeder_Configure(
    YuvFeederImpl* impl,
    Uint32         cmd,
    YuvInfo        yuv
);



static BOOL yuvYuvFeeder_Create(
    YuvFeederImpl *impl,
    const char*   path,
    Uint32        packed,
    Uint32        fbStride,
    Uint32        fbHeight)
{
    yuvContext*   ctx;
    osal_file_t*  fp;
    Uint8*        pYuv;

    if ((fp=osal_fopen(path, "rb")) == NULL) {
        VLOG(ERR, "%s:%d failed to open yuv file: %s\n", __FUNCTION__, __LINE__, path);
        return FALSE;
    }

    if ( packed == 1 )
        pYuv = osal_malloc(fbStride*fbHeight*3*2*2);//packed, unsigned short
    else
        pYuv = osal_malloc(fbStride*fbHeight*3*2);//unsigned short

    if ((ctx=(yuvContext*)osal_malloc(sizeof(yuvContext))) == NULL) {
        osal_free(pYuv);
        osal_fclose(fp);
        return FALSE;
    }

    osal_memset(ctx, 0, sizeof(yuvContext));

    ctx->fp       = fp;
    ctx->pYuv     = pYuv;
    impl->context = ctx;

    return TRUE;
}

static BOOL yuvYuvFeeder_Destroy(
    YuvFeederImpl *impl
    )
{
    yuvContext*    ctx = (yuvContext*)impl->context;

    osal_fclose(ctx->fp);
    osal_free(ctx->pYuv);
    osal_free(ctx);
    return TRUE;
}

static BOOL yuvYuvFeeder_Feed(
    YuvFeederImpl*  impl,
    Int32           coreIdx,
    FrameBuffer*    fb,
    size_t          picWidth,
    size_t          picHeight,
    Uint32          srcFbIndex,
    ENC_subFrameSyncCfg *subFrameSyncConfig,
    void*           arg
    )
{
    yuvContext* ctx = (yuvContext*)impl->context;
    Uint8*      pYuv = ctx->pYuv;
    size_t      frameSize;
    size_t      frameSizeY;
    size_t      frameSizeC;
    Int32       bitdepth=0;
    Int32       yuv3p4b=0;
    Int32       packedFormat=0;
    Uint32      outWidth=0;
    Uint32      outHeight=0;
    Uint32      subFrameSyncSrcWriteMode = subFrameSyncConfig->subFrameSyncSrcWriteMode;
    BOOL        subFrameSyncEn = subFrameSyncConfig->subFrameSyncOn; 
    Uint32      writeSrcLine = subFrameSyncSrcWriteMode & ~REMAIN_SRC_DATA_WRITE;
    if ( subFrameSyncEn == TRUE && subFrameSyncSrcWriteMode == SRC_0LINE_WRITE ) {
        return TRUE;
    }
    CalcYuvSize(fb->format, picWidth, picHeight, fb->cbcrInterleave, &frameSizeY, &frameSizeC, &frameSize, &bitdepth, &packedFormat, &yuv3p4b);

    // Load source one picture image to encode to SDRAM frame buffer.
    if (!osal_fread(pYuv, 1, frameSize, ctx->fp)) {
        if (ctx->NoLastFrameCheck) {
            osal_fseek(ctx->fp, 0, SEEK_SET);
            osal_fread(pYuv, 1, frameSize, ctx->fp);
            VLOG(TRACE, "NoLastFrameCheckre enabled(rewind input data)\n");
        } else {
            if (osal_feof(ctx->fp) == 0) 
                VLOG(ERR, "Yuv Data osal_fread failed file handle is 0x%x\n", ctx->fp);
            return FALSE;
        }
    }

    if ( subFrameSyncEn ==TRUE && (subFrameSyncSrcWriteMode & REMAIN_SRC_DATA_WRITE) == 0)
        osal_fseek(ctx->fp, -((long)frameSize), SEEK_CUR);

    //Already wrote in SRC_XXXLINE_WRITE mode
    if (subFrameSyncEn == TRUE && subFrameSyncSrcWriteMode & REMAIN_SRC_DATA_WRITE && writeSrcLine == picHeight) {
        return TRUE;
    }
    if (fb->mapType == LINEAR_FRAME_MAP ) {
        outWidth  = (yuv3p4b&&packedFormat==0) ? ((picWidth+31)/32)*32  : picWidth;
        outHeight = (yuv3p4b) ? ((picHeight+7)/8)*8 : picHeight;

        if ( yuv3p4b  && packedFormat) {
            outWidth = ((picWidth*2)+2)/3*4;
        }
        else if(packedFormat) {
            outWidth *= 2;           // 8bit packed mode(YUYV) only. (need to add 10bit cases later)
            if (bitdepth != 0)       // 10bit packed
                outWidth *= 2;
        }
#ifdef SUPPORT_ENC_BIG_ENDIAN_YUV
#else
        //yuv input in REF-C and yuv input in FPGA are different. actually need endian setting.
        if (bitdepth == 10 && yuv3p4b == 0)
            byte_swap((unsigned char *)pYuv, (int)frameSize); //little endian 10bit yuv to big endian 10bit yuv
#endif
#if defined(CNM_FPGA_VU440_INTERFACE) || defined(CNM_FPGA_VU19P_INTERFACE)
        LoadYuvImageBurstFormat(coreIdx, pYuv, outWidth, outHeight, fb, ctx->srcPlanar);
#else
        LoadYuvImageByYCbCrLine(impl->handle, coreIdx, pYuv, outWidth, outHeight, fb, arg, subFrameSyncConfig, srcFbIndex);
#endif
    }
    else {
        TiledMapConfig  mapConfig;

        osal_memset((void*)&mapConfig, 0x00, sizeof(TiledMapConfig));
        if (arg != NULL) {
            osal_memcpy((void*)&mapConfig, arg, sizeof(TiledMapConfig));
        }

        LoadTiledImageYuvBurst(impl->handle, coreIdx, pYuv, picWidth, picHeight, fb, mapConfig);
    }

    return TRUE;
}

static BOOL yuvYuvFeeder_Configure(
    YuvFeederImpl* impl,
    Uint32         cmd,
    YuvInfo        yuv
    )
{
    yuvContext* ctx = (yuvContext*)impl->context;
    UNREFERENCED_PARAMETER(cmd);

    ctx->fbStride         = yuv.srcStride;
    ctx->cbcrInterleave   = yuv.cbcrInterleave;
    ctx->srcPlanar        = yuv.srcPlanar;
    ctx->NoLastFrameCheck = yuv.NoLastFrameCheck;

    return TRUE;
}

YuvFeederImpl yuvYuvFeederImpl = {
    NULL,
    yuvYuvFeeder_Create,
    yuvYuvFeeder_Feed,
    yuvYuvFeeder_Destroy,
    yuvYuvFeeder_Configure
};

/*lint -esym(438, ap) */
YuvFeeder YuvFeeder_Create(
    Uint32          type,
    const char*     srcFilePath,
    YuvInfo         yuvInfo
    )
{
    AbstractYuvFeeder *feeder;
    YuvFeederImpl     *impl;
    BOOL              success = FALSE;

    if (srcFilePath == NULL) {
        VLOG(ERR, "%s:%d src path is NULL\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    // Create YuvFeeder for type.
    switch (type) {
    case SOURCE_YUV:
        impl = osal_malloc(sizeof(YuvFeederImpl));
        impl->Create    = yuvYuvFeeder_Create;
        impl->Feed      = yuvYuvFeeder_Feed;
        impl->Destroy   = yuvYuvFeeder_Destroy;
        impl->Configure = yuvYuvFeeder_Configure;
        if ((success=impl->Create(impl, srcFilePath, yuvInfo.packedFormat, yuvInfo.srcStride, yuvInfo.srcHeight)) == TRUE) {
            impl->Configure(impl, 0, yuvInfo);
        }
        break;
    case SOURCE_YUV_WITH_LOADER:
        impl = osal_malloc(sizeof(YuvFeederImpl));
        impl->Create    = loaderYuvFeeder_Create;
        impl->Feed      = loaderYuvFeeder_Feed;
        impl->Destroy   = loaderYuvFeeder_Destroy;
        impl->Configure = loaderYuvFeeder_Configure;
        if ((success=impl->Create(impl, srcFilePath, yuvInfo.packedFormat, yuvInfo.srcStride, yuvInfo.srcHeight)) == TRUE) {
            impl->Configure(impl, 0, yuvInfo);
        }
        break;
    default:
        VLOG(ERR, "%s:%d Unknown YuvFeeder Type\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    if (success == FALSE)
        return NULL;

    feeder = (AbstractYuvFeeder*)osal_malloc(sizeof(AbstractYuvFeeder));
    if ( !feeder )
        return NULL;
    feeder->impl = impl;

    return feeder;
}
/*lint +esym(438, ap) */

BOOL YuvFeeder_Destroy(
    YuvFeeder feeder
    )
{
    YuvFeederImpl*     impl = NULL;
    AbstractYuvFeeder* yuvfeeder =  (AbstractYuvFeeder*)feeder;

    if (yuvfeeder == NULL) {
        VLOG(ERR, "%s:%d Invalid handle\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    impl = yuvfeeder->impl;

    impl->Destroy(impl);
    osal_free(impl);
    return TRUE;
}

BOOL YuvFeeder_Feed(
    YuvFeeder       feeder,
    Uint32          coreIdx,
    FrameBuffer*    fb,
    size_t          picWidth,
    size_t          picHeight,
    Uint32          srcFbIndex,
    ENC_subFrameSyncCfg *subFrameSyncConfig,
    void*           arg
    )
{
    YuvFeederImpl*      impl = NULL;
    AbstractYuvFeeder*  absFeeder = (AbstractYuvFeeder*)feeder;
    Int32               ret;
    if (absFeeder == NULL) {
        VLOG(ERR, "%s:%d Invalid handle\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    impl = absFeeder->impl;

    ret = impl->Feed(impl, coreIdx, fb, picWidth, picHeight, srcFbIndex, arg, subFrameSyncConfig);

    return ret;
}

