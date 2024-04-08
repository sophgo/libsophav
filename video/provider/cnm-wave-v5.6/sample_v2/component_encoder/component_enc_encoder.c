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
#include "cnm_app.h"
#include "component.h"
#include "misc/debug.h"
#include "main_helper.h"

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif


#define ENC_DESTROY_TIME_OUT (60000*2) //2 min

typedef enum {
    ENC_INT_STATUS_NONE,        // Interrupt not asserted yet
    ENC_INT_STATUS_FULL,        // Need more buffer
    ENC_INT_STATUS_DONE,        // Interrupt asserted
    ENC_INT_STATUS_LOW_LATENCY,
    ENC_INT_STATUS_TIMEOUT,     // Interrupt not asserted during given time.
    ENC_INT_STATUS_SRC_RELEASED,
} ENC_INT_STATUS;

typedef enum {
    ENCODER_STATE_OPEN,
    ENCODER_STATE_INIT_SEQ,
    ENCODER_STATE_REGISTER_FB,
    ENCODER_STATE_ENCODE_HEADER,
    ENCODER_STATE_ENCODING,
} EncoderState;

typedef struct {
    EncHandle                   handle;
    TestEncConfig               testEncConfig;
    EncOpenParam                encOpenParam;
    ParamEncNeedFrameBufferNum  fbCount;
    Uint32                      fbCountValid;
    Uint32                      frameIdx;
    vpu_buffer_t                vbNalSizeReportData;
    vpu_buffer_t                vbCustomLambda;
    vpu_buffer_t                vbScalingList;
    Uint32                      customLambda[NUM_CUSTOM_LAMBDA];
    UserScalingList             scalingList;
    vpu_buffer_t                vbCustomMap[MAX_FRAME_NUM];
    vpu_buffer_t                vbPrefixSeiNal[MAX_FRAME_NUM];
    vpu_buffer_t                vbSuffixSeiNal[MAX_FRAME_NUM];
    vpu_buffer_t                vbHrdRbsp;
    vpu_buffer_t                vbVuiRbsp;
    vpu_buffer_t                vbWork;
    vpu_buffer_t                vbTemp;
    vpu_buffer_t                vbSecAxi;
    vpu_buffer_t                vbAr;
    vpu_buffer_t                vbAux[AUX_BUF_TYPE_MAX][MAX_FRAME_NUM];
    EncoderState                state;
    BOOL                        stateDoing;
    EncInitialInfo              initialInfo;
    Queue*                      encOutQ;
    EncParam                    encParam;
    Int32                       encodedSrcFrmIdxArr[ENC_SRC_BUF_NUM];
    ParamEncBitstreamBuffer     bsBuf;
    BOOL                        fullInterrupt;
    Uint32                      changedCount;
    Uint32                      changedPicParamCount;
    Uint64                      startTimeout;
    Uint64                      desStTimeout;
    Uint32                      iterationCnt;
    VpuAttr                     attr;
#ifdef SUPPORT_READ_BITSTREAM_IN_ENCODER
    char                        bitstreamFileName[MAX_FILE_PATH];
    osal_file_t                 bs_fp;
    BitstreamReader             bsReader;
#endif
    Uint32                      hdr_enc_flag;
    ENC_subFrameSyncCfg         subFrameSyncCfg;
    Uint32                      cyclePerTick;
    vpu_buffer_t                *bsBuffer[20];
    Uint32                      frameOutNum;
    Uint32                      checkEncodedCnt;
    Uint32                      srcEndFlag;
} EncoderContext;



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

static BOOL AllocateworkBuffer(ComponentImpl* com)
{
    EncoderContext* ctx       = (EncoderContext*)com->context;
    TestEncConfig testEncConfig = ctx->testEncConfig;
    EncOpenParam* pOpenParam = &ctx->encOpenParam;

    ctx->vbWork.phys_addr = 0;
    ctx->vbWork.size      = VPU_ALIGN4096(WAVE627ENC_WORKBUF_SIZE);
    if (vdi_allocate_dma_memory(testEncConfig.coreIdx, &ctx->vbWork, ENC_WORK, 0) < 0)
        return RETCODE_INSUFFICIENT_RESOURCE;

    vdi_clear_memory(testEncConfig.coreIdx, ctx->vbWork.phys_addr, ctx->vbWork.size, 0);

    pOpenParam->instBuffer.workBufBase = ctx->vbWork.phys_addr;
    pOpenParam->instBuffer.workBufSize = ctx->vbWork.size;

    return TRUE;
}

static BOOL AllocateTempBuffer(ComponentImpl* com)
{
    EncoderContext* ctx       = (EncoderContext*)com->context;
    TestEncConfig testEncConfig = ctx->testEncConfig;
    EncOpenParam* pOpenParam = &ctx->encOpenParam;

    ctx->vbTemp.phys_addr = 0;
    ctx->vbTemp.size      = VPU_ALIGN4096(WAVE6_TEMPBUF_SIZE);
    if (vdi_allocate_dma_memory(testEncConfig.coreIdx, &ctx->vbTemp, ENC_TEMP, 0) < 0)
        return RETCODE_INSUFFICIENT_RESOURCE;
    pOpenParam->instBuffer.tempBufBase = ctx->vbTemp.phys_addr;
    pOpenParam->instBuffer.tempBufSize = ctx->vbTemp.size;

    return TRUE;
}

static BOOL AllocateSecAxiBuffer(ComponentImpl* com)
{
    EncoderContext* ctx       = (EncoderContext*)com->context;
    TestEncConfig testEncConfig = ctx->testEncConfig;
    EncOpenParam* pOpenParam = &ctx->encOpenParam;

    if (vdi_get_sram_memory(testEncConfig.coreIdx, &ctx->vbSecAxi) < 0)
        return RETCODE_INSUFFICIENT_RESOURCE;

    pOpenParam->instBuffer.secAxiBufBase = ctx->vbSecAxi.phys_addr;
    pOpenParam->instBuffer.secAxiBufSize = ctx->vbSecAxi.size;

    return TRUE;
}

static BOOL AllocateArTableBuffer(ComponentImpl* com)
{
    EncoderContext* ctx       = (EncoderContext*)com->context;
    TestEncConfig testEncConfig = ctx->testEncConfig;
    EncOpenParam*   pOpenParam;
    pOpenParam = &ctx->encOpenParam;

#define WAVE6_AR_TABLE_BUF_SIZE 1024 //adative round
    ctx->vbAr.phys_addr = 0;
    ctx->vbAr.size      = VPU_ALIGN4096(WAVE6_AR_TABLE_BUF_SIZE);
    if (vdi_allocate_dma_memory(testEncConfig.coreIdx, &ctx->vbAr, ENC_AR, 0) < 0)
        return RETCODE_INSUFFICIENT_RESOURCE;
    pOpenParam->instBuffer.arTblBufBase = ctx->vbAr.phys_addr;
    return TRUE;
}

