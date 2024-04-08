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
#include <time.h>
#include <sys/time.h>
#include "cnm_app.h"
#include "component.h"
#include "misc/debug.h"
#include "misc/bw_monitor.h"

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#define DEFAULT_READ_SIZE 1460 // = MTU(ethernet) - (IP_HEADER+TCP_HEADER)
#define MAX_SIZE_ENC_HEADER 256
#define MAX_SIZE_ENCODED_STREAM_BUF 0x700000 // only for RingBuffer

typedef enum {
    ENC_INT_STATUS_NONE,        // Interrupt not asserted yet
    ENC_INT_STATUS_FULL,        // Need more buffer
    ENC_INT_STATUS_DONE,        // Interrupt asserted
    ENC_INT_STATUS_LOW_LATENCY,
    ENC_INT_STATUS_TIMEOUT,     // Interrupt not asserted during given time.
    ENC_INT_STATUS_BUF_FULL,
} ENC_INT_STATUS;

typedef enum {
    ENCODER_STATE_OPEN,
    ENCODER_STATE_INIT_SEQ,
    ENCODER_STATE_REGISTER_FB,
    ENCODER_STATE_ENCODE_HEADER,
    ENCODER_STATE_ENCODING,
} EncoderState;

typedef struct {
    EncSubFrameSyncState        subFrameSyncState;
    EncSubFrameSyncConfig       subFrameSyncCfg;
} SubFrameSyncInfo;

typedef struct {
    BOOL                        fieldDone;
    Int32                       iPicCnt;
    ENC_CFG                     encCfg;
    vpu_buffer_t                vbRoi;
} CODA980Ctx;

typedef struct {
    EncHandle                   handle;
    TestEncConfig               testEncConfig;
    EncOpenParam                encOpenParam;
    ParamEncNeedFrameBufferNum  fbCount;
    Uint32                      frameIdx;
    vpu_buffer_t                vbCustomLambda;
    vpu_buffer_t                vbScalingList;
    Uint32                      customLambda[NUM_CUSTOM_LAMBDA];
    UserScalingList             scalingList;
    vpu_buffer_t                vbCustomMap[MAX_FRAME_NUM];
    EncoderState                state;
    BOOL                        stateDoing;
    EncInitialInfo              initialInfo;
    Queue*                      encOutQ;
    EncParam                    encParam;
    Int32                       encodedReConFrmIdx;
    ParamEncBitstreamBuffer     bsBuf;
    BOOL                        fullInterrupt;
    Uint32                      changedCount;
    Uint64                      startTimeout;
    EncodedStreamBufInfo        encStreamBufInfo;
    Int32                       encHeaderSize;
    Uint8*                      encHeaderBuf; //EncodedHeaderBufInfo

    //CODA980 variable
    CODA980Ctx                  ctx980;
    Uint32                      cyclePerTick;
    Int32                       encodedSrcFrmIdxArr[ENC_SRC_BUF_NUM];
    SubFrameSyncInfo            subFrameSyncInfo;
    BOOL                        readBsWhenFullInterrupt;
} EncoderContext;

