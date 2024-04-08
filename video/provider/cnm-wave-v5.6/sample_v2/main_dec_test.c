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

static BOOL CheckTestConfig(TestDecConfig* testConfig) {
    BOOL isValidParameters = TRUE;
    const TestDecConfig* valid_config   = testConfig;
    TestDecConfig* reset_config         = testConfig;

    /* Check parameters */
    if (valid_config->skipMode < 0 || valid_config->skipMode == 3 || valid_config->skipMode > 4) {
        VLOG(ERR, "Invalid skip mode: %d\n", valid_config->skipMode);
        isValidParameters = FALSE;
    }
    if ((FORMAT_224 <= valid_config->wtlFormat && valid_config->wtlFormat <= FORMAT_400) ||
        valid_config->wtlFormat < FORMAT_420 || valid_config->wtlFormat > FORMAT_422_P10_32BIT_LSB) {
            VLOG(ERR, "Invalid WTL format(%d)\n", valid_config->wtlFormat);
            isValidParameters = FALSE;
    }
    if (valid_config->renderType < RENDER_DEVICE_NULL || valid_config->renderType >= RENDER_DEVICE_MAX) {
        VLOG(ERR, "unknown render device type(%d)\n", valid_config->renderType);
        isValidParameters = FALSE;
    }
    if (valid_config->thumbnailMode == TRUE && valid_config->skipMode != 0) {
        VLOG(ERR, "Turn off thumbnail mode or skip mode\n");
        isValidParameters = FALSE;
    }

    if (STD_AVC == valid_config->bitFormat) {
        if (valid_config->productId != PRODUCT_ID_521 && valid_config->productId != PRODUCT_ID_511) {
            isValidParameters = (valid_config->secondaryAXI&0x04) ? FALSE : TRUE;
            if (!isValidParameters) {
                VLOG(ERR, "AVC do not support  tBPU secondary AXI. \n");
            }
        }
    } else if (STD_AV1 == valid_config->bitFormat) {
        isValidParameters = (valid_config->secondaryAXI&0x04) ? FALSE : TRUE;
        if (!isValidParameters) {
            VLOG(ERR, "AVC/AV1  do not support  tBPU secondary AXI. \n");
        }
    }


    if (valid_config->productId == PRODUCT_ID_637 || valid_config->productId == PRODUCT_ID_617) {
        if (valid_config->bitstreamMode != BS_MODE_PIC_END) {
            VLOG(WARN, "WAVE6xx Decoder only support pic end mode \n");
            reset_config->feedingMode = FEEDING_METHOD_FRAME_SIZE;
            reset_config->bitstreamMode = BS_MODE_PIC_END;
        }
    }

    return isValidParameters;
}

