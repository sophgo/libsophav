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

#define MAX_SEQ_CHANGE_QUEUE_NUM 32

typedef struct {
    DecHandle                   handle;                 /*!<< A decoder handle */
    TestDecConfig               testDecConfig;
    Uint32                      framebufStride;
    Uint32                      displayPeriodTime;
    FrameBuffer                 pFrame[MAX_FRAME_NUM];
    vpu_buffer_t                pFbMem[MAX_FRAME_NUM];
    BOOL                        enablePPU;
    FrameBuffer                 pPPUFrame[MAX_FRAME_NUM];
    vpu_buffer_t                pPPUFbMem[MAX_FRAME_NUM];
    Queue*                      ppuQ;
    BOOL                        fbAllocated;
    ParamDecNeedFrameBufferNum  fbCount;
    Queue*                      seqMemQ;
    Queue*                      dispFlagQ;
    osal_mutex_t                lock;
    char                        outputPath[256];
    FILE*                       fpOutput[OUTPUT_FP_NUMBER];
    Uint64                      displayed_num;
    Uint32                      frameNumToStop;
    Int32                       sequenceNo;

} RendererContext;

typedef struct SequenceMemInfo {
    Uint32              nonLinearNum;
    Uint32              linearNum;
    Uint32              remainingCount;
    Uint32              sequenceNo;
    vpu_buffer_t        pFbMem[MAX_FRAME_NUM];
    vpu_buffer_t        vbFbcYTbl[MAX_FRAME_NUM];
    vpu_buffer_t        vbFbcCTbl[MAX_FRAME_NUM];
    vpu_buffer_t        vbTask;
} SequenceMemInfo;

static void ReleaseFrameBuffers(ComponentImpl* com)
{
    RendererContext* ctx = (RendererContext*)com->context;
    Uint32           coreIdx = ctx->testDecConfig.coreIdx;
    Uint32           i;

    for (i = 0; i < MAX_FRAME_NUM; i++) {
        if (ctx->pFbMem[i].size) {
            if (i < ctx->fbCount.linearNum)
                vdi_free_dma_memory(coreIdx, &ctx->pFbMem[i], DEC_FBC, ctx->handle->instIndex);
            else
                vdi_free_dma_memory(coreIdx, &ctx->pFbMem[i], DEC_FB_LINEAR, ctx->handle->instIndex);
        }
    }

    for (i = 0; i < MAX_FRAME_NUM; i++) {
        if (ctx->pPPUFrame[i].size) vdi_free_dma_memory(coreIdx, &ctx->pPPUFbMem[i], DEC_ETC, ctx->handle->instIndex);
    }

}

static void FreeFrameBuffer(DecHandle handle, Uint32 idx, SequenceMemInfo* info)
{
    Int32 coreIdx = handle->coreIdx;
    Int32 ins_idx = VPU_HANDLE_INSTANCE_NO(handle);

    if (info->pFbMem[idx].size > 0) {
        if (idx < info->nonLinearNum)
            vdi_free_dma_memory(coreIdx, &info->pFbMem[idx], DEC_FBC, ins_idx);
        else
            vdi_free_dma_memory(coreIdx, &info->pFbMem[idx], DEC_FB_LINEAR, ins_idx);
        osal_memset((void*)&info->pFbMem[idx], 0x00, sizeof(vpu_buffer_t));
    }

    if (info->vbFbcYTbl[idx].size > 0) {
        vdi_free_dma_memory(coreIdx, &info->vbFbcYTbl[idx], DEC_FBCY_TBL, ins_idx);
        osal_memset((void*)&info->vbFbcYTbl[idx], 0x00, sizeof(vpu_buffer_t));
    }

    if (info->vbFbcCTbl[idx].size > 0) {
        vdi_free_dma_memory(coreIdx, &info->vbFbcCTbl[idx], DEC_FBCC_TBL, ins_idx);
        osal_memset((void*)&info->vbFbcCTbl[idx], 0x00, sizeof(vpu_buffer_t));
    }
}

