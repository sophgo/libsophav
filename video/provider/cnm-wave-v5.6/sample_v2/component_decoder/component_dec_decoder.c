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
#include "cnm_app.h"
#include "misc/debug.h"

#define EXTRA_FRAME_BUFFER_NUM 1

#define DEC_DESTROY_TIME_OUT (60000*2) //2 min

#define USERDATA_BUFFER_SIZE (1320*1024);

typedef enum {
    DEC_INT_STATUS_ERR = -1,
    DEC_INT_STATUS_NONE,        // Interrupt not asserted yet
    DEC_INT_STATUS_EMPTY,       // Need more es
    DEC_INT_STATUS_DONE,        // Interrupt asserted
    DEC_INT_STATUS_TIMEOUT,     // Interrupt not asserted during given time.
} DEC_INT_STATUS;

typedef enum {
    DEC_STATE_NONE,
    DEC_STATE_OPEN_DECODER,
    DEC_STATE_INIT_SEQ,
    DEC_STATE_REGISTER_FB,
    DEC_STATE_DECODING,
    DEC_STATE_RELEASE,
    DEC_STATE_DESTROY,
} DecoderState;

typedef enum {
    DEC_HOST_SCENARIO_INT       = 0x00,
    DEC_HOST_SCENARIO_INT_CQ    = 0x01,
    DEC_HOST_SCENARIO_PICEND    = 0x02,
    DEC_HOST_SCENARIO_PICEND_CQ = 0x03,
    DEC_HOST_SCENARIO_MAX       = 0xFF,
} DecoderHostScenario;

typedef struct {
    Uint32 interrupt_dec_init_seq;
    Uint32 interrupt_dec_pic;
    Uint32 interrupt_dec_bs_empty;
} DecoderInterruptFlag;

typedef struct {
    TestDecConfig           testDecConfig;
    DecOpenParam            decOpenParam;
    DecParam                decParam;
    FrameBufferFormat       wtlFormat;
    DecHandle               handle;
    Uint64                  startTimeout;
    Uint64                  desStTimeout;
    Uint32                  iterationCnt;
    vpu_buffer_t            vbUserData[COMMAND_QUEUE_DEPTH];
    vpu_buffer_t            vbWork;
    vpu_buffer_t            vbTemp;
    vpu_buffer_t            vbSecAxi;
    vpu_buffer_t            vbAr;
    vpu_buffer_t            vbAux[AUX_BUF_TYPE_MAX][MAX_FRAME_NUM];
    Uint32                  nonLinearNum;
    Queue*                  userDataQ;
    BOOL                    doFlush;
    BOOL                    stateDoing;
    DecoderState            state;
    DecInitialInfo          initialInfo;
    Uint32                  numDecoded;             /*!<< The number of decoded frames */
    Uint32                  numOutput;
    PhysicalAddress         decodedAddr;
    Uint32                  frameNumToStop;
    BOOL                    doingReset;
    Uint32                  cyclePerTick;
    Uint32                  chromaIDCFlag;
    size_t                  bsSize;
    VpuAttr                 attr;
    Int32                   sequenceNo;
    Int32                   compSync;
    BOOL                    force_stop;
    DecoderInterruptFlag    dec_interrupt_flag;
    struct {
        BOOL    enable;
        Uint32  skipCmd;                        /*!<< a skip command to be restored */
    } autoErrorRecovery;
} DecoderContext;

static BOOL AllocateworkBuffer(ComponentImpl* com)
{
    DecoderContext* ctx       = (DecoderContext*)com->context;
    TestDecConfig testDecConfig = ctx->testDecConfig;
    DecOpenParam* pOpenParam = &ctx->decOpenParam;

    ctx->vbWork.phys_addr = 0;
    if (ctx->testDecConfig.productId == PRODUCT_ID_617)
        ctx->vbWork.size = (Uint32)WAVE637DEC_WORKBUF_SIZE;
    else if (ctx->testDecConfig.productId == PRODUCT_ID_637)
        ctx->vbWork.size = (Uint32)WAVE637DEC_WORKBUF_SIZE;

    if (vdi_allocate_dma_memory(testDecConfig.coreIdx, &ctx->vbWork, DEC_WORK, 0) < 0)
        return RETCODE_INSUFFICIENT_RESOURCE;

#ifdef WAVE6_FW_WORKBUF_MEMSET
#else
    vdi_clear_memory(testDecConfig.coreIdx, ctx->vbWork.phys_addr, ctx->vbWork.size, 0);
#endif

    pOpenParam->instBuffer.workBufBase = ctx->vbWork.phys_addr;
    pOpenParam->instBuffer.workBufSize = ctx->vbWork.size;

    return TRUE;
}

static BOOL AllocateTempBuffer(ComponentImpl* com)
{
    DecoderContext* ctx       = (DecoderContext*)com->context;
    TestDecConfig testDecConfig = ctx->testDecConfig;
    DecOpenParam* pOpenParam = &ctx->decOpenParam;

    ctx->vbTemp.phys_addr = 0;
    ctx->vbTemp.size      = VPU_ALIGN4096(WAVE6_TEMPBUF_SIZE);
    if (vdi_allocate_dma_memory(testDecConfig.coreIdx, &ctx->vbTemp, DEC_TEMP, 0) < 0)
        return RETCODE_INSUFFICIENT_RESOURCE;
    pOpenParam->instBuffer.tempBufBase = ctx->vbTemp.phys_addr;
    pOpenParam->instBuffer.tempBufSize = ctx->vbTemp.size;

    return TRUE;
}

static BOOL AllocateSecAxiBuffer(ComponentImpl* com)
{
    DecoderContext* ctx       = (DecoderContext*)com->context;
    TestDecConfig testDecConfig = ctx->testDecConfig;
    DecOpenParam* pOpenParam = &ctx->decOpenParam;

    if (vdi_get_sram_memory(testDecConfig.coreIdx, &ctx->vbSecAxi) < 0)
        return RETCODE_INSUFFICIENT_RESOURCE;

    pOpenParam->instBuffer.secAxiBufBase = ctx->vbSecAxi.phys_addr;
    pOpenParam->instBuffer.secAxiBufSize = ctx->vbSecAxi.size;

    return TRUE;
}

static RetCode AllocateAuxBuffer(ComponentImpl* com, DecAuxBufferSizeInfo sizeInfo, AuxBufferType type, MemTypes memTypes)
{
    DecoderContext* ctx = (DecoderContext*)com->context;
    AuxBufferInfo auxBufferInfo;
    AuxBuffer bufArr[MAX_REG_FRAME];
    Uint32 i;
    RetCode ret_code = RETCODE_FAILURE;

    sizeInfo.type = type;
    auxBufferInfo.type = type;
    auxBufferInfo.num = ctx->nonLinearNum;

    if (type == AUX_BUF_DEF_CDF     ||
        type == AUX_BUF_SEG_MAP) {
        auxBufferInfo.num = 1;
    }

    osal_memset(bufArr, 0x00, sizeof(bufArr));
    auxBufferInfo.bufArray = bufArr;

    for (i = 0; i < auxBufferInfo.num; i++) {
        if ((ret_code = VPU_DecGetAuxBufSize(ctx->handle, sizeInfo, &ctx->vbAux[type][i].size)) != RETCODE_SUCCESS) {
            break;
        } else {
            if (vdi_allocate_dma_memory(ctx->handle->coreIdx, &ctx->vbAux[type][i], memTypes, ctx->handle->instIndex) < 0) {
                ret_code = RETCODE_INSUFFICIENT_RESOURCE;
                break;
            }
            auxBufferInfo.bufArray[i].index = i;
            auxBufferInfo.bufArray[i].addr = ctx->vbAux[type][i].phys_addr;
            auxBufferInfo.bufArray[i].size = ctx->vbAux[type][i].size;
        }
    }

    if (ret_code == RETCODE_SUCCESS) {
        // host to API, to used
        ret_code = VPU_DecRegisterAuxBuffer(ctx->handle, auxBufferInfo);
    }

    return ret_code;
}

static void FreeAuxBuffer(ComponentImpl* com, Int32 idx, AuxBufferType type, MemTypes memTypes)
{
    DecoderContext* ctx = (DecoderContext*)com->context;

    if (ctx->vbAux[type][idx].size) {
        vdi_free_dma_memory(ctx->testDecConfig.coreIdx, &ctx->vbAux[type][idx], memTypes, VPU_HANDLE_INSTANCE_NO(ctx->handle));
    }
}


static void LockCompSync(Int32* compSync) {
    LockCompMutex();
    if (compSync) {
        (*compSync)++;
    }
    UnlockCompMutex();
}

static void UnLockCompSync(Int32* compSync) {
    LockCompMutex();
    if (compSync) {
        (*compSync)--;
    }
    UnlockCompMutex();
}

static BOOL CheckCompSync(DecoderContext* ctx) {
    if (ctx->compSync > 0) {
        return TRUE;
    }
    return FALSE;
}