static BOOL ParseArgumentAndSetTestConfig(CommandLineArgument argument, TestDecConfig* testConfig)
{
    Int32  opt  = 0, i = 0, index = 0;
    Uint32 argFeeding = 0;            /* auto, depeding on --bsmode */
    int    argc = argument.argc;
    char** argv = argument.argv;
    char** stop = NULL;
    char* optString = "c:hvn:";

    struct option options[MAX_GETOPT_OPTIONS];
    struct OptionExt options_help[MAX_GETOPT_OPTIONS] = {
        {"output",                1, NULL, 0, "--output                    output YUV path\n"},    /*  0 */
        {"xml-output",            1, NULL, 0, "--xml-output                xml output file path\n"},
        {"input",                 1, NULL, 0, "--input                     bitstream path\n"},
        {"codec",                 1, NULL, 0, "--codec                     codec index, HEVC : 12, VP9 : 13, AVS2 : 14, AVC : 0\n"},
        {"render",                1, NULL, 0, "--render                    0 : no rendering picture\n"
        "                                                                  1 : render a picture with the framebuffer device\n"},
        {"bsmode",                1, NULL, 0, "--bsmode                    0: INTERRUPT MODE, 1: reserved, 2: PICEND MODE\n"},
        {"disable-wtl",           0, NULL, 0, "--disable-wtl               disable WTL. default on.\n"},    /*  5 */
        {"cropped-display",       1, NULL, 0, "--cropped-display           cropped decoded picture \n"},
        {"coreIdx",               1, NULL, 0, "--coreIdx                   core index: default 0\n"},
        {"loop-count",            1, NULL, 0, "--loop-count                integer number. loop test, default 0\n"},
        {"enable-cbcrinterleave", 0, NULL, 0, "--enable-cbcrinterleave     enable cbcrInterleave(NV12), default off\n"},
        {"stream-endian",         1, NULL, 0, "--stream-endian             16~31, default 31(LE) Please refer programmer's guide or datasheet\n"},
        {"frame-endian",          1, NULL, 0, "--frame-endian              16~31, default 31(LE) Please refer programmer's guide or datasheet\n"},
        {"enable-nv21",           0, NULL, 0, "--enable-nv21               enable NV21, default off\n"},
        {"secondary-axi",         1, NULL, 0, "--secondary-axi             0~7: bit oring values, Please refer programmer's guide or datasheet\n"},
        {"wtl-format",            1, NULL, 0, "--wtl-format                yuv or rgb format. default 0.\n"},
        {"disable-reorder",       0, NULL, 0, "--disable-reorder           disable re-order.\n"},
        {"enable-thumbnail",      0, NULL, 0, "--enable-thumbnail          enable thumbnail mode. default off\n"},
        {"ref-yuv",               1, NULL, 0, "--ref-yuv                   golden yuv path\n"},
        {"skip-mode",             1, NULL, 0, "--skip-mode                 0: off, 1: Non-IRAP, 2: Non-Ref, 4: skip-all\n"},
        {"bwopt",                 1, NULL, 0, "--bwopt                     bandwidth opimization\n"},
        {"err-conceal-mode",      1, NULL, 0, "--err-conceal-mode          error concealment mode \n"},
        {"err-conceal-unit",      1, NULL, 0, "--err-conceal-unit          error concealment unit \n"},
        {"sclw",                  1, NULL, 0, "--sclw                      scale width of picture down. ceil8(width/8) <= scaledW <= width, 99 for random test\n"},    /* 20 */
        {"sclh",                  1, NULL, 0, "--sclh                      scale height of picture down, ceil8(height/8) <= scaledH <= height, 99 for random test\n"},
        {"scl-coef-mode",         1, NULL, 0, "--scl-coef-mode             0 : Bicubic sharpness parameter 0.5, 1 : Bicubic sharpness parameter 1.0, 2 : Bicubic sharpness parameter 1.5, 3 : Lanczos-2\n"},
        {"cra-bla",               0, NULL, 0, "--cra-bla                   Handle CRA as BLA\n"},
        {"userdata",              1, NULL, 0, "--userdata   hexadecimal(hevc)     0 - disable userdata            4 - VUI                     10 - pic_timing                 20 - itu_t_35 prefix\n"
                                              "                                  40 - unregistered prefix        80 - itu_t_35 suffix        100 - unregistered suffix       400 - mastering_col_vol\n"
                                              "                                 800 - chroma resample hint     1000 - knee function         2000 - tone mapping             4000 - film grain\n"
                                              "                                8000 - content light level     10000 - color remapping   10000000 - itu_t_35 prefix 1    20000000 - itu_t_35 prefix 2\n"
                                              "                            40000000 - itu_t_35 suffix 1    80000000 - itu_t_35 suffix 2 \n"
                                              "--userdata   hexadecimal(avc)      0 - disable userdata            4 - VUI                     10 - pic_timing                 20 - itu_t_35\n"
                                              "                                  40 - unregistered               80 - reserved               100 - reserved                  400 - reserved\n"
                                              "                                 800 - reserved                 1000 - reserved              2000 - tone mapping             4000 - film grain\n"
                                              "                                8000 - reserved                10000 - color remapping   10000000 - reserved             20000000 - reserved\n"
                                              "                            All bits can be or-ing. ex) 404 -> mastering_color_volume | VUI\n"},
        {"feeding",               1, NULL, 0, "--feeding                   0: auto, 1: fixed-size, 2: ffempg, 3: size(4byte)+es\n"},    /* 25 */
        {"pf-clock",              1, NULL, 0, "--pf-clock                  performance measure clock in Hz. BCLK\n"},
        {"pf",                    1, NULL, 0, "--pf                        1:enable performance measurement\n"},
        {"fps",                   1, NULL, 0, "--fps                       frames per second, default 30, valid values 30 or 60\n"},
        {"rseed",                 1, NULL, 0, "--rseed                     random seed value, Hexadecimal\n"},
        {"av1-format",            1, NULL, 0, "--av1-format                temp parameter to seperate out av1 stream format. 0: ivf 1: obu 2: annexb\n"},
        {"nonRefFbcWrite",        1, NULL, 0, "--nonRefFbcWrite            1:write all fbc data, 0:without non-Ref-Fbc Data(bandwidth reduce)\n"},
        {NULL,                    0, NULL, 0},
    };
    Uint32  minScaleWidth, minScaleHeight;


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
        case 0:
            if (!strcmp(options[index].name, "output")) {
                osal_memcpy(testConfig->outputPath, optarg, strlen(optarg));
                ChangePathStyle(testConfig->outputPath);
            } else if (!strcmp(options[index].name, "xml-output")) {
                osal_memcpy(testConfig->xmlOutputPath, optarg, strlen(optarg));
                ChangePathStyle(testConfig->xmlOutputPath);
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
                testConfig->wtlMode = FF_NONE;
            } else if (!strcmp(options[index].name, "cropped-display")) {
                testConfig->enableCrop = TRUE;
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
            } else if (!strcmp(options[index].name, "disable-reorder")) {
                testConfig->disable_reorder = TRUE;
            } else if (!strcmp(options[index].name, "enable-thumbnail")) {
                testConfig->thumbnailMode = TRUE;
            } else if (!strcmp(options[index].name, "ref-yuv")) {
                osal_memcpy(testConfig->refYuvPath, optarg, strlen(optarg));
                ChangePathStyle(testConfig->refYuvPath);
            } else if (!strcmp(options[index].name, "skip-mode")) {
                testConfig->skipMode = atoi(optarg);
            } else if (!strcmp(options[index].name, "err-conceal-mode")) {
                testConfig->errConcealMode = atoi(optarg);
            } else if (!strcmp(options[index].name, "err-conceal-unit")) {
                testConfig->errConcealUnit = atoi(optarg);
            } else if (!strcmp(options[index].name, "sclw")) {
                testConfig->scaleDownWidth = VPU_CEIL(atoi(optarg), 2);
            } else if (!strcmp(options[index].name, "sclh")) {
                testConfig->scaleDownHeight = VPU_CEIL(atoi(optarg), 2);
            } else if (!strcmp(options[index].name, "scl-coef-mode")) {
                testConfig->sclCoefMode = atoi(optarg);
            } else if (!strcmp(options[index].name, "cra-bla")) {
                testConfig->wave.craAsBla = TRUE;
            } else if (!strcmp(options[index].name, "userdata")) {
                testConfig->enableUserData = strtoul(optarg, stop, 16);
            } else if (!strcmp(options[index].name, "feeding")) {
                argFeeding = (Uint32)atoi(optarg);
            } else if (!strcmp(options[index].name, "pf")) {
                testConfig->performance = atoi(optarg);
            } else if (!strcmp(options[index].name, "pf-clock")) {
                testConfig->pfClock = atoi(optarg);
            } else if (!strcmp(options[index].name, "fps")) {
                testConfig->fps = atoi(optarg);
            } else if (!strcmp(options[index].name, "nonRefFbcWrite")) {
                testConfig->nonRefFbcWrite = atoi(optarg);
            } else if (!strcmp(options[index].name, "fw-path")) {
                osal_memcpy(testConfig->fwPath, optarg, strlen(optarg));
            } else if (!strcmp(options[index].name, "av1-format")) {
                testConfig->wave.av1Format = atoi(optarg);
            } else if (!strcmp(options[index].name, "rseed")) {
                randomSeed = strtoul(optarg, NULL, 16);
                srand(randomSeed);
            } else if (!strcmp(options[index].name, "bs-size")) {
                testConfig->bsSize = (size_t)atoi(optarg);
            } else {
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
        } else {
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
    case 4:
        testConfig->feedingMode = FEEDING_METHOD_HOST_FRAME_SIZE;
        break;
    default:
        VLOG(ERR, "--feeding=%d not supported\n", argFeeding);
        return FALSE;
        }

    if (testConfig->bitFormat == STD_VP9) {
        testConfig->feedingMode = FEEDING_METHOD_FRAME_SIZE;
        testConfig->bitstreamMode = BS_MODE_PIC_END;
    }

    minScaleWidth  = 8;
    minScaleHeight = 8;

    if (testConfig->scaleDownWidth != 0 && testConfig->scaleDownWidth < minScaleWidth) {
        VLOG(ERR, "The minimum scale-downed width: %d\n", minScaleWidth);
        return FALSE;
    }
    if (testConfig->scaleDownHeight != 0 && testConfig->scaleDownHeight < minScaleHeight) {
        VLOG(ERR, "The minimum scale-downed height: %d\n", minScaleHeight);
        return FALSE;
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
    SetDefaultDecTestConfig(&testConfig);

    argument.argc = argc;
    argument.argv = argv;
    if (ParseArgumentAndSetTestConfig(argument, &testConfig) == FALSE) {
        VLOG(ERR, "fail to ParseArgumentAndSetTestConfig()\n");
        return 1;
    }


    /* Need to Add */
    /* FW load & show version case*/
    testConfig.productId  = (ProductId)VPU_GetProductId(testConfig.coreIdx);

    switch (testConfig.productId) {
    case PRODUCT_ID_521: fwPath = WAVE521_BITCODE_PATH; break;
    case PRODUCT_ID_511: fwPath = WAVE511_BITCODE_PATH; break;
    case PRODUCT_ID_517: fwPath = WAVE517_BITCODE_PATH; break;
    case PRODUCT_ID_617: fwPath = WAVE637_BITCODE_PATH; break;
    case PRODUCT_ID_637: fwPath = WAVE637_BITCODE_PATH; break;
    default:
        VLOG(ERR, "Unknown product id: %d\n", testConfig.productId);
        return 1;
    }

    if (CheckTestConfig(&testConfig) == FALSE) {
        VLOG(ERR, "fail to CheckTestConfig()\n");
        return 1;
    }

    VLOG(TRACE, "FW PATH = %s\n", fwPath);
    if (LoadFirmware(testConfig.productId, (Uint8**)&pusBitCode, &sizeInWord, fwPath) < 0) {
        VLOG(ERR, "%s:%d Failed to load firmware: %s\n", __FUNCTION__, __LINE__, fwPath);
        return 1;
    }

    config.testDecConfig = testConfig;
    config.bitcode       = (Uint8*)pusBitCode;
    config.sizeOfBitcode = sizeInWord;

    if (RETCODE_SUCCESS != (ret=SetUpDecoderOpenParam(&(config.decOpenParam), &(config.testDecConfig)))) {
        VLOG(ERR, "%s:%d SetUpDecoderOpenParam failed Error code is 0x%x \n", __FUNCTION__, __LINE__, ret);
        return 1;
    }

    CNMAppInit();

    task     = CNMTaskCreate();
    feeder   = ComponentCreate("feeder",   &config);
    decoder  = ComponentCreate("wave_decoder",  &config);
    renderer = ComponentCreate("renderer", &config);

    CNMTaskAdd(task, feeder);
    CNMTaskAdd(task, decoder);
    CNMTaskAdd(task, renderer);
    CNMAppAdd(task);

    if ((ret=SetupDecListenerContext(&lsnCtx, &config, renderer)) == TRUE) {
        ComponentRegisterListener(decoder, COMPONENT_EVENT_DEC_ALL, DecoderListener, (void*)&lsnCtx);
        ret = CNMAppRun(appCfg);
    }
    else {
        CNMAppStop();
    }

    osal_free(pusBitCode);
    ClearDecListenerContext(&lsnCtx);


    testResult = (ret == TRUE && lsnCtx.match == TRUE && CNMErrorGet() == 0);


    VLOG(INFO, "[RESULT] %s\n", (testResult == TRUE)  ? "SUCCESS" : "FAILURE");

    DeInitLog();

    if (testResult != TRUE && CNMErrorGet() != 0 )
        return CNMErrorGet();

    return testResult == TRUE ? 0 : 1;
}

