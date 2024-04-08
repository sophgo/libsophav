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
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include "main_helper.h"
#include "cnm_app.h"
#include "decoder_listener.h"
#include "encoder_listener.h"

enum waitStatus {
    WAIT_AFTER_INIT         = 0,

    WAIT_AFTER_DEC_OPEN     = 1,
    WAIT_AFTER_SEQ_INIT     = 2,
    WAIT_AFTER_REG_BUF      = 3,
    WAIT_BEFORE_DEC_START   = 4,
    WAIT_BEFORE_DEC_CLOSE   = 5,

    WAIT_AFTER_ENC_OPEN     = 6,
    WAIT_AFTER_INIT_SEQ     = 7,
    WAIT_AFTER_REG_FRAME    = 8,
    WAIT_BEFORE_ENC_START   = 9,
    WAIT_BEFORE_ENC_CLOSE   = 10,
    WAIT_NOT_DEFINED        = 11,
    WAIT_STATUS_MAX
};

enum get_result_status{
    GET_RESULT_INIT,
    GET_RESULT_BEFORE_WAIT_INT, //before calling wait interrupt.
    GET_RESULT_AFTER_WAIT_INT   //got interrupt.
};

static Uint32           sizeInWord;
static Uint16*          pusBitCode;


typedef struct {
    char            inputFilePath[256];
    char            localFile[256];
    char            outputFilePath[256];
    char            refFilePath[256];
    CodStd          stdMode;
    BOOL            afbce;
    BOOL            afbcd;
    BOOL            scaler;
    Uint32          sclw;
    Uint32          sclh;
    Int32           bsmode;
    BOOL            enableWTL;
    BOOL            enableMVC;
    int             compareType;
    TestDecConfig   decConfig;
    FrameBufferFormat wtlFormat;
    EndianMode      streamEndian;
    EndianMode      frameEndian;
    EndianMode      sourceEndian;
    EndianMode      customMapEndian;
    TestEncConfig   encConfig;
    BOOL            isEncoder;
} InstTestConfig;

typedef struct {
    Uint32          totProcNum;             /* the number of process */
    Uint32          curProcNum;
    Uint32          numMulti;
    Uint32          cores;
    BOOL            performance;            /* for performance measurement */
    Uint32          pfClock;                /* for performance measurement */
    Uint32          bandwidth;              /* for bandwidth measurement */
    Uint32          fps;
    Uint32          numFrames;
    InstTestConfig  instConfig[MAX_NUM_INSTANCE];
    Uint32          feedingMode;
} TestMultiConfig;


static void WaitForNextStep(Int32 instIdx, Int32 curStepName)
{
}

static void Help(struct OptionExt *opt, const char *programName)
{
    int i;

    VLOG(INFO, "------------------------------------------------------------------------------\n");
    VLOG(INFO, "%s(API v%d.%d.%d)\n", GetBasename(programName), API_VERSION_MAJOR, API_VERSION_MINOR, API_VERSION_PATCH);
    VLOG(INFO, "\tAll rights reserved by Chips&Media(C)\n");
    VLOG(INFO, "\tSample program controlling the Chips&Media VPU\n");
    VLOG(INFO, "------------------------------------------------------------------------------\n");
    VLOG(INFO, "%s [option] --input <stream list file, aka cmd file>\n", GetBasename(programName));
    VLOG(INFO, "-h                          help\n");
    VLOG(INFO, "-c                          enable comparison mode\n");
    VLOG(INFO, "                            1 : compare with golden stream that specified --ref_file_path option\n");
    VLOG(INFO, "-e                          0 : decoder, 1: encoder\n");
    VLOG(INFO, "-n                          number of frames\n");
    VLOG(INFO, "-f                          conformance test without reset CNM FPGA\n");
    for (i = 0;i < MAX_GETOPT_OPTIONS;i++) {
        if (opt[i].name == NULL)
            break;
        VLOG(INFO, "%s", opt[i].help);
    }
}