static RetCode AllocateAuxBuffer(ComponentImpl* com, EncAuxBufferSizeInfo sizeInfo, AuxBufferType type, MemTypes memTypes)
{
    EncoderContext* ctx = (EncoderContext*)com->context;
    AuxBufferInfo auxBufferInfo;
    AuxBuffer bufArr[MAX_REG_FRAME];
    Uint32 i;
    RetCode ret_code = RETCODE_FAILURE;

    sizeInfo.type = type;
    auxBufferInfo.type = type;
    auxBufferInfo.num = ctx->fbCount.reconFbNum;
    if (type == AUX_BUF_DEF_CDF)
        auxBufferInfo.num = 1;
    
    osal_memset(bufArr, 0x00, sizeof(bufArr));
    auxBufferInfo.bufArray = bufArr;

    for (i = 0; i < auxBufferInfo.num; i++) {
        if ((ret_code = VPU_EncGetAuxBufSize(ctx->handle, sizeInfo, &ctx->vbAux[type][i].size)) != RETCODE_SUCCESS) {
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
        ret_code = VPU_EncRegisterAuxBuffer(ctx->handle, auxBufferInfo);
    }
    
    return ret_code;
}

static void SetEncPicParam(ComponentImpl* com, PortContainerYuv* in, EncParam* encParam)
{
    EncoderContext* ctx           = (EncoderContext*)com->context;
    TestEncConfig   testEncConfig = ctx->testEncConfig;
    Uint32          frameIdx      = ctx->frameIdx;
    Int32           srcFbWidth    = VPU_ALIGN8(ctx->encOpenParam.picWidth);
    Int32           srcFbHeight   = VPU_ALIGN8(ctx->encOpenParam.picHeight);
    Uint32          productId;
    vpu_buffer_t*   buf           = (vpu_buffer_t*)Queue_Peek(ctx->encOutQ);

    productId = VPU_GetProductId(ctx->encOpenParam.coreIdx);

    encParam->picStreamBufferAddr                = buf->phys_addr;
    encParam->picStreamBufferSize                = buf->size;
    encParam->srcIdx                             = in->srcFbIndex;
    encParam->srcEndFlag                         = in->last;
    encParam->sourceFrame                        = &in->fb;
    encParam->sourceFrame->sourceLBurstEn        = 0;
    if (testEncConfig.useAsLongtermPeriod > 0 && testEncConfig.refLongtermPeriod > 0) {
        encParam->useCurSrcAsLongtermPic         = (frameIdx % testEncConfig.useAsLongtermPeriod) == 0 ? 1 : 0;
        encParam->useLongtermRef                 = (frameIdx % testEncConfig.refLongtermPeriod)   == 0 ? 1 : 0;
    }

    if (((Int32)frameIdx >= testEncConfig.force_picskip_start) && ((Int32)frameIdx <= testEncConfig.force_picskip_end))
        encParam->skipPicture                    = 1;
    else
        encParam->skipPicture                    = 0;

   if (((Int32)frameIdx >= testEncConfig.force_coefdrop_start) && ((Int32)frameIdx <= testEncConfig.force_coefdrop_end))
        encParam->forceAllCtuCoefDropEnable	     = 1;
    else
        encParam->forceAllCtuCoefDropEnable      = 0;

    if (PRODUCT_ID_W6_SERIES(ctx->testEncConfig.productId)) {
        if (in->prevDataReuse == FALSE) {
            
            // set encParam change pic param to its default value
            encParam->forcePicQpEnable   = 0;
            encParam->forcePicQpI        = 0;
            encParam->forcePicQpP        = 0;
            encParam->forcePicQpB        = 0;
            encParam->forcePicTypeEnable = 0;
            encParam->forcePicType       = 0;

            // to support old cfg (when cpp from default cfg)
            if(testEncConfig.force_picskip_end == -1 && testEncConfig.force_picskip_start == -1)
                encParam->skipPicture = 0;

            if ((ctx->testEncConfig.numChangePicParam > ctx->changedPicParamCount) &&
                 (ctx->testEncConfig.changePicParam[ctx->changedPicParamCount].setParaChgFrmNum == ctx->frameIdx)) {
                //get information(forcePicQpEnable, forcePicQpI, forcePicQpP, forcePicQpB, forcePicTypeEnable, forcePicType)
                W6SetChangePicParam(ctx->handle, &ctx->testEncConfig, ctx->changedPicParamCount, encParam);
                ctx->changedPicParamCount++;
            }
        }
    } else {
        encParam->forcePicQpEnable                   = 0;
        encParam->forcePicQpI                        = 0;
        encParam->forcePicQpP                        = 0;
        encParam->forcePicQpB                        = 0;
        encParam->forcePicTypeEnable                 = 0;
        encParam->forcePicType                       = 0;
    }


    if ( testEncConfig.forceIdrPicIdx != 0) {
        if (testEncConfig.forceIdrPicIdx == frameIdx) {
            encParam->forcePicTypeEnable = 1;
            encParam->forcePicType = 3;    // IDR
        }
    }
    
    //force pic type
    if ( testEncConfig.forcePicTypePicIdx != 0) {
        if (testEncConfig.forcePicTypePicIdx == frameIdx) {
            if (testEncConfig.forcePicTypeValue == 2) {
                encParam->forcePicTypeEnable = 0;
                encParam->forcePicType = 0;
            } else {
                encParam->forcePicTypeEnable = 1;
                encParam->forcePicType = testEncConfig.forcePicTypeValue;
            }
        }
    }
    if ( testEncConfig.forcePicQpPicIdx != 0) { 
        if (testEncConfig.forcePicQpPicIdx == frameIdx) {
            encParam->forcePicQpEnable = 1;
            encParam->forcePicQpI = testEncConfig.cur_w6_param.ForcePicQpValue;
            encParam->forcePicQpP = testEncConfig.cur_w6_param.ForcePicQpValue;
            encParam->forcePicQpB = testEncConfig.cur_w6_param.ForcePicQpValue;
        }
    }

    // FW will encode header data implicitly when changing the header syntaxes
    encParam->codeOption.implicitHeaderEncode    = 1;
    if(PRODUCT_ID_W5_SERIES(productId)) {
        encParam->codeOption.encodeAUD               = testEncConfig.encAUD; //WAVE5 only
    }
    if (frameIdx == (testEncConfig.outNum - 1)) { // last frame
        encParam->codeOption.encodeEOS           = testEncConfig.encEOS;
        encParam->codeOption.encodeEOB           = testEncConfig.encEOB;
    }
    else {
        encParam->codeOption.encodeEOS           = 0;
        encParam->codeOption.encodeEOB           = 0;
    }

    // set custom map param
    if (productId != PRODUCT_ID_521)
        encParam->customMapOpt.roiAvgQp           = testEncConfig.roi_avg_qp;
    encParam->customMapOpt.customLambdaMapEnable = testEncConfig.lambda_map_enable;
    encParam->customMapOpt.customModeMapEnable	 = testEncConfig.mode_map_flag & 0x1;
    encParam->customMapOpt.customCoefDropEnable  = (testEncConfig.mode_map_flag & 0x2) >> 1;
    encParam->customMapOpt.customRoiMapEnable    = testEncConfig.roi_enable;


    if (in->prevDataReuse == FALSE && encParam->srcEndFlag != TRUE) {
        if (PRODUCT_ID_W6_SERIES(testEncConfig.productId))
        {
            if (ctx->testEncConfig.roi_enable || ctx->testEncConfig.mode_map_flag) {
                SetMapData(testEncConfig.coreIdx, testEncConfig, ctx->encOpenParam, encParam, srcFbWidth, srcFbHeight, ctx->vbCustomMap[encParam->srcIdx].phys_addr);
            }
        } else {
            // packaging roi/lambda/mode data to custom map buffer.
            if (encParam->customMapOpt.customRoiMapEnable  || encParam->customMapOpt.customLambdaMapEnable ||
                encParam->customMapOpt.customModeMapEnable || encParam->customMapOpt.customCoefDropEnable) {
                SetMapData(testEncConfig.coreIdx, testEncConfig, ctx->encOpenParam, encParam, srcFbWidth, srcFbHeight, ctx->vbCustomMap[encParam->srcIdx].phys_addr);
            }
        }

        // host should set proper value.
        // set weighted prediction param.
        if (testEncConfig.wp_param_flag) {
            char lineStr[256] = {0,};
            Uint32 meanY, meanCb, meanCr, sigmaY, sigmaCb, sigmaCr;
            Uint32 maxMean  = (ctx->encOpenParam.bitstreamFormat == STD_AVC) ? 0xffff : ((1 << ctx->encOpenParam.EncStdParam.waveParam.internalBitDepth) - 1);
            Uint32 maxSigma = (ctx->encOpenParam.bitstreamFormat == STD_AVC) ? 0xffff : ((1 << (ctx->encOpenParam.EncStdParam.waveParam.internalBitDepth + 6)) - 1);
            fgets(lineStr, 256, testEncConfig.wp_param_file);
            sscanf(lineStr, "%d %d %d %d %d %d\n", &meanY, &meanCb, &meanCr, &sigmaY, &sigmaCb, &sigmaCr);

            meanY	= max(min(maxMean, meanY), 0);
            meanCb	= max(min(maxMean, meanCb), 0);
            meanCr	= max(min(maxMean, meanCr), 0);
            sigmaY	= max(min(maxSigma, sigmaY), 0);
            sigmaCb = max(min(maxSigma, sigmaCb), 0);
            sigmaCr = max(min(maxSigma, sigmaCr), 0);

            // set weighted prediction param.
            encParam->wpPixSigmaY	= sigmaY;
            encParam->wpPixSigmaCb	= sigmaCb;
            encParam->wpPixSigmaCr	= sigmaCr;
            encParam->wpPixMeanY    = meanY;
            encParam->wpPixMeanCb	= meanCb;
            encParam->wpPixMeanCr	= meanCr;
        }
        if ((testEncConfig.seiDataEnc.prefixSeiNalEnable || testEncConfig.seiDataEnc.suffixSeiNalEnable) && encParam->srcEndFlag != 1) {
            if (testEncConfig.prefix_sei_nal_file_name[0] && testEncConfig.seiDataEnc.prefixSeiNalEnable) {
                Uint8   *pUserBuf;

                if((testEncConfig.seiDataEnc.prefixSeiDataSize > 0) && (testEncConfig.seiDataEnc.prefixSeiDataSize < SEI_NAL_DATA_BUF_SIZE)) {
                    long    cur_pt = 0;
                    
                    pUserBuf = (Uint8*)osal_malloc(testEncConfig.seiDataEnc.prefixSeiDataSize);
                    osal_memset(pUserBuf, 0, testEncConfig.seiDataEnc.prefixSeiDataSize);
                    cur_pt = osal_ftell(testEncConfig.prefix_sei_nal_fp);
                
                    if(testEncConfig.prefix_sei_file_max_size < (cur_pt + testEncConfig.seiDataEnc.prefixSeiDataSize)) {
                        int read_size_1 = testEncConfig.prefix_sei_file_max_size - cur_pt;
                        int read_size_2 = (cur_pt + testEncConfig.seiDataEnc.prefixSeiDataSize) - testEncConfig.prefix_sei_file_max_size;

                        osal_fread(pUserBuf, 1, read_size_1, testEncConfig.prefix_sei_nal_fp);
                        osal_fseek(testEncConfig.prefix_sei_nal_fp, 0, SEEK_SET);
                        osal_fread(&pUserBuf[read_size_1], 1, read_size_2, testEncConfig.prefix_sei_nal_fp);
                    } else {
                        osal_fread(pUserBuf, 1, testEncConfig.seiDataEnc.prefixSeiDataSize, testEncConfig.prefix_sei_nal_fp);
                    }
                    vdi_write_memory(testEncConfig.coreIdx, ctx->vbPrefixSeiNal[encParam->srcIdx].phys_addr, pUserBuf, testEncConfig.seiDataEnc.prefixSeiDataSize, HOST_ENDIAN);
                    osal_free(pUserBuf);
                } else {
                    VLOG(ERR, "Invalid prefix SEI data size: (Cur:%d, Max:%d)\n", testEncConfig.seiDataEnc.prefixSeiDataSize, SEI_NAL_DATA_BUF_SIZE);
                }
            }
            testEncConfig.seiDataEnc.prefixSeiNalAddr = ctx->vbPrefixSeiNal[encParam->srcIdx].phys_addr;

            if (testEncConfig.suffix_sei_nal_file_name[0] && testEncConfig.seiDataEnc.suffixSeiNalEnable) {
                Uint8   *pUserBuf;

                if((testEncConfig.seiDataEnc.suffixSeiDataSize > 0) && (testEncConfig.seiDataEnc.suffixSeiDataSize < SEI_NAL_DATA_BUF_SIZE)) {
                    long    cur_pt = 0;

                    pUserBuf = (Uint8*)osal_malloc(testEncConfig.seiDataEnc.suffixSeiDataSize);
                    osal_memset(pUserBuf, 0, testEncConfig.seiDataEnc.suffixSeiDataSize);
                    cur_pt = osal_ftell(testEncConfig.suffix_sei_nal_fp);

                    if(testEncConfig.suffix_sei_file_max_size < (cur_pt + testEncConfig.seiDataEnc.suffixSeiDataSize)) {
                        int read_size_1 = testEncConfig.suffix_sei_file_max_size - cur_pt;
                        int read_size_2 = (cur_pt + testEncConfig.seiDataEnc.suffixSeiDataSize) - testEncConfig.suffix_sei_file_max_size;

                        osal_fread(pUserBuf, 1, read_size_1, testEncConfig.suffix_sei_nal_fp);
                        osal_fseek(testEncConfig.suffix_sei_nal_fp, 0, SEEK_SET);
                        osal_fread(&pUserBuf[read_size_1], 1, read_size_2, testEncConfig.suffix_sei_nal_fp);
                    } else {
                        osal_fread(pUserBuf, 1, testEncConfig.seiDataEnc.suffixSeiDataSize, testEncConfig.suffix_sei_nal_fp);
                    }

                    vdi_write_memory(testEncConfig.coreIdx, ctx->vbSuffixSeiNal[encParam->srcIdx].phys_addr, pUserBuf, testEncConfig.seiDataEnc.suffixSeiDataSize,  HOST_ENDIAN);
                    osal_free(pUserBuf);
                } else {
                     VLOG(ERR, "Invalid suffix SEI data size: (Cur:%d, Max:%d)\n", testEncConfig.seiDataEnc.suffixSeiDataSize, SEI_NAL_DATA_BUF_SIZE);
                }
            }
            testEncConfig.seiDataEnc.suffixSeiNalAddr = ctx->vbSuffixSeiNal[encParam->srcIdx].phys_addr;

            VPU_EncGiveCommand(ctx->handle, ENC_SET_SEI_NAL_DATA, &testEncConfig.seiDataEnc);
        }
        encParam->updateLast2Bit          = testEncConfig.forced_10b;
        encParam->last2BitData            = testEncConfig.last_2bit_in_8to10;
        if (testEncConfig.csc_enable == 1) {
            encParam->csc.formatOrder     = testEncConfig.csc_format_order;
        }
    }
}

static BOOL RegisterFrameBuffers(ComponentImpl* com)
{
    EncoderContext*         ctx = (EncoderContext*)com->context;
    FrameBuffer*            pReconFb      = NULL;
    FrameBuffer*            pSrcFb        = NULL;
    FrameBufferAllocInfo    srcFbAllocInfo;
    Uint32                  reconFbStride = 0;
    Uint32                  reconFbHeight = 0;
    ParamEncFrameBuffer     paramFb;
    RetCode                 ret_code = RETCODE_FAILURE;
    CNMComponentParamRet    comp_ret_code;
    BOOL                    success;
    Uint32                  idx;
    TiledMapType            mapType;
    ctx->stateDoing = TRUE;
    comp_ret_code = ComponentGetParameter(com, com->srcPort.connectedComponent, GET_PARAM_YUVFEEDER_FRAME_BUF, (void*)&paramFb);
    if (ComponentParamReturnTest(comp_ret_code, &success) == FALSE) return success;

    pReconFb      = paramFb.reconFb;
    reconFbStride = paramFb.reconFbAllocInfo.stride;
    reconFbHeight = paramFb.reconFbAllocInfo.height;

    if ((ctx->attr.productId == PRODUCT_ID_521) && ctx->attr.supportDualCore == TRUE) {
        mapType = ctx->encOpenParam.EncStdParam.waveParam.internalBitDepth == 8 ? COMPRESSED_FRAME_MAP_DUAL_CORE_8BIT : COMPRESSED_FRAME_MAP_DUAL_CORE_10BIT;
        ret_code = VPU_EncRegisterFrameBuffer(ctx->handle, pReconFb, ctx->fbCount.reconFbNum, reconFbStride, reconFbHeight, mapType);
    } else {
        mapType = COMPRESSED_FRAME_MAP;
        if (PRODUCT_ID_W6_SERIES(ctx->testEncConfig.productId)) {
            EncAuxBufferSizeInfo sizeInfo = {0,};
            sizeInfo.width = ctx->encOpenParam.picWidth;
            sizeInfo.height = ctx->encOpenParam.picHeight;
            sizeInfo.mirrorDirection =  ctx->testEncConfig.mirDir;
            sizeInfo.rotationAngle = ctx->testEncConfig.rotAngle;

            if ((ret_code = AllocateAuxBuffer(com, sizeInfo, AUX_BUF_FBC_Y_TBL, ENC_FBCY_TBL)) != RETCODE_SUCCESS) {
                VLOG(ERR, "<%s:%d> return code : %d \n", __FUNCTION__, __LINE__, ret_code);
                return FALSE;
            }
            if ((ret_code = AllocateAuxBuffer(com, sizeInfo, AUX_BUF_FBC_C_TBL, ENC_FBCC_TBL)) != RETCODE_SUCCESS) {
                VLOG(ERR, "<%s:%d> return code : %d \n", __FUNCTION__, __LINE__, ret_code);
                return FALSE;
            }
            if ((ret_code = AllocateAuxBuffer(com, sizeInfo, AUX_BUF_MV_COL, ENC_MV)) != RETCODE_SUCCESS) {
                VLOG(ERR, "<%s:%d> return code : %d \n", __FUNCTION__, __LINE__, ret_code);
                return FALSE;
            }
            if ((ret_code = AllocateAuxBuffer(com, sizeInfo, AUX_BUF_SUB_SAMPLE, ENC_SUBSAMBUF)) != RETCODE_SUCCESS) {
                VLOG(ERR, "<%s:%d> return code : %d \n", __FUNCTION__, __LINE__, ret_code);
                return FALSE;
            }
            if (ctx->encOpenParam.bitstreamFormat == STD_AV1) {
                if ((ret_code = AllocateAuxBuffer(com, sizeInfo, AUX_BUF_DEF_CDF, ENC_DEF_CDF)) != RETCODE_SUCCESS) {
                    VLOG(ERR, "<%s:%d> return code : %d \n", __FUNCTION__, __LINE__, ret_code);
                    return FALSE;
                }
            }
        }

        ret_code = VPU_EncRegisterFrameBuffer(ctx->handle, pReconFb, ctx->fbCount.reconFbNum, reconFbStride, reconFbHeight, mapType);
    }

    if (ret_code != RETCODE_SUCCESS) {
        VLOG(ERR, "%s:%d Failed to VPU_EncRegisterFrameBuffer(%d)\n", __FUNCTION__, __LINE__, ret_code);
        ChekcAndPrintDebugInfo(ctx->handle, TRUE, ret_code);
        return FALSE;
    }
    ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_REGISTER_FB, NULL);

    pSrcFb = paramFb.srcFb;
    srcFbAllocInfo = paramFb.srcFbAllocInfo;
    ret_code = VPU_EncAllocateFrameBuffer(ctx->handle, srcFbAllocInfo, pSrcFb);
    if (ret_code != RETCODE_SUCCESS) {
        VLOG(ERR, "VPU_EncAllocateFrameBuffer fail to allocate source frame buffer\n");
        ChekcAndPrintDebugInfo(ctx->handle, TRUE, ret_code);
        return FALSE;
    }



    if (PRODUCT_ID_W6_SERIES(ctx->testEncConfig.productId)) {
        if (ctx->testEncConfig.roi_enable || ctx->testEncConfig.mode_map_flag) {
            for (idx = 0; idx < ctx->fbCount.srcFbNum; idx++) {
                ctx->vbCustomMap[idx].size = (ctx->encOpenParam.bitstreamFormat == STD_AVC) ? MAX_MB_NUM : MAX_CTU_NUM * 4;
                if (vdi_allocate_dma_memory(ctx->testEncConfig.coreIdx, &ctx->vbCustomMap[idx], ENC_ETC, ctx->handle->instIndex) < 0) {
                    VLOG(ERR, "fail to allocate ROI buffer\n");
                    return FALSE;
                }
            }
        }
    }
    else {
        if (ctx->testEncConfig.roi_enable || ctx->testEncConfig.lambda_map_enable || ctx->testEncConfig.mode_map_flag) {
            for (idx = 0; idx < ctx->fbCount.srcFbNum; idx++) {
                ctx->vbCustomMap[idx].size = (ctx->encOpenParam.bitstreamFormat == STD_AVC) ? MAX_MB_NUM : MAX_CTU_NUM * 8;
                if (vdi_allocate_dma_memory(ctx->testEncConfig.coreIdx, &ctx->vbCustomMap[idx], ENC_ETC, ctx->handle->instIndex) < 0) {
                    VLOG(ERR, "fail to allocate ROI buffer\n");
                    return FALSE;
                }
            }
        }
    }

    // allocate User data buffer amount of source buffer num
    if (ctx->testEncConfig.seiDataEnc.prefixSeiNalEnable) {
        if (ctx->testEncConfig.prefix_sei_nal_file_name[0]) {
            ChangePathStyle(ctx->testEncConfig.prefix_sei_nal_file_name);
            if ((ctx->testEncConfig.prefix_sei_nal_fp = osal_fopen(ctx->testEncConfig.prefix_sei_nal_file_name, "rb")) == NULL) {
                VLOG(ERR, "fail to open Prefix SEI NAL Data file, %s\n", ctx->testEncConfig.prefix_sei_nal_file_name);
                return FALSE;
            }
            osal_fseek(ctx->testEncConfig.prefix_sei_nal_fp , 0, SEEK_END);
            ctx->testEncConfig.prefix_sei_file_max_size = osal_ftell(ctx->testEncConfig.prefix_sei_nal_fp);
            osal_fseek(ctx->testEncConfig.prefix_sei_nal_fp, 0, SEEK_SET);
        }

        for (idx = 0; idx < srcFbAllocInfo.num; idx++) {         // the number of roi buffer should be the same as source buffer num.
            ctx->vbPrefixSeiNal[idx].size = SEI_NAL_DATA_BUF_SIZE;
            if (vdi_allocate_dma_memory(ctx->testEncConfig.coreIdx, &ctx->vbPrefixSeiNal[idx], ENC_ETC, ctx->handle->instIndex) < 0) {
                VLOG(ERR, "fail to allocate ROI buffer\n");
                return FALSE;
            }
        }
    }

    if (ctx->testEncConfig.seiDataEnc.suffixSeiNalEnable) {
        if (ctx->testEncConfig.suffix_sei_nal_file_name[0]) {
            ChangePathStyle(ctx->testEncConfig.suffix_sei_nal_file_name);
            if ((ctx->testEncConfig.suffix_sei_nal_fp = osal_fopen(ctx->testEncConfig.suffix_sei_nal_file_name, "rb")) == NULL) {
                VLOG(ERR, "fail to open Prefix SEI NAL Data file, %s\n", ctx->testEncConfig.suffix_sei_nal_file_name);
                return FALSE;
            }

            osal_fseek(ctx->testEncConfig.suffix_sei_nal_fp, 0, SEEK_END);
            ctx->testEncConfig.suffix_sei_file_max_size = osal_ftell(ctx->testEncConfig.suffix_sei_nal_fp);
            osal_fseek(ctx->testEncConfig.suffix_sei_nal_fp, 0, SEEK_SET);
        }

        for (idx = 0; idx < srcFbAllocInfo.num; idx++) {         // the number of roi buffer should be the same as source buffer num.
            ctx->vbSuffixSeiNal[idx].size = SEI_NAL_DATA_BUF_SIZE;
            if (vdi_allocate_dma_memory(ctx->testEncConfig.coreIdx, &ctx->vbSuffixSeiNal[idx], ENC_ETC, ctx->handle->instIndex) < 0) {
                VLOG(ERR, "fail to allocate ROI buffer\n");
                return FALSE;
            }
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
    ParamVpuStatus*             status;
    QueueStatusInfo             cqInfo;

    switch(commandType) {
    case GET_PARAM_COM_IS_CONTAINER_CONUSUMED:
        container = (PortContainerYuv*)data;
        if ( ctx->subFrameSyncCfg.subFrameSyncOn == TRUE) {
            container->consumed = TRUE;
        }
        else {
            if (ctx->encodedSrcFrmIdxArr[container->srcFbIndex]) {
                ctx->encodedSrcFrmIdxArr[container->srcFbIndex] = 0;
                container->consumed = TRUE;
            }
        }
        break;
    case GET_PARAM_SRC_FRAME_INFO:
        container = (PortContainerYuv*)data;
        if ( container->type == SUB_FRAME_SYNC_SRC_ADDR_SEND) {
            container->srcCanBeWritten = ctx->encodedSrcFrmIdxArr[container->srcFbIndex];
            break;
        }
        //if ( container->type == SUB_FRAME_SYNC_DATA_FEED) {
        container->srcCanBeWritten = ctx->encodedSrcFrmIdxArr[container->srcFbIndex];
        ctx->encodedSrcFrmIdxArr[container->srcFbIndex] = 0;
        //}
        break;
    case GET_PARAM_ENC_HANDLE:
        if (ctx->handle == NULL) return CNM_COMPONENT_PARAM_NOT_READY;
        *(EncHandle*)data = ctx->handle;
        break;
    case GET_PARAM_ENC_FRAME_BUF_NUM:
        if (ctx->fbCountValid == FALSE) return CNM_COMPONENT_PARAM_NOT_READY;
        fbCount = (ParamEncNeedFrameBufferNum*)data;
        fbCount->reconFbNum = ctx->fbCount.reconFbNum;
        fbCount->srcFbNum   = ctx->fbCount.srcFbNum;
        break;
    case GET_PARAM_ENC_FRAME_BUF_REGISTERED:
        if (ctx->state <= ENCODER_STATE_REGISTER_FB) return CNM_COMPONENT_PARAM_NOT_READY;
        *(BOOL*)data = TRUE;
        break;
    case GET_PARAM_VPU_STATUS:
        if (ctx->state != ENCODER_STATE_ENCODING) return CNM_COMPONENT_PARAM_NOT_READY;
        VPU_EncGiveCommand(ctx->handle, ENC_GET_QUEUE_STATUS, &cqInfo);
        status = (ParamVpuStatus*)data;
        status->cq = cqInfo;
        break;
    default:
        result = FALSE;
        break;
    }

    return (result == TRUE) ? CNM_COMPONENT_PARAM_SUCCESS : CNM_COMPONENT_PARAM_FAILURE;
}

static CNMComponentParamRet SetParameterEncoder(ComponentImpl* from, ComponentImpl* com, SetParameterCMD commandType, void* data)
{
    BOOL                    result = TRUE;

    switch (commandType) {
    case SET_PARAM_COM_PAUSE:
        com->pause   = *(BOOL*)data;
        break;
    default:
        VLOG(ERR, "Unknown SetParameterCMD Type : %d\n", commandType);
        result = FALSE;
        break;
    }

    return (result == TRUE) ? CNM_COMPONENT_PARAM_SUCCESS : CNM_COMPONENT_PARAM_FAILURE;
}

static ENC_INT_STATUS HandlingInterruptFlag(ComponentImpl* com)
{
    EncoderContext*   ctx               = (EncoderContext*)com->context;
    EncHandle       handle                = ctx->handle;
    Int32           interruptFlag         = 0;
    Uint32          interruptWaitTime     = VPU_WAIT_TIME_OUT_CQ;
    Uint32          interruptTimeout      = VPU_ENC_TIMEOUT;
    ENC_INT_STATUS  status                = ENC_INT_STATUS_NONE;

    if (com->thread != NULL) {
        if (1 < vdi_get_instance_num(VPU_HANDLE_CORE_INDEX(ctx->handle))) {
            interruptWaitTime = VPU_WAIT_TIME_OUT_LONG;
        }
    }
    if (ctx->attr.supportCommandQueue== FALSE)
        interruptWaitTime = interruptTimeout;


    if (ctx->startTimeout == 0ULL) {
        ctx->startTimeout = osal_gettime();
    }
    do {
        interruptFlag = VPU_WaitInterruptEx(handle, interruptWaitTime);
        if (INTERRUPT_TIMEOUT_VALUE == interruptFlag) {
            Uint64   currentTimeout = osal_gettime();

            if ((currentTimeout - ctx->startTimeout) >= interruptTimeout) {
                VLOG(ERR, "<%s:%d> hangup Timeout = %lldms\n", __FUNCTION__, __LINE__, interruptTimeout);
                CNMErrorSet(CNM_ERROR_HANGUP);
                status = ENC_INT_STATUS_TIMEOUT;
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

            if (interruptFlag & (1<<INT_WAVE5_ENC_SET_PARAM)) {
                status = ENC_INT_STATUS_DONE;
                break;
            }

            if (interruptFlag & (1<<INT_WAVE5_ENC_PIC)) {
                status = ENC_INT_STATUS_DONE;
                break;
            }

            if (interruptFlag & (1<<INT_WAVE5_BSBUF_FULL)) {
                status = ENC_INT_STATUS_FULL;
                break;
            }

#ifdef SUPPORT_SOURCE_RELEASE_INTERRUPT
            if (interruptFlag & (1 << INT_WAVE5_ENC_SRC_RELEASE)) {
                status = ENC_INT_STATUS_SRC_RELEASED;
            }
#endif
        }
    } while (FALSE);

    return status;
}

static BOOL SetSequenceInfo(ComponentImpl* com)
{
    EncoderContext* ctx = (EncoderContext*)com->context;
    EncHandle       handle  = ctx->handle;
    RetCode         ret     = RETCODE_SUCCESS;
    ENC_INT_STATUS  status;
    EncInitialInfo* initialInfo = &ctx->initialInfo;
    CNMComListenerEncCompleteSeq lsnpCompleteSeq   = {0};

    if (ctx->stateDoing == FALSE) {
        do {
            ret = VPU_EncIssueSeqInit(handle);
        } while (ret == RETCODE_QUEUEING_FAILURE && com->terminate == FALSE);

        if (ret != RETCODE_SUCCESS) {
            VLOG(ERR, "%s:%d Failed to VPU_EncIssueSeqInit() ret(%d)\n", __FUNCTION__, __LINE__, ret);
            ChekcAndPrintDebugInfo(ctx->handle, TRUE, ret);
            LeaveLock(ctx->handle->coreIdx);
            return FALSE;
        }
        ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_ISSUE_SEQ, NULL);
    }
    ctx->stateDoing = TRUE;

    while (com->terminate == FALSE) {
        if ((status=HandlingInterruptFlag(com)) == ENC_INT_STATUS_DONE) {
            break;
        }
        else if (status == ENC_INT_STATUS_NONE) {
            return TRUE;
        }
        else if (status == ENC_INT_STATUS_TIMEOUT) {
            VLOG(INFO, "%s:%d INSTANCE #%d INTERRUPT TIMEOUT\n", __FUNCTION__, __LINE__, handle->instIndex);
            HandleEncoderError(ctx->handle, ctx->frameIdx, NULL);
            LeaveLock(ctx->handle->coreIdx);
            return FALSE;
        }
        else {
            VLOG(INFO, "%s:%d Unknown interrupt status: %d\n", __FUNCTION__, __LINE__, status);
            LeaveLock(ctx->handle->coreIdx);
            return FALSE;
        }
    }

    if ((ret=VPU_EncCompleteSeqInit(handle, initialInfo)) != RETCODE_SUCCESS) {
        VLOG(ERR, "%s:%d FAILED TO VPU_EncCompleteSeqInit: ret(%d), SEQERR(%08x)\n",
            __FUNCTION__, __LINE__, ret, initialInfo->seqInitErrReason);
        ChekcAndPrintDebugInfo(ctx->handle, TRUE, ret);
        return FALSE;
    }

    lsnpCompleteSeq.handle = handle;
    ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_COMPLETE_SEQ, (void*)&lsnpCompleteSeq);

    ctx->fbCount.reconFbNum   = initialInfo->minFrameBufferCount;
    if (ctx->attr.supportCommandQueue == FALSE) {
        ctx->fbCount.srcFbNum     = initialInfo->minSrcFrameCount + EXTRA_SRC_BUFFER_NUM;
    }
    else {
        ctx->fbCount.srcFbNum     = initialInfo->minSrcFrameCount + COMMAND_QUEUE_DEPTH + EXTRA_SRC_BUFFER_NUM;
    }

    if ( ctx->encOpenParam.sourceBufCount > ctx->fbCount.srcFbNum)
        ctx->fbCount.srcFbNum = ctx->encOpenParam.sourceBufCount;


    if (ctx->subFrameSyncCfg.subFrameSyncOn == TRUE) {
        if (ctx->fbCount.srcFbNum > 5) {
            ctx->fbCount.srcFbNum = 5;
            VLOG(TRACE, "Set src frame buffer number to 5 if subFrameSync is enabled(constraint)\n");
        }
    }
    ctx->fbCountValid = TRUE;

    VLOG(INFO, "[ENCODER] Required  reconFbCount=%d, srcFbCount=%d, outNum=%d, size=%dx%d\n",
        ctx->fbCount.reconFbNum, ctx->fbCount.srcFbNum, ctx->testEncConfig.outNum, ctx->encOpenParam.picWidth, ctx->encOpenParam.picHeight);
    ctx->stateDoing = FALSE;

    return TRUE;
}

static BOOL EncodeHeader(ComponentImpl* com, PortContainerES* out)
{
    EncoderContext*         ctx = (EncoderContext*)com->context;
    RetCode                 ret     = RETCODE_SUCCESS;
    EncHeaderParam          encHeaderParam;
    vpu_buffer_t*           buf;
    Uint32                  productID = ctx->attr.productId;

    buf = Queue_Dequeue(ctx->encOutQ);
    osal_memset(&encHeaderParam, 0x00, sizeof(EncHeaderParam));

    if(PRODUCT_ID_W5_SERIES(productID)) {
        encHeaderParam.encodeAUD = ctx->testEncConfig.encAUD;
    }
    encHeaderParam.buf       = buf->phys_addr;
    encHeaderParam.size      = buf->size;

    if (ctx->encOpenParam.bitstreamFormat == STD_HEVC) {
        encHeaderParam.headerType = CODEOPT_ENC_VPS | CODEOPT_ENC_SPS | CODEOPT_ENC_PPS;
    } else {
        // H.264, AV1
        encHeaderParam.headerType = CODEOPT_ENC_SPS | CODEOPT_ENC_PPS;
    }

    ret = VPU_EncGiveCommand(ctx->handle, ENC_PUT_VIDEO_HEADER, &encHeaderParam);

    if (ctx->attr.supportCommandQueue == FALSE) {
        if ( ret != RETCODE_SUCCESS ) {
            VLOG(ERR, "%s:%d ENC_PUT_VIDEO_HEADER, Error code : %d \n", __FUNCTION__, __LINE__, ret);
            return FALSE;
        }
    } else {
        while(ret == RETCODE_QUEUEING_FAILURE) {
            ret = VPU_EncGiveCommand(ctx->handle, ENC_PUT_VIDEO_HEADER, &encHeaderParam);
        }
    }
    if(PRODUCT_ID_W6_SERIES(productID)) { //if no CQ
#ifdef SUPPORT_READ_BITSTREAM_IN_ENCODER
        ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_HEADER, ctx->bsReader);
#else
        out->buf.phys_addr = buf->phys_addr;
        out->buf.size = buf->size;
        out->size = encHeaderParam.size;
        out->reuse = FALSE;
#endif
        VLOG(INFO, "header only encoding rdPtr=0x%x, wrPtr=0x%x, size = %d(0x%x)\n", buf->phys_addr, buf->phys_addr + encHeaderParam.size, encHeaderParam.size, encHeaderParam.size);
    }

    DisplayEncodedInformation(ctx->handle, ctx->encOpenParam.bitstreamFormat, 0, NULL, 0, 0, ctx->testEncConfig.performance);
    return TRUE;
}