static BOOL SetParamFreeFrameBuffers(ComponentImpl* com, Uint32 fbFlags)
{
    RendererContext*    ctx         = (RendererContext*)com->context;
    BOOL                remainingFbs[MAX_FRAME_NUM] = { 0 };
    Uint32              idx;
    Uint32              fbIndex;
    BOOL                wtlEnable   = ctx->testDecConfig.enableWTL;
    DecGetFramebufInfo  curFbInfo;
    Uint32              coreIdx     = ctx->testDecConfig.coreIdx;
    SequenceMemInfo     seqMem      = {0};
    Int32               ins_idx     = VPU_HANDLE_INSTANCE_NO(ctx->handle);

    osal_mutex_lock(ctx->lock);
    VPU_DecGiveCommand(ctx->handle, DEC_GET_FRAMEBUF_INFO, (void*)&curFbInfo);

    if (PRODUCT_ID_W_SERIES(ctx->testDecConfig.productId) && wtlEnable == TRUE) {
        for (idx=0; idx<MAX_GDI_IDX; idx++) {
            if ((fbFlags>>idx) & 0x01) {
                fbIndex = VPU_CONVERT_WTL_INDEX(ctx->handle, idx);
                seqMem.remainingCount++;
                remainingFbs[fbIndex] = TRUE;
            }
        }
    }

    seqMem.nonLinearNum = ctx->fbCount.nonLinearNum;
    seqMem.linearNum    = ctx->fbCount.linearNum;
    osal_memcpy((void*)seqMem.pFbMem,    ctx->pFbMem,         sizeof(ctx->pFbMem));
    osal_memcpy((void*)seqMem.vbFbcYTbl, curFbInfo.vbFbcYTbl, sizeof(curFbInfo.vbFbcYTbl));
    osal_memcpy((void*)seqMem.vbFbcCTbl, curFbInfo.vbFbcCTbl, sizeof(curFbInfo.vbFbcCTbl));
    osal_memcpy((void*)&seqMem.vbTask,  &curFbInfo.vbTask,    sizeof(curFbInfo.vbTask));

    for (idx = 0; idx < MAX_FRAME_NUM; idx++) {
        if (remainingFbs[idx] == FALSE) {
            FreeFrameBuffer(ctx->handle, idx, &seqMem);
        }
        // Free a mvcol buffer
        if(curFbInfo.vbMvCol[idx].size > 0) {
            vdi_free_dma_memory(coreIdx, &curFbInfo.vbMvCol[idx], DEC_MV, ins_idx);
        }
    }

    if (curFbInfo.vbTask.size > 0) {
        vdi_free_dma_memory(coreIdx, &curFbInfo.vbTask, DEC_TASK, ins_idx);
        osal_memset((void*)&curFbInfo.vbTask, 0x00, sizeof(vpu_buffer_t));
    }

    if (seqMem.remainingCount > 0) {
        Queue_Enqueue(ctx->seqMemQ, (void*)&seqMem);
        Queue_Enqueue(ctx->dispFlagQ, (void*)&fbFlags);
    }

    ctx->fbAllocated = FALSE;
    osal_mutex_unlock(ctx->lock);

    return TRUE;
}

static BOOL IntResChangedFreeFrameBuffers(ComponentImpl* com, ParamDecReallocFB* decParam)
{
    RendererContext*    ctx = (RendererContext*)com->context;
    Uint32              picWidth, picHeight;
    Int8                fbcIndex = 0;
    Int8                linearIndex = 0;
    Int8                mvColIndex;
    Uint32              coreIdx = ctx->testDecConfig.coreIdx;

    osal_mutex_lock(ctx->lock);

    fbcIndex    = (Int8)(decParam->indexInterFrameDecoded & 0xff);
    linearIndex = (Int8)((decParam->indexInterFrameDecoded >> 8) & 0xff);
    if (linearIndex >= 0)
        linearIndex = (Int8)VPU_CONVERT_WTL_INDEX(ctx->handle, linearIndex);
    mvColIndex  = (Int8)((decParam->indexInterFrameDecoded >> 16) & 0xff);

    picWidth  = decParam->width;
    picHeight = decParam->height;

    VLOG(INFO, "FBC IDX  : %d\n", fbcIndex);
    VLOG(INFO, "LIN IDX  : %d\n", linearIndex);
    VLOG(INFO, "COL IDX  : %d\n", mvColIndex);
    VLOG(INFO, "SIZE     : WIDTH(%d), HEIGHT(%d)\n", picWidth, picHeight);

    if (fbcIndex >= 0) {
        /* Release the FBC framebuffer */
        vdi_free_dma_memory(coreIdx, ctx->pFbMem+fbcIndex, DEC_FBC, ctx->handle->instIndex);
        osal_memset((void*)(ctx->pFbMem+fbcIndex), 0x00, sizeof(vpu_buffer_t));
    }

    if (linearIndex >= 0) {
        /* Release the linear framebuffer */
        vdi_free_dma_memory(coreIdx, ctx->pFbMem+linearIndex, DEC_FB_LINEAR, ctx->handle->instIndex);
        osal_memset((void*)(ctx->pFbMem+linearIndex), 0x00, sizeof(vpu_buffer_t));
    }

    ctx->fbAllocated = FALSE;
    osal_mutex_unlock(ctx->lock);

    return TRUE;
}