static void SetEncMultiParam(TestMultiConfig* multiConfig, Uint32 idx, Int32 productId)
{
    InstTestConfig *instConfig = &multiConfig->instConfig[idx];
    TestEncConfig *encCfg = &instConfig->encConfig;

    encCfg->stdMode             = instConfig->stdMode;
    encCfg->frame_endian        = instConfig->frameEndian;
    encCfg->stream_endian       = instConfig->streamEndian;
    encCfg->source_endian       = instConfig->sourceEndian;
    encCfg->custom_map_endian   = instConfig->customMapEndian;
    encCfg->mapType             = LINEAR_FRAME_MAP;
    encCfg->performance         = multiConfig->performance;
    if (TRUE == PRODUCT_ID_W_SERIES(productId)) {
        encCfg->mapType         = COMPRESSED_FRAME_MAP;
    }

    strcpy(encCfg->cfgFileName, instConfig->inputFilePath);
    strcpy(encCfg->bitstreamFileName, instConfig->outputFilePath);
    if (TRUE == PRODUCT_ID_W_SERIES(productId)) {
        if (instConfig->compareType == MD5_COMPARE) {
            encCfg->compareType |= (1 << MODE_COMP_RECON);
            strcpy(encCfg->ref_recon_md5_path, instConfig->refFilePath);
        }
        else if (instConfig->compareType == STREAM_COMPARE) {
            encCfg->compareType |= (1 << MODE_COMP_ENCODED);
            strcpy(encCfg->ref_stream_path, instConfig->refFilePath);
        }
    }
    else { // coda9 encoder case
        char imageRootDir[256]="./yuv";
        encCfg->compareType = (instConfig->compareType == NO_COMPARE) ? NO_COMPARE : STREAM_COMPARE;
        if (encCfg->compareType == NO_COMPARE) {
            VLOG(ERR, "Invalid compare type(%d)\n", encCfg->compareType);
        } else {
            encCfg->compareType |= (1 << MODE_COMP_ENCODED);
        }
        strcpy(encCfg->ref_stream_path, instConfig->refFilePath);
        strcpy(encCfg->yuvSourceBaseDir, imageRootDir);
        instConfig->encConfig.lineBufIntEn = TRUE;
    }
    instConfig->encConfig.performance = multiConfig->performance;
    instConfig->encConfig.pfClock     = multiConfig->pfClock;
    instConfig->encConfig.bandwidth   = multiConfig->bandwidth;
    instConfig->encConfig.fps         = multiConfig->fps;
    instConfig->encConfig.outNum      = multiConfig->numFrames;
    if (encCfg->csc_enable == TRUE) {
        encCfg->cbcrInterleave       = 1;
        encCfg->yuvfeeder            = 1;
    }

    instConfig->encConfig.subFrameSyncMode = REGISTER_BASE_SUB_FRAME_SYNC;
    instConfig->encConfig.productId = productId;
}

static void SetDecMultiParam(TestMultiConfig* multiConfig, Uint32 idx, Int32 productId)
{
    InstTestConfig *instConfig = &multiConfig->instConfig[idx];
    TestDecConfig *decCfg = &instConfig->decConfig;

    decCfg->bitFormat            = instConfig->stdMode;
    decCfg->streamEndian         = instConfig->streamEndian;
    decCfg->frameEndian          = instConfig->frameEndian;
    decCfg->cbcrInterleave       = FALSE;
    decCfg->nv21                 = FALSE;
    decCfg->enableWTL            = instConfig->enableWTL;
    decCfg->coda9.enableMvc      = instConfig->enableMVC;
    decCfg->wtlMode              = FF_FRAME;
    decCfg->wtlFormat            = instConfig->wtlFormat;
    decCfg->performance          = multiConfig->performance;

    if (decCfg->bitFormat == STD_VP9) {
        decCfg->bitstreamMode = BS_MODE_PIC_END;
        decCfg->feedingMode = FEEDING_METHOD_FRAME_SIZE;
    } else {
        decCfg->bitstreamMode        = instConfig->bsmode;
        decCfg->feedingMode          = (instConfig->bsmode == BS_MODE_INTERRUPT) ? FEEDING_METHOD_FIXED_SIZE : FEEDING_METHOD_FRAME_SIZE;
    }

    if (decCfg->bitstreamMode == BS_MODE_PIC_END) {
        if (multiConfig->feedingMode != 0) {
            switch (multiConfig->feedingMode) {
            case 2: /* Using FFMPEG */
                decCfg->feedingMode = FEEDING_METHOD_FRAME_SIZE;
                break;
            case 3: /* for every frame, size(4byte) + es(1frame) */
                decCfg->feedingMode = FEEDING_METHOD_SIZE_PLUS_ES;
                break;
            case 4:
                decCfg->feedingMode = FEEDING_METHOD_HOST_FRAME_SIZE;
                break;
            default:
                VLOG(ERR, "--feeding=%d not supported\n", multiConfig->feedingMode);
            }
        }
    }

    if (TRUE == PRODUCT_ID_W_SERIES(productId)) {
        decCfg->mapType          = COMPRESSED_FRAME_MAP;
    }
    else {
        decCfg->mapType          = LINEAR_FRAME_MAP;
    }
    if (TRUE == PRODUCT_ID_W_SERIES(productId)) {
        VLOG(WARN, "INS_NUM : %d , bit stream buffer size : 0x%x \n", idx, decCfg->bsSize);
        VLOG(WARN, "If bs-size is 0, it use defined values in component_dec_feeder.c according to standards \n");
    } else {
        decCfg->bsSize               = (5*1024*1024);
    }
    decCfg->productId            = productId;

    if (instConfig->scaler == TRUE) {
        decCfg->scaleDownWidth   = instConfig->sclw;
        decCfg->scaleDownHeight  = instConfig->sclh;
    }

    strcpy(decCfg->inputPath, instConfig->inputFilePath);
    if (instConfig->localFile[0]) {
        strcpy(decCfg->inputPath, instConfig->localFile);
    }
    strcpy(decCfg->outputPath, instConfig->outputFilePath);
    if (instConfig->compareType == MD5_COMPARE) {
        decCfg->compareType = MD5_COMPARE;
        strcpy(decCfg->md5Path, instConfig->refFilePath);
    }
    else if (instConfig->compareType == YUV_COMPARE) {
        decCfg->compareType = YUV_COMPARE;
        strcpy(decCfg->refYuvPath, instConfig->refFilePath);
    }
    else {
        decCfg->compareType = NO_COMPARE;
        VLOG(ERR, "Invalid compare type(%d)\n", decCfg->compareType);
    }

    decCfg->performance = multiConfig->performance;
    decCfg->pfClock     = multiConfig->pfClock;
    decCfg->bandwidth   = multiConfig->bandwidth;
    decCfg->fps         = multiConfig->fps;
    decCfg->forceOutNum = multiConfig->numFrames;

}