static BOOL Encode(ComponentImpl* com, PortContainerYuv* in, PortContainerES* out)
{
    EncoderContext*                 ctx             = (EncoderContext*)com->context;
    BOOL                            doEncode        = FALSE;
    EncParam*                       encParam        = &ctx->encParam;
    EncOutputInfo                   encOutputInfo;
    ENC_INT_STATUS                  intStatus;
    RetCode                         result;
    CNMComListenerHandlingInt       lsnpHandlingInt   = {0};
    CNMComListenerEncDone           lsnpPicDone     = {0};
    CNMComListenerEncReadyOneFrame  lsnpReadyOneFrame = {0,};
    ENC_QUERY_WRPTR_SEL             encWrPtrSel     = GET_ENC_PIC_DONE_WRPTR;
    QueueStatusInfo                 qStatus;
    CNMComListenerEncStartOneFrame  lsn;

    //Encode a codec header
    if(!ctx->hdr_enc_flag) {
        BOOL    ret;

        if(in) {
            in->reuse = TRUE;
            in->consumed = FALSE;
        }

        ret = EncodeHeader(com, out);
        if(!ret) {
            return TRUE;
        }

        ctx->hdr_enc_flag = TRUE;
        return TRUE;
    }

    lsnpReadyOneFrame.handle = ctx->handle;
    ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_READY_ONE_FRAME, &lsnpReadyOneFrame);

    ctx->stateDoing = TRUE;
    if (in && in->type == SUB_FRAME_SYNC_DATA_FEED) {
        in->reuse = FALSE;
        in->consumed = TRUE;
        return TRUE;
    }

    if (out)
    {
        if (out->buf.phys_addr != 0)
        {
            Queue_Enqueue(ctx->encOutQ, (void*)&out->buf);
            out->buf.phys_addr = 0;
            out->buf.size      = 0;
        }
        if (encParam->srcEndFlag == TRUE)
        {
            doEncode = (BOOL)(Queue_Get_Cnt(ctx->encOutQ) > 0);
            if(doEncode == TRUE)
            {
                vpu_buffer_t*      buf = (vpu_buffer_t*)Queue_Peek(ctx->encOutQ);
                encParam->picStreamBufferAddr = buf->phys_addr;
                encParam->picStreamBufferSize = buf->size;
                encParam->srcIdx = -1;
            }
        }
    }

    VPU_EncGiveCommand(ctx->handle, ENC_GET_QUEUE_STATUS, &qStatus);

    if ((qStatus.instanceQueueCount == COMMAND_QUEUE_DEPTH) || (qStatus.instanceQueueFull == 1) || (com->pause == TRUE))
    {
        doEncode = FALSE;
    }
    else
    {
        if (in)
        {
            if (Queue_Get_Cnt(ctx->encOutQ) > 0)
            {
                SetEncPicParam(com, in, encParam);
                doEncode = TRUE;
                in->prevDataReuse = TRUE;
            }
        }
    }
    if ( ctx->srcEndFlag == TRUE) {
        encParam->srcEndFlag = TRUE;
    }

    if ((ctx->testEncConfig.numChangeParam > ctx->changedCount) &&
        (ctx->testEncConfig.changeParam[ctx->changedCount].setParaChgFrmNum == ctx->frameIdx))
    {
        if(PRODUCT_ID_W6_SERIES(ctx->testEncConfig.productId)) {        //WAVE6
            result = W6SetChangeParam(ctx->handle, &ctx->testEncConfig, &ctx->encOpenParam, ctx->changedCount, &ctx->vbHrdRbsp, &ctx->vbVuiRbsp);
        } else {                                                        //WAVE5 + CODA Series
            result = SetChangeParam(ctx->handle, ctx->testEncConfig, ctx->encOpenParam, ctx->changedCount);
        }

        if (result == RETCODE_SUCCESS) {
            VLOG(TRACE, "ENC_SET_PARA_CHANGE queue success\n");
            ctx->changedCount++;
        } else if (result == RETCODE_QUEUEING_FAILURE) { // Just retry
            VLOG(INFO, "ENC_SET_PARA_CHANGE Queue Full\n");
            doEncode  = FALSE;
        } else { // Error
            VLOG(ERR, "VPU_EncGiveCommand[ENC_SET_PARA_CHANGE] failed Error code is 0x%x \n", result);
            ChekcAndPrintDebugInfo(ctx->handle, TRUE, result);
            return FALSE;
        }
    }



    if (doEncode == TRUE) {
        result = VPU_EncStartOneFrame(ctx->handle, encParam);
        if (result == RETCODE_SUCCESS) {

            if (in) in->prevDataReuse = FALSE;
            Queue_Dequeue(ctx->encOutQ);
            ctx->frameIdx++;
            if (in) in->reuse = FALSE;
            if (out) out->reuse = FALSE;
        }
        else if (result == RETCODE_QUEUEING_FAILURE) {

        }
        else { // Error
            VLOG(ERR, "VPU_EncStartOneFrame failed Error code is 0x%x \n", result);
            ChekcAndPrintDebugInfo(ctx->handle, TRUE, result);
            CNMErrorSet(CNM_ERROR_HANGUP);
            HandleEncoderError(ctx->handle, ctx->frameIdx, NULL);
            return FALSE;
        }
        lsn.handle = ctx->handle;
        lsn.result = result;
        ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_START_ONE_FRAME, (void*)&lsn);
    }
    else
    {
        if (ctx->attr.supportCommandQueue == FALSE) {
            if (FALSE == ctx->fullInterrupt) {
                if ( out )
                    out->reuse = FALSE;
                return TRUE;
            }
        }
    }

    if ((intStatus=HandlingInterruptFlag(com)) == ENC_INT_STATUS_TIMEOUT) {
        ChekcAndPrintDebugInfo(ctx->handle, TRUE, RETCODE_VPU_RESPONSE_TIMEOUT);
        HandleEncoderError(ctx->handle, ctx->frameIdx, NULL);
        VPU_SWReset(ctx->testEncConfig.coreIdx, SW_RESET_SAFETY, ctx->handle);
        LeaveLock(ctx->testEncConfig.coreIdx);
        return FALSE;
    }
    else if (intStatus == ENC_INT_STATUS_FULL || intStatus == ENC_INT_STATUS_LOW_LATENCY) {
        CNMComListenerEncFull   lsnpFull;
        PhysicalAddress         paRdPtr;
        PhysicalAddress         paWrPtr;
        int                     size;

        if (PRODUCT_ID_W6_SERIES(ctx->testEncConfig.productId)) {
            vpu_buffer_t    vb;

            encWrPtrSel = GET_ENC_BSBUF_FULL_WRPTR;
            VPU_EncGiveCommand(ctx->handle, ENC_WRPTR_SEL, &encWrPtrSel);
            VPU_EncGetBitstreamBuffer(ctx->handle, &paRdPtr, &paWrPtr, &size);
            VLOG(TRACE, "<%s:%d> INT_BSBUF_FULL inst=%d, rdptr=%p, wrptr=%p, size=%x\n", __FUNCTION__, __LINE__, ctx->handle->instIndex, paRdPtr, paWrPtr, size);
            vb.size = ctx->encOpenParam.bitstreamBufferSize;
            if (vdi_allocate_dma_memory(ctx->testEncConfig.coreIdx, &vb, ENC_BS, ctx->handle->instIndex) < 0 ) {
                VLOG(ERR, "fail to allocate BS buffer\n");
                return FALSE;
            }
            if ( out )
                osal_memcpy(&out->newBsBuf, &vb, sizeof(vb));
        }
        else {
            encWrPtrSel = (intStatus==ENC_INT_STATUS_FULL) ? GET_ENC_BSBUF_FULL_WRPTR : GET_ENC_LOW_LATENCY_WRPTR;
            VPU_EncGiveCommand(ctx->handle, ENC_WRPTR_SEL, &encWrPtrSel);
            VPU_EncGetBitstreamBuffer(ctx->handle, &paRdPtr, &paWrPtr, &size);
            VLOG(TRACE, "<%s:%d> INT_BSBUF_FULL inst=%d, %p, %p\n", __FUNCTION__, __LINE__, ctx->handle->instIndex, paRdPtr, paWrPtr);

        }

        lsnpFull.handle         = ctx->handle;
#ifdef SUPPORT_READ_BITSTREAM_IN_ENCODER
        lsnpFull.bsReader       = ctx->bsReader;
        lsnpFull.rdPtr          = paRdPtr;
        lsnpFull.wrPtr          = paWrPtr;
        lsnpFull.size           = size;
        lsnpFull.paBsBufStart   = paRdPtr;
        lsnpFull.paBsBufEnd     = paRdPtr + size;
        lsnpFull.fp             = ctx->bs_fp;
        lsnpFull.vbStream.phys_addr = paRdPtr;
        lsnpFull.vbStream.virt_addr = paRdPtr;
        lsnpFull.vbStream.size  = size;
#endif
        ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_FULL_INTERRUPT, (void*)&lsnpFull);

        if (PRODUCT_ID_W6_SERIES(ctx->testEncConfig.productId) && com->terminate == TRUE) {
            LeaveLock(ctx->testEncConfig.coreIdx);
        }
        if ( out ) {
            if (ctx->encOpenParam.ringBufferEnable ==  TRUE) {
                out->buf.phys_addr = paRdPtr;
                out->buf.size = size;
                out->size  = size;
                out->reuse = FALSE;
                out->streamBufFull = TRUE;
                out->rdPtr = paRdPtr;
                out->wrPtr = paWrPtr;
                out->paBsBufStart = ctx->encOpenParam.bitstreamBuffer;
                out->paBsBufEnd = ctx->encOpenParam.bitstreamBuffer + ctx->encOpenParam.bitstreamBufferSize;
            }
            else {
                if (FindEsBuffer(ctx, paRdPtr, &out->buf) == FALSE) {
                    VLOG(ERR, "%s:%d Failed to find buffer(%p)\n", __FUNCTION__, __LINE__, paRdPtr);
                    if (PRODUCT_ID_W6_SERIES(ctx->testEncConfig.productId)) {
                        LeaveLock(ctx->testEncConfig.coreIdx);
                    }
                    return FALSE;
                }
                out->size  = size;
                out->reuse = FALSE;
                out->streamBufFull = TRUE;
            }
        }
        ctx->fullInterrupt = TRUE;
        return TRUE;
    }
