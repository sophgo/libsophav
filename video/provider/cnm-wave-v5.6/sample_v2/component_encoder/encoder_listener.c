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

#include "cnm_app.h"
#include "encoder_listener.h"
#include "misc/debug.h"
#include "misc/bw_monitor.h"
#include "string.h"

void HandleEncOpenEvent(Component com, CNMComListenerEncOpen* param, EncListenerContext* ctx)
{
    return;
}

void HandleEncHandlingIntEvent(Component com, CNMComListenerHandlingInt* param, EncListenerContext* ctx)
{
    int productID = VPU_GetProductId(ctx->coreIdx);
    if (TRUE == PRODUCT_ID_W5_SERIES(productID)) {
        if (ctx->bwCtx != NULL) {
            BWMonitorUpdate(ctx->bwCtx, 1, BW_PARAM_NOT_USED);
        }
    }
}

void HandleEncFullEvent(Component com, CNMComListenerEncFull* param, EncListenerContext* ctx)
{
#ifdef SUPPORT_READ_BITSTREAM_IN_ENCODER
#define STREAM_READ_CNT 2
    Uint8* buf = (Uint8*)osal_malloc(param->size);
    int i;
    int productID = VPU_GetProductId(ctx->coreIdx);
    AbstractBitstreamReader* absReader = (AbstractBitstreamReader*)param->bsReader;

    if (NULL == buf) {
        VLOG(ERR, "%s:%d NULL point exception\n", __FUNCTION__, __LINE__);
        return;
    }

    if (TRUE == PRODUCT_ID_W_SERIES(productID)) {
        for ( i = 0 ; i < STREAM_READ_CNT ; i++ ) {
            if ((param->rdPtr+param->size) > param->paBsBufEnd) {
                Uint32          room;
                room = param->paBsBufEnd - param->rdPtr;
                vdi_read_memory(ctx->coreIdx, param->rdPtr, buf, room,  ctx->streamEndian);
                vdi_read_memory(ctx->coreIdx, param->paBsBufStart, buf+room, (param->size-room), ctx->streamEndian);
            }
            else {
                vdi_read_memory(ctx->coreIdx, param->rdPtr, buf, param->size, ctx->streamEndian);
            }

            if ( i== 0 ) {
                if (ctx->ringBufferEnable == TRUE) {
                    if (ctx->ringBufferWrapEnable == TRUE) {
                        // --ringBuffer=1
                        VPU_EncUpdateBitstreamBuffer(param->handle, param->size);
                    }
                    else {
                        // --ringBuffer=2
                        vdi_free_dma_memory(ctx->coreIdx, &param->vbStream, ENC_BS, param->handle->instIndex);
                    }
                }
                else
                    VPU_EncUpdateBitstreamBuffer(param->handle, param->size);
            }

        }
    } else { //CODA series
        if (NULL != param->fullStreamBuf) {
            osal_memcpy(buf, param->fullStreamBuf, param->size);
            if (FALSE == (ctx->match = Comparator_Act(ctx->es, buf, param->size, param->handle->instIndex)) ) {
                VLOG(ERR, "%s:%d Bitstream Mismatch\n", __FUNCTION__, __LINE__);
            }
        }
    }

    if (param->fp) {
        osal_fwrite(buf, param->size, 1, param->fp);
    }
    else if (TRUE == PRODUCT_ID_W_SERIES(productID) && absReader->fp ) {
        osal_fwrite(buf, param->size, 1, absReader->fp);
    }

    osal_free(buf);
    if (ctx->match == FALSE) 
        CNMAppStop();
#endif
}

void HandleEncGetEncCloseEvent(Component com, CNMComListenerEncClose* param, EncListenerContext* ctx)
{
    if (ctx->pfCtx != NULL) {
        PFMonitorRelease(ctx->pfCtx);
    }
    if (ctx->bwCtx != NULL) {
        BWMonitorRelease(ctx->bwCtx);
    }
}

void HandleEncCompleteSeqEvent(Component com, CNMComListenerEncCompleteSeq* param, EncListenerContext* ctx)
{
    if (ctx->performance == TRUE) {
        Uint32 fps = (ctx->fps == 0) ? 30 : ctx->fps;
        ctx->pfCtx = PFMonitorSetup(ctx->coreIdx, 0, ctx->pfClock, fps, GetBasename((const char *)ctx->cfgFileName), 1);
    }
    if (ctx->bandwidth != 0) {
        ctx->bwCtx = BWMonitorSetup(param->handle, TRUE, (ctx->bandwidth-1),GetBasename((const char *)ctx->cfgFileName));
    }
}