static int MultiDecoderListener(Component com, Uint64 event, void* data, void* context)
{
    DecHandle               decHandle = NULL;
    int                     ret = 0;

    ComponentGetParameter(NULL, com, GET_PARAM_DEC_HANDLE, &decHandle);
    if (decHandle == NULL && event != COMPONENT_EVENT_DEC_DECODED_ALL) {
        // Terminated state
        return ret;
    }

    switch (event) {
    case COMPONENT_EVENT_DEC_OPEN:
        WaitForNextStep(decHandle->instIndex, WAIT_AFTER_DEC_OPEN);
        break;
    case COMPONENT_EVENT_DEC_COMPLETE_SEQ:
        WaitForNextStep(decHandle->instIndex, WAIT_AFTER_SEQ_INIT);
        HandleDecCompleteSeqEvent(com, (CNMComListenerDecCompleteSeq*)data, (DecListenerContext*)context);
        break;
    case COMPONENT_EVENT_DEC_REGISTER_FB:
        WaitForNextStep(decHandle->instIndex, WAIT_AFTER_REG_BUF);
        HandleDecRegisterFbEvent(com, (CNMComListenerDecRegisterFb*)data, (DecListenerContext*)context);
        break;
    case COMPONENT_EVENT_DEC_READY_ONE_FRAME:
        WaitForNextStep(decHandle->instIndex, WAIT_BEFORE_DEC_START);
        break;
    case COMPONENT_EVENT_DEC_INTERRUPT:
        HandleDecInterruptEvent(com, (CNMComListenerDecInt*)data, (DecListenerContext*)context);
        break;
    case COMPONENT_EVENT_DEC_GET_OUTPUT_INFO:
        HandleDecGetOutputEvent(com, (CNMComListenerDecDone*)data, (DecListenerContext*)context);
        break;
    case COMPONENT_EVENT_DEC_DECODED_ALL:
        // It isn't possible to get handle when a component is terminated state.
        decHandle = (DecHandle)data;
        if (decHandle) WaitForNextStep(decHandle->instIndex, WAIT_BEFORE_DEC_CLOSE);
        break;
    default:
        break;
    }
    return ret;
}