BOOL HandleEncFullInterrupt(ComponentImpl* com, EncoderContext* ctx, PortContainerES* out)
{
    Int32 streamSize =0;
    PhysicalAddress paRdPtr;
    PhysicalAddress paWrPtr;
    Uint32 fullStreamSize = 0;
    EncInfo* encInfo    = NULL;

    VPU_EncGetBitstreamBuffer(ctx->handle, &paRdPtr, &paWrPtr, &streamSize);
    VLOG(TRACE, "<%s:%d> INT_BSBUF_FULL %p, %p\n", __FUNCTION__, __LINE__, paRdPtr, paWrPtr);

    fullStreamSize = streamSize;

    encInfo = VPU_HANDLE_TO_ENCINFO(ctx->handle);

    if (NULL == encInfo) {
        VLOG(ERR, "<%s:%d> NULL point exception\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

#ifdef SUPPORT_READ_BITSTREAM_IN_ENCODER
    Uint8* encodedStreamBuf = ctx->encStreamBufInfo.encodedStreamBuf;
    Uint32 encodedLength = ctx->encStreamBufInfo.encodedStreamBufLength;

    GetBitstreamToBuffer(ctx->handle, encodedStreamBuf, paRdPtr, paWrPtr, encInfo->streamBufStartAddr,
        encInfo->streamBufEndAddr, streamSize, ctx->testEncConfig.stream_endian, ctx->testEncConfig.ringBufferEnable);

    CNMComListenerEncFull   lsnpFull;
    lsnpFull.handle         = ctx->handle;
    lsnpFull.fullStreamBuf  = encodedStreamBuf;
    if (TRUE == ctx->testEncConfig.ringBufferEnable) {
        lsnpFull.size       = streamSize + encodedLength;
    } else { // LineBuffer
        lsnpFull.size       = streamSize;
    }
    lsnpFull.fp             = ctx->fp;

    ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_FULL_INTERRUPT, (void*)&lsnpFull);

    RetCode result;
    if (TRUE == ctx->testEncConfig.ringBufferEnable) {
        if (RETCODE_SUCCESS != (result = VPU_EncUpdateBitstreamBuffer(ctx->handle, fullStreamSize))) {
            VLOG(ERR, "Failed to VPU_EncUpdateBitstreamBuffer(), ret code : %d\n", result);
            return FALSE;
        }
    }
    VPU_ClearInterrupt(ctx->testEncConfig.coreIdx);
#endif /* SUPPORT_READ_BITSTREAM_IN_ENCODER */

    out->paBsBufStart   = encInfo->streamBufStartAddr;
    out->paBsBufEnd     = encInfo->streamBufEndAddr;
    out->rdPtr          = paRdPtr;
    out->wrPtr          = paWrPtr;
    out->size           = fullStreamSize;

    return TRUE;
}


BOOL MakeEncHeader(EncoderContext* ctx, EncHeaderParam encHeaderParam) {

    if ((MAX_SIZE_ENC_HEADER-1) < ctx->encHeaderSize + encHeaderParam.size) {
        VLOG(WARN, "Enc Header Buffer overrun\n");
        return FALSE;
    }

    if (NULL != ctx->encHeaderBuf) {
        vdi_read_memory(ctx->testEncConfig.coreIdx, encHeaderParam.buf, ctx->encHeaderBuf+ctx->encHeaderSize, encHeaderParam.size, ctx->testEncConfig.stream_endian);
        ctx->encHeaderSize = ctx->encHeaderSize + encHeaderParam.size;
    } else {
        VLOG(ERR, "%s:%d NULL point Exception: %s\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    return TRUE;
}

static BOOL FindEsBuffer(EncoderContext* ctx, PhysicalAddress addr, vpu_buffer_t* bs)
{
    Uint32 i;

    for (i=0; i<ctx->bsBuf.num; i++) {
        if (addr == ctx->bsBuf.bs[i].phys_addr) {
            *bs = ctx->bsBuf.bs[i];
            return TRUE;
        }
    }

    return FALSE;
}

static void SetEncPicParam(ComponentImpl* com, PortContainerYuv* in, EncParam* encParam)
{
    EncoderContext* ctx      = (EncoderContext*)com->context;
    vpu_buffer_t*   buf      = (vpu_buffer_t*)Queue_Peek(ctx->encOutQ);
    BOOL   isIdr      = FALSE;
    BOOL   isIPic     = FALSE;
    Uint32 minSkipNum = 0;
    Uint32 idx        = 0;

    encParam->picStreamBufferAddr = buf->phys_addr;
    encParam->picStreamBufferSize = buf->size;

    encParam->srcIdx                      = in->srcFbIndex;
    encParam->srcEndFlag                  = in->last;
    encParam->sourceFrame                 = &in->fb;
    encParam->sourceFrame->sourceLBurstEn = FALSE;

    encParam->sourceFrame->endian         = ctx->encOpenParam.frameEndian;
    encParam->sourceFrame->cbcrInterleave = ctx->encOpenParam.cbcrInterleave;
    encParam->skipPicture                 = 0;

    if (ctx->encOpenParam.EncStdParam.avcParam.fieldFlag)
        encParam->fieldRun = TRUE;

    if (0 == ctx->frameIdx) {
        isIdr  = TRUE;
        isIPic = TRUE;
    }
    else {
        Int32 gopNum = ctx->encOpenParam.EncStdParam.avcParam.fieldFlag ? (2 * ctx->encOpenParam.gopSize) : ctx->encOpenParam.gopSize;
        Int32 picIdx = ctx->encOpenParam.EncStdParam.avcParam.fieldFlag ? (2 * ctx->frameIdx + ctx->ctx980.fieldDone) : ctx->frameIdx;
        if ((ctx->encOpenParam.idrInterval > 0) && (gopNum > 0)) {
            if ((picIdx >= gopNum)) {
                picIdx = 0;
            }
            if (picIdx == 0) {
                isIPic = TRUE;
                ctx->ctx980.iPicCnt++;
            }
        }
    }

    if (ctx->encOpenParam.bitstreamFormat == STD_MPEG4) {
        if (isIPic == TRUE && ctx->encOpenParam.idrInterval > 0) {
            if ((ctx->ctx980.iPicCnt% ctx->encOpenParam.idrInterval) == 0) {
                isIdr = TRUE;
            }
        }
    }

    encParam->forceIPicture = ctx->ctx980.fieldDone ? FALSE : isIdr;
    encParam->skipPicture   = 0;

    minSkipNum = ctx->encOpenParam.EncStdParam.avcParam.mvcExtension ? 1 : 0;

    for (idx = 0; idx<MAX_PIC_SKIP_NUM; idx++) {
        Uint32 numPicSkip = ctx->ctx980.encCfg.field_flag ? ctx->testEncConfig.skipPicNums[idx] / 2 : ctx->testEncConfig.skipPicNums[idx];
        if (numPicSkip > minSkipNum && numPicSkip == (Uint32)ctx->frameIdx) {
            if (ctx->ctx980.encCfg.field_flag == FALSE) {
                encParam->skipPicture = TRUE;
            }
            else {
                if (ctx->testEncConfig.skipPicNums[idx] % 2)
                    encParam->skipPicture = ctx->ctx980.fieldDone;
                else
                    encParam->skipPicture = !(ctx->ctx980.fieldDone);
                /* check next skip field */
                if ((idx + 1) < MAX_PIC_SKIP_NUM) {
                    numPicSkip = ctx->testEncConfig.skipPicNums[idx + 1] / 2;
                    if (numPicSkip == (Uint32)ctx->frameIdx)
                        encParam->skipPicture = TRUE;
                }
            }
            break;
        }
    }

    encParam->coda9RoiEnable = ctx->encOpenParam.coda9RoiEnable;

    if (ctx->encOpenParam.coda9RoiEnable) {
        encParam->roiQpMapAddr     = ctx->ctx980.vbRoi.phys_addr;
        encParam->coda9RoiPicAvgQp = ctx->encOpenParam.RoiPicAvgQp;
        if (ctx->testEncConfig.roi_file_name) {
            Uint8 *pRoiBuf;
            int    pic_height = ctx->encOpenParam.EncStdParam.avcParam.fieldFlag ? (ctx->encOpenParam.picHeight/2) : (ctx->encOpenParam.picHeight);
            Int32  roiMapSize = VPU_ALIGN16(ctx->encOpenParam.picWidth) * VPU_ALIGN16(pic_height) / 256;  // 1 Byte per 1 MB.

            pRoiBuf = (Uint8*)osal_malloc(roiMapSize);

            if (osal_fread(pRoiBuf, 1, roiMapSize, ctx->testEncConfig.roi_file) != roiMapSize) {
                osal_fseek(ctx->testEncConfig.roi_file, 0, SEEK_SET);
                osal_fread(pRoiBuf, 1, roiMapSize, ctx->testEncConfig.roi_file);
            }

            vdi_write_memory(ctx->testEncConfig.coreIdx, encParam->roiQpMapAddr, pRoiBuf, roiMapSize, VDI_BIG_ENDIAN);
            osal_free(pRoiBuf);
        }
    }
}

static BOOL RegisterFrameBuffers(ComponentImpl* com)
{
    EncoderContext*         ctx= (EncoderContext*)com->context;
    FrameBuffer*            pReconFb      = NULL;
    FrameBuffer*            pSrcFb        = NULL;
    FrameBufferAllocInfo    srcFbAllocInfo;
    FrameBufferAllocInfo    reconFbAllocInfo;
    Uint32                  reconFbStride = 0;
    Uint32                  reconFbHeight = 0;
    ParamEncFrameBuffer     paramFb;
    RetCode                 result;
    CNMComponentParamRet    ret;
    BOOL                    success;

    ctx->stateDoing = TRUE;
    ret = ComponentGetParameter(com, com->srcPort.connectedComponent, GET_PARAM_YUVFEEDER_FRAME_BUF, (void*)&paramFb);
    if (ComponentParamReturnTest(ret, &success) == FALSE) return success;

    //Reconstructed frame buffers
    pReconFb      = paramFb.reconFb;
    reconFbStride = paramFb.reconFbAllocInfo.stride;
    reconFbHeight = paramFb.reconFbAllocInfo.height;
    reconFbAllocInfo = paramFb.reconFbAllocInfo;

    if(0 != reconFbStride && 0 !=reconFbHeight) {
        if (VPU_EncAllocateFrameBuffer(ctx->handle, reconFbAllocInfo, pReconFb) != RETCODE_SUCCESS) {
            VLOG(ERR, "VPU_EncAllocateFrameBuffer fail to allocate reconstructed frame buffers\n");
            return FALSE;
        }
    }

    result = VPU_EncRegisterFrameBuffer(ctx->handle, pReconFb, ctx->fbCount.reconFbNum, reconFbStride, reconFbHeight, (TiledMapType)reconFbAllocInfo.mapType);

    if (result != RETCODE_SUCCESS) {
        VLOG(ERR, "%s:%d Failed to VPU_EncRegisterFrameBuffer(%d)\n", __FUNCTION__, __LINE__, result);
        return FALSE;
    }
    ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_REGISTER_FB, NULL);

    //Reference frame buffers
    pSrcFb         = paramFb.srcFb;
    srcFbAllocInfo = paramFb.srcFbAllocInfo;

    if(0 != srcFbAllocInfo.stride && 0 !=srcFbAllocInfo.height ) {
        if (VPU_EncAllocateFrameBuffer(ctx->handle, srcFbAllocInfo, pSrcFb) != RETCODE_SUCCESS) {
            VLOG(ERR, "VPU_EncAllocateFrameBuffer fail to allocate source frame buffers\n");
            return FALSE;
        }
    }

    if (NULL == ctx->encStreamBufInfo.encodedStreamBuf) {
        ctx->encStreamBufInfo.encodedStreamBufSize = MAX_SIZE_ENCODED_STREAM_BUF;
        //ctx->encStreamBufInfo.encodedStreamBufSize = ctx->encOpenParam.bitstreamBufferSize;
        ctx->encStreamBufInfo.encodedStreamBuf = (Uint8 *)osal_malloc(ctx->encStreamBufInfo.encodedStreamBufSize);
        if (NULL != ctx->encStreamBufInfo.encodedStreamBuf) {
            osal_memset(ctx->encStreamBufInfo.encodedStreamBuf, 0x00, ctx->encStreamBufInfo.encodedStreamBufSize);
        }
        else {
            VLOG(WARN, "%s:%d NULL point exception :: encodedStreamBuf\n",__FUNCTION__, __LINE__);
        }
    }

    ctx->stateDoing = FALSE;

    return TRUE;
}

static CNMComponentParamRet GetParameterEncoder(ComponentImpl* from, ComponentImpl* com, GetParameterCMD commandType, void* data)
{
    EncoderContext*             ctx = (EncoderContext*)com->context;
    BOOL                        result  = TRUE;
    ParamEncNeedFrameBufferNum* fbCount;
    PortContainerYuv*           container;

    switch(commandType) {
    case GET_PARAM_COM_IS_CONTAINER_CONUSUMED:
        container = (PortContainerYuv*)data;
        if (ctx->subFrameSyncInfo.subFrameSyncCfg.subFrameSyncOn == TRUE) {
            container->consumed = TRUE;
        }
        else {
            if (0 <= ctx->encodedReConFrmIdx) {
                container->consumed = TRUE;
            }
        }
        break;
    case GET_PARAM_ENC_HANDLE:
        if (ctx->handle == NULL) return CNM_COMPONENT_PARAM_NOT_READY;
        *(EncHandle*)data = ctx->handle;
        break;
    case GET_PARAM_SRC_FRAME_INFO:
        container = (PortContainerYuv*)data;
        if (container->type == SUB_FRAME_SYNC_SRC_ADDR_SEND) {
            container->srcCanBeWritten = ctx->encodedSrcFrmIdxArr[container->srcFbIndex];
            break;
        }
        //if ( container->type == SUB_FRAME_SYNC_DATA_FEED) {
        container->srcCanBeWritten = ctx->encodedSrcFrmIdxArr[container->srcFbIndex];
        ctx->encodedSrcFrmIdxArr[container->srcFbIndex] = 0;
        //}
        break;
    case GET_PARAM_ENC_FRAME_BUF_NUM:
        if (ctx->fbCount.srcFbNum == 0) return CNM_COMPONENT_PARAM_NOT_READY;
        fbCount = (ParamEncNeedFrameBufferNum*)data;
        fbCount->reconFbNum = ctx->fbCount.reconFbNum;
        fbCount->srcFbNum   = ctx->fbCount.srcFbNum;
        break;
    case GET_PARAM_ENC_FRAME_BUF_REGISTERED:
        if (ctx->state <= ENCODER_STATE_REGISTER_FB) return CNM_COMPONENT_PARAM_NOT_READY;
        *(BOOL*)data = TRUE;
        break;
    default:
        result = FALSE;
        break;
    }

    return (result == TRUE) ? CNM_COMPONENT_PARAM_SUCCESS : CNM_COMPONENT_PARAM_FAILURE;
}

static CNMComponentParamRet SetParameterEncoder(ComponentImpl* from, ComponentImpl* com, SetParameterCMD commandType, void* data)
{
    BOOL result = TRUE;
    EncoderContext* ctx = (EncoderContext*)com->context;
    switch(commandType) {
    case SET_PARAM_COM_PAUSE:
        com->pause   = *(BOOL*)data;
        break;
    case SET_PARAM_ENC_SUBFRAMESYNC:
        if (NULL != data) {
            SubFrameSyncInfo* pSubFrameInfo = data;
            ctx->subFrameSyncInfo.subFrameSyncCfg.sourceBufIndexBase = pSubFrameInfo->subFrameSyncCfg.sourceBufIndexBase;
            ctx->subFrameSyncInfo.subFrameSyncState.ipuEndOfRow = pSubFrameInfo->subFrameSyncState.ipuEndOfRow;
            ctx->subFrameSyncInfo.subFrameSyncState.ipuNewFrame = pSubFrameInfo->subFrameSyncState.ipuNewFrame;
            ctx->subFrameSyncInfo.subFrameSyncState.ipuCurFrameIndex = pSubFrameInfo->subFrameSyncState.ipuCurFrameIndex;
        }
        break;
    case SET_PARAM_ENC_READ_BS_WHEN_FULL_INTERRUPT:
        ctx->readBsWhenFullInterrupt = *(BOOL*)data;
        break;
    default:
        VLOG(ERR, "Unknown SetParameterCMD Type : %d\n", commandType);
        result = FALSE;
        break;
    }

    return (result == TRUE) ? CNM_COMPONENT_PARAM_SUCCESS : CNM_COMPONENT_PARAM_FAILURE;
}

static ENC_INT_STATUS HandlingInterruptFlagNoCQ(ComponentImpl* com, InterruptBit waitInterrptFlag)
{
    EncoderContext*      ctx               = (EncoderContext*)com->context;
    EncHandle            handle            = ctx->handle;
    Int32                interruptFlag     = 0;
    Uint32               interruptWaitTime = VPU_ENC_TIMEOUT;
    Uint32               interruptTimeout  = VPU_ENC_TIMEOUT;
    ENC_INT_STATUS       status            = ENC_INT_STATUS_NONE;
    BOOL                 repeat            = TRUE;
    Uint32               toggleCount = 0;

    ctx->startTimeout = osal_gettime();

    if (ctx->testEncConfig.subFrameSyncEn) {
        interruptWaitTime = 100;
    }

    do {
        do {
            interruptFlag = VPU_WaitInterruptEx(handle, interruptWaitTime);
            if (INTERRUPT_TIMEOUT_VALUE == interruptFlag) {
                Uint64   currentTimeout = osal_gettime();
                if ((currentTimeout - ctx->startTimeout) > interruptTimeout) {
                    CNMErrorSet(CNM_ERROR_HANGUP);
                    VLOG(ERR, "INSTANCE #%d INTERRUPT TIMEOUT.\n", handle->instIndex);
                    status = ENC_INT_STATUS_TIMEOUT;
                    break;
                }
                interruptFlag = 0;
            }
            if (ctx->testEncConfig.subFrameSyncEn) {
                //this is a sample to control VPU sub frame sync in view of Camera Interface.
                if ((VPU_ALIGN16(ctx->encOpenParam.picHeight) / 16) > toggleCount) {
                    ctx->subFrameSyncInfo.subFrameSyncState.ipuEndOfRow = !ctx->subFrameSyncInfo.subFrameSyncState.ipuEndOfRow; // Camera interface module should toggle ipuEndOfRow whenever 64line image is saved in frame buffer
                    VPU_EncGiveCommand(ctx->handle, ENC_SET_SUB_FRAME_SYNC, &(ctx->subFrameSyncInfo.subFrameSyncState));
                    toggleCount++;
                }
                if (0 < interruptFlag) {
                    break;
                }
            }
        } while (ctx->testEncConfig.subFrameSyncEn);
        if (INTERRUPT_TIMEOUT_VALUE == interruptFlag) {
            break;
        }

        if (0 > interruptFlag) {
            VLOG(ERR, "<%s:%d> interruptFlag is negative value! %08x\n", __FUNCTION__, __LINE__, interruptFlag);
            status = ENC_INT_STATUS_NONE;
        }

        if (interruptFlag & (1<<INT_BIT_BIT_BUF_FULL)) {
            status = ENC_INT_STATUS_BUF_FULL;
            break;
        }

        if (0 < interruptFlag) {
            VPU_ClearInterruptEx(handle, interruptFlag);
        }

        if (interruptFlag & (1 << waitInterrptFlag)) {
            status = ENC_INT_STATUS_DONE;
            repeat = FALSE;
        }

    } while (repeat);

    return status;
}

static BOOL SetSequenceInfo(ComponentImpl* com)
{
    EncoderContext* ctx = (EncoderContext*)com->context;
    EncHandle       handle  = ctx->handle;
    RetCode         ret     = RETCODE_SUCCESS;
    EncInitialInfo* initialInfo = &ctx->initialInfo;
    CNMComListenerEncCompleteSeq lsnpCompleteSeq   = {0};
    MaverickCacheConfig     encCacheConfig;

    ret = VPU_EncGetInitialInfo(handle, initialInfo);
    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "%s:%d FAILED TO ENC_PIC_HDR: ret(%d) \n", __FUNCTION__, __LINE__, ret);
        return FALSE;
    }

    osal_memset(&encCacheConfig, 0x00, sizeof(MaverickCacheConfig));


    MaverickCache2Config(
        &encCacheConfig,
        0,                    //encoder
        ctx->encOpenParam.cbcrInterleave, // cb cr interleave
        0,                    /* bypass */
        0,                    /* burst  */
        3,                    /* merge mode */
        ctx->testEncConfig.mapType,
        15 /* shape */);

    VPU_EncGiveCommand(handle, SET_CACHE_CONFIG, &encCacheConfig);

    lsnpCompleteSeq.handle = handle;
    ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_COMPLETE_SEQ, (void*)&lsnpCompleteSeq);

    ctx->fbCount.reconFbNum = initialInfo->minFrameBufferCount;
    ctx->fbCount.srcFbNum = ENC_SRC_BUF_NUM; //2

    if ( ctx->encOpenParam.sourceBufCount > ctx->fbCount.srcFbNum) {
        //if you want to change the number of source buffers,
        //use a variable sourceBufCount of encOpenParam
        //ctx->fbCount.srcFbNum = ctx->encOpenParam.sourceBufCount;
    }

    VLOG(INFO, "[ENCODER] Required  reconFbCount=%d, srcFbCount=%d, outNum=%d, size=%dx%d\n",
        ctx->fbCount.reconFbNum, ctx->fbCount.srcFbNum, ctx->testEncConfig.outNum, ctx->encOpenParam.picWidth, ctx->encOpenParam.picHeight);
    ctx->stateDoing = FALSE;

    return TRUE;
}