#ifdef SUPPORT_SOURCE_RELEASE_INTERRUPT
    else if (intStatus == ENC_INT_STATUS_SRC_RELEASED) {
        Uint32 srcBufFlag = 0;
        Uint32 i = 0;
        VPU_EncGiveCommand(ctx->handle, ENC_GET_SRC_BUF_FLAG, &srcBufFlag);
        for (i = 0; i < ctx->fbCount.srcFbNum; i++) {
            if ( (srcBufFlag >> i) & 0x01) {
                ctx->encodedSrcFrmIdxArr[i] = 1;
            }
        }
        return TRUE;
    }
#endif
    else if (intStatus == ENC_INT_STATUS_NONE) {
        if (out) {
            out->size  = 0;
            out->reuse = FALSE;
        }
        return TRUE; /* Try again */
    }
    ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_HANDLING_INT, (void*)&lsnpHandlingInt);

    VPU_EncGiveCommand(ctx->handle, ENC_WRPTR_SEL, &encWrPtrSel);

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
        VLOG(ERR, "Failed to encode error = %d, %x\n", result, encOutputInfo.errorReason);
        ChekcAndPrintDebugInfo(ctx->handle, TRUE, result);
        HandleEncoderError(ctx->handle, encOutputInfo.encPicCnt, &encOutputInfo);
        VPU_SWReset(ctx->testEncConfig.coreIdx, SW_RESET_SAFETY, ctx->handle);
        return FALSE;
    }
    else {
        ;/* SUCCESS */
    }

    if (encOutputInfo.reconFrameIndex == RECON_IDX_FLAG_CHANGE_PARAM) {
        VLOG(TRACE, "CHANGE PARAMETER!\n");
        return TRUE; /* Try again */
    }
    else {
        DisplayEncodedInformation(ctx->handle, ctx->encOpenParam.bitstreamFormat, ctx->frameIdx, &encOutputInfo, encParam->srcEndFlag, encParam->srcIdx, ctx->testEncConfig.performance);
    }

    if ( encOutputInfo.warnInfo & WAVE5_ETCWARN_FORCED_SPLIT_BY_CU8X8 )
        VLOG(TRACE, "WAVE5_ETCWARN_FORCED_SPLIT_BY_CU8X8\n");

    if (encOutputInfo.bitstreamSize > 0) {
        if (ctx->testEncConfig.enReportNalSize) {
            SaveEncNalSizeReport(ctx->handle, ctx->testEncConfig.stdMode,  ctx->testEncConfig.nalSizeDataPath, encOutputInfo, encOutputInfo.encPicCnt);
        }
    }

    lsnpPicDone.handle = ctx->handle;
    lsnpPicDone.ret = result;
    lsnpPicDone.output = &encOutputInfo;
    lsnpPicDone.fullInterrupted = ctx->fullInterrupt;