static BOOL ReallocateFrameBuffers(ComponentImpl* com, ParamDecReallocFB* param)
{
    RendererContext* ctx        = (RendererContext*)com->context;
    Int32            fbcIndex    = param->compressedIdx;
    Int32            linearIndex = param->linearIdx;
    vpu_buffer_t*    pFbMem      = ctx->pFbMem;
    FrameBuffer*     pFrame      = ctx->pFrame;
    FrameBuffer*     newFbs      = param->newFbs;

    if (fbcIndex >= 0) {
        /* Release the FBC framebuffer */
        vdi_free_dma_memory(ctx->testDecConfig.coreIdx, &pFbMem[fbcIndex], DEC_FBC, ctx->handle->instIndex);
        osal_memset((void*)&pFbMem[fbcIndex], 0x00, sizeof(vpu_buffer_t));
    }

    if (linearIndex >= 0) {
        /* Release the linear framebuffer */
        vdi_free_dma_memory(ctx->testDecConfig.coreIdx, &pFbMem[linearIndex], DEC_FB_LINEAR, ctx->handle->instIndex);
        osal_memset((void*)&pFbMem[linearIndex], 0x00, sizeof(vpu_buffer_t));
    }

    if (fbcIndex >= 0) {
        newFbs[0].myIndex = fbcIndex;
        newFbs[0].width   = param->width;
        newFbs[0].height  = param->height;
        pFrame[fbcIndex]  = newFbs[0];
    }

    if (linearIndex >= 0) {
        newFbs[1].myIndex = linearIndex;
        newFbs[1].width   = param->width;
        newFbs[1].height  = param->height;
        pFrame[linearIndex]  = newFbs[1];
    }

    return TRUE;
}

static void HandleSeqChangedBuffers(ComponentImpl* com)
{
    RendererContext*    ctx = (RendererContext*)com->context;
    SequenceMemInfo*    memInfo = NULL;
    Int32               idx = 0;
    Int32               wtl_idx = 0;
    Uint32*             p_disp_flag = NULL;

    memInfo = Queue_Peek(ctx->seqMemQ);
    p_disp_flag = Queue_Peek(ctx->dispFlagQ);

    if (memInfo != NULL && p_disp_flag != NULL) {
        for (idx = 0; idx < MAX_GDI_IDX; idx++) {
            if (((*p_disp_flag) >> idx) & 0x1) {
                wtl_idx = idx + memInfo->nonLinearNum;
                if (memInfo->remainingCount > 0 ) {
                    VLOG(WARN, "%s, fbc_num : %d, free disp buffer[%d] seq_num : %d \n", __FUNCTION__, memInfo->nonLinearNum, wtl_idx, ctx->sequenceNo);
                    FreeFrameBuffer(ctx->handle, wtl_idx, memInfo);
                }
            }
        }
        Queue_Dequeue(ctx->seqMemQ);
        Queue_Dequeue(ctx->dispFlagQ);
    }
}

static void DisplayFrame(ComponentImpl* com, DecOutputInfo* result)
{
    RendererContext*        ctx = (RendererContext*)com->context;
    ParamDecClrDispInfo     clr_disp_info;
    osal_mutex_lock(ctx->lock);
    if (result->indexFrameDisplay >= 0) {
        DecInitialInfo  initialInfo;
        Int32 productID = ctx->testDecConfig.productId;
        VPU_DecGiveCommand(ctx->handle, DEC_GET_SEQ_INFO, &initialInfo);
        if (PRODUCT_ID_W_SERIES(productID)) {
            if(ctx->sequenceNo < result->sequenceNo) {
                HandleSeqChangedBuffers(com);
                ctx->sequenceNo = result->sequenceNo;
            }
        }
        clr_disp_info.indexFrameDisplay = result->indexFrameDisplay;
        clr_disp_info.sequenceNo = ctx->sequenceNo;
        ComponentSetParameter(com, com->srcPort.connectedComponent, SET_PARAM_DEC_CLR_DISP, &clr_disp_info);
#ifdef SUPPORT_MULTI_INSTANCE_TEST
        {
            Uint32 curTime = osal_gettime()/1000;
            VLOG(INFO, "%d <%s> INSTANCE#%d CLEAR DISPLAY INDEX: %d\n", curTime, __FUNCTION__, ctx->handle->instIndex, result->indexFrameDisplay);
        }
#endif
        ctx->displayed_num++;
        if (ctx->frameNumToStop > 0) {
            if (ctx->frameNumToStop == ctx->displayed_num) {
                VLOG(WARN, "%s:%d force stop : %d\n",__FUNCTION__, __LINE__, ctx->displayed_num);
                ComponentSetParameter(com, com->srcPort.connectedComponent, SET_PARAM_COM_STOP, NULL);
                com->pause = TRUE;
            }
        }
    }
    osal_mutex_unlock(ctx->lock);
}