static BOOL EncodeHeader(ComponentImpl* com)
{
    EncoderContext*         ctx = (EncoderContext*)com->context;
    EncHandle               handle  = ctx->handle;
    RetCode                 ret     = RETCODE_SUCCESS;
    EncHeaderParam          encHeaderParam;
    vpu_buffer_t*           buf;
    Int32                   ringBufEnable =ctx->encOpenParam.ringBufferEnable;

    ctx->stateDoing = TRUE;

    ctx->encParam.forceIPicture = 0;
    ctx->encParam.skipPicture = 0;
    ctx->encParam.quantParam = ctx->testEncConfig.picQpY;

    if (FALSE == ringBufEnable && NULL == ctx->encHeaderBuf) {
        ctx->encHeaderSize =0;
        ctx->encHeaderBuf = (Uint8 *)osal_malloc(MAX_SIZE_ENC_HEADER);
        if (NULL != ctx->encHeaderBuf) {
            osal_memset(ctx->encHeaderBuf, 0x00, MAX_SIZE_ENC_HEADER);
        }
        else {
            VLOG(WARN, "%s:%d NULL point exception :: encHeaderBuf\n",__FUNCTION__, __LINE__);
        }
    }

    buf = Queue_Peek(ctx->encOutQ);
    osal_memset(&encHeaderParam, 0x00, sizeof(EncHeaderParam));
    encHeaderParam.zeroPaddingEnable = 0;

    encHeaderParam.buf  = buf->phys_addr;
    encHeaderParam.size = buf->size;

    if (ctx->encOpenParam.bitstreamFormat == STD_MPEG4) {
        encHeaderParam.headerType = 0;
        if((ret =VPU_EncGiveCommand(handle, ENC_PUT_VIDEO_HEADER, &encHeaderParam)) != RETCODE_SUCCESS) {
            VLOG(ERR, "VPU_EncGiveCommand ( ENC_PUT_VIDEO_HEADER ) for VOL_HEADER failed Error code is 0x%x \n", ret);
            return FALSE;
        }
        if (FALSE == ringBufEnable) {
            MakeEncHeader(ctx, encHeaderParam);
        }
#ifdef MP4_ENC_VOL_HEADER
        encHeaderParam.headerType = VOL_HEADER;
        if ((ret=VPU_EncGiveCommand(handle, ENC_PUT_VIDEO_HEADER, &encHeaderParam)) != RETCODE_SUCCESS) {
            VLOG(ERR, "VPU_EncGiveCommand ( ENC_PUT_VIDEO_HEADER ) for VOL_HEADER failed Error code is 0x%x \n", ret);
            return FALSE;
        }
        if (FALSE == ringBufEnable) {
            MakeEncHeader(ctx, encHeaderParam);
        }
#endif
    }
    else if(ctx->encOpenParam.bitstreamFormat == STD_AVC) {
        if (ctx->encOpenParam.EncStdParam.avcParam.mvcExtension && ctx->encOpenParam.EncStdParam.avcParam.parasetRefreshEn) {
            if ((ret=VPU_EncGiveCommand(handle, ENC_SET_PARAM, &(ctx->encOpenParam))) != RETCODE_SUCCESS) {
                VLOG(ERR, "VPU_EncGiveCommand ( ENC_SET_PARAM ) for VOL_HEADER failed Error code is 0x%x \n", ret);
                return FALSE;
            }
            //SKIP_PUT_HEADER
            ctx->stateDoing = FALSE;
            return TRUE;
        }
        encHeaderParam.headerType = SPS_RBSP;
        encHeaderParam.size = buf->size;
        if ((ret=VPU_EncGiveCommand(handle, ENC_PUT_VIDEO_HEADER, &encHeaderParam)) != RETCODE_SUCCESS) {
            VLOG(ERR, "VPU_EncGiveCommand ( ENC_PUT_VIDEO_HEADER ) for SPS_RBSP failed Error code is 0x%x \n", ret);
            return FALSE;
        }
        if (FALSE == ringBufEnable) {
            MakeEncHeader(ctx, encHeaderParam);
        }
        if (ctx->encOpenParam.EncStdParam.avcParam.mvcExtension == TRUE) {
            encHeaderParam.headerType = SPS_RBSP_MVC;
            encHeaderParam.size = buf->size;
            if ((ret =VPU_EncGiveCommand(handle, ENC_PUT_VIDEO_HEADER, &encHeaderParam)) != RETCODE_SUCCESS) {
                VLOG(ERR, "VPU_EncGiveCommand ( ENC_PUT_VIDEO_HEADER ) for SPS_RBSP_MVC failed Error code is 0x%x \n", ret);
                return FALSE;
            }
            if (FALSE == ringBufEnable) {
                MakeEncHeader(ctx, encHeaderParam);
            }
        }
        encHeaderParam.headerType = PPS_RBSP;
        encHeaderParam.size = buf->size;
        if ((ret =VPU_EncGiveCommand(handle, ENC_PUT_VIDEO_HEADER, &encHeaderParam))!= RETCODE_SUCCESS) {
            VLOG(ERR, "VPU_EncGiveCommand ( ENC_PUT_VIDEO_HEADER ) for PPS_RBSP failed Error code is 0x%x \n", ret);
            return FALSE;
        }
        if (FALSE == ringBufEnable) {
            MakeEncHeader(ctx, encHeaderParam);
        }

        if (ctx->encOpenParam.EncStdParam.avcParam.mvcExtension) {
            encHeaderParam.headerType = PPS_RBSP_MVC;
            encHeaderParam.size = buf->size;
            if ((ret=VPU_EncGiveCommand(handle, ENC_PUT_VIDEO_HEADER, &encHeaderParam)) != RETCODE_SUCCESS) {
                VLOG(ERR, "VPU_EncGiveCommand ( ENC_PUT_VIDEO_HEADER ) for PPS_RBSP_MVC failed Error code is 0x%x \n", ret);
                return FALSE;
            }
            if (FALSE == ringBufEnable) {
                MakeEncHeader(ctx, encHeaderParam);
            }
        }
    }

    DisplayEncodedInformation(ctx->handle, ctx->encOpenParam.bitstreamFormat, 0, NULL, 0, 0, ctx->testEncConfig.performance, 0);


    ctx->stateDoing = FALSE;

    return TRUE;
}