#ifdef SUPPORT_READ_BITSTREAM_IN_ENCODER
    lsnpPicDone.fp        = ctx->bs_fp;
    lsnpPicDone.bsReader  = ctx->bsReader;
    lsnpPicDone.bitstreamBuffer = ctx->encOpenParam.bitstreamBuffer;
    lsnpPicDone.bitstreamBufferSize = ctx->encOpenParam.bitstreamBufferSize;
#endif
    ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_GET_OUTPUT_INFO, (void*)&lsnpPicDone);

    if (result != RETCODE_SUCCESS )
        return FALSE;

    if (PRODUCT_ID_W5_SERIES(ctx->testEncConfig.productId) && (ctx->attr.supportCommandQueue == TRUE)) {
        Uint32 i;
        for (i = 0; i < ctx->fbCount.srcFbNum; i++) {
            if ((encOutputInfo.releaseSrcFlag >> i) & 0x01) {
                ctx->encodedSrcFrmIdxArr[i] = 1;
            }
        }
    } else {
        if (encOutputInfo.encSrcIdx >= 0)
            ctx->encodedSrcFrmIdxArr[encOutputInfo.encSrcIdx] = 1;
    }

    ctx->fullInterrupt      = FALSE;

    if ( out ) {
        if (ctx->encOpenParam.ringBufferEnable ==  TRUE) {
            out->buf.phys_addr = encOutputInfo.rdPtr;
            out->size  = encOutputInfo.bitstreamSize;
        } else {
            if (FindEsBuffer(ctx, encOutputInfo.bitstreamBuffer, &out->buf) == FALSE) {
                VLOG(ERR, "%s:%d Failed to find buffer(%p)\n", __FUNCTION__, __LINE__, encOutputInfo.bitstreamBuffer);
                return FALSE;
            }
            out->size  = encOutputInfo.bitstreamSize;
        }
        out->reuse = (BOOL)(out->size == 0);
    }

    // Finished encoding a frame
    if ((encOutputInfo.reconFrameIndex == RECON_IDX_FLAG_ENC_END) || (ctx->checkEncodedCnt && (ctx->frameOutNum <= encOutputInfo.encPicCnt)) ) {
        if (ctx->testEncConfig.outNum != encOutputInfo.encPicCnt && ctx->testEncConfig.outNum != -1) {
            VLOG(ERR, "outnum(%d) != encoded cnt(%d)\n", ctx->testEncConfig.outNum, encOutputInfo.encPicCnt);
            return FALSE;
        }
        if(out) out->last  = TRUE;  // Send finish signal
        if(out) out->reuse = FALSE;
        ctx->stateDoing    = FALSE;
        com->terminate     = TRUE;

        //to read remain data
        if (ctx->encOpenParam.ringBufferEnable ==  TRUE && ctx->encOpenParam.ringBufferWrapEnable == FALSE) {
            if(out) out->rdPtr = encOutputInfo.rdPtr;
            if(out) out->wrPtr = encOutputInfo.wrPtr;
            if(out) out->size = encOutputInfo.wrPtr - encOutputInfo.rdPtr;
        }
    }


    return TRUE;
}