static BOOL FlushFrameBuffers(ComponentImpl* com, Uint32* flushedIndexes)
{
    PortContainerDisplay*   srcData;
    RendererContext*        ctx = (RendererContext*)com->context;
    Int32                   idx = 0;

    osal_mutex_lock(ctx->lock);
    if (flushedIndexes) *flushedIndexes = 0;
    while ((srcData=(PortContainerDisplay*)ComponentPortGetData(&com->srcPort)) != NULL) {
        idx = srcData->decInfo.indexFrameDisplay;
        if (0 <= idx) {
            VPU_DecClrDispFlag(ctx->handle, idx);
            if (flushedIndexes) *flushedIndexes |= (1<<idx);
        }
        ComponentPortSetData(&com->srcPort, (PortContainer*)srcData);
    }
    osal_mutex_unlock(ctx->lock);

    return TRUE;
}
static BOOL AllocateFrameBuffer(ComponentImpl* com)
{
    RendererContext*     ctx            = (RendererContext*)com->context;
    BOOL                 success;
    Uint32               compressedNum;
    Uint32               linearNum;
    CNMComponentParamRet ret;

    ret = ComponentGetParameter(com, com->srcPort.connectedComponent, GET_PARAM_DEC_FRAME_BUF_NUM, &ctx->fbCount);
    if (ComponentParamReturnTest(ret, &success) == FALSE) {
        return success;
    }

    osal_memset((void*)ctx->pFbMem, 0x00, sizeof(ctx->pFbMem));
    osal_memset((void*)ctx->pFrame, 0x00, sizeof(ctx->pFrame));

    compressedNum  = ctx->fbCount.nonLinearNum;
    linearNum      = ctx->fbCount.linearNum;

    if (compressedNum == 0 && linearNum == 0) {
        VLOG(ERR, "%s:%d The number of framebuffers are zero. compressed %d, linear: %d\n",
            __FUNCTION__, __LINE__, compressedNum, linearNum);
        return FALSE;
    }

    if (AllocateDecFrameBuffer(ctx->handle, &ctx->testDecConfig, compressedNum, linearNum, ctx->pFrame, ctx->pFbMem, &ctx->framebufStride) == FALSE) {
        VLOG(INFO, "%s:%d Failed to AllocateDecFrameBuffer()\n", __FUNCTION__, __LINE__);
        return FALSE;
    }
    ctx->fbAllocated = TRUE;

    return TRUE;
}

static BOOL AllocateIntResChangedFrameBuffer(ComponentImpl* com, ParamDecReallocFB* decParam)
{
    RendererContext*    ctx = (RendererContext*)com->context;
    Int8                fbcIndex = 0, linearIndex = 0, mvColIndex = 0;
    FrameBuffer*        pFbcFb = NULL;
    FrameBuffer*        pLinearFb = NULL;
    FrameBuffer         intResChangedFbs[2];
    vpu_buffer_t        intResChangedMem[2];
    Uint32              intResChangedStride = 0, picWidth = 0, picHeight = 0;
    Uint32              ret;

    fbcIndex    = (Int8)(decParam->indexInterFrameDecoded & 0xff);
    linearIndex = (Int8)((decParam->indexInterFrameDecoded >> 8) & 0xff);
    if (linearIndex >= 0)
        linearIndex = (Int8)VPU_CONVERT_WTL_INDEX(ctx->handle, linearIndex);
    mvColIndex  = (Int8)((decParam->indexInterFrameDecoded >> 16) & 0xff);

    picWidth  = decParam->width;
    picHeight = decParam->height;

    VLOG(INFO, "%s:%d Inter res channged framebuffer idx fbcIndex %d, linearIndex: %d\n", __FUNCTION__, __LINE__, fbcIndex, linearIndex);

    if (AllocateDecFrameBuffer(ctx->handle, &ctx->testDecConfig, (fbcIndex>=0?1:0), (linearIndex>=0?1:0), intResChangedFbs, intResChangedMem, &intResChangedStride) == FALSE) {
        VLOG(INFO, "%s:%d Failed to AllocateDecFrameBuffer()\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    if (fbcIndex >= 0) {
        pFbcFb = ctx->pFrame+fbcIndex;
        *pFbcFb = intResChangedFbs[0];
        pFbcFb->myIndex = fbcIndex;
        pFbcFb->width  = picWidth;
        pFbcFb->height = picHeight;
        ctx->pFbMem[fbcIndex] = intResChangedMem[0];
    }
    if (linearIndex >= 0) {
        pLinearFb = ctx->pFrame+linearIndex;
        *pLinearFb = intResChangedFbs[1];
        pLinearFb->myIndex = linearIndex;
        pLinearFb->width = picWidth;
        pLinearFb->height = picHeight;
        ctx->pFbMem[linearIndex] = intResChangedMem[1];
    }

    ret = VPU_DecUpdateFrameBuffer(ctx->handle, pFbcFb, pLinearFb, (Uint32)mvColIndex, picWidth, picHeight);
    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "%s:%d Failed to INST(%02d) VPU_DecUpdateFrameBuffer(err: %08x)\n", __FUNCTION__, __LINE__, ctx->handle->instIndex, ret);
        return FALSE;
    }

    ctx->fbAllocated = TRUE;

    return TRUE;
}

