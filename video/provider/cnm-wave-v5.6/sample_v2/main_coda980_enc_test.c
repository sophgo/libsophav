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
#include <string.h>
#include <time.h>
#include <getopt.h>
#include "cnm_app.h"
#include "encoder_listener.h"
#include "bw_monitor.h"


typedef struct CommandLineArgument{
    int argc;
    char **argv;
}CommandLineArgument;


static void Help(struct OptionExt *opt, const char *programName)
{
    int i;

    VLOG(INFO, "------------------------------------------------------------------------------\n");
    VLOG(INFO, "%s(API v%d.%d.%d)\n", GetBasename(programName), API_VERSION_MAJOR, API_VERSION_MINOR, API_VERSION_PATCH);
    VLOG(INFO, "\tAll rights reserved by Chips&Media(C)\n");
    VLOG(INFO, "------------------------------------------------------------------------------\n");
    VLOG(INFO, "%s [option] --input bistream\n", GetBasename(programName));
    VLOG(INFO, "-h                          help\n");
    VLOG(INFO, "-n [num]                    output frame number, -1,0:unlimited encoding(depends on YUV size)\n");
    VLOG(INFO, "-v                          print version information\n");
    VLOG(INFO, "-c                          compare with golden bitstream\n");

    for (i = 0;i < MAX_GETOPT_OPTIONS;i++) {
        if (opt[i].name == NULL)
            break;
        VLOG(INFO, "%s", opt[i].help);
    }
}


static BOOL PreSetupEncOpenParam(EncOpenParam *encOpenParam, const TestEncConfig* testEncConfig)
{
    if (NULL == encOpenParam || NULL == testEncConfig) {
        VLOG(ERR, "%s:%d NULL point exception", __FUNCTION__, __LINE__);
        return FALSE;
    }
    encOpenParam->EncStdParam.avcParam.mvcExtension = testEncConfig->coda9.enableMvc;
    return TRUE;
}

static void DisplayEncOptions(const TestEncConfig* testEncConfig)
{
    VLOG(INFO, "--------------------- CODA980(SampleV2) ENCODER OPTIONS ---------------------\n");
    VLOG(INFO, "[coreIdx            ]: %d\n", testEncConfig->coreIdx);
    VLOG(INFO, "[mapType            ]: %d\n", testEncConfig->mapType);
    VLOG(INFO, "[cbcrInterleave     ]: %d\n", testEncConfig->cbcrInterleave);
    VLOG(INFO, "[sourceEndian       ]: %d\n", testEncConfig->source_endian);
    VLOG(INFO, "[enableMvc          ]: %d\n", testEncConfig->coda9.enableMvc);
    VLOG(INFO, "-----------------------------------------------------------------------------\n");
}