static BOOL AllocateCustomBuffer(ComponentImpl* com)
{
    EncoderContext* ctx       = (EncoderContext*)com->context;
    TestEncConfig testEncConfig = ctx->testEncConfig;
    EncOpenParam*   pOpenParam;
    pOpenParam = &ctx->encOpenParam;

    /* Allocate Buffer and Set Data */
    if(PRODUCT_ID_W6_SERIES(testEncConfig.productId)) {
        EncWave6Param*  pParam;
        pParam = &pOpenParam->EncStdParam.wave6Param;
        if (pParam->enCustomLambda) {
            int i, j = 0;
            char lineStr[256] = {0, };

            for(i = 0; i < 104; i++) {
                if( NULL == fgets(lineStr, 256, testEncConfig.custom_lambda_file) ) {
                    VLOG(ERR,"Error: can't read custom_lambda\n");
                    return 0;
                } else {
                    if ( i < 52 ) {
                        sscanf(lineStr, "%d\n", &pParam->customLambdaSSD[j++]);
                    } else {
                        if (i == 52)
                            j=0;
                        sscanf(lineStr, "%d\n", &pParam->customLambdaSAD[j++]);
                    }
                }
            }
        }
    }
    if(PRODUCT_ID_W5_SERIES(testEncConfig.productId)) {
        EncWave5Param*  pParam;
        pParam = &pOpenParam->EncStdParam.waveParam;
        if (pParam->scalingListEnable) {
            ctx->vbScalingList.size = 0x1000;
            if (vdi_allocate_dma_memory(testEncConfig.coreIdx, &ctx->vbScalingList, ENC_ETC, 0) < 0) {
                VLOG(ERR, "fail to allocate scaling list buffer\n");
                return FALSE;
            }
            pParam->userScalingListAddr = ctx->vbScalingList.phys_addr;

            parse_user_scaling_list(&ctx->scalingList, testEncConfig.scaling_list_file, testEncConfig.stdMode);
            vdi_write_memory(testEncConfig.coreIdx, ctx->vbScalingList.phys_addr, (unsigned char*)&ctx->scalingList, ctx->vbScalingList.size, VDI_LITTLE_ENDIAN);
        }

        if (pParam->customLambdaEnable) {
            ctx->vbCustomLambda.size = 0x200;
            if (vdi_allocate_dma_memory(testEncConfig.coreIdx, &ctx->vbCustomLambda, ENC_ETC, 0) < 0) {
                VLOG(ERR, "fail to allocate Lambda map buffer\n");
                return FALSE;
            }
            pParam->customLambdaAddr = ctx->vbCustomLambda.phys_addr;

            parse_custom_lambda(ctx->customLambda, testEncConfig.custom_lambda_file);
            vdi_write_memory(testEncConfig.coreIdx, ctx->vbCustomLambda.phys_addr, (unsigned char*)&ctx->customLambda[0], ctx->vbCustomLambda.size, VDI_LITTLE_ENDIAN);
        }

        if ( testEncConfig.enReportNalSize ) {
            ctx->vbNalSizeReportData.size = NAL_SIZE_REPORT_SIZE;
            ctx->vbNalSizeReportData.size *= REPORT_QUEUE_COUNT;
            if (vdi_allocate_dma_memory(testEncConfig.coreIdx, &ctx->vbNalSizeReportData, ENC_ETC, 0) < 0) {
                VLOG(ERR, "fail to allocate Report buffer\n");
                return FALSE;
            }
            pParam->reportNalSizeAddr  = ctx->vbNalSizeReportData.phys_addr;
            pParam->enReportNalSize = TRUE;
        }
    }

    if(PRODUCT_ID_W_SERIES(testEncConfig.productId)) {
        if (writeVuiRbsp(ctx->testEncConfig.coreIdx, &ctx->testEncConfig, pOpenParam, &ctx->vbVuiRbsp) == FALSE)
            return FALSE;
        if (writeHrdRbsp(ctx->testEncConfig.coreIdx, &ctx->testEncConfig, pOpenParam, &ctx->vbHrdRbsp) == FALSE)
            return FALSE;
    }
    return TRUE;
}