static RetCode UpdateChangedBuffer(ComponentImpl* com, ParamDecReallocFB* decParam)
{
    RendererContext*        ctx = (RendererContext*)com->context;
    Int32                   fbcIndex = 0, linearIndex = 0, mvColIndex = 0;
    FrameBuffer             intResChangedFbs[2];
    vpu_buffer_t            intResChangedMem[2];
    Uint32                  intResChangedStride = 0;
    RetCode                 ret_code = RETCODE_FAILURE;
    DecAuxBufferSizeInfo    sizeInfo = {0,};
    AuxBufferInfo           auxBufferInfo ={0,};
    AuxBuffer               bufArr ={0,};
    vpu_buffer_t            buf_info ={0,};
    Int32                   core_idx = -1;
    Int32                   inst_idx = -1;

    core_idx = VPU_HANDLE_CORE_INDEX(ctx->handle);
    inst_idx = VPU_HANDLE_INSTANCE_NO(ctx->handle);

    fbcIndex    = (Int8)(decParam->indexInterFrameDecoded & 0xff);
    linearIndex = (Int8)((decParam->indexInterFrameDecoded >> 8) & 0xff);
    if (linearIndex >= 0) {
        linearIndex = (Int8)VPU_CONVERT_WTL_INDEX(ctx->handle, linearIndex);
    }
    mvColIndex  = (Int8)((decParam->indexInterFrameDecoded >> 16) & 0xff);

    sizeInfo.width  = decParam->width;
    sizeInfo.height = decParam->height;

    VLOG(INFO, "%s:%d Changed framebuffer idx fbcIndex %d, linearIndex: %d\n", __FUNCTION__, __LINE__, fbcIndex, linearIndex);

    osal_memset(intResChangedFbs, 0x00, sizeof(FrameBuffer)*2);
    osal_memset(intResChangedMem, 0x00, sizeof(vpu_buffer_t)*2);
    
    intResChangedFbs[0].width = sizeInfo.width;
    intResChangedFbs[0].height = sizeInfo.height;
    intResChangedFbs[1].width = sizeInfo.width;
    intResChangedFbs[1].height = sizeInfo.height;

    intResChangedFbs[0].myIndex = -1;
    intResChangedFbs[1].myIndex = -1;

    if (mvColIndex >= 0) {
        sizeInfo.type = AUX_BUF_MV_COL;
        auxBufferInfo.num = 1;
        auxBufferInfo.bufArray = &bufArr;
        auxBufferInfo.type = AUX_BUF_MV_COL;
        if ((ret_code = VPU_DecGetAuxBufSize(ctx->handle, sizeInfo, &(buf_info.size))) == RETCODE_SUCCESS) {
            if (vdi_allocate_dma_memory(core_idx, &buf_info, DEC_MV, inst_idx) < 0) {
                    ret_code = RETCODE_INSUFFICIENT_RESOURCE;
            } else {
                auxBufferInfo.bufArray[0].index = mvColIndex;
                auxBufferInfo.bufArray[0].size = buf_info.size;
                auxBufferInfo.bufArray[0].addr = buf_info.phys_addr;
                
                if ((ret_code = VPU_DecRegisterAuxBuffer(ctx->handle, auxBufferInfo)) == RETCODE_SUCCESS) {
                    ret_code = VPU_DecUpdateFrameBuffer(ctx->handle, &(intResChangedFbs[0]), &(intResChangedFbs[1]), (Int32)mvColIndex, sizeInfo.width, sizeInfo.height);
                }
            }
        }
    }

    if (ret_code == RETCODE_SUCCESS) {
        if (AllocateDecFrameBuffer(ctx->handle, &ctx->testDecConfig, (fbcIndex>=0?1:0), (linearIndex>=0?1:0), intResChangedFbs, intResChangedMem, &intResChangedStride) == FALSE) {
            VLOG(INFO, "%s:%d Failed to AllocateDecFrameBuffer()\n", __FUNCTION__, __LINE__);
            return RETCODE_FAILURE;
        }
    } else {
        return RETCODE_FAILURE;
    }

    intResChangedFbs[0].myIndex = -1;
    intResChangedFbs[1].myIndex = -1;

    if (fbcIndex >= 0) {
        ctx->pFrame[fbcIndex] = intResChangedFbs[0];
        ctx->pFbMem[fbcIndex] = intResChangedMem[0];
        ctx->pFrame[fbcIndex].myIndex = fbcIndex;
    }
    if (linearIndex >= 0) {
        ctx->pFrame[linearIndex] = intResChangedFbs[1];
        ctx->pFbMem[linearIndex] = intResChangedMem[1];
        ctx->pFrame[linearIndex].myIndex = linearIndex;
    }



    osal_memset(&buf_info, 0x00, sizeof(vpu_buffer_t));
    osal_memset(&auxBufferInfo, 0x00, sizeof(AuxBufferInfo));

    if (fbcIndex >= 0) {
        sizeInfo.type = AUX_BUF_FBC_Y_TBL;
        auxBufferInfo.num = 1;
        auxBufferInfo.bufArray = &bufArr;
        auxBufferInfo.type = AUX_BUF_FBC_Y_TBL;
        if ((ret_code = VPU_DecGetAuxBufSize(ctx->handle, sizeInfo, &(buf_info.size))) == RETCODE_SUCCESS) {
            if (vdi_allocate_dma_memory(core_idx, &buf_info, DEC_FBCY_TBL, inst_idx) < 0) {
                    ret_code = RETCODE_INSUFFICIENT_RESOURCE;
            } else {
                auxBufferInfo.bufArray[0].index = fbcIndex;
                auxBufferInfo.bufArray[0].size = buf_info.size;
                auxBufferInfo.bufArray[0].addr = buf_info.phys_addr;
                ret_code = VPU_DecRegisterAuxBuffer(ctx->handle, auxBufferInfo);
            }
        }
    }

    osal_memset(&buf_info, 0x00, sizeof(vpu_buffer_t));
    osal_memset(&auxBufferInfo, 0x00, sizeof(AuxBufferInfo));

    if (ret_code == RETCODE_SUCCESS && fbcIndex >= 0) {
        sizeInfo.type = AUX_BUF_FBC_C_TBL;
        auxBufferInfo.num = 1;
        auxBufferInfo.bufArray = &bufArr;
        auxBufferInfo.type = AUX_BUF_FBC_C_TBL;
        if ((ret_code = VPU_DecGetAuxBufSize(ctx->handle, sizeInfo, &buf_info.size)) == RETCODE_SUCCESS) {
            if (vdi_allocate_dma_memory(core_idx, &buf_info, DEC_FBCC_TBL, inst_idx) < 0) {
                    ret_code = RETCODE_INSUFFICIENT_RESOURCE;
            } else {
                auxBufferInfo.bufArray[0].index = fbcIndex;
                auxBufferInfo.bufArray[0].size = buf_info.size;
                auxBufferInfo.bufArray[0].addr = buf_info.phys_addr;
                ret_code = VPU_DecRegisterAuxBuffer(ctx->handle, auxBufferInfo);
            }
        }
    } else {
        return RETCODE_FAILURE;
    }
    
    if (fbcIndex >= 0) {
        ret_code = VPU_DecUpdateFrameBuffer(ctx->handle, ctx->pFrame+fbcIndex, &(intResChangedFbs[1]), (Int32)-1, sizeInfo.width, sizeInfo.height);
    }

    if (linearIndex >= 0 && ret_code == RETCODE_SUCCESS) {
        ret_code = VPU_DecUpdateFrameBuffer(ctx->handle,&(intResChangedFbs[0]), ctx->pFrame+linearIndex, (Int32)-1, sizeInfo.width, sizeInfo.height);
    }

    ctx->fbAllocated = TRUE;

    return ret_code;
}