static BOOL CreateDecoderTask(CNMTask task, CNMComponentConfig* config, DecListenerContext* lsnCtx)
{
    BOOL    success = FALSE;
    RetCode ret     = RETCODE_SUCCESS;

/*
    if (STD_AV1 == config->testDecConfig.bitFormat) {
        if (0 == strcmp("obu", GetFileExtension(config->testDecConfig.inputPath))) {
            // Actually, the streams having the obu extension are annex-b format streams.
            config->testDecConfig.wave.av1Format = 2;
        }
        else {
            config->testDecConfig.wave.av1Format = 0;
        }
    }
*/
    if (RETCODE_SUCCESS != (ret=SetUpDecoderOpenParam(&(config->decOpenParam), &(config->testDecConfig)))) {
        VLOG(ERR, "%s:%d SetUpDecoderOpenParam failed Error code is 0x%x \n", __FUNCTION__, __LINE__, ret);
        return FALSE;
    }

    Component feeder   = ComponentCreate("feeder", config);
    Component renderer = ComponentCreate("renderer", config);
#ifdef CNM_WAVE_SERIES
    Component decoder  = ComponentCreate("wave_decoder",  config);
#else
    Component decoder  = ComponentCreate("coda9_decoder", config);
#endif

    CNMTaskAdd(task, feeder);
    CNMTaskAdd(task, decoder);
    CNMTaskAdd(task, renderer);

    if ((success=SetupDecListenerContext(lsnCtx, config, renderer)) == TRUE) {
        ComponentRegisterListener(decoder, COMPONENT_EVENT_DEC_ALL, MultiDecoderListener, (void*)lsnCtx);
    }

    return success;
}

static int MultiEncoderListener(Component com, Uint64 event, void* data, void* context)
{
    EncHandle                       encHandle   = NULL;
    CNMComListenerEncStartOneFrame* lsnStartEnc = NULL;
    ProductId                       productId;
    int                             ret = 0;

    UNREFERENCED_PARAMETER(lsnStartEnc);

    ComponentGetParameter(NULL, com, GET_PARAM_ENC_HANDLE, &encHandle);
    if (encHandle == NULL && event != COMPONENT_EVENT_ENC_ENCODED_ALL) {
        // Terminated state
        return ret;
    }

    switch (event) {
    case COMPONENT_EVENT_ENC_OPEN:
        WaitForNextStep(encHandle->instIndex, WAIT_AFTER_ENC_OPEN);
        break;
    case COMPONENT_EVENT_ENC_COMPLETE_SEQ:
        WaitForNextStep(encHandle->instIndex, WAIT_AFTER_INIT_SEQ);
        HandleEncCompleteSeqEvent(com, (CNMComListenerEncCompleteSeq*)data, (EncListenerContext*)context);
        break;
    case COMPONENT_EVENT_ENC_REGISTER_FB:
        WaitForNextStep(encHandle->instIndex, WAIT_AFTER_REG_FRAME);
        break;
    case COMPONENT_EVENT_ENC_HEADER:
        HandleEncCompHeaderEvent(com, (BitstreamReader)data, (EncListenerContext*)context);
        break;
    case COMPONENT_EVENT_ENC_READY_ONE_FRAME:
        WaitForNextStep(encHandle->instIndex, WAIT_BEFORE_ENC_START);
        break;
    case COMPONENT_EVENT_ENC_START_ONE_FRAME:
        break;
    case COMPONENT_EVENT_ENC_HANDLING_INT:
        HandleEncHandlingIntEvent(com, (CNMComListenerHandlingInt*)data, (EncListenerContext*)context);
        break;
    case COMPONENT_EVENT_ENC_GET_OUTPUT_INFO:
        encHandle = ((CNMComListenerEncDone*)data)->handle;
        productId = VPU_GetProductId(VPU_HANDLE_CORE_INDEX(encHandle));
        if (TRUE == PRODUCT_ID_W_SERIES(productId)) {
            HandleEncGetOutputEvent(com, (CNMComListenerEncDone*)data, (EncListenerContext*)context);
        }
        else {
            Coda9HandleEncGetOutputEvent(com, (CNMComListenerEncDone*)data, (EncListenerContext*)context);
        }
        break;
    case COMPONENT_EVENT_CODA9_ENC_MAKE_HEADER:
        Coda9HandleEncMakeHeaderEvent(com, (CNMComListenerEncMakeHeader*)data, (EncListenerContext*)context);
        break;
    case COMPONENT_EVENT_ENC_ENCODED_ALL:
        // It isn't possible to get handle when a component is terminated state.
        encHandle = (EncHandle)data;
        if (encHandle)
            WaitForNextStep(encHandle->instIndex, WAIT_BEFORE_ENC_CLOSE);
        break;
    case COMPONENT_EVENT_ENC_FULL_INTERRUPT:
        HandleEncFullEvent(com, (CNMComListenerEncFull*)data, (EncListenerContext*)context);
        break;
    case COMPONENT_EVENT_ENC_CLOSE:
        HandleEncGetEncCloseEvent(com, (CNMComListenerEncClose*)data, (EncListenerContext*)context);
        break;
    default:
        break;
    }
    return ret;
}