static BOOL OpenEncoder(ComponentImpl* com)
{
    EncoderContext*         ctx = (EncoderContext*)com->context;
    SecAxiUse               secAxiUse;
    MirrorDirection         mirrorDirection;
    RetCode                 ret_code;
    CNMComListenerEncOpen   lspn    = {0};

    ctx->stateDoing = TRUE;

    ctx->encOpenParam.bitstreamBuffer     = ctx->bsBuf.bs[0].phys_addr;
    ctx->encOpenParam.bitstreamBufferSize = ctx->bsBuf.bs[0].size;
    if (AllocateCustomBuffer(com) == FALSE)
        return FALSE;

    if (PRODUCT_ID_W6_SERIES(ctx->testEncConfig.productId)) {
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
        if (AllocateArTableBuffer(com) != TRUE) {
            VLOG(ERR, "<%s:%d> Failed to AllocateArTableBuffer()\n", __FUNCTION__, __LINE__);
            return FALSE;
        }
    }

    if ((ret_code = VPU_EncOpen(&ctx->handle, &ctx->encOpenParam)) != RETCODE_SUCCESS) {
        VLOG(ERR, "VPU_EncOpen failed Error code is 0x%x \n", ret_code);
        if ( ret_code == RETCODE_VPU_RESPONSE_TIMEOUT ) {
            CNMErrorSet(CNM_ERROR_HANGUP);
        }
        CNMAppStop();
        return FALSE;
    }
#if defined(SUPPORT_MULTI_INSTANCE_TEST) && defined(SUPPORT_READ_BITSTREAM_IN_ENCODER)
    VLOG(TRACE, "enc instIdx = %d, CFG=%s, bitstream=%s\n", ctx->handle->instIndex, ctx->testEncConfig.cfgFileName, ctx->bitstreamFileName);
#endif
    //VPU_EncGiveCommand(ctx->handle, ENABLE_LOGGING, 0);

    lspn.handle = ctx->handle;
    ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_OPEN, (void*)&lspn);

    if (ctx->testEncConfig.rotAngle != 0 || ctx->testEncConfig.mirDir != 0) {
        VPU_EncGiveCommand(ctx->handle, ENABLE_ROTATION, 0);
        VPU_EncGiveCommand(ctx->handle, ENABLE_MIRRORING, 0);
        VPU_EncGiveCommand(ctx->handle, SET_ROTATION_ANGLE, &ctx->testEncConfig.rotAngle);
        mirrorDirection = (MirrorDirection)ctx->testEncConfig.mirDir;
        VPU_EncGiveCommand(ctx->handle, SET_MIRROR_DIRECTION, &mirrorDirection);
    }
    osal_memset(&secAxiUse,   0x00, sizeof(SecAxiUse));
    secAxiUse.u.wave.useEncRdoEnable = (ctx->testEncConfig.secondaryAXI & 0x1) ? TRUE : FALSE;
    secAxiUse.u.wave.useEncLfEnable  = (ctx->testEncConfig.secondaryAXI & 0x2) ? TRUE : FALSE;

    VPU_EncGiveCommand(ctx->handle, SET_SEC_AXI, &secAxiUse);
    VPU_EncGiveCommand(ctx->handle, SET_CYCLE_PER_TICK,   (void*)&ctx->cyclePerTick);

    ctx->subFrameSyncCfg.subFrameSyncOn = ctx->testEncConfig.subFrameSyncEn;
    ctx->subFrameSyncCfg.subFrameSyncMode = ctx->testEncConfig.subFrameSyncMode;
    ctx->subFrameSyncCfg.subFrameSyncFbCount = ctx->testEncConfig.subFrameSyncFbCount;
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
        ((EncoderContext*)com->context)->hdr_enc_flag = FALSE;
        ret = Encode(com, (PortContainerYuv*)in, (PortContainerES*)out);
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
    EncoderContext*      ctx       = (EncoderContext*)com->context;
    TestEncConfig*       testEncConfig = &ctx->testEncConfig;
    CNMComponentParamRet ret;
    BOOL                 success;
    Uint32               i;

    *done = FALSE;

    ret = ComponentGetParameter(com, com->sinkPort.connectedComponent, GET_PARAM_READER_BITSTREAM_BUF, &ctx->bsBuf);
    if (ComponentParamReturnTest(ret, &success) == FALSE) return success;

    if (ctx->encOpenParam.ringBufferEnable ==  TRUE) {
        ctx->encOutQ = Queue_Create(com->numSinkPortQueue, sizeof(vpu_buffer_t));
        for (i=0; i<com->numSinkPortQueue; i++) {
            if ( i < ctx->bsBuf.num) {
                ctx->bsBuffer[i] = &ctx->bsBuf.bs[i]; }
            else {
                ctx->bsBuffer[i] = osal_malloc(sizeof(vpu_buffer_t));
                osal_memcpy(ctx->bsBuffer[i], ctx->bsBuffer[0], sizeof(vpu_buffer_t));
            }

            Queue_Enqueue(ctx->encOutQ, (void*)ctx->bsBuffer[i]);//same addr enqueue
        }
    } else {
        ctx->encOutQ = Queue_Create(ctx->bsBuf.num, sizeof(vpu_buffer_t));
        for (i=0; i<ctx->bsBuf.num; i++) {
            Queue_Enqueue(ctx->encOutQ, (void*)&ctx->bsBuf.bs[i]);
        }
    }

    /* Open Data File*/
    if (PRODUCT_ID_W6_SERIES(testEncConfig->productId)) {
        if (testEncConfig->roi_enable || testEncConfig->mode_map_flag) {
            if (testEncConfig->CustomMapFile) {
                ChangePathStyle(testEncConfig->CustomMapFile);
                if ((testEncConfig->CustomMapFile_fp = osal_fopen(testEncConfig->CustomMapFile, "rb")) == NULL) {
                    VLOG(ERR, "fail to open Custom Map file, %s\n", testEncConfig->CustomMapFile);
                    return FALSE;
                }
            }
        }
        if (ctx->encOpenParam.EncStdParam.wave6Param.enCustomLambda) {
            if (testEncConfig->custom_lambda_fileName) {
                ChangePathStyle(testEncConfig->custom_lambda_fileName);
                if ((testEncConfig->custom_lambda_file = osal_fopen(testEncConfig->custom_lambda_fileName, "rt")) == NULL) {
                    VLOG(ERR, "fail to open custom lambda file, %s\n", testEncConfig->custom_lambda_fileName);
                    return FALSE;
                }
            }
        }
    } else {
        if (ctx->encOpenParam.EncStdParam.waveParam.scalingListEnable) {
            if (testEncConfig->scaling_list_fileName) {
                ChangePathStyle(testEncConfig->scaling_list_fileName);
                if ((testEncConfig->scaling_list_file = osal_fopen(testEncConfig->scaling_list_fileName, "r")) == NULL) {
                    VLOG(ERR, "fail to open scaling list file, %s\n", testEncConfig->scaling_list_fileName);
                    return FALSE;
                }
            }
        }

        if (ctx->encOpenParam.EncStdParam.waveParam.customLambdaEnable) {
            if (testEncConfig->custom_lambda_fileName) {
                ChangePathStyle(testEncConfig->custom_lambda_fileName);
                if ((testEncConfig->custom_lambda_file = osal_fopen(testEncConfig->custom_lambda_fileName, "rt")) == NULL) {
                    VLOG(ERR, "fail to open custom lambda file, %s\n", testEncConfig->custom_lambda_fileName);
                    return FALSE;
                }
            }
        }

        if (testEncConfig->roi_enable) {
            if (testEncConfig->roi_file_name) {
                ChangePathStyle(testEncConfig->roi_file_name);
                if ((testEncConfig->roi_file = osal_fopen(testEncConfig->roi_file_name, "rt")) == NULL) {
                    VLOG(ERR, "fail to open ROI file, %s\n", testEncConfig->roi_file_name);
                    return FALSE;
                }
            }
        }

        if (testEncConfig->lambda_map_enable) {
            if (testEncConfig->lambda_map_fileName) {
                ChangePathStyle(testEncConfig->lambda_map_fileName);
                if ((testEncConfig->lambda_map_file = osal_fopen(testEncConfig->lambda_map_fileName, "r")) == NULL) {
                    VLOG(ERR, "fail to open lambda map file, %s\n", testEncConfig->lambda_map_fileName);
                    return FALSE;
                }
            }
        }

        if (testEncConfig->mode_map_flag) {
            if (testEncConfig->mode_map_fileName) {
                ChangePathStyle(testEncConfig->mode_map_fileName);
                if ((testEncConfig->mode_map_file = osal_fopen(testEncConfig->mode_map_fileName, "r")) == NULL) {
                    VLOG(ERR, "fail to open custom mode map file, %s\n", testEncConfig->mode_map_fileName);
                    return FALSE;
                }
            }
        }

        if (testEncConfig->wp_param_flag) {
            if (testEncConfig->wp_param_fileName) {
                ChangePathStyle(testEncConfig->wp_param_fileName);
                if ((testEncConfig->wp_param_file = osal_fopen(testEncConfig->wp_param_fileName, "r")) == NULL) {
                    VLOG(ERR, "fail to open Weight Param file, %s\n", testEncConfig->wp_param_fileName);
                    return FALSE;
                }
            }
        }
    }

#ifdef SUPPORT_READ_BITSTREAM_IN_ENCODER
    VLOG(TRACE, "SUPPORT_READ_BITSTREAM_IN_ENCODER=%s\n", ctx->bitstreamFileName);
    ctx->bsReader = BitstreamReader_Create(BUFFER_MODE_TYPE_LINEBUFFER, ctx->bitstreamFileName, ctx->testEncConfig.stream_endian, &ctx->handle);
    if (ctx->bsReader == NULL) {
        VLOG(ERR, "%s:%d Failed to BitstreamReader_Create\n", __FUNCTION__, __LINE__);
        return FALSE;
    }
    ctx->bs_fp = ((AbstractBitstreamReader*)ctx->bsReader)->fp;
#endif

    *done = TRUE;

    return TRUE;
}

static void ReleaseEncoder(ComponentImpl* com)
{
    EncoderContext* ctx = (EncoderContext*)com->context;
    com->stateDoing = FALSE;
    ctx->iterationCnt = 0;
}

