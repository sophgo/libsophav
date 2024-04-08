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
#include <getopt.h>
#include "vpuapi.h"
#include "cnm_app.h"
#include "encoder_listener.h"
#include "bw_monitor.h"
#include "main_helper.h"
#include "component/cnm_app.h"
#include "component/component.h"
#include "component_encoder/encoder_listener.h"

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

static BOOL CheckTestConfig(TestEncConfig *testConfig)
{
    if ( (testConfig->compareType & (1<<MODE_SAVE_ENCODED)) && testConfig->bitstreamFileName[0] == 0) {
        testConfig->compareType &= ~(1<<MODE_SAVE_ENCODED);
        VLOG(ERR,"You want to Save bitstream data. Set the path\n");
        return FALSE;
    }

    if ( (testConfig->compareType & (1<<MODE_COMP_ENCODED)) && testConfig->ref_stream_path[0] == 0) {
        testConfig->compareType &= ~(1<<MODE_COMP_ENCODED);
        VLOG(ERR,"You want to Compare bitstream data. Set the path\n");
        return FALSE;
    }

    return TRUE;
}

static BOOL ParseArgumentAndSetTestConfig(CommandLineArgument argument, TestEncConfig* testConfig) {
    Int32  opt  = 0, i = 0, idx = 0;
    int    argc = argument.argc;
    char** argv = argument.argv;
    char* optString = "rbhvn:";
    struct option options[MAX_GETOPT_OPTIONS];
    struct OptionExt options_help[MAX_GETOPT_OPTIONS] = {
        {"output",                1, NULL, 0, "--output                    An output bitstream file path\n"},
        {"input",                 1, NULL, 0, "--input                     YUV file path. The value of InputFile in a cfg is replaced to this value.\n"},
        {"codec",                 1, NULL, 0, "--codec                     codec index, 16: AV1, 12:HEVC, 0:AVC\n"},
        {"cfgFileName",           1, NULL, 0, "--cfgFileName               cfg file path\n"},
        {"coreIdx",               1, NULL, 0, "--coreIdx                   core index: default 0\n"},
        {"picWidth",              1, NULL, 0, "--picWidth                  source width\n"},
        {"picHeight",             1, NULL, 0, "--picHeight                 source height\n"},
        {"kbps",                  1, NULL, 0, "--kbps                      RC bitrate in kbps. In case of without cfg file, if this option has value then RC will be enabled\n"},
#ifdef SUPPORT_SOURCE_RELEASE_INTERRUPT
        {"srcReleaseInt",         1, NULL, 0, "--srcReleaseInt             1 : enable source release Interrupt\n"},
#endif
        {"lineBufInt",            1, NULL, 0, "--lineBufInt                1 : enable linebuffer interrupt\n"},
        {"subFrameSyncEn",        1, NULL, 0, "--subFrameSyncEn            subFrameSync 0: off 1: on, default off\n"},
        {"sfs",                   1, NULL, 0, "--sfs                       enable Sub Frame Sync(SFS)\n"},
        {"sfs-reg-base",          1, NULL, 0, "--sfs-reg-base              0 : siginal(HW) base SFS, 1 : register base SFS - WAVE6 only \n"},
        {"sfs-fb-count",          1, NULL, 0, "--sfs-fb-count              Frame buffer count for SFS - WAVE6 only\n"},
        {"nalSizeReportDataPath", 1, NULL, 0, "--nalSizeReportDataPath     nal size report data path to save(if exist, enable)\n"},
        {"forced-10b",            1, NULL, 0, "--forced-10b                forced 10b flag \n"},
        {"last-2bit-in-8to10",    1, NULL, 0, "--last-2bit-in-8to10        forced 10b LSB data. range : 0~3 \n"},
        {"csc-enable",            1, NULL, 0, "--csc-enable                1:CSC enable flag in PRP, default 0\n"},
        {"csc-format-order",      1, NULL, 0, "--csc-format-order          0~13:CSC format, default 0\n"
                                              "                            0:ARGB, 1:ARBG,  2:AGRB,  3:AGBR,  4:ABGR,  5:ABRG\n"
                                              "                            8:RGBA, 9:RBGA, 10:GRBA, 11:GBRA, 12:BGRA, 13:BRGA\n"},
        {"loop-count",            1, NULL, 0, "--loop-count                integer number. loop test, default 0\n"},
        {"enable-cbcrInterleave", 0, NULL, 0, "--enable-cbcrInterleave     enable cbcr interleave\n"},
        {"nv21",                  1, NULL, 0, "--nv21                      enable NV21(must set enable-cbcrInterleave)\n"},
        {"i422",                  1, NULL, 0, "--i422                      enable I422\n"},
        {"packedFormat",          1, NULL, 0, "--packedFormat              1:YUYV, 2:YVYU, 3:UYVY, 4:VYUY\n"},
        {"rotAngle",              1, NULL, 0, "--rotAngle                  rotation angle(0,90,180,270), Not supported on WAVE420L, WAVE525, WAVE521C_DUAL\n"},
        {"mirDir",                1, NULL, 0, "--mirDir                    1:Vertical, 2: Horizontal, 3:Vert-Horz, Not supported on WAVE420L, WAVE525, WAVE521C_DUAL\n"},
        {"secondary-axi",         1, NULL, 0, "--secondary-axi             0~3: bit mask values, Please refer programmer's guide or datasheet\n"
                                              "                            1: RDO, 2: LF\n"},
        {"frame-endian",          1, NULL, 0, "--frame-endian              16~31, default 31(LE) Please refer programmer's guide or datasheet\n"},
        {"stream-endian",         1, NULL, 0, "--stream-endian             16~31, default 31(LE) Please refer programmer's guide or datasheet\n"},
        {"source-endian",         1, NULL, 0, "--source-endian             16~31, default 31(LE) Please refer programmer's guide or datasheet\n"},
        {"custom-map-endian",     1, NULL, 0, "--custom-map-endian         16~31, default 31(LE) Please refer programmer's guide or datasheet\n"},
        {"ref_stream_path",       1, NULL, 0, "--ref_stream_path           golden data which is compared with encoded stream when -c option\n"},
        {"srcFormat",             1, NULL, 0, "--srcFormat                 0:8bit, 1:10bit 1P2B_MSB, 2:10bit 1P2B_LSB\n"
                                              "                            1P2B : 10bit 1pixel 2bytes source format\n"},
        {"rseed",                 1, NULL, 0, "--rseed                     random seed value, Hexadecimal\n"},
        {"nonRefFbcWrite",        1, NULL, 0, "--nonRefFbcWrite            1:write all fbc data, 0:without non-Ref-Fbc Data(bandwidth reduce)\n"},
        {"add-prefix",            1, NULL, 0, "--add-prefix                add a prefix in a file-name(string)\n"},
        {"isSecureInst",          1, NULL, 0, "--isSecureInst              the instance is secure instance or non-secure instance. (WAVE6 only).\n"},
        {"instPriority",          1, NULL, 0, "--instPriority              the priority value of instance. (0 ~ 255).\n"},
        {NULL,                    0, NULL, 0},
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
            testConfig->compareType |= (1 << MODE_COMP_ENCODED);
            VLOG(TRACE, "Stream compare Enable\n");
            break;
        case 'h':
            Help(options_help, argv[0]);
            exit(0);
        case 0:
            if (!strcmp(options[idx].name, "output")) {
                osal_memcpy(testConfig->bitstreamFileName, optarg, strlen(optarg));
                ChangePathStyle(testConfig->bitstreamFileName);
            } else if (!strcmp(options[idx].name, "input")) {
                strcpy(testConfig->optYuvPath, optarg);
                ChangePathStyle(testConfig->optYuvPath);
            } else if (!strcmp(options[idx].name, "codec")) {
                testConfig->stdMode = (CodStd)atoi(optarg);
            } else if (!strcmp(options[idx].name, "cfgFileName")) {
                osal_memcpy(testConfig->cfgFileName, optarg, strlen(optarg));
            } else if (!strcmp(options[idx].name, "coreIdx")) {
                testConfig->coreIdx = atoi(optarg);
            } else if (!strcmp(options[idx].name, "picWidth")) {
                testConfig->picWidth = atoi(optarg);
            } else if (!strcmp(options[idx].name, "picHeight")) {
                testConfig->picHeight = atoi(optarg);
            } else if (!strcmp(options[idx].name, "kbps")) {
                testConfig->kbps = atoi(optarg);
#ifdef SUPPORT_SOURCE_RELEASE_INTERRUPT
            } else if (!strcmp(options[idx].name, "srcReleaseInt")) {
                testConfig->srcReleaseIntEnable = atoi(optarg);
#endif
            } else if (!strcmp(options[idx].name, "lineBufInt")) {
                testConfig->lineBufIntEn = atoi(optarg);
            } else if (!strcmp(options[idx].name, "subFrameSyncEn")) {
                testConfig->subFrameSyncEn = atoi(optarg);
            } else if (!strcmp(options[idx].name, "sfs")) {
                testConfig->subFrameSyncEn = atoi(optarg);
            } else if (!strcmp(options[idx].name, "sfs-reg-base")) {
                testConfig->subFrameSyncMode = atoi(optarg);
            } else if (!strcmp(options[idx].name, "sfs-fb-count")) {
                testConfig->subFrameSyncFbCount = atoi(optarg);
            } else if (!strcmp(options[idx].name, "nalSizeReportDataPath")) {
                osal_memcpy(testConfig->nalSizeDataPath, optarg, strlen(optarg));
                testConfig->enReportNalSize = TRUE;
            } else if (!strcmp(options[idx].name, "forced-10b")) {
                testConfig->forced_10b = atoi(optarg);
            } else if (!strcmp(options[idx].name, "last-2bit-in-8to10")) {
                testConfig->last_2bit_in_8to10 = atoi(optarg);
            } else if (!strcmp(options[idx].name, "csc-enable")) {
                testConfig->csc_enable = atoi(optarg);
            } else if (!strcmp(options[idx].name, "csc-format-order")) {
                testConfig->csc_format_order = atoi(optarg);
            } else if (!strcmp(options[idx].name, "loop-count")) {
                testConfig->loopCount = atoi(optarg);
            } else if (!strcmp(options[idx].name, "enable-cbcrInterleave")) {
                testConfig->cbcrInterleave = 1;
            } else if (!strcmp(options[idx].name, "nv21")) {
                testConfig->nv21 = atoi(optarg);
            } else if (!strcmp(options[idx].name, "i422")) {
                testConfig->i422 = atoi(optarg);
            } else if (!strcmp(options[idx].name, "packedFormat")) {
                testConfig->packedFormat = atoi(optarg);
            } else if (!strcmp(options[idx].name, "rotAngle")) {
                testConfig->rotAngle = atoi(optarg);
            } else if (!strcmp(options[idx].name, "mirDir")) {
                testConfig->mirDir = (MirrorDirection)atoi(optarg) ;
            } else if (!strcmp(options[idx].name, "secondary-axi")) {
                testConfig->secondaryAXI = atoi(optarg);
            } else if (!strcmp(options[idx].name, "frame-endian")) {
                testConfig->frame_endian = atoi(optarg);
            } else if (!strcmp(options[idx].name, "stream-endian")) {
                testConfig->stream_endian = (EndianMode)atoi(optarg);
            } else if (!strcmp(options[idx].name, "source-endian")) {
                testConfig->source_endian = atoi(optarg);
            } else if (!strcmp(options[idx].name, "custom-map-endian")) {
                testConfig->custom_map_endian = (EndianMode)atoi(optarg);
            } else if (!strcmp(options[idx].name, "ref_stream_path")) {
                osal_memcpy(testConfig->ref_stream_path, optarg, strlen(optarg));
                ChangePathStyle(testConfig->ref_stream_path);
            } else if (!strcmp(options[idx].name, "srcFormat")) {
                Uint32 temp = atoi(optarg);
                testConfig->srcFormat = FORMAT_420;
                switch (temp)
                {
                case 1: testConfig->srcFormat = FORMAT_420_P10_16BIT_MSB; break;
                case 2: testConfig->srcFormat = FORMAT_420_P10_16BIT_LSB; break;
                case 5: testConfig->srcFormat = FORMAT_422;               break;
                case 6: testConfig->srcFormat = FORMAT_422_P10_16BIT_MSB; break;
                case 7: testConfig->srcFormat = FORMAT_422_P10_16BIT_LSB; break;
                default:
                    testConfig->srcFormat = (FrameBufferFormat)temp;
                    break;
                }
            } else if (!strcmp(options[idx].name, "nonRefFbcWrite")) {
                testConfig->nonRefFbcWrite = atoi(optarg);
            } else if (!strcmp(options[idx].name, "isSecureInst")) {
                testConfig->isSecureInst = atoi(optarg);
            } else if (!strcmp(options[idx].name, "instPriority")) {
                testConfig->instPriority = atoi(optarg);
            } else if (!strcmp(options[idx].name, "rseed")) {
                randomSeed = strtoul(optarg, NULL, 16);
                srand(randomSeed);
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

    if (testConfig->csc_enable == TRUE) {
        if (testConfig->srcFormat <= FORMAT_RGB_32BIT_PACKED && testConfig->srcFormat >= FORMAT_YUV444_P10_32BIT_PACKED) {
            VLOG(ERR, "srcFormat err. support 90~93\n");
            return FALSE;
        }
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
    SetDefaultEncTestConfig(&testConfig);
    argument.argc = argc;
    argument.argv = argv;
    if (ParseArgumentAndSetTestConfig(argument, &testConfig) == FALSE) {
        VLOG(ERR, "fail to ParseArgumentAndSetTestConfig()\n");
        return 1;
    }


    if (testConfig.packedFormat) {
        testConfig.cbcrInterleave = 0;
        testConfig.nv21 = 0;
    }


    testConfig.productId = (ProductId)VPU_GetProductId(testConfig.coreIdx);
    if (CheckTestConfig(&testConfig) == FALSE) {
        VLOG(ERR, "fail to CheckTestConfig()\n");
        return 1;
    }

    switch (testConfig.productId)
    {
        case PRODUCT_ID_627:
            fwPath = WAVE627_BITCODE_PATH;
            break;
        case PRODUCT_ID_637:
            fwPath = WAVE637_BITCODE_PATH;
            break;
        case PRODUCT_ID_521:
            fwPath = WAVE521_BITCODE_PATH;
            break;
        default:
            VLOG(ERR, "Unknown product id: %d\n", testConfig.productId);
            return 1;
    }


    VLOG(TRACE, "FW PATH = %s\n", fwPath);
    if (LoadFirmware(testConfig.productId, (Uint8**)&pusBitCode, &sizeInWord, fwPath) < 0) {
        VLOG(ERR, "%s:%d Failed to load firmware: %s\n", __FUNCTION__, __LINE__, fwPath);
        return 1;
    }
    config.testEncConfig = testConfig;
    config.bitcode       = (Uint8*)pusBitCode;
    config.sizeOfBitcode = sizeInWord;

    if (SetupEncoderOpenParam(&config.encOpenParam, &config.testEncConfig, NULL) == FALSE) {
        VLOG(ERR, "SetupEncoderOpenParam error\n");
        return 1;
    }

    CNMAppInit();

    task       = CNMTaskCreate();

    yuvFeeder  = ComponentCreate("yuvfeeder",       &config);
    encoder    = ComponentCreate("wave_encoder",    &config);
    reader     = ComponentCreate("reader",          &config);

    CNMTaskAdd(task, yuvFeeder);
    CNMTaskAdd(task, encoder);
    CNMTaskAdd(task, reader);

    CNMAppAdd(task);

    if ((ret=SetupEncListenerContext(&lsnCtx, &config)) == TRUE) {
        ComponentRegisterListener(encoder, COMPONENT_EVENT_ENC_ALL, EncoderListener, (void*)&lsnCtx);
        ret = CNMAppRun(appCfg);
    } else {
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

