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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "vpuapifunc.h"
#include "main_helper.h"

#define MAX_FEEDING_SIZE        0x400000        /* 4MBytes */

typedef struct FeederEsContext {
    osal_file_t fp;
    Uint32      feedingSize;
    BOOL        eos;
} FeederEsContext;

void* BSFeederSizePlusEs_Create(
    const char* path,
    CodStd      codecId
    )
{
    osal_file_t     fp = NULL;
    FeederEsContext*  context=NULL;

    UNREFERENCED_PARAMETER(codecId);

    if ((fp=osal_fopen(path, "rb")) == NULL) {
        VLOG(ERR, "%s:%d failed to open %s\n", __FUNCTION__, __LINE__, path);
        return NULL;
    }

    context = (FeederEsContext*)osal_malloc(sizeof(FeederEsContext));
    if (context == NULL) {
        VLOG(ERR, "%s:%d failed to allocate memory\n", __FUNCTION__, __LINE__);
        osal_fclose(fp);
        return NULL;
    }

    context->fp          = fp;
    context->feedingSize = 0;
    context->eos         = FALSE;

    return (void*)context;
}

BOOL BSFeederSizePlusEs_Destroy(
    void* feeder
    )
{
    FeederEsContext* context = (FeederEsContext*)feeder;

    if (context == NULL) {
        VLOG(ERR, "%s:%d Null handle\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    if (context->fp)
        osal_fclose(context->fp);

    osal_free(context);

    return TRUE;
}

Int32 BSFeederSizePlusEs_Act(
    void*       feeder,
    BSChunk*    chunk
    )
{
    FeederEsContext*  context = (FeederEsContext*)feeder;
    size_t          nRead;
    Uint32          chunkSize = 0;

    if (context == NULL) {
        VLOG(ERR, "%s:%d Null handle\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    if (context->eos == TRUE) {
        return 0;
    }

    nRead = osal_fread(&chunkSize, 1, 4, context->fp);
    if (nRead < 4) {
        chunk->eos = TRUE;
        chunk->size = 0;
        return 0;
    }
    nRead = osal_fread(chunk->data, 1, chunkSize, context->fp);
    if ((Int32)nRead < 0) {
        VLOG(ERR, "%s:%d failed to read bitstream(errno: %d)\n", __FUNCTION__, __LINE__, errno);
        return 0;
    }
    else if (nRead < chunkSize) {
        context->eos = TRUE;
    }
    chunk->size = chunkSize;

    return nRead;
}

BOOL BSFeederSizePlusEs_Rewind(
    void*       feeder
    )
{
    FeederEsContext*  context = (FeederEsContext*)feeder;
    Int32           ret;

    if ((ret=osal_fseek(context->fp, 0, SEEK_SET)) != 0) {
        VLOG(ERR, "%s:%d failed osal_fseek(ret:%d)\n", __FUNCTION__, __LINE__, ret);
        return FALSE;
    }

    return TRUE;
}

