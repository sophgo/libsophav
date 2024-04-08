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
#include <string.h>
#include <stdarg.h>

#include "getopt.h"
#include "vpuapi.h"
#include "vpuapifunc.h"
#include "main_helper.h"
#include "vdi.h"
#include "new_map_conv.h"
#include "wave/wave5_regdefine.h"


#define MAPI_TEST_MAX_LOG_MSG           128
static int log_idx = 0;
static char log_msg[MAPI_TEST_MAX_LOG_MSG][MAPI_TEST_MAX_LOG_MSG]; // 128x128

typedef struct CommandLineArgument{
    int argc;
    char **argv;
}CommandLineArgument;

static void PutLogMsg(const char *format, ...)
{
    va_list ptr;
    char    logBuf[MAX_PRINT_LENGTH] = {0};
    //char*   prefix = "";
    //char*   postfix= "";

    if (log_idx < MAPI_TEST_MAX_LOG_MSG) {
        va_start( ptr, format );
        vsnprintf(logBuf, MAX_PRINT_LENGTH, format, ptr );
        va_end(ptr);
        osal_memcpy(log_msg[log_idx], logBuf, MAPI_TEST_MAX_LOG_MSG-1);
        log_idx++;
    } else {
        VLOG(WARN, "LOG MESSAGES BUFFER FULLED \n");
    }
}

static void ShowLogMsg()
{
    Int32 idx = 0;

    VLOG(INFO, "\n***** [MAPI_TEST_INFO] *****\n");
    for (; idx < log_idx; idx ++) {
        VLOG(INFO, "[MAPI_TEST] %s", log_msg[idx]);
    }
}

static char* GetCodecName(Int32 std_idx)
{
    char *codec_str = NULL;
    switch (std_idx)
    {
    case STD_AVC: codec_str = "AVC"; break;
    case STD_HEVC: codec_str = "HEVC"; break;
    case STD_AVS2: codec_str = "AVS2"; break;
    case STD_VP9: codec_str = "VP9"; break;
    case STD_AV1: codec_str = "AV1"; break;
    }

    return codec_str;
}

static void DispPreRunMsg()
{
    VLOG(INFO, "NO   PATH_IDX   CODEC       W*H       fb_size   fb_y_size   fb_cb_size   fb_cb_size   fb_y_arr   fb_c_arr\n");
    VLOG(INFO, "-------------------------------------------------------------------------------------------------------------------------\n");
}

static void DispPostRunMsg(const TestMapConvConfig* p_test_config)
{
    static Uint32 run_idx = 0;
    Int32 width = 0, height = 0, fb_size, fb_y_size, fb_cb_size, fb_cr_size;
    Int32 idx = 0;
    Int32 output_fb_idx = 0;
    PhysicalAddress y_addr, c_addr;

    if (p_test_config->output_fb_num > 1) {
        output_fb_idx = (p_test_config->frame_idx % p_test_config->output_fb_num);
    }

    for (idx = 0; idx < MAPI_TEST_MAX_OUTPUT_PATH; idx++) {
        if (p_test_config->out_mode[idx] > MAPI_O_MODE_NONE) {
            width = (p_test_config->scl_width[idx] != 0) ? p_test_config->scl_width[idx] : p_test_config->width;
            height = (p_test_config->scl_height[idx] != 0) ? p_test_config->scl_height[idx] : p_test_config->height;
            y_addr = p_test_config->out_frame_buffer[idx][output_fb_idx].bufY;
            c_addr = p_test_config->out_frame_buffer[idx][output_fb_idx].bufCb;
            fb_size = p_test_config->out_frame_buffer[idx][output_fb_idx].size;
            fb_y_size = p_test_config->out_frame_buffer[idx][output_fb_idx].bufYSize;
            fb_cb_size = p_test_config->out_frame_buffer[idx][output_fb_idx].bufCbSize;
            fb_cr_size = p_test_config->out_frame_buffer[idx][output_fb_idx].bufCrSize;
            VLOG(INFO, "%2d   %2d          %3s     %2dx%2d   %7d   %8d   %10d   %10d   0x%6x   0x%7x\n", \
                run_idx, 0, GetCodecName(p_test_config->bitFormat), width, height, fb_size, fb_y_size, fb_cb_size, fb_cr_size, y_addr, c_addr);
        }
    }
    run_idx++;
}

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

