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

#include "config.h"
#include "main_helper.h"
#if defined(PLATFORM_LINUX) || defined(PLATFORM_QNX)
#include <sys/types.h>
#include <sys/stat.h>
#endif
#ifdef PLATFORM_NON_OS
#ifdef LIB_C_STUB
#include <sys/types.h>
#include <sys/stat.h>
#endif
#endif

typedef struct {
    osal_file_t fp;
    Uint32      width;
    Uint32      height;
    Uint32      frameSize;
    Uint32      lumaSize;
    BOOL        isMonochrome;
    BOOL        cbcrInterleave;
    FrameBufferFormat format;
    char        *path;
    Uint32      isVp9;
    Uint8*      lastYuv;
} YuvCompContext;

static Uint32 Calculate(
    YuvCompContext*    ctx
    )
{
    Uint32      lumaSize   = 0;
    Uint32      chromaSize = 0;
    Uint32      frameSize  = 0;
    Uint32      frames     = 0;
    Uint32      width      = ctx->width;
    Uint32      height     = ctx->height;
    Uint64      endPos     = 0;
    BOOL        cbcrInterleave = ctx->cbcrInterleave;
    FrameBufferFormat format   = ctx->format;
#if defined(PLATFORM_LINUX) || defined(PLATFORM_QNX)
    struct   stat  file_info;
#endif
#ifdef PLATFORM_NON_OS
#ifdef LIB_C_STUB
    struct   stat  file_info;
#endif
#endif

    CalcRealFrameSize(format, width, height, cbcrInterleave, &lumaSize, &chromaSize, &frameSize);

#ifdef PLATFORM_WIN32
#if (_MSC_VER == 1200)
    osal_fseek(ctx->fp, 0, SEEK_END);
    endPos = ftell(ctx->fp);
    osal_fseek(ctx->fp, 0, SEEK_SET);
#else
    _fseeki64((FILE*)ctx->fp, 0LL, SEEK_END);
    endPos = _ftelli64((FILE*)ctx->fp);
    _fseeki64((FILE*)ctx->fp, 0LL, SEEK_SET);
#endif
#else
    stat( ctx->path, &file_info);
    endPos = file_info.st_size;
#endif

    frames = (Uint32)(endPos / frameSize);

    if (endPos % frameSize) {
        VLOG(WARN, "%s:%d Mismatch - file_size: %llu frameSize: %d\n",
            __FUNCTION__, __LINE__, endPos, frameSize);
    }
    ctx->frameSize  = frameSize;
    ctx->lumaSize   = lumaSize;

    return frames;
}

BOOL YUVComparator_Create(
    ComparatorImpl* impl,
    char*           path
    )
{
    YuvCompContext*        ctx;
    osal_file_t*    fp;

    if ((fp=osal_fopen(path, "rb")) == NULL) {
        VLOG(ERR, "%s:%d failed to open yuv file: %s\n", __FUNCTION__, __LINE__, path);
        return FALSE;
    }

    if ((ctx=(YuvCompContext*)osal_malloc(sizeof(YuvCompContext))) == NULL) {
        osal_fclose(fp);
        return FALSE;
    }
    osal_memset((void*)ctx, 0x00, sizeof(YuvCompContext));

    ctx->fp             = fp;
    ctx->path           = path;
    ctx->lastYuv        = NULL;
    ctx->isMonochrome   = FALSE;
    impl->context       = ctx;
    impl->eof           = FALSE;

    return TRUE;
}

BOOL YUVComparator_Destroy(
    ComparatorImpl*  impl
    )
{
    YuvCompContext*    ctx = (YuvCompContext*)impl->context;

    osal_fclose(ctx->fp);
    if (ctx->lastYuv) 
        osal_free(ctx->lastYuv);
    osal_free(ctx);

    return TRUE;
}