static BOOL ParseArgumentAndSetTestConfig(CommandLineArgument argument, TestEncConfig* testConfig) {
    Int32  opt  = 0, i = 0, idx = 0;
    int    argc = argument.argc;
    char** argv = argument.argv;
   char* optString = "c:hn:v";
    struct option options[MAX_GETOPT_OPTIONS];
    struct OptionExt options_help[MAX_GETOPT_OPTIONS] = {
        {"output",                1, NULL, 0, "--output                    An output bitstream file path\n"},
        {"yuv-base",              1, NULL, 0, "--yuv_base                  YUV file path. The value of InputFile in a cfg is replaced to this value.\n"},
        {"codec",                 1, NULL, 0, "--codec                     The index of codec (H.264:0, MP4:3, H263:4), default 0\n"},
        {"cfg-path",              1, NULL, 0, "--cfg-path                  encoder configuration file path\n\n"},
        {"coreIdx",               1, NULL, 0, "--coreIdx                   core index: default 0\n"},
        {"enable-ringBuffer",     0, NULL, 0, "--enable-ringBuffer         enable ringBuffer\n"},
        {"enable-lineBufInt",     0, NULL, 0, "--enable-lineBufInt         enable linebuffer interrupt\n"},
        {"maptype",               1, NULL, 0, "--maptype                   CODA: 0~9, default 0(LINEAR_FRAME_MAP)\n"},
        {"enable-cbcrInterleave", 0, NULL, 0, "--enable-cbcrInterleave     enable cbcr interleave\n"},
        {"enable-nv21",           0, NULL, 0, "--enable-nv21               enable NV21(must set enable-cbcrInterleave)\n"},
        {"enable-linear2tiled",   0, NULL, 0, "--enable-linear2tiled\n"},
        {"rotate",                1, NULL, 0, "--rotate                    rotation angle(0,90,180,270)\n"},
        {"mirror",                1, NULL, 0, "--mirror                    1:Vertical, 2: Horizontal, 3:Vert-Horz\n"}, /* 20 */
        {"secondary-axi",         1, NULL, 0, "--secondary-axi             0~3: bit mask values, Please refer programmer's guide or datasheet\n"
        "                            1: RDO, 2: LF\n"},
        {"frame-endian",          1, NULL, 0, "--frame-endian              16~31, default 31(LE) Please refer programmer's guide or datasheet\n"},
        {"stream-endian",         1, NULL, 0, "--stream-endian             16~31, default 31(LE) Please refer programmer's guide or datasheet\n"},
        {"ref_stream_path",       1, NULL, 0, "--ref_stream_path           golden data which is compared with encoded stream when -c option\n"},
        {"subframesync",          1, NULL, 0, "--subframesync              1:Enable subframe-sync, 0:disable \n" },
        {"sfs",                   1, NULL, 0, "--sfs                       1:Enable subframe-sync, 0:disable\n" },
        {"enable-mvc",            0, NULL, 0, "--enable-mvc                 enable mvc\n"},
        {NULL,                    0, NULL, 0, NULL},
    };


    for (i = 0; i < MAX_GETOPT_OPTIONS;i++) {
        if (options_help[i].name == NULL)
            break;
        osal_memcpy(&options[i], &options_help[i], sizeof(struct option));
    }

    while ((opt=getopt_long(argc, argv, optString, options, (int *)&idx)) != -1) {
        switch (opt) {
        case 'n':
            testConfig->outNum = atoi(optarg);
            break;
        case 'c':
            if (TRUE == (BOOL)atoi(optarg)) {
                testConfig->compareType |= (1 << MODE_COMP_ENCODED);
            }
            else {
                testConfig->compareType = NO_COMPARE;
                VLOG(ERR, "Invalid compare type(%d)\n", testConfig->compareType);
            }
            break;
        case 'h':
            Help(options_help, argv[0]);
            exit(0);
        case 0:
            if (!strcmp(options[idx].name, "output")) {
                osal_memcpy(testConfig->bitstreamFileName, optarg, strlen(optarg));
                ChangePathStyle(testConfig->bitstreamFileName);
            } else if (!strcmp(options[idx].name, "yuv-base")) {
                strcpy(testConfig->yuvSourceBaseDir, optarg);
                ChangePathStyle(testConfig->yuvSourceBaseDir);
            } else if (!strcmp(options[idx].name, "codec")) {
                testConfig->stdMode = (CodStd)atoi(optarg);
            } else if (!strcmp(options[idx].name, "cfg-path")) {
                osal_memcpy(testConfig->cfgFileName, optarg, strlen(optarg));
            } else if (!strcmp(options[idx].name, "coreIdx")) {
                testConfig->coreIdx = atoi(optarg);
            } else if (!strcmp(options[idx].name, "enable-ringBuffer")) {
                testConfig->ringBufferEnable = TRUE;
            } else if (!strcmp(options[idx].name, "enable-lineBufInt")) {
                testConfig->lineBufIntEn = TRUE;
            } else if ((!strcmp(options[idx].name, "maptype"))) {
                testConfig->mapType = (TiledMapType)atoi(optarg);
            } else if (!strcmp(options[idx].name, "enable-cbcrInterleave")) {
                testConfig->cbcrInterleave = TRUE;
            } else if (!strcmp(options[idx].name, "enable-nv21")) {
                testConfig->nv21 = TRUE;
            } else if (!strcmp(options[idx].name, "enable-linear2tiled")) {
                testConfig->coda9.enableLinear2Tiled =  TRUE;
                testConfig->coda9.linear2TiledMode = FF_FRAME;
            } else if (!strcmp(options[idx].name, "rotate")) {
                testConfig->rotAngle = atoi(optarg);
            } else if (!strcmp(options[idx].name, "mirror")) {
                testConfig->mirDir = (MirrorDirection)atoi(optarg);
            } else if (!strcmp(options[idx].name, "secondary-axi")) {
                testConfig->secondaryAXI = atoi(optarg);
            } else if (!strcmp(options[idx].name, "frame-endian")) {
                testConfig->frame_endian = atoi(optarg);
            } else if (!strcmp(options[idx].name, "stream-endian")) {
                testConfig->stream_endian = (EndianMode)atoi(optarg);
            } else if (!strcmp(options[idx].name, "ref_stream_path")) {
                osal_memcpy(testConfig->ref_stream_path, optarg, strlen(optarg));
                ChangePathStyle(testConfig->ref_stream_path);
            } else if (!strcmp(options[idx].name, "subframesync")) {
                testConfig->subFrameSyncEn = atoi(optarg);
            } else if (!strcmp(options[idx].name, "sfs")) {
                testConfig->subFrameSyncEn = atoi(optarg);
            } else if ((!strcmp(options[idx].name, "enable-mvc"))) {
                testConfig->coda9.enableMvc = TRUE;
            } else {
                VLOG(ERR, "not exist param = %s\n", options[idx].name);
                Help(options_help, argv[0]);
                return FALSE;
            }
            break;
        default:
            VLOG(ERR, "%s\n", optarg);
            Help(options_help, argv[0]);
            return FALSE;
        }
    }
    VLOG(INFO, "\n");


    if (testConfig->mapType == TILED_FRAME_MB_RASTER_MAP || testConfig->mapType == TILED_FIELD_MB_RASTER_MAP) {
        testConfig->cbcrInterleave = TRUE;
    }

    return TRUE;
}