static BOOL SetInterruptFlag(DecoderContext* ctx)
{
    if (!ctx) {
        VLOG(ERR, "%s:%d Null pointer exception \n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    if (PRODUCT_ID_W6_SERIES(ctx->attr.productId)) {
        ctx->dec_interrupt_flag.interrupt_dec_init_seq  = INT_WAVE6_INIT_SEQ;
        ctx->dec_interrupt_flag.interrupt_dec_pic       = INT_WAVE6_DEC_PIC;
        ctx->dec_interrupt_flag.interrupt_dec_bs_empty  = INT_WAVE6_BSBUF_EMPTY;
    } else if (PRODUCT_ID_W5_SERIES(ctx->attr.productId)) {
        ctx->dec_interrupt_flag.interrupt_dec_init_seq  = INT_WAVE5_INIT_SEQ;
        ctx->dec_interrupt_flag.interrupt_dec_pic       = INT_WAVE5_DEC_PIC;
        ctx->dec_interrupt_flag.interrupt_dec_bs_empty  = INT_WAVE5_BSBUF_EMPTY;
    } else {
        VLOG(ERR, "%s:%d Check product id : %d \n", __FUNCTION__, __LINE__, ctx->attr.productId);
        return FALSE;
    }

    return TRUE;
}


static BOOL HandleInterruptReason(DecoderContext* ctx, DEC_INT_STATUS status, BOOL *re_cmd)
{
    Uint32 host_scenario = DEC_HOST_SCENARIO_MAX;
    Uint32 support_cq = 0;
    Uint32 stream_mode = 0;
    BOOL ret = FALSE;

    if (ctx == NULL || re_cmd == NULL) {
        VLOG(ERR, "%s:%d Null point exception \n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    support_cq = ctx->attr.supportCommandQueue;
    stream_mode = ctx->decOpenParam.bitstreamMode;
    host_scenario = support_cq | stream_mode;

    if (status == DEC_INT_STATUS_DONE) {
        return TRUE;
    } else if (status == DEC_INT_STATUS_EMPTY) {
    } else if (status == DEC_INT_STATUS_NONE) {
    } else {
        VLOG(ERR, "%s:%d status : %d , host scenario : %d \n", __FUNCTION__, __LINE__, status, host_scenario);
        return FALSE;
    }

    if (host_scenario == DEC_HOST_SCENARIO_INT) {
        ret = (status == DEC_INT_STATUS_EMPTY) ? TRUE : FALSE;
        *re_cmd = ret;
    } else if (host_scenario == DEC_HOST_SCENARIO_INT_CQ) {
        ret = TRUE;
        *re_cmd = ret;
    } else if (host_scenario == DEC_HOST_SCENARIO_PICEND) {
    } else if (host_scenario == DEC_HOST_SCENARIO_PICEND_CQ) {
        ret = (status == DEC_INT_STATUS_NONE) ? TRUE : FALSE;
        *re_cmd = ret;
    } else {
    }

    if (ret == FALSE) {
        VLOG(WARN, "%s:%d status : %d , host scenario : %d \n", __FUNCTION__, __LINE__, status, host_scenario);
    }

    return ret;
}

static BOOL RegisterFrameBuffers(ComponentImpl* com)
{
    DecoderContext*         ctx               = (DecoderContext*)com->context;
    FrameBuffer*            pFrame            = NULL;
    Uint32                  framebufStride    = 0;
    ParamDecFrameBuffer     paramFb;
    RetCode                 result;
    DecInitialInfo*         codecInfo         = &ctx->initialInfo;
    BOOL                    success;
    CNMComponentParamRet    comp_ret_code;
    RetCode                 ret_code;
    CNMComListenerDecRegisterFb  lsnpRegisterFb;

    ctx->stateDoing = TRUE;
    comp_ret_code = ComponentGetParameter(com, com->sinkPort.connectedComponent, GET_PARAM_RENDERER_FRAME_BUF, (void*)&paramFb);
    if (ComponentParamReturnTest(comp_ret_code, &success) == FALSE) {
        return success;
    }

    pFrame               = paramFb.fb;
    framebufStride       = paramFb.stride;
    VLOG(TRACE, "<%s> COMPRESSED: %d, LINEAR: %d\n", __FUNCTION__, paramFb.nonLinearNum, paramFb.linearNum);

    ctx->nonLinearNum = paramFb.nonLinearNum;
    if (PRODUCT_ID_W6_SERIES(ctx->testDecConfig.productId)) {
        DecAuxBufferSizeInfo sizeInfo = {0,};
        sizeInfo.width = codecInfo->picWidth;
        sizeInfo.height = codecInfo->picHeight;

        if ((ret_code = AllocateAuxBuffer(com, sizeInfo, AUX_BUF_FBC_Y_TBL, DEC_FBCY_TBL)) != RETCODE_SUCCESS) {
            VLOG(ERR, "<%s:%d> return code : %d \n", __FUNCTION__, __LINE__, ret_code);
            return FALSE;
        }

        if ((ret_code = AllocateAuxBuffer(com, sizeInfo, AUX_BUF_FBC_C_TBL, DEC_FBCC_TBL)) != RETCODE_SUCCESS) {
            VLOG(ERR, "<%s:%d> return code : %d \n", __FUNCTION__, __LINE__, ret_code);
            return FALSE;
        }

        if ((ret_code = AllocateAuxBuffer(com, sizeInfo, AUX_BUF_MV_COL, DEC_MV)) != RETCODE_SUCCESS) {
            VLOG(ERR, "<%s:%d> return code : %d \n", __FUNCTION__, __LINE__, ret_code);
            return FALSE;
        }

        if (ctx->decOpenParam.bitstreamFormat == STD_AV1) {
            if ((ret_code = AllocateAuxBuffer(com, sizeInfo, AUX_BUF_DEF_CDF, DEC_DEF_CDF)) != RETCODE_SUCCESS) {
                VLOG(ERR, "<%s:%d> return code : %d \n", __FUNCTION__, __LINE__, ret_code);
                return FALSE;
            }
        }
        if (ctx->decOpenParam.bitstreamFormat == STD_VP9) {
            if ((ret_code = AllocateAuxBuffer(com, sizeInfo, AUX_BUF_SEG_MAP, DEC_SEG_MAP)) != RETCODE_SUCCESS) {
                VLOG(ERR, "<%s:%d> return code : %d \n", __FUNCTION__, __LINE__, ret_code);
                return FALSE;
            }
        }
    }

    if ((ctx->attr.productId == PRODUCT_ID_521) && ctx->attr.supportDualCore == TRUE) {
        if (ctx->initialInfo.lumaBitdepth == 8 && ctx->initialInfo.chromaBitdepth == 8)
            result = VPU_DecRegisterFrameBufferEx(ctx->handle, pFrame, paramFb.nonLinearNum, paramFb.linearNum, framebufStride, codecInfo->picHeight, COMPRESSED_FRAME_MAP_DUAL_CORE_8BIT);
        else
            result = VPU_DecRegisterFrameBufferEx(ctx->handle, pFrame, paramFb.nonLinearNum, paramFb.linearNum, framebufStride, codecInfo->picHeight, COMPRESSED_FRAME_MAP_DUAL_CORE_10BIT);
    }
    else {
        result = VPU_DecRegisterFrameBufferEx(ctx->handle, pFrame, paramFb.nonLinearNum, paramFb.linearNum, framebufStride, codecInfo->picHeight, COMPRESSED_FRAME_MAP);
    }

    lsnpRegisterFb.handle          = ctx->handle;
    lsnpRegisterFb.numNonLinearFb  = paramFb.nonLinearNum;
    lsnpRegisterFb.numLinearFb     = paramFb.linearNum;
    ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_REGISTER_FB, (void*)&lsnpRegisterFb);

    if (result != RETCODE_SUCCESS) {
        VLOG(ERR, "%s:%d Failed to VPU_DecRegisterFrameBufferEx(%d)\n", __FUNCTION__, __LINE__, result);
        ChekcAndPrintDebugInfo(ctx->handle, FALSE, result);
        return FALSE;
    }




    ctx->stateDoing = FALSE;

    return TRUE;
}

static BOOL SequenceChange(ComponentImpl* com, DecOutputInfo* outputInfo)
{
    DecoderContext* ctx               = (DecoderContext*)com->context;
    DecInitialInfo  initialInfo;
    BOOL            dpbChanged, sizeChanged, bitDepthChanged, chromaIDCChanged;
    BOOL            intReschanged = FALSE;
    Uint32          sequenceChangeFlag = outputInfo->sequenceChanged;
    ParamDecReallocFB decReallocFB;
    Int32 fbc_idx = -1, mv_col_idx = -1;

    dpbChanged      = (sequenceChangeFlag&SEQ_CHANGE_ENABLE_DPB_COUNT) ? TRUE : FALSE;
    sizeChanged     = (sequenceChangeFlag&SEQ_CHANGE_ENABLE_SIZE)      ? TRUE : FALSE;
    bitDepthChanged = (sequenceChangeFlag&SEQ_CHANGE_ENABLE_BITDEPTH)  ? TRUE : FALSE;
    chromaIDCChanged = (sequenceChangeFlag&SEQ_CHANGE_CHROMA_FORMAT_IDC)  ? TRUE : FALSE;

    if (STD_VP9 == ctx->decOpenParam.bitstreamFormat) {
        intReschanged = (sequenceChangeFlag&SEQ_CHANGE_INTER_RES_CHANGE)  ? TRUE : FALSE;
    }

    if (dpbChanged || sizeChanged || bitDepthChanged || intReschanged || chromaIDCChanged) {

        VPU_DecGiveCommand(ctx->handle, DEC_GET_SEQ_INFO, &initialInfo);

        if (FALSE == intReschanged) {
            VLOG(INFO, "----- SEQUENCE CHANGED -----\n");
        } else {
            VLOG(INFO, "----- INTER RESOLUTION CHANGED -----\n");
            decReallocFB.indexInterFrameDecoded = outputInfo->indexInterFrameDecoded;
            decReallocFB.width = initialInfo.picWidth;
            decReallocFB.height = initialInfo.picHeight;
        }
        // Get current(changed) sequence information.
        // Flush all remaining framebuffers of previous sequence.
        VLOG(INFO, "sequenceChanged : %x\n", sequenceChangeFlag);
        VLOG(INFO, "INFO SEQUENCE NO     : %d\n", initialInfo.sequenceNo);
        VLOG(INFO, "OUTPUT SEQUENCE NO   : %d\n", outputInfo->sequenceNo);
        VLOG(INFO, "DPB COUNT       : %d\n", initialInfo.minFrameBufferCount);
        VLOG(INFO, "BITDEPTH        : LUMA(%d), CHROMA(%d)\n", initialInfo.lumaBitdepth, initialInfo.chromaBitdepth);
        VLOG(INFO, "SIZE            : WIDTH(%d), HEIGHT(%d)\n", initialInfo.picWidth, initialInfo.picHeight);
        VLOG(INFO, "DISPFLAG        : 0x%x\n", outputInfo->frameDisplayFlag);

        if (FALSE == intReschanged) {
            PhysicalAddress rd_ptr;
            ctx->sequenceNo = initialInfo.sequenceNo;
            VPU_DecGetBitstreamBuffer(ctx->handle, &rd_ptr, NULL, NULL);
            VLOG(WARN, "Seq-changed RD PTR : 0x%x \n", rd_ptr);
            ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_RENDERER_FREE_FRAMEBUFFERS, (void*)&outputInfo->frameDisplayFlag);
            VPU_DecGiveCommand(ctx->handle, DEC_RESET_FRAMEBUF_INFO, NULL);
        } else {
            ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_RENDERER_INTRES_CHANGED_FREE_FRAMEBUFFERS, (void*)&decReallocFB);
        }

        ScalerInfo sclInfo = {0};
        if (ctx->testDecConfig.scaleDownWidth > 0 || ctx->testDecConfig.scaleDownHeight > 0) {
            sclInfo.scaleWidth  = CalcScaleDown(initialInfo.picWidth, ctx->testDecConfig.scaleDownWidth);
            sclInfo.scaleHeight = CalcScaleDown(initialInfo.picHeight, ctx->testDecConfig.scaleDownHeight);
            VLOG(INFO, "[SCALE INFO] %dx%d to %dx%d\n", initialInfo.picWidth, initialInfo.picHeight, sclInfo.scaleWidth, sclInfo.scaleHeight);
            sclInfo.enScaler    = TRUE;
            if (VPU_DecGiveCommand(ctx->handle, DEC_SET_SCALER_INFO, (void*)&sclInfo) != RETCODE_SUCCESS) {
                VLOG(ERR, "Failed to VPU_DecGiveCommand(DEC_SET_SCALER_INFO)\n");
                return FALSE;
            }
        }
        osal_memcpy((void*)&ctx->initialInfo, (void*)&initialInfo, sizeof(DecInitialInfo));
        if (FALSE == intReschanged) {
            ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_RENDERER_ALLOC_FRAMEBUFFERS, NULL);
            ctx->state = DEC_STATE_REGISTER_FB;
        } else {
            if (ctx->attr.productId == PRODUCT_ID_637) {
                fbc_idx = (Int8)(outputInfo->indexInterFrameDecoded & 0xff);
                mv_col_idx = (Int8)(outputInfo->indexInterFrameDecoded >> 16 & 0xff);
                if (fbc_idx >= 0) {
                    FreeAuxBuffer(com , fbc_idx, AUX_BUF_FBC_Y_TBL, DEC_FBCY_TBL);
                    FreeAuxBuffer(com , fbc_idx, AUX_BUF_FBC_C_TBL, DEC_FBCC_TBL);
                }
                if (mv_col_idx >= 0) {
                    FreeAuxBuffer(com , mv_col_idx, AUX_BUF_MV_COL, DEC_MV);
                }
            }
            ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_RENDERER_INTRES_CHANGED_ALLOC_FRAMEBUFFERS, (void*)&decReallocFB);
        }
        VLOG(INFO, "----------------------------\n");
    }

    return TRUE;
}