static BOOL AllocatePPUFrameBuffer(ComponentImpl* com)
{
    RendererContext*        ctx = (RendererContext*)com->context;
    TestDecConfig*          decConfig = &(ctx->testDecConfig);
    BOOL                    resOf = FALSE;

    osal_memset((void*)ctx->pPPUFbMem, 0x00, sizeof(ctx->pPPUFbMem));
    osal_memset((void*)ctx->pPPUFrame, 0x00, sizeof(ctx->pPPUFrame));

    if(PRODUCT_ID_960 == decConfig->productId || PRODUCT_ID_980 == decConfig->productId) {
        if (FALSE == Coda9AllocateDecPPUFrameBuffer(&(ctx->enablePPU), ctx->handle, &ctx->testDecConfig, ctx->pPPUFrame, ctx->pPPUFbMem, ctx->ppuQ)) {
            VLOG(INFO, "%s:%d Failed to Coda9AllocateDecPPUFrameBuffer()\n", __FUNCTION__, __LINE__);
        }
        resOf = TRUE;
    }

    return resOf;
}

static CNMComponentParamRet GetParameterRenderer(ComponentImpl* from, ComponentImpl* com, GetParameterCMD commandType, void* data)
{
    RendererContext*        ctx     = (RendererContext*)com->context;
    ParamDecFrameBuffer*    allocFb = NULL;
    ParamDecPPUFrameBuffer* allocPPUFb = NULL;
    PortContainer*         container;

    if (ctx->fbAllocated == FALSE) return CNM_COMPONENT_PARAM_NOT_READY;

    switch(commandType) {
    case GET_PARAM_COM_IS_CONTAINER_CONUSUMED:
        // This query command is sent from the comonponent core.
        // If input data are consumed in sequence, it should return TRUE through PortContainer::consumed.
        container = (PortContainer*)data;
        container->consumed = TRUE;
        break;
    case GET_PARAM_RENDERER_FRAME_BUF:
        allocFb = (ParamDecFrameBuffer*)data;
        allocFb->stride        = ctx->framebufStride;
        allocFb->linearNum     = ctx->fbCount.linearNum;
        allocFb->nonLinearNum  = ctx->fbCount.nonLinearNum;
        allocFb->fb            = ctx->pFrame;
        break;
    case GET_PARAM_RENDERER_PPU_FRAME_BUF:
        allocPPUFb = (ParamDecPPUFrameBuffer*)data;
        if (TRUE == AllocatePPUFrameBuffer(com)) {
            allocPPUFb->enablePPU   = ctx->enablePPU;
            allocPPUFb->ppuQ        = ctx->ppuQ;
            allocPPUFb->fb          = ctx->pPPUFrame;
        }
        else {
            allocPPUFb->enablePPU   = FALSE;
            allocPPUFb->ppuQ        = NULL;
            allocPPUFb->fb          = NULL;
        }
        break;
    default:
        return CNM_COMPONENT_PARAM_NOT_FOUND;
    }

    return CNM_COMPONENT_PARAM_SUCCESS;
}