static BOOL Encode(ComponentImpl* com, PortContainerYuv* in, PortContainerES* out)
{
    EncoderContext*                 ctx = (EncoderContext*)com->context;
    BOOL                            doEncode = FALSE;
    EncParam*                       encParam = &ctx->encParam;
    EncOutputInfo                   encOutputInfo;
    ENC_INT_STATUS                  intStatus = ENC_INT_STATUS_NONE;
    RetCode                         result;
    CNMComListenerHandlingInt       lsnpHandlingInt = {0};
    CNMComListenerEncDone           lsnpPicDone = {0};
    CNMComListenerEncStartOneFrame  lsn;
    PhysicalAddress                 rdPrt;
    PhysicalAddress                 wrPtr;
    Int32                           lengthOfStream;
    EncInfo* encInfo                = NULL;

    if (NULL == in) {
        //Just retry
        return TRUE;
    }
    if (in && in->type == SUB_FRAME_SYNC_DATA_FEED) {
        in->reuse = FALSE;
        in->consumed = TRUE;
        return TRUE;
    }
    lsnpPicDone.handle = ctx->handle;
    ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_READY_ONE_FRAME, &lsnpPicDone);

    ctx->stateDoing = TRUE;

    if (out->buf.phys_addr != 0) {
        Queue_Enqueue(ctx->encOutQ, (void*)&out->buf);
        out->buf.phys_addr = 0;
        out->buf.size      = 0;
    }

    if (FALSE == ctx->fullInterrupt) {
        if (encParam->srcEndFlag == TRUE) {
            doEncode = (BOOL)(Queue_Get_Cnt(ctx->encOutQ) > 0);
        }

        if (Queue_Get_Cnt(ctx->encOutQ) > 0) {
            SetEncPicParam(com, in, encParam);
            doEncode = TRUE;
            in->prevDataReuse = TRUE;
        }
        if(TRUE == in->last) {
            if (out) out->last  = TRUE;
            if (out) out->reuse = FALSE;
            if (out) out->size = 0;
            ctx->stateDoing    = FALSE;
            com->terminate     = TRUE;
            VLOG(INFO, "End of YUV stream.\n");
            return TRUE;
        }
    } else {
        doEncode = FALSE;
    }

    if (TRUE == doEncode) {
        if ((ctx->encOpenParam.bitstreamFormat == STD_AVC) && (ctx->frameIdx > 0) &&
            (ctx->frameIdx == ctx->testEncConfig.coda9ParamChange.ChangeFrameNum) &&
            (strcmp(ctx->testEncConfig.coda9ParamChange.pchChangeCfgFileName, "") != 0))
        {
            SetCoda9ChangeParam(ctx->handle, &ctx->testEncConfig, ctx->ctx980.fieldDone);
        }


        if (ctx->testEncConfig.subFrameSyncEn) {
            ParamEncFrameBuffer     paramFb;
            CNMComponentParamRet    ret;
            BOOL                    success;
            ret = ComponentGetParameter(com, com->srcPort.connectedComponent, GET_PARAM_YUVFEEDER_FRAME_BUF, (void*)&paramFb);
            if (ComponentParamReturnTest(ret, &success) == FALSE) return success;

            //ctx->subFrameSyncInfo.subFrameSyncCfg.sourceBufIndexBase = pSubFrameInfo->subFrameSyncCfg.sourceBufIndexBase;

            ctx->subFrameSyncInfo.subFrameSyncCfg.sourceBufIndexBase  = paramFb.srcFb[in->srcFbIndex].myIndex;
            VPU_EncGiveCommand(ctx->handle, ENC_CONFIG_SUB_FRAME_SYNC, &ctx->subFrameSyncInfo.subFrameSyncCfg);

            ctx->subFrameSyncInfo.subFrameSyncState.ipuNewFrame = !ctx->subFrameSyncInfo.subFrameSyncState.ipuNewFrame; // indicate the current is not start of the frame;
            VPU_EncGiveCommand(ctx->handle, ENC_SET_SUB_FRAME_SYNC, &(ctx->subFrameSyncInfo.subFrameSyncState));
        }

        result = VPU_EncStartOneFrame(ctx->handle, encParam);
        if (RETCODE_SUCCESS == result) {
            if (in)  in->prevDataReuse = FALSE;
            Queue_Dequeue(ctx->encOutQ);
            //ctx->frameIdx++;
            in->reuse = FALSE;
            out->reuse = FALSE;
            if(TRUE == ctx->testEncConfig.ringBufferEnable && NULL != ctx->encStreamBufInfo.encodedStreamBuf) {
                osal_memset(ctx->encStreamBufInfo.encodedStreamBuf, 0x00, ctx->encStreamBufInfo.encodedStreamBufSize);
                ctx->encStreamBufInfo.encodedStreamBufLength = 0;
            }
        }
        else { // Error
            VLOG(ERR, "VPU_EncStartOneFrame failed Error code is 0x%x \n", result);
            HandleEncoderError(ctx->handle, ctx->frameIdx, NULL);
            return FALSE;
        }
        lsn.handle = ctx->handle;
        lsn.result = result;
        ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_START_ONE_FRAME, (void*)&lsn);
    } else {
        if (FALSE == ctx->fullInterrupt) {
            //VLOG(TRACE, "waiting for reader!!! \n");
            out->reuse = FALSE;
            return TRUE;
        }
    }

    ctx->fullInterrupt = FALSE;
    if ((intStatus = HandlingInterruptFlagNoCQ(com, INT_BIT_PIC_RUN)) == ENC_INT_STATUS_TIMEOUT) {
        vdi_log(ctx->testEncConfig.coreIdx, 0, PIC_RUN, 0);
        VLOG(ERR, "failed to HandlingInterruptFlagNoCQ() \n");
        HandleEncoderError(ctx->handle, ctx->frameIdx, NULL);
        VPU_SWReset(ctx->testEncConfig.coreIdx, SW_RESET_FORCE, ctx->handle);
        return FALSE;
    }
    else if (ENC_INT_STATUS_BUF_FULL == intStatus) {
        if (FALSE == HandleEncFullInterrupt(com, ctx, out) )
        {
            VLOG(ERR, "failed to HandleEncFullInterrupt() \n");
            return FALSE;
        }

        out->reuse          = FALSE;
        out->streamBufFull  = TRUE;
        in->reuse           = TRUE;
        ctx->fullInterrupt  = TRUE;
        return TRUE; //Go back to execution
    }
    else if (ENC_INT_STATUS_NONE == intStatus) {
        out->size  = 0;
        out->reuse = FALSE;
        return TRUE; /* Try again */
    }
    ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_HANDLING_INT, (void*)&lsnpHandlingInt);


    osal_memset(&encOutputInfo, 0x00, sizeof(EncOutputInfo));
    result = VPU_EncGetOutputInfo(ctx->handle, &encOutputInfo);
    if (result == RETCODE_REPORT_NOT_READY) {
        return TRUE; /* Not encoded yet */
    }
    else if (result == RETCODE_VLC_BUF_FULL) {
        VLOG(ERR, "VLC BUFFER FULL!!! ALLOCATE MORE TASK BUFFER(%d)!!!\n", ONE_TASKBUF_SIZE_FOR_CQ);
    }
    else if (result != RETCODE_SUCCESS) {
        /* ERROR */
        HandleEncoderError(ctx->handle, encOutputInfo.encPicCnt, &encOutputInfo);
        VPU_SWReset(ctx->testEncConfig.coreIdx, SW_RESET_FORCE, ctx->handle);
        VLOG(ERR, "Failed to encode error = %d\n", result);
        return FALSE;
    }
    else {;
    /* SUCCESS */
    }

    DisplayEncodedInformation(ctx->handle, ctx->encOpenParam.bitstreamFormat, ctx->frameIdx, &encOutputInfo, encParam->srcEndFlag, encParam->srcIdx, ctx->testEncConfig.performance, ctx->cyclePerTick);

    if (FALSE  == ctx->testEncConfig.ringBufferEnable && encOutputInfo.bitstreamWrapAround) {
        // If LineBuffer interrupt is set to 1, it is ok to work.
        VLOG(WARN, "Warning!! BitStream buffer wrap-arounded. prepare more large buffer. Consumed all remained stream\n");
    }


    if (encOutputInfo.encSrcIdx >= 0)
        ctx->encodedSrcFrmIdxArr[encOutputInfo.encSrcIdx] = 1;

    lsnpPicDone.handle = ctx->handle;
    lsnpPicDone.ret = result;
    lsnpPicDone.output = &encOutputInfo;
    lsnpPicDone.fullInterrupted = ctx->fullInterrupt;

    if (TRUE == ctx->testEncConfig.ringBufferEnable) {
        ctx->encStreamBufInfo.ringBufferEnable = ctx->testEncConfig.ringBufferEnable;
        osal_memcpy(&(lsnpPicDone.encodedStreamInfo), &(ctx->encStreamBufInfo), sizeof(EncodedStreamBufInfo));
    }

    ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_GET_OUTPUT_INFO, (void*)&lsnpPicDone);
    if (result != RETCODE_SUCCESS )
        return FALSE;

    ctx->encodedReConFrmIdx = encOutputInfo.encSrcIdx;
    if (0 > ctx->encodedReConFrmIdx) {
        VLOG(WARN, "negative value of encodedReConFrmIdx : %d\n", ctx->encodedReConFrmIdx);
    }

    ctx->fullInterrupt      = FALSE;

    if (out) {
        encInfo = VPU_HANDLE_TO_ENCINFO(ctx->handle);
        VPU_EncGetBitstreamBuffer(ctx->handle, &rdPrt, &wrPtr, &lengthOfStream);
        if (in) in->reuse = FALSE;
        out->rdPtr = rdPrt;
        out->wrPtr = wrPtr;
        out->paBsBufStart = encInfo->streamBufStartAddr;
        out->paBsBufEnd = encInfo->streamBufEndAddr;
        if (TRUE == ctx->testEncConfig.ringBufferEnable) {
            out->buf.phys_addr = ctx->encOpenParam.bitstreamBuffer;
            out->size = lengthOfStream;
#ifdef SUPPORT_READ_BITSTREAM_IN_ENCODER
        if (RETCODE_SUCCESS != (result = VPU_EncUpdateBitstreamBuffer(ctx->handle, lengthOfStream))) {
            VLOG(ERR, "Failed to VPU_EncUpdateBitstreamBuffer(), ret code : %d\n", result);
            return FALSE;
        }
#endif // CNM_FPGA_PLATFORM
            //VLOG(TRACE, "addr : %x\n", out->rdPtr);
        } else {
            out->size = lengthOfStream;
            if (FALSE == FindEsBuffer(ctx, encOutputInfo.bitstreamBuffer, &out->buf)) {
                VLOG(ERR, "%s:%d Failed to find buffer(%p)\n", __FUNCTION__, __LINE__, encOutputInfo.bitstreamBuffer);
                return FALSE;
            }
        }
        out->reuse = (BOOL)(out->size == 0);
    }

    if (ctx->encParam.fieldRun && FALSE == ctx->ctx980.fieldDone) {
        ctx->ctx980.fieldDone   = TRUE;
        if (in) in->reuse       = TRUE;
        if (out) out->reuse     = FALSE;
        return TRUE;
    } else {
        ctx->ctx980.fieldDone = FALSE;
    }
    // Finished encoding a frame
    if(ctx->testEncConfig.outNum != -1 && ctx->frameIdx > (ctx->testEncConfig.outNum-1)) {
        if (ctx->testEncConfig.outNum != ctx->frameIdx) {
            VLOG(ERR, "outnum(%d) != encoded cnt(%d)\n", ctx->testEncConfig.outNum, ctx->frameIdx);
            return FALSE;
        }
        if(out) out->last  = TRUE;  // Send finish signal
        if(out) out->reuse = FALSE;
        ctx->stateDoing    = FALSE;
        com->terminate     = TRUE;
    }

    ctx->frameIdx++;

    return TRUE;
}