static Int32 CheckChromaFormatFlag(ComponentImpl* com, DecOutputInfo* const outputInfo)
{
    DecoderContext* ctx     = (DecoderContext*)com->context;
    Int32 chromaIDCFlag     = 0;
    Uint32 sequenceChangeFlag = outputInfo->sequenceChanged;

    if (0 != sequenceChangeFlag) {
        DecInfo decInfo;
        VPU_DecGiveCommand(ctx->handle, DEC_GET_SEQ_INFO, &decInfo);
        chromaIDCFlag = decInfo.initialInfo.chromaFormatIDC;
    } else {
        chromaIDCFlag = ctx->chromaIDCFlag;
    }
    return chromaIDCFlag;
}

static BOOL CheckAndDoSequenceChange(ComponentImpl* com, DecOutputInfo* outputInfo)
{
    if (outputInfo->sequenceChanged == 0) {
        return TRUE;
    }
    else {
        return SequenceChange(com, outputInfo);
    }
}

static void ClearDpb(ComponentImpl* com, BOOL backupDpb)
{
    DecoderContext* ctx             = (DecoderContext*)com->context;
    Uint32          timeoutCount;
    Int32           intReason;
    DecOutputInfo   outputInfo;
    BOOL            pause;
    Uint32          idx;
    Uint32          flushedFbs      = 0;
    QueueStatusInfo cqInfo;
    const Uint32    flushTimeout = 2000; // 2 sec

    if (TRUE == backupDpb) {
        pause = TRUE;
        ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_COM_PAUSE, (void*)&pause);
    }

    /* Send the renderer the signal to drop all frames.
     * VPU_DecClrDispFlag() is called in SE_PARAM_RENDERER_FLUSH.
     */
    ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_RENDERER_FLUSH, (void*)&flushedFbs);

    while (RETCODE_SUCCESS == VPU_DecGetOutputInfo(ctx->handle, &outputInfo)) {
        if (0 <= outputInfo.indexFrameDisplay) {
            flushedFbs |= outputInfo.indexFrameDisplay;
            VPU_DecClrDispFlag(ctx->handle, outputInfo.indexFrameDisplay);
            VLOG(INFO, "<%s> FLUSH DPB INDEX: %d\n", __FUNCTION__, outputInfo.indexFrameDisplay);
        }
        osal_msleep(1);
    }

    VLOG(INFO, "========== FLUSH FRAMEBUFFER & CMDs ========== \n");
    timeoutCount = 0;
    while (VPU_DecFrameBufferFlush(ctx->handle, NULL, NULL) == RETCODE_VPU_STILL_RUNNING) {
        // Clear an interrupt
        if (0 < (intReason=VPU_WaitInterruptEx(ctx->handle, VPU_WAIT_TIME_OUT_CQ))) {
            VPU_ClearInterruptEx(ctx->handle, intReason);
            VPU_DecGetOutputInfo(ctx->handle, &outputInfo);  // ignore the return value and outputinfo
            if (0 <= outputInfo.indexFrameDisplay) {
                flushedFbs |= outputInfo.indexFrameDisplay;
                VPU_DecClrDispFlag(ctx->handle, outputInfo.indexFrameDisplay);
            }
        }

        if (timeoutCount >= flushTimeout) {
            VLOG(ERR, "NO RESPONSE FROM VPU_DecFrameBufferFlush()\n");
            return;
        }
        timeoutCount++;
        osal_msleep(1);
    }

    VPU_DecGetOutputInfo(ctx->handle, &outputInfo);
    VPU_DecGiveCommand(ctx->handle, DEC_GET_QUEUE_STATUS, &cqInfo);
    VLOG(INFO, "<%s> REPORT_QUEUE(%d), INSTANCE_QUEUE(%d)\n", __FUNCTION__, cqInfo.reportQueueCount, cqInfo.instanceQueueCount);

    if (TRUE == backupDpb) {
        for (idx=0; idx<32; idx++) {
            if (flushedFbs & (1<<idx)) {
                VLOG(INFO, "SET DISPLAY FLAG : %d\n", idx);
                VPU_DecGiveCommand(ctx->handle, DEC_SET_DISPLAY_FLAG , &idx);
            }
        }
        pause = FALSE;
        ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_COM_PAUSE, (void*)&pause);
    }
}

static void ClearCpb(ComponentImpl* com)
{
    DecoderContext* ctx = (DecoderContext*)com->context;
    PhysicalAddress curRdPtr, curWrPtr;

    if (BS_MODE_INTERRUPT == ctx->decOpenParam.bitstreamMode) {
        /* Clear CPB */
        // In order to stop processing bitstream.
        VPU_DecUpdateBitstreamBuffer(ctx->handle, EXPLICIT_END_SET_FLAG);
        VPU_DecGetBitstreamBuffer(ctx->handle, &curRdPtr, &curWrPtr, NULL);
        VPU_DecSetRdPtr(ctx->handle, curWrPtr, TRUE);
        VLOG(INFO, "CLEAR CPB(RD_PTR: %08x, WR_PTR: %08x)\n", curWrPtr, curWrPtr);
    }
}

static BOOL PrepareSkip(ComponentImpl* com)
{
    DecoderContext*   ctx = (DecoderContext*)com->context;

    // Flush the decoder
    if (ctx->doFlush == TRUE) {
        ClearDpb(com, FALSE);
        if (STD_VP9 != ctx->decOpenParam.bitstreamFormat && STD_AV1 != ctx->decOpenParam.bitstreamFormat) {
            ClearCpb(com);
        }
        ctx->doFlush = FALSE;
    }

    return TRUE;
}