static BOOL DestroyEncoder(ComponentImpl* com)
{
    EncoderContext* ctx = (EncoderContext*)com->context;
    Uint32          i   = 0;
    BOOL            success = TRUE;
    ENC_INT_STATUS  intStatus;
    Uint64          currentTime = 0;
    Uint32          timeout = ENC_DESTROY_TIME_OUT;

    if ( NULL == ctx )
        return FALSE;
    if (FALSE == com->stateDoing) {
        if (ctx && ctx->handle) {
            ctx->desStTimeout = osal_gettime();
            com->stateDoing = TRUE;
        }
    }

    while (VPU_EncClose(ctx->handle) == RETCODE_VPU_STILL_RUNNING) {
        if ((intStatus = HandlingInterruptFlag(com)) == ENC_INT_STATUS_TIMEOUT) {
            HandleEncoderError(ctx->handle, ctx->frameIdx, NULL);
            VLOG(ERR, "NO RESPONSE FROM VPU_EncClose2()\n");
            com->stateDoing = FALSE;
            success = FALSE;
            break;
        }
        else if (intStatus == ENC_INT_STATUS_DONE) {
            EncOutputInfo   outputInfo;
            VLOG(INFO, "VPU_EncClose() : CLEAR REMAIN INTERRUPT\n");
            VPU_EncGetOutputInfo(ctx->handle, &outputInfo);
            continue;
        }

        currentTime = osal_gettime();
        if ( (currentTime - ctx->desStTimeout) > timeout) {
            VLOG(ERR, "INSTANCE #%d VPU Close TIMEOUT.\n", ctx->handle->instIndex);
            com->stateDoing = FALSE;
            success = FALSE;
            break;
        }
        osal_msleep(10);
    }
    ComponentNotifyListeners(com, COMPONENT_EVENT_ENC_CLOSE, NULL);

    if ( ctx->handle )
    {
        for (i = 0; i < ctx->fbCount.srcFbNum; i++) {
            if (ctx->vbCustomMap[i].size) {
                vdi_free_dma_memory(ctx->testEncConfig.coreIdx, &ctx->vbCustomMap[i], ENC_ETC, ctx->handle->instIndex);
                ctx->vbCustomMap[i].size = 0;
                ctx->vbCustomMap[i].phys_addr = 0UL;
            }
        }
        if (ctx->vbCustomLambda.size) {
            vdi_free_dma_memory(ctx->testEncConfig.coreIdx, &ctx->vbCustomLambda, ENC_ETC, ctx->handle->instIndex);
            ctx->vbCustomLambda.size = 0;
            ctx->vbCustomLambda.phys_addr = 0UL;
        }
        if (ctx->vbScalingList.size) {
            vdi_free_dma_memory(ctx->testEncConfig.coreIdx, &ctx->vbScalingList, ENC_ETC, ctx->handle->instIndex);
            ctx->vbScalingList.size = 0;
            ctx->vbScalingList.phys_addr = 0UL;
        }
        for (i = 0; i < MAX_FRAME_NUM ; i++) {         // the number of roi buffer should be the same as source buffer num.
            if (ctx->vbPrefixSeiNal[i].size) {
                vdi_free_dma_memory(ctx->testEncConfig.coreIdx, &ctx->vbPrefixSeiNal[i], ENC_ETC, ctx->handle->instIndex);
                ctx->vbPrefixSeiNal[i].size = 0;
                ctx->vbPrefixSeiNal[i].phys_addr = 0UL;
            }
            if (ctx->vbSuffixSeiNal[i].size) {
                vdi_free_dma_memory(ctx->testEncConfig.coreIdx, &ctx->vbSuffixSeiNal[i], ENC_ETC, ctx->handle->instIndex);
                ctx->vbSuffixSeiNal[i].size = 0;
                ctx->vbSuffixSeiNal[i].phys_addr = 0UL;
            }
        }
        if (ctx->vbVuiRbsp.size) {
            vdi_free_dma_memory(ctx->testEncConfig.coreIdx, &ctx->vbVuiRbsp, ENC_ETC, ctx->handle->instIndex);
            ctx->vbVuiRbsp.size = 0;
            ctx->vbVuiRbsp.phys_addr = 0UL;
        }
        if (ctx->vbHrdRbsp.size) {
            vdi_free_dma_memory(ctx->testEncConfig.coreIdx, &ctx->vbHrdRbsp, ENC_ETC, ctx->handle->instIndex);
            ctx->vbHrdRbsp.size = 0;
            ctx->vbHrdRbsp.phys_addr = 0UL;
        }
        if ( ctx->vbNalSizeReportData.size ) {
            vdi_free_dma_memory(ctx->testEncConfig.coreIdx, &ctx->vbNalSizeReportData, ENC_ETC, ctx->handle->instIndex);
            ctx->vbNalSizeReportData.size = 0;
            ctx->vbNalSizeReportData.phys_addr = 0UL;
        }
        if (ctx->vbWork.size) {
            vdi_free_dma_memory(ctx->testEncConfig.coreIdx, &ctx->vbWork, ENC_WORK, ctx->handle->instIndex);
            ctx->vbWork.size = 0;
            ctx->vbWork.phys_addr = 0UL;
        }
        if (ctx->vbTemp.size) {
            vdi_free_dma_memory(ctx->testEncConfig.coreIdx, &ctx->vbTemp, ENC_TEMP, ctx->handle->instIndex);
            ctx->vbTemp.size = 0;
            ctx->vbTemp.phys_addr = 0UL;
        }
        if (ctx->vbAr.size) {
            vdi_free_dma_memory(ctx->testEncConfig.coreIdx, &ctx->vbAr, ENC_ETC, ctx->handle->instIndex);
            ctx->vbAr.size = 0;
            ctx->vbAr.phys_addr = 0UL;
        }

        for (i = 0; i < AUX_BUF_TYPE_MAX; i++) {
            Uint32 j;
            for (j = 0; j < ctx->fbCount.reconFbNum; j++) {
                if (ctx->vbAux[i][j].size) {
                    vdi_free_dma_memory(ctx->testEncConfig.coreIdx, &ctx->vbAux[i][j], ENC_ETC, ctx->handle->instIndex);
                    ctx->vbAux[i][j].size = 0;
                    ctx->vbAux[i][j].phys_addr = 0UL;
                }
            }
        }
    }

    if (ctx->testEncConfig.roi_file) {
        osal_fclose(ctx->testEncConfig.roi_file);
    }
    if (ctx->testEncConfig.lambda_map_file) {
        osal_fclose(ctx->testEncConfig.lambda_map_file);
    }
    if (ctx->testEncConfig.mode_map_file) {
        osal_fclose(ctx->testEncConfig.mode_map_file);
    }
    if (ctx->testEncConfig.scaling_list_file) {
        osal_fclose(ctx->testEncConfig.scaling_list_file);
    }
    if (ctx->testEncConfig.custom_lambda_file) {
        osal_fclose(ctx->testEncConfig.custom_lambda_file);
    }
    if (ctx->testEncConfig.wp_param_file) {
        osal_fclose(ctx->testEncConfig.wp_param_file);
    }
    if (ctx->encOutQ) {
        Queue_Destroy(ctx->encOutQ);
    }

    VPU_DeInit(ctx->testEncConfig.coreIdx);

#ifdef SUPPORT_READ_BITSTREAM_IN_ENCODER
    if (ctx->bsReader)
        osal_free(ctx->bsReader);
    if (ctx->bs_fp)
        osal_fclose(ctx->bs_fp);
#endif
    osal_free(ctx);

    com->stateDoing = FALSE;

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
        VLOG(ERR, "%s:%d Failed to VPU_InitWidthBitCode, ret(%08x)\n", __FUNCTION__, __LINE__, retCode);
        return FALSE;
    }

    com->context = osal_malloc(sizeof(EncoderContext));
    ctx     = (EncoderContext*)com->context;
    osal_memset((void*)ctx, 0, sizeof(EncoderContext));

    retCode = PrintVpuProductInfo(coreIdx, &ctx->attr);
    if (retCode == RETCODE_VPU_RESPONSE_TIMEOUT ) {
        CNMErrorSet(CNM_ERROR_HANGUP);
        VLOG(ERR, "<%s:%d> Failed to PrintVpuProductInfo()\n", __FUNCTION__, __LINE__);
        HandleEncoderError(ctx->handle, 0, NULL);
        return FALSE;
    }
    ctx->cyclePerTick = 32768;
    if (TRUE == ctx->attr.supportNewTimer)
        ctx->cyclePerTick = 256;

    ctx->handle                      = NULL;
    ctx->frameIdx                    = 0;
    ctx->fbCount.reconFbNum          = 0;
    ctx->fbCount.srcFbNum            = 0;
    ctx->testEncConfig               = componentParam->testEncConfig;
    ctx->encOpenParam                = componentParam->encOpenParam;
    for (i=0; i<ENC_SRC_BUF_NUM ; i++ ) {
        ctx->encodedSrcFrmIdxArr[i] = 0;
        if ( ctx->testEncConfig.subFrameSyncEn)
            ctx->encodedSrcFrmIdxArr[i] = 1;
    }
    osal_memset(&ctx->vbCustomLambda,  0x00, sizeof(vpu_buffer_t));
    osal_memset(&ctx->vbScalingList,   0x00, sizeof(vpu_buffer_t));
    osal_memset(&ctx->scalingList,     0x00, sizeof(UserScalingList));
    osal_memset(&ctx->customLambda[0], 0x00, sizeof(ctx->customLambda));
    osal_memset(ctx->vbCustomMap,      0x00, sizeof(ctx->vbCustomMap));
#ifdef SUPPORT_READ_BITSTREAM_IN_ENCODER
    strcpy(ctx->bitstreamFileName, componentParam->testEncConfig.bitstreamFileName);
#endif
    if (ctx->encOpenParam.ringBufferEnable)
        com->numSinkPortQueue = 10;
    else
        com->numSinkPortQueue = componentParam->encOpenParam.streamBufCount;

    ctx->frameOutNum          = componentParam->testEncConfig.outNum;
    ctx->checkEncodedCnt   = componentParam->testEncConfig.NoLastFrameCheck;

    VLOG(INFO, "PRODUCT ID: %d\n", ctx->attr.productId);
    VLOG(INFO, "SUPPORT_COMMAND_QUEUE : %d, COMMAND_QUEUE_DEPTH=%d\n", ctx->attr.supportCommandQueue, COMMAND_QUEUE_DEPTH);

    return (Component)com;
}


ComponentImpl waveEncoderComponentImpl = {
    "wave_encoder",
    NULL,
    {0,},
    {0,},
    sizeof(PortContainerES),
    5,                       /* encoder's numSinkPortQueue(relates to streambufcount) */
    CreateEncoder,
    GetParameterEncoder,
    SetParameterEncoder,
    PrepareEncoder,
    ExecuteEncoder,
    ReleaseEncoder,
    DestroyEncoder
};

