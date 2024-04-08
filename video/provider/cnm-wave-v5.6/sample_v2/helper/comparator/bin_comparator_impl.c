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

typedef struct {
    FILE*       fp;
} binCompContext;

BOOL BinComparator_Create(
    ComparatorImpl* impl,
    char*           path
    )
{
    binCompContext*    ctx;
    FILE*       fp;

    if ((fp=osal_fopen(path, "rb")) == NULL) { //prebuilt
        VLOG(ERR, "%s:%d failed to open bin file: %s\n", __FUNCTION__, __LINE__, path);
        return FALSE;
    }

    if ((ctx=(binCompContext*)osal_malloc(sizeof(binCompContext))) == NULL) {
        osal_fclose(fp);
        return FALSE;
    }

    ctx->fp        = fp;
    impl->context  = ctx;
    impl->eof      = FALSE;

    return TRUE;
}

BOOL BinComparator_Destroy(
    ComparatorImpl*  impl
    )
{
    binCompContext*    ctx = (binCompContext*)impl->context;

    osal_fclose(ctx->fp);
    osal_free(ctx);

    return TRUE;
}

BOOL BinComparator_Compare(
    ComparatorImpl* impl,
    void*           data,
    PhysicalAddress size,
    Uint32          insNum
    )
{
    Uint8*      pBin = NULL;
    binCompContext*    ctx = (binCompContext*)impl->context;
    BOOL        match = FALSE;

    pBin = (Uint8*)osal_malloc(size);
        
    osal_fread(pBin, size, 1, ctx->fp);

    if (IsEndOfFile(ctx->fp) == TRUE)
        impl->eof = TRUE;
    else
        impl->numOfFrames++;

    match = (osal_memcmp(data, (void*)pBin, size) == 0 ? TRUE : FALSE);
    if (match == FALSE) {
        FILE* fpGolden;
        FILE* fpOutput;
        char tmp[200];


        sprintf(tmp, "./golden_%s_%05d.bin", GetBasename(impl->filename), impl->curIndex-1);
        if ((fpGolden=osal_fopen(tmp, "wb")) == NULL) {
            VLOG(ERR, "Faild to create %s\n", tmp);
            osal_free(pBin);
            return FALSE;
        }
        //VLOG(INFO, "Saving... Golden Bin at %s\n", tmp);
        osal_fwrite(pBin, size, 1, fpGolden);
        osal_fclose(fpGolden);

        sprintf(tmp, "./encoded_%s_%05d.bin", GetBasename(impl->filename), impl->curIndex-1);
        if ((fpOutput=osal_fopen(tmp, "wb")) == NULL) {
            VLOG(ERR, "Faild to create %s\n", tmp);
            osal_free(pBin);
            return FALSE;
        }
        //VLOG(INFO, "Saving... encoded Bin at %s\n", tmp);
        osal_fwrite(data, size, 1, fpOutput);
        osal_fclose(fpOutput);
    }

    osal_free(pBin);

    return match;
}

BOOL BinComparator_Configure(
    ComparatorImpl*     impl,
    ComparatorConfType  type,
    void*               val
    )
{
    UNREFERENCED_PARAMETER(impl);
    UNREFERENCED_PARAMETER(type);
    UNREFERENCED_PARAMETER(val);
    return FALSE;
}

ComparatorImpl binComparatorImpl = {
    NULL,
    NULL,
    0,
    0,
    BinComparator_Create,
    BinComparator_Destroy,
    BinComparator_Compare,
    BinComparator_Configure,
    FALSE,
};
 