static BOOL CreateEncoderTask(CNMTask task, CNMComponentConfig* config, EncListenerContext* lsnCtx)
{
    Component feeder;
    Component encoder;
    Component reader;
    BOOL      success = FALSE;

    if (SetupEncoderOpenParam(&config->encOpenParam, &config->testEncConfig, NULL) == FALSE) {
        VLOG(ERR, "SetupEncoderOpenParam error\n");
        return FALSE;
    }

    config->encOpenParam.streamBufSize = 0x300000; /* 3MB : in order to reduce the amount of memory. */
    feeder  = ComponentCreate("yuvfeeder",      config);
    encoder = ComponentCreate("coda9_encoder",  config);
    reader  = ComponentCreate("reader",         config);

    CNMTaskAdd(task, feeder);
    CNMTaskAdd(task, encoder);
    CNMTaskAdd(task, reader);

    if ((success=SetupEncListenerContext(lsnCtx, config)) == TRUE) {
        ComponentRegisterListener(encoder, COMPONENT_EVENT_ENC_ALL, MultiEncoderListener, (void*)lsnCtx);
    }

    return success;
}

static BOOL MultiInstanceTest(TestMultiConfig* multiConfig, Uint16* fw, Uint32 size)
{
    Uint32              i;
    CNMComponentConfig  config;
    CNMTask             task;
    DecListenerContext* decListenerCtx = (DecListenerContext*)osal_malloc(sizeof(DecListenerContext) * multiConfig->numMulti);
    EncListenerContext* encListenerCtx = (EncListenerContext*)osal_malloc(sizeof(EncListenerContext) * multiConfig->numMulti);
    BOOL                ret     = FALSE;
    BOOL                success = TRUE;
    BOOL                match   = TRUE;
    CNMAppConfig        appCfg = {0,};

    CNMAppInit();

    for (i=0; i < multiConfig->numMulti; i++) {
        task = CNMTaskCreate();
        memset((void*)&config, 0x00, sizeof(CNMComponentConfig));
        config.bitcode       = (Uint8*)fw;
        config.sizeOfBitcode = size;
        if (multiConfig->instConfig[i].isEncoder == TRUE) {
            osal_memcpy((void*)&config.testEncConfig, &multiConfig->instConfig[i].encConfig, sizeof(TestEncConfig));
            success = CreateEncoderTask(task, &config, &encListenerCtx[i]);
        }
        else
        {
            osal_memcpy((void*)&config.testDecConfig, &multiConfig->instConfig[i].decConfig, sizeof(TestDecConfig));
            success = CreateDecoderTask(task, &config, &decListenerCtx[i]);
            if (config.testDecConfig.outputPvric.pvricVersion == 5) {
                appCfg.task_interval = 2000;
            }
        }
        CNMAppAdd(task);
        if (success == FALSE) {
            CNMAppStop();
            return FALSE;
        }
    }

    ret = CNMAppRun(appCfg);

    for (i=0; i<multiConfig->numMulti; i++) {
        if (multiConfig->instConfig[i].isEncoder == TRUE) {
            match &= (encListenerCtx[i].match == TRUE && encListenerCtx[i].matchOtherInfo == TRUE);
            ClearEncListenerContext(&encListenerCtx[i]);
        }
        else
        {
            match &= decListenerCtx[i].match;
            ClearDecListenerContext(&decListenerCtx[i]);
        }
    }
    if (ret == TRUE) ret = match;

    osal_free(decListenerCtx);
    osal_free(encListenerCtx);

    return ret;
}

static BOOL CheckTestConfig(const TestMultiConfig* testConfig) 
{
    return TRUE;
}

void SetDefaultMultiTestConfig(TestMultiConfig *mtest_cfg) {
    int     i = 0;
    osal_memset(mtest_cfg, 0, sizeof(TestMultiConfig));

    //Initailize instances
    for(i = 0; i < MAX_NUM_INSTANCE; i++) {
        mtest_cfg->instConfig[i].stdMode = STD_HEVC;
        mtest_cfg->instConfig[i].frameEndian = VPU_FRAME_ENDIAN;
        mtest_cfg->instConfig[i].streamEndian = VPU_STREAM_ENDIAN;
        mtest_cfg->instConfig[i].sourceEndian = VPU_SOURCE_ENDIAN;
        mtest_cfg->instConfig[i].customMapEndian = VPU_CUSTOM_MAP_ENDIAN;
        mtest_cfg->instConfig[i].encConfig.srcFormat = FORMAT_420;
    
    }

    return;
}