static DEC_INT_STATUS HandlingInterruptFlag(ComponentImpl* com)
{
    DecoderContext*         ctx               = (DecoderContext*)com->context;
    DecHandle               handle            = ctx->handle;
    Int32                   interruptFlag     = 0;
    Uint32                  interruptWaitTime = VPU_WAIT_TIME_OUT_CQ;
    Uint32                  interruptTimeout  = VPU_DEC_TIMEOUT;
    DEC_INT_STATUS          status            = DEC_INT_STATUS_NONE;
    CNMComListenerDecInt    lsn;
    Uint64                  currentTimeout = 0;


    if (ctx->attr.supportCommandQueue == FALSE) {
        interruptWaitTime = interruptTimeout;
    }

#ifdef SUPPORT_GDB
    if (__VPU_BUSY_TIMEOUT == 0) interruptTimeout = 0;
#endif

    if (ctx->startTimeout == 0ULL) {
        ctx->startTimeout = osal_gettime();
    }
    do {
        interruptFlag = VPU_WaitInterruptEx(handle, interruptWaitTime);
        if (INTERRUPT_TIMEOUT_VALUE == interruptFlag) {
            currentTimeout = osal_gettime();
            if (0 < interruptTimeout && (currentTimeout - ctx->startTimeout) > interruptTimeout) {
                VLOG(ERR, "INSTANCE #%d INTERRUPT TIMEOUT.\n", handle->instIndex);
                status = DEC_INT_STATUS_TIMEOUT;
                break;
            }
            interruptFlag = 0;
        }

        if (interruptFlag < 0) {
            VLOG(ERR, "<%s:%d> interruptFlag is negative value! %08x\n", __FUNCTION__, __LINE__, interruptFlag);
        }

        if (interruptFlag > 0) {
            VPU_ClearInterruptEx(handle, interruptFlag);
            ctx->startTimeout = 0ULL;
            status = DEC_INT_STATUS_DONE;
            if (interruptFlag & (1<< (ctx->dec_interrupt_flag.interrupt_dec_init_seq) )) {
                break;
            }

            if (interruptFlag & (1<< (ctx->dec_interrupt_flag.interrupt_dec_pic) )) {
                break;
            }

            if (interruptFlag & (1<< (ctx->dec_interrupt_flag.interrupt_dec_bs_empty) )) {
                status = DEC_INT_STATUS_EMPTY;
                break;
            }
        }
    } while (FALSE);

    if (interruptFlag != 0) {
        lsn.handle   = handle;
        lsn.flag     = interruptFlag;
        lsn.decIndex = ctx->numDecoded;
        ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_INTERRUPT, (void*)&lsn);
    }

    return status;
}

static BOOL DoReset(ComponentImpl* com)
{
    DecoderContext* ctx = (DecoderContext*)com->context;
    DecOutputInfo outputInfo;
    Uint32 ret;
    BOOL pause  = TRUE;
    PhysicalAddress curRdPtr, curWrPtr;
    Uint32 timeout = VPU_DEC_TIMEOUT;
    Uint64 currentTimeout = 0;

#ifdef SUPPORT_GDB
    extern Uint32 __VPU_BUSY_TIMEOUT;
    if (__VPU_BUSY_TIMEOUT == 0) return TRUE;
#endif /* SUPPORT_GDB */

    VLOG(INFO, "========== %s ==========\n", __FUNCTION__);

    ComponentSetParameter(com, com->srcPort.connectedComponent, SET_PARAM_COM_PAUSE, (void*)&pause);
    ComponentSetParameter(com, com->srcPort.connectedComponent, SET_PARAM_FEEDER_RESET, (void*)&pause);

    VPU_DecUpdateBitstreamBuffer(ctx->handle, EXPLICIT_END_SET_FLAG);

    //ClearDpb(com, FALSE);

    while (RETCODE_SUCCESS == VPU_DecGetOutputInfo(ctx->handle, &outputInfo)) {
        if (0 <= outputInfo.indexFrameDisplay) {
            VPU_DecClrDispFlag(ctx->handle, outputInfo.indexFrameDisplay);
            VLOG(INFO, "<%s> FLUSH DPB INDEX: %d\n", __FUNCTION__, outputInfo.indexFrameDisplay);
        }
        osal_msleep(1);
    }

    VLOG(INFO, "> Reset VPU\n");

    if (0ULL == ctx->startTimeout) {
        ctx->startTimeout = osal_gettime();
    }

    ret = VPU_SWReset(ctx->handle->coreIdx, SW_RESET_SAFETY, ctx->handle);
    currentTimeout = osal_gettime();
    if (RETCODE_SUCCESS != ret) {
        if (RETCODE_VPU_STILL_RUNNING == ret) {
            if ((currentTimeout - ctx->startTimeout) > timeout) {
                VLOG(ERR, "INSTANCE #%d VPU SWRest TIMEOUT.\n", ctx->handle->instIndex);
                return FALSE;
            }
            return TRUE;
        } else {
            VLOG(ERR, "<%s:%d> Failed to VPU_SWReset() ret(%d)\n", __FUNCTION__, __LINE__, ret);
            return FALSE;
        }
    }

    //ClearCpb(com);
    if (BS_MODE_INTERRUPT == ctx->decOpenParam.bitstreamMode) {
        VPU_DecGetBitstreamBuffer(ctx->handle, &curRdPtr, &curWrPtr, NULL);
        VPU_DecSetRdPtr(ctx->handle, curWrPtr, TRUE);
    }

    VLOG(INFO, "========== %s ==========\n", __FUNCTION__);

    ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_RESET_DONE, NULL);

    VLOG(INFO, "> FLUSH INPUT PORT\n");
    ComponentPortFlush(com);

    pause = FALSE;
    ComponentSetParameter(com, com->srcPort.connectedComponent, SET_PARAM_COM_PAUSE, (void*)&pause);

    ctx->doingReset                 = FALSE;
    ctx->startTimeout               = 0ULL;
    ctx->autoErrorRecovery.enable   = TRUE;
    ctx->autoErrorRecovery.skipCmd  = ctx->decParam.skipframeMode;
    ctx->decParam.skipframeMode     = WAVE_SKIPMODE_NON_IRAP;

    VLOG(INFO, "========== %s Finished==========\n", __FUNCTION__);

    return TRUE;
}




static BOOL HandleDecodedInfo(ComponentImpl* com, DecOutputInfo* p_out_info, PortContainerES* in, PortContainerDisplay* out)
{
    DecoderContext* ctx = (DecoderContext*)com->context;

    if (p_out_info->decodingSuccess & 0x01) {
        if (TRUE == ctx->autoErrorRecovery.enable) {
            ctx->decParam.skipframeMode     = ctx->autoErrorRecovery.skipCmd;
            ctx->autoErrorRecovery.enable   = FALSE;
            ctx->autoErrorRecovery.skipCmd  = WAVE_SKIPMODE_WAVE_NONE;
        }
    }

    if (CheckAndDoSequenceChange(com, p_out_info) == FALSE) {
        return FALSE;
    }

    if (p_out_info->indexFrameDecoded >= 0 || p_out_info->indexFrameDecoded == DECODED_IDX_FLAG_SKIP) {
        ctx->numDecoded++;
        ctx->decodedAddr = p_out_info->bytePosFrameStart;
        // Return a used data to a source port.
        out->reuse = FALSE;
    }

    if ((p_out_info->indexFrameDisplay >= 0) || (p_out_info->indexFrameDisplay == DISPLAY_IDX_FLAG_SEQ_END)) {
        ctx->numOutput++;
        out->last  = (BOOL)(p_out_info->indexFrameDisplay == DISPLAY_IDX_FLAG_SEQ_END);
        out->reuse = FALSE;
        out->chromaIDCFlag = CheckChromaFormatFlag(com, p_out_info);
    }

    if (out->reuse == FALSE) {
        osal_memcpy((void*)&out->decInfo, (void*)p_out_info, sizeof(DecOutputInfo));
    }

    if (ctx->frameNumToStop > 0) {
        if (ctx->frameNumToStop == ctx->numOutput) {
            ctx->force_stop = TRUE;
        }
    }
    if (p_out_info->indexFrameDisplay == DISPLAY_IDX_FLAG_SEQ_END) {
        ctx->force_stop = TRUE;
    }

    if (ctx->decOpenParam.bitstreamMode == BS_MODE_PIC_END && ctx->attr.supportCommandQueue == FALSE) {
        if(in && in->reuse == FALSE) {
            if (p_out_info->indexFrameDecoded == DECODED_IDX_FLAG_SKIP) {
                in->consumed = TRUE;
            }
        }
    }

    return TRUE;
}

static BOOL Decode(ComponentImpl* com, PortContainerES* in, PortContainerDisplay* out)
{
    DecoderContext*                 ctx           = (DecoderContext*)com->context;
    DecOutputInfo                   decOutputInfo;
    DEC_INT_STATUS                  intStatus;
    CNMComListenerStartDecOneFrame  lsnpPicStart;
    BitStreamMode                   bsMode        = ctx->decOpenParam.bitstreamMode;
    RetCode                         result;
    CNMComListenerDecDone           lsnpPicDone;
    CNMComListenerDecReadyOneFrame  lsnpReadyOneFrame = {0,};
    BOOL                            doDecode;
    BOOL                            re_dec_pic = FALSE;
    QueueStatusInfo                 qStatus;

    lsnpReadyOneFrame.handle = ctx->handle;
    
    ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_READY_ONE_FRAME, (void*)&lsnpReadyOneFrame);

    ctx->stateDoing = TRUE;

    if (PrepareSkip(com) == FALSE) {
        return FALSE;
    }

    if (ctx->force_stop == TRUE) {
        return TRUE;
    }

    if (com->pause == TRUE) {
        return TRUE;
    } else {
        doDecode = TRUE;
    }

    /* decode a frame except when the bitstream mode is PIC_END and no data */
    doDecode  = !(bsMode == BS_MODE_PIC_END && in == NULL);

#ifdef SUPPORT_DEC_RINGBUFFER_PERFORMANCE
    if (TRUE == com->pause && TRUE == ctx->testDecConfig.performance && BS_MODE_INTERRUPT == ctx->testDecConfig.bitstreamMode) {
        return TRUE;
    }