static BOOL ParseArgumentAndSetTestConfig(CommandLineArgument argument, TestMapConvConfig* testConfig)
{
    Int32  opt  = 0, i = 0, idx = 0, tok_idx = 0;
    int    argc = argument.argc;
    char** argv = argument.argv;
    char* tempArg;
    char* optString = "c:hvn:";
    struct option options[MAX_GETOPT_OPTIONS];
    struct OptionExt options_help[MAX_GETOPT_OPTIONS] = {
        {"output",                  1, NULL, 0, "--output                    output data path\n"},
        {"input",                   1, NULL, 0, "--input                     input data path\n"},
        {"codec",                   1, NULL, 0, "--codec                     codec idx, AVC : 0, HEVC : 12, VP9 : 13, AVS2 : 14, AV1 : 0\n"},
        {"width",                   1, NULL, 0, "--width                     picture width \n"},
        {"height",                  1, NULL, 0, "--height                    picture height \n"},
        {"input-endian",            1, NULL, 0, "--input-endian              16~31, default 31(LE) Please refer programmer's guide or datasheet\n"},
        {"output-endian",           1, NULL, 0, "--output-endian             16~31, default 31(LE) Please refer programmer's guide or datasheet\n"},
        {"in-nv21",                 1, NULL, 0, "--in-nv21                   0: disable(default) nv21 1: enable nv21 \n"},
        {"out-nv21",                1, NULL, 0, "--out-nv21                  0: disable(default) nv21 1: enable nv21 \n"},
        {"in-mapi-format",          1, NULL, 0, "--in-mapi-format            input data format. default 0.\n"},
        {"out-mapi-format",         1, NULL, 0, "--out-mapi-format           output data format. default 0.\n"},
        {"out-mapi-format-path-1",  1, NULL, 0, "--out-mapi-format-path-1    output data format for path 1. default 0.\n"},
        {"input_display_2port",     1, NULL, 0, "--input_display_2port       input display 2port mode\n"},
        {"output_display_2port",    1, NULL, 0, "--output_display_2port      output display 2port mode\n"},
        {"ref-path",                1, NULL, 0, "--ref-path                  golden data path 0\n"},
        {"input-fb-num",            1, NULL, 0, "--input-fb-num              number of input buffers, default 1 \n"},
        {"output-fb-num",           1, NULL, 0, "--output-fb-num             number of output buffers, default 1 \n"},
        {"in-ctrl-mode",            1, NULL, 0, "--in-ctrl-mode              1: DISPLAY, 2: INPUT_CF10, 4:RDMA\n"},
        {"out-ctrl-mode",           1, NULL, 0, "--out-ctrl-mode             0: ERR, 1: DISPLAY, 2: RESERVED_1, 3: RESERVED_2, 4: WDMA, 5:DISP_WDMA\n"},
        {"rgb-gcr-coef",            1, NULL, 0, "--rgb-gcr-coef              yc2rgb gcr coef \n"},
        {"rgb-rcr-coef",            1, NULL, 0, "--rgb-rcr-coef              yc2rgb rcr coef \n"},
        {"rgb-bcb-coef",            1, NULL, 0, "--rgb-bcb-coef              yc2rgb bcb coef \n"},
        {"rgb-gcb-coef",            1, NULL, 0, "--rgb-gcb-coef              yc2rgb gcb coef \n"},
        {"enable-fgn",              1, NULL, 0, "--enable-fgn                1: enable fgn (default 0, off) \n"},
        {"av1-otbg64",              1, NULL, 0, "--av1-otbg64                1: av1-otbg64(offset table burst group 64x64 (default 0 : (256x16)) \n"},
        {"avs2_10b_to_8b",          1, NULL, 0, "--avs2_10b_to_8b            1: enable 10bit to 8bit in avs2\n"},
        {"mono-chrome",             1, NULL, 0, "--mono-chrome               1: enable mono-chrome\n"},
        {"fbc-dir",                 1, NULL, 0, "--fbc-dir                   directory of fbc data, ex) fbc_dir/f000_fbc_data_y.bin (default fbc_result/)\n"},
        {"sclw",                    1, NULL, 0, "--sclw                      scale width of picture down. ceil4(width/8) <= scaledW <= width, 99 for random test\n"},
        {"sclh",                    1, NULL, 0, "--sclh                      scale height of picture down, ceil4(height/8) <= scaledH <= height, 99 for random test\n"},
        {"enableUserPhase",         1, NULL, 0, "--enableUserPhase           0: Phase values are set by API automatically. 1:Phase values are set by User input\n"},
        {"scl-y-hor-phase-0",       1, NULL, 0, "--scl-y-hor-phase-0         Luma horiziontal Phase value for path 0          (-512 ~ 511)\n"},
        {"scl-y-ver-phase-0",       1, NULL, 0, "--scl-y-ver-phase-0         Luma vertical Phase value for path 0             (-512 ~ 511)\n"},
        {"scl-c-hor-phase-0",       1, NULL, 0, "--scl-c-hor-phase-0         Chroma horiziontal Phase value for path 0        (-512 ~ 511)\n"},
        {"scl-c-ver-phase-0",       1, NULL, 0, "--scl-c-ver-phase-0         Chroma vertical Phase value for path 0           (-512 ~ 511)\n"},
        {"scl-y-hor-phase-1",       1, NULL, 0, "--scl-y-hor-phase-1         Luma horiziontal Phase value for path 1          (-512 ~ 511)\n"},
        {"scl-y-ver-phase-1",       1, NULL, 0, "--scl-y-ver-phase-1         Luma vertical Phase value for path 1             (-512 ~ 511)\n"},
        {"scl-c-hor-phase-1",       1, NULL, 0, "--scl-c-hor-phase-1         Chroma horiziontal Phase value for path 1        (-512 ~ 511)\n"},
        {"scl-c-ver-phase-1",       1, NULL, 0, "--scl-c-ver-phase-1         Chroma vertical Phase value for path 1           (-512 ~ 511)\n"},
        {"bilinear_mode",           1, NULL, 0, "--bilinear_mode             bilinear mode. 0 : disenable, 1 : phase 0.5, 2 : phase 0 (default=0)\n"},
        {"scaler_mode",             1, NULL, 0, "--scaler_mode               Scaler type and sharpness. 0 : cubic sharpness = 0.5, 1 : cubic sharpness = 1, 2 : cubic sharpness = 1.5, 3 : lancoz (default=1)\n"},
        {NULL,                    0, NULL, 0},
    };
    for (i = 0; i < MAX_GETOPT_OPTIONS;i++) {
        if (options_help[i].name == NULL)
            break;
        osal_memcpy(&options[i], &options_help[i], sizeof(struct option));
    }
    while ((opt=getopt_long(argc, argv, optString, options, (int *)&idx)) != -1) {
        switch (opt) {
        case 'c':
            testConfig->compareType = atoi(optarg);
            break;
        case 'n':
            testConfig->forceOutNum = atoi(optarg);
            break;
        case 'h':
            Help(options_help, argv[0]);
            return FALSE;
        case 0:
            if (!strcmp(options[idx].name, "output")) {
                tempArg = strtok(optarg, ",");
                for (tok_idx = 0; tok_idx < MAPI_TEST_MAX_OUTPUT_PATH; tok_idx++) {
                    if (tempArg == NULL) break;
                    osal_memcpy(testConfig->outputPath[tok_idx], tempArg, strlen(tempArg));
                    ChangePathStyle(testConfig->outputPath[tok_idx]);
                    tempArg = strtok(NULL, ",");
                }
            } else if (!strcmp(options[idx].name, "input")) {
                osal_memcpy(testConfig->inputPath, optarg, strlen(optarg));
                ChangePathStyle(testConfig->inputPath);
            } else if (!strcmp(options[idx].name, "codec")) {
                testConfig->bitFormat = atoi(optarg);
            } else if (!strcmp(options[idx].name, "width")) {
                testConfig->width = atoi(optarg);
            } else if (!strcmp(options[idx].name, "height")) {
                testConfig->height = atoi(optarg);
            } else if (!strcmp(options[idx].name, "input-endian")) {
                testConfig->inputEndian = (EndianMode)atoi(optarg);
            } else if (!strcmp(options[idx].name, "output-endian")) {
                testConfig->outputEndian = (EndianMode)atoi(optarg);
            } else if (!strcmp(options[idx].name, "in-nv21")) {
                testConfig->in_nv21 = atoi(optarg);
            } else if (!strcmp(options[idx].name, "out-nv21")) {
                tempArg = strtok(optarg, ",");
                for (tok_idx = 0; tok_idx < MAPI_TEST_MAX_OUTPUT_PATH; tok_idx++) {
                    if (tempArg == NULL) break;
                    testConfig->out_nv21[tok_idx] = (BOOL)atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                }
            } else if (!strcmp(options[idx].name, "in-mapi-format")) {
                testConfig->input_buf_format = (FrameMapConvBufferFormat)atoi(optarg);
            } else if (!strcmp(options[idx].name, "out-mapi-format")) {
                tempArg = strtok(optarg, ",");
                for (tok_idx = 0; tok_idx < MAPI_TEST_MAX_OUTPUT_PATH; tok_idx++) {
                    if (tempArg == NULL) break;
                    testConfig->output_buf_format[tok_idx] = (FrameMapConvBufferFormat)atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                }
            } else if (!strcmp(options[idx].name, "input_display_2port")) {
                testConfig->input_display_2port = atoi(optarg);
            } else if (!strcmp(options[idx].name, "output_display_2port")) {
                testConfig->output_display_2port = atoi(optarg);
            } else if (!strcmp(options[idx].name, "ref-path")) {
                tempArg = strtok(optarg, ",");
                for (tok_idx = 0; tok_idx < MAPI_TEST_MAX_OUTPUT_PATH; tok_idx++) {
                    if (tempArg == NULL) break;
                    osal_memcpy(testConfig->ref_path[tok_idx], tempArg, strlen(tempArg));
                    ChangePathStyle(testConfig->ref_path[tok_idx]);
                    tempArg = strtok(NULL, ",");
                }
            } else if (!strcmp(options[idx].name, "input-fb-num")) {
                testConfig->input_fb_num = atoi(optarg);
            } else if (!strcmp(options[idx].name, "output-fb-num")) {
                testConfig->output_fb_num = atoi(optarg);
            } else if (!strcmp(options[idx].name, "in-ctrl-mode")) {
                testConfig->input_ctrl_mode = (InputCtrlMode)atoi(optarg);
            } else if (!strcmp(options[idx].name, "out-ctrl-mode")) {
                testConfig->output_ctrl_mode = (OutputCtrlMode)atoi(optarg);
            } else if (!strcmp(options[idx].name, "rgb-gcr-coef")) {
                testConfig->mapi_ctrl.mapi_output_path[0].csc.coefGCR = (OutputCtrlMode)atoi(optarg);
                testConfig->mapi_ctrl.mapi_output_path[1].csc.coefGCR = (OutputCtrlMode)atoi(optarg);
            } else if (!strcmp(options[idx].name, "rgb-rcr-coef")) {
                testConfig->mapi_ctrl.mapi_output_path[0].csc.coefRCR = (OutputCtrlMode)atoi(optarg);
                testConfig->mapi_ctrl.mapi_output_path[1].csc.coefRCR = (OutputCtrlMode)atoi(optarg);
            } else if (!strcmp(options[idx].name, "rgb-bcb-coef")) {
                testConfig->mapi_ctrl.mapi_output_path[0].csc.coefBCB = (OutputCtrlMode)atoi(optarg);
                testConfig->mapi_ctrl.mapi_output_path[1].csc.coefBCB = (OutputCtrlMode)atoi(optarg);
            } else if (!strcmp(options[idx].name, "rgb-gcb-coef")) {
                testConfig->mapi_ctrl.mapi_output_path[0].csc.coefGCB = (OutputCtrlMode)atoi(optarg);
                testConfig->mapi_ctrl.mapi_output_path[1].csc.coefGCB = (OutputCtrlMode)atoi(optarg);
            } else if (!strcmp(options[idx].name, "enable-fgn")) {
                testConfig->enable_fgn =  atoi(optarg);
            } else if (!strcmp(options[idx].name, "av1-otbg64")) {
                testConfig->is_offset_tb_64 =  atoi(optarg);
            } else if (!strcmp(options[idx].name, "mono-chrome")) {
                testConfig->is_monochrome =  atoi(optarg);
            } else if (!strcmp(options[idx].name, "avs2_10b_to_8b")) {
                testConfig->cf10_10b_to_8b =  atoi(optarg);
            } else if (!strcmp(options[idx].name, "fbc-dir")) {
                osal_memcpy(testConfig->fbc_dir, optarg, strlen(optarg));
                ChangePathStyle(testConfig->fbc_dir);
            } else if (!strcmp(options[idx].name, "sclw")) {
                tempArg = strtok(optarg, ",");
                for (tok_idx = 0; tok_idx < MAPI_TEST_MAX_OUTPUT_PATH; tok_idx++) {
                    if (tempArg == NULL) break;
                    testConfig->scl_width[tok_idx] = VPU_CEIL(atoi(tempArg), 2);
                    tempArg = strtok(NULL, ",");
                }
            } else if (!strcmp(options[idx].name, "sclh")) {
                tempArg = strtok(optarg, ",");
                for (tok_idx = 0; tok_idx < MAPI_TEST_MAX_OUTPUT_PATH; tok_idx++) {
                    if (tempArg == NULL) break;
                    testConfig->scl_height[tok_idx] = VPU_CEIL(atoi(tempArg), 2);
                    tempArg = strtok(NULL, ",");
                }
            } else if (!strcmp(options[idx].name, "scl-y-hor-phase-0")) {
                testConfig->mapi_ctrl.mapi_output_scaler[0].yHorPhase = atoi(optarg);
            } else if (!strcmp(options[idx].name, "scl-y-ver-phase-0")) {
                testConfig->mapi_ctrl.mapi_output_scaler[0].yVerPhase = atoi(optarg);
            } else if (!strcmp(options[idx].name, "scl-c-hor-phase-0")) {
                testConfig->mapi_ctrl.mapi_output_scaler[0].cHorPhase = atoi(optarg);
            } else if (!strcmp(options[idx].name, "scl-c-ver-phase-0")) {
                testConfig->mapi_ctrl.mapi_output_scaler[0].cVerPhase = atoi(optarg);
            } else if (!strcmp(options[idx].name, "enableUserPhase")) {
                tempArg = strtok(optarg, ",");
                for (tok_idx = 0; tok_idx < MAPI_TEST_MAX_OUTPUT_PATH; tok_idx++) {
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL) break;
                    testConfig->mapi_ctrl.mapi_output_scaler[tok_idx].enableUserPhase = atoi(tempArg);
                }
            } else if (!strcmp(options[idx].name, "scl-y-hor-phase-1")) {
                testConfig->mapi_ctrl.mapi_output_scaler[1].yHorPhase = atoi(optarg);
            } else if (!strcmp(options[idx].name, "scl-y-ver-phase-1")) {
                testConfig->mapi_ctrl.mapi_output_scaler[1].yVerPhase = atoi(optarg);
            } else if (!strcmp(options[idx].name, "scl-c-hor-phase-1")) {
                testConfig->mapi_ctrl.mapi_output_scaler[1].cHorPhase = atoi(optarg);
            } else if (!strcmp(options[idx].name, "scl-c-ver-phase-1")) {
                testConfig->mapi_ctrl.mapi_output_scaler[1].cVerPhase = atoi(optarg);
            } else if (!strcmp(options[idx].name, "bilinear_mode")) {
                testConfig->bilinear_mode = atoi(optarg);
            } else if (!strcmp(options[idx].name, "scaler_mode")) {
                testConfig->scaler_mode = atoi(optarg);
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

    if (testConfig->width < MIN_WIDTH || testConfig->width > MAX_WIDTH) {
        VLOG(ERR, "Min/Max width : %d / %d \n", MIN_WIDTH, MAX_WIDTH);
        VLOG(ERR, "Check width : %d\n", testConfig->width);
        return FALSE;
    }
    if (testConfig->height < MIN_HEIGHT || testConfig->height > MAX_HEIGHT) {
        VLOG(ERR, "Min/Max height : %d / %d \n", MIN_HEIGHT, MAX_HEIGHT);
        VLOG(ERR, "Check height : %d\n", testConfig->height);
        return FALSE;
    }
    for (tok_idx = 0 ; tok_idx < MAPI_TEST_MAX_OUTPUT_PATH; tok_idx++) {
        if (testConfig->scl_width[tok_idx] != 0 && testConfig->scl_width[tok_idx] < MIN_SCL_WIDTH) {
            if (testConfig->scl_width[tok_idx] < MIN_SCL_WIDTH || testConfig->scl_width[tok_idx] > MAX_SCL_WIDTH) {
                VLOG(ERR, "Min/Max scale width : %d / %d \n", MIN_SCL_WIDTH, MAX_SCL_WIDTH);
                VLOG(ERR, "Check scale-width : %d\n", testConfig->scl_width[tok_idx]);
                return FALSE;
            }
        }
        if (testConfig->scl_height[tok_idx] != 0) {
            if (testConfig->scl_height[tok_idx] < MIN_SCL_HEIGHT || testConfig->scl_height[tok_idx] > MAX_SCL_HEIGHT) {
                VLOG(ERR, "Min/Max scale height : %d / %d \n", MIN_SCL_HEIGHT, MAX_SCL_HEIGHT);
                VLOG(ERR, "Check scale-height : %d\n", testConfig->scl_height[tok_idx]);
                return FALSE;
            }
        }
    }
    if (testConfig->width > MAX_SCL_WIDTH && testConfig->bilinear_mode == 0) {
        VLOG(ERR, "Check width : %d, bilinear_mode : %d\n", testConfig->width, testConfig->bilinear_mode);
        return FALSE;
    }
    if ((testConfig->input_fb_num > MAPI_TEST_MAX_NUM_INPUT_BUF || testConfig->input_fb_num < 1) || (testConfig->output_fb_num > MAPI_TEST_MAX_NUM_OUTPUT_BUF || testConfig->output_fb_num < 1)) {
        VLOG(ERR, "input frame num : %d and output frame num : %d \n", testConfig->input_fb_num, testConfig->output_fb_num);
        return FALSE;
    }

    return TRUE;
}

/**
 * SetDefaultTestConfig() - Initialize test variables
 * @p_test_config: test data struct
 */
static void SetDefaultTestConfig(TestMapConvConfig* p_test_config)
{
    Int32 idx = 0;
    osal_memset(p_test_config, 0x00, sizeof(TestMapConvConfig));

    p_test_config->inputEndian = HOST_ENDIAN;
    p_test_config->outputEndian = HOST_ENDIAN;

    for(idx = 0 ; idx < MAPI_TEST_MAX_OUTPUT_PATH; idx++) {
        p_test_config->output_buf_format[idx] = MAPI_FORMAT_ERR;
        p_test_config->mapi_ctrl.mapi_output_scaler[idx].enableUserPhase = FALSE; // use phase value 0
    }

    p_test_config->input_ctrl_mode = MAPI_TEST_INPUT_ERR;
    p_test_config->output_ctrl_mode = MAPI_OUTPUT_ERR;

    p_test_config->forceOutNum = 1;
    p_test_config->input_fb_num = 1;
    p_test_config->output_fb_num = 1;

    //BT.709(HDTV)
    p_test_config->mapi_ctrl.mapi_output_path[0].csc.coefGCR = 0x1D6;
    p_test_config->mapi_ctrl.mapi_output_path[0].csc.coefRCR = 0x629;
    p_test_config->mapi_ctrl.mapi_output_path[0].csc.coefBCB = 0x744;
    p_test_config->mapi_ctrl.mapi_output_path[0].csc.coefGCB = 0x0BB;

    p_test_config->mapi_ctrl.mapi_output_path[1].csc.coefGCR = 0x1D6;
    p_test_config->mapi_ctrl.mapi_output_path[1].csc.coefRCR = 0x629;
    p_test_config->mapi_ctrl.mapi_output_path[1].csc.coefBCB = 0x744;
    p_test_config->mapi_ctrl.mapi_output_path[1].csc.coefGCB = 0x0BB;

    sprintf(p_test_config->fbc_dir, "fbc_result/");

}

/**
 * GetMAPIFormat() - Get MAPIFormat type value by FrameMapConvBufferFormat
 * @mapi_format: FrameMapConvBufferFormat type value
 */
static MAPIFormat GetMAPIFormat(FrameMapConvBufferFormat mapi_format)
{
    MAPIFormat fmt = MAPI_FMT_420;

    switch (mapi_format) {
    case MAPI_FORMAT_420 :
    case MAPI_FORMAT_420_P10_16BIT_MSB :
    case MAPI_FORMAT_420_P10_16BIT_LSB :
    case MAPI_FORMAT_420_P10_32BIT_MSB :
    case MAPI_FORMAT_420_P10_32BIT_LSB : fmt = MAPI_FMT_420; 
        break;
    case MAPI_FORMAT_422 :
    case MAPI_FORMAT_422_P10_16BIT_MSB :
    case MAPI_FORMAT_422_P10_16BIT_LSB :
    case MAPI_FORMAT_422_P10_32BIT_MSB :
    case MAPI_FORMAT_422_P10_32BIT_LSB : fmt = MAPI_FMT_422;
        break;
    case MAPI_FORMAT_444 :
    case MAPI_FORMAT_444_P10_16BIT_MSB :
    case MAPI_FORMAT_444_P10_16BIT_LSB :
    case MAPI_FORMAT_444_P10_32BIT_MSB :
    case MAPI_FORMAT_444_P10_32BIT_LSB : fmt = MAPI_FMT_444;
        break;
    case MAPI_FORMAT_RGB :
    case MAPI_FORMAT_RGB_P10_1P2B_LSB :
    case MAPI_FORMAT_RGB_P10_1P2B_MSB :
    case MAPI_FORMAT_RGB_P10_3P4B_LSB : 
    case MAPI_FORMAT_RGB_P10_3P4B_MSB : 
    case MAPI_FORMAT_RGB_PACKED : 
    case MAPI_FORMAT_RGB_P10_1P2B_LSB_PACKED : 
    case MAPI_FORMAT_RGB_P10_1P2B_MSB_PACKED : fmt = MAPI_FMT_RGB;
        break;
        /* 1PLANE */
    case MAPI_FORMAT_422_P8_8BIT_1PLANE:
    case MAPI_FORMAT_422_P10_16BIT_MSB_1PLANE:
    case MAPI_FORMAT_422_P10_16BIT_LSB_1PLANE:
    case MAPI_FORMAT_422_P10_32BIT_MSB_1PLANE:
    case MAPI_FORMAT_422_P10_32BIT_LSB_1PLANE: fmt = MAPI_FMT_422;
        break;
    case MAPI_FORMAT_444_P8_8BIT_1PLANE:
    case MAPI_FORMAT_444_P10_16BIT_MSB_1PLANE:
    case MAPI_FORMAT_444_P10_16BIT_LSB_1PLANE:
    case MAPI_FORMAT_444_P10_32BIT_MSB_1PLANE:
    case MAPI_FORMAT_444_P10_32BIT_LSB_1PLANE:
    case MAPI_FORMAT_444_P8_8BIT_1PLANE_PACKED:
    case MAPI_FORMAT_444_P10_16BIT_MSB_1PLANE_PACKED:
    case MAPI_FORMAT_444_P10_16BIT_LSB_1PLANE_PACKED: fmt = MAPI_FMT_444;
        break;
    default :
        VLOG(ERR, "[MAP_CONV] %s:%d Check formats : %d \n", __FUNCTION__, __LINE__, mapi_format);
    }
    return fmt;
}

/**
 * IsPacked() - decision whether it is a packed format
 * @mapi_format: FrameMapConvBufferFormat type value
 */
static BOOL IsPacked(FrameMapConvBufferFormat mapi_format)
{
    BOOL is_packed = FALSE;
    switch (mapi_format) {
    case MAPI_FORMAT_RGB_PACKED:
    case MAPI_FORMAT_RGB_P10_1P2B_LSB_PACKED :
    case MAPI_FORMAT_RGB_P10_1P2B_MSB_PACKED :
    case MAPI_FORMAT_444_P8_8BIT_1PLANE_PACKED :
    case MAPI_FORMAT_444_P10_16BIT_MSB_1PLANE_PACKED :
    case MAPI_FORMAT_444_P10_16BIT_LSB_1PLANE_PACKED :
        is_packed = TRUE;
        break;
    default :
        break;
    }

    return is_packed;
}

/**
 * Is3P4B() - decision whether it is a 3 pixel 4 byte format
 * @mapi_format: FrameMapConvBufferFormat type value
 */
static BOOL Is3P4B(FrameMapConvBufferFormat mapi_format)
{
    BOOL is_3p4b = FALSE;
    switch (mapi_format) {
    case MAPI_FORMAT_420_P10_32BIT_MSB :
    case MAPI_FORMAT_420_P10_32BIT_LSB :
    case MAPI_FORMAT_422_P10_32BIT_MSB :
    case MAPI_FORMAT_422_P10_32BIT_LSB :
    case MAPI_FORMAT_444_P10_32BIT_MSB :
    case MAPI_FORMAT_444_P10_32BIT_LSB :
    case MAPI_FORMAT_RGB_P10_3P4B_LSB :
    case MAPI_FORMAT_RGB_P10_3P4B_MSB :
    case MAPI_FORMAT_422_P10_32BIT_MSB_1PLANE :
    case MAPI_FORMAT_422_P10_32BIT_LSB_1PLANE :
    case MAPI_FORMAT_444_P10_32BIT_MSB_1PLANE :
    case MAPI_FORMAT_444_P10_32BIT_LSB_1PLANE :
        is_3p4b = TRUE;
        break;
    default :
        break;
    }
    return is_3p4b;
}

/**
 * IsMSB() - decision whether it is a MSB format
 * @mapi_format: FrameMapConvBufferFormat type value
 */
static BOOL IsMSB(FrameMapConvBufferFormat mapi_format)
{
    BOOL is_MSB = FALSE;

    switch (mapi_format) {
    case MAPI_FORMAT_420_P10_16BIT_MSB :
    case MAPI_FORMAT_420_P10_32BIT_MSB :
    case MAPI_FORMAT_422_P10_16BIT_MSB :
    case MAPI_FORMAT_422_P10_32BIT_MSB :
    case MAPI_FORMAT_444_P10_16BIT_MSB:
    case MAPI_FORMAT_444_P10_32BIT_MSB:
    case MAPI_FORMAT_RGB_P10_1P2B_MSB:
    case MAPI_FORMAT_RGB_P10_3P4B_MSB:
    case MAPI_FORMAT_RGB_P10_1P2B_MSB_PACKED:
    case MAPI_FORMAT_422_P10_16BIT_MSB_1PLANE:
    case MAPI_FORMAT_422_P10_32BIT_MSB_1PLANE:
    case MAPI_FORMAT_444_P10_16BIT_MSB_1PLANE:
    case MAPI_FORMAT_444_P10_32BIT_MSB_1PLANE:
    case MAPI_FORMAT_444_P10_16BIT_MSB_1PLANE_PACKED:
        is_MSB = TRUE;
        break;
    default : 
        break;
    }
    return is_MSB;
}

/**
 * Is1Plane() - decision whether it is a one plane format
 * @mapi_format: FrameMapConvBufferFormat type value
 */
static BOOL Is1Plane(FrameMapConvBufferFormat mapi_format)
{
    BOOL is_1plane = FALSE;
    switch (mapi_format) {
    case MAPI_FORMAT_RGB:
    case MAPI_FORMAT_RGB_P10_1P2B_LSB:
    case MAPI_FORMAT_RGB_P10_1P2B_MSB:
    case MAPI_FORMAT_RGB_P10_3P4B_LSB:
    case MAPI_FORMAT_RGB_P10_3P4B_MSB:
    case MAPI_FORMAT_RGB_PACKED:
    case MAPI_FORMAT_RGB_P10_1P2B_LSB_PACKED:
    case MAPI_FORMAT_RGB_P10_1P2B_MSB_PACKED:

    case MAPI_FORMAT_422_P8_8BIT_1PLANE:
    case MAPI_FORMAT_422_P10_16BIT_MSB_1PLANE:
    case MAPI_FORMAT_422_P10_16BIT_LSB_1PLANE:
    case MAPI_FORMAT_422_P10_32BIT_MSB_1PLANE:
    case MAPI_FORMAT_422_P10_32BIT_LSB_1PLANE:

    case MAPI_FORMAT_444_P8_8BIT_1PLANE:
    case MAPI_FORMAT_444_P10_16BIT_MSB_1PLANE:
    case MAPI_FORMAT_444_P10_16BIT_LSB_1PLANE:
    case MAPI_FORMAT_444_P10_32BIT_MSB_1PLANE:
    case MAPI_FORMAT_444_P10_32BIT_LSB_1PLANE:

    case MAPI_FORMAT_444_P8_8BIT_1PLANE_PACKED:
    case MAPI_FORMAT_444_P10_16BIT_MSB_1PLANE_PACKED:
    case MAPI_FORMAT_444_P10_16BIT_LSB_1PLANE_PACKED:
        is_1plane = TRUE;
        break;
    default:
        break;
    }
    return is_1plane;
}

/**
 * IsAlignedFMT() - decision whether it is an aligned format
 * @mapi_format: FrameMapConvBufferFormat type value
 */
static BOOL IsAlignedFMT(FrameMapConvBufferFormat mapi_format)
{
    BOOL is_aligned_fmt = FALSE;
    switch (mapi_format) {
    case MAPI_FORMAT_RGB:
    case MAPI_FORMAT_RGB_P10_1P2B_LSB:
    case MAPI_FORMAT_RGB_P10_1P2B_MSB:
    case MAPI_FORMAT_444_P8_8BIT_1PLANE:
    case MAPI_FORMAT_444_P10_16BIT_MSB_1PLANE:
    case MAPI_FORMAT_444_P10_16BIT_LSB_1PLANE:
        is_aligned_fmt = TRUE;
        break;
    default:
        break;
    }
    return is_aligned_fmt;
}

/**
 * WtlFormatToBitdepth() - Get a bit depth by FrameMapConvBufferFormat
 * @mapi_format: FrameMapConvBufferFormat type value
 */
static Int32 WtlFormatToBitdepth(FrameMapConvBufferFormat mapi_format)
{
    Int32 bit_depth = 0;
    switch (mapi_format)
    {
    case MAPI_FORMAT_420 :
    case MAPI_FORMAT_422 : 
    case MAPI_FORMAT_444 :
    case MAPI_FORMAT_422_P8_8BIT_1PLANE:
    case MAPI_FORMAT_444_P8_8BIT_1PLANE:
    case MAPI_FORMAT_444_P8_8BIT_1PLANE_PACKED:
    case MAPI_FORMAT_RGB :
    case MAPI_FORMAT_RGB_PACKED:
        bit_depth = 8;
        break;
    case MAPI_FORMAT_420_P10_16BIT_MSB :
    case MAPI_FORMAT_420_P10_16BIT_LSB :
    case MAPI_FORMAT_420_P10_32BIT_MSB :
    case MAPI_FORMAT_420_P10_32BIT_LSB :
    case MAPI_FORMAT_422_P10_16BIT_MSB :
    case MAPI_FORMAT_422_P10_16BIT_LSB :
    case MAPI_FORMAT_422_P10_32BIT_MSB :
    case MAPI_FORMAT_422_P10_32BIT_LSB :
    case MAPI_FORMAT_444_P10_16BIT_MSB :
    case MAPI_FORMAT_444_P10_16BIT_LSB :
    case MAPI_FORMAT_444_P10_32BIT_MSB :
    case MAPI_FORMAT_444_P10_32BIT_LSB :

    case MAPI_FORMAT_RGB_P10_1P2B_LSB :
    case MAPI_FORMAT_RGB_P10_1P2B_MSB :
    case MAPI_FORMAT_RGB_P10_3P4B_LSB :
    case MAPI_FORMAT_RGB_P10_3P4B_MSB :
    case MAPI_FORMAT_RGB_P10_1P2B_LSB_PACKED:
    case MAPI_FORMAT_RGB_P10_1P2B_MSB_PACKED:
    case MAPI_FORMAT_422_P10_16BIT_MSB_1PLANE:
    case MAPI_FORMAT_422_P10_16BIT_LSB_1PLANE:
    case MAPI_FORMAT_422_P10_32BIT_MSB_1PLANE:
    case MAPI_FORMAT_422_P10_32BIT_LSB_1PLANE:
    case MAPI_FORMAT_444_P10_16BIT_MSB_1PLANE:
    case MAPI_FORMAT_444_P10_16BIT_LSB_1PLANE:
    case MAPI_FORMAT_444_P10_32BIT_MSB_1PLANE:
    case MAPI_FORMAT_444_P10_32BIT_LSB_1PLANE:
    case MAPI_FORMAT_444_P10_16BIT_MSB_1PLANE_PACKED:
    case MAPI_FORMAT_444_P10_16BIT_LSB_1PLANE_PACKED:
        bit_depth = 10;
        break;
    default: 
        VLOG(ERR, "not support mapi_format =%d\n", mapi_format);
        break;
    }
    return bit_depth;
}

/**
 * SetTestConfig() - Set test values after parsing host options
 * @p_test_config: test data struct
 */
BOOL SetTestConfig(TestMapConvConfig* p_test_config)
{
    FrameMapConvBufferFormat input_buf_fmt;
    FrameMapConvBufferFormat* output_buf_fmt;
    MapiFormatProperty* input_fmt_property = &(p_test_config->input_format_property);
    MapiFormatProperty* output_fmt_property = p_test_config->output_format_property;
    Int32 idx = 0;

    input_buf_fmt = p_test_config->input_buf_format;
    output_buf_fmt = p_test_config->output_buf_format;

    switch (p_test_config->output_ctrl_mode) {
    case MAPI_OUTPUT_DISP :
        p_test_config->out_mode[0] = MAPI_O_MODE_DIS;
        break;
    case MAPI_OUTPUT_WDMA:
        p_test_config->out_mode[0] = MAPI_O_MODE_YUV;
        break;
    case MAPI_OUTPUT_DISP_WDMA:
        p_test_config->out_mode[0] = MAPI_O_MODE_DIS;
        p_test_config->out_mode[1] = MAPI_O_MODE_YUV;
        break;
    default :
        VLOG(ERR, "%s:%d check out ctrl mode : %d \n", p_test_config->output_ctrl_mode);
        return FALSE;
    }

    input_fmt_property->bit_depth  = WtlFormatToBitdepth(input_buf_fmt);
    input_fmt_property->is_msb     = IsMSB(input_buf_fmt);
    input_fmt_property->is_3p4b    = Is3P4B(input_buf_fmt);
    input_fmt_property->is_packed  = IsPacked(input_buf_fmt);
    input_fmt_property->is_1plane  = Is1Plane(input_buf_fmt);
    input_fmt_property->is_aligned = IsAlignedFMT(input_buf_fmt);
    input_fmt_property->mapi_fmt   = GetMAPIFormat(input_buf_fmt);
    input_fmt_property->is_nv21    = p_test_config->in_nv21;
    input_fmt_property->endian     = p_test_config->inputEndian;

    for(idx = 0; idx < MAPI_TEST_MAX_OUTPUT_PATH; idx++) {
        if (p_test_config->out_mode[idx] > MAPI_O_MODE_NONE) {
            output_fmt_property[idx].mapi_fmt   = GetMAPIFormat(output_buf_fmt[idx]);
            output_fmt_property[idx].bit_depth  = WtlFormatToBitdepth(output_buf_fmt[idx]);
            output_fmt_property[idx].is_msb     = IsMSB(output_buf_fmt[idx]);
            output_fmt_property[idx].is_3p4b    = Is3P4B(output_buf_fmt[idx]);
            output_fmt_property[idx].is_nv21    = p_test_config->out_nv21[idx];
            output_fmt_property[idx].is_packed  = IsPacked(output_buf_fmt[idx]);
            output_fmt_property[idx].is_1plane  = Is1Plane(output_buf_fmt[idx]);
            output_fmt_property[idx].is_aligned = IsAlignedFMT(output_buf_fmt[idx]);
            output_fmt_property[idx].endian     = p_test_config->outputEndian;
        }
    }
    return TRUE;
}



/**
 * CalFrameSize() - calculate a linear buffer size
 * @p_test_config:  test data struct
 * @format:         FrameMapConvBufferFormat type value
 * @width:          original width
 * @height:         original height
 * @p_frame_size:   FrameSizeInfo pointer type value used to store calculated buffer size
 */
static BOOL CalFrameSize(
    TestMapConvConfig*  p_test_config,
    FrameMapConvBufferFormat   format,
    Int32   width,
    Int32   height,
    FrameSizeInfo* p_frame_size
    )
{
    Uint32  dstWidth=0, dstHeight=0;
    Uint32  dstChromaHeight = 0;
    Uint32  dstChromaWidth = 0;
    Uint32  div_x = 1, div_y = 1;
    Uint32  lumaSize = 0, chromaSize = 0, frameSize = 0;
    Uint32  luma_stride = 0, chroma_stride = 0;
    BOOL    is_fmt_444 = FALSE;
    Uint32  aligned_frame_size = 0, aligned_luma_size = 0, aligned_chroma_size = 0;

    dstHeight = height;

    if (p_frame_size == NULL) {
        return FALSE;
    }

    switch (format) {
    case MAPI_FORMAT_420:
    case MAPI_FORMAT_420_P10_16BIT_LSB:
    case MAPI_FORMAT_420_P10_16BIT_MSB:
    case MAPI_FORMAT_420_P10_32BIT_LSB:
    case MAPI_FORMAT_420_P10_32BIT_MSB:
    case MAPI_FORMAT_422:
    case MAPI_FORMAT_422_P10_16BIT_LSB:
    case MAPI_FORMAT_422_P10_16BIT_MSB:
    case MAPI_FORMAT_422_P10_32BIT_LSB:
    case MAPI_FORMAT_422_P10_32BIT_MSB:
        div_x = 2;
        break;
    default:
        div_x = 1;
    }

    switch (format) {
    case MAPI_FORMAT_420:
    case MAPI_FORMAT_422:
    case MAPI_FORMAT_420_P10_16BIT_LSB:
    case MAPI_FORMAT_420_P10_16BIT_MSB:
    case MAPI_FORMAT_420_P10_32BIT_LSB:
    case MAPI_FORMAT_420_P10_32BIT_MSB:
    case MAPI_FORMAT_422_P10_16BIT_LSB:
    case MAPI_FORMAT_422_P10_16BIT_MSB:
    case MAPI_FORMAT_422_P10_32BIT_LSB:
    case MAPI_FORMAT_422_P10_32BIT_MSB:
        div_y = 2;
        break;
    default:
        div_y = 1;
    }

    dstChromaWidth = 0;
    dstChromaHeight = 0;
    switch (format) {
    case MAPI_FORMAT_420_P10_16BIT_LSB:
    case MAPI_FORMAT_420_P10_16BIT_MSB:
        dstWidth = width * 2;
        dstChromaHeight = (dstHeight + 1) / div_y;
        dstChromaWidth  =  ((width + 1) / div_x) * 2 * 2;
        luma_stride = VPU_ALIGN32(width * 2);
        break;
    case MAPI_FORMAT_420_P10_32BIT_LSB:
    case MAPI_FORMAT_420_P10_32BIT_MSB:
        dstWidth = (width+2)/3*4;
        dstChromaHeight = (dstHeight + 1) / div_y;
        dstChromaWidth = ((width + 1) / div_x * 2 + 2) / 3 * 4;
        luma_stride = VPU_ALIGN32((width * 4) / 3);
        break;
    case MAPI_FORMAT_420:
        dstWidth = width;
        dstChromaHeight = (dstHeight + 1) / div_y;
        dstChromaWidth  = width / div_x * 2;
        luma_stride = VPU_ALIGN16(width);
        break;
    case MAPI_FORMAT_422:
        dstWidth = width;
        dstChromaHeight = ((dstHeight + 1) / div_y) * 2;
        dstChromaWidth  = (width + 1) / div_x * 2;
        luma_stride = VPU_ALIGN16(width);
        break;
    case MAPI_FORMAT_444:
        dstWidth = width;
        dstChromaHeight = dstHeight;
        dstChromaWidth = width * 2;
        luma_stride = VPU_ALIGN16(width);
        is_fmt_444 = TRUE;
        break;
    case MAPI_FORMAT_422_P10_16BIT_LSB:
    case MAPI_FORMAT_422_P10_16BIT_MSB:
        dstWidth = width * 2;
        dstChromaHeight = ((dstHeight + 1) / div_y) * 2;
        dstChromaWidth  = (width + 1) / div_x * 2 * 2;
        luma_stride = VPU_ALIGN32(width * 2);
        break;
    case MAPI_FORMAT_444_P10_16BIT_MSB:
    case MAPI_FORMAT_444_P10_16BIT_LSB:
        dstWidth = width * 2;
        dstChromaHeight = dstHeight;
        dstChromaWidth = width * 2 * 2;
         luma_stride = VPU_ALIGN32(width * 2);
         is_fmt_444 = TRUE;
        break;
    case MAPI_FORMAT_422_P10_32BIT_LSB:
    case MAPI_FORMAT_422_P10_32BIT_MSB:
        dstWidth = (width + 2) / 3 * 4;
        dstChromaHeight = ((dstHeight + 1) / div_y) * 2;
        dstChromaWidth = (width / div_x * 2 + 2) / 3 * 4;
        luma_stride = VPU_ALIGN32((width * 4) / 3);
        break;
    case MAPI_FORMAT_444_P10_32BIT_LSB:
    case MAPI_FORMAT_444_P10_32BIT_MSB:
        dstWidth = (width + 2) / 3 * 4;
        dstChromaHeight = dstHeight;
        dstChromaWidth = (width * 2 + 2) / 3 * 4;
        luma_stride = VPU_ALIGN32((width * 4) / 3);
        is_fmt_444 = TRUE;
        break;
    case MAPI_FORMAT_RGB:
    case MAPI_FORMAT_444_P8_8BIT_1PLANE:
        dstWidth = width * 4;
        luma_stride = VPU_ALIGN16(dstWidth);
        break;
    case MAPI_FORMAT_RGB_P10_1P2B_LSB:
    case MAPI_FORMAT_RGB_P10_1P2B_MSB:
    case MAPI_FORMAT_444_P10_16BIT_MSB_1PLANE:
    case MAPI_FORMAT_444_P10_16BIT_LSB_1PLANE:
        dstWidth = width * 4 * 2;
        luma_stride = VPU_ALIGN32(dstWidth);
        break;
    case MAPI_FORMAT_RGB_P10_3P4B_LSB:
    case MAPI_FORMAT_RGB_P10_3P4B_MSB:
        dstWidth = width * 3 * 4 / 3;
        luma_stride = VPU_ALIGN32(dstWidth);
        break;
    case MAPI_FORMAT_RGB_PACKED:
    case MAPI_FORMAT_444_P8_8BIT_1PLANE_PACKED:
        dstWidth = width * 3;
        luma_stride = VPU_ALIGN16(dstWidth);
        break;
    case MAPI_FORMAT_RGB_P10_1P2B_LSB_PACKED:
    case MAPI_FORMAT_RGB_P10_1P2B_MSB_PACKED:
    case MAPI_FORMAT_444_P10_16BIT_MSB_1PLANE_PACKED:
    case MAPI_FORMAT_444_P10_16BIT_LSB_1PLANE_PACKED:
        dstWidth = width * 3 * 2;
        luma_stride = VPU_ALIGN32(dstWidth);
        break;
    case MAPI_FORMAT_422_P8_8BIT_1PLANE:
        dstWidth = width * 2;
        luma_stride = VPU_ALIGN16(dstWidth);
        break;
    case MAPI_FORMAT_422_P10_16BIT_MSB_1PLANE:
    case MAPI_FORMAT_422_P10_16BIT_LSB_1PLANE:
        dstWidth = width * 2 * 2;
        luma_stride = VPU_ALIGN32(dstWidth);
        break;
    case MAPI_FORMAT_422_P10_32BIT_MSB_1PLANE:
    case MAPI_FORMAT_422_P10_32BIT_LSB_1PLANE:
        dstWidth = ((width * 2 + 2) / 3) * 4;
        luma_stride = VPU_ALIGN32(dstWidth);
        break;
    case MAPI_FORMAT_444_P10_32BIT_MSB_1PLANE:
    case MAPI_FORMAT_444_P10_32BIT_LSB_1PLANE:
        //dstWidth = width * 3 * 4 / 3;
        dstWidth = (width * 3 / 3) * 4;
        luma_stride = VPU_ALIGN32(dstWidth);
        break;
    default:
        VLOG(ERR, "%s:%d unsupprt format %d)\n", __FUNCTION__, __LINE__, format);
        return FALSE;
    }

    lumaSize   = dstWidth * dstHeight;
    chromaSize = dstChromaWidth * dstChromaHeight;
    frameSize  = lumaSize + chromaSize;

    chroma_stride       = (is_fmt_444 == TRUE) ? luma_stride * 2 : luma_stride;
    aligned_luma_size   = luma_stride * dstHeight;
    aligned_chroma_size = chroma_stride * dstChromaHeight;
    aligned_frame_size  = aligned_luma_size + aligned_chroma_size;


    p_frame_size->luma_width          = dstWidth;
    p_frame_size->luma_height         = dstHeight;
    p_frame_size->chroma_width        = dstChromaWidth;
    p_frame_size->chroma_height       = dstChromaHeight;
    p_frame_size->luma_size           = lumaSize;
    p_frame_size->chroma_size         = chromaSize;
    p_frame_size->frame_size          = frameSize;
    p_frame_size->luma_stride         = luma_stride;
    p_frame_size->chroma_stride       = chroma_stride;
    p_frame_size->aligned_luma_size   = aligned_luma_size;
    p_frame_size->aligned_chroma_size = aligned_chroma_size;
    p_frame_size->aligned_frame_size  = aligned_frame_size;



    return TRUE;
}

/**
 * CalCompressedFrameSize() - calculate a compressed buffer size (compressed data/offset table)
 * RETURNS calculated frame buffer size
 */
static Uint32 CalCompressedFrameSize(
    TestMapConvConfig*  p_test_config,
    FrameMapConvCompressedType comp_type,
    Int32   format,
    Int32   bitFormat,
    Int32   width,
    Int32   height,
    size_t  *p_luma_size,
    size_t  *p_chroma_size
    )
{
    Uint32 frame_size = 0, luma_size = 0, chroma_size = 0;
    Int32 luma_stride = 0, chroma_stride = 0;

    if (p_test_config->input_buf_format == MAPI_FORMAT_420_P10_16BIT_LSB || p_test_config->cf10_10b_to_8b == TRUE) {
        luma_stride = VPU_ALIGN32(VPU_ALIGN16(width)*5);
        chroma_stride = VPU_ALIGN32(VPU_ALIGN16(width/2)*5);
    } else {
        luma_stride = VPU_ALIGN32(VPU_ALIGN16(width)*4);
        chroma_stride = VPU_ALIGN32(VPU_ALIGN16(width/2)*4);
    }

    if (comp_type == MAP_COMPRESSED_FRAME_MAP) {
        height = VPU_ALIGN4(height);
        luma_size = (height / 4) * luma_stride;
        chroma_size = (height / 4) * chroma_stride;
    } else if (comp_type == MAP_COMPRESSED_FRAME_LUMA_TABLE) {
        switch (bitFormat) {
        case STD_HEVC:    frame_size = WAVE5_FBC_LUMA_TABLE_SIZE(width, height); break;
        case STD_VP9:     frame_size = WAVE5_FBC_LUMA_TABLE_SIZE(VPU_ALIGN64(width), VPU_ALIGN64(height)); break;
        case STD_AVS2:    frame_size = WAVE5_FBC_LUMA_TABLE_SIZE(width, height); break;
        case STD_AVC:     frame_size = WAVE5_FBC_LUMA_TABLE_SIZE(width, height); break;
        case STD_AV1:     frame_size = WAVE5_FBC_LUMA_TABLE_SIZE(VPU_ALIGN16(width), VPU_ALIGN8(height)); break;
        default:
            return FALSE;
        }
        return frame_size;
    } else if (comp_type == MAP_COMPRESSED_FRAME_CHROMA_TABLE) {
        switch (bitFormat) {
        case STD_HEVC:    frame_size = WAVE5_FBC_CHROMA_TABLE_SIZE(width, height); break;
        case STD_VP9:     frame_size = WAVE5_FBC_CHROMA_TABLE_SIZE(VPU_ALIGN64(width), VPU_ALIGN64(height)); break;
        case STD_AVS2:    frame_size = WAVE5_FBC_CHROMA_TABLE_SIZE(width, height); break;
        case STD_AVC:     frame_size = WAVE5_FBC_CHROMA_TABLE_SIZE(width, height); break;
        case STD_AV1:     frame_size = WAVE5_FBC_CHROMA_TABLE_SIZE(VPU_ALIGN16(width), VPU_ALIGN8(height)); break;
        default:
            return RETCODE_NOT_SUPPORTED_FEATURE;
        }
        return frame_size;
    } else {
        VLOG(ERR, "%s:%d Check compressed buffer format : %d \n", format);
    }

    if(p_luma_size != NULL) {
        *p_luma_size = luma_size;
    }
    if (p_chroma_size != NULL) {
        *p_chroma_size = chroma_size;
    }
    frame_size = luma_size + chroma_size;

    return frame_size;
}

/**
 * AllocateLinearFrameBuffer() - allocate linear frame buffers for input/output frame buffer
 * RETURNS TRUE if allocated successfully
 */
static BOOL AllocateLinearFrameBuffer(
    TestMapConvConfig*  p_test_config,
    Uint32                      width,
    Uint32                      height,
    FrameBuffer*                p_fb,
    vpu_buffer_t*               p_vpu_buf,
    const char*                 buf_name,
    BOOL                        is_out_buf
    )
{
    FrameSizeInfo   fb_size_info;

    CalFrameSize(p_test_config, (FrameMapConvBufferFormat)p_fb->format, width, height, &fb_size_info);

    p_vpu_buf->size = fb_size_info.aligned_frame_size;

    if (vdi_allocate_dma_memory(p_test_config->coreIdx, p_vpu_buf, DEC_FB_LINEAR, 0) < 0) {
        VLOG(ERR, "%s:%d fail to allocate frame buffer\n", __FUNCTION__, __LINE__);
        return FALSE;
    }
#ifdef MAP_CONV_CLEAR_MEMORY
    vdi_clear_memory(coreIdx, p_vpu_buf->phys_addr, fb_size_info.aligned_frame_size, VDI_128BIT_LITTLE_ENDIAN);
#endif

    p_fb->bufY           = p_vpu_buf->phys_addr;
    p_fb->bufCb          = p_vpu_buf->phys_addr + fb_size_info.aligned_luma_size;
    p_fb->size           = fb_size_info.aligned_frame_size;
    p_fb->lumaBitDepth   = WtlFormatToBitdepth((FrameMapConvBufferFormat)p_fb->format);
    p_fb->chromaBitDepth = WtlFormatToBitdepth((FrameMapConvBufferFormat)p_fb->format);
    p_fb->width          = width;
    p_fb->height         = height;

    PutLogMsg("Allocated Frame Buffers : %s \n", buf_name);
    PutLogMsg("Buf Info, size : %d , y_addr : 0x%x , cb_addr : 0x%x , cr_addr : 0x%x\n\n", p_fb->size, p_fb->bufY, p_fb->bufCb, p_fb->bufCr);

    return TRUE;
}

/**
 * AllocateAddtionalBuffer() - allocate additional frame buffers like a fgn memory buffer
 * RETURNS TRUE if allocated successfully
 */
static BOOL AllocateAddtionalBuffer(
    TestMapConvConfig*  p_test_config,
    vpu_buffer_t*       p_vpu_buf,
    const char*         buf_name
    )
{
    if (vdi_allocate_dma_memory(p_test_config->coreIdx, p_vpu_buf, DEC_FBC, 0) < 0) {
        VLOG(ERR, "%s:%d fail to allocate frame buffer\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    PutLogMsg("Allocated Buffers for %s \n", buf_name);
    PutLogMsg("Buf Info, size : %d , base_addr : 0x%x \n", p_vpu_buf, p_vpu_buf->phys_addr);

    return TRUE;
}

/**
 * AllocateCompressedFrameBuffer() - allocate compressed frame buffers
 * RETURNS TRUE if allocated successfully
 */
static BOOL AllocateCompressedFrameBuffer(
    TestMapConvConfig*  p_test_config,
    FrameMapConvCompressedType compressed_type,
    Uint32                      width,
    Uint32                      height,
    FrameBuffer*                p_fb,
    vpu_buffer_t*               p_vpu_buf,
    char*                       buf_name
    )
{
    size_t          frameSizeY;     // the size of luma
    size_t          frameSizeC;     // the size of chroma
    size_t          frameSize;      // the size of frame

    frameSize = CalCompressedFrameSize(p_test_config, compressed_type, p_fb->format, p_test_config->bitFormat, width, height, &frameSizeY, &frameSizeC);

    p_vpu_buf->size = frameSize;
    if (vdi_allocate_dma_memory(p_test_config->coreIdx, p_vpu_buf, DEC_FBC, 0) < 0) {
        VLOG(ERR, "%s:%d fail to allocate frame buffer\n", __FUNCTION__, __LINE__);
        return FALSE;
    }
#ifdef MAP_CONV_CLEAR_MEMORY
    vdi_clear_memory(coreIdx, p_vpu_buf->phys_addr, frameSize, VDI_128BIT_LITTLE_ENDIAN);
#endif

    //switch case format

    p_fb->bufY           = p_vpu_buf->phys_addr;
    p_fb->bufCb          = p_vpu_buf->phys_addr + frameSizeY;
    p_fb->bufCr          = 0x00;
    p_fb->size           = frameSize;
    p_fb->endian         = VDI_128BIT_BIG_ENDIAN;
    p_fb->lumaBitDepth   = WtlFormatToBitdepth((FrameMapConvBufferFormat)p_fb->format);
    p_fb->chromaBitDepth = WtlFormatToBitdepth((FrameMapConvBufferFormat)p_fb->format);
    p_fb->bufYSize       = frameSizeY;
    p_fb->bufCbSize      = frameSizeC;

    p_fb->width = width;
    p_fb->height = height;

    PutLogMsg("Allocated Compressed Frame Buffers : %s \n", buf_name);
    PutLogMsg("Buf Info, size : %d , y_addr : 0x%x , cb_addr : 0x%x \n\n", p_fb->size, p_fb->bufY, p_fb->bufCb);

    return TRUE;
}

/**
 * PrepareMapConvTest() - preparation for running MAPI
 * @p_map_conv_cfg: test data struct
 */
static BOOL PrepareMapConvTest(
    TestMapConvConfig*   p_map_conv_cfg
    )
{
    Uint32  width, height;
    Int32 idx = 0, jdx = 0;
    Uint32 input_fb_num;
    width  = p_map_conv_cfg->width;
    height = p_map_conv_cfg->height;
    char buf_name[MAX_FILE_PATH];

    input_fb_num = p_map_conv_cfg->input_fb_num;

    //Input Buffer
    if (IS_LINEAR_INPUT(p_map_conv_cfg->input_ctrl_mode)) {
        for (idx = 0; idx < input_fb_num; idx++) {
            p_map_conv_cfg->in_frame_buffer[idx].format = (FrameBufferFormat)p_map_conv_cfg->input_buf_format;
            p_map_conv_cfg->in_frame_buffer[idx].endian = (int)p_map_conv_cfg->inputEndian;
            osal_memset(buf_name, 0x00, MAX_FILE_PATH);
            sprintf(buf_name,"INPUT_LINEAR_BUFFER_%d", idx);
            AllocateLinearFrameBuffer(p_map_conv_cfg, width, height, &(p_map_conv_cfg->in_frame_buffer[idx]), &(p_map_conv_cfg->in_vpu_buf[idx]), buf_name, FALSE);
        }
    } else if (IS_COMPRESSED_INPUT(p_map_conv_cfg->input_ctrl_mode)) {
        p_map_conv_cfg->in_compressed_fb[0].format = (FrameBufferFormat)p_map_conv_cfg->input_buf_format;
        p_map_conv_cfg->in_compressed_fb[1].format = (FrameBufferFormat)p_map_conv_cfg->input_buf_format;
        p_map_conv_cfg->in_compressed_fb[2].format = (FrameBufferFormat)p_map_conv_cfg->input_buf_format;
        AllocateCompressedFrameBuffer(p_map_conv_cfg, MAP_COMPRESSED_FRAME_MAP, width, height, &(p_map_conv_cfg->in_compressed_fb[0]), &(p_map_conv_cfg->in_vpu_buf[0]), "INPUT_COMPRESSED_BUFFER");
        AllocateCompressedFrameBuffer(p_map_conv_cfg, MAP_COMPRESSED_FRAME_LUMA_TABLE, width, height, &(p_map_conv_cfg->in_compressed_fb[1]), &(p_map_conv_cfg->in_vpu_buf[1]), "INPUT_COMPRESSED_LUMA_TABLE");
        AllocateCompressedFrameBuffer(p_map_conv_cfg, MAP_COMPRESSED_FRAME_CHROMA_TABLE, width, height, &(p_map_conv_cfg->in_compressed_fb[2]), &(p_map_conv_cfg->in_vpu_buf[2]), "INPUT_COMPRESSED_CHROMA_TABLE");
        if (p_map_conv_cfg->enable_fgn == TRUE) {
            p_map_conv_cfg->in_vpu_buf[3].size = MAPI_CF10_FGN_DATA_SIZE;
            p_map_conv_cfg->in_vpu_buf[4].size = FGN_MEM_SIZE;
            AllocateAddtionalBuffer(p_map_conv_cfg, &(p_map_conv_cfg->in_vpu_buf[3]), "FGN_INFO_BUFFER");
            AllocateAddtionalBuffer(p_map_conv_cfg, &(p_map_conv_cfg->in_vpu_buf[4]), "FGN_MEM_BUFFER");
            p_map_conv_cfg->fgn_input_data.fgn_pic_param_buf.phys_addr = p_map_conv_cfg->in_vpu_buf[3].phys_addr;
            p_map_conv_cfg->fgn_input_data.fgn_mem_buf.phys_addr = p_map_conv_cfg->in_vpu_buf[4].phys_addr;
        }
    }

    //Output Buffer
    for (idx = 0; idx < MAPI_TEST_MAX_OUTPUT_PATH; idx++) {
        if (p_map_conv_cfg->out_mode[idx] > MAPI_O_MODE_NONE) {
            width = (p_map_conv_cfg->scl_width[idx] != 0) ? p_map_conv_cfg->scl_width[idx] : width;
            height = (p_map_conv_cfg->scl_height[idx] != 0) ? p_map_conv_cfg->scl_height[idx] : height;
            PutLogMsg("[%02d] ori_width*height : %d*%d \n", idx, p_map_conv_cfg->width, p_map_conv_cfg->height);
            PutLogMsg("[%02d] scaled_down_width*height : %d*%d \n",idx,  width, height);
        }
        if (p_map_conv_cfg->out_mode[idx] == MAPI_O_MODE_DIS || p_map_conv_cfg->out_mode[idx] == MAPI_O_MODE_YUV) {
            for(jdx = 0; jdx < p_map_conv_cfg->output_fb_num; jdx++) {
                p_map_conv_cfg->out_frame_buffer[idx][jdx].endian = p_map_conv_cfg->outputEndian;
                p_map_conv_cfg->out_frame_buffer[idx][jdx].format = (FrameBufferFormat)(p_map_conv_cfg->output_buf_format[idx]);
                AllocateLinearFrameBuffer(p_map_conv_cfg, width, height, &(p_map_conv_cfg->out_frame_buffer[idx][jdx]), &(p_map_conv_cfg->out_vpu_buf[idx][jdx]), "OUTPUT LINEAR BUFFER", TRUE);
            }
        }
        if (strlen((const char*)(p_map_conv_cfg->outputPath+idx)) > 0) {
            if ((p_map_conv_cfg->fp_output[idx] = osal_fopen(p_map_conv_cfg->outputPath[idx], "wb")) == NULL) {
                VLOG(ERR, "%s:%d failed to open reference files : %s\n", __FUNCTION__, __LINE__, p_map_conv_cfg->outputPath[idx]);
            }
        }
    }

    return TRUE;
}

/**
 * GetDataFromFrameBuffer() - preparation for running MAPI
 * @p_test_config: test data struct
 * @fmt_property: property values of buffers
 * @fb: allocated frame buffers to store data
 * @width: picture width
 * @height: picture height
 * @mapi_format: FrameMapConvBufferFormat type value
 * RETURNS buffer pointer point to dumpped data
 */
static Uint8* GetDataFromFrameBuffer(
    TestMapConvConfig* p_test_config,
    MapiFormatProperty fmt_property,
    FrameBuffer*    fb,
    Uint32          width,
    Uint32          height,
    FrameMapConvBufferFormat mapi_format
    )
{
    Uint8*          p_frame;
    Uint8*          p_aligned_frame;
    Uint8*          pos;
    Uint8*          aligned_pos;
    Int32           idx = 0;
    FrameSizeInfo   frame_size_info;
    BOOL is_3p4b, is_msb, is_1plane, is_aligned;
    Uint8 valid_y_cnt = 0, valid_c_cnt = 0;
    Uint32 st_idx;
    Uint32* st_pos;
    MAPIFormat fmt;
    //FrameSizeInfo   aligned_frame_size_info;
    Int32 remain_size, quo_size = 0, re_idx;
    Int32 endian;

    is_3p4b    = fmt_property.is_3p4b;
    is_msb     = fmt_property.is_msb;
    is_1plane  = fmt_property.is_1plane;
    is_aligned = fmt_property.is_aligned;
    fmt        = fmt_property.mapi_fmt;

    osal_memset(&frame_size_info, 0x00, sizeof(FrameSizeInfo));

    CalFrameSize(p_test_config, mapi_format, width, height, &frame_size_info);

    if (frame_size_info.frame_size <= 0 || frame_size_info.aligned_frame_size <= 0) {
        VLOG(ERR, "%s:%d Check frame size : %d, aligned frame size : %d \n", frame_size_info.frame_size, frame_size_info.aligned_frame_size);
        return NULL;
    }

    if ((p_aligned_frame=(Uint8*)osal_malloc(frame_size_info.aligned_frame_size)) == NULL) {
        VLOG(ERR, "%s:%d Failed to allocate memory\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    if ((p_frame=(Uint8*)osal_malloc(frame_size_info.frame_size)) == NULL) {
        VLOG(ERR, "%s:%d Failed to allocate memory\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    osal_memset(p_frame, 0x00, frame_size_info.frame_size);
    osal_memset(p_aligned_frame, 0x00, frame_size_info.aligned_frame_size);

    endian = ~(p_test_config->outputEndian) & 0xf;
    vdi_read_memory(p_test_config->coreIdx, fb->bufY, p_aligned_frame, frame_size_info.aligned_frame_size, endian);


    if(is_3p4b) {
        if(is_1plane) {
            if (fmt == MAPI_FMT_444 || fmt == MAPI_FMT_RGB) {
                valid_y_cnt = (is_aligned) ? (width * 4) % 3 : (width * 3) % 3;
            } else {
                valid_y_cnt = (width * 2) % 3;
            }
            valid_c_cnt = 0;
        } else {
            valid_y_cnt = width % 3;
            valid_c_cnt = (fmt == MAPI_FMT_444) ? (width * 2) % 3 : width % 3;
        }
    }

    pos = p_frame;
    aligned_pos = p_aligned_frame;

    if (frame_size_info.aligned_frame_size != frame_size_info.frame_size) {
        for (idx =0 ; idx <frame_size_info.luma_height; idx++) { // Luma
            st_pos = (Uint32*)pos;
            if (is_3p4b && is_msb) {
                remain_size = (frame_size_info.luma_width % 16); //128 BIT BUS SYSTEM
                re_idx = 16 - remain_size;
                if (remain_size) {
                    quo_size = (frame_size_info.luma_width / 16) * 16;
                    osal_memcpy(pos, aligned_pos, quo_size);
                    osal_memcpy(pos + quo_size , (aligned_pos + quo_size + re_idx), remain_size);
                } else {
                    osal_memcpy(pos, aligned_pos, frame_size_info.luma_width);
                }
            } else {
                osal_memcpy(pos, aligned_pos, frame_size_info.luma_width);
            }
            if (is_3p4b && valid_y_cnt) {
                st_idx = (frame_size_info.luma_width - 4) / 4;
                if (is_msb) {
                    if (valid_y_cnt == 1) {
                        st_pos[quo_size/4] &= 0xff030000;
                    } else {
                        st_pos[quo_size/4] &= 0xffff0f00;
                    }
                } else {
                    if (valid_y_cnt == 1) {
                        st_pos[st_idx] &= 0x0000C0FF;
                    } else {
                        st_pos[st_idx] &= 0x00F0FFFF;
                    }
                }
            }
            aligned_pos += frame_size_info.luma_stride;
            pos += frame_size_info.luma_width;
        }
        for (idx =0 ; idx <frame_size_info.chroma_height; idx++) { // Chroma
            st_pos = (Uint32*)pos;
            osal_memcpy(pos, aligned_pos, frame_size_info.chroma_width);
            if (is_3p4b && is_msb) {
                remain_size = (frame_size_info.chroma_width % 16); //128 BIT BUS SYSTEM
                re_idx = 16 - remain_size;
                if (remain_size) {
                    quo_size = (frame_size_info.chroma_width / 16) * 16;
                    osal_memcpy(pos, aligned_pos, quo_size);
                    osal_memcpy(pos + quo_size , (aligned_pos + quo_size) + re_idx, remain_size);
                } else {
                    osal_memcpy(pos, aligned_pos, frame_size_info.chroma_width);
                }
            } else {
                osal_memcpy(pos, aligned_pos, frame_size_info.chroma_width);
            }
            if (is_3p4b && valid_c_cnt) {
                st_idx = (frame_size_info.chroma_width - 4) / 4;
                if (is_msb) {
                    if (valid_c_cnt == 1) {
                        st_pos[quo_size/4] &= 0xff030000;
                    } else {
                        st_pos[quo_size/4] &= 0xffff0f00;
                    }
                } else {
                    if (valid_c_cnt == 1) {
                        st_pos[st_idx] &= 0x0000C0FF;
                    } else {
                        st_pos[st_idx] &= 0x00F0FFFF;
                    }
                }
            }
            aligned_pos += frame_size_info.chroma_stride;
            pos += frame_size_info.chroma_width;
        }
    } else {
        osal_memcpy(p_frame, p_aligned_frame, frame_size_info.frame_size);
    }

#ifdef MAPI_TEST_ENABLE_DEBUG_MSG
    VLOG(INFO, "[MAP_CONV] %s -> dumpped frame size info frame size : %d \n", __FUNCTION__, frame_size);
#endif

    if (p_aligned_frame)
        osal_free(p_aligned_frame);

    return p_frame;
}

static BOOL CompareFrames(TestMapConvConfig* p_test_config, Int32 frame_idx)
{
    BOOL match = FALSE, ret = TRUE;
    Int32 idx = 0;
    Uint8 *p_frame[MAPI_TEST_MAX_OUTPUT_PATH] = {0,};
    Uint8 *p_ref_frame[MAPI_TEST_MAX_OUTPUT_PATH] = {0,};
    Int32 width, height;
    osal_file_t* p_fp_ref = p_test_config->fp_ref;
    FrameSizeInfo fb_size_info[MAPI_TEST_MAX_OUTPUT_PATH];
    Int32 compare_cnt = MAPI_TEST_MAX_OUTPUT_PATH;
    Int32 buf_idx = 0, output_fb_num = 0;

    output_fb_num = p_test_config->output_fb_num;
    buf_idx = (output_fb_num > 1) ? (frame_idx % output_fb_num) : 0;

    osal_memset(fb_size_info, 0x00, sizeof(FrameSizeInfo)*MAPI_TEST_MAX_OUTPUT_PATH);

    width  = p_test_config->width;
    height = p_test_config->height;

    for (idx =0; idx < compare_cnt; idx++) {
        if (p_test_config->out_mode[idx] > MAPI_O_MODE_NONE) {
            if (!p_fp_ref[idx]) {
                if ((p_fp_ref[idx]=osal_fopen(p_test_config->ref_path[idx], "rb")) == NULL) {
                    VLOG(ERR, "%s:%d failed to open reference files : %s\n", __FUNCTION__, __LINE__, p_test_config->ref_path[idx]);
                    return FALSE;
                }
            }
        }
    }

    for (idx = 0 ; idx < compare_cnt; idx++) {
        if (p_test_config->out_mode[idx] > MAPI_O_MODE_NONE) {
            width  = p_test_config->width;
            height = p_test_config->height;
            if (p_test_config->scl_width[idx] != 0) {
                width  = p_test_config->scl_width[idx];
            }
            if (p_test_config->scl_height[idx] != 0) {
                height = p_test_config->scl_height[idx];
            }
            CalFrameSize(p_test_config, p_test_config->output_buf_format[idx], width, height, &fb_size_info[idx]);

            if (fb_size_info[idx].frame_size < 0 ) {
                VLOG(ERR, "%s:%d check frame size : %d\n", __FUNCTION__, __LINE__, fb_size_info[idx].frame_size);
            return FALSE;
            }

            p_ref_frame[idx] = (Uint8*)osal_malloc(fb_size_info[idx].frame_size);
            osal_fread(p_ref_frame[idx], 1, fb_size_info[idx].frame_size, p_fp_ref[idx]);
            p_frame[idx] = GetDataFromFrameBuffer(p_test_config, p_test_config->output_format_property[idx] ,&(p_test_config->out_frame_buffer[idx][buf_idx]), width, height, p_test_config->output_buf_format[idx]);

            if (p_test_config->fp_output[idx]) {
                osal_fwrite(p_frame[idx], 1, fb_size_info[idx].frame_size, p_test_config->fp_output[idx]);
            }
        }
    }



    for (idx =0 ; idx < compare_cnt; idx++) {
        if (p_test_config->out_mode[idx] > MAPI_O_MODE_NONE) {
            match = (osal_memcmp((void*)p_ref_frame[idx], (void*)p_frame[idx], fb_size_info[idx].frame_size) == 0 ? TRUE : FALSE);

            if (match == FALSE) {
                ret = FALSE;
                VLOG(ERR, "%d frame out_mode[%d] comparison failure\n", frame_idx + 1, idx);
                osal_file_t fp_out_golden;
                osal_file_t fp_out_fpga;
                char temp_str[MAX_FILE_PATH] = {0,};

                VLOG(ERR, "MISMATCH WITH GOLDEN YUV at %d frame\n", frame_idx+1);
                sprintf(temp_str, "./_golden_%d.yuv", idx);

                if ((fp_out_golden=osal_fopen(temp_str, "wb")) == NULL) {
                    VLOG(ERR, "Faild to create golden.yuv\n");
                    osal_free(p_ref_frame[idx]);
                    return FALSE;
                }
                VLOG(INFO, "Saving... Golden YUV at %s\n", temp_str);

                sprintf(temp_str, "./_decoded_%d.yuv", idx);

                osal_fwrite(p_ref_frame[idx], 1, fb_size_info[idx].frame_size, fp_out_golden);
                osal_fclose(fp_out_golden);
                if ((fp_out_fpga=osal_fopen(temp_str, "wb")) == NULL) {
                    VLOG(ERR, "Faild to create golden.yuv\n");
                    osal_free(p_frame[idx]);
                    return FALSE;
                }
                VLOG(INFO, "Saving... decoded YUV at %s\n", temp_str);
                osal_fwrite(p_frame[idx], 1, fb_size_info[idx].frame_size, fp_out_fpga);
                osal_fclose(fp_out_fpga);
            } else {
                VLOG(INFO, "%d frame out_mode[%d] comparison success\n", frame_idx + 1, idx);
            }
        }
    }
    
    if ((frame_idx+1) == p_test_config->forceOutNum) {
        for (idx =0; idx < compare_cnt; idx++) {
            if (p_fp_ref[idx]) {
                osal_fclose(p_fp_ref[idx]);
            }
        }
    }

    for (idx =0 ; idx < compare_cnt; idx++) {
        if(p_frame[idx])
            osal_free(p_frame[idx]);
        if(p_ref_frame[idx])
            osal_free(p_ref_frame[idx]);
    }

    return ret;
}

/**
 * DestroyMapConv() - release allcated resources
 * @p_test_config: test data struct
 */
static BOOL DestroyMapConv(TestMapConvConfig* p_test_config)
{
    Int32 idx = 0, jdx = 0;

    for (idx = 0; idx < MAPI_TEST_MAX_NUM_INPUT_BUF; idx++) {
        if (p_test_config->in_vpu_buf[idx].size > 0) {
            vdi_free_dma_memory(p_test_config->coreIdx, &(p_test_config->in_vpu_buf[idx]), DEC_FB_LINEAR, 0);
        }
    }

    for(idx =0; idx < MAPI_TEST_MAX_OUTPUT_PATH; idx++) {
        for (jdx=0; jdx < MAPI_TEST_MAX_NUM_OUTPUT_BUF; jdx++) {
            if (p_test_config->out_vpu_buf[idx][jdx].size > 0) {
                vdi_free_dma_memory(p_test_config->coreIdx, &(p_test_config->out_vpu_buf[idx][jdx]), DEC_FB_LINEAR, 0);
            }
        }
    }

    for(idx =0; idx < MAPI_TEST_MAX_OUTPUT_PATH; idx++) {
        if (p_test_config->fp_output[idx]) {
            osal_fclose(p_test_config->fp_output[idx]);
        }
    }
    return TRUE;
}

/**
 * LoadInputLinearData() - read input linear data and write input data to external memory
 * @p_test_config: test data struct
 */
static BOOL LoadInputLinearData(
    TestMapConvConfig* p_test_config,
    MapiFormatProperty fmt_property
    )
{
    Uint32 file_size = 0, idx = 0, remainder = 0;
    PhysicalAddress addr;
    char str_path[MAX_FILE_PATH];
    FrameSizeInfo fb_size_info;
    Uint8* p_frame = NULL;
    Uint8* p_aligned_frame = NULL;
    Uint8* pos = NULL;
    Uint8* aligned_pos = NULL;
    static Uint32 frame_cnt = 0;
    static Uint32 load_cnt = 0;
    Uint32 buf_idx = 0, input_fb_num = 1;
    Int32 endian;
    BOOL is_3p4b, is_msb;
    Int32 width, height;
    Int32 remain_size, quo_size, re_idx;

     width  = p_test_config->width;
     height = p_test_config->height;

    input_fb_num = p_test_config->input_fb_num;
    if (input_fb_num > 1) {
        buf_idx = load_cnt % input_fb_num;
    }

    osal_memset(str_path, 0x00, sizeof(str_path));
    sprintf(str_path, "%s", p_test_config->inputPath);

    CalFrameSize(p_test_config, p_test_config->input_buf_format, width, height, &fb_size_info);

    if (load_cnt !=0 && frame_cnt < 1) {
        for (idx = 0; idx < p_test_config->input_fb_num; idx++) {
            if (p_test_config->fp_input[idx]) {
                osal_fclose(p_test_config->fp_input[idx]);
                p_test_config->fp_input[idx] = NULL;
            }
        }
        return TRUE;
    }

    //YUV Data file open
    if (load_cnt == 0) {
        if (NULL == (p_test_config->fp_input[0]=osal_fopen(str_path, "r"))) {
            VLOG(ERR, "%s:%d fail to open, %s \n", __FUNCTION__, __LINE__, str_path);
            return FALSE;
        }
    }

    if (load_cnt == 0) {
        file_size = CalcFileSize(str_path);
        frame_cnt = file_size / fb_size_info.frame_size;
        remainder = file_size % fb_size_info.frame_size;
        
        if (remainder) {
            VLOG(WARN, "%s:%d Check file size : %d and frame size : %d \n", __FUNCTION__, __LINE__, file_size, fb_size_info.frame_size);
        }
        if (frame_cnt < p_test_config->forceOutNum) {
            p_test_config->forceOutNum = frame_cnt;
            VLOG(WARN, "%s:%d decrease number of outputs : %d --> %d \n", __FUNCTION__, __LINE__, p_test_config->forceOutNum, frame_cnt);
        }
    }

    if ((p_frame=(Uint8*)osal_malloc(fb_size_info.frame_size)) == NULL) {
        VLOG(ERR, "%s:%d fail to allocate buffer\n", __FUNCTION__, __LINE__);
        return FALSE;
    }
    if ((p_aligned_frame=(Uint8*)osal_malloc(fb_size_info.aligned_frame_size)) == NULL) {
        VLOG(ERR, "%s:%d fail to allocate buffer\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    addr = p_test_config->in_frame_buffer[buf_idx].bufY;

    osal_memset(p_frame, 0x00, fb_size_info.frame_size);
    osal_memset(p_aligned_frame, 0x00, fb_size_info.aligned_frame_size);
    osal_fread(p_frame, 1, fb_size_info.frame_size, p_test_config->fp_input[0]);

    pos = p_frame;
    aligned_pos = p_aligned_frame;

    is_3p4b = fmt_property.is_3p4b;
    is_msb  = fmt_property.is_msb;

    if (fb_size_info.aligned_frame_size != fb_size_info.frame_size) {
        for (idx = 0; idx < fb_size_info.luma_height; idx++) {  // LUMA
            if (is_3p4b && is_msb) {
                remain_size = (fb_size_info.luma_width % 16); //128 BIT BUS SYSTEM
                re_idx = 16 - remain_size;
                if (remain_size) {
                    quo_size = (fb_size_info.luma_width / 16) * 16;
                    osal_memcpy(aligned_pos, pos, quo_size);
                    osal_memcpy(aligned_pos + quo_size + re_idx, (pos + quo_size), remain_size);
                } else {
                    osal_memcpy(aligned_pos, pos, fb_size_info.luma_width);
                }
            } else {
                osal_memcpy(aligned_pos, pos, fb_size_info.luma_width);
            }
            aligned_pos += fb_size_info.luma_stride;
            pos += fb_size_info.luma_width;
        }
        for (idx = 0; idx < fb_size_info.chroma_height; idx++) { // Chroma
            osal_memcpy(aligned_pos, pos, fb_size_info.chroma_width);
            if (is_3p4b && is_msb) {
                remain_size = (fb_size_info.chroma_width % 16); //128 BIT BUS SYSTEM
                re_idx = 16 - remain_size;
                if (remain_size) {
                    quo_size = (fb_size_info.chroma_width / 16) * 16;
                    osal_memcpy(aligned_pos, pos, quo_size);
                    osal_memcpy(aligned_pos + quo_size + re_idx, (pos + quo_size), remain_size);
                } else {
                    osal_memcpy(aligned_pos, pos, fb_size_info.luma_width);
                }
            } else {
                osal_memcpy(aligned_pos, pos, fb_size_info.chroma_width);
            }
            aligned_pos += fb_size_info.chroma_stride;
            pos += fb_size_info.chroma_width;
        }
    } else {
        osal_memcpy(p_aligned_frame, p_frame, fb_size_info.frame_size);
    }

    endian = ~(p_test_config->inputEndian) & 0xf;
    vdi_write_memory(p_test_config->coreIdx, addr, p_aligned_frame, fb_size_info.aligned_frame_size, endian);

    frame_cnt--;

    if (p_frame) {
        osal_free(p_frame);
    }
    if (p_aligned_frame) {
        osal_free(p_aligned_frame);
    }

    load_cnt++;

    return TRUE;
}

/**
 * LoadInputFBCData() - read input compressed data and write input data to external memory
 * @p_test_config: test data struct
 */
static BOOL LoadInputFBCData(
    TestMapConvConfig* p_test_config
    )
{
    Uint8* p_fbc_data[MAPI_TEST_MAX_INPUT_PATH] ;
    Int32 idx = 0;
    Int32 data_size_arr[MAPI_TEST_MAX_INPUT_PATH];
    char str_fbc_path[MAPI_TEST_MAX_INPUT_PATH][MAX_FILE_PATH];
    Uint32 frame_idx = p_test_config->frame_idx;
    char* p_fbc_dir = p_test_config->fbc_dir;

    osal_memset(p_fbc_data, 0x00, sizeof(p_fbc_data));
    osal_memset(data_size_arr, 0x00, sizeof(data_size_arr));
    osal_memset(str_fbc_path, 0x00, sizeof(str_fbc_path));

    for (idx = 0; idx < MAPI_TEST_MAX_INPUT_PATH; idx++) {
        switch (idx)
        {
        case 0:
            sprintf(str_fbc_path[idx], "%sf%03d_fbc_data_y.bin", p_fbc_dir, frame_idx);
            data_size_arr[idx] = p_test_config->in_compressed_fb[0].bufYSize;
            break;
        case 1: 
            sprintf(str_fbc_path[idx], "%sf%03d_fbc_data_c.bin", p_fbc_dir, frame_idx);
            data_size_arr[idx] = p_test_config->in_compressed_fb[0].bufCbSize;
            break;
        case 2: 
            sprintf(str_fbc_path[idx], "%sf%03d_fbc_table_y.bin", p_fbc_dir, frame_idx);
            data_size_arr[idx] = p_test_config->in_compressed_fb[1].size;
            break;
        case 3: 
            sprintf(str_fbc_path[idx], "%sf%03d_fbc_table_c.bin", p_fbc_dir, frame_idx);
            data_size_arr[idx] = p_test_config->in_compressed_fb[2].size;
            break;
        default: break;
        }
        if (data_size_arr[idx] > 0) {
            if (NULL == (p_test_config->fp_input[idx]=osal_fopen(str_fbc_path[idx], "r"))) {
                VLOG(ERR, "%s:%d fail to open : %s\n", __FUNCTION__, __LINE__, str_fbc_path[idx]);
                return FALSE;
            }
        }
    }

    for (idx = 0 ; idx < MAPI_TEST_MAX_INPUT_PATH; idx++) {
        if (data_size_arr[idx] > 0) {
            if ((p_fbc_data[idx]=(Uint8*)osal_malloc(data_size_arr[idx])) == NULL) {
                VLOG(ERR, "%s:%d fail to allocate buffer\n", __FUNCTION__, __LINE__);
                return FALSE;
            }
            osal_fread(p_fbc_data[idx], 1, data_size_arr[idx], p_test_config->fp_input[idx]);
        }
    }

    vdi_write_memory(p_test_config->coreIdx, p_test_config->in_compressed_fb[0].bufY, p_fbc_data[0], data_size_arr[0], p_test_config->in_compressed_fb[0].endian);
    vdi_write_memory(p_test_config->coreIdx, p_test_config->in_compressed_fb[0].bufCb, p_fbc_data[1], data_size_arr[1], p_test_config->in_compressed_fb[0].endian);
    vdi_write_memory(p_test_config->coreIdx, p_test_config->in_compressed_fb[1].bufY, p_fbc_data[2], data_size_arr[2], p_test_config->in_compressed_fb[1].endian);
    vdi_write_memory(p_test_config->coreIdx, p_test_config->in_compressed_fb[2].bufY, p_fbc_data[3], data_size_arr[3], p_test_config->in_compressed_fb[2].endian);

    for (idx = 0 ; idx < MAPI_TEST_MAX_INPUT_PATH; idx++) {
        if (p_test_config->fp_input[idx]) {
            osal_fclose(p_test_config->fp_input[idx]);
        }
        if (p_fbc_data[idx])
            osal_free(p_fbc_data[idx]);
    }

    return TRUE;
}

/**
 * LoadInputData() - read input data and write input data to external memory
 * @p_test_config: test data struct
 */
static BOOL LoadInputData(TestMapConvConfig* p_test_config)
{
    BOOL ret = FALSE;
    if (MAPI_INPUT_RDMA == p_test_config->input_ctrl_mode || MAPI_INPUT_DISP == p_test_config->input_ctrl_mode)
    {
        ret = LoadInputLinearData(p_test_config, p_test_config->input_format_property);

    } else if (MAPI_INPUT_CF10 == p_test_config->input_ctrl_mode){
        ret = LoadInputFBCData(p_test_config);
    } else {
        VLOG(ERR, "Check Input Control Mode : %d \n", p_test_config->input_ctrl_mode);
    }
    return ret;
}

/**
 * ParseFGNPicParam() - read and parse lf_film_grain_pic_param.txt
 * @p_test_config: test data struct
 */
static BOOL ParseFGNPicParam(TestMapConvConfig* p_test_config)
{
    osal_file_t fp;
    Int32 idx = 0;
    FgnInputData* p_fgn_input_data = &(p_test_config->fgn_input_data);
    char fgn_info_path[MAX_FILE_PATH];
    char line_str[MAX_FILE_PATH];
    char* token;
    Int32 val;
    Uint32 frame_idx = p_test_config->frame_idx;

    memset(fgn_info_path, 0x00, MAX_FILE_PATH);

    sprintf(fgn_info_path, "%sf%03d_%s", FGN_DATA_PRE_PATH, frame_idx, FGN_PIC_PARAM_POST_PATH);

    fp = osal_fopen(fgn_info_path, "r");
    if (NULL == fp) {
        VLOG(ERR, "%s:%d fail to open : %s\n", __FUNCTION__, __LINE__, fgn_info_path);
        return FALSE;
    }

    for (idx = 0; idx < FGN_PIC_PARAM_DATA_NUM; idx++) {
        fgets(line_str, sizeof(line_str), fp);
        token = strtok(line_str, ":");
        token = strtok(NULL, ":");
        if (token)
            val = atoi(token);
        else
            return FALSE;
        p_fgn_input_data->fgn_pic_param_data[idx] = val;
    }
    osal_fclose(fp);

    return TRUE;
}

/**
 * SetFgnPicParam() - make fgn info data and write data to extermal memory
 * @p_test_config: test data struct
 */
BOOL SetFgnPicParam(TestMapConvConfig* p_test_config)
{
    Int32 idx = 0, dat_idx = 0, pos_idx = 0;
    Uint32* p_fgn_raw_data;
    Int32 *p_fgn_info = p_test_config->fgn_input_data.fgn_pic_param_data;
    PhysicalAddress fgn_info_addr = p_test_config->fgn_input_data.fgn_pic_param_buf.phys_addr;
    PhysicalAddress fgn_mem_addr= p_test_config->fgn_input_data.fgn_mem_buf.phys_addr;
    Uint32 mapi_fgn_ctrl_0 = 0;
    Uint32 mapi_fgn_ctrl_1 = 0;
    Uint32 mapi_fgn_ctrl_2 = 0;
    osal_file_t fp;
    char fgn_mem_path[MAX_FILE_PATH];
    Uint8* p_mem_buf;
    Uint32 frame_idx = p_test_config->frame_idx;
    Uint32 fgn_mem_file_size;

    memset(fgn_mem_path, 0x00, MAX_FILE_PATH);
    sprintf(fgn_mem_path, "%sf%03d_%s", FGN_DATA_PRE_PATH, frame_idx, FGN_MEM_POST_PATH);

    p_fgn_raw_data = osal_malloc(MAPI_CF10_FGN_DATA_SIZE);

    osal_memset(p_fgn_raw_data, 0x00, sizeof(MAPI_CF10_FGN_DATA_SIZE));

    mapi_fgn_ctrl_0 = ((p_fgn_info[FGN_INFO_SCALING_SHIFT_MINUS8] << 28) | (p_fgn_info[FGN_INFO_MATRIX_COEFFICIENTS] << 20) 
            | (p_fgn_info[FGN_INFO_CLIP_TO_RESTRICTED_RANGE] << 19) | (p_fgn_info[FGN_INFO_OVERLAP_FLAG] << 18)
            | (p_fgn_info[FGN_INFO_CHROMA_SCALING_FROM_LUMA] << 17) | (p_fgn_info[FGN_INFO_GRAIN_SEED] << 1)
            | (p_fgn_info[FGN_INFO_APPLY_GRAIN]));

    p_fgn_raw_data[dat_idx++]  = mapi_fgn_ctrl_0;

    mapi_fgn_ctrl_1 = ((p_fgn_info[FGN_INFO_CR_MULT] << 24) | (p_fgn_info[FGN_INFO_CR_LUMA_MULT] << 16)
            | (p_fgn_info[FGN_INFO_CB_MULT] << 8) | p_fgn_info[FGN_INFO_CB_LUMA_MULT]);

    p_fgn_raw_data[dat_idx++]  = mapi_fgn_ctrl_1;

    mapi_fgn_ctrl_2 = ((p_fgn_info[FGN_INFO_NUM_CR_POINTS] << 26) | (p_fgn_info[FGN_INFO_NUM_CB_POINTS] << 22)
            | (p_fgn_info[FGN_INFO_NUM_Y_POINTS] << 18) | (p_fgn_info[FGN_INFO_CR_OFFSET] << 9)
            | p_fgn_info[FGN_INFO_CB_OFFSET]);

    p_fgn_raw_data[dat_idx++]  = mapi_fgn_ctrl_2;

    p_fgn_raw_data[dat_idx++] = fgn_mem_addr;

    for(idx = 0; idx < FGN_POINT_Y_DATA_NUM/4; idx++) {
        pos_idx = FGN_INFO_POINT_Y_VALUE + (idx * 4);
        p_fgn_raw_data[dat_idx++] = (p_fgn_info[pos_idx + 0] | (p_fgn_info[pos_idx + 1] << 8)
            | (p_fgn_info[pos_idx + 2] << 16) | (p_fgn_info[pos_idx + 3] << 24));
    }

    for(idx = 0; idx < FGN_POINT_CB_DATA_NUM/4; idx++) {
        pos_idx = FGN_INFO_POINT_CB_VALUE + (idx * 4);
        p_fgn_raw_data[dat_idx++] = (p_fgn_info[pos_idx + 0] | (p_fgn_info[pos_idx + 1] << 8)
            | (p_fgn_info[pos_idx + 2] << 16) | (p_fgn_info[pos_idx + 3] << 24));
    }

    for(idx = 0; idx < FGN_POINT_CR_DATA_NUM/4; idx++) {
        pos_idx = FGN_INFO_POINT_CR_VALUE + (idx * 4);
        p_fgn_raw_data[dat_idx++] = (p_fgn_info[pos_idx + 0] | (p_fgn_info[pos_idx + 1] << 8)
            | (p_fgn_info[pos_idx + 2] << 16) | (p_fgn_info[pos_idx + 3] << 24));
    }


    VpuWriteMem(p_test_config->coreIdx, fgn_info_addr, (Uint8 *)p_fgn_raw_data, MAPI_CF10_FGN_DATA_SIZE, VDI_128BIT_BIG_ENDIAN);

    if (NULL == (fp = osal_fopen(fgn_mem_path, "r"))) {
        VLOG(ERR, "%s:%d fail to open : %s\n", __FUNCTION__, __LINE__, fgn_mem_path);
        return FALSE;
    }

    fgn_mem_file_size = CalcFileSize(fgn_mem_path);
    p_mem_buf = osal_malloc(FGN_MEM_SIZE);

    if(!p_mem_buf) {
        VLOG(ERR, "%s:%d NULL poiter exception \n", __FUNCTION__, __LINE__);
        return FALSE;
    }
    osal_fread(p_mem_buf, 1, fgn_mem_file_size, fp);
    VpuWriteMem(p_test_config->coreIdx, fgn_mem_addr, p_mem_buf, FGN_MEM_SIZE, VDI_128BIT_BE_WORD_BYTE_SWAP);
    osal_fclose(fp);

    if (p_fgn_raw_data) {
        osal_free(p_fgn_raw_data);
    }

    return TRUE;
}


/**
 * SetLinearInputProperty() - Set linear input propery values according to input mode
 * @p_test_config: test data struct
 */
static BOOL SetLinearInputProperty(TestMapConvConfig* p_test_config, TestMAPIAPICtrl* p_mapi_ctrl, MapiFormatProperty fmt_property)
{
    FrameMapConvBufferFormat mapi_format = p_test_config->input_buf_format;
    BOOL is_MSB = FALSE, is_3p4b = FALSE;
    BOOL is_nv21 = p_test_config->in_nv21;
    FrameSizeInfo fb_size_info = {0, };
    Uint32 width, height;

    is_3p4b = fmt_property.is_3p4b;
    is_MSB  = fmt_property.is_msb;

    width  = p_test_config->width;
    height = p_test_config->height;

    CalFrameSize(p_test_config, mapi_format, width, height, &fb_size_info);

    if (MAPI_INPUT_RDMA == p_test_config->input_ctrl_mode) {
        p_mapi_ctrl->mapi_input_rdma.yBase    = p_test_config->in_frame_buffer[0].bufY;
        p_mapi_ctrl->mapi_input_rdma.cbBase   = p_test_config->in_frame_buffer[0].bufCb;
        p_mapi_ctrl->mapi_input_rdma.crBase   = p_test_config->in_frame_buffer[0].bufCr;
        p_mapi_ctrl->mapi_input_rdma.endian   = (EndianMode)(p_test_config->in_frame_buffer[0].endian);
        p_mapi_ctrl->mapi_input_rdma.isMSB    = is_MSB;
        p_mapi_ctrl->mapi_input_rdma.is3P4B   = is_3p4b;
        p_mapi_ctrl->mapi_input_rdma.isNV21   = is_nv21;
    }
    else if (MAPI_INPUT_DISP == p_test_config->input_ctrl_mode) {
        p_mapi_ctrl->mapi_input_display.enable2PortMode = p_test_config->input_display_2port;
        p_mapi_ctrl->mapi_input_display.vsync = TRUE;
    } else {
        VLOG(ERR, "%s:%d Check input control mode : %d\n", __FUNCTION__, __LINE__, p_test_config->input_ctrl_mode);
        return FALSE;
    }
    return TRUE;
}

/**
 * ReSetLinearInputProperty() - Reset linear input propery values according to input mode
 */
static BOOL ReSetLinearInputProperty(TestMapConvConfig* p_test_config, TestMAPIAPICtrl* p_mapi_ctrl, MapiFormatProperty fmt_property)
{
    Uint32 frame_idx = p_test_config->frame_idx;
    Int32 in_fb_num  = p_test_config->input_fb_num;
    Uint32 buf_idx   = 0;
    buf_idx = (in_fb_num > 1) ? (frame_idx % in_fb_num) : 0 ;
    if (MAPI_INPUT_RDMA == p_test_config->input_ctrl_mode) {
        p_mapi_ctrl->mapi_input_rdma.yBase  = p_test_config->in_frame_buffer[buf_idx].bufY;
        p_mapi_ctrl->mapi_input_rdma.cbBase = p_test_config->in_frame_buffer[buf_idx].bufCb;
        p_mapi_ctrl->mapi_input_rdma.crBase = p_test_config->in_frame_buffer[buf_idx].bufCr;
    }
    else if (MAPI_INPUT_DISP == p_test_config->input_ctrl_mode) {
    } else {
        VLOG(ERR, "%s:%d Check input control mode : %d\n", __FUNCTION__, __LINE__, p_test_config->input_ctrl_mode);
        return FALSE;
    }
    return TRUE;
}

/**
 * SetCompressedInputProperty() - Set compressed input property values according to input mode
 */
static BOOL SetCompressedInputProperty(TestMapConvConfig* p_test_config, TestMAPIAPICtrl* p_mapi_ctrl) {
    if (MAPI_INPUT_CF10 == p_test_config->input_ctrl_mode) {
        p_mapi_ctrl->mapi_input_cf10.yCompBase = p_test_config->in_compressed_fb[0].bufY;
        p_mapi_ctrl->mapi_input_cf10.cCompBase = p_test_config->in_compressed_fb[0].bufCb;
        p_mapi_ctrl->mapi_input_cf10.yOffsetBase = p_test_config->in_compressed_fb[1].bufY;
        p_mapi_ctrl->mapi_input_cf10.cOffsetBase = p_test_config->in_compressed_fb[2].bufY;
        p_mapi_ctrl->mapi_input_cf10.endian = (EndianMode)(p_test_config->in_compressed_fb[0].endian);
        p_mapi_ctrl->mapi_input_cf10.isOffsetTbl64x64 = p_test_config->is_offset_tb_64;
        p_mapi_ctrl->mapi_input_cf10.isMonoChroma = p_test_config->is_monochrome;
        p_mapi_ctrl->mapi_input_cf10.round10bitTo8bit = p_test_config->cf10_10b_to_8b;
        p_mapi_ctrl->mapi_input_cf10.fgn.enable = p_test_config->enable_fgn;
        if (p_test_config->enable_fgn) {
            p_mapi_ctrl->mapi_input_cf10.fgn.bufBase = p_test_config->fgn_input_data.fgn_pic_param_buf.phys_addr;
            p_mapi_ctrl->mapi_input_cf10.fgn.enable = TRUE;
            ParseFGNPicParam(p_test_config);
            SetFgnPicParam(p_test_config);
        }
    }
    else {
        VLOG(ERR, "%s:%d Check input control mode : %d\n", __FUNCTION__, __LINE__, p_test_config->input_ctrl_mode);
        return FALSE;
    }
    return TRUE;
}

/**
 * ReSetCompressedInputProperty() - Reset compressed input property values according to input mode
 */
static BOOL ReSetCompressedInputProperty(TestMapConvConfig* p_test_config, TestMAPIAPICtrl* p_mapi_ctrl) {
    if (MAPI_INPUT_CF10 == p_test_config->input_ctrl_mode) {
        if (p_test_config->enable_fgn) {
            ParseFGNPicParam(p_test_config);
            SetFgnPicParam(p_test_config);
        }
        p_mapi_ctrl->mapi_input_cf10.fgn = p_mapi_ctrl->mapi_input_fgn;
    }
    else {
        VLOG(ERR, "%s:%d Check input control mode : %d\n", __FUNCTION__, __LINE__, p_test_config->input_ctrl_mode);
        return FALSE;
    }
    return TRUE;
}

/**
 * SetInputPath() - Set input common values
 */
static BOOL SetInputPath(TestMapConvConfig* p_test_config, MapiFormatProperty fmt_property)
{
    TestMAPIAPICtrl* p_mapi_ctrl = &(p_test_config->mapi_ctrl);
    MAPIMode mode;
    Uint32 width, height;
    MAPIFormat fmt;
    Uint32 bit_depth;
    void* p_data;
    FrameSizeInfo fb_size_info;
    BOOL enable_crop = FALSE;
    VpuRect crop_rect = { 0, };

    width = p_test_config->width;
    height = p_test_config->height;

    if (p_test_config->input_ctrl_mode == MAPI_INPUT_DISP || p_test_config->input_ctrl_mode == MAPI_INPUT_RDMA) {
        bit_depth = p_test_config->in_frame_buffer[0].lumaBitDepth;
    } else { //CF10
        bit_depth = p_test_config->in_compressed_fb[0].lumaBitDepth;
    }

    switch (p_test_config->input_ctrl_mode)
    {
    case MAPI_INPUT_RDMA:
        mode = MAPI_I_MODE_YUV;
        p_data = &(p_mapi_ctrl->mapi_input_rdma);
        break;
    case MAPI_INPUT_DISP:
        mode = MAPI_I_MODE_DIS;
        p_data = &(p_mapi_ctrl->mapi_input_display);
        break;
    case MAPI_INPUT_CF10: 
        mode = MAPI_I_MODE_CF10;
        p_data = &(p_mapi_ctrl->mapi_input_cf10);
        break;
    default : 
        return FALSE;
    }

    fmt = fmt_property.mapi_fmt;

    p_mapi_ctrl->mapi_input_path.mode       = mode;
    p_mapi_ctrl->mapi_input_path.width      = width;
    p_mapi_ctrl->mapi_input_path.height     = height;
    p_mapi_ctrl->mapi_input_path.bitDepth   = bit_depth;
    p_mapi_ctrl->mapi_input_path.format     = fmt;
    p_mapi_ctrl->mapi_input_path.data       = p_data;
    p_mapi_ctrl->mapi_input_path.filter     = (MAPIFilter)(p_test_config->bilinear_mode);
    CalFrameSize(p_test_config, p_test_config->input_buf_format, width, height, &fb_size_info);

    if (mode == MAPI_I_MODE_CF10) {
        if (p_test_config->input_buf_format == MAPI_FORMAT_420_P10_16BIT_LSB || p_test_config->cf10_10b_to_8b == TRUE ) {
            fb_size_info.luma_stride   = VPU_ALIGN32(VPU_ALIGN16(width)*5);
            fb_size_info.chroma_stride = VPU_ALIGN32(VPU_ALIGN16(width/2)*5);
        } else {
            fb_size_info.luma_stride   = VPU_ALIGN32(VPU_ALIGN16(width)*4);
            fb_size_info.chroma_stride = VPU_ALIGN32(VPU_ALIGN16(width/2)*4);
        }
    }

    p_mapi_ctrl->mapi_input_path.stride     = fb_size_info.luma_stride;
    p_mapi_ctrl->mapi_input_path.chStride   = fb_size_info.chroma_stride;
    p_mapi_ctrl->mapi_input_path.scalerMode = (MAPIScalerMode)(p_test_config->scaler_mode);

    if (fmt_property.is_1plane == TRUE && fmt_property.is_packed == TRUE)
        p_mapi_ctrl->mapi_input_path.planeMode = MAPI_PLANE_MODE_1P_PACKED;
    else if (fmt_property.is_1plane == TRUE && fmt_property.is_packed == FALSE)
        p_mapi_ctrl->mapi_input_path.planeMode = MAPI_PLANE_MODE_1P;
    else
        p_mapi_ctrl->mapi_input_path.planeMode = MAPI_PLANE_MODE_DEFAULT;

    p_mapi_ctrl->mapi_input_path.enableCrop = enable_crop; //TODO Check
    p_mapi_ctrl->mapi_input_path.cropRect = crop_rect;//TODO Check

    return TRUE;
}


/**
 * ReSetOutputPath() - Reset output common values
 */
static BOOL ReSetOutputPath(TestMapConvConfig* p_test_config)
{
    TestMAPIAPICtrl* p_mapi_ctrl = &(p_test_config->mapi_ctrl);
    FrameBuffer (*out_fbs)[MAPI_TEST_MAX_NUM_OUTPUT_BUF] = p_test_config->out_frame_buffer;
    Int32 idx = 0, output_fb_num, frame_idx, output_fb_idx = 0;
    MAPIMode out_mode = MAPI_O_MODE_NONE;

    output_fb_num = p_test_config->output_fb_num;
    frame_idx     = p_test_config->frame_idx;

    for (idx = 0; idx < MAPI_TEST_MAX_OUTPUT_PATH; idx++) {
        out_mode = p_test_config->out_mode[idx];
        if (out_mode == MAPI_O_MODE_NONE) break;
        if (output_fb_num > 1) {
            output_fb_idx = (frame_idx % output_fb_num);
        }

        if (out_mode == MAPI_O_MODE_DIS) {
        } else if (out_mode == MAPI_O_MODE_YUV) {
            p_mapi_ctrl->mapi_output_wdma.yBase  = out_fbs[idx][output_fb_idx].bufY;
            p_mapi_ctrl->mapi_output_wdma.cbBase = out_fbs[idx][output_fb_idx].bufCb;
            p_mapi_ctrl->mapi_output_wdma.crBase = out_fbs[idx][output_fb_idx].bufCr;
            VLOG(TRACE, "[Check] wdma y base : 0x%x \n", p_mapi_ctrl->mapi_output_wdma.yBase);
        }
    }
    return TRUE;
}

/**
 * SetOutputProperty() - Set output property values according to output mode
 */
static BOOL SetOutputProperty(TestMapConvConfig* p_test_config, MapiFormatProperty* fmt_properties)
{
    TestMAPIAPICtrl* p_mapi_ctrl = &(p_test_config->mapi_ctrl);
    FrameBuffer (*out_fbs)[MAPI_TEST_MAX_NUM_OUTPUT_BUF] = p_test_config->out_frame_buffer;
    Int32 idx = 0;
    MAPIMode out_mode = MAPI_O_MODE_NONE;

    for (idx = 0; idx < MAPI_TEST_MAX_OUTPUT_PATH; idx++) {
        out_mode = p_test_config->out_mode[idx];
        if (out_mode > MAPI_O_MODE_NONE) {
            if (out_mode == MAPI_O_MODE_DIS) {
            p_mapi_ctrl->mapi_output_display.enable2PortMode = p_test_config->output_display_2port;
            p_mapi_ctrl->mapi_output_display.scaler          = p_test_config->mapi_ctrl.mapi_output_scaler[idx];
            p_mapi_ctrl->mapi_output_display.disable         = FALSE;

            } else if (out_mode == MAPI_O_MODE_YUV) {
                p_mapi_ctrl->mapi_output_wdma.yBase          = out_fbs[idx][0].bufY;
                p_mapi_ctrl->mapi_output_wdma.cbBase         = out_fbs[idx][0].bufCb;
                p_mapi_ctrl->mapi_output_wdma.crBase         = out_fbs[idx][0].bufCr;
                p_mapi_ctrl->mapi_output_wdma.endian         = (EndianMode)out_fbs[idx][0].endian;
                p_mapi_ctrl->mapi_output_wdma.isMSB          = fmt_properties[idx].is_msb;
                p_mapi_ctrl->mapi_output_wdma.is3P4B         = fmt_properties[idx].is_3p4b;
                p_mapi_ctrl->mapi_output_wdma.isNV21         = p_test_config->out_nv21[0];
                p_mapi_ctrl->mapi_output_wdma.scaler         = p_test_config->mapi_ctrl.mapi_output_scaler[idx];
            }
        }
    }
    return TRUE;
}

/**
 * SetOutputPath() - Set output common values
 */
static BOOL SetOutputPath(TestMapConvConfig* p_test_config, MapiFormatProperty* fmt_properties)
{
    TestMAPIAPICtrl* p_mapi_ctrl = &(p_test_config->mapi_ctrl);
    FrameMapConvBufferFormat mapi_format;
    Int32 idx = 0;
    MAPIMode out_mode = MAPI_O_MODE_NONE;
    Uint32  width, height;
    FrameSizeInfo fb_size_info = {0,};
    void* data = NULL;

    width  = p_test_config->width;
    height = p_test_config->height;

    for (idx = 0; idx < MAPI_TEST_MAX_OUTPUT_PATH; idx++) {
        out_mode = p_test_config->out_mode[idx];
        if (out_mode > MAPI_O_MODE_NONE) {
            width  = (p_test_config->scl_width[idx] != 0) ? p_test_config->scl_width[idx] : width;
            height = (p_test_config->scl_height[idx] != 0) ? p_test_config->scl_height[idx] : height;
            mapi_format = p_test_config->output_buf_format[idx];
            if (out_mode == MAPI_O_MODE_DIS) {
                data = &(p_mapi_ctrl->mapi_output_display);
                CalFrameSize(p_test_config, mapi_format, width, height, &fb_size_info);
            } else if (out_mode == MAPI_O_MODE_YUV) {
                data = &(p_mapi_ctrl->mapi_output_wdma);
                CalFrameSize(p_test_config, mapi_format, width, height, &fb_size_info);
            }
            p_mapi_ctrl->mapi_output_path[idx].mode     = out_mode;
            p_mapi_ctrl->mapi_output_path[idx].width    = width;
            p_mapi_ctrl->mapi_output_path[idx].height   = height;
            p_mapi_ctrl->mapi_output_path[idx].format   = fmt_properties[idx].mapi_fmt;
            p_mapi_ctrl->mapi_output_path[idx].data     = data;
            p_mapi_ctrl->mapi_output_path[idx].stride   = fb_size_info.luma_stride;
            p_mapi_ctrl->mapi_output_path[idx].chStride = fb_size_info.chroma_stride;

            if (fmt_properties[idx].is_1plane == TRUE && fmt_properties[idx].is_packed == TRUE)
                p_mapi_ctrl->mapi_output_path[idx].planeMode = MAPI_PLANE_MODE_1P_PACKED;
            else if (fmt_properties[idx].is_1plane == TRUE && fmt_properties[idx].is_packed == FALSE)
                p_mapi_ctrl->mapi_output_path[idx].planeMode = MAPI_PLANE_MODE_1P;
            else
                p_mapi_ctrl->mapi_output_path[idx].planeMode = MAPI_PLANE_MODE_DEFAULT;
        }
    }
    return TRUE;
}

/**
 * SetMapConv() - Set values for MAPI API
 */
static BOOL SetMapConv(
    TestMapConvConfig* p_test_config
    )
{
    TestMAPIAPICtrl* p_mapi_ctrl = &(p_test_config->mapi_ctrl);

    if (p_test_config->out_mode[0] == MAPI_O_MODE_NONE && p_test_config->out_mode[1] == MAPI_O_MODE_NONE) {
        VLOG(ERR, "Check output mode, both of output mode are off.");
        return FALSE;
    }

    if (IS_LINEAR_INPUT(p_test_config->input_ctrl_mode)) {
        if (SetLinearInputProperty(p_test_config, p_mapi_ctrl, p_test_config->input_format_property) != TRUE) {
            return FALSE;
        }
    } else if (IS_COMPRESSED_INPUT(p_test_config->input_ctrl_mode)) {
        if (SetCompressedInputProperty(p_test_config, p_mapi_ctrl) != TRUE) {
            return FALSE;
        }
    }

    if (SetInputPath(p_test_config, p_test_config->input_format_property) != TRUE) {
        return FALSE;
    }

    if (SetOutputProperty(p_test_config, p_test_config->output_format_property) != TRUE) {
        return FALSE;
    }

    if (SetOutputPath(p_test_config, p_test_config->output_format_property) != TRUE) {
        return FALSE;
    }

    return TRUE;
}

/**
 * RetMapConv() - Reset values for MAPI API
 */
static BOOL ReSetMapConv(
    TestMapConvConfig* p_test_config
    )
{
    TestMAPIAPICtrl* p_mapi_ctrl = &(p_test_config->mapi_ctrl);

    if (p_test_config->out_mode[0] == MAPI_O_MODE_NONE && p_test_config->out_mode[1] == MAPI_O_MODE_NONE) {
        return FALSE;
    }

    if (IS_LINEAR_INPUT(p_test_config->input_ctrl_mode)) {
        ReSetLinearInputProperty(p_test_config, p_mapi_ctrl, p_test_config->input_format_property);
    } else if (IS_COMPRESSED_INPUT(p_test_config->input_ctrl_mode)) {
        ReSetCompressedInputProperty(p_test_config, p_mapi_ctrl);
    }

    ReSetOutputPath(p_test_config);

    return TRUE;
}

/**
 * RunMapConv()
 */
static BOOL RunMapConv(
    TestMapConvConfig* p_test_config
    )
{
    RetCode ret;
    TestMAPIAPICtrl* p_mapi_ctrl = &(p_test_config->mapi_ctrl);

    ret = VPU_RunMAPI(p_test_config->coreIdx, p_mapi_ctrl->mapi_input_path, p_mapi_ctrl->mapi_output_path);
    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "%s:%d VPU_RunMAPI failure : %d \n",__FUNCTION__, __LINE__, ret);
        return FALSE;
    }
    return TRUE;
}



/**
 * StopMapConv()
 */
static void StopMapConv(
    TestMapConvConfig* p_test_config
    )
{
    if (p_test_config->output_ctrl_mode == MAPI_OUTPUT_DISP || p_test_config->output_ctrl_mode  == MAPI_OUTPUT_DISP_WDMA ) {
        VPU_StopMAPI(p_test_config->coreIdx, MAPI_O_MODE_DIS);
    }
}

/**
 * TestMapConv() - test scenario function (loop , load data -> run -> done -> compare)
 */
static BOOL TestMapConv(
    TestMapConvConfig* p_test_config
    )
{
    Int32   idx = 0;
    BOOL    ret = TRUE;
    RetCode ret_code;

    DispPreRunMsg();

    for (idx = 0; idx < p_test_config->forceOutNum; idx++) {
        if (TRUE != LoadInputData(p_test_config)) {
            return FALSE;
        }
        if(idx > 0) {
            ReSetMapConv(p_test_config);
        }
        ret = RunMapConv(p_test_config);
        if (ret != TRUE) {
            VLOG(ERR, "Failed to RunMapConv \n", ret);
            return FALSE;
        }
        if (p_test_config->input_ctrl_mode != MAPI_INPUT_DISP) {
            ret_code = VPU_DoneMAPI(p_test_config->coreIdx);
            if (ret_code != RETCODE_SUCCESS) {
                VLOG(ERR, "Failed to VPU_DoneMAPI : %d \n", ret_code);
                return FALSE;
            }
        }
        if (p_test_config->input_ctrl_mode == MAPI_INPUT_DISP) {
            ret_code = VPU_DoneMAPI(p_test_config->coreIdx);
            if (ret_code != RETCODE_SUCCESS) {
                VLOG(ERR, "Failed to VPU_DoneMAPI : %d \n", ret_code);
                return FALSE;
            }
        }
        DispPostRunMsg(p_test_config);
        if ( p_test_config->compareType == RGB_COMPARE || p_test_config->compareType == YUV_COMPARE ) {
            if (p_test_config->output_fb_num <= MAPI_TEST_MAX_NUM_OUTPUT_BUF) {
                if (ret == TRUE) {
                    ret = CompareFrames(p_test_config, p_test_config->frame_idx);
                } else {
                    ret = FALSE;
                }
            }
        } else {
            VLOG(WARN, "No Comparison \n");
        }
        if (TRUE != ret) {
            break;
        }
        p_test_config->frame_idx++;
    }



    return ret;
}

int main(int argc, char **argv)
{
    BOOL                ret = TRUE;
    TestMapConvConfig   testMapConvCfg;
    CommandLineArgument argument;


    InitLog();

    argument.argc = argc;
    argument.argv = argv;
    //default setting.
    SetDefaultTestConfig(&testMapConvCfg);
    if (ParseArgumentAndSetTestConfig(argument, &testMapConvCfg) == FALSE) {
        VLOG(ERR, "fail to ParseArgumentAndSetTestConfig()\n");
        return 1;
    }
    SetTestConfig(&testMapConvCfg);


    vdi_init(testMapConvCfg.coreIdx);

    osal_init_keyboard();

    if (FALSE == PrepareMapConvTest(&testMapConvCfg)) {
        VLOG(ERR, "[MAP_CONV] %s:%d \n", __FUNCTION__, __LINE__);
    }

    SetMapConv(&testMapConvCfg);

    ShowLogMsg();

    ret = TestMapConv(&testMapConvCfg);

    StopMapConv(&testMapConvCfg);

    DestroyMapConv(&testMapConvCfg);

    osal_close_keyboard();

    vdi_release(testMapConvCfg.coreIdx);

    if (ret == TRUE) {
        VLOG(INFO, "\nFinished MAPI TEST \n");
    }
    DeInitLog();
    return ret == TRUE ? 0 : 1;
}