int main(int argc, char **argv)
{
    Int32           coreIndex   = 0;
    Uint32          productId   = 0;
    TestMultiConfig multiConfig;
    int             opt, index, i;
    char*           tempArg;
    char*           optString   = "fc:e:hn:";

    struct option options[MAX_GETOPT_OPTIONS];
    struct OptionExt options_help[MAX_GETOPT_OPTIONS] = {
        {"instance-num",          1, NULL, 0, "--instance-num              total instance-number to run\n"},
        {"codec",                 1, NULL, 0, "--codec                     The index of codec (AVC = 0, VC1 = 1, MPEG2 = 2, MPEG4 = 3, H263 = 4, DIV3 = 5, RV = 6, AVS = 7, THO = 9, VP3 = 10, VP8 = 11, HEVC = 12, VP9 = 13, AVS2 = 14, AV1 = 16)\n"},
        {"input",                 1, NULL, 0, "--input                     bitstream(decoder) or cfg(encoder) path\n"},
        {"output",                1, NULL, 0, "--output                    yuv(decoder) or bitstream(encoder) path\n"},
        {"xml-output",            1, NULL, 0, "--xml-output                xml output file path\n"},
        {"ref_file_path",         1, NULL, 0, "--ref_file_path             Golden md5 or stream path\n"},
        {"coreIdx",               1, NULL, 0, "--coreIdx                   core index: default 0\n"},
        {"bs-size",               1, NULL, 0, "--bs-size                   bit stream buffer size in byte \n"},
        {"feeding",               1, NULL, 0, "--feeding                   0: auto, 1: fixed-size, 2: ffempg, 3: size(4byte)+es\n"},
        {"scaler",                1, NULL, 0, "--scaler                    enable scaler option. default 0\n"},
        {"sclw",                  1, NULL, 0, "--sclw                      set scale width value.\n"},
        {"sclh",                  1, NULL, 0, "--sclh                      set scale height value.\n"},
        {"bsmode",                1, NULL, 0, "--bsmode                    set bitstream mode.\n"},
        {"enable-wtl",            1, NULL, 0, "--enable-wtl                enable wtl option. default 0\n"},
        {"wtl-format",            1, NULL, 0, "--wtl-format                yuv format. default 0 \n"},
        {"stream-endian",         1, NULL, 0, "--stream-endian             16~31, default 31(LE) Please refer programmer's guide or datasheet \n"},
        {"frame-endian",          1, NULL, 0, "--frame-endian              16~31, default 31(LE) Please refer programmer's guide or datasheet \n"},
        {"source-endian",         1, NULL, 0, "--source-endian             16~31, default 31(LE) Please refer programmer's guide or datasheet \n"},
        {"custom-map-endian",     1, NULL, 0, "--custom-map-endian         16~31, default 31(LE) Please refer programmer's guide or datasheet\n"},
        {"av1-format",            1, NULL, 0, "--av1-format                temp parameter to seperate out av1 stream format. 0: ivf 1: obu 2: annexb\n"},
        {"sfs",                   1, NULL, 0, "--sfs                       subFrameSync 0: off 1: on, default off\n"},
        {"sfs-size",              1, NULL, 0, "--sfs-size                  SFS(subFrameSync) yuv toggle height size(8, 16, 32, 64) - WAVE6 only\n"},
        {"sfs-reg-base",          1, NULL, 0, "--sfs-reg-base              0 : siginal(HW) base SFS, 1 : register base SFS - WAVE6 only \n"},
        {"sfs-fb-count",          1, NULL, 0, "--sfs-fb-count              Frame buffer count for SFS - WAVE6 only\n"},
        {"forced-10b",            1, NULL, 0, "--forced-10b                forced 10b flag \n"},
        {"last-2bit-in-8to10",    1, NULL, 0, "--last-2bit-in-8to10        forced 10b LSB data. range : 0~3 \n"},
        {"csc-enable",            1, NULL, 0, "--csc-enable                1:enable csc option. default 0\n"},
        {"csc-format-order",      1, NULL, 0, "--csc-format-order          0~13:CSC format, default 0\n"
                                              "                            0:ARGB, 1:ARBG,  2:AGRB,  3:AGBR,  4:ABGR,  5:ABRG\n"
                                              "                            8:RGBA, 9:RBGA, 10:GRBA, 11:GBRA, 12:BGRA, 13:BRGA\n"},
        {"csc-srcFormat",         1, NULL, 0, "--csc-srcFormat             90~93:CSC src format, default 90\n"
                                              "                            90:FORMAT_RGB_32BIT_PACKED, 91:FORMAT_YUV444_32BIT_PACKED, 92:FORMAT_RGB_P10_32BIT_PACKED, 93:FORMAT_YUV444_P10_32BIT_PACKED\n"},
        {"rotAngle",              1, NULL, 0, "--rotAngle                  rotation angle(0,90,180,270), Not supported on WAVE420L, WAVE525, WAVE521C_DUAL\n"},
        {"mirDir",                1, NULL, 0, "--mirDir                    1:Vertical, 2: Horizontal, 3:Vert-Horz, Not supported on WAVE420L, WAVE525, WAVE521C_DUAL\n"},
        {"pf",                    1, NULL, 0, "--pf                        0: disable peformance report(default), 1: enable performance report\n"},
        {"pf-clock",              1, NULL, 0, "--pf-clock                  peformance clock in Hz(It must be BCLK for WAVE5 series).\n"},
        {"bw",                    1, NULL, 0, "--bw                        0: disable bandwidth report(default) 1: enable bandwidth report\n"},
        {"fps",                   1, NULL, 0, "--fps                       frame per seconds\n"},
        {"cores",                 1, NULL, 0, "--cores                     The number of vcores for each instance\n"},
        {"vcore-idc",             1, NULL, 0, "--vcore-idc                 select vcore to run(0:auto, 1:core1, 2:core2, 3:core1,2\n"},
        {NULL,                    0, NULL, 0},
    };
    const char*     name;
    char*           firmwarePath = NULL;
    Uint32          ret = 0;


    SetDefaultMultiTestConfig(&multiConfig);



    for (i = 0; i < MAX_GETOPT_OPTIONS;i++) {
        if (options_help[i].name == NULL)
            break;
        osal_memcpy(&options[i], &options_help[i], sizeof(struct option));
    }

    while ((opt=getopt_long(argc, argv, optString, options, &index)) != -1) {
        switch (opt) {
        case 'c':
            tempArg = strtok(optarg, ",");
            for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                multiConfig.instConfig[i].compareType = atoi(tempArg);
                tempArg = strtok(NULL, ",");
                if (tempArg == NULL)
                    break;
            }
            break;
        case 'e':
            tempArg = strtok(optarg, ",");
            for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                multiConfig.instConfig[i].isEncoder = atoi(tempArg);
                tempArg = strtok(NULL, ",");
                if (tempArg == NULL)
                    break;
            }
            break;
        case 'h':
            Help(options_help, argv[0]);
            return 0;
        case 'n':
            multiConfig.numFrames = atoi(optarg);
            break;
        case 0:
            name = options[index].name;
            if (strcmp("instance-num", name) == 0) {
                multiConfig.numMulti = atoi(optarg);
            }
            else if (strcmp("codec", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].stdMode = (CodStd)atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("input", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    osal_memcpy(multiConfig.instConfig[i].inputFilePath, tempArg, strlen(tempArg));
                    ChangePathStyle(multiConfig.instConfig[i].inputFilePath);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("output", name) == 0) {
                char output_temp[730] = { 0, };
                char *output_name;
                sprintf(output_temp, "%s", optarg);
                output_name = output_temp;
                i = 0;
                while ((tempArg = strsep(&output_name, ",")) != NULL) {
                    osal_memcpy(multiConfig.instConfig[i].outputFilePath, tempArg, strlen(tempArg));
                    ChangePathStyle(multiConfig.instConfig[i].outputFilePath);
                    i++;
                }
            }
            else if (strcmp("xml-output", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    osal_memcpy(multiConfig.instConfig[i].decConfig.xmlOutputPath, tempArg, strlen(tempArg));
                    ChangePathStyle(multiConfig.instConfig[i].decConfig.xmlOutputPath);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("ref_file_path", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    osal_memcpy(multiConfig.instConfig[i].refFilePath, tempArg, strlen(tempArg));
                    ChangePathStyle(multiConfig.instConfig[i].refFilePath);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("bs-size", name) == 0) {
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].decConfig.bsSize = atoi(optarg);
                }
            }
            else if (strcmp("feeding", name) == 0) {
                multiConfig.feedingMode = (Uint32)atoi(optarg);
            }
            else if (!strcmp(options[index].name, "coreIdx")) {
                VLOG(INFO, "just DPI constraint");
            }
            else if (strcmp("scaler", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].scaler = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("sclw", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].sclw = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("sclh", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].sclh = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("bsmode", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].bsmode = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("enable-wtl", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].enableWTL = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("wtl-format", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].wtlFormat = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("stream-endian", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].streamEndian = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("frame-endian", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].frameEndian = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("source-endian", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].sourceEndian = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("custom-map-endian", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].customMapEndian = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("av1-format", name) == 0) {
                tempArg = strtok(optarg, ",");
                for (i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].decConfig.wave.av1Format = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("sfs", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].encConfig.subFrameSyncEn = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL) break;
                }
            }
            else if (strcmp("sfs-reg-base", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].encConfig.subFrameSyncMode = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL) break;
                }
            }
            else if (strcmp("sfs-fb-count", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].encConfig.subFrameSyncFbCount = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL) break;
                }
            }
            else if (strcmp("forced-10b", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].encConfig.forced_10b = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL) break;
                }
            }
            else if (strcmp("last-2bit-in-8to10", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].encConfig.last_2bit_in_8to10 = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL) break;
                }
            }
            else if (strcmp("csc-enable", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].encConfig.csc_enable = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL) break;
                }
            }
            else if (strcmp("csc-format-order", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].encConfig.csc_format_order = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL) break;
                }
            }
            else if (strcmp("csc-srcFormat", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].encConfig.srcFormat = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL) break;
                }
            }
            else if (strcmp("rotAngle", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].encConfig.rotAngle = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL) break;
                }
            }
            else if (strcmp("mirDir", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].encConfig.mirDir = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL) break;
                }
            }
            else if (!strcmp(options[index].name, "pf")) {
                multiConfig.performance = (BOOL)atoi(optarg);
            }
            else if (!strcmp(options[index].name, "bw")) {
                multiConfig.bandwidth = (BOOL)atoi(optarg);
            }
            else if (!strcmp(options[index].name,  "pf-clock")) {
                multiConfig.pfClock = (Uint32)atoi(optarg);
            }
            else if (!strcmp(options[index].name,  "fps")) {
                multiConfig.fps = (Uint32)atoi(optarg);
            }
            else {
                VLOG(ERR, "unknown --%s\n", name);
                Help(options_help, argv[0]);
                return 1;
            }
            break;
        case '?':
            return 1;
        }
    }

    InitLog();

    if (CheckTestConfig(&multiConfig) == FALSE) {
        VLOG(ERR, "fail to CheckTestConfig()\n");
        return FALSE;
    }



    productId = VPU_GetProductId(coreIndex);
    switch (productId) {
    case PRODUCT_ID_960: firmwarePath = CODA960_BITCODE_PATH; break;
    case PRODUCT_ID_980: firmwarePath = CODA980_BITCODE_PATH; break;
    case PRODUCT_ID_511: firmwarePath = WAVE511_BITCODE_PATH; break;
    case PRODUCT_ID_521: firmwarePath = WAVE521_BITCODE_PATH; break;
    case PRODUCT_ID_517: firmwarePath = WAVE517_BITCODE_PATH; break;
    case PRODUCT_ID_617: firmwarePath = WAVE637_BITCODE_PATH; break;
    case PRODUCT_ID_627: firmwarePath = WAVE627_BITCODE_PATH; break;
    case PRODUCT_ID_637: firmwarePath = WAVE637_BITCODE_PATH; break;
    default:
        VLOG(ERR, "<%s:%d> Unknown productId(%d)\n", __FUNCTION__, __LINE__, productId);
        return 1;
    }

    VLOG(TRACE, "FW PATH = %s\n", firmwarePath);

    if (LoadFirmware(productId, (Uint8**)&pusBitCode, &sizeInWord, firmwarePath) < 0) {
        VLOG(ERR, "%s:%d Failed to load firmware: %s\n", __FUNCTION__, __LINE__, firmwarePath);
        return 1;
    }
    for (i = 0; i < multiConfig.numMulti; i++) {
        if (multiConfig.instConfig[i].isEncoder == FALSE)
            SetDecMultiParam(&multiConfig, i, productId);
        else
            SetEncMultiParam(&multiConfig, i, productId);
    }

    do {

        if (MultiInstanceTest(&multiConfig, pusBitCode, sizeInWord) == FALSE) {
            ret = 1;
            break;
        }
    } while(FALSE);


    osal_free(pusBitCode);

    if (ret != 0) {
        VLOG(ERR, "Failed to MultiInstanceTest()\n");
    }

    DeInitLog();
    return ret;
}

