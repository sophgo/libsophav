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
    EncHandle       handle;
    Uint32          streamBufCount;
    Uint32          streamBufSize;
    vpu_buffer_t*   bsBuffer;
    Uint32          coreIdx;
    EndianMode      streamEndian;
    char            bitstreamFileName[MAX_FILE_PATH];
    BitstreamReader bsReader;
    osal_file_t     fp;
    BOOL            ringBuffer;
    BOOL            ringBufferWrapEnable;
    Int32           productID;
    Comparator      es;
} ReaderContext;

static CNMComponentParamRet GetParameterReader(ComponentImpl* from, ComponentImpl* com, GetParameterCMD commandType, void* data) 
{
    ReaderContext*           ctx    = (ReaderContext*)com->context;
    BOOL                     result = TRUE;
    ParamEncBitstreamBuffer* bsBuf  = NULL;
    PortContainer*           container;

    switch(commandType) {
    case GET_PARAM_COM_IS_CONTAINER_CONUSUMED:
        container = (PortContainer*)data;
        container->consumed = TRUE;
        break;
    case GET_PARAM_READER_BITSTREAM_BUF:
        if (ctx->bsBuffer == NULL) return CNM_COMPONENT_PARAM_NOT_READY;
        bsBuf      = (ParamEncBitstreamBuffer*)data;
        bsBuf->bs  = ctx->bsBuffer;
        bsBuf->num = ctx->streamBufCount;
        break;
    default:
        return CNM_COMPONENT_PARAM_NOT_FOUND;
    }

    return (result == TRUE) ? CNM_COMPONENT_PARAM_SUCCESS : CNM_COMPONENT_PARAM_FAILURE;
}

static CNMComponentParamRet SetParameterReader(ComponentImpl* from, ComponentImpl* com, SetParameterCMD commandType, void* data) 
{
    BOOL result = TRUE;

    switch(commandType) {
    case SET_PARAM_COM_PAUSE:
        com->pause = *(BOOL*)data;
        break;
    default:
        result = FALSE;
        break;
    }

    return (result == TRUE) ? CNM_COMPONENT_PARAM_SUCCESS : CNM_COMPONENT_PARAM_FAILURE;
}

static BOOL ExecuteReader(ComponentImpl* com, PortContainer* in, PortContainer* out) 
{
    ReaderContext*          ctx     = (ReaderContext*)com->context;
    PortContainerES*        srcData = (PortContainerES*)in;
    BOOL                    success = TRUE;

    srcData->reuse = FALSE;
    if (srcData->size > 0 || 
        (ctx->ringBuffer == TRUE && ctx->ringBufferWrapEnable == FALSE && srcData->last == TRUE) ) {
        if ( ctx->ringBuffer == TRUE) {
            Uint8* buf = (Uint8*)osal_malloc(srcData->size);
#ifdef SUPPORT_READ_BITSTREAM_IN_ENCODER
            if ( PRODUCT_ID_W_SERIES(ctx->productID) ) {
                if (srcData->streamBufFull == TRUE) {
                    if (ctx->ringBufferWrapEnable == FALSE) {
                        osal_memcpy(ctx->bsBuffer, &srcData->newBsBuf, sizeof(*ctx->bsBuffer));
                    }
                }
            }
#else
            PhysicalAddress rd, paBsBufStart, paBsBufEnd;
            Int32           readSize;
            Uint32          room;

            rd = srcData->rdPtr;
            paBsBufStart = srcData->paBsBufStart;
            paBsBufEnd   = srcData->paBsBufEnd;
            readSize = srcData->size;

            if (ctx->ringBufferWrapEnable == TRUE ) {
                if (ctx->bsReader != 0) {
                    if ((rd+readSize) > paBsBufEnd) {
                        room = paBsBufEnd - rd;
                        vdi_read_memory(ctx->coreIdx, rd, buf, room,  ctx->streamEndian);
                        vdi_read_memory(ctx->coreIdx, paBsBufStart, buf+room, (readSize-room), ctx->streamEndian);
                    } 
                    else {
                        vdi_read_memory(ctx->coreIdx, rd, buf, readSize, ctx->streamEndian);
                    }
                }
                VPU_EncUpdateBitstreamBuffer(ctx->handle, readSize);
                if (ctx->fp) {
                    osal_fwrite(buf, readSize, 1, ctx->fp);
                }
            }
            else { //ring=1, wrap=0
                if (srcData->streamBufFull == TRUE || srcData->last == TRUE) { // read whole data at once and no UpdateBS
                    if (ctx->bsReader != 0) {
                        vdi_read_memory(ctx->coreIdx, rd, buf, readSize, ctx->streamEndian);
                    }
                    if (ctx->fp) {
                        osal_fwrite(buf, readSize, 1, ctx->fp);
                    }
                }
            }
            if (srcData->streamBufFull == TRUE) {
                if (ctx->ringBufferWrapEnable == FALSE) {
                    vdi_free_dma_memory(ctx->coreIdx, &srcData->buf, ENC_BS, ctx->handle->instIndex);
                    osal_memcpy(ctx->bsBuffer, &srcData->newBsBuf, sizeof(*ctx->bsBuffer));
                }
                srcData->buf.size = 0;
            }
#endif
            osal_free(buf);
        }
        else { /* line buffer mode */
#if !defined(SUPPORT_READ_BITSTREAM_IN_ENCODER)
            if (ctx->bsReader) {
                ((AbstractBitstreamReader*)ctx->bsReader)->streamBufFull = srcData->streamBufFull;
                if (0 != srcData->buf.phys_addr) {//for sequence change
                    if (FALSE == BitstreamReader_Act(ctx->bsReader, srcData->buf.phys_addr, 0, srcData->size, ctx->es, 1)) {
                        VLOG(ERR, "Error! Failed to compare bitstream!\n");
                        success = FALSE;
                    }
                }
                if(PRODUCT_ID_CODA_SERIES(ctx->productID) && TRUE == srcData->streamBufFull) {
                    VPU_ClearInterrupt(ctx->coreIdx);
                }
            }
#endif
            if (srcData->streamBufFull == TRUE) {
                srcData->buf.phys_addr = 0;
            }
        }
        if (TRUE == srcData->streamBufFull) {
            srcData->streamBufFull = FALSE;
        }
    }

    srcData->consumed = TRUE;
    com->terminate    = srcData->last;

    return success;
}