static BOOL OpenEncoder(ComponentImpl* com)
{
    EncoderContext*         ctx = (EncoderContext*)com->context;
    SecAxiUse               secAxiUse;
    MirrorDirection         mirrorDirection;
    RetCode                 result;
    CNMComListenerEncOpen   lspn    = {0};

    ctx->stateDoing = TRUE;
    ctx->encOpenParam.bitstreamBuffer     = ctx->bsBuf.bs[0].phys_addr;
    ctx->encOpenParam.bitstreamBufferSize = ctx->bsBuf.bs[0].size;

    if ((result = VPU_EncOpen(&ctx->handle, &ctx->encOpenParam)) != RETCODE_SUCCESS) {
        VLOG(ERR, "VPU_EncOpen failed Error code is 0x%x \n", result);
        if ( result == RETCODE_VPU_RESPONSE_TIMEOUT ) {
            CNMErrorSet(CNM_ERROR_HANGUP);
        }
        CNMAppStop();
        return FALSE;
    }

    lspn.handle = ctx->handle;
    ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_OPEN, (void*)&lspn);

    //Show register Logs
    //VPU_EncGiveCommand(ctx->handle, ENABLE_LOGGING, 0);

    if (ctx->testEncConfig.rotAngle != 0 || ctx->testEncConfig.mirDir != 0) {
        VPU_EncGiveCommand(ctx->handle, ENABLE_ROTATION, 0);
        VPU_EncGiveCommand(ctx->handle, ENABLE_MIRRORING, 0);
        VPU_EncGiveCommand(ctx->handle, SET_ROTATION_ANGLE, &ctx->testEncConfig.rotAngle);
        mirrorDirection = (MirrorDirection)ctx->testEncConfig.mirDir;
        VPU_EncGiveCommand(ctx->handle, SET_MIRROR_DIRECTION, &mirrorDirection);
    }

    osal_memset(&secAxiUse,   0x00, sizeof(SecAxiUse));

    secAxiUse.u.coda9.useBitEnable  = (ctx->testEncConfig.secondaryAXI>>0) & 0x01;
    secAxiUse.u.coda9.useIpEnable   = (ctx->testEncConfig.secondaryAXI>>1) & 0x01;
    secAxiUse.u.coda9.useDbkYEnable = (ctx->testEncConfig.secondaryAXI>>2) & 0x01;
    secAxiUse.u.coda9.useDbkCEnable = (ctx->testEncConfig.secondaryAXI>>3) & 0x01;
    secAxiUse.u.coda9.useBtpEnable  = (ctx->testEncConfig.secondaryAXI>>4) & 0x01;
    secAxiUse.u.coda9.useOvlEnable  = (ctx->testEncConfig.secondaryAXI>>5) & 0x01;

    VPU_EncGiveCommand(ctx->handle, SET_SEC_AXI, &secAxiUse);

    if (ctx->testEncConfig.subFrameSyncEn == TRUE) {
        ctx->subFrameSyncInfo.subFrameSyncCfg.subFrameSyncMode = ctx->testEncConfig.subFrameSyncMode;
        ctx->subFrameSyncInfo.subFrameSyncCfg.subFrameSyncOn = 1;
        ctx->subFrameSyncInfo.subFrameSyncCfg.sourceBufNumber = 3;	// we just use one buffer in reference software
        result = VPU_EncGiveCommand(ctx->handle, ENC_CONFIG_SUB_FRAME_SYNC, &(ctx->subFrameSyncInfo.subFrameSyncCfg));
        if (result != RETCODE_SUCCESS) {
            VLOG(ERR, "VPU_EncGiveCommand ( ENC_CONFIG_SUB_FRAME_SYNC ) failed Error code is 0x%x \n", result);
            return FALSE;
        }
        ctx->subFrameSyncInfo.subFrameSyncState.ipuEndOfRow = 0;
        ctx->subFrameSyncInfo.subFrameSyncState.ipuNewFrame = 0;
        ctx->subFrameSyncInfo.subFrameSyncState.ipuCurFrameIndex = 1;
        VPU_EncGiveCommand(ctx->handle, ENC_SET_SUB_FRAME_SYNC, &ctx->subFrameSyncInfo.subFrameSyncState);
    }


    if (ctx->encOpenParam.coda9RoiEnable) {
        ctx->ctx980.vbRoi.size = VPU_ALIGN256(VPU_ALIGN16(ctx->encOpenParam.picWidth) * VPU_ALIGN16(ctx->encOpenParam.picHeight)/256);  // 1 Byte per 1 MB.

        if (vdi_allocate_dma_memory(ctx->testEncConfig.coreIdx, &ctx->ctx980.vbRoi, ENC_ETC, ctx->handle->instIndex) < 0) {
            VLOG(ERR, "Failed to allocate ROI buffer\n");
            return FALSE;
        }

        if (ctx->testEncConfig.roi_file_name) {
            ChangePathStyle(ctx->testEncConfig.roi_file_name);
            if ((ctx->testEncConfig.roi_file = osal_fopen(ctx->testEncConfig.roi_file_name, "r")) == NULL) {
                VLOG(ERR, "Failed to open ROI file, %s\n", ctx->testEncConfig.roi_file_name);
                return FALSE;
            }
        }
    }

    if (ctx->encOpenParam.EncStdParam.avcParam.fieldFlag) {
        VLOG(INFO, "interlaced field encoding\n");
    }

    ctx->stateDoing = FALSE;

    return TRUE;
}