int main(int argc, char **argv)
{
    char*               fwPath     = NULL;
    TestEncConfig       testConfig;
    CommandLineArgument argument;
    CNMComponentConfig  config;
    CNMTask             task;
    Component           yuvFeeder;
    Component           encoder;
    Component           reader;
    Uint32              sizeInWord;
    Uint16*             pusBitCode;
    BOOL                ret;
    BOOL                testResult;
    EncListenerContext  lsnCtx;
    CNMAppConfig        appCfg = {0,};

    osal_memset(&argument, 0x00, sizeof(CommandLineArgument));
    osal_memset(&config,   0x00, sizeof(CNMComponentConfig));

    InitLog();
    Coda9SetDefaultEncTestConfig(&testConfig);
    argument.argc = argc;
    argument.argv = argv;
    if (ParseArgumentAndSetTestConfig(argument, &testConfig) == FALSE) {
        VLOG(ERR, "fail to ParseArgumentAndSetTestConfig()\n");
        return 1;
    }


    testConfig.productId = (ProductId)VPU_GetProductId(testConfig.coreIdx);
    switch (testConfig.productId) {
    case PRODUCT_ID_960:    fwPath = CODA960_BITCODE_PATH; break;
    case PRODUCT_ID_980:    fwPath = CODA980_BITCODE_PATH; break;
    default:
        VLOG(ERR, "Unknown product id: %d\n", testConfig.productId);
        return 1;
    }

    VLOG(INFO, "FW PATH = %s\n", fwPath);

    if (LoadFirmware(testConfig.productId, (Uint8**)&pusBitCode, &sizeInWord, fwPath) < 0) {
        VLOG(ERR, "%s:%d Failed to load firmware: %s\n", __FUNCTION__, __LINE__, fwPath);
        return 1;
    }

    config.testEncConfig = testConfig;
    config.bitcode       = (Uint8*)pusBitCode;
    config.sizeOfBitcode = sizeInWord;

    PreSetupEncOpenParam(&(config.encOpenParam), &testConfig);

    DisplayEncOptions(&(config.testEncConfig));

    if (SetupEncoderOpenParam(&config.encOpenParam, &config.testEncConfig, &config.encCfg) == FALSE)
        return 1;

    CNMAppInit();

    task       = CNMTaskCreate();
    yuvFeeder  = ComponentCreate("yuvfeeder", &config);
    encoder    = ComponentCreate("coda9_encoder", &config);
    reader     = ComponentCreate("reader", &config);

    CNMTaskAdd(task, yuvFeeder);
    CNMTaskAdd(task, encoder);
    CNMTaskAdd(task, reader);

    CNMAppAdd(task);

    if ((ret=SetupEncListenerContext(&lsnCtx, &config)) == TRUE) {
        ComponentRegisterListener(encoder, COMPONENT_EVENT_ENC_ALL, EncoderListener, (void*)&lsnCtx);
        ret = CNMAppRun(appCfg);
    }
    else {
        CNMAppStop();
    }

    osal_free(pusBitCode);
    ClearEncListenerContext(&lsnCtx);

    testResult = (ret == TRUE && lsnCtx.match == TRUE && lsnCtx.matchOtherInfo == TRUE);
    if (testResult == TRUE) VLOG(INFO, "[RESULT] SUCCESS\n");
    else                    VLOG(ERR,  "[RESULT] FAILURE\n");

    DeInitLog();
    if ( CNMErrorGet() != 0 )
        return CNMErrorGet();

    return (testResult == TRUE) ? 0 : 1;
}