static CNMComponentParamRet SetParameterRenderer(ComponentImpl* from, ComponentImpl* com, SetParameterCMD commandType, void* data)
{
    RendererContext*    ctx    = (RendererContext*)com->context;
    BOOL                result = TRUE;

    UNREFERENCED_PARAMETER(ctx);
    switch(commandType) {
    case SET_PARAM_COM_PAUSE:
        com->pause = *(BOOL*)data;
        break;
    case SET_PARAM_COM_STOP:
        com->terminate = *(BOOL*)data;
        break;
    case SET_PARAM_RENDERER_REALLOC_FRAMEBUFFER:
        result = ReallocateFrameBuffers(com, (ParamDecReallocFB*)data);
        break;
    case SET_PARAM_RENDERER_FREE_FRAMEBUFFERS:
        result = SetParamFreeFrameBuffers(com, *(Uint32*)data);
        break;
    case SET_PARAM_RENDERER_FLUSH:
        result = FlushFrameBuffers(com, (Uint32*)data);
        break;
    case SET_PARAM_RENDERER_ALLOC_FRAMEBUFFERS:
        result = AllocateFrameBuffer(com);
        break;
    case SET_PARAM_RENDERER_INTRES_CHANGED_ALLOC_FRAMEBUFFERS:
        if (ctx->testDecConfig.productId == PRODUCT_ID_637) {
            result = (UpdateChangedBuffer(com, (ParamDecReallocFB*)data) == RETCODE_SUCCESS) ? TRUE : FALSE ;
        } else {
            result = AllocateIntResChangedFrameBuffer(com, (ParamDecReallocFB*)data);
        }
        break;
    case SET_PARAM_RENDERER_CHANGE_COM_STATE:
        result = ComponentChangeState(com, *(Uint32*)data);
        break;
    case SET_PARAM_RENDERER_INTRES_CHANGED_FREE_FRAMEBUFFERS:
        result = IntResChangedFreeFrameBuffers(com, (ParamDecReallocFB*)data);
        break;
    case SET_PARAM_RENDERER_RELEASE_FRAME_BUFFRES:
        ReleaseFrameBuffers(com);
        break;
    default:
        return CNM_COMPONENT_PARAM_NOT_FOUND;
    }

    if (result == TRUE) return CNM_COMPONENT_PARAM_SUCCESS;
    else                return CNM_COMPONENT_PARAM_FAILURE;
}