static BOOL ExecuteEncoder(ComponentImpl* com, PortContainer* in, PortContainer* out)
{
    EncoderContext* ctx             = (EncoderContext*)com->context;
    BOOL            ret;

    switch (ctx->state) {
    case ENCODER_STATE_OPEN:
        ret = OpenEncoder(com);
        if (ctx->stateDoing == FALSE) ctx->state = ENCODER_STATE_INIT_SEQ;
        break;
    case ENCODER_STATE_INIT_SEQ:
        ret = SetSequenceInfo(com);
        if (ctx->stateDoing == FALSE) ctx->state = ENCODER_STATE_REGISTER_FB;
        break;
    case ENCODER_STATE_REGISTER_FB:
        ret = RegisterFrameBuffers(com);
        if (ctx->stateDoing == FALSE) ctx->state = ENCODER_STATE_ENCODE_HEADER;
        break;
    case ENCODER_STATE_ENCODE_HEADER:
        ret = EncodeHeader(com);
        if (ctx->stateDoing == FALSE) ctx->state = ENCODER_STATE_ENCODING;
        break;
    case ENCODER_STATE_ENCODING:
        ret = Encode(com, (PortContainerYuv*)in, (PortContainerES*)out);
        break;
    default:
        ret = FALSE;
        break;
    }

    if (ret == FALSE || com->terminate == TRUE) {
        ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_ENCODED_ALL, (void*)ctx->handle);
        if (out != NULL) {
            out->reuse = FALSE;
            out->last  = TRUE;
        }
    }
    return ret;
}