#endif

    if (ctx->attr.supportCommandQueue == TRUE) {
        VPU_DecGiveCommand(ctx->handle, DEC_GET_QUEUE_STATUS, &qStatus);
        if (qStatus.instanceQueueFull) {
            doDecode = FALSE;
        }
    }
    if (ctx->attr.supportCommandQueue == FALSE && doDecode == FALSE) {
        VLOG(WARN, "%s:%d Chceck doDecode : %d \n", __FUNCTION__, __LINE__, doDecode);
        return FALSE;
    }

    if (in) {
        if (in->last == TRUE && bsMode == BS_MODE_PIC_END) {
            in->reuse = TRUE;
            VLOG(INFO, "%s instance[%d] last pic_end data size : %d \n", __FUNCTION__, VPU_HANDLE_INSTANCE_NO(ctx->handle), in->size);
        }
    }

    if (TRUE == doDecode) {


        if (ctx->testDecConfig.enableUserData) {
            vpu_buffer_t* p_userData = Queue_Dequeue(ctx->userDataQ);
            if (p_userData) {
                VPU_DecGiveCommand(ctx->handle, SET_ADDR_REP_USERDATA, (void*)&(p_userData->phys_addr));
                VPU_DecGiveCommand(ctx->handle, SET_SIZE_REP_USERDATA, (void*)&(p_userData->size));
                VLOG(TRACE, "SET_ADDR_REP_USERDATA : 0x%x \n", p_userData->phys_addr);
            } else {
                VLOG(WARN, "Empty User Data Queue \n");
            }
        }

        result = VPU_DecStartOneFrame(ctx->handle, &ctx->decParam);

        lsnpPicStart.result   = result;
        lsnpPicStart.decParam = ctx->decParam;
        ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_START_ONE_FRAME, (void*)&lsnpPicStart);

        if (result == RETCODE_SUCCESS) {
        }
        else if (result == RETCODE_QUEUEING_FAILURE) {
            // Just retry
            if (in) in->reuse = (bsMode == BS_MODE_PIC_END);
        }
        else if (result == RETCODE_VPU_RESPONSE_TIMEOUT) {
            VLOG(ERR, "<%s:%d> Failed to VPU_DecStartOneFrame() ret(%d)\n", __FUNCTION__, __LINE__, result);
            CNMErrorSet(CNM_ERROR_HANGUP);
            HandleDecoderError(ctx->handle, ctx->numDecoded, NULL);
            return FALSE;
        }
        else {
            ChekcAndPrintDebugInfo(ctx->handle, FALSE, result);
            return FALSE;
        }
    }
    else {
        if (in) in->reuse = (bsMode == BS_MODE_PIC_END);
    }

    intStatus=HandlingInterruptFlag(com);


    if (intStatus == DEC_INT_STATUS_TIMEOUT) {
        ChekcAndPrintDebugInfo(ctx->handle, FALSE, RETCODE_VPU_RESPONSE_TIMEOUT);
        DoReset(com);
        return (TRUE == ctx->testDecConfig.ignoreHangup) ? TRUE : FALSE;
    }

    if (HandleInterruptReason(ctx, intStatus, &re_dec_pic) != TRUE) {
        return FALSE;
    } else {
        if (re_dec_pic == TRUE) {
            return TRUE;
        }
    }

    // Get data from the sink component.
    if ((result=VPU_DecGetOutputInfo(ctx->handle, &decOutputInfo)) == RETCODE_SUCCESS) {
        DisplayDecodedInformation(ctx->handle, ctx->decOpenParam.bitstreamFormat, ctx->numDecoded, &decOutputInfo, ctx->testDecConfig.performance, ctx->bsSize);
    }

    lsnpPicDone.handle      = ctx->handle;
    lsnpPicDone.ret         = result;
    lsnpPicDone.decParam    = &ctx->decParam;
    lsnpPicDone.output      = &decOutputInfo;
    lsnpPicDone.numDecoded  = ctx->numDecoded;
    lsnpPicDone.numDisplay  = ctx->numOutput;
    lsnpPicDone.vbUser.phys_addr = decOutputInfo.decOutputExtData.userDataBufAddr;
    lsnpPicDone.vbUser.size = decOutputInfo.decOutputExtData.userDataSize;
    lsnpPicDone.bitstreamFormat = ctx->decOpenParam.bitstreamFormat;
    if (ctx->testDecConfig.enableUserData) {
        vpu_buffer_t user_data;
        user_data.phys_addr = decOutputInfo.decOutputExtData.userDataBufAddr;
        user_data.size = USERDATA_BUFFER_SIZE;
        Queue_Enqueue(ctx->userDataQ, &user_data);
    }
    if (ctx->frameNumToStop > 0) {
        if (ctx->frameNumToStop > ctx->numOutput) {
            ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_GET_OUTPUT_INFO, (void*)&lsnpPicDone);
        }
    } else {
        ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_GET_OUTPUT_INFO, (void*)&lsnpPicDone);
    }

    if (result == RETCODE_REPORT_NOT_READY) {
        return TRUE; // Not decoded yet. Try again
    }
    else if (result != RETCODE_SUCCESS) {
        /* ERROR */
        VLOG(ERR, "Failed to decode error\n");
        ChekcAndPrintDebugInfo(ctx->handle, FALSE, result);
        return FALSE;
    }

    if (TRUE != HandleDecodedInfo(com, &decOutputInfo, in, out)) {
        return FALSE;
    }


    return TRUE;
}

static CNMComponentParamRet GetParameterDecoder(ComponentImpl* from, ComponentImpl* com, GetParameterCMD commandType, void* data)
{
    DecoderContext*             ctx     = (DecoderContext*)com->context;
    BOOL                        result  = TRUE;
    CNMComponentParamRet        reason  = CNM_COMPONENT_PARAM_FAILURE;
    PhysicalAddress             rdPtr, wrPtr;
    Uint32                      room;
    ParamDecBitstreamBufPos*    bsPos   = NULL;
    ParamDecNeedFrameBufferNum* fbNum;
    ParamVpuStatus*             status;
    QueueStatusInfo             cqInfo;
    PortContainerES*            container;
    vpu_buffer_t                vb;

    LockCompSync(&ctx->compSync);

    if (ctx->handle == NULL || ctx->doingReset == TRUE)  {
        UnLockCompSync(&ctx->compSync);
        return CNM_COMPONENT_PARAM_NOT_READY;
    }

    if (ctx->state == DEC_STATE_RELEASE || ctx->state == DEC_STATE_DESTROY) {
        UnLockCompSync(&ctx->compSync);
        return CNM_COMPONENT_PARAM_TERMINATED;
    }

    switch(commandType) {
    case GET_PARAM_COM_IS_CONTAINER_CONUSUMED:
        // This query command is sent from the comonponent core.
        // If input data are consumed in sequence, it should return TRUE through PortContainer::consumed.
        container = (PortContainerES*)data;
        vb = container->buf;
        if (vb.phys_addr <= ctx->decodedAddr && ctx->decodedAddr < (vb.phys_addr+vb.size)) {
            container->consumed = TRUE;
            ctx->decodedAddr = 0;
        }
        break;
    case GET_PARAM_DEC_HANDLE:
        *(DecHandle*)data = ctx->handle;
        break;
    case GET_PARAM_DEC_FRAME_BUF_NUM:
        if (ctx->state <= DEC_STATE_INIT_SEQ) {
            reason = CNM_COMPONENT_PARAM_NOT_READY;
            result = FALSE;
        } else {
            fbNum = (ParamDecNeedFrameBufferNum*)data;
            fbNum->nonLinearNum = ctx->initialInfo.minFrameBufferCount + EXTRA_FRAME_BUFFER_NUM;   // max_dec_pic_buffering
            if (ctx->decOpenParam.wtlEnable == TRUE) {
                fbNum->linearNum = (ctx->initialInfo.frameBufDelay+1) + EXTRA_FRAME_BUFFER_NUM;     // The frameBufDelay can be zero.
                if ((ctx->decOpenParam.bitstreamFormat == STD_VP9) || (ctx->decOpenParam.bitstreamFormat == STD_AVS2) || (ctx->decOpenParam.bitstreamFormat == STD_AV1)) {
                    fbNum->linearNum = fbNum->nonLinearNum;
                }
                if (ctx->testDecConfig.performance == TRUE) {
                    if ((ctx->decOpenParam.bitstreamFormat == STD_VP9) || (ctx->decOpenParam.bitstreamFormat == STD_AVS2)) {
                        fbNum->linearNum++;
                        fbNum->nonLinearNum++;
                    } else if (ctx->decOpenParam.bitstreamFormat == STD_AV1) {
                        fbNum->linearNum++;
                        fbNum->nonLinearNum++;
                    }
                    else {
                        fbNum->linearNum += 3;
                    }
                }
            }
            else {
                fbNum->linearNum = 0;
            }
            if (TRUE == ctx->testDecConfig.thumbnailMode) {
                //fbNum->linearNum    = (TRUE == ctx->decOpenParam.wtlEnable) ? 1 : 0;
                fbNum->nonLinearNum = 1;
            }
        }
        break;
    case GET_PARAM_DEC_BITSTREAM_BUF_POS:
        if (ctx->state < DEC_STATE_INIT_SEQ) {
            reason = CNM_COMPONENT_PARAM_NOT_READY;
            result = FALSE;
        } else {
            VPU_DecGetBitstreamBuffer(ctx->handle, &rdPtr, &wrPtr, &room);
            bsPos = (ParamDecBitstreamBufPos*)data;
            bsPos->rdPtr = rdPtr;
            bsPos->wrPtr = wrPtr;
            bsPos->avail = room;
        }
        break;
    case GET_PARAM_DEC_CODEC_INFO:
        if (ctx->state <= DEC_STATE_INIT_SEQ) {
            reason = CNM_COMPONENT_PARAM_NOT_READY;
            result = FALSE;
        } else {
            VPU_DecGiveCommand(ctx->handle, DEC_GET_SEQ_INFO, data);
        }
        break;
    case GET_PARAM_VPU_STATUS:
        if (ctx->state != DEC_STATE_DECODING) {
            reason = CNM_COMPONENT_PARAM_NOT_READY;
            result = FALSE;
        } else {
            VPU_DecGiveCommand(ctx->handle, DEC_GET_QUEUE_STATUS, &cqInfo);
            status = (ParamVpuStatus*)data;
            status->cq = cqInfo;
        }
        break;
    default:
        result = FALSE;
        break;
    }

    UnLockCompSync(&ctx->compSync);

    return (result == TRUE) ? CNM_COMPONENT_PARAM_SUCCESS : reason;
}