void HandleEncGetOutputEvent(Component com, CNMComListenerEncDone* param, EncListenerContext* ctx)
{
    EncOutputInfo* output = param->output;
    int productID         = VPU_GetProductId(ctx->coreIdx);
#ifdef SUPPORT_READ_BITSTREAM_IN_ENCODER
    EncHandle      handle = param->handle;
#else
    if (output->reconFrameIndex == RECON_IDX_FLAG_ENC_END)
        return;
#endif


    if (ctx->pfCtx != NULL) {
        PFMonitorUpdate(ctx->coreIdx, ctx->pfCtx, output->frameCycle, output->encPrepareEndTick - output->encPrepareStartTick,
            output->encProcessingEndTick - output->encProcessingStartTick, output->encEncodeEndTick - output->encEncodeStartTick);
    }
    if (TRUE == PRODUCT_ID_W6_SERIES(productID)) {
        if (ctx->bwCtx != NULL) {
            BWMonitorUpdate(ctx->bwCtx, 1, output->reconFrameIndex);
        }
    }
    if (ctx->bwCtx != NULL) {
        BWMonitorUpdatePrint(ctx->bwCtx, output->picType, output->reconFrameIndex, output->encPicCnt);
    }
#ifdef SUPPORT_READ_BITSTREAM_IN_ENCODER
    if ( ctx->ringBufferEnable == TRUE) {
        if ( ctx->ringBufferWrapEnable == FALSE ) {
            //check remain data(not cpb full data)
            if (output->reconFrameIndex == RECON_IDX_FLAG_ENC_END) {
                int compSize = output->wrPtr - output->rdPtr;
                int i;
                Uint8*          buf      = NULL;
                buf = (Uint8*)osal_malloc(compSize);
                if ( !buf ) {
                    VLOG(ERR, "<%s:%d> memory malloc error\n", __FUNCTION__, __LINE__);
                    return ;
                }
                for ( i = 0 ; i < STREAM_READ_CNT ; i++) {
                    vdi_read_memory(ctx->coreIdx, output->rdPtr, buf, compSize, ctx->streamEndian);
                    if (ctx->es) {
                        if ((ctx->match=Comparator_Act(ctx->es, buf, compSize, param->handle->instIndex)) == TRUE) {
                            break;
                        }
                        VLOG(ERR, "<%s:%d> INSTANCE #%d Bitstream Mismatch\n", __FUNCTION__, __LINE__, handle->instIndex);
                        if (i < STREAM_READ_CNT-1) {
                            osal_msleep(100);
                            VLOG(TRACE, "BIN Mismatch RETRY\n");
                            ((AbstractComparator*)ctx->es)->impl->usePrevDataOneTime = 1;
                        }
                    }
                }
                if (param->fp) {
                    osal_fwrite(buf, compSize, 1, param->fp);
                    osal_fflush(param->fp);
                }
                osal_free(buf);
                return;
            }
            else {
                return;
            }
        }
    }

    if (output->bitstreamSize > 0) {
        if (ctx->es || param->fp) {
            int             compSize = output->bitstreamSize;
            if ( BitstreamReader_Act(param->bsReader, 0, 0, compSize, ctx->es, 0) == FALSE )
                ctx->match = FALSE;
        }

    }
#endif /* SUPPORT_READ_BITSTREAM_IN_ENCODER */

    if (ctx->headerEncDone[param->handle->instIndex] == FALSE) {
        ctx->headerEncDone[param->handle->instIndex] = TRUE;
    }
    if (ctx->match == FALSE) {
        CNMAppStop();
    }
}