static BOOL PrepareEncoder(ComponentImpl* com, BOOL* done)
{
    EncoderContext*         ctx       = (EncoderContext*)com->context;
    CNMComponentParamRet    ret;
    BOOL                    success;
    Uint32                  idx =0;
    Uint32                  jdx =0;
    Uint32                  num = 0;

    num = ComponentPortGetSize(&com->sinkPort);

    *done = FALSE;

    ret = ComponentGetParameter(com, com->sinkPort.connectedComponent, GET_PARAM_READER_BITSTREAM_BUF, &ctx->bsBuf);
    if (ComponentParamReturnTest(ret, &success) == FALSE) return success;

    if (num < ctx->bsBuf.num) {
        VLOG(ERR, "number of port is less than number of bitstream.\n");
        return FALSE;
    }

    ctx->encOutQ = Queue_Create(num, sizeof(vpu_buffer_t));

    if (1 == ctx->bsBuf.num) { //Ring-Buffer
        Queue_Enqueue(ctx->encOutQ, (void*)&ctx->bsBuf.bs[0]);
    } else {
        for (idx=0; idx<num; idx++) { //Line-Buffer
            if (jdx > (ctx->bsBuf.num - 1)) {
                jdx =0;
            }
            Queue_Enqueue(ctx->encOutQ, (void*)&ctx->bsBuf.bs[jdx]);
            jdx++;
        }
    }


    *done = TRUE;

    return TRUE;
}