static BOOL PrepareReader(ComponentImpl* com, BOOL* done)
{
    ReaderContext*          ctx = (ReaderContext*)com->context;
    Uint32                  i;
    vpu_buffer_t*           bsBuffer;
    Uint32                  num = ctx->streamBufCount;
    BOOL                    success = TRUE;
    CNMComponentParamRet    ret;

    *done = FALSE;
    if (ctx->bsBuffer == NULL) {
        bsBuffer = (vpu_buffer_t*)osal_malloc(num*sizeof(vpu_buffer_t));
        for (i = 0; i < num; i++) {
            bsBuffer[i].size = ctx->streamBufSize;
            if (vdi_allocate_dma_memory(ctx->coreIdx, &bsBuffer[i], ENC_BS, /*ctx->handle->instIndex*/0) < 0) {
                VLOG(ERR, "%s:%d fail to allocate bitstream buffer\n", __FUNCTION__, __LINE__);
                osal_free(bsBuffer);
                return FALSE;
            }
        }
        ctx->bsBuffer   = bsBuffer;
    }

    ret = ComponentGetParameter(com, com->srcPort.connectedComponent, GET_PARAM_ENC_HANDLE, &ctx->handle); 
    if (ComponentParamReturnTest(ret, &success) == FALSE) {
        return success;
    }
#if !defined(SUPPORT_READ_BITSTREAM_IN_ENCODER)
    if ( ctx->bitstreamFileName[0] != 0) {
        ctx->bsReader = BitstreamReader_Create(BUFFER_MODE_TYPE_LINEBUFFER, ctx->bitstreamFileName, ctx->streamEndian, &ctx->handle);
        if (ctx->bsReader == NULL) {
            VLOG(ERR, "%s:%d Failed to BitstreamReader_Create\n", __FUNCTION__, __LINE__);
            return FALSE;
        }
    }
#endif

    *done = TRUE;
    return TRUE;
}

static void ReleaseReader(ComponentImpl* com)
{
    ReaderContext*  ctx = (ReaderContext*)com->context;
    Uint32          i   = 0;

    if (ctx->bsBuffer != NULL) {
        for (i = 0; i < ctx->streamBufCount ; i++) {
            if (ctx->bsBuffer[i].size)
                vdi_free_dma_memory(ctx->coreIdx, &ctx->bsBuffer[i], ENC_BS, 0);
        }
    }
}

static BOOL DestroyReader(ComponentImpl* com) 
{
    ReaderContext*  ctx = (ReaderContext*)com->context;

    if (ctx->bsBuffer != NULL) 
        osal_free(ctx->bsBuffer);
    if (ctx->fp) 
        osal_fclose(ctx->fp);
    if (ctx->es)
        Comparator_Destroy(ctx->es);
#ifdef SUPPORT_READ_BITSTREAM_IN_ENCODER
#else
    if (ctx->bsReader)
        osal_free(ctx->bsReader);
#endif
    osal_free(ctx);

    return TRUE;
}

static Component CreateReader(ComponentImpl* com, CNMComponentConfig* componentParam) 
{
    ReaderContext* ctx;

    com->context = osal_malloc(sizeof(ReaderContext));
    ctx = (ReaderContext*)com->context;
    osal_memset((void*)ctx, 0, sizeof(ReaderContext));

    strcpy(ctx->bitstreamFileName, componentParam->testEncConfig.bitstreamFileName);
    ctx->handle       = NULL;
    ctx->coreIdx      = componentParam->testEncConfig.coreIdx;
    ctx->productID    = componentParam->testEncConfig.productId;
    ctx->streamEndian = (EndianMode)componentParam->testEncConfig.stream_endian;

    ctx->streamBufCount = componentParam->encOpenParam.streamBufCount;
    ctx->streamBufSize  = componentParam->encOpenParam.streamBufSize;
    ctx->ringBuffer     = componentParam->encOpenParam.ringBufferEnable;
    ctx->ringBufferWrapEnable = componentParam->encOpenParam.ringBufferWrapEnable;
    ctx->bsReader = NULL;
    ctx->es = NULL;
    return (Component)com;
}

ComponentImpl readerComponentImpl = {
    "reader",
    NULL,
    {0,},
    {0,},
    sizeof(PortContainer),
    5,
    CreateReader,
    GetParameterReader,
    SetParameterReader,
    PrepareReader,
    ExecuteReader,
    ReleaseReader,
    DestroyReader,
};

