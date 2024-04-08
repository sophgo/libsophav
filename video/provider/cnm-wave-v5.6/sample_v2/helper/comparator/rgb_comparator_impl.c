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
    BOOL        cbcrInterleave;
    FrameBufferFormat format;
    char        *path;
    Uint32      isVp9;
    Uint8*      lastRgb;
} RgbCompContext;

static Uint32 Calculate(
    RgbCompContext*    ctx
    )
{
    Uint32      lumaSize   = 0;
    Uint32      chromaSize = 0;
    Uint32      frameSize  = 0;
    Uint32      frames = 0;
    Uint32      width  = ctx->width;
    Uint32      height = ctx->height;
    Uint64      endPos = 0;
    FrameBufferFormat format = ctx->format;
#if defined(PLATFORM_LINUX) || defined(PLATFORM_QNX)
    struct   stat  file_info;
#endif
#ifdef PLATFORM_NON_OS
#ifdef LIB_C_STUB
    struct   stat  file_info;
#endif
#endif

    lumaSize = width * height;
    switch (format) {
    default:
        VLOG(ERR, "%s:%d Invalid format: %d\n", __FILE__, __LINE__, format);
    }
    frameSize = lumaSize + chromaSize;

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

    return frames;
}

BOOL RGBComparator_Create(
    ComparatorImpl* impl,
    char*           path
    )
{
    RgbCompContext*        ctx;
    osal_file_t*    fp;

    if ((fp=osal_fopen(path, "rb")) == NULL) {
        VLOG(ERR, "%s:%d failed to open rgb file: %s\n", __FUNCTION__, __LINE__, path);
        return FALSE;
    }

    if ((ctx=(RgbCompContext*)osal_malloc(sizeof(RgbCompContext))) == NULL) {
        osal_fclose(fp);
        return FALSE;
    }
    osal_memset((void*)ctx, 0x00, sizeof(RgbCompContext));

    ctx->fp        = fp;
    ctx->path      = path;
    ctx->lastRgb   = NULL;
    impl->context  = ctx;
    impl->eof      = FALSE;

    return TRUE;
}

BOOL RGBComparator_Destroy(
    ComparatorImpl*  impl
    )
{
    RgbCompContext*    ctx = (RgbCompContext*)impl->context;

    osal_fclose(ctx->fp);
    if (ctx->lastRgb) 
        osal_free(ctx->lastRgb);
    osal_free(ctx);

    return TRUE;
}

BOOL RGBComparator_Compare(
    ComparatorImpl* impl,
    void*           data,
    PhysicalAddress size,
    Uint32          insNUm
    )
{
    Uint8*      p_rgb = NULL;
    RgbCompContext*    ctx = (RgbCompContext*)impl->context;
    BOOL        match = FALSE;

    if (data == (void *)COMPARATOR_SKIP) {
        int fpos;
        fpos = osal_ftell(ctx->fp);
        osal_fseek(ctx->fp, fpos+size, SEEK_SET);

        if (IsEndOfFile(ctx->fp) == TRUE)
            impl->eof = TRUE;

        return TRUE;
    }

    if (impl->usePrevDataOneTime == TRUE) {
        p_rgb = ctx->lastRgb;
        impl->usePrevDataOneTime = FALSE;
    }
    else {
        if (ctx->lastRgb) {
            osal_free(ctx->lastRgb);
            ctx->lastRgb = NULL;
        }

        p_rgb = (Uint8*)osal_malloc(size);
        osal_fread(p_rgb, 1, size, ctx->fp);
        ctx->lastRgb = p_rgb;
    }

    if (IsEndOfFile(ctx->fp) == TRUE)
        impl->eof = TRUE;

    match = (osal_memcmp(data, (void*)p_rgb, size) == 0 ? TRUE : FALSE);
    if (match == FALSE) {
        osal_file_t* fpGolden;
        osal_file_t* fpOutput;
        char tmp[200];

        VLOG(ERR, "MISMATCH WITH GOLDEN RGB at %d frame\n", impl->curIndex);
        sprintf(tmp, "./golden.rgb");
        if ((fpGolden=osal_fopen(tmp, "wb")) == NULL) {
            VLOG(ERR, "Faild to create golden.rgb\n");
            osal_free(p_rgb);
            return FALSE;
        }
        VLOG(INFO, "Saving... Golden RGB at %s\n", tmp);

        sprintf(tmp, "./decoded.rgb");
        osal_fwrite(p_rgb, 1, size, fpGolden);
        osal_fclose(fpGolden);
        if ((fpOutput=osal_fopen(tmp, "wb")) == NULL) {
            VLOG(ERR, "Faild to create golden.rgb\n");
            osal_free(p_rgb);
            return FALSE;
        }
        VLOG(INFO, "Saving... decoded RGB at %s\n", tmp);
        osal_fwrite(data, 1, size, fpOutput);
        osal_fclose(fpOutput);
    }

    return match;
}

BOOL RGBComparator_Configure(
    ComparatorImpl*     impl,
    ComparatorConfType  type,
    void*               val
    )
{
    PictureInfo*        rgb = NULL;
    RgbCompContext*            ctx = (RgbCompContext*)impl->context;
    BOOL                ret = TRUE;

    switch (type) {
    case COMPARATOR_CONF_SET_PICINFO:
        rgb = (PictureInfo*)val;
        ctx->width  = rgb->width;
        ctx->height = rgb->height;
        ctx->format = rgb->format;
        ctx->cbcrInterleave = rgb->cbcrInterleave;
        //can not calculate a sequence changed RGB
        impl->numOfFrames   = Calculate(ctx);
        break;
    default:
        ret = FALSE;
        break;
    }
    return ret;
}

BOOL RGBComparator_Rewind(
    ComparatorImpl*     impl
    )
{
    RgbCompContext*    ctx = (RgbCompContext*)impl->context;
    Int32       ret;

    if ((ret=osal_fseek(ctx->fp, 0, SEEK_SET)) != 0) {
        VLOG(ERR, "%s:%d failed to osal_fseek(ret:%d)\n", __FUNCTION__, __LINE__, ret);
        return FALSE;
    }

    impl->eof = FALSE;

    return TRUE;
}

ComparatorImpl rgbComparatorImpl = {
    NULL,
    NULL,
    0,
    0,
    RGBComparator_Create,
    RGBComparator_Destroy,
    RGBComparator_Compare,
    RGBComparator_Configure,
    RGBComparator_Rewind,
    FALSE,
};
 
