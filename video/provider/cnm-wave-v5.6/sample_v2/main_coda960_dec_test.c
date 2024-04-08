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
#include "decoder_listener.h"


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
    VLOG(INFO, "-n [num]                    output frame number\n");
    VLOG(INFO, "-c                          compare with golden\n");
    VLOG(INFO, "                            0 : no comparator\n");
    VLOG(INFO, "                            1 : compare with golden yuv that specified --ref-yuv option\n");
    VLOG(INFO, "-d                          dual display. A linear fb is synchronous with a compressed fb.\n");

    for (i = 0;i < MAX_GETOPT_OPTIONS;i++) {
        if (opt[i].name == NULL)
            break;
        VLOG(INFO, "%s", opt[i].help);
    }
}

static BOOL CheckTestConfig(TestDecConfig testConfig) {
    BOOL isValidParameters = TRUE;

    // Check parameters
    if (testConfig.skipMode < 0 || testConfig.skipMode == 3 || testConfig.skipMode > 4) {
        VLOG(ERR, "Invalid skip mode: %d\n", testConfig.skipMode);
        isValidParameters = FALSE;
    }

    if ((FORMAT_422 <= testConfig.wtlFormat && testConfig.wtlFormat <= FORMAT_400) ||
        testConfig.wtlFormat < FORMAT_420 || testConfig.wtlFormat > FORMAT_420_P10_32BIT_LSB) {
            VLOG(ERR, "Invalid WTL format(%d)\n", testConfig.wtlFormat);
            isValidParameters = FALSE;
    }

    if (testConfig.renderType < RENDER_DEVICE_NULL || testConfig.renderType >= RENDER_DEVICE_MAX) {
        VLOG(ERR, "unknown render device type(%d)\n", testConfig.renderType);
        isValidParameters = FALSE;
    }
    if (testConfig.thumbnailMode == TRUE && testConfig.skipMode != 0) {
        VLOG(ERR, "Turn off thumbnail mode or skip mode\n");
        isValidParameters = FALSE;
    }

    return isValidParameters;
}

static void CheckCombinationOfParamOfDecoder(TestDecConfig* testDecConfig)
{
    if(NULL ==testDecConfig) {
        VLOG(ERR, "<%s : %d>Check testConfig pointer \n", __FUNCTION__, __LINE__);
        return;
    }

    VLOG(WARN, "------------------------ WRONG PARAMETER COMBINATION ------------------------\n");

    if (FALSE == testDecConfig->coda9.enableTiled2Linear) {
        if (LINEAR_FRAME_MAP != testDecConfig->mapType) {
            VLOG(WARN, "[WARN] Enable WTL mode for tiled frame maps \n");
            testDecConfig->enableWTL    = TRUE;
            testDecConfig->wtlMode      = FF_FRAME;
        }
    }

    switch (testDecConfig->mapType) {
    case LINEAR_FRAME_MAP:
    case LINEAR_FIELD_MAP:
        if (testDecConfig->coda9.enableTiled2Linear == TRUE || testDecConfig->enableWTL == TRUE) {
            VLOG(WARN, "[WARN] It can't enable Tiled2Linear OR WTL option where maptype is LINEAR_FRAME_MAP\n");
            VLOG(WARN, "       Disable WTL or Tiled2Linear\n");
            testDecConfig->coda9.enableTiled2Linear = FALSE;
            testDecConfig->coda9.tiled2LinearMode = FF_NONE;
            testDecConfig->enableWTL = FALSE;
            testDecConfig->wtlMode = FF_NONE;
        }
        break;
    case TILED_FRAME_MB_RASTER_MAP:
    case TILED_FIELD_MB_RASTER_MAP:
        if (testDecConfig->cbcrInterleave == FALSE) {
            VLOG(WARN, "[WARN] CBCR-interleave must be enable when maptype is TILED_FRAME/FIELD_MB_RASTER_MAP.\n");
            VLOG(WARN, "       Enable cbcr-interleave\n");
            testDecConfig->cbcrInterleave = TRUE;
        }
        break;
    default:
        break;
    }

    if (testDecConfig->coda9.enableTiled2Linear == TRUE) {
        VLOG(WARN, "[WARN] In case of Tiledmap, disabled BWB for better performance.\n");
        testDecConfig->coda9.enableBWB = FALSE;
    }
    if ((testDecConfig->bitFormat == STD_DIV3 || testDecConfig->bitFormat == STD_THO) && testDecConfig->bitstreamMode != BS_MODE_PIC_END) {
        char *ext;
        VLOG(WARN, "[WARN] VPU can decode DIV3 or Theora codec on picend mode.\n");
        VLOG(WARN, "       BSMODE %d -> %d\n", BS_MODE_INTERRUPT, BS_MODE_PIC_END);

        ext = GetFileExtension(testDecConfig->inputPath);
        if (ext != NULL && (strcmp(ext, "mp4") == 0 || strcmp(ext, "mkv") == 0 || strcmp(ext, "avi") == 0 ||
                            strcmp(ext, "MP4") == 0 || strcmp(ext, "MKV") == 0 || strcmp(ext, "AVI") == 0)) {
            testDecConfig->bitstreamMode = BS_MODE_PIC_END;
        }
        if (testDecConfig->bitFormat == STD_THO) {
            testDecConfig->bitstreamMode = BS_MODE_PIC_END;
        }
    }

}