static BOOL ExecuteRenderer(ComponentImpl* com, PortContainer* in, PortContainer* out)
{
    RendererContext*        ctx               = (RendererContext*)com->context;
    PortContainerDisplay*   srcData           = (PortContainerDisplay*)in;
    Int32                   indexFrameDisplay;
    TestDecConfig*          decConfig         = &ctx->testDecConfig;
    RendererOutInfo         rendererOutInfo;

    if (TRUE == com->pause) {
        return TRUE;
    }

    in->reuse = TRUE;


    indexFrameDisplay = srcData->decInfo.indexFrameDisplay;
    if (indexFrameDisplay == DISPLAY_IDX_FLAG_SEQ_END) {
        ComponentSetParameter(com, com->srcPort.connectedComponent, SET_PARAM_COM_STOP, NULL);
        Int32 idx = 0;
        for (idx = 0; idx < MAX_SEQ_CHANGE_QUEUE_NUM; idx++) {
            if (Queue_Peek(ctx->seqMemQ) !=NULL) {
                HandleSeqChangedBuffers(com);
            }
        }
        com->pause = TRUE;
    }
    else if (indexFrameDisplay >= 0) {

        rendererOutInfo.pOutInfo = &(srcData->decInfo);
        rendererOutInfo.scaleX = srcData->scaleX;
        rendererOutInfo.scaleY = srcData->scaleY;
        rendererOutInfo.displayed_num = ctx->displayed_num;

        if (ctx->testDecConfig.outputPvric.pvricMode && ctx->testDecConfig.outputPvric.pvricVersion == 5) {
        } else {
            if (strlen((const char*)decConfig->outputPath) > 0) {
                VpuRect      rcDisplay  = {0,};
                TiledMapType mapType    = srcData->decInfo.dispFrame.mapType;

                rcDisplay.right = srcData->decInfo.rcDisplay.right;
                rcDisplay.left  = srcData->decInfo.rcDisplay.left;
                rcDisplay.top   = srcData->decInfo.rcDisplay.top;
                rcDisplay.bottom= srcData->decInfo.rcDisplay.bottom;
                if (decConfig->scaleDownWidth > 0 || decConfig->scaleDownHeight > 0) {
                    rcDisplay.right  = VPU_CEIL(srcData->decInfo.dispPicWidth, 16);
                }

                if (ctx->fpOutput[0] == NULL) {
                    if (OpenDisplayBufferFile(decConfig->bitFormat, decConfig->outputPath, rcDisplay, mapType, ctx->fpOutput) == FALSE) {
                        return FALSE;
                    }
                }
                SaveDisplayBufferToFile(ctx->handle, decConfig->bitFormat, &rendererOutInfo, srcData->decInfo.dispFrame, rcDisplay, ctx->fpOutput, FALSE);
            }
        }

        DisplayFrame(com, &srcData->decInfo);
        osal_msleep(ctx->displayPeriodTime);
    }

    srcData->consumed = TRUE;
    srcData->reuse    = FALSE;

    if (srcData->last == TRUE) {
        ComponentSetParameter(com, com->srcPort.connectedComponent, SET_PARAM_COM_STOP, NULL);
        com->pause = TRUE;
    }

    return TRUE;
}

static BOOL PrepareRenderer(ComponentImpl* com, BOOL* done)
{
    RendererContext* ctx = (RendererContext*)com->context;
    BOOL             ret;

    if (ctx->handle == NULL) {
        CNMComponentParamRet paramRet;
        BOOL                 success;
        paramRet = ComponentGetParameter(com, com->srcPort.connectedComponent, GET_PARAM_DEC_HANDLE, &ctx->handle);
        if (ComponentParamReturnTest(paramRet, &success) == FALSE) {
            return success;
        }
    }

    if ((ret=AllocateFrameBuffer(com)) == FALSE || ctx->fbAllocated == FALSE) {
        return ret;
    }

    *done = TRUE;
    return TRUE;
}

static Component CreateRenderer(ComponentImpl* com, CNMComponentConfig* componentParam)
{
    RendererContext*    ctx;

    com->context = (RendererContext*)osal_malloc(sizeof(RendererContext));
    osal_memset(com->context, 0, sizeof(RendererContext));
    ctx = com->context;

    osal_memcpy((void*)&ctx->testDecConfig, (void*)&componentParam->testDecConfig, sizeof(TestDecConfig));
    if (componentParam->testDecConfig.fps > 0)
        ctx->displayPeriodTime = (1000 / componentParam->testDecConfig.fps);
    else
        ctx->displayPeriodTime = 1000/30;
    ctx->frameNumToStop    = componentParam->testDecConfig.forceOutNum;
    ctx->fbAllocated       = FALSE;
    ctx->seqMemQ           = Queue_Create(MAX_SEQ_CHANGE_QUEUE_NUM, sizeof(SequenceMemInfo));
    ctx->dispFlagQ         = Queue_Create(MAX_SEQ_CHANGE_QUEUE_NUM, sizeof(Int32));
    ctx->lock              = osal_mutex_create();
    ctx->ppuQ              = Queue_Create(MAX_FRAME_NUM, sizeof(FrameBuffer));

    return (Component)com;
}

static void ReleaseComponent(ComponentImpl* com)
{
}

static BOOL DestroyComponent(ComponentImpl* com)
{
    RendererContext* ctx = (RendererContext*)com->context;

    CloseDisplayBufferFile(ctx->fpOutput);
    Queue_Destroy(ctx->seqMemQ);
    Queue_Destroy(ctx->dispFlagQ);
    Queue_Destroy(ctx->ppuQ);
    osal_mutex_destroy(ctx->lock);
    osal_free(ctx);

    return TRUE;
}

ComponentImpl rendererComponentImpl = {
    "renderer",
    NULL,
    {0,},
    {0,},
    sizeof(PortContainer),     /* No output */
    5,
    CreateRenderer,
    GetParameterRenderer,
    SetParameterRenderer,
    PrepareRenderer,
    ExecuteRenderer,
    ReleaseComponent,
    DestroyComponent
};