static CNMComponentParamRet SetParameterDecoder(ComponentImpl* from, ComponentImpl* com, SetParameterCMD commandType, void* data)
{
    BOOL                result  = TRUE;
    DecoderContext*     ctx     = (DecoderContext*)com->context;
    Int32               skipCmd;
    ParamDecTargetTid*  tid     = NULL;
    ParamDecTargetSid*  spatial_info = NULL;

    LockCompSync(&ctx->compSync);

    if (ctx->handle == NULL /*|| ctx->doingReset == TRUE*/)  {
        UnLockCompSync(&ctx->compSync);
        return CNM_COMPONENT_PARAM_NOT_READY;
    }

    if (ctx->state == DEC_STATE_RELEASE || ctx->state == DEC_STATE_DESTROY) {
        UnLockCompSync(&ctx->compSync);
        return CNM_COMPONENT_PARAM_TERMINATED;
    }

    switch(commandType) {
    case SET_PARAM_COM_PAUSE:
        com->pause   = *(BOOL*)data;
        break;
    case SET_PARAM_COM_STOP:
        ctx->state = DEC_STATE_RELEASE;
        break;
    case SET_PARAM_DEC_SKIP_COMMAND:
        skipCmd = *(Int32*)data;
        ctx->decParam.skipframeMode = skipCmd;
        if (skipCmd == WAVE_SKIPMODE_NON_IRAP) {
            Uint32 userDataMask = (1<<H265_USERDATA_FLAG_RECOVERY_POINT);
            ctx->decParam.craAsBlaFlag = TRUE;
            /* For finding recovery point */
            VPU_DecGiveCommand(ctx->handle, ENABLE_REP_USERDATA, &userDataMask);
        }
        else {
            ctx->decParam.craAsBlaFlag = FALSE;
        }
        if (ctx->numDecoded > 0) {
            ctx->doFlush = (BOOL)(ctx->decParam.skipframeMode == WAVE_SKIPMODE_NON_IRAP);
        }
        break;
    case SET_PARAM_DEC_RESET:
        ctx->doingReset  = TRUE;
        break;
    case SET_PARAM_DEC_TARGET_TID:
        tid = (ParamDecTargetTid*)data;
        /*! NOTE: DO NOT CHANGE THE ORDER OF COMMANDS BELOW. */
        VPU_DecGiveCommand(ctx->handle, DEC_SET_TEMPORAL_ID_MODE,   (void*)&tid->tidMode);
        VPU_DecGiveCommand(ctx->handle, DEC_SET_TARGET_TEMPORAL_ID, (void*)&tid->targetTid);
        break;
    case SET_PARAM_DEC_TARGET_SPATIAL_ID:
        //av1 only
        spatial_info = (ParamDecTargetSid*)data;
        VPU_DecGiveCommand(ctx->handle, DEC_SET_TARGET_SPATIAL_ID,   (void*)&spatial_info->targetSid);
        break;
    case SET_PARAM_DEC_FLUSH:
        ClearDpb(com, TRUE);
        break;
    case SET_PARAM_DEC_BS_SIZE:
        ctx->bsSize = *(size_t*)data;
        break;
    case SET_PARAM_DEC_CLR_DISP:
        if (ctx->state == DEC_STATE_DECODING) {
            if (ctx->sequenceNo == ((ParamDecClrDispInfo*)data)->sequenceNo) {
                VPU_DecClrDispFlag(ctx->handle, ((ParamDecClrDispInfo*)data)->indexFrameDisplay);
            }
        }
        break;
    default:
        result = FALSE;
        break;
    }

    UnLockCompSync(&ctx->compSync);

    return (result == TRUE) ? CNM_COMPONENT_PARAM_SUCCESS : CNM_COMPONENT_PARAM_FAILURE;
}

static BOOL UpdateBitstream(ComponentImpl* com, DecoderContext* ctx, PortContainerES* in)
{
    RetCode       ret    = RETCODE_SUCCESS;
    BitStreamMode bsMode = ctx->decOpenParam.bitstreamMode;
    BOOL          update = TRUE;
    Uint32        updateSize;

    if (in == NULL) return TRUE;


    if (bsMode == BS_MODE_PIC_END) {
        VPU_DecSetRdPtr(ctx->handle, in->buf.phys_addr, TRUE);
    }
    else {
        if (in->size > 0) {
            PhysicalAddress rdPtr, wrPtr;
            Uint32          room;
            VPU_DecGetBitstreamBuffer(ctx->handle, &rdPtr, &wrPtr, &room);
            if ((Int32)room < in->size) {
                in->reuse = TRUE;
                return TRUE;
            }
        }
    }

    if (in->last == TRUE) {
        updateSize = (in->size == 0) ? STREAM_END_SET_FLAG : in->size;
    }
    else {
        updateSize = in->size;
        update     = in->size > 0;
    }


    if (update == TRUE) {
        if ((ret=VPU_DecUpdateBitstreamBuffer(ctx->handle, updateSize)) != RETCODE_SUCCESS) {
            VLOG(ERR, "<%s:%d> Failed to VPU_DecUpdateBitstreamBuffer() ret(%d)\n", __FUNCTION__, __LINE__, ret);
            ChekcAndPrintDebugInfo(ctx->handle, FALSE, ret);
            return FALSE;
        }
        if (in->last == TRUE && updateSize != STREAM_END_SET_FLAG) {
            VPU_DecUpdateBitstreamBuffer(ctx->handle, STREAM_END_SET_FLAG);
        }
    }

    in->reuse = FALSE;

    return TRUE;
}

static BOOL OpenDecoder(ComponentImpl* com)
{
    DecoderContext*         ctx     = (DecoderContext*)com->context;
    ParamDecBitstreamBuffer bsBuf;
    CNMComponentParamRet    ret;
    CNMComListenerDecOpen   lspn    = {0};
    BOOL                    success = FALSE;
    BitStreamMode           bsMode  = ctx->decOpenParam.bitstreamMode;
    vpu_buffer_t*           vbUserData = ctx->vbUserData;
    RetCode                 retCode;
    Int32                   idx = 0;

    ctx->stateDoing = TRUE;
    ret = ComponentGetParameter(com, com->srcPort.connectedComponent, GET_PARAM_FEEDER_BITSTREAM_BUF, &bsBuf);
    if (ComponentParamReturnTest(ret, &success) == FALSE) {
        return success;
    }

    ctx->decOpenParam.bitstreamBuffer     = (bsMode == BS_MODE_PIC_END) ? 0 : bsBuf.bs->phys_addr;
    ctx->decOpenParam.bitstreamBufferSize = (bsMode == BS_MODE_PIC_END) ? 0 : bsBuf.bs->size;

    if (PRODUCT_ID_W6_SERIES(ctx->testDecConfig.productId)) {
        if (AllocateworkBuffer(com) != TRUE) {
            VLOG(ERR, "<%s:%d> Failed to AllocateworkBuffer()\n", __FUNCTION__, __LINE__);
            return FALSE;
        }
        if (AllocateTempBuffer(com) != TRUE) {
            VLOG(ERR, "<%s:%d> Failed to AllocateTempBuffer()\n", __FUNCTION__, __LINE__);
            return FALSE;
        }
        if (AllocateSecAxiBuffer(com) != TRUE) {
            VLOG(ERR, "<%s:%d> Failed to AllocateSecAxiBuffer()\n", __FUNCTION__, __LINE__);
            return FALSE;
        }
    }

    retCode = VPU_DecOpen(&ctx->handle, &ctx->decOpenParam);

    lspn.handle = ctx->handle;
    lspn.ret    = retCode;

    if (retCode != RETCODE_SUCCESS) {
        VLOG(ERR, "<%s:%d> Failed to VPU_DecOpen(ret:%d)\n", __FUNCTION__, __LINE__, retCode);
        if ( retCode == RETCODE_VPU_RESPONSE_TIMEOUT)
            CNMErrorSet(CNM_ERROR_HANGUP);

        return FALSE;
    }

    if (SetInterruptFlag(ctx) == FALSE) {
        return FALSE;
    }

    ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_OPEN, (void*)&lspn);

#ifdef SUPPORT_MULTI_INSTANCE_TEST
    VLOG(TRACE, "dec instIdx = %d, BS=%s\n", ctx->handle->instIndex, ctx->testDecConfig.inputPath);
#endif

    // VPU_DecGiveCommand(ctx->handle, ENABLE_LOGGING, 0);

    if (ctx->testDecConfig.enableUserData) {
        ctx->userDataQ = Queue_Create(COMMAND_QUEUE_DEPTH, sizeof(vpu_buffer_t));
        for (idx = 0; idx < COMMAND_QUEUE_DEPTH; idx++) {
            if (ctx->vbUserData[idx].size == 0) {
                vbUserData[idx].size = USERDATA_BUFFER_SIZE;  /* 40KB * (queue_depth + report_queue_depth+1) = 40KB * (16 + 16 +1) */
                vdi_allocate_dma_memory(ctx->testDecConfig.coreIdx, &(vbUserData[idx]), DEC_ETC, ctx->handle->instIndex);
            }
            Queue_Enqueue(ctx->userDataQ, &(vbUserData[idx]));
        }
        VPU_DecGiveCommand(ctx->handle, SET_ADDR_REP_USERDATA, (void*)&vbUserData[0].phys_addr);
        VPU_DecGiveCommand(ctx->handle, SET_SIZE_REP_USERDATA, (void*)&vbUserData[0].size);
        VPU_DecGiveCommand(ctx->handle, ENABLE_REP_USERDATA,   (void*)&ctx->testDecConfig.enableUserData);
    }

    VPU_DecGiveCommand(ctx->handle, SET_CYCLE_PER_TICK,   (void*)&ctx->cyclePerTick);
    if (ctx->testDecConfig.thumbnailMode == TRUE) {
        VPU_DecGiveCommand(ctx->handle, ENABLE_DEC_THUMBNAIL_MODE, NULL);
    }

    if (ctx->testDecConfig.disable_reorder == TRUE) {
        VPU_DecGiveCommand(ctx->handle, DEC_DISABLE_REORDER, NULL);
    }

    ctx->stateDoing = FALSE;



    return TRUE;
}