void Coda9HandleEncGetOutputEvent(Component com, CNMComListenerEncDone* param, EncListenerContext* ctx)
{
    EncOutputInfo* output = param->output;
    EncHandle      handle = param->handle;

    if (output->reconFrameIndex == RECON_IDX_FLAG_ENC_END) return;

    if (ctx->pfCtx != NULL) {
        PFMonitorUpdate(ctx->coreIdx, ctx->pfCtx, output->frameCycle, output->encPrepareEndTick - output->encPrepareStartTick,
            output->encProcessingEndTick - output->encProcessingStartTick, output->encEncodeEndTick- output->encEncodeStartTick);
    }
    if (ctx->bwCtx != NULL) {
        BWMonitorUpdatePrint(ctx->bwCtx, output->picType, output->reconFrameIndex, output->encPicCnt);
    }

    if (output->bitstreamSize > 0) {
#ifdef SUPPORT_READ_BITSTREAM_IN_ENCODER
        if (ctx->es != NULL || param->fp) {
#else
        if (ctx->es != NULL) {
#endif
            Uint8*          buf      = NULL;
            int             compSize = output->bitstreamSize;
            PhysicalAddress addr;

            if (param->fullInterrupted == TRUE) {
                PhysicalAddress rd, wr;
                VPU_EncGetBitstreamBuffer(param->handle, &rd, &wr, &compSize);
                addr     = rd;
            }
            else {
                addr     = output->bitstreamBuffer;
                compSize = output->bitstreamSize;
            }

            buf = (Uint8*)osal_malloc(compSize);
            vdi_read_memory(ctx->coreIdx, addr, buf, compSize, ctx->streamEndian);

            if ( ctx->es != NULL ) {
                if ((ctx->match=Comparator_Act(ctx->es, buf, compSize, param->handle->instIndex)) == FALSE) {
                    VLOG(ERR, "<%s:%d> INSTANCE #%d Bitstream Mismatch\n", __FUNCTION__, __LINE__, handle->instIndex);
                }
            }
#ifdef SUPPORT_READ_BITSTREAM_IN_ENCODER
            if (param->fp) {
                osal_fwrite(buf, compSize, 1, param->fp);
                osal_fflush(param->fp);
            }
#endif
            osal_free(buf);
        }
    } else if (TRUE == param->encodedStreamInfo.ringBufferEnable && NULL != param->encodedStreamInfo.encodedStreamBuf) {

        if ( ctx->es != NULL ) {
            if ((ctx->match=Comparator_Act(ctx->es, param->encodedStreamInfo.encodedStreamBuf, param->encodedStreamInfo.encodedStreamBufLength, param->handle->instIndex)) == FALSE) {
                VLOG(ERR, "<%s:%d> INSTANCE #%d Bitstream Mismatch\n", __FUNCTION__, __LINE__, handle->instIndex);
            }
        }
#ifdef SUPPORT_READ_BITSTREAM_IN_ENCODER
        if (param->fp) {
            osal_fwrite(param->encodedStreamInfo.encodedStreamBuf, param->encodedStreamInfo.encodedStreamBufLength, 1, param->fp);
            osal_fflush(param->fp);
        }
#endif
    }

    if (ctx->headerEncDone[param->handle->instIndex] == FALSE) {
        ctx->headerEncDone[param->handle->instIndex] = TRUE;
    }

    if (ctx->match == FALSE) CNMAppStop();
}

void Coda9HandleEncMakeHeaderEvent(Component com, CNMComListenerEncMakeHeader* param, EncListenerContext* ctx)
{
    EncodedHeaderBufInfo encHeaderInfo = param->encHeaderInfo;
    if (0 < encHeaderInfo.encodedHeaderBufSize && NULL != encHeaderInfo.encodedHeaderBuf) {
        if (NULL != ctx->es) {
                if ((ctx->match=Comparator_Act(ctx->es, encHeaderInfo.encodedHeaderBuf, encHeaderInfo.encodedHeaderBufSize, param->handle->instIndex)) == FALSE) {
                    VLOG(ERR, "<%s:%d> Header Mismatch\n", __FUNCTION__, __LINE__, param->handle->instIndex);
                    VLOG(ERR, "<%s:%d> INSTANCE #%d Bitstream Mismatch\n", __FUNCTION__, __LINE__, param->handle->instIndex);
                }
        }
        if (encHeaderInfo.fp) {
            osal_fwrite(encHeaderInfo.encodedHeaderBuf, encHeaderInfo.encodedHeaderBufSize, 1, encHeaderInfo.fp);
            osal_fflush(encHeaderInfo.fp);
        }
    } else {
        VLOG(WARN, "<%s:%d> INSTANCE #%d Empty Header\n", __FUNCTION__, __LINE__, param->handle->instIndex);
    }

    if (ctx->match == FALSE) CNMAppStop();
}

void HandleEncCompHeaderEvent(Component com, BitstreamReader pBr, EncListenerContext* ctx)
{
    AbstractBitstreamReader *pABr = (AbstractBitstreamReader *)pBr;
    EncInfo     *pEinfo;

    if (pABr == NULL || ctx == NULL){
        VLOG(ERR, "[%s:%d]Error! Arguments is NULL!\n", __FUNCTION__, __LINE__);
        return;
    }
    ctx->match = TRUE;
    pEinfo = VPU_HANDLE_TO_ENCINFO(*pABr->handle);
    if(ctx->es || pABr->fp) {
        int compSize = pEinfo->streamWrPtr - pEinfo->streamRdPtr;
        if ( BitstreamReader_Act(pBr, 0, 0, compSize, ctx->es, 0) == FALSE ) {
            ctx->match = FALSE;
            VLOG(ERR, "Encoder Header MISMATCH !\n", __FUNCTION__, __LINE__);
        }
    } else {
        VLOG(WARN, "[%s:%d]Error! There is not comparator or file pointer\n", __FUNCTION__, __LINE__);
    }

    return;
}

int EncoderListener(Component com, Uint64 event, void* data, void* context)
{
    int         productId;
    int         ret = 0;
    EncHandle   handle;
#if defined(SUPPORT_MULTI_INSTANCE_TEST)
#else
    int         key=0;

    if (osal_kbhit()) {
        key = osal_getch();
        osal_flush_ch();
        if (key) {
            if (key == 'q' || key == 'Q') {
                CNMAppStop();
                return ret;
            }
        }
    }
#endif
    switch (event) {
    case COMPONENT_EVENT_ENC_OPEN:
        HandleEncOpenEvent(com, (CNMComListenerEncOpen*)data, (EncListenerContext*)context);
        break;
    case COMPONENT_EVENT_ENC_ISSUE_SEQ:
        break;
    case COMPONENT_EVENT_ENC_COMPLETE_SEQ:
        HandleEncCompleteSeqEvent(com, (CNMComListenerEncCompleteSeq*)data, (EncListenerContext*)context);
        break;
    case COMPONENT_EVENT_ENC_REGISTER_FB:
        break;
    case COMPONENT_EVENT_CODA9_ENC_MAKE_HEADER:
        Coda9HandleEncMakeHeaderEvent(com, (CNMComListenerEncMakeHeader*)data, (EncListenerContext*)context);
        break;
    case COMPONENT_EVENT_ENC_HEADER:
        HandleEncCompHeaderEvent(com, (BitstreamReader)data, (EncListenerContext*)context);
        break;
    case COMPONENT_EVENT_ENC_READY_ONE_FRAME:
        break;
    case COMPONENT_EVENT_ENC_START_ONE_FRAME:
        break;
    case COMPONENT_EVENT_ENC_HANDLING_INT:
        HandleEncHandlingIntEvent(com, (CNMComListenerHandlingInt*)data, (EncListenerContext*)context);
        break;
    case COMPONENT_EVENT_ENC_GET_OUTPUT_INFO:
        handle = ((CNMComListenerEncDone*)data)->handle;
        productId = VPU_GetProductId(VPU_HANDLE_CORE_INDEX(handle));
        if (TRUE == PRODUCT_ID_W_SERIES(productId)) {
            HandleEncGetOutputEvent(com, (CNMComListenerEncDone*)data, (EncListenerContext*)context);
        } else {
            Coda9HandleEncGetOutputEvent(com, (CNMComListenerEncDone*)data, (EncListenerContext*)context);
        }
        break;
    case COMPONENT_EVENT_ENC_ENCODED_ALL:
        break;
    case COMPONENT_EVENT_ENC_CLOSE:
        HandleEncGetEncCloseEvent(com, (CNMComListenerEncClose*)data, (EncListenerContext*)context);
        break;
    case COMPONENT_EVENT_ENC_FULL_INTERRUPT:
        HandleEncFullEvent(com, (CNMComListenerEncFull*)data, (EncListenerContext*)context);
        break;
    default:
        break;
    }
    return ret;
}

BOOL SetupEncListenerContext(EncListenerContext* ctx, CNMComponentConfig* config)
{
    TestEncConfig* encConfig = &config->testEncConfig;

    osal_memset((void*)ctx, 0x00, sizeof(EncListenerContext));

    if (encConfig->compareType & (1 << MODE_COMP_ENCODED)) {
        if ((ctx->es=Comparator_Create(STREAM_COMPARE, encConfig->ref_stream_path, encConfig->cfgFileName)) == NULL) {
            VLOG(ERR, "%s:%d Failed to Comparator_Create(%s)\n", __FUNCTION__, __LINE__, encConfig->ref_stream_path);
            return FALSE;
        }
    }
    ctx->coreIdx       = encConfig->coreIdx;
    ctx->streamEndian  = encConfig->stream_endian;
    ctx->match         = TRUE;
    ctx->matchOtherInfo= TRUE;
    ctx->performance   = encConfig->performance;
    ctx->bandwidth     = encConfig->bandwidth;
    ctx->pfClock       = encConfig->pfClock;
    osal_memcpy(ctx->cfgFileName, encConfig->cfgFileName, sizeof(ctx->cfgFileName));
    ctx->ringBufferEnable     = encConfig->ringBufferEnable;
    ctx->ringBufferWrapEnable = encConfig->ringBufferWrapEnable;

    return TRUE;
}

void ClearEncListenerContext(EncListenerContext* ctx)
{
    if (ctx->es)    Comparator_Destroy(ctx->es);
}