static void DisplayDecOptions(const TestDecConfig* testDecConfig)
{
    VLOG(INFO, "--------------------- CODA960(SampleV2) DECODER OPTIONS ---------------------\n");
    VLOG(INFO, "[coreIdx            ]: %d\n", testDecConfig->coreIdx);
    VLOG(INFO, "[mapType            ]: %d\n", testDecConfig->mapType);
    VLOG(INFO, "[tiled2linear       ]: %d\n", testDecConfig->coda9.enableTiled2Linear);
    VLOG(INFO, "[wtlEnable          ]: %d\n", testDecConfig->enableWTL);
    VLOG(INFO, "[wtlMode            ]: %d\n", testDecConfig->wtlMode);
    VLOG(INFO, "[bitstreamMode      ]: %d\n", testDecConfig->bitstreamMode);
    VLOG(INFO, "[cbcrInterleave     ]: %d\n", testDecConfig->cbcrInterleave);
    VLOG(INFO, "[streamEndian       ]: %d\n", testDecConfig->streamEndian);
    VLOG(INFO, "[frameEndian        ]: %d\n", testDecConfig->frameEndian);
    VLOG(INFO, "[BWB                ]: %d\n", testDecConfig->coda9.enableBWB);
    VLOG(INFO, "[rotate             ]: %d\n", testDecConfig->coda9.rotate);
    VLOG(INFO, "[mirror             ]: %d\n", testDecConfig->coda9.mirror);
    VLOG(INFO, "[mp4class           ]: %d\n", testDecConfig->coda9.mp4class);
    VLOG(INFO, "[enableDering       ]: %d\n", testDecConfig->coda9.enableDering);
    VLOG(INFO, "[enableDeblock      ]: %d\n", testDecConfig->coda9.enableDeblock);
    VLOG(INFO, "[enableMvc          ]: %d\n", testDecConfig->coda9.enableMvc);
    VLOG(INFO, "-----------------------------------------------------------------------------\n");
}