static BOOL DecodeHeader(ComponentImpl* com, PortContainerES* in, BOOL* done)
{
    DecoderContext*                ctx     = (DecoderContext*)com->context;
    DecHandle                      handle  = ctx->handle;
    Uint32                         coreIdx = ctx->testDecConfig.coreIdx;
    RetCode                        ret     = RETCODE_SUCCESS;
    DEC_INT_STATUS                 status  = DEC_INT_STATUS_ERR;
    DecInitialInfo*                initialInfo = &ctx->initialInfo;
    SecAxiUse                      secAxiUse;
    CNMComListenerDecCompleteSeq   lsnpCompleteSeq;
    BOOL                           re_seq_init = FALSE;

    *done = FALSE;

    if (BS_MODE_PIC_END == ctx->decOpenParam.bitstreamMode && NULL == in) {
        return TRUE;
    }

    if (ctx->stateDoing == FALSE) {
        /* previous state done */
        ret = VPU_DecIssueSeqInit(handle);
        if (RETCODE_QUEUEING_FAILURE == ret) {
            return TRUE; // Try again
        }
        ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_ISSUE_SEQ, NULL);
        if (ret != RETCODE_SUCCESS) {
            ChekcAndPrintDebugInfo(ctx->handle, FALSE, ret);
            VLOG(ERR, "%s:%d Failed to VPU_DecIssueSeqInit() ret(%d)\n", __FUNCTION__, __LINE__, ret);
            CNMErrorSet(CNM_ERROR_SEQ_INIT);
            HandleDecoderError(ctx->handle, 0, NULL);
            return FALSE;
        }
    }

    ctx->stateDoing = TRUE;

    while (com->terminate == FALSE) {
        if ((status=HandlingInterruptFlag(com)) == DEC_INT_STATUS_DONE) {
            break;
        }
        else if (status == DEC_INT_STATUS_TIMEOUT) {
            CNMErrorSet(CNM_ERROR_HANGUP);
            HandleDecoderError(ctx->handle, 0, NULL);
            VPU_DecUpdateBitstreamBuffer(handle, STREAM_END_SIZE);    /* To finish bitstream empty status */
            VPU_SWReset(coreIdx, SW_RESET_SAFETY, handle);
            VPU_DecUpdateBitstreamBuffer(handle, STREAM_END_CLEAR_FLAG);    /* To finish bitstream empty status */
            return FALSE;
        } else {
            if (HandleInterruptReason(ctx, status, &re_seq_init) == TRUE) {
                if (re_seq_init == TRUE) {
                    return TRUE;
                }
            }
        }
    }



    ret = VPU_DecCompleteSeqInit(handle, initialInfo);
    if ((ctx->attr.productId == PRODUCT_ID_521) && ctx->attr.supportDualCore == TRUE) {
        if ( initialInfo->lumaBitdepth == 8 && initialInfo->chromaBitdepth == 8) {
            ctx->testDecConfig.mapType = COMPRESSED_FRAME_MAP_DUAL_CORE_8BIT;
        }
        else {
            ctx->testDecConfig.mapType = COMPRESSED_FRAME_MAP_DUAL_CORE_10BIT;
        }
    }


    strcpy(lsnpCompleteSeq.refYuvPath, ctx->testDecConfig.refYuvPath);
    lsnpCompleteSeq.ret             = ret;
    lsnpCompleteSeq.initialInfo     = initialInfo;
    lsnpCompleteSeq.wtlFormat       = ctx->wtlFormat;
    lsnpCompleteSeq.cbcrInterleave  = ctx->decOpenParam.cbcrInterleave;
    lsnpCompleteSeq.bitstreamFormat = ctx->decOpenParam.bitstreamFormat;
    ctx->chromaIDCFlag              = initialInfo->chromaFormatIDC;
    ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_COMPLETE_SEQ, (void*)&lsnpCompleteSeq);

    VLOG(INFO, "Dec InitialInfo -> pic size %dx%d \n", initialInfo->picWidth, initialInfo->picHeight);
    VLOG(INFO, "instance idx: %d, open instance num: %d, minframeBuffercount: %u\n", ctx->handle->instIndex, VPU_GetOpenInstanceNum(ctx->handle->coreIdx), initialInfo->minFrameBufferCount);
    if (initialInfo->fRateDenominator) {
        VLOG(INFO, "frameRate: %.2f\n", ((double)initialInfo->fRateNumerator / (initialInfo->fRateDenominator)));
        VLOG(INFO, "frRes: %u, frDiv: %u\n", initialInfo->fRateNumerator, initialInfo->fRateDenominator);
    }

    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "%s:%d FAILED TO DEC_PIC_HDR: ret(%d), SEQERR(%08x)\n", __FUNCTION__, __LINE__, ret, initialInfo->seqInitErrReason);
        ChekcAndPrintDebugInfo(ctx->handle, FALSE, ret);
        CNMErrorSet(CNM_ERROR_GET_SEQ_INFO);
        return FALSE;
    }

    if (TRUE == ctx->decOpenParam.wtlEnable) {
        if ((ret = VPU_DecGiveCommand(ctx->handle, DEC_SET_WTL_FRAME_FORMAT, &ctx->wtlFormat)) != RETCODE_SUCCESS) {
            VLOG(ERR, "%s:%d FAILED TO VPU_DecGiveCommand ret(%d) \n", __FUNCTION__, __LINE__, ret);
            return FALSE;
        }
    }

   /* Set up the secondary AXI is depending on H/W configuration.
    * Note that turn off all the secondary AXI configuration
    * if target ASIC has no memory only for IP, LF and BIT.
    */
    secAxiUse.u.wave.useIpEnable    = (ctx->testDecConfig.secondaryAXI&0x01) ? TRUE : FALSE;
    secAxiUse.u.wave.useLfRowEnable = (ctx->testDecConfig.secondaryAXI&0x02) ? TRUE : FALSE;
    secAxiUse.u.wave.useBitEnable   = (ctx->testDecConfig.secondaryAXI&0x04) ? TRUE : FALSE;
    secAxiUse.u.wave.useSclEnable   = (ctx->testDecConfig.secondaryAXI&0x08) ? TRUE : FALSE;
    VPU_DecGiveCommand(ctx->handle, SET_SEC_AXI, &secAxiUse);
    // Set up scale
    if (ctx->testDecConfig.scaleDownWidth > 0 || ctx->testDecConfig.scaleDownHeight > 0) {
        ScalerInfo sclInfo = {0};

        sclInfo.scaleWidth  = ctx->testDecConfig.scaleDownWidth;
        sclInfo.scaleHeight = ctx->testDecConfig.scaleDownHeight;
        sclInfo.scaleMode   = ctx->testDecConfig.sclCoefMode;
        VLOG(INFO, "[SCALE INFO] %dx%d\n", sclInfo.scaleWidth, sclInfo.scaleHeight);
        sclInfo.enScaler    = TRUE;
        if (VPU_DecGiveCommand(ctx->handle, DEC_SET_SCALER_INFO, (void*)&sclInfo) != RETCODE_SUCCESS) {
            VLOG(ERR, "Failed to VPU_DecGiveCommand(DEC_SET_SCALER_INFO)\n");
            return FALSE;
        }
    }

    ctx->stateDoing = FALSE;
    *done = TRUE;

    if (ctx->testDecConfig.bitstreamMode == BS_MODE_PIC_END) {
        if (ctx->testDecConfig.feedingMode != FEEDING_METHOD_HOST_FRAME_SIZE) {
            if (ctx->testDecConfig.bitFormat == STD_AVC || ctx->testDecConfig.bitFormat == STD_HEVC) {
                if (in) {
                    in->reuse = FALSE;
                    in->consumed = TRUE;
                }
            }
        }
    }



    return TRUE;
}

static BOOL ReleaseDecoder(ComponentImpl* com)
{
    DecoderContext* ctx     = (DecoderContext*)com->context;
    BOOL pause              = TRUE;
    Int32 explicit_pause    = FALSE;
    com->stateDoing         = FALSE;
    ctx->iterationCnt       = 0;

    ComponentSetParameter(com, com->srcPort.connectedComponent, SET_PARAM_COM_PAUSE, (void*)&pause);
    ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_COM_PAUSE, (void*)&pause);

    ComponentGetParameter(com, com->srcPort.connectedComponent, GET_PARAM_EXPLICIT_PAUSE, (void*)&explicit_pause);

    if (explicit_pause == FALSE ) {
        VLOG(INFO, "Waiting for component... explicit_pause : %d \n", explicit_pause);
        osal_msleep(10);
        return TRUE;
    }

    if (CheckCompSync(ctx)) {
        VLOG(INFO, "%s waiting for ins_num : %d \n", __FUNCTION__, VPU_HANDLE_INSTANCE_NO(ctx->handle));
        osal_msleep(10);
        return TRUE;
    }

    ctx->state = DEC_STATE_DESTROY;
    com->pause = TRUE;

    return TRUE;
}