BOOL YUVComparator_Compare(
    ComparatorImpl* impl,
    void*           data,
    PhysicalAddress size,
    Uint32          insNUm
    )
{
    Uint8*      pYuv = NULL;
    YuvCompContext*    ctx = (YuvCompContext*)impl->context;
    BOOL        match = FALSE;

    if (ctx->isMonochrome == TRUE) {
        size = ctx->lumaSize;
        VLOG(WARN, "Monochrome Comparison, Frame size : %d, Luma size : %d \n", ctx->frameSize, ctx->lumaSize);
    }

    if (data == (void *)COMPARATOR_SKIP) {
        int fpos;
        fpos = osal_ftell(ctx->fp);
        osal_fseek(ctx->fp, fpos+size, SEEK_SET);

        if (IsEndOfFile(ctx->fp) == TRUE)
            impl->eof = TRUE;

        return TRUE;
    }

    if (impl->usePrevDataOneTime == TRUE) {
        pYuv = ctx->lastYuv;
        impl->usePrevDataOneTime = FALSE;
    }
    else {
        if (ctx->lastYuv) {
            osal_free(ctx->lastYuv);
            ctx->lastYuv = NULL;
        }

        pYuv = (Uint8*)osal_malloc(size);
        osal_fread(pYuv, 1, size, ctx->fp);
        ctx->lastYuv = pYuv;
    }

    if (IsEndOfFile(ctx->fp) == TRUE)
        impl->eof = TRUE;

    match = (osal_memcmp(data, (void*)pYuv, size) == 0 ? TRUE : FALSE);
    if (match == FALSE) {
        osal_file_t* fpGolden;
        osal_file_t* fpOutput;
        char tmp[MAX_FILE_PATH];

        VLOG(ERR, "MISMATCH WITH GOLDEN YUV at %d frame\n", impl->curIndex);
        sprintf(tmp, "./golden.yuv");
        if ((fpGolden=osal_fopen(tmp, "wb")) == NULL) {
            VLOG(ERR, "Faild to create golden.yuv\n");
            osal_free(pYuv);
            return FALSE;
        }
        VLOG(INFO, "Saving... Golden YUV at %s\n", tmp);

        sprintf(tmp, "./decoded.yuv");
        osal_fwrite(pYuv, 1, size, fpGolden);
        osal_fclose(fpGolden);
        if ((fpOutput=osal_fopen(tmp, "wb")) == NULL) {
            VLOG(ERR, "Faild to create golden.yuv\n");
            osal_free(pYuv);
            return FALSE;
        }
        VLOG(INFO, "Saving... decoded YUV at %s\n", tmp);
        osal_fwrite(data, 1, size, fpOutput);
        osal_fclose(fpOutput);
    }

    return match;
}

BOOL YUVComparator_Configure(
    ComparatorImpl*     impl,
    ComparatorConfType  type,
    void*               val
    )
{
    PictureInfo*        yuv = NULL;
    YuvCompContext*     ctx = (YuvCompContext*)impl->context;
    BOOL                ret = TRUE;

    switch (type) {
    case COMPARATOR_CONF_SET_PICINFO:
        yuv = (PictureInfo*)val;
        ctx->width  = yuv->width;
        ctx->height = yuv->height;
        ctx->format = yuv->format;
        ctx->cbcrInterleave = yuv->cbcrInterleave;
        //can not calculate a sequence changed YUV
        impl->numOfFrames   = Calculate(ctx);
        break;
    case COMPARATOR_CONF_SET_MONOCHROME:
        ctx->isMonochrome = TRUE;
        break;
    case COMPARATOR_CONF_SET_NOT_MONOCHROME:
        ctx->isMonochrome = FALSE;
        break;
    default:
        ret = FALSE;
        break;
    }
    return ret;
}

BOOL YUVComparator_Rewind(
    ComparatorImpl*     impl
    )
{
    YuvCompContext*    ctx = (YuvCompContext*)impl->context;
    Int32       ret;

    if ((ret=osal_fseek(ctx->fp, 0, SEEK_SET)) != 0) {
        VLOG(ERR, "%s:%d failed to osal_fseek(ret:%d)\n", __FUNCTION__, __LINE__, ret);
        return FALSE;
    }

    impl->eof = FALSE;

    return TRUE;
}

ComparatorImpl yuvComparatorImpl = {
    NULL,
    NULL,
    0,
    0,
    YUVComparator_Create,
    YUVComparator_Destroy,
    YUVComparator_Compare,
    YUVComparator_Configure,
    YUVComparator_Rewind,
    FALSE,
};
 