static BOOL ParseArgumentAndSetTestConfig(CommandLineArgument argument, TestDecConfig* testConfig)
{
    Int32   opt  = 0, i = 0, index = 0;
    Uint32  argFeeding = 0;            /* auto, depeding on --bsmode */
    int     argc = argument.argc;
    char**  argv = argument.argv;
    char**  stop = NULL;
    Int32   val = 0;
    char* optString = "c:hvn:u";
    struct option options[MAX_GETOPT_OPTIONS];
    struct OptionExt options_help[MAX_GETOPT_OPTIONS] = {
        {"output",                  1, NULL, 0, "--output                    output YUV path\n"},    /*  0 */
        {"input",                   1, NULL, 0, "--input                     bitstream path\n"},
        {"codec",                   1, NULL, 0, "--codec                     codec index, STD_AVC : 0, VC1 : 1, MPEG2 : 2, MPEG4 : 3, H263 : 4, DIV3 : 5, RV : 6, AVS : 7, THO : 9, VP3 : 10. VP8 : 11\n"},
        {"render",                  1, NULL, 0, "--render                    0 : no rendering picture\n"
        "                                                                  1 : render a picture with the framebuffer device\n"},
        {"bsmode",                  1, NULL, 0, "--bsmode                    0: INTERRUPT MODE, 1: reserved, 2: PICEND MODE\n"},
        {"disable-wtl",             0, NULL, 0, "--disable-wtl               disable WTL. default on.\n"},    /*  5 */
        {"coreIdx",                 1, NULL, 0, "--coreIdx                   core index: default 0\n"},
        {"loop-count",              1, NULL, 0, "--loop-count                integer number. loop test, default 0\n"},
        {"enable-cbcrinterleave",   0, NULL, 0, "--enable-cbcrinterleave     enable cbcrInterleave(NV12), default off\n"},
        {"stream-endian",           1, NULL, 0, "--stream-endian             16~31, default 31(LE) Please refer programmer's guide or datasheet\n"},
        {"frame-endian",            1, NULL, 0, "--frame-endian              16~31, default 31(LE) Please refer programmer's guide or datasheet\n"},
        {"enable-nv21",             0, NULL, 0, "--enable-nv21               enable NV21, default off\n"},
        {"secondary-axi",           1, NULL, 0, "--secondary-axi             0~7: bit oring values, Please refer programmer's guide or datasheet\n"},
        {"wtl-format",              1, NULL, 0, "--wtl-format                yuv format. default 0.\n"},
        {"enable-thumbnail",        0, NULL, 0, "--enable-thumbnail          enable thumbnail mode. default off\n"},
        {"ref-yuv",                 1, NULL, 0, "--ref-yuv                   golden yuv path\n"},
        {"skip-mode",               1, NULL, 0, "--skip-mode                 0: off, 1: Non-IRAP, 2: Non-Ref, 4: skip-all\n"},
        {"enable-mvc",              0, NULL, 0, "--enable-mvc                enable mvc\n"},
        {"userdata",                1, NULL, 0, "--userdata    hexadecimal          0 - disable userdata            4 - VUI                     10 - pic_timing                 20 - itu_t_35 prefix\n"
        "                                  40 - unregistered prefix        80 - itu_t_35 suffix        100 - unregistered suffix       400 - mastering_col_vol\n"
        "                                 800 - chroma resample hint     1000 - knee function         2000 - tone mapping             4000 - film grain\n"
        "                                8000 - content light level     10000 - color remapping   10000000 - itu_t_35 prefix 1    20000000 - itu_t_35 prefix 2\n"
        "                            40000000 - itu_t_35 suffix 1    80000000 - itu_t_35 suffix 2 \n"
        "                            All bits can be or-ing. ex) 404 -> mastering_color_volume | VUI\n"},
        {"feeding",                 1, NULL, 0, "--feeding                   0: auto, 1: fixed-size, 2: ffempg, 3: size(4byte)+es\n"},    /* 25 */
        {"rseed",                   1, NULL, 0, "--rseed                     random seed value, Hexadecimal\n"},
        {"bs-size",                 1, NULL, 0, "--bs-size size              bitstream buffer size in byte\n"},
        {"mp4class",                1, NULL, 0, "--mp4class                  0: MPEG-4, 1: divx, 2: xvid, 5: divx4, 256: sorenson spark\n"},
        {"maptype",                 1, NULL, 0, "--maptype                   CODA960: 0~6 default 0\n"},
        {"enable-dering",           0, NULL, 0, "--enable-dering             MPEG-2, MPEG-4 only\n"},
        {"enable-deblock",          0, NULL, 0, "--enable-deblock            MPEG-2, MPEG-4 only\n"},
        {"enable-tiled2linear",     0, NULL, 0, "--enable-tiled2linear       enable tiled2linear. default off\n"},
        {"rotate",                  1, NULL, 0, "--rotate                    90, 180, 270\n"},
        {"mirror",                  1, NULL, 0, "--mirror                    0: none, 1: vertical, 2: horizontal, 3: both\n"},
        {NULL,                      0, NULL,},
    };


    for (i = 0; i < MAX_GETOPT_OPTIONS;i++) {
        if (options_help[i].name == NULL)
            break;
        osal_memcpy(&options[i], &options_help[i], sizeof(struct option));
    }

    while ((opt=getopt_long(argc, argv, optString, options, (int *)&index)) != -1) {
        switch (opt) {
        case 'c':
            testConfig->compareType = atoi(optarg);
            if (testConfig->compareType < NO_COMPARE || testConfig->compareType > YUV_COMPARE) {
                VLOG(ERR, "Invalid compare type(%d)\n", testConfig->compareType);
                Help(options_help, argv[0]);
                exit(1);
            }
            break;
        case 'n':
            testConfig->forceOutNum = atoi(optarg);
            break;
        case 'h':
            Help(options_help, argv[0]);
            return FALSE;
        case 'u':
            testConfig->enableUserData = TRUE;
        case 0:
            if (!strcmp(options[index].name, "output")) {
                osal_memcpy(testConfig->outputPath, optarg, strlen(optarg));
                ChangePathStyle(testConfig->outputPath);
            } else if (!strcmp(options[index].name, "input")) {
                osal_memcpy(testConfig->inputPath, optarg, strlen(optarg));
                ChangePathStyle(testConfig->inputPath);
            } else if (!strcmp(options[index].name, "codec")) {
                testConfig->bitFormat = atoi(optarg);
            } else if (!strcmp(options[index].name, "render")) {
                testConfig->renderType = (RenderDeviceType)atoi(optarg);
                if (testConfig->renderType < RENDER_DEVICE_NULL || testConfig->renderType >= RENDER_DEVICE_MAX) {
                    VLOG(ERR, "unknown render device type(%d)\n", testConfig->renderType);
                    Help(options_help, argv[0]);
                    return FALSE;
                }
            } else if (!strcmp(options[index].name, "bsmode")) {
                testConfig->bitstreamMode = atoi(optarg);
            } else if (!strcmp(options[index].name, "disable-wtl")) {
                testConfig->enableWTL = FALSE;
            } else if (!strcmp(options[index].name, "coreIdx")) {
                testConfig->coreIdx = atoi(optarg);
            } else if (!strcmp(options[index].name, "loop-count")) {
                testConfig->loopCount = atoi(optarg);
            } else if (!strcmp(options[index].name, "enable-cbcrinterleave")) {
                testConfig->cbcrInterleave = TRUE;
            } else if (!strcmp(options[index].name, "stream-endian")) {
                testConfig->streamEndian = (EndianMode)atoi(optarg);
            } else if (!strcmp(options[index].name, "frame-endian")) {
                testConfig->frameEndian = (EndianMode)atoi(optarg);
            } else if (!strcmp(options[index].name, "enable-nv21")) {
                testConfig->nv21 = 1;
            } else if (!strcmp(options[index].name, "secondary-axi")) {
                testConfig->secondaryAXI = strtoul(optarg, stop, !strncmp("0x", optarg, 2) ? 16 : 10);
            } else if (!strcmp(options[index].name, "wtl-format")) {
                testConfig->wtlFormat = (FrameBufferFormat)atoi(optarg);
            } else if (!strcmp(options[index].name, "enable-thumbnail")) {
                testConfig->thumbnailMode = TRUE;
            } else if (!strcmp(options[index].name, "ref-yuv")) {
                osal_memcpy(testConfig->refYuvPath, optarg, strlen(optarg));
                ChangePathStyle(testConfig->refYuvPath);
            } else if (!strcmp(options[index].name, "skip-mode")) {
                testConfig->skipMode = atoi(optarg);
            } else if (!strcmp(options[index].name, "enable-mvc")) {
                testConfig->coda9.enableMvc = TRUE;
            } else if (!strcmp(options[index].name, "userdata")) {
                testConfig->enableUserData = strtoul(optarg, stop, 16);
            } else if (!strcmp(options[index].name, "feeding")) {
                argFeeding = (Uint32)atoi(optarg);
            } else if (!strcmp(options[index].name, "rseed")) {
                randomSeed = strtoul(optarg, NULL, 16);
                srand(randomSeed);
            } else if (!strcmp(options[index].name, "bs-size")) {
                testConfig->bsSize = (size_t)atoi(optarg);
            } else if (!strcmp(options[index].name, "mp4class")) {
                testConfig->coda9.mp4class = (size_t)atoi(optarg);
            } else if(!strcmp(options[index].name, "maptype")) {
                testConfig->mapType = (size_t)atoi(optarg);
            } else if(!strcmp(options[index].name, "enable-dering")) {
                testConfig->coda9.enableDering = TRUE;
            } else if(!strcmp(options[index].name, "enable-deblock")) {
                testConfig->coda9.enableDeblock = TRUE;
            } else if(!strcmp(options[index].name, "enable-tiled2linear")) {
                testConfig->coda9.enableTiled2Linear = TRUE;
                testConfig->coda9.tiled2LinearMode = FF_FRAME;
                testConfig->enableWTL = FALSE;
            } else if(!strcmp(options[index].name, "rotate")) {
                val = atoi(optarg);
                if ((val%90) != 0) {
                    VLOG(ERR, "Invalid rotation value: %d\n", val);
                    Help(options_help, argv[0]);
                    return FALSE;
                }
                testConfig->coda9.rotate = val;
            } else if(!strcmp(options[index].name, "mirror")) {
                val = atoi(optarg);
                if (val < 0 || val > 3) {
                    VLOG(ERR, "Invalid mirror option: %d\n", val);
                    Help(options_help, argv[0]);
                    return FALSE;
                }
                testConfig->coda9.mirror = val;
            }
            else {
                VLOG(ERR, "not exist param = %s\n", options[index].name);
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

    switch (argFeeding) {
    case 0: /* AUTO */
        if (testConfig->bitstreamMode == BS_MODE_INTERRUPT) {
            testConfig->feedingMode = FEEDING_METHOD_FIXED_SIZE;
        }
        else {
            testConfig->feedingMode = FEEDING_METHOD_FRAME_SIZE;
        }
        break;
    case 1: /* FIXED SIZE */
        testConfig->feedingMode = FEEDING_METHOD_FIXED_SIZE;
        break;
    case 2: /* Using FFMPEG */
        testConfig->feedingMode = FEEDING_METHOD_FRAME_SIZE;
        break;
    case 3: /* for every frame, size(4byte) + es(1frame) */
        testConfig->feedingMode = FEEDING_METHOD_SIZE_PLUS_ES;
        break;
    default:
        VLOG(ERR, "--feeding=%d not supported\n", argFeeding);
        return FALSE;
    }

    if (testConfig->bitFormat == STD_VP9) {
        testConfig->feedingMode = FEEDING_METHOD_FRAME_SIZE;
        testConfig->bitstreamMode = BS_MODE_PIC_END;
    }

    return TRUE;
}

int main(int argc, char **argv)
{
    char*               fwPath     = NULL;
    TestDecConfig       testConfig;
    CommandLineArgument argument;
    CNMComponentConfig  config;
    CNMTask             task;
    Component           feeder;
    Component           decoder;
    Component           renderer;
    Uint32              sizeInWord;
    Uint16*             pusBitCode;
    BOOL                ret;
    BOOL                testResult;
    DecListenerContext  lsnCtx;
    CNMAppConfig        appCfg = {0,};

    osal_memset(&argument, 0x00, sizeof(CommandLineArgument));
    osal_memset(&config,   0x00, sizeof(CNMComponentConfig));

    InitLog();
    Coda9SetDefaultDecTestConfig(&testConfig);

    argument.argc = argc;
    argument.argv = argv;

    if (ParseArgumentAndSetTestConfig(argument, &testConfig) == FALSE) {
        VLOG(ERR, "fail to ParseArgumentAndSetTestConfig()\n");
        return 1;
    }

    if (CheckTestConfig(testConfig) == FALSE) {
        VLOG(ERR, "fail to CheckTestConfig()\n");
        return 1;
    }


    testConfig.productId = (ProductId)VPU_GetProductId(testConfig.coreIdx);
    switch (testConfig.productId) {
    case PRODUCT_ID_960: fwPath = CODA960_BITCODE_PATH; break;
    case PRODUCT_ID_980: fwPath = CODA980_BITCODE_PATH; break;
    default:
        VLOG(ERR, "Unknown product id: %d\n", testConfig.productId);
        return 1;
    }

    VLOG(INFO, "FW PATH = %s\n", fwPath);
    if (LoadFirmware(testConfig.productId, (Uint8**)&pusBitCode, &sizeInWord, fwPath) < 0) {
        VLOG(ERR, "%s:%d Failed to load firmware: %s\n", __FUNCTION__, __LINE__, fwPath);
        return 1;
    }

    config.testDecConfig = testConfig;
    config.bitcode       = (Uint8*)pusBitCode;
    config.sizeOfBitcode = sizeInWord;

    //Check and change option parameters for CODA960
    CheckCombinationOfParamOfDecoder(&(config.testDecConfig));
    DisplayDecOptions(&(config.testDecConfig));

    if (RETCODE_SUCCESS != (ret=SetUpDecoderOpenParam(&(config.decOpenParam), &(config.testDecConfig)))) {
        VLOG(ERR, "%s:%d SetUpDecoderOpenParam failed Error code is 0x%x \n", __FUNCTION__, __LINE__, ret);
        return 1;
    }

    CNMAppInit();

    task     = CNMTaskCreate();
    feeder   = ComponentCreate("feeder",        &config);
    decoder  = ComponentCreate("coda9_decoder", &config);
    renderer = ComponentCreate("renderer",&config);

    CNMTaskAdd(task, feeder);
    CNMTaskAdd(task, decoder);
    CNMTaskAdd(task, renderer);
    CNMAppAdd(task);

    //Up to here, preparation for decoding

    if ((ret=SetupDecListenerContext(&lsnCtx, &config, renderer)) == TRUE) {
        ComponentRegisterListener(decoder, COMPONENT_EVENT_DEC_ALL, DecoderListener, (void*)&lsnCtx);
        ret = CNMAppRun(appCfg); //  Run bitstreem feeding, decoding, rendering
    }
    else {
        CNMAppStop();
    }

    osal_free(pusBitCode);
    ClearDecListenerContext(&lsnCtx);
    DeInitLog();

    testResult = (ret == TRUE && lsnCtx.match == TRUE);
    VLOG(INFO, "[RESULT] %s\n", (testResult == TRUE)  ? "SUCCESS" : "FAILURE");

    DeInitLog();
    if ( CNMErrorGet() != 0 )
        return CNMErrorGet();

    return testResult == TRUE ? 0 : 1;
}