static void ReleaseEncoder(ComponentImpl* com)
{
}

static BOOL DestroyEncoder(ComponentImpl* com)
{
    EncoderContext* ctx = (EncoderContext*)com->context;
    Uint32          i   = 0;
    BOOL            success = TRUE;

    VPU_EncClose(ctx->handle);

    ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_CLOSE, NULL);

    for (i = 0; i < ctx->fbCount.srcFbNum; i++) {
        if (ctx->vbCustomMap[i].size)
            vdi_free_dma_memory(ctx->testEncConfig.coreIdx, &ctx->vbCustomMap[i], ENC_ETC, ctx->handle->instIndex);
    }
    if (ctx->encOpenParam.coda9RoiEnable) {
        vdi_free_dma_memory(ctx->testEncConfig.coreIdx, &ctx->ctx980.vbRoi, ENC_ETC, ctx->handle->instIndex);
    }

    if (ctx->encOutQ) Queue_Destroy(ctx->encOutQ);

    VPU_DeInit(ctx->testEncConfig.coreIdx);

    if (NULL != ctx->encHeaderBuf) {
        osal_free(ctx->encHeaderBuf);
    }
    if (NULL != ctx->encStreamBufInfo.encodedStreamBuf) {
        osal_free(ctx->encStreamBufInfo.encodedStreamBuf);
    }

    osal_free(ctx);

    return success;
}

static Component CreateEncoder(ComponentImpl* com, CNMComponentConfig* componentParam)
{
    EncoderContext* ctx;
    RetCode         retCode;
    Uint32          coreIdx      = componentParam->testEncConfig.coreIdx;
    Uint16*         firmware     = (Uint16*)componentParam->bitcode;
    Uint32          firmwareSize = componentParam->sizeOfBitcode;
    Uint32          i;

    retCode = VPU_InitWithBitcode(coreIdx, firmware, firmwareSize);
    if (retCode != RETCODE_SUCCESS && retCode != RETCODE_CALLED_BEFORE) {
        VLOG(INFO, "%s:%d Failed to VPU_InitWidthBitCode, ret(%08x)\n", __FUNCTION__, __LINE__, retCode);
        return FALSE;
    }

    com->context = osal_malloc(sizeof(EncoderContext));
    ctx     = (EncoderContext*)com->context;
    osal_memset((void*)ctx, 0, sizeof(EncoderContext));

    ctx->handle                 = NULL;
    ctx->frameIdx               = 0;
    ctx->fbCount.reconFbNum     = 0;
    ctx->fbCount.srcFbNum       = 0;
    ctx->testEncConfig          = componentParam->testEncConfig;
    ctx->encOpenParam           = componentParam->encOpenParam;
    ctx->encHeaderSize          = 0;
    ctx->encHeaderBuf           = NULL;
    ctx->encodedReConFrmIdx     = 0;
    ctx->ctx980.encCfg          = componentParam->encCfg;
    osal_memset(&(ctx->encStreamBufInfo), 0x00, sizeof(EncodedStreamBufInfo));
    ctx->ctx980.fieldDone        = FALSE;

    for (i = 0; i < ENC_SRC_BUF_NUM; i++) {
        ctx->encodedSrcFrmIdxArr[i] = 0;
        if (ctx->testEncConfig.subFrameSyncEn)
            ctx->encodedSrcFrmIdxArr[i] = 1;
    }
    return (Component)com;
}

ComponentImpl coda9EncoderComponentImpl = {
    "coda9_encoder",
    NULL,
    {0,},
    {0,},
    sizeof(PortContainerES),
    5,                       /* encoder's numSinkPortQueue(related to streambufcount) */
    CreateEncoder,
    GetParameterEncoder,
    SetParameterEncoder,
    PrepareEncoder,
    ExecuteEncoder,
    ReleaseEncoder,
    DestroyEncoder
};