static BOOL DestroyDecoder(ComponentImpl* com)
{
    DecoderContext* ctx         = (DecoderContext*)com->context;
    DEC_INT_STATUS  intStatus;
    BOOL            success     = FALSE;
    Uint32          i           = 0;
    Uint64          currentTime = 0;
    Uint32          timeout = DEC_DESTROY_TIME_OUT;
    RetCode         ret;
    BOOL            stop = TRUE;

    if (NULL != ctx->handle) {
        if (FALSE == com->stateDoing) {
            ctx->desStTimeout = osal_gettime();
            if (RETCODE_SUCCESS != (ret=VPU_DecUpdateBitstreamBuffer(ctx->handle, STREAM_END_SET_FLAG))) {
                VLOG(WARN, "%s:%d Failed to VPU_DecUpdateBitstreamBuffer, ret(%08x) \n",__FUNCTION__, __LINE__, ret);
            }
            com->stateDoing = TRUE;
        }

        while ((ret = VPU_DecClose(ctx->handle)) == RETCODE_VPU_STILL_RUNNING) {
            if ((intStatus=HandlingInterruptFlag(com)) == DEC_INT_STATUS_TIMEOUT) {
                CNMErrorSet(CNM_ERROR_HANGUP);
                HandleDecoderError(ctx->handle, ctx->numDecoded, NULL);
                VLOG(ERR, "<%s:%d> NO RESPONSE FROM VPU_DecClose()\n", __FUNCTION__, __LINE__);
                break;
            }
            else if (intStatus == DEC_INT_STATUS_DONE) {
                DecOutputInfo outputInfo;
                VLOG(INFO, "VPU_DecClose() : CLEAR REMAIN INTERRUPT\n");
                VPU_DecGetOutputInfo(ctx->handle, &outputInfo);
                continue;
            }

            for (i = 0; i < MAX_FRAME_NUM; i++) {
                VPU_DecClrDispFlag(ctx->handle, i);
            }

            currentTime = osal_gettime();
            if ( (currentTime - ctx->desStTimeout) > timeout) {
                ChekcAndPrintDebugInfo(ctx->handle, FALSE, RETCODE_VPU_RESPONSE_TIMEOUT);
                VLOG(ERR, "INSTANCE #%d VPU Close TIMEOUT.\n", ctx->handle->instIndex);
                break;
            }
            osal_msleep(100);
        }
        if (ret == RETCODE_SUCCESS) {
            success = TRUE;
        } else {
            VLOG(ERR, "%s:%d Failed to VPU_DecClose(), ret(%08x) \n",__FUNCTION__, __LINE__, ret);
        }
        ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_CLOSE, NULL);
    }

    ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_RENDERER_RELEASE_FRAME_BUFFRES, NULL);
    ComponentSetParameter(com, com->srcPort.connectedComponent, SET_PARAM_FEEDER_RELEASE_BS_BUFFRES, NULL);

    for (i=0; i < COMMAND_QUEUE_DEPTH; i++) {
        if (ctx->vbUserData[i].size) {
            vdi_free_dma_memory(ctx->testDecConfig.coreIdx, &(ctx->vbUserData[i]), DEC_ETC, VPU_HANDLE_INSTANCE_NO(ctx->handle));
        }
    }

    if (ctx->vbWork.size) {
        vdi_free_dma_memory(ctx->testDecConfig.coreIdx, &ctx->vbWork, DEC_WORK, ctx->handle->instIndex);
        ctx->vbWork.size = 0;
        ctx->vbWork.phys_addr = 0UL;
    }
    if (ctx->vbTemp.size) {
        vdi_free_dma_memory(ctx->testDecConfig.coreIdx, &ctx->vbTemp, DEC_TEMP, ctx->handle->instIndex);
        ctx->vbTemp.size = 0;
        ctx->vbTemp.phys_addr = 0UL;
    }
    if (ctx->vbAr.size) {
        vdi_free_dma_memory(ctx->testDecConfig.coreIdx, &ctx->vbAr, DEC_ETC, ctx->handle->instIndex);
        ctx->vbAr.size = 0;
        ctx->vbAr.phys_addr = 0UL;
    }

    // TODO, code refacotoring 
    for (i = 0; i < AUX_BUF_TYPE_MAX; i++) {
        Uint32 j;
        for (j = 0; j < ctx->nonLinearNum; j++) {
            if (ctx->vbAux[i][j].size) {
                vdi_free_dma_memory(ctx->testDecConfig.coreIdx, &ctx->vbAux[i][j], DEC_ETC, ctx->handle->instIndex);
                ctx->vbAux[i][j].size = 0;
                ctx->vbAux[i][j].phys_addr = 0UL;
            }
        }
    }

    VPU_DeInit(ctx->decOpenParam.coreIdx);

    ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_COM_STOP, (void*)&stop);

    osal_free(ctx);

    com->stateDoing = FALSE;
    com->terminate = TRUE;
    com->success = success;

    return success;
}

static BOOL ExecuteDecoder(ComponentImpl* com, PortContainer* in , PortContainer* out)
{
    DecoderContext* ctx    = (DecoderContext*)com->context;
    BOOL            ret    = FALSE;
    BitStreamMode   bsMode = ctx->decOpenParam.bitstreamMode;
    BOOL            done   = FALSE;

    if (in)  in->reuse = TRUE;
    if (out) out->reuse = TRUE;
    if (ctx->state == DEC_STATE_INIT_SEQ || ctx->state == DEC_STATE_DECODING) {
        if (UpdateBitstream(com, ctx, (PortContainerES*)in) == FALSE) {
            return FALSE;
        }
        if (in) {
            // In ring-buffer mode, it has to return back a container immediately.
            if (bsMode == BS_MODE_PIC_END) {
                if (ctx->state == DEC_STATE_INIT_SEQ) {
                    in->reuse = TRUE;
                }
                in->consumed = FALSE;
            }
            else {
                in->consumed = (in->reuse == FALSE);
            }
        }
    }

    if (ctx->doingReset  == TRUE) {
        if (in) in->reuse = FALSE;
        ctx->startTimeout = 0ULL;
        return DoReset(com);
    }
    switch (ctx->state) {
    case DEC_STATE_OPEN_DECODER:
        ret = OpenDecoder(com);
        if (ctx->stateDoing == FALSE) ctx->state = DEC_STATE_INIT_SEQ;
        break;
    case DEC_STATE_INIT_SEQ:
        ret = DecodeHeader(com, (PortContainerES*)in, &done);
        if (TRUE == done) ctx->state = DEC_STATE_REGISTER_FB;
        break;
    case DEC_STATE_REGISTER_FB:
        ret = RegisterFrameBuffers(com);
        if (ctx->stateDoing == FALSE) {
            ctx->state = DEC_STATE_DECODING;
            DisplayDecodedInformation(ctx->handle, ctx->decOpenParam.bitstreamFormat, 0, NULL, ctx->testDecConfig.performance, 0);
        }
        break;
    case DEC_STATE_DECODING:
        ret = Decode(com, (PortContainerES*)in, (PortContainerDisplay*)out);
        break;
    case DEC_STATE_RELEASE:
        ret = ReleaseDecoder(com);
        break;
    case DEC_STATE_DESTROY:
        ret = DestroyDecoder(com);
        break;
    default:
        ret = FALSE;
    }

    if (ret == FALSE || com->terminate == TRUE) {
        ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_DECODED_ALL, (void*)ctx->handle);
        if (out) {
            out->reuse = FALSE;
            out->last  = TRUE;
        }
        if (ctx->state == DEC_STATE_DECODING) {
            ReleaseDecoder(com);
            DestroyDecoder(com);
        }
    }

    return ret;
}

static BOOL PrepareDecoder(ComponentImpl* com, BOOL* done)
{
    *done = TRUE;

    return TRUE;
}

static Component CreateDecoder(ComponentImpl* com, CNMComponentConfig* componentParam)
{
    DecoderContext* ctx;
    Uint32          coreIdx      = componentParam->testDecConfig.coreIdx;
    Uint16*         firmware     = (Uint16*)componentParam->bitcode;
    Uint32          firmwareSize = componentParam->sizeOfBitcode;
    RetCode         retCode;

    retCode = VPU_InitWithBitcode(coreIdx, firmware, firmwareSize);
    if (retCode != RETCODE_SUCCESS && retCode != RETCODE_CALLED_BEFORE) {
        VLOG(ERR, "%s:%d Failed to VPU_InitiWithBitcode, ret(%08x)\n", __FUNCTION__, __LINE__, retCode);
        return FALSE;
    }

    com->context = (DecoderContext*)osal_malloc(sizeof(DecoderContext));
    osal_memset(com->context, 0, sizeof(DecoderContext));
    ctx = (DecoderContext*)com->context;

    retCode = PrintVpuProductInfo(coreIdx, &ctx->attr);
    if (retCode == RETCODE_VPU_RESPONSE_TIMEOUT ) {
        VLOG(ERR, "<%s:%d> Failed to PrintVpuProductInfo()\n", __FUNCTION__, __LINE__);
        CNMErrorSet(CNM_ERROR_FAILURE);
        HandleDecoderError(ctx->handle, ctx->numDecoded, NULL);
        return FALSE;
    }
    ctx->cyclePerTick = 32768;
    if (TRUE == ctx->attr.supportNewTimer)
        ctx->cyclePerTick = 256;

    osal_memcpy(&(ctx->decOpenParam), &(componentParam->decOpenParam), sizeof(DecOpenParam));

    ctx->wtlFormat                    = componentParam->testDecConfig.wtlFormat;
    ctx->frameNumToStop               = componentParam->testDecConfig.forceOutNum;
    ctx->testDecConfig                = componentParam->testDecConfig;
    ctx->state                        = DEC_STATE_OPEN_DECODER;
    ctx->stateDoing                   = FALSE;
    VLOG(INFO, "PRODUCT ID: %d\n", ctx->attr.productId);
    VLOG(INFO, "SUPPORT_COMMAND_QUEUE : %d, COMMAND_QUEUE_DEPTH=%d\n", ctx->attr.supportCommandQueue, COMMAND_QUEUE_DEPTH);

    return (Component)com;
}

static void ReleaseComponent(ComponentImpl* com)
{
}

static BOOL DestroyComponent(ComponentImpl* com)
{
    return TRUE;
}

ComponentImpl waveDecoderComponentImpl = {
    "wave_decoder",
    NULL,
    {0,},
    {0,},
    sizeof(PortContainerDisplay),
    5,
    CreateDecoder,
    GetParameterDecoder,
    SetParameterDecoder,
    PrepareDecoder,
    ExecuteDecoder,
    ReleaseComponent,
    DestroyComponent
};

