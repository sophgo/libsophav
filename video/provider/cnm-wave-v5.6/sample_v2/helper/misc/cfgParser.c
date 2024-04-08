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
#include <errno.h>
#include <string.h>
#include "vpuapi.h"
#include "vpuapifunc.h"
#include "main_helper.h"

const WaveCfgInfo waveCfgInfo[] = {
    //name                          min            max              default
    {"InputFile",                   0,              0,                      0},
    {"SourceWidth",                 0,              W_MAX_ENC_PIC_WIDTH,    0},
    {"SourceHeight",                0,              W_MAX_ENC_PIC_HEIGHT,   0},
    {"InputBitDepth",               8,              10,                     8},
    {"FrameRate",                   0,              480,                    0},
    {"FramesToBeEncoded",           0,              INT_MAX,                0},
    {"IntraPeriod",                 0,              2047,                   0},
    {"DecodingRefreshType",         0,              2,                      1},
    {"GOPSize",                     1,              MAX_GOP_NUM,            1},
    {"IntraNxN",                    0,              1,                      1},
    {"IntraTransSkip",              0,              1,                      1},
    {"ConstrainedIntraPred",        0,              1,                      0},
    {"IntraCtuRefreshMode",         0,              4,                      0},
    {"IntraCtuRefreshArg",          0,              UI16_MAX,               0},
    {"MaxNumMerge",                 0,              2,                      2},
    {"EnTemporalMVP",               0,              1,                      1},
    {"ScalingList",                 0,              2,                      0},
    {"IndeSliceMode",               0,              1,                      0},
    {"IndeSliceArg",                0,              UI16_MAX,               0},
    {"DeSliceMode",                 0,              2,                      0},
    {"DeSliceArg",                  0,              UI16_MAX,               0},
    {"EnDBK",                       0,              1,                      1},
    {"EnSAO",                       0,              1,                      1},
    {"LFCrossSliceBoundaryFlag",    0,              1,                      1},
    {"BetaOffsetDiv2",             -6,              6,                      0},
    {"TcOffsetDiv2",               -6,              6,                      0},
    {"WaveFrontSynchro",            0,              1,                      0},
    {"LosslessCoding",              0,              1,                      0},
    {"UsePresetEncTools",           0,              3,                      0},
    {"GopPreset",                   0,              16,                     0},
    {"RateControl",                 0,              1,                      0},
    {"EncBitrate",                  0,              700000000,              0},
    {"InitialDelay",                10,             3000,                   0},
    {"EnHvsQp",                     0,              1,                      1},
    {"CULevelRateControl",          0,              1,                      1},
    {"ConfWindSizeTop",             0,              W_MAX_ENC_PIC_HEIGHT,   0},
    {"ConfWindSizeBot",             0,              W_MAX_ENC_PIC_HEIGHT,   0},
    {"ConfWindSizeRight",           0,              W_MAX_ENC_PIC_WIDTH,    0},
    {"ConfWindSizeLeft",            0,              W_MAX_ENC_PIC_WIDTH,    0},
    {"HvsQpScaleDiv2",              0,              4,                      2},
    {"MinQp",                       0,              63,                     8},
    {"MaxQp",                       0,              63,                    51},
    {"MaxDeltaQp",                  0,              12,                    10},
    {"QP",                          0,              63,                    30},
    {"BitAllocMode",                0,              2,                      0},
#define FIXED_BIT_RATIO 45
    {"FixedBitRatio%d",             1,              255,                    1},
    {"InternalBitDepth",            0,              10,                     0},
    {"EnRoi",                       0,              1,                      0},
    {"NumUnitsInTick",              0,              INT_MAX,                0},
    {"TimeScale",                   0,              INT_MAX,                0},
    {"NumTicksPocDiffOne",          0,              INT_MAX,                0},
    {"EncAUD",                      0,              1,                      0},
    {"EncEOS",                      0,              1,                      0},
    {"EncEOB",                      0,              1,                      0},
    {"CbQpOffset",                  -12,            12,                     0},
    {"CrQpOffset",                  -12,            12,                     0},
    {"RcInitialQp",                 -1,             63,                    63},
    {"EnNoiseReductionY",           0,              1,                      0},
    {"EnNoiseReductionCb",          0,              1,                      0},
    {"EnNoiseReductionCr",          0,              1,                      0},
    {"EnNoiseEst",                  0,              1,                      1},
    {"NoiseSigmaY",                 0,              255,                    0},
    {"NoiseSigmaCb",                0,              255,                    0},
    {"NoiseSigmaCr",                0,              255,                    0},
    {"IntraNoiseWeightY",           0,              31,                     7},
    {"IntraNoiseWeightCb",          0,              31,                     7},
    {"IntraNoiseWeightCr",          0,              31,                     7},
    {"InterNoiseWeightY",           0,              31,                     4},
    {"InterNoiseWeightCb",          0,              31,                     4},
    {"InterNoiseWeightCr",          0,              31,                     4},
    {"UseAsLongTermRefPeriod",      0,              INT_MAX,                0},
    {"RefLongTermPeriod",           0,              INT_MAX,                0},
    {"BitstreamFile",               0,              0,                      0},
// newly added for WAVE ENCODER
    {"EncMonochrome",               0,              1,                      0},
    {"StrongIntraSmoothing",        0,              1,                      1},
    {"RoiAvgQP",                    0,              63,                     0},
    {"WeightedPred",                0,              3,                      0},
    {"EnBgDetect",                  0,              1,                      0},
    {"BgThDiff",                    0,              255,                    8},
    {"BgThMeanDiff",                0,              255,                    1},
    {"BgLambdaQp",                  0,              63,                    32},
    {"BgDeltaQp",                   -16,            15,                     3},
    {"EnLambdaMap",                 0,              1,                      0},
    {"EnCustomLambda",              0,              1,                      0},
    {"EnCustomMD",                  0,              1,                      0},
    {"PU04DeltaRate",               0,              255,                    0},
    {"PU08DeltaRate",               0,              255,                    0},
    {"PU16DeltaRate",               0,              255,                    0},
    {"PU32DeltaRate",               0,              255,                    0},
    {"PU04IntraPlanarDeltaRate",    0,              255,                    0},
    {"PU04IntraDcDeltaRate",        0,              255,                    0},
    {"PU04IntraAngleDeltaRate",     0,              255,                    0},
    {"PU08IntraPlanarDeltaRate",    0,              255,                    0},
    {"PU08IntraDcDeltaRate",        0,              255,                    0},
    {"PU08IntraAngleDeltaRate",     0,              255,                    0},
    {"PU16IntraPlanarDeltaRate",    0,              255,                    0},
    {"PU16IntraDcDeltaRate",        0,              255,                    0},
    {"PU16IntraAngleDeltaRate",     0,              255,                    0},
    {"PU32IntraPlanarDeltaRate",    0,              255,                    0},
    {"PU32IntraDcDeltaRate",        0,              255,                    0},
    {"PU32IntraAngleDeltaRate",     0,              255,                    0},
    {"CU08IntraDeltaRate",          0,              255,                    0},
    {"CU08InterDeltaRate",          0,              255,                    0},
    {"CU08MergeDeltaRate",          0,              255,                    0},
    {"CU16IntraDeltaRate",          0,              255,                    0},
    {"CU16InterDeltaRate",          0,              255,                    0},
    {"CU16MergeDeltaRate",          0,              255,                    0},
    {"CU32IntraDeltaRate",          0,              255,                    0},
    {"CU32InterDeltaRate",          0,              255,                    0},
    {"CU32MergeDeltaRate",          0,              255,                    0},
    {"DisableCoefClear",            0,              1,                      0},
    {"EnModeMap",                   0,              3,                      0},
    {"ForcePicSkipStart",          -1,              INT_MAX,               -1},
    {"ForcePicSkipEnd",            -1,              INT_MAX,               -1},
    {"ForceCoefDropStart",         -1,              INT_MAX,               -1},
    {"ForceCoefDropEnd",           -1,              INT_MAX,               -1},
    {"StillPictureProfile",         0,              1,                      0},
    {"VbvBufferSize",              10,              3000,                3000},
    // newly added for H.264 on WAVE5
    {"IdrPeriod",                   0,              2047,                   0},
    {"RdoSkip",                     0,              1,                      1},
    {"LambdaScaling",               0,              1,                      1},
    {"Transform8x8",                0,              1,                      1},
    {"SliceMode",                   0,              1,                      0},
    {"SliceArg",                    0,              INT_MAX,                0},
    {"IntraMbRefreshMode",          0,              3,                      0},
    {"IntraMbRefreshArg",           1,              INT_MAX,                1},
    {"MBLevelRateControl",          0,              1,                      0},
    {"CABAC",                       0,              1,                      1},
    {"RoiQpMapFile",                0,              1,                      1},
    {"S2fmeOff",                    0,              1,                      0},
    {"RcWeightParaCtrl",            1,              31,                    16},
    {"RcWeightBufCtrl",             1,              255,                  128},
    // HLS
    {"EnPrefixSeiData",             0,              1,                      0},
    {"PrefixSeiDataSize",           0,              1024,                   1},
    {"EnSuffixSeiData",             0,              1,                      0},
    {"SuffixSeiDataSize",           0,              1024,                   1},
    {"EncodeRbspVui",               0,              1,                      0},
    {"RbspVuiSize",                 0,              16383,                  1},
    {"EncodeRbspHrdInVps",          0,              1,                      0},
    {"RbspHrdSize",                 0,              16383,                  1},
    {"EnForcedIDRHeader",           0,              2,                      0},
    {"ForceIdrPicIdx",             -1,              INT_MAX,               -1},
    {"Profile",                     0,              H264_PROFILE_HIGH444,   0},
};

static CFGINFO w6_cfg_info[CFGP_IDX_MAX] = {
/*  | codec type                          | type              | name (string)                | min           | max                   | default	  | lvalue   | exception (N-N=Range, N=Number, Delimiter=',')*/
// CFG & Input
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_STR,      "InputFile",                   0,              0,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "SourceWidth",                 256,            W_MAX_ENC_PIC_WIDTH,    0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "SourceHeight",                128,            W_MAX_ENC_PIC_HEIGHT,   0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "InputBitDepth",               8,              10,                     8,           NULL,      "9"},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "FrameRate",                   1,              480,                    30,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "FramesToBeEncoded",          -1,              UI16_MAX,               -1,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_STR,      "BitstreamFile",               0,              0,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_STR,      "ReconFile",                   0,              0,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "InternalBitDepth",            8,              10,                     8,           NULL,      "9"},
// GOP
    {(CFG_HEVC),                           CFG_DTYPE_INT,      "DecodingRefreshType",         0,              2,                      1,           NULL,      NULL}, // HEVC
    {(CFG_AVC),                            CFG_DTYPE_INT,      "IdrPeriod",                   0,              UI16_MAX,               0,           NULL,      NULL}, // AVC
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "IntraPeriod",                 0,              UI16_MAX,               0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "GopPreset",                   0,              16,                     5,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "GOPSize",                     1,              MAX_GOP_NUM,            1,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_COUNT,    "Frame%d",                     1,              MAX_GOP_FRAME_NUM,      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "UseAsLongTermRefPeriod",      0,              UI16_MAX,               0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "RefLongTermPeriod",           0,              UI16_MAX,               0,           NULL,      NULL},
// Rate Control
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "RateControl",                 0,              2,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "EncBitrate",                  0,              1500000000,             0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "VbvBufferSize",               10,             100000,                 10000,       NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "CULevelRateControl",          0,              1,                      1,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "EnHvsQp",                     0,              1,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "HvsQpScaleDiv2",              1,              4,                      2,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "HvsMaxDeltaQp",               0,              12,                     10,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "RcInitialQp",                 -1,             51,                     -1,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "RcUpdateSpeed",               0,              255,                    16,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "PicRcMaxDqp",                 0,              51,                     3,           NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "MinQp",                       0,              51,                     8,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "MinQp",                       0,              51,                     8,           NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "MaxQp",                       0,              51,                     51,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "MaxQp",                       0,              51,                     51,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "MaxBitrate",                  0,              1500000000,             0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "RcInitLevel",                 0,              15,                     8,           NULL,      NULL},
// Background Encoding
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "EnBgDetect",                  0,              1,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "BgThDiff",                    0,              255,                    8,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "BgThMeanDiff",                0,              255,                    1,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "BgDeltaQp",                   -16,            15,                     5,           NULL,      NULL},
// Intra
    {(CFG_HEVC),                           CFG_DTYPE_INT,      "StrongIntraSmoothing",        0,              1,                      1,           NULL,      NULL}, // HEVC
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "ConstrainedIntraPred",        0,              1,                      0,           NULL,      NULL}, // HEVC, AVC
    {(CFG_HEVC),                           CFG_DTYPE_INT,      "IntraNxN",                    -1,             3,                      3,           NULL,      "1"},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "IntraNxN",                    -1,             3,                      2,           NULL,      "1"},
    {(CFG_HEVC|CFG_AV1),                   CFG_DTYPE_INT,      "IntraTransSkip",              0,              1,                      1,           NULL,      NULL}, // HEVC, AV1
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "IntraRefreshMode",            0,              2,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "IntraRefreshArg",             1,              511,                    1,           NULL,      NULL},
// Inter
    {(CFG_HEVC),                           CFG_DTYPE_INT,      "EnTemporalMVP",               0,              1,                      1,           NULL,      NULL}, // HEVC
    {(CFG_HEVC|CFG_AV1|CFG_AVC),           CFG_DTYPE_INT,      "MeCenter",                    -1,             1,                      -1,          NULL,      NULL},
// Extra Tools
    {(CFG_AVC),                            CFG_DTYPE_INT,      "CABAC",                       0,              1,                      1,           NULL,      NULL}, // AVC
    {(CFG_AVC),                            CFG_DTYPE_INT,      "Transform8x8",                0,              1,                      1,           NULL,      NULL}, // AVC
// Loop Filter
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "LFCrossSliceBoundaryFlag",    0,              1,                      1,           NULL,      NULL}, // HEVC, AVC
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "EnDBK",                       0,              1,                      1,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "BetaOffsetDiv2",              -6,             6,                      0,           NULL,      NULL}, // HEVC, AVC
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "TcOffsetDiv2",                -6,             6,                      0,           NULL,      NULL}, // HEVC, AVC
    {(CFG_AV1),                            CFG_DTYPE_INT,      "LfSharpness",                 0,              7,                      0,           NULL,      NULL}, // AV1
    {(CFG_HEVC),                           CFG_DTYPE_INT,      "EnSAO",                       0,              1,                      1,           NULL,      NULL}, // HEVC
    {(CFG_AV1),                            CFG_DTYPE_INT,      "EnCdef",                      0,              1,                      1,           NULL,      NULL}, // AV1
    {(CFG_AV1),                            CFG_DTYPE_INT,      "EnWiener",                    0,              1,                      1,           NULL,      NULL}, // AV1
// Quantization
	{(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "QP",                          0,              51,                     30,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "CbQpOffset",                  -12,            12,                     0,           NULL,      NULL}, // HEVC, AVC
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "CrQpOffset",                  -12,            12,                     0,           NULL,      NULL}, // HEVC, AVC
    {(CFG_AV1),                            CFG_DTYPE_INT,      "YDcQpDelta",                  -64,            63,                     0,           NULL,      NULL}, // AV1
    {(CFG_AV1),                            CFG_DTYPE_INT,      "UDcQpDelta",                  -64,            63,                     0,           NULL,      NULL}, // AV1
    {(CFG_AV1),                            CFG_DTYPE_INT,      "VDcQpDelta",                  -64,            63,                     0,           NULL,      NULL}, // AV1
    {(CFG_AV1),                            CFG_DTYPE_INT,      "UAcQpDelta",                  -64,            63,                     0,           NULL,      NULL}, // AV1
    {(CFG_AV1),                            CFG_DTYPE_INT,      "VAcQpDelta",                  -64,            63,                     0,           NULL,      NULL}, // AV1
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "ScalingList",                 0,              1,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "AdaptiveRound",               0,              1,                      1,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "QRoundIntra",                 0,              255,                    171,         NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "QRoundInter",                 0,              255,                    85,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "DisableCoefClear",            0,              1,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "LambdaDqpIntra",              -32,            31,                     0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "LambdaDqpInter",              -32,            31,                     0,           NULL,      NULL},
// Slice
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "SliceMode",                   0,              2,                      0,           NULL,      NULL}, // HEVC, AVC
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "SliceArg",                    0,              262143,                 1,           NULL,      NULL}, // HEVC, AVC
    {(CFG_AV1),                            CFG_DTYPE_INT,      "NumColTiles",                 1,              2,                      1,           NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "NumRowTiles",                 1,              16,                     1,           NULL,      NULL},
// Custom
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "EnQpMap",                     0,              1,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "EnModeMap",                   0,              1,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_STR,      "CustomMapFile",               0,              0,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "EnCustomLambda",              0,              1,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_STR,      "CustomLambdaFile",            0,              0,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "ForcePicSkipStart",           -1,             UI16_MAX,               -1,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "ForcePicSkipEnd",             -1,             UI16_MAX,               -1,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "ForceIdrPicIdx",              -1,             UI16_MAX,               0,           NULL,      NULL},
    //{(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "RdoBiasBlk4",                 0,              255,                    0,           NULL,      NULL},
    //{(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "RdoBiasBlk8",                 0,              255,                    0,           NULL,      NULL},
    //{(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "RdoBiasBlk16",                0,              255,                    0,           NULL,      NULL},
    //{(CFG_HEVC|CFG_AV1),                   CFG_DTYPE_INT,      "RdoBiasBlk32",                0,              255,                    0,           NULL,      NULL}, // HEVC, AV1
    //{(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "RdoBiasIntra",                0,              255,                    0,           NULL,      NULL},
    //{(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "RdoBiasInter",                0,              255,                    0,           NULL,      NULL},
    //{(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "RdoBiasMerge",                0,              255,                    0,           NULL,      NULL},
// Misc
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "NumUnitsInTick",              0,              UI16_MAX,               0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "TimeScale",                   0,              UI16_MAX,               0,           NULL,      NULL},
// CONF_WIND_SIZE
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "ConfWindSizeTop",             0,              8192,                   0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "ConfWindSizeBot",             0,              8192,                   0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "ConfWindSizeRight",           0,              8192,                   0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "ConfWindSizeLeft",            0,              8192,                   0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "NoLastFrameCheck",            0,              1,                      0,           NULL,      NULL},
//SEI
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "EnPrefixSeiData",             0,              1,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_STR,      "PrefixSeiDataFile",           0,              0,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "PrefixSeiSize",               0,              UI16_MAX,               0,           NULL,      NULL},
    {(CFG_HEVC),                           CFG_DTYPE_INT,      "EnSuffixSeiData",             0,              1,                      0,           NULL,      NULL},
    {(CFG_HEVC),                           CFG_DTYPE_STR,      "SuffixSeiDataFile",           0,              0,                      0,           NULL,      NULL},
    {(CFG_HEVC),                           CFG_DTYPE_INT,      "SuffixSeiSize",               0,              UI16_MAX,               0,           NULL,      NULL},
//HRD_RBSP
    {(CFG_HEVC),                           CFG_DTYPE_INT,      "EncodeRbspHrd",               0,              1,                      0,           NULL,      NULL},
    {(CFG_HEVC),                           CFG_DTYPE_STR,      "RbspHrdFile",                 0,              0,                      0,           NULL,      NULL},
    {(CFG_HEVC),                           CFG_DTYPE_INT,      "RbspHrdSize",                 0,              UI16_MAX,               0,           NULL,      NULL},
//VUI_RBSP
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "EncodeRbspVui",               0,              1,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_STR,      "RbspVuiFile",                 0,              0,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "RbspVuiSize",                 0,              UI16_MAX,               0,           NULL,      NULL},
//AUD_EOS_EOB
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "EncAUD",                      0,              1,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "EncEOS",                      0,              1,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "EncEOB",                      0,              1,                      0,           NULL,      NULL},
//New parameters in 21.10.22
    {(CFG_HEVC),                           CFG_DTYPE_INT,      "NumTicksPocDiffOne",          1,              UI16_MAX,               1,           NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "MinQpI",                      0,              51,                     8,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "MinQpI",                      0,              51,                     8,           NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "MaxQpI",                      0,              51,                     51,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "MaxQpI",                      0,              51,                     51,          NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "MinQpP",                      0,              51,                     8,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "MinQpP",                      0,              51,                     8,           NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "MaxQpP",                      0,              51,                     51,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "MaxQpP",                      0,              51,                     51,          NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "MinQpB",                      0,              51,                     8,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "MinQpB",                      0,              51,                     8,           NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "MaxQpB",                      0,              51,                     51,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "MaxQpB",                      0,              51,                     51,          NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "Tid0_QpI",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "Tid0_QpI",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "Tid0_QpP",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "Tid0_QpP",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "Tid0_QpB",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "Tid0_QpB",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "Tid1_QpI",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "Tid1_QpI",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "Tid1_QpP",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "Tid1_QpP",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "Tid1_QpB",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "Tid1_QpB",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "Tid2_QpI",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "Tid2_QpI",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "Tid2_QpP",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "Tid2_QpP",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "Tid2_QpB",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "Tid2_QpB",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "Tid3_QpI",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "Tid3_QpI",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "Tid3_QpP",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "Tid3_QpP",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "Tid3_QpB",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "Tid3_QpB",                    0,              51,                     30,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "ForcePicQpPicIdx",            -1,             UI16_MAX,               -1,          NULL,      NULL},
    {(CFG_AV1),                            CFG_DTYPE_INT,      "ForcePicQpValue",             0,              51,                     1,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC),                   CFG_DTYPE_INT,      "ForcePicQpValue",             0,              51,                     0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "ForcePicTypePicIdx",          -1,             UI16_MAX,               -1,          NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "ForcePicTypeValue",           0,              3,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "TemporalLayerCount",          1,              4,                      1,           NULL,      NULL},
//Change Parameters
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_INT,      "ApiScenario",                 0,              1,                      0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_COUNT,    "SPCh%d",                      1,              120,                    0,           NULL,      NULL},
    {(CFG_AVC|CFG_HEVC|CFG_AV1),           CFG_DTYPE_COUNT,    "PPCh%d",                      0,              120,                    0,           NULL,      NULL},
};


//------------------------------------------------------------------------------
// ENCODE PARAMETER PARSE FUNCSIONS
//------------------------------------------------------------------------------
// Parameter parsing helper
static int GetValue(osal_file_t fp, char *para, char *value)
{
    char lineStr[256];
    char paraStr[256];
    osal_fseek(fp, 0, SEEK_SET);

    while (1) {
        if (fgets(lineStr, 256, fp) == NULL)
            return 0;
        sscanf(lineStr, "%s %s", paraStr, value);
        if (paraStr[0] != ';') {
            if (strcmp(para, paraStr) == 0)
                return 1;
        }
    }
}

// Parse "string number number ..." at most "num" numbers
// e.g. SKIP_PIC_NUM 1 3 4 5
static int GetValues(osal_file_t fp, char *para, int *values, int num)
{
    char line[1024];

    osal_fseek(fp, 0, SEEK_SET);

    while (1)
    {
        int  i;
        char *str;

        if (fgets(line, sizeof(line)-1, fp) == NULL)
            return 0;

        // empty line
        if ((str = strtok(line, " ")) == NULL)
            continue;

        if(strcmp(str, para) != 0)
            continue;

        for (i=0; i<num; i++)
        {
            if ((str = strtok(NULL, " ")) == NULL)
                return 1;
            if (!isdigit((Int32)str[0]))
                return 1;
            values[i] = atoi(str);
        }
        return 1;
    }
}

int parseMp4CfgFile(ENC_CFG *pEncCfg, char *FileName)
{
    osal_file_t fp;
    char sValue[1024];
    int  ret = 0;

    fp = osal_fopen(FileName, "rt");
    if (fp == NULL) {
        return ret;
    }

    if (GetValue(fp, "YUV_SRC_IMG", sValue) == 0)
        goto __end_parseMp4CfgFile;
    else
        strcpy(pEncCfg->SrcFileName, sValue);

    if (GetValue(fp, "FRAME_NUMBER_ENCODED", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->NumFrame = atoi(sValue);
    if (GetValue(fp, "PICTURE_WIDTH", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->PicX = atoi(sValue);
    if (GetValue(fp, "PICTURE_HEIGHT", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->PicY = atoi(sValue);
    if (GetValue(fp, "FRAME_RATE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    {
        double frameRate;
        int  timeRes, timeInc;
#ifdef ANDROID
        frameRate = atoi(sValue);
#else
        frameRate = atof(sValue);
#endif
        timeInc = 1;
        while ((int)frameRate != frameRate) {
            timeInc *= 10;
            frameRate *= 10;
        }
        timeRes = (int) frameRate;
        // divide 2 or 5
        if (timeInc%2 == 0 && timeRes%2 == 0) {
            timeInc /= 2;
            timeRes /= 2;
        }
        if (timeInc%5 == 0 && timeRes%5 == 0) {
            timeInc /= 5;
            timeRes /= 5;
        }
        if (timeRes == 2997 && timeInc == 100) {
            timeRes = 30000;
            timeInc = 1001;
        }
        pEncCfg->FrameRate = (timeInc - 1) << 16;
        pEncCfg->FrameRate |= timeRes;
    }
    if (GetValue(fp, "VERSION_ID", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->VerId = atoi(sValue);
    if (GetValue(fp, "DATA_PART_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->DataPartEn = atoi(sValue);
    if (GetValue(fp, "REV_VLC_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->RevVlcEn = atoi(sValue);

    if (GetValue(fp, "INTRA_DC_VLC_THRES", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->IntraDcVlcThr = atoi(sValue);
    if (GetValue(fp, "SHORT_VIDEO", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->ShortVideoHeader = atoi(sValue);
    if (GetValue(fp, "ANNEX_I_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->AnnexI = atoi(sValue);
    if (GetValue(fp, "ANNEX_J_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->AnnexJ = atoi(sValue);
    if (GetValue(fp, "ANNEX_K_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->AnnexK = atoi(sValue);
    if (GetValue(fp, "ANNEX_T_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->AnnexT = atoi(sValue);

    if (GetValue(fp, "VOP_QUANT_SCALE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->VopQuant = atoi(sValue);
    if (GetValue(fp, "GOP_PIC_NUMBER", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->GopPicNum = atoi(sValue);
    if (GetValue(fp, "SLICE_MODE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->SliceMode = atoi(sValue);
    if (GetValue(fp, "SLICE_SIZE_MODE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->SliceSizeMode = atoi(sValue);
    if (GetValue(fp, "SLICE_SIZE_NUMBER", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->SliceSizeNum = atoi(sValue);

    if (GetValue(fp, "RATE_CONTROL_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->RcEnable = atoi(sValue);
    if (GetValue(fp, "BIT_RATE_KBPS", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->RcBitRate = atoi(sValue);
    if (GetValue(fp, "DELAY_IN_MS", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->RcInitDelay = atoi(sValue);
    if (GetValue(fp, "VBV_BUFFER_SIZE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->RcBufSize = atoi(sValue);
    if (GetValue(fp, "INTRA_MB_REFRESH", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->IntraRefreshNum = atoi(sValue);

    pEncCfg->ConscIntraRefreshEnable = 0;
    pEncCfg->CountIntraMbEnable = 0;
    pEncCfg->FieldSeqIntraRefreshEnable = 0;
    if (pEncCfg->IntraRefreshNum > 0)
    {
        if (GetValue(fp, "CONSC_INTRA_REFRESH_EN", sValue) == 0)
            pEncCfg->ConscIntraRefreshEnable = 0;
        else
            pEncCfg->ConscIntraRefreshEnable = atoi(sValue);
        if (GetValue(fp, "COUNT_INTRA_MB_EN", sValue) == 0)
            pEncCfg->CountIntraMbEnable = 0;
        else
            pEncCfg->CountIntraMbEnable = atoi(sValue);
    }
    if (GetValue(fp, "CONST_INTRA_QP_EN", sValue) == 0)
        pEncCfg->ConstantIntraQPEnable = 0;
    else
        pEncCfg->ConstantIntraQPEnable = atoi(sValue);
    if (GetValue(fp, "CONST_INTRA_QP", sValue) == 0)
        pEncCfg->RCIntraQP = 0;
    else
        pEncCfg->RCIntraQP = atoi(sValue);

    if (GetValue(fp, "HEC_ENABLE", sValue) == 0)
        pEncCfg->HecEnable = 0;
    else
        pEncCfg->HecEnable = atoi(sValue);

    if (GetValue(fp, "SEARCH_RANGE_X", sValue) == 0)
        pEncCfg->SearchRangeX = 0;
    else
        pEncCfg->SearchRangeX = atoi(sValue);

    if (GetValue(fp, "SEARCH_RANGE_Y", sValue) == 0)
        pEncCfg->SearchRangeY = 0;
    else
        pEncCfg->SearchRangeY = atoi(sValue);

    pEncCfg->RcGopIQpOffsetEn = 0;
    pEncCfg->RcGopIQpOffset = 0;
    if (GetValue(fp, "ME_USE_ZERO_PMV", sValue) == 0)
        pEncCfg->MeUseZeroPmv = 0;
    else
        pEncCfg->MeUseZeroPmv = atoi(sValue);
    if (GetValue(fp, "WEIGHT_INTRA_COST", sValue) == 0)
        pEncCfg->intraCostWeight = 0;
    else
        pEncCfg->intraCostWeight = atoi(sValue);

    if (GetValue(fp, "MAX_QP_SET_ENABLE", sValue) == 0)
        pEncCfg->MaxQpSetEnable= 0;
    else
        pEncCfg->MaxQpSetEnable = atoi(sValue);
    if (GetValue(fp, "MAX_QP", sValue) == 0)
        pEncCfg->MaxQp = 0;
    else
        pEncCfg->MaxQp = atoi(sValue);
    if (GetValue(fp, "GAMMA_SET_ENABLE", sValue) == 0)
        pEncCfg->GammaSetEnable = 0;
    else
        pEncCfg->GammaSetEnable = atoi(sValue);
    if (GetValue(fp, "GAMMA", sValue) == 0)
        pEncCfg->Gamma = 0;
    else
        pEncCfg->Gamma = atoi(sValue);

    if (GetValue(fp, "RC_INTERVAL_MODE", sValue) == 0)
        pEncCfg->rcIntervalMode = 0;
    else
        pEncCfg->rcIntervalMode = atoi(sValue);

    if (GetValue(fp, "RC_MB_INTERVAL", sValue) == 0)
        pEncCfg->RcMBInterval = 0;
    else
        pEncCfg->RcMBInterval = atoi(sValue);

    ret = 1; /* Success */

__end_parseMp4CfgFile:
    osal_fclose(fp);
    return ret;
}


int parseAvcCfgFile(ENC_CFG *pEncCfg, char *FileName)
{
    osal_file_t fp;
    char sValue[1024];
    int  ret = 0;

    fp = osal_fopen(FileName, "r");
    if (fp == NULL) {
        return 0;
    }

    if (GetValue(fp, "YUV_SRC_IMG", sValue) == 0)
        goto __end_parseAvcCfgFile;
    else
        strcpy(pEncCfg->SrcFileName, sValue);

    if (GetValue(fp, "FRAME_NUMBER_ENCODED", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->NumFrame = atoi(sValue);
    if (GetValue(fp, "PICTURE_WIDTH", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->PicX = atoi(sValue);
    if (GetValue(fp, "PICTURE_HEIGHT", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->PicY = atoi(sValue);
    if (GetValue(fp, "FRAME_RATE", sValue) == 0)
        goto __end_parseAvcCfgFile;
    {
        double frameRate;
        int  timeRes, timeInc;

#ifdef ANDROID
        frameRate = atoi(sValue);
#else
        frameRate = atof(sValue);
#endif

        timeInc = 1;
        while ((int)frameRate != frameRate) {
            timeInc *= 10;
            frameRate *= 10;
        }
        timeRes = (int) frameRate;
        // divide 2 or 5
        if (timeInc%2 == 0 && timeRes%2 == 0) {
            timeInc /= 2;
            timeRes /= 2;
        }
        if (timeInc%5 == 0 && timeRes%5 == 0) {
            timeInc /= 5;
            timeRes /= 5;
        }

        if (timeRes == 2997 && timeInc == 100) {
            timeRes = 30000;
            timeInc = 1001;
        }
        pEncCfg->FrameRate = (timeInc - 1) << 16;
        pEncCfg->FrameRate |= timeRes;
    }
    if (GetValue(fp, "CONSTRAINED_INTRA", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->ConstIntraPredFlag = atoi(sValue);
    if (GetValue(fp, "DISABLE_DEBLK", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->DisableDeblk = atoi(sValue);
    if (GetValue(fp, "DEBLK_ALPHA", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->DeblkOffsetA = atoi(sValue);
    if (GetValue(fp, "DEBLK_BETA", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->DeblkOffsetB = atoi(sValue);
    if (GetValue(fp, "CHROMA_QP_OFFSET", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->ChromaQpOffset = atoi(sValue);

    if (GetValue(fp, "LEVEL", sValue) == 0)
    {
        pEncCfg->level = 0;//note : 0 means auto calculation.
    }
    else
    {
        pEncCfg->level = atoi(sValue);
        if (pEncCfg->level<0 || pEncCfg->level>51)
            goto __end_parseAvcCfgFile;
    }

    if (GetValue(fp, "PIC_QP_Y", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->PicQpY = atoi(sValue);
    if (GetValue(fp, "GOP_PIC_NUMBER", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->GopPicNum = atoi(sValue);
    if (GetValue(fp, "IDR_INTERVAL", sValue) == 0)
        pEncCfg->IDRInterval = 0;
    else
        pEncCfg->IDRInterval = atoi(sValue);
    if (GetValue(fp, "SLICE_MODE", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->SliceMode = atoi(sValue);
    if (GetValue(fp, "SLICE_SIZE_MODE", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->SliceSizeMode = atoi(sValue);
    if (GetValue(fp, "SLICE_SIZE_NUMBER", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->SliceSizeNum = atoi(sValue);
    if (GetValue(fp, "AUD_ENABLE", sValue) == 0)
        pEncCfg->audEnable = 0;
    else
        pEncCfg->audEnable = atoi(sValue);

    /**
    * Error Resilience
    */
    // Intra Cost Weight : not mandatory. default zero
    if (GetValue(fp, "WEIGHT_INTRA_COST", sValue) == 0)
        pEncCfg->intraCostWeight = 0;
    else
        pEncCfg->intraCostWeight = atoi(sValue);

    /**
    * CROP information
    */
    if (GetValue(fp, "FRAME_CROP_LEFT", sValue) == 0)
        pEncCfg->frameCropLeft = 0;
    else
        pEncCfg->frameCropLeft = atoi(sValue);

    if (GetValue(fp, "FRAME_CROP_RIGHT", sValue) == 0)
        pEncCfg->frameCropRight = 0;
    else
        pEncCfg->frameCropRight = atoi(sValue);

    if (GetValue(fp, "FRAME_CROP_TOP", sValue) == 0)
        pEncCfg->frameCropTop = 0;
    else
        pEncCfg->frameCropTop = atoi(sValue);

    if (GetValue(fp, "FRAME_CROP_BOTTOM", sValue) == 0)
        pEncCfg->frameCropBottom = 0;
    else
        pEncCfg->frameCropBottom = atoi(sValue);

    /**
    * ME Option
    */

    if (GetValue(fp, "ME_USE_ZERO_PMV", sValue) == 0)
        pEncCfg->MeUseZeroPmv = 0;
    else
        pEncCfg->MeUseZeroPmv = atoi(sValue);

    if (GetValue(fp, "ME_BLK_MODE_ENABLE", sValue) == 0)
        pEncCfg->MeBlkModeEnable = 0;
    else
        pEncCfg->MeBlkModeEnable = atoi(sValue);

    if (GetValue(fp, "RATE_CONTROL_ENABLE", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->RcEnable = atoi(sValue);
    if (GetValue(fp, "BIT_RATE_KBPS", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->RcBitRate = atoi(sValue);
    if (GetValue(fp, "DELAY_IN_MS", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->RcInitDelay = atoi(sValue);

    if (GetValue(fp, "VBV_BUFFER_SIZE", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->RcBufSize = atoi(sValue);
    if (GetValue(fp, "INTRA_MB_REFRESH", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->IntraRefreshNum = atoi(sValue);

    pEncCfg->ConscIntraRefreshEnable = 0;
    pEncCfg->CountIntraMbEnable = 0;
    pEncCfg->FieldSeqIntraRefreshEnable = 0;
    if (pEncCfg->IntraRefreshNum > 0)
    {
        if (GetValue(fp, "CONSC_INTRA_REFRESH_EN", sValue) == 0)
            pEncCfg->ConscIntraRefreshEnable = 0;
        else
            pEncCfg->ConscIntraRefreshEnable = atoi(sValue);
        if (GetValue(fp, "COUNT_INTRA_MB_EN", sValue) == 0)
            pEncCfg->CountIntraMbEnable = 0;
        else
            pEncCfg->CountIntraMbEnable = atoi(sValue);

        if (GetValue(fp, "FIELD_SEQ_INTRA_REFRESH_EN", sValue) == 0)
            pEncCfg->FieldSeqIntraRefreshEnable = 0;
        else
            pEncCfg->FieldSeqIntraRefreshEnable = atoi(sValue);
    }

    if (GetValue(fp, "FRAME_SKIP_DISABLE", sValue) == 0)
        pEncCfg->frameSkipDisable = 0;
    else
        pEncCfg->frameSkipDisable = atoi(sValue);

    if (GetValue(fp, "CONST_INTRAQP_ENABLE", sValue) == 0)
        pEncCfg->ConstantIntraQPEnable = 0;
    else
        pEncCfg->ConstantIntraQPEnable = atoi(sValue);

    if (GetValue(fp, "RC_INTRA_QP", sValue) == 0)
        pEncCfg->RCIntraQP = 0;
    else
        pEncCfg->RCIntraQP = atoi(sValue);
    if (GetValue(fp, "MAX_QP_SET_ENABLE", sValue) == 0)
        pEncCfg->MaxQpSetEnable= 0;
    else
        pEncCfg->MaxQpSetEnable = atoi(sValue);

    if (GetValue(fp, "MAX_QP", sValue) == 0)
        pEncCfg->MaxQp = 0;
    else
        pEncCfg->MaxQp = atoi(sValue);

    if (GetValue(fp, "MIN_QP_SET_ENABLE", sValue) == 0)
        pEncCfg->MinQpSetEnable= 0;
    else
        pEncCfg->MinQpSetEnable = atoi(sValue);
    if (GetValue(fp, "MIN_QP", sValue) == 0)
        pEncCfg->MinQp = 0;
    else
        pEncCfg->MinQp = atoi(sValue);

    if (GetValue(fp, "MAX_DELTA_QP_SET_ENABLE", sValue) == 0)
        pEncCfg->MaxDeltaQpSetEnable= 0;
    else
        pEncCfg->MaxDeltaQpSetEnable = atoi(sValue);
    if (GetValue(fp, "MAX_DELTA_QP", sValue) == 0)
        pEncCfg->MaxDeltaQp = 0;
    else
        pEncCfg->MaxDeltaQp = atoi(sValue);

    if (GetValue(fp, "MIN_DELTA_QP_SET_ENABLE", sValue) == 0)
        pEncCfg->MinDeltaQpSetEnable= 0;
    else
        pEncCfg->MinDeltaQpSetEnable = atoi(sValue);
    if (GetValue(fp, "MIN_DELTA_QP", sValue) == 0)
        pEncCfg->MinDeltaQp = 0;
    else
        pEncCfg->MinDeltaQp = atoi(sValue);

    if (GetValue(fp, "GAMMA_SET_ENABLE", sValue) == 0)
        pEncCfg->GammaSetEnable = 0;
    else
        pEncCfg->GammaSetEnable = atoi(sValue);
    if (GetValue(fp, "GAMMA", sValue) == 0)
        pEncCfg->Gamma = 0;
    else
        pEncCfg->Gamma = atoi(sValue);
    /* CODA960 features */
    if (GetValue(fp, "RC_INTERVAL_MODE", sValue) == 0)
        pEncCfg->rcIntervalMode = 0;
    else
        pEncCfg->rcIntervalMode = atoi(sValue);

    if (GetValue(fp, "RC_MB_INTERVAL", sValue) == 0)
        pEncCfg->RcMBInterval = 0;
    else
        pEncCfg->RcMBInterval = atoi(sValue);
    /***************************************/
    if (GetValue(fp, "MAX_INTRA_SIZE", sValue) == 0)
        pEncCfg->RcMaxIntraSize = 0;
    else
        pEncCfg->RcMaxIntraSize = atoi(sValue);

    if (GetValue(fp, "RC_INTERVAL_MODE", sValue) == 0)
        pEncCfg->rcIntervalMode = 0;
    else
    {
        pEncCfg->rcIntervalMode = atoi(sValue);
        if (pEncCfg->rcIntervalMode != 0 && pEncCfg->rcIntervalMode != 3) {
            VLOG_EX(ERR, "RC_INTERVAL_MODE Error: %d \n", pEncCfg->rcIntervalMode);
            goto __end_parseAvcCfgFile;
        }

    }

    if (GetValue(fp, "RC_MB_INTERVAL", sValue) == 0)
        pEncCfg->RcMBInterval = 1;
    else
    {
        pEncCfg->RcMBInterval = atoi(sValue);
    }

    if (pEncCfg->RcEnable==0)
    {
        pEncCfg->RcGopIQpOffsetEn = 0;
        pEncCfg->RcGopIQpOffset = 0;
    }
    else
    {
        if (GetValue(fp, "RC_GOP_I_QP_OFFSET_EN", sValue) ==0)
        {
            pEncCfg->RcGopIQpOffsetEn = 0;
            pEncCfg->RcGopIQpOffset = 0;
        }
        else
        {
            pEncCfg->RcGopIQpOffsetEn = atoi(sValue);

            if (GetValue(fp, "RC_GOP_I_QP_OFFSET", sValue) == 0)
                pEncCfg->RcGopIQpOffset = 0;
            else
                pEncCfg->RcGopIQpOffset = atoi(sValue);

            if (pEncCfg->RcGopIQpOffset<-4)
                pEncCfg->RcGopIQpOffset = -4;
            else if (pEncCfg->RcGopIQpOffset>4)
                pEncCfg->RcGopIQpOffset = 4;
        }
    }

    if (GetValue(fp, "SEARCH_RANGE_X", sValue) == 0)	// 3: -16~15, 2:-32~31, 1:-48~47, 0:-64~63, H.263(Short Header : always 0)
        pEncCfg->SearchRangeX = 0;
    else
        pEncCfg->SearchRangeX = atoi(sValue);

    if (GetValue(fp, "SEARCH_RANGE_Y", sValue) == 0)	// 2: -16~15, 1:-32~31, 0:-48~47, H.263(Short Header : always 0)
        pEncCfg->SearchRangeY = 0;
    else
        pEncCfg->SearchRangeY = atoi(sValue);

    if (GetValue(fp, "ENTROPY_CODING_MODE", sValue) == 0)	// 0 : CAVLC, 1 : CABAC, 2: CAVLC/CABAC select according to PicType
        pEncCfg->entropyCodingMode = 0;
    else
        pEncCfg->entropyCodingMode = atoi(sValue);

    if (GetValue(fp, "CABAC_INIT_IDC", sValue) == 0)	// cabac init idc ( 0 ~ 2 )
        pEncCfg->cabacInitIdc = 0;
    else
        pEncCfg->cabacInitIdc = atoi(sValue);

    if (GetValue(fp, "TRANSFORM_8X8_MODE", sValue) == 0)	// 0 : disable(BP), 1 : enable(HP)
        pEncCfg->transform8x8Mode = 0;
    else
        pEncCfg->transform8x8Mode = atoi(sValue);

    if (GetValue(fp, "CHROMA_FORMAT_400", sValue) == 0)		// 0 : 420, 1 : 400 (MONO mode allowed only on HP)
        pEncCfg->chroma_format_400 = 0;
    else
        pEncCfg->chroma_format_400 = atoi(sValue);

    if (GetValue(fp, "INTERLACED_PIC", sValue) == 0)
        pEncCfg->field_flag = 0;
    else
        pEncCfg->field_flag = atoi(sValue);

    if (GetValue(fp, "FIELD_REFERENCE_MODE", sValue) == 0)
        pEncCfg->field_ref_mode = 1;
    else
        pEncCfg->field_ref_mode = atoi(sValue);



    if (GetValue(fp, "EN_ROI", sValue) == 0)
        pEncCfg->coda9RoiEnable = 0;
    else
        pEncCfg->coda9RoiEnable = atoi(sValue)!=0;

    if(pEncCfg->coda9RoiEnable == 1)
    {
        if (GetValue(fp, "ROI_FILE", sValue)!=0)
            sprintf(pEncCfg->RoiFile, "%s",sValue);

        if (GetValue(fp, "ROI_PIC_AVG_QP", sValue) == 0)
            pEncCfg->RoiPicAvgQp = 25;
        else
            pEncCfg->RoiPicAvgQp = atoi(sValue);
    }

    if(GetValue(fp, "SET_DPQ_PIC_NUM", sValue) == 0)
        pEncCfg->set_dqp_pic_num = 1;
    else
        pEncCfg->set_dqp_pic_num = atoi(sValue);

    if(pEncCfg->set_dqp_pic_num == 0)
    {
        printf("SET_DPQ_PIC_NUM must larger than 0");
        goto __end_parseAvcCfgFile;
    }
    if(pEncCfg->set_dqp_pic_num >= 1)
    {
        int i;
        char name[1024];
        for(i=0 ; i< pEncCfg->set_dqp_pic_num; i++)
        {
            sprintf(name,"Frame%d", i);
            if(GetValue(fp, name, sValue) != 0)
            {
                pEncCfg->dqp[i] = atoi(sValue);
            }
            else
            {
                pEncCfg->dqp[i] = 0;
            }
        }
    }
    if(GetValue(fp, "HVS_QP_SCALE_DIV2", sValue) == 0)
        pEncCfg->HvsQpScaleDiv2 = 4;
    else
        pEncCfg->HvsQpScaleDiv2 = atoi(sValue);

    if(GetValue(fp, "EN_HVS_QP", sValue) == 0)
        pEncCfg->EnHvsQp = 1;
    else
        pEncCfg->EnHvsQp = atoi(sValue);

    if(GetValue(fp, "EN_ROW_LEVEL_RC", sValue) == 0)
        pEncCfg->EnRowLevelRc = 1;
    else
        pEncCfg->EnRowLevelRc = atoi(sValue);

    if(GetValue(fp,"RC_INITIAL_QP", sValue) == 0)
        pEncCfg->RcInitialQp = -1;
    else
        pEncCfg->RcInitialQp = atoi(sValue);

    if(GetValue(fp,"HVS_MAX_DELTA_QP", sValue) == 0)
        pEncCfg->RcHvsMaxDeltaQp = 10;
    else
        pEncCfg->RcHvsMaxDeltaQp = atoi(sValue);


    if (pEncCfg->RcEnable == 4)
    {
        if(GetValue(fp,"RC_WEIGHT_UPDATE_FACTOR", sValue) != 0)
            pEncCfg->rcWeightFactor = atoi(sValue);
        else
            pEncCfg->rcWeightFactor = 1;
    }

    osal_memset(pEncCfg->skipPicNums, 0, sizeof(pEncCfg->skipPicNums));
    GetValues(fp, "SKIP_PIC_NUMS", pEncCfg->skipPicNums, sizeof(pEncCfg->skipPicNums));

    if (GetValue(fp, "INTERVIEW_ENABLE", sValue) == 0)
        pEncCfg->interviewEn = 0;
    else
        pEncCfg->interviewEn = atoi(sValue);

    if (GetValue(fp, "PARASET_REFRESH_ENABLE", sValue) == 0)
        pEncCfg->parasetRefreshEn = 0;
    else
        pEncCfg->parasetRefreshEn = atoi(sValue);

    if (GetValue(fp, "PREFIX_NAL_ENABLE", sValue) == 0)
        pEncCfg->prefixNalEn = 0;
    else
        pEncCfg->prefixNalEn = atoi(sValue);

    if (GetValue(fp, "CHANGE_PARAM_CONFIG_FILE", sValue)!=0)
    {
        strcpy(pEncCfg->coda9ParamChange.pchChangeCfgFileName, sValue);
        if (GetValue(fp, "CHANGE_PARAM_FRAME_NUM", sValue)!=0)
            pEncCfg->coda9ParamChange.ChangeFrameNum = atoi(sValue);
        else
            pEncCfg->coda9ParamChange.ChangeFrameNum = 0;
    }
    else
    {
        /* Disable parameter change */
        strcpy(pEncCfg->coda9ParamChange.pchChangeCfgFileName, "");
        pEncCfg->coda9ParamChange.ChangeFrameNum = 0;
    }
    ret = 1; /* Success */
__end_parseAvcCfgFile:
    osal_fclose(fp);
    return ret;
}

static int WAVE_GetStringValue(
    osal_file_t fp,
    char* para,
    char* value
    )
{
    char* token = NULL;
    char lineStr[256] = {0, };

    if((fp == NULL) || (para == NULL) || (value == NULL)){
        return -1;
    }

    osal_fseek(fp, 0, SEEK_SET);

    while (fgets(lineStr, 256, fp) != NULL ) {
        if((lineStr[0] == '#') || (lineStr[0] == ';') || (lineStr[0] == ':')) { // check comment
            continue;
        }
        token = strtok(lineStr, ": "); // parameter name is separated by ' ' or ':'
        if( token != NULL ) {
            if (strcasecmp(para, token) == 0) { // check parameter name
                token = strtok(NULL, "\r\n");
                if(token) {
                    int idx=0, len=0;
                    while ((token[idx] != 0) && ((token[idx] == ' ') || (token[idx] == ':'))) {
                        idx++;
                    }
                    len=strlen(&token[idx]);
                    osal_memcpy(value, &token[idx], len);
                    value[len] = 0;
                    return 1;
                } else {
                    return 0;
                }
            } else {
                continue;
            }
        } else {
            continue;
        }
    }
    return 0;
}

static int WAVE_GetValue(
    osal_file_t fp,
    char* cfgName,
    int* value
    )
{
    int i;
    int iValue;
    int ret;
    char sValue[256] = {0, };
    int maxCfg = sizeof(waveCfgInfo) / sizeof(WaveCfgInfo);

    for (i=0; i < maxCfg ;i++) {
        if ( strcmp(waveCfgInfo[i].name, cfgName) == 0)
            break;
    }
    if ( i == maxCfg ) {
        VLOG_EX(ERR, "CFG param error : %s\n", cfgName);
        return 0;
    }

    ret = WAVE_GetStringValue(fp, cfgName, sValue);
    if(ret == 1) {
        iValue = atoi(sValue);
        if( (iValue >= waveCfgInfo[i].min) && (iValue <= waveCfgInfo[i].max) ) { // Check min, max
            *value = iValue;
            return 1;
        }
        else {
            VLOG_EX(ERR, "CFG file error : %s value is not available. ( min = %d, max = %d)\n", waveCfgInfo[i].name, waveCfgInfo[i].min, waveCfgInfo[i].max);
            return 0;
        }
    }
    else if ( ret == -1 ) {
        VLOG_EX(ERR, "CFG file error : %s value is not available. ( min=%d, max=%d)\n", waveCfgInfo[i].name, waveCfgInfo[i].min, waveCfgInfo[i].max);
        return 0;
    }
    else {
        *value = waveCfgInfo[i].def;
        return 1;
    }
}

static int WAVE_SetGOPInfo(
    char* lineStr,
    CustomGopPicParam* gopPicParam,
    int intraQp
    )
{
    int numParsed;
    char sliceType;

    osal_memset(gopPicParam, 0, sizeof(CustomGopPicParam));

    numParsed = sscanf(lineStr, "%c %d %d %d %d %d",
        &sliceType, &gopPicParam->pocOffset, &gopPicParam->picQp,
        &gopPicParam->temporalId, &gopPicParam->refPocL0, &gopPicParam->refPocL1);

    if (sliceType=='I') {
        gopPicParam->picType = PIC_TYPE_I;
    }
    else if (sliceType=='P') {
        gopPicParam->picType = PIC_TYPE_P;
        gopPicParam->useMultiRefP = (numParsed == 6) ? 1 : 0;
    }
    else if (sliceType=='B') {
        gopPicParam->picType = PIC_TYPE_B;
    }
    else {
        return 0;
    }
    if (sliceType=='B' && numParsed != 6) {
        return 0;
    }
    if (gopPicParam->temporalId < 0) {
        return 0;
    }

    gopPicParam->picQp = MIN(63, gopPicParam->picQp + intraQp);

    return 1;
}

static int WAVE_AVCSetGOPInfo(
    char* lineStr,
    CustomGopPicParam* gopPicParam,
    int intraQp
    )
{
    int numParsed;
    char sliceType;

    osal_memset(gopPicParam, 0, sizeof(CustomGopPicParam));

    numParsed = sscanf(lineStr, "%c %d %d %d %d %d",
        &sliceType, &gopPicParam->pocOffset, &gopPicParam->picQp,
        &gopPicParam->temporalId, &gopPicParam->refPocL0, &gopPicParam->refPocL1);

    if (sliceType=='I') {
        gopPicParam->picType = PIC_TYPE_I;
    }
    else if (sliceType=='P') {
        gopPicParam->picType = PIC_TYPE_P;
        gopPicParam->useMultiRefP = (numParsed == 6) ? 1 : 0;
    }
    else if (sliceType=='B') {
        gopPicParam->picType = PIC_TYPE_B;
    }
    else {
        return 0;
    }
    if (sliceType=='B' && numParsed != 6) {
        return 0;
    }

    gopPicParam->picQp = gopPicParam->picQp + intraQp;

    return 1;
}

int parseRoiCtuModeParam(
    char* lineStr,
    VpuRect* roiRegion,
    int* roiQp,
    int picX,
    int picY
    )
{
    int numParsed;

    osal_memset(roiRegion, 0, sizeof(VpuRect));
    *roiQp = 0;

    numParsed = sscanf(lineStr, "%d %d %d %d %d",
        &roiRegion->left, &roiRegion->right, &roiRegion->top, &roiRegion->bottom, roiQp);

    if (numParsed != 5) {
        return 0;
    }
    if (*roiQp < 0 || *roiQp > 51) {
        return 0;
    }
    if ((Int32)roiRegion->left < 0 || (Int32)roiRegion->top < 0) {
        return 0;
    }
    if (roiRegion->left > (Uint32)((picX + CTB_SIZE - 1) >> LOG2_CTB_SIZE) || \
        roiRegion->top > (Uint32)((picY + CTB_SIZE - 1) >> LOG2_CTB_SIZE)) {
        return 0;
    }
    if (roiRegion->right > (Uint32)((picX + CTB_SIZE - 1) >> LOG2_CTB_SIZE) || \
        roiRegion->bottom > (Uint32)((picY + CTB_SIZE - 1) >> LOG2_CTB_SIZE)) {
        return 0;
    }
    if (roiRegion->left > roiRegion->right) {
        return 0;
    }
    if (roiRegion->top > roiRegion->bottom) {
        return 0;
    }

    return 1;
}

int parseWaveEncCfgFile(
    ENC_CFG *pEncCfg,
    char *FileName,
    int bitFormat
    )
{
    osal_file_t fp;
    char sValue[256] = {0, };
    char tempStr[256] = {0, };
    int iValue = 0, ret = 0, i = 0;

    fp = osal_fopen(FileName, "r");
    if (fp == NULL) {
        VLOG_EX(ERR, "file open err : %s, errno(%d)\n", FileName, errno);
        return ret;
    }

    if (WAVE_GetStringValue(fp, "BitstreamFile", sValue) == 1)
        strcpy(pEncCfg->BitStreamFileName, sValue);

    if (WAVE_GetStringValue(fp, "InputFile", sValue) == 1)
        strcpy(pEncCfg->SrcFileName, sValue);
    else
        goto __end_parse;

    if (WAVE_GetValue(fp, "SourceWidth", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.picX = iValue;
    if (WAVE_GetValue(fp, "SourceHeight", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.picY = iValue;
    if (WAVE_GetValue(fp, "FramesToBeEncoded", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->NumFrame = iValue;
    if (WAVE_GetValue(fp, "InputBitDepth", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->SrcBitDepth = iValue; // BitDepth == 8 ? HEVC_PROFILE_MAIN : HEVC_PROFILE_MAIN10

    if (WAVE_GetValue(fp, "InternalBitDepth", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.internalBitDepth   = iValue;

    if (WAVE_GetValue(fp, "LosslessCoding", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.losslessEnable = iValue;
    if (WAVE_GetValue(fp, "ConstrainedIntraPred", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.constIntraPredFlag = iValue;
    if (WAVE_GetValue(fp, "DecodingRefreshType", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.decodingRefreshType = iValue;

    if (WAVE_GetValue(fp, "StillPictureProfile", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.enStillPicture = iValue;

    // BitAllocMode
    if (WAVE_GetValue(fp, "BitAllocMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bitAllocMode = iValue;

    // FixedBitRatio 0 ~ 7
    for (i=0; i<MAX_GOP_NUM; i++) {
        sprintf(tempStr, "FixedBitRatio%d", i);
        if (WAVE_GetStringValue(fp, tempStr, sValue) == 1) {
            iValue = atoi(sValue);
            if ( iValue >= waveCfgInfo[FIXED_BIT_RATIO].min && iValue <= waveCfgInfo[FIXED_BIT_RATIO].max )
                pEncCfg->waveCfg.fixedBitRatio[i] = iValue;
            else
                pEncCfg->waveCfg.fixedBitRatio[i] = waveCfgInfo[FIXED_BIT_RATIO].def;
        }
        else
            pEncCfg->waveCfg.fixedBitRatio[i] = waveCfgInfo[FIXED_BIT_RATIO].def;

    }

    if (WAVE_GetValue(fp, "QP", &iValue) == 0) //INTRA_QP
        goto __end_parse;
    else
        pEncCfg->waveCfg.intraQP = iValue;

    if (WAVE_GetValue(fp, "IntraPeriod", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.intraPeriod = iValue;

    if (WAVE_GetValue(fp, "ConfWindSizeTop", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.confWinTop = iValue;

    if (WAVE_GetValue(fp, "ConfWindSizeBot", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.confWinBot = iValue;

    if (WAVE_GetValue(fp, "ConfWindSizeRight", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.confWinRight = iValue;

    if (WAVE_GetValue(fp, "ConfWindSizeLeft", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.confWinLeft = iValue;

    if (WAVE_GetValue(fp, "FrameRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.frameRate = iValue;

    if (WAVE_GetValue(fp, "IndeSliceMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.independSliceMode = iValue;

    if (WAVE_GetValue(fp, "IndeSliceArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.independSliceModeArg = iValue;

    if (WAVE_GetValue(fp, "DeSliceMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.dependSliceMode = iValue;

    if (WAVE_GetValue(fp, "DeSliceArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.dependSliceModeArg = iValue;


    if (WAVE_GetValue(fp, "IntraCtuRefreshMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.intraRefreshMode = iValue;


    if (WAVE_GetValue(fp, "IntraCtuRefreshArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.intraRefreshArg = iValue;

    if (WAVE_GetValue(fp, "UsePresetEncTools", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.useRecommendEncParam = iValue;

    if (WAVE_GetValue(fp, "ScalingList", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.scalingListEnable = iValue;

    if (WAVE_GetValue(fp, "EnTemporalMVP", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.tmvpEnable = iValue;

    if (WAVE_GetValue(fp, "WaveFrontSynchro", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.wppenable = iValue;

    if (WAVE_GetValue(fp, "MaxNumMerge", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.maxNumMerge = iValue;

    if (WAVE_GetValue(fp, "EnDBK", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.disableDeblk = !(iValue);

    if (WAVE_GetValue(fp, "LFCrossSliceBoundaryFlag", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.lfCrossSliceBoundaryEnable = iValue;

    if (WAVE_GetValue(fp, "BetaOffsetDiv2", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.betaOffsetDiv2 = iValue;

    if (WAVE_GetValue(fp, "TcOffsetDiv2", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.tcOffsetDiv2 = iValue;

    if (WAVE_GetValue(fp, "IntraTransSkip", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.skipIntraTrans = iValue;

    if (WAVE_GetValue(fp, "EnSAO", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.saoEnable = iValue;

    if (WAVE_GetValue(fp, "IntraNxN", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.intraNxNEnable = iValue;

    if (WAVE_GetValue(fp, "RateControl", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->RcEnable = iValue;

    if (WAVE_GetValue(fp, "EncBitrate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->RcBitRate = iValue;

    if (WAVE_GetValue(fp, "CULevelRateControl", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cuLevelRCEnable = iValue;

    if (WAVE_GetValue(fp, "EnHvsQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.hvsQPEnable = iValue;

    if (WAVE_GetValue(fp, "HvsQpScaleDiv2", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.hvsQpScale = iValue;

    if (WAVE_GetValue(fp, "InitialDelay", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->VbvBufferSize = iValue;

    if (pEncCfg->VbvBufferSize == 0) {
        if (WAVE_GetValue(fp, "VbvBufferSize", &iValue) == 0)
            goto __end_parse;
        else
            pEncCfg->VbvBufferSize = iValue;
    }

    if (WAVE_GetValue(fp, "MinQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.minQp = iValue;

    if (WAVE_GetValue(fp, "MaxQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.maxQp = iValue;

    if (WAVE_GetValue(fp, "MaxDeltaQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.maxDeltaQp = iValue;

    if (WAVE_GetValue(fp, "GOPSize", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.gopParam.customGopSize = iValue;

    if (WAVE_GetValue(fp, "EnRoi", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.roiEnable = iValue;

    if (WAVE_GetValue(fp, "GopPreset", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.gopPresetIdx = iValue;

    if (WAVE_GetValue(fp, "NumUnitsInTick", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.numUnitsInTick = iValue;

    if (WAVE_GetValue(fp, "TimeScale", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.timeScale = iValue;

    if (WAVE_GetValue(fp, "NumTicksPocDiffOne", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.numTicksPocDiffOne = iValue;

    if (WAVE_GetValue(fp, "EncAUD", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.encAUD = iValue;

    if (WAVE_GetValue(fp, "EncEOS", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.encEOS = iValue;

    if (WAVE_GetValue(fp, "EncEOB", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.encEOB = iValue;

    if (WAVE_GetValue(fp, "CbQpOffset", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.chromaCbQpOffset = iValue;

    if (WAVE_GetValue(fp, "CrQpOffset", &iValue) == 0)
            goto __end_parse;
    else
        pEncCfg->waveCfg.chromaCrQpOffset = iValue;

    if (WAVE_GetValue(fp, "RcInitialQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.initialRcQp = iValue;

    if (WAVE_GetValue(fp, "EnNoiseReductionY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrYEnable = iValue;

    if (WAVE_GetValue(fp, "EnNoiseReductionCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrCbEnable = iValue;

    if (WAVE_GetValue(fp, "EnNoiseReductionCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrCrEnable = iValue;

    if (WAVE_GetValue(fp, "EnNoiseEst", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrNoiseEstEnable = iValue;

    if (WAVE_GetValue(fp, "NoiseSigmaY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrNoiseSigmaY = iValue;

    if (WAVE_GetValue(fp, "NoiseSigmaCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrNoiseSigmaCb = iValue;

    if (WAVE_GetValue(fp, "NoiseSigmaCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrNoiseSigmaCr = iValue;

    if (WAVE_GetValue(fp, "IntraNoiseWeightY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrIntraWeightY = iValue;

    if (WAVE_GetValue(fp, "IntraNoiseWeightCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrIntraWeightCb= iValue;

    if (WAVE_GetValue(fp, "IntraNoiseWeightCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrIntraWeightCr = iValue;

    if (WAVE_GetValue(fp, "InterNoiseWeightY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrInterWeightY = iValue;

    if (WAVE_GetValue(fp, "InterNoiseWeightCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrInterWeightCb = iValue;

    if (WAVE_GetValue(fp, "InterNoiseWeightCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrInterWeightCr = iValue;

    if (WAVE_GetValue(fp, "UseAsLongTermRefPeriod", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.useAsLongtermPeriod = iValue;

    if (WAVE_GetValue(fp, "RefLongTermPeriod", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.refLongtermPeriod = iValue;



    //GOP
    if (pEncCfg->waveCfg.intraPeriod == 1) {
        pEncCfg->waveCfg.gopParam.picParam[0].picType = PIC_TYPE_I;
        pEncCfg->waveCfg.gopParam.picParam[0].picQp = pEncCfg->waveCfg.intraQP;
        if (pEncCfg->waveCfg.gopParam.customGopSize > 1) {
            VLOG_EX(ERR, "CFG file error : gop size should be smaller than 2 for all intra case\n");
            goto __end_parse;
        }
    }
    else {
        for (i = 0; pEncCfg->waveCfg.gopPresetIdx == PRESET_IDX_CUSTOM_GOP && i < pEncCfg->waveCfg.gopParam.customGopSize; i++) {

            sprintf(tempStr, "Frame%d", i+1);
            if (WAVE_GetStringValue(fp, tempStr, sValue) != 1) {
                VLOG_EX(ERR, "CFG file error : %s value is not available. \n", tempStr);
                goto __end_parse;
            }

            if ( bitFormat == STD_AVC ) {
                if (WAVE_AVCSetGOPInfo(sValue, &pEncCfg->waveCfg.gopParam.picParam[i], pEncCfg->waveCfg.intraQP) != 1) {
                    VLOG_EX(ERR, "CFG file error : %s value is not available. \n", tempStr);
                    goto __end_parse;
                }
            }
            else {
                if (WAVE_SetGOPInfo(sValue, &pEncCfg->waveCfg.gopParam.picParam[i], pEncCfg->waveCfg.intraQP) != 1) {
                    VLOG_EX(ERR, "CFG file error : %s value is not available. \n", tempStr);
                    goto __end_parse;
                }
#if TEMP_SCALABLE_RC
                if ( (pEncCfg->waveCfg.gopParam.picParam[i].temporalId + 1) > MAX_NUM_TEMPORAL_LAYER) {
                    VLOG_EX(ERR, "CFG file error : %s MaxTempLayer %d exceeds MAX_TEMP_LAYER(7). \n", tempStr, pEncCfg->waveCfg.gopParam.picParam[i].temporalId + 1);
                    goto __end_parse;
                }
#endif
            }
        }
    }

    //ROI
    if (pEncCfg->waveCfg.roiEnable) {
        sprintf(tempStr, "RoiFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.roiFileName);
        }
    }

    if (pEncCfg->waveCfg.losslessEnable) {
        pEncCfg->waveCfg.disableDeblk = 1;
        pEncCfg->waveCfg.saoEnable = 0;
        pEncCfg->RcEnable = 0;
    }

    if (pEncCfg->waveCfg.roiEnable) {
        sprintf(tempStr, "RoiQpMapFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.roiQpMapFile);
        }
    }

    /*======================================================*/
    /*          ONLY for H.264                              */
    /*======================================================*/
    if (WAVE_GetValue(fp, "IdrPeriod", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.idrPeriod = iValue;
    if (WAVE_GetValue(fp, "RdoSkip", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.rdoSkip = iValue;

    if (WAVE_GetValue(fp, "LambdaScaling", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.lambdaScalingEnable = iValue;

    if (WAVE_GetValue(fp, "Transform8x8", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.transform8x8 = iValue;

    if (WAVE_GetValue(fp, "SliceMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.avcSliceMode = iValue;

    if (WAVE_GetValue(fp, "SliceArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.avcSliceArg = iValue;

    if (WAVE_GetValue(fp, "IntraMbRefreshMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.intraMbRefreshMode = iValue;

    if (WAVE_GetValue(fp, "IntraMbRefreshArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.intraMbRefreshArg = iValue;

    if (WAVE_GetValue(fp, "MBLevelRateControl", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.mbLevelRc = iValue;

    if (WAVE_GetValue(fp, "CABAC", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.entropyCodingMode = iValue;

    // H.264 END

    if (WAVE_GetValue(fp, "EncMonochrome", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.monochromeEnable = iValue;

    if (WAVE_GetValue(fp, "StrongIntraSmoothing", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.strongIntraSmoothEnable = iValue;

    if (WAVE_GetValue(fp, "RoiAvgQP", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.roiAvgQp = iValue;

    if (WAVE_GetValue(fp, "WeightedPred", &iValue) == 0)
        goto __end_parse;
    else {
        if      (iValue == 0) {pEncCfg->waveCfg.weightPredEnable = 0;}
        else if (iValue == 3) {pEncCfg->waveCfg.weightPredEnable = 1;}
        else                  {goto __end_parse;}
    }

    if (WAVE_GetValue(fp, "EnBgDetect", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bgDetectEnable = iValue;

    if (WAVE_GetValue(fp, "BgThDiff", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bgThrDiff = iValue;

    if (WAVE_GetValue(fp, "S2fmeOff", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.s2fmeDisable = iValue;

    if (WAVE_GetValue(fp, "BgThMeanDiff", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bgThrMeanDiff = iValue;

    if (WAVE_GetValue(fp, "BgLambdaQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bgLambdaQp = iValue;

    if (WAVE_GetValue(fp, "BgDeltaQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bgDeltaQp = iValue;

    if (WAVE_GetValue(fp, "EnLambdaMap", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.customLambdaMapEnable = iValue;

    if (WAVE_GetValue(fp, "EnCustomLambda", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.customLambdaEnable = iValue;

    if (WAVE_GetValue(fp, "EnCustomMD", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.customMDEnable = iValue;

    if (WAVE_GetValue(fp, "PU04DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu04DeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU08DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu08DeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU16DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu16DeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU32DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu32DeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU04IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu04IntraPlanarDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU04IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu04IntraDcDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU04IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu04IntraAngleDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU08IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu08IntraPlanarDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU08IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu08IntraDcDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU08IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu08IntraAngleDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU16IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu16IntraPlanarDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU16IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu16IntraDcDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU16IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu16IntraAngleDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU32IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu32IntraPlanarDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU32IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu32IntraDcDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU32IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu32IntraAngleDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU08IntraDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu08IntraDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU08InterDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu08InterDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU08MergeDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu08MergeDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU16IntraDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu16IntraDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU16InterDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu16InterDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU16MergeDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu16MergeDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU32IntraDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu32IntraDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU32InterDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu32InterDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU32MergeDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu32MergeDeltaRate = iValue;


    if (WAVE_GetValue(fp, "DisableCoefClear", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.coefClearDisable = iValue;


    if (WAVE_GetValue(fp, "EnModeMap", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.customModeMapFlag = iValue;


    if (WAVE_GetValue(fp, "ForcePicSkipStart", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.forcePicSkipStart = iValue;

    if (WAVE_GetValue(fp, "ForcePicSkipEnd", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.forcePicSkipEnd = iValue;

    if (WAVE_GetValue(fp, "ForceCoefDropStart", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.forceCoefDropStart = iValue;

    if (WAVE_GetValue(fp, "ForceCoefDropEnd", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.forceCoefDropEnd = iValue;

    if (WAVE_GetValue(fp, "RcWeightParaCtrl", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.rcWeightParam = iValue;

    if (WAVE_GetValue(fp, "RcWeightBufCtrl", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.rcWeightBuf = iValue;


    if (WAVE_GetValue(fp, "EnForcedIDRHeader", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.forcedIdrHeaderEnable = iValue;

    if (WAVE_GetValue(fp, "ForceIdrPicIdx", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.forceIdrPicIdx = iValue;

    // Scaling list
    if (pEncCfg->waveCfg.scalingListEnable) {
        sprintf(tempStr, "ScalingListFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.scalingListFileName);
        }
    }
    // Custom Lambda
    if (pEncCfg->waveCfg.customLambdaEnable) {
        sprintf(tempStr, "CustomLambdaFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.customLambdaFileName);
        }
    }


    // custom Lambda Map
    if (pEncCfg->waveCfg.customLambdaMapEnable) {
        sprintf(tempStr, "LambdaMapFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.customLambdaMapFileName);
        }
    }

    // custom Lambda Map
    if (pEncCfg->waveCfg.customModeMapFlag) {
        sprintf(tempStr, "ModeMapFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.customModeMapFileName);
        }
    }

    if (pEncCfg->waveCfg.weightPredEnable) {
        sprintf(tempStr, "WpParamFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.WpParamFileName);
        }
    }

#define NUM_MAX_PARAM_CHANGE    10
    for (i=0; i<NUM_MAX_PARAM_CHANGE; i++) {
        sprintf(tempStr, "SPCh%d", i+1);
        if (WAVE_GetStringValue(fp, tempStr, sValue) != 0) {
            sscanf(sValue, "%d %x %s\n", &(pEncCfg->changeParam[i].setParaChgFrmNum), &(pEncCfg->changeParam[i].enableOption), pEncCfg->changeParam[i].cfgName);
        }
        else {
            pEncCfg->numChangeParam = i;
            break;
        }
    }

    if (WAVE_GetValue(fp, "EnPrefixSeiData", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.prefixSeiEnable = iValue;

    if (WAVE_GetValue(fp, "PrefixSeiDataSize", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.prefixSeiDataSize = iValue;


    if (WAVE_GetValue(fp, "EnSuffixSeiData", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.suffixSeiEnable = iValue;

    if (WAVE_GetValue(fp, "SuffixSeiDataSize", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.suffixSeiDataSize = iValue;


    if (pEncCfg->waveCfg.prefixSeiEnable) {
        sprintf(tempStr, "PrefixSeiDataFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) != 0) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.prefixSeiDataFileName);
        }
    }

    if (pEncCfg->waveCfg.suffixSeiEnable) {
        sprintf(tempStr, "SuffixSeiDataFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) != 0) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.suffixSeiDataFileName);
        }
    }

    if (WAVE_GetValue(fp, "EncodeRbspVui", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.vuiDataEnable = iValue;

    if (WAVE_GetValue(fp, "RbspVuiSize", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.vuiDataSize = iValue;

    if (WAVE_GetValue(fp, "EncodeRbspHrdInVps", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.hrdInVPS = iValue;

    if (WAVE_GetValue(fp, "RbspHrdSize", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.hrdDataSize = iValue;

    if (pEncCfg->waveCfg.hrdInVPS) {
        sprintf(tempStr, "RbspHrdFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) != 0) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.hrdDataFileName);
        }
    }

    if (pEncCfg->waveCfg.vuiDataEnable) {
        sprintf(tempStr, "RbspVuiFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) != 0) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.vuiDataFileName);
        }
    }

    if (WAVE_GetValue(fp, "Profile", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->Profile = iValue;

    ret = 1; /* Success */

__end_parse:
    osal_fclose(fp);
    return ret;
}

int parseWaveChangeParamCfgFile(
    ENC_CFG *pEncCfg,
    char *FileName
    )
{
    osal_file_t fp;
    char sValue[256] = {0, };
    char tempStr[256] = {0, };
    int iValue = 0, ret = 0, i = 0;

    fp = osal_fopen(FileName, "r");
    if (fp == NULL) {
        VLOG_EX(ERR, "file open err : %s, errno(%d)\n", FileName, errno);
        return ret;
    }

    if (WAVE_GetValue(fp, "ConstrainedIntraPred", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.constIntraPredFlag = iValue;

    // FixedBitRatio 0 ~ 7
    for (i=0; i<MAX_GOP_NUM; i++) {
        sprintf(tempStr, "FixedBitRatio%d", i);
        if (WAVE_GetStringValue(fp, tempStr, sValue) == 1) {
            iValue = atoi(sValue);
            if ( iValue >= waveCfgInfo[FIXED_BIT_RATIO].min && iValue <= waveCfgInfo[FIXED_BIT_RATIO].max )
                pEncCfg->waveCfg.fixedBitRatio[i] = iValue;
            else
                pEncCfg->waveCfg.fixedBitRatio[i] = waveCfgInfo[FIXED_BIT_RATIO].def;
        }
        else
            pEncCfg->waveCfg.fixedBitRatio[i] = waveCfgInfo[FIXED_BIT_RATIO].def;

    }

    if (WAVE_GetValue(fp, "IndeSliceMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.independSliceMode = iValue;

    if (WAVE_GetValue(fp, "IndeSliceArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.independSliceModeArg = iValue;

    if (WAVE_GetValue(fp, "DeSliceMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.dependSliceMode = iValue;

    if (WAVE_GetValue(fp, "DeSliceArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.dependSliceModeArg = iValue;

    if (WAVE_GetValue(fp, "MaxNumMerge", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.maxNumMerge = iValue;

    if (WAVE_GetValue(fp, "EnDBK", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.disableDeblk = !(iValue);

    if (WAVE_GetValue(fp, "LFCrossSliceBoundaryFlag", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.lfCrossSliceBoundaryEnable = iValue;

    if (WAVE_GetValue(fp, "DecodingRefreshType", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.decodingRefreshType = iValue;

    if (WAVE_GetValue(fp, "BetaOffsetDiv2", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.betaOffsetDiv2 = iValue;

    if (WAVE_GetValue(fp, "TcOffsetDiv2", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.tcOffsetDiv2 = iValue;

    if (WAVE_GetValue(fp, "IntraNxN", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.intraNxNEnable = iValue;

    if (WAVE_GetValue(fp, "QP", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.intraQP = iValue;

    if (WAVE_GetValue(fp, "IntraPeriod", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.intraPeriod = iValue;

    if (WAVE_GetValue(fp, "EnForcedIDRHeader", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.forcedIdrHeaderEnable = iValue;

    if (WAVE_GetValue(fp, "FrameRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.frameRate = iValue;

    if (WAVE_GetValue(fp, "EncBitrate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->RcBitRate = iValue;

    if (WAVE_GetValue(fp, "EnHvsQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.hvsQPEnable = iValue;

    if (WAVE_GetValue(fp, "HvsQpScaleDiv2", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.hvsQpScale = iValue;

    if (WAVE_GetValue(fp, "InitialDelay", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->VbvBufferSize = iValue;

    if (pEncCfg->VbvBufferSize == 0) {
        if (WAVE_GetValue(fp, "VbvBufferSize", &iValue) == 0)
            goto __end_parse;
        else
            pEncCfg->VbvBufferSize = iValue;
    }

    if (WAVE_GetValue(fp, "MinQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.minQp = iValue;

    if (WAVE_GetValue(fp, "MaxQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.maxQp = iValue;

    if (WAVE_GetValue(fp, "MaxDeltaQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.maxDeltaQp = iValue;


    if (WAVE_GetValue(fp, "CbQpOffset", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.chromaCbQpOffset = iValue;

    if (WAVE_GetValue(fp, "CrQpOffset", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.chromaCrQpOffset = iValue;

    if (WAVE_GetValue(fp, "EnNoiseReductionY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrYEnable = iValue;

    if (WAVE_GetValue(fp, "EnNoiseReductionCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrCbEnable = iValue;

    if (WAVE_GetValue(fp, "EnNoiseReductionCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrCrEnable = iValue;

    if (WAVE_GetValue(fp, "EnNoiseEst", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrNoiseEstEnable = iValue;

    if (WAVE_GetValue(fp, "NoiseSigmaY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrNoiseSigmaY = iValue;

    if (WAVE_GetValue(fp, "NoiseSigmaCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrNoiseSigmaCb = iValue;

    if (WAVE_GetValue(fp, "NoiseSigmaCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrNoiseSigmaCr = iValue;

    if (WAVE_GetValue(fp, "IntraNoiseWeightY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrIntraWeightY = iValue;

    if (WAVE_GetValue(fp, "IntraNoiseWeightCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrIntraWeightCb= iValue;

    if (WAVE_GetValue(fp, "IntraNoiseWeightCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrIntraWeightCr = iValue;

    if (WAVE_GetValue(fp, "InterNoiseWeightY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrInterWeightY = iValue;

    if (WAVE_GetValue(fp, "InterNoiseWeightCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrInterWeightCb = iValue;

    if (WAVE_GetValue(fp, "InterNoiseWeightCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrInterWeightCr = iValue;


    /*======================================================*/
    /*          newly added for WAVE Encoder                */
    /*======================================================*/

    if (WAVE_GetValue(fp, "WeightedPred", &iValue) == 0)
        goto __end_parse;
    else {
        if      (iValue == 0) {pEncCfg->waveCfg.weightPredEnable = 0;}
        else if (iValue == 3) {pEncCfg->waveCfg.weightPredEnable = 1;}
        else                  {goto __end_parse;}
    }

    if (WAVE_GetValue(fp, "EnBgDetect", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bgDetectEnable = iValue;

    if (WAVE_GetValue(fp, "BgThDiff", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bgThrDiff = iValue;

    if (WAVE_GetValue(fp, "S2fmeOff", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.s2fmeDisable = iValue;

    if (WAVE_GetValue(fp, "BgThMeanDiff", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bgThrMeanDiff = iValue;

    if (WAVE_GetValue(fp, "BgLambdaQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bgLambdaQp = iValue;

    if (WAVE_GetValue(fp, "BgDeltaQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bgDeltaQp = iValue;


    if (WAVE_GetValue(fp, "EnCustomLambda", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.customLambdaEnable = iValue;

    if (WAVE_GetValue(fp, "EnCustomMD", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.customMDEnable = iValue;

    if (WAVE_GetValue(fp, "DisableCoefClear", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.coefClearDisable = iValue;

    if (WAVE_GetValue(fp, "RcWeightParaCtrl", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.rcWeightParam = iValue;

    if (WAVE_GetValue(fp, "RcWeightBufCtrl", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.rcWeightBuf = iValue;

    if (WAVE_GetValue(fp, "PU04DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu04DeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU08DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu08DeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU16DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu16DeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU32DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu32DeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU04IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu04IntraPlanarDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU04IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu04IntraDcDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU04IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu04IntraAngleDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU08IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu08IntraPlanarDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU08IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu08IntraDcDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU08IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu08IntraAngleDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU16IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu16IntraPlanarDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU16IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu16IntraDcDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU16IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu16IntraAngleDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU32IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu32IntraPlanarDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU32IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu32IntraDcDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU32IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu32IntraAngleDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU08IntraDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu08IntraDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU08InterDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu08InterDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU08MergeDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu08MergeDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU16IntraDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu16IntraDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU16InterDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu16InterDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU16MergeDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu16MergeDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU32IntraDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu32IntraDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU32InterDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu32InterDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU32MergeDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu32MergeDeltaRate = iValue;

    /*======================================================*/
    /*          only for H.264 encoder                      */
    /*======================================================*/
    if (WAVE_GetValue(fp, "IdrPeriod", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.idrPeriod = iValue;

    if (WAVE_GetValue(fp, "Transform8x8", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.transform8x8 = iValue;

    if (WAVE_GetValue(fp, "SliceMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.avcSliceMode = iValue;

    if (WAVE_GetValue(fp, "SliceArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.avcSliceArg = iValue;

    if (WAVE_GetValue(fp, "CABAC", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.entropyCodingMode = iValue;

    if (WAVE_GetValue(fp, "RdoSkip", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.rdoSkip = iValue;

    if (WAVE_GetValue(fp, "LambdaScaling", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.lambdaScalingEnable = iValue;


    ret = 1; /* Success */

__end_parse:
    osal_fclose(fp);
    return ret;
}
//-------------------------------------------------------------------- Wave6
int w6_get_gop_size(int gop_preset)
{
    int gop_size;

    switch(gop_preset){
        case GOP_PRESET_I_1:
            gop_size=1;
            break;
        case GOP_PRESET_P_1:
            gop_size=1;
            break;
        case GOP_PRESET_B_1:
            gop_size=1;
            break;
        case GOP_PRESET_BP_2:
            gop_size=2;
            break;
        case GOP_PRESET_BBBP_3:
            gop_size=3;
            break;
        case GOP_PRESET_LP_4:
            gop_size=4;
            break;
        case GOP_PRESET_LD_4:
            gop_size=4;
            break;
        case GOP_PRESET_RA_8:
            gop_size=8;
            break;
        case GOP_PRESET_SP_1:
            gop_size=1;
            break;
        case GOP_PRESET_BSP_2:
            gop_size=2;
            break;
        case GOP_PRESET_BBBSP_3:
            gop_size=3;
            break;
        case GOP_PRESET_LSP_4:
            gop_size=4;
            break;
        case GOP_PRESET_BBP_3:
            gop_size=3;
            break;
        case GOP_PRESET_BBSP_3:
            gop_size=3;
            break;
        case GOP_PRESET_BBBBBBBP_8:
            gop_size=8;
            break;
        case GOP_PRESET_BBBBBBBSP_8:
            gop_size=8;
            break;
        default:
            gop_size=1;
            break;
    }
    return gop_size;
}

static void w6_parse_gop_frame(CustomGopParam *p_gop_param, int frame_idx, char *sval) {
    int         step = CGOP_IDX_SLICE_TYPE;
    char        *token = NULL;

    if((p_gop_param == NULL) || (sval == NULL)){
        VLOG_EX(WARN,"[%s:%d]Warning! WAVE6_ENC_CFG is NULL!\n", __FUNCTION__, __LINE__);
        return;
    }

    token=strtok(sval," ");
    if(token == NULL){
        VLOG_EX(WARN,"[%s:%d]Warning! gop frame token is NULL!\n", __FUNCTION__, __LINE__);
        return;
    }
    while(token)
    {
        switch(step) {
        case CGOP_IDX_SLICE_TYPE:
            if(strncmp(token,"P",1) == 0) {
                p_gop_param->picParam[frame_idx].picType=PIC_TYPE_P;
            } else if(strncmp(token,"I",1) == 0) {
                p_gop_param->picParam[frame_idx].picType=PIC_TYPE_I;
            } else if(strncmp(token,"B",1) == 0) {
                p_gop_param->picParam[frame_idx].picType=PIC_TYPE_B;
            } else {
                VLOG_EX(INFO, "Warning! Unknown slice type!, ret: %s\n", token);
            }
            step++;
            break;
        case CGOP_IDX_CURRNET_POC:
            p_gop_param->picParam[frame_idx].pocOffset=((token != NULL) ? atoi(token) : 0);
            step++;
            break;
        case CGOP_IDX_DELTA_QP:
            p_gop_param->picParam[frame_idx].picQp=((token != NULL) ? atoi(token) : 0);
            step++;
            break;
        case CGOP_IDX_USE_MULTI_REF_P:
            p_gop_param->picParam[frame_idx].useMultiRefP=((token != NULL) ? atoi(token) : 0);
            step++;
            break;
        case CGOP_IDX_TEMPORAL_ID:
            p_gop_param->picParam[frame_idx].temporalId=((token != NULL) ? atoi(token) : 0);
            step++;
            break;
        case CGOP_IDX_REF_PIC_POC_0:
            p_gop_param->picParam[frame_idx].refPocL0=((token != NULL) ? atoi(token) : 0);
            step++;
            break;
        case CGOP_IDX_REF_PIC_POC_1:
            p_gop_param->picParam[frame_idx].refPocL1=((token != NULL) ? atoi(token) : 0);
            step++;
            break;
        default:
            VLOG_EX(WARN, "Warnning! Invalid value : CGOP_FRAME_PARAM\n");
            break;
        }
        token=strtok(NULL," ");
    }
    p_gop_param->customGopSize++;
}

static void w6_parse_chnage_pic_param_info(ChangePicParamInfo *chp_info, char *sval) {
    int         step = CHP_IDX_CHNAGE;
    char*       token = NULL;

    if ((chp_info == NULL) || (sval == NULL)) {
        VLOG_EX(WARN, "Warning! chp_info or svals is NULL!\n");
        return;
    }

    token = strtok(sval, " ");
    if (token == NULL) {
        VLOG_EX(WARN, "Warning! gop frame token is NULL!\n");
        return;
    }

    if (chp_info->chgPicParamNum < 0) {
        chp_info->chgPicParamNum = 0;
    }

    while (token) {
        switch (step) {
        case CHP_IDX_CHNAGE:
            chp_info->chgPicParam[chp_info->chgPicParamNum].setParaChgFrmNum = ((token != NULL) ? atoi(token) : 0);
            step++;
            break;
        case CHP_IDX_ENABLE:
            chp_info->chgPicParam[chp_info->chgPicParamNum].enableOption = ((token != NULL) ? atoi(token) : 0);
            step++;
            break;
        case CHP_IDX_CFGDIR:
            osal_memcpy(chp_info->chgPicParam[chp_info->chgPicParamNum].cfgName, token, strlen(token));
            step++;
            break;
        default:
            VLOG_EX(WARN, "Warnning! Invalid value : CHANGE_PARAM_IDX\n");
            break;
        }
        token = strtok(NULL, " ");
    }
    chp_info->chgPicParamNum++;
    return;
}

static void w6_parse_chnage_param_info(ChangeParamInfo *chp_info, char *sval) {
    int         step = CHP_IDX_CHNAGE;
    char*       token = NULL;

    if ((chp_info == NULL) || (sval == NULL)) {
        VLOG_EX(WARN, "Warning! chp_info or svals is NULL!\n");
        return;
    }

    token = strtok(sval," ");
    if (token == NULL) {
        VLOG_EX(WARN, "Warning! gop frame token is NULL!\n");
        return;
    }

    if(chp_info->chgParamNum < 0 ) {
        chp_info->chgParamNum = 0;
    }

    while (token) {
        switch (step) {
            case CHP_IDX_CHNAGE:
                chp_info->chgParam[chp_info->chgParamNum].setParaChgFrmNum = ((token != NULL) ? atoi(token) : 0);
                step++;
                break;
            case CHP_IDX_ENABLE:
                chp_info->chgParam[chp_info->chgParamNum].enableOption = ((token != NULL) ? atoi(token) : 0);
                step++;
                break;
            case CHP_IDX_CFGDIR:
                osal_memcpy(chp_info->chgParam[chp_info->chgParamNum].cfgName, token, strlen(token));
                step++;
                break;
            default:
                VLOG_EX(WARN, "Warnning! Invalid value : CHANGE_PARAM_IDX\n");
                break;
        }
        token = strtok(NULL," ");
    }
    chp_info->chgParamNum++;
    return;
}


#ifdef CHECK_2_PARSE_INVALID_PARAM
int w6_check_invalid_param(osal_file_t fp) {
    int             max_cnt=sizeof(w6_cfg_info)/sizeof(CFGINFO);
    char            line[256] = {0, };
    char            *token = NULL;
    int             i;

    if(fp == NULL) {
        VLOG_EX(ERR, "[%s:%d]Error! Check if fp is NULL!", __FUNCTION__, __LINE__);
        return -1;
    }

    osal_fseek(fp, 0, SEEK_SET);
    while (fgets(line, 256, fp) != NULL) {
        if((line[0] == '#') || (line[0] == ';') || (line[0] == ':')) { // check comment
            continue;
        }
        token = strtok(line, ": "); // parameter name is separated by ' ' or ':'
        if(token != NULL) {
            for(i = 0; i < max_cnt; i++){
                if (strcasecmp(w6_cfg_info[i].name, token) == 0) {
                    break;
                }
            }
            if(i == max_cnt)
            {
                VLOG_EX(ERR, "Error! Invalid Parameter : %s\n", token);
                return -2;
            }
        } else {
            VLOG_EX(ERR, "Error! Invalid Line: %s\n", line);
            return -3;
        }
    }
    return 0;
}
#endif
int w6_check_exception_value(char *except, int val) {
    char    *token;
    char    ex_buffer[MAX_FILE_PATH] = {0, };
    if(except == NULL) {
        VLOG_EX(ERR, "Error! The exception string is NULL!\n");
        return -1;
    }

    if((MAX_FILE_PATH-1) < strlen(except)) {
        osal_memcpy(ex_buffer, except, (MAX_FILE_PATH-1));
    } else {
        osal_memcpy(ex_buffer, except, strlen(except));
    }

    token=strtok(ex_buffer,", ");
    while(token)
    {
        char *pos=strstr(token, "-");
        if(pos) {
            char	smin[32]={0, }; 
            char	smax[32]={0, }; 
            int		imin=0;
            int		imax=0;

            osal_memcpy(smin, token, pos-token);
            osal_memcpy(smax, (char*)(pos+1), strlen((char*)(pos+1)));
            imin=atoi(smin);
            imax=atoi(smax);
            if((imin <= val) && (imax >= val)) {
                VLOG_EX(ERR, "Error! The value is in the exception range from %d to %d (val:%d)\n",imin, imax, val);
                return -2;
            }
        } else {
            int		str_size=strlen(token);
            int		i = 0;
            int		ret = 0;
            for(i = 0; i < str_size; i++){
                if (!isdigit(token[i])) {
                    ret = 1;
                    break;
                }
            }
            if(!ret) {
                int icmp=atoi(token);
                if(icmp == val) {
                    VLOG_EX(ERR, "Error! The value eqaul to the exception value, %d (val:%d)\n",icmp, val);
                    return -3;
                }
            } else {
                VLOG_EX(WARN, "Warning! The exception is skipped by invalid character, %c (Exception str:%s)\n", token[i], token);
            }
        }
        token=strtok(NULL,", ");
    }
    return 0;
}

int w6_parse_cfg_file(osal_file_t fp, CFG_CODEC cd_std)
 {
    int             ci_idx=0;
    int             v_idx=0;
    int             cnt_idx=0;
    int             cp_size=0;
    int             max_cnt=sizeof(w6_cfg_info)/sizeof(CFGINFO);
    char            sval[256] = {0, };
    char            line[256] = {0, };
    char            comp_name[256]= {0, };
    char            *name_tok = NULL;
    char            *val_tok = NULL;

    if(fp == NULL){
        VLOG_EX(ERR, "[%s:%d]Error! Check if arguments are NULL!", __FUNCTION__, __LINE__);
        return -1;
    }

    osal_fseek(fp, 0, SEEK_SET);
    while (fgets(line, 256, fp) != NULL) {
        name_tok = strtok(line, ": "); // parameter name is separated by ' ' or ':'
        if (name_tok == NULL) {
            continue;
        }

        for (ci_idx = 0; ci_idx < max_cnt; ci_idx++) {
            if((w6_cfg_info[ci_idx].dtype == CFG_DTYPE_STR) || (w6_cfg_info[ci_idx].dtype == CFG_DTYPE_INT)) {
                if((strncmp(w6_cfg_info[ci_idx].name, name_tok, strlen(name_tok)) == 0) && (w6_cfg_info[ci_idx].codec & cd_std) == cd_std) {
                    break;
                }
            } else if (w6_cfg_info[ci_idx].dtype == CFG_DTYPE_COUNT) {
                for(cnt_idx=w6_cfg_info[ci_idx].min; cnt_idx < w6_cfg_info[ci_idx].max; cnt_idx++){
                    osal_memset(comp_name, 0, 256);
                    sprintf(comp_name, w6_cfg_info[ci_idx].name, cnt_idx);
                    if(strncmp(comp_name, name_tok, strlen(comp_name)) == 0) {
                        break;
                    }
                }
                if(w6_cfg_info[ci_idx].max != cnt_idx){
                    break;
                }
            }
        }

        if (ci_idx == max_cnt) {
            continue;
        }

        val_tok = strtok(NULL,"\r\n");
        if (val_tok != NULL) {
            for(v_idx=0; ((val_tok[v_idx] != 0) && ((val_tok[v_idx] == ' ') || (val_tok[v_idx] == ':'))); v_idx++);
            cp_size=strlen(&val_tok[v_idx]);
            osal_memset(sval, 0, 256);
            osal_memcpy(sval, &val_tok[v_idx], cp_size);

            switch(w6_cfg_info[ci_idx].dtype) {
                case CFG_DTYPE_STR:
                    if(w6_cfg_info[ci_idx].lval) {
                        char *pclval=w6_cfg_info[ci_idx].lval;
                        osal_memcpy(pclval, sval, strlen(sval));
                    } else {
                        VLOG_EX(WARN, "lval of %s is NULL!", w6_cfg_info[ci_idx].name);
                    }
                    break;
                case CFG_DTYPE_INT:
                    if(w6_cfg_info[ci_idx].except)
                    {
                        if(w6_check_exception_value(w6_cfg_info[ci_idx].except, atoi(sval)) < 0) {
                         VLOG_EX(ERR, "%s:%d Error! the value is exeption number! %s:%s\n", __FUNCTION__, __LINE__, w6_cfg_info[ci_idx].name, sval);
                         return -2;
                        }   
                    }

                    if(w6_cfg_info[ci_idx].lval) {
                        int     *pilval=w6_cfg_info[ci_idx].lval;
                        int     val= atoi(sval);
                        if((val >= w6_cfg_info[ci_idx].min) && (val <= w6_cfg_info[ci_idx].max)){
                            *pilval = val;
                        } else {
                            VLOG_EX(ERR, "%s:%d Error! this value is NOT within the range from min to max, name: %s, min: %d, max: %d, val: %d\n", __FUNCTION__, __LINE__, w6_cfg_info[ci_idx].name, w6_cfg_info[ci_idx].min, w6_cfg_info[ci_idx].max, val);
                            return -3;
                        }
                    } else {
                        VLOG_EX(WARN, "lval of %s is NULL!", w6_cfg_info[ci_idx].name);
                    }
                    break;
                case CFG_DTYPE_COUNT:
                    if(w6_cfg_info[ci_idx].lval) {
                        if(strncmp(name_tok, "Frame", strlen("Frame")) == 0) {
                            w6_parse_gop_frame((CustomGopParam *)w6_cfg_info[ci_idx].lval, cnt_idx - 1, sval);
                        } else if (strncmp(name_tok, "SPCh", strlen("SPCh")) == 0) {
                            w6_parse_chnage_param_info((ChangeParamInfo *)w6_cfg_info[ci_idx].lval, sval);
                        } else if (strncmp(name_tok, "PPCh", strlen("PPCh")) == 0) {
                            w6_parse_chnage_pic_param_info((ChangePicParamInfo *)w6_cfg_info[ci_idx].lval, sval);
                        } else {
                            VLOG_EX(WARN, "Warning! The parameter with count type is invalid! : %s\n", sval);
                        }
                    } else {
                        VLOG_EX(WARN, "lval of %s is NULL!\n", w6_cfg_info[ci_idx].name);
                    }
                    break;
                default:
                    VLOG_EX(WARN, "Warning! Invalid data type! : %d\n", w6_cfg_info[ci_idx].dtype);
                    break;
            }
        } else {
            int *pilval=w6_cfg_info[ci_idx].lval;
            *pilval=w6_cfg_info[ci_idx].def;
        }
    }

    return 0;
}

int w6_init_cfg_info(CFG_CODEC cd_std, WAVE6_ENC_CFG *p_w6_enc_cfg, BOOL init_flag) {
    int i=0;
    int max_num = sizeof(w6_cfg_info)/sizeof(CFGINFO);

    if (!p_w6_enc_cfg) {
        return -1;
    }

    w6_cfg_info[CFGP_IDX_InputFile].lval                    = p_w6_enc_cfg->InputFile;
    w6_cfg_info[CFGP_IDX_SourceWidth].lval                  = &p_w6_enc_cfg->SourceWidth;
    w6_cfg_info[CFGP_IDX_SourceHeight].lval                 = &p_w6_enc_cfg->SourceHeight;
    w6_cfg_info[CFGP_IDX_InputBitDepth].lval                = &p_w6_enc_cfg->InputBitDepth;
    w6_cfg_info[CFGP_IDX_FrameRate].lval                    = &p_w6_enc_cfg->FrameRate;
    w6_cfg_info[CFGP_IDX_FramesToBeEncoded].lval            = &p_w6_enc_cfg->FramesToBeEncoded;
    w6_cfg_info[CFGP_IDX_BitstreamFile].lval                = p_w6_enc_cfg->BitstreamFile;
    w6_cfg_info[CFGP_IDX_ReconFile].lval                    = p_w6_enc_cfg->ReconFile;
    w6_cfg_info[CFGP_IDX_InternalBitDepth].lval             = &p_w6_enc_cfg->InternalBitDepth;
    w6_cfg_info[CFGP_IDX_DecodingRefreshType].lval          = &p_w6_enc_cfg->DecodingRefreshType;
    w6_cfg_info[CFGP_IDX_IdrPeriod].lval                    = &p_w6_enc_cfg->IdrPeriod;
    w6_cfg_info[CFGP_IDX_IntraPeriod].lval                  = &p_w6_enc_cfg->IntraPeriod;
    w6_cfg_info[CFGP_IDX_GopPreset].lval                    = &p_w6_enc_cfg->GopPreset;
    w6_cfg_info[CFGP_IDX_GOPSize].lval                      = &p_w6_enc_cfg->GOPSize;
    w6_cfg_info[CFGP_IDX_FrameN].lval                       = &p_w6_enc_cfg->gopParam;
    w6_cfg_info[CFGP_IDX_UseAsLongTermRefPeriod].lval       = &p_w6_enc_cfg->UseAsLongTermRefPeriod;
    w6_cfg_info[CFGP_IDX_RefLongTermPeriod].lval            = &p_w6_enc_cfg->RefLongTermPeriod;
    w6_cfg_info[CFGP_IDX_RateControl].lval                  = &p_w6_enc_cfg->RateControl;
    w6_cfg_info[CFGP_IDX_EncBitrate].lval                   = &p_w6_enc_cfg->EncBitrate;
    w6_cfg_info[CFGP_IDX_VbvBufferSize].lval                = &p_w6_enc_cfg->VbvBufferSize;
    w6_cfg_info[CFGP_IDX_CULevelRateControl].lval           = &p_w6_enc_cfg->CULevelRateControl;
    w6_cfg_info[CFGP_IDX_EnHvsQp].lval                      = &p_w6_enc_cfg->EnHvsQp;
    w6_cfg_info[CFGP_IDX_HvsQpScaleDiv2].lval               = &p_w6_enc_cfg->HvsQpScaleDiv2;
    w6_cfg_info[CFGP_IDX_HvsMaxDeltaQp].lval                = &p_w6_enc_cfg->HvsMaxDeltaQp;
    w6_cfg_info[CFGP_IDX_RcInitialQp].lval                  = &p_w6_enc_cfg->RcInitialQp;
    w6_cfg_info[CFGP_IDX_RcUpdateSpeed].lval                = &p_w6_enc_cfg->RcUpdateSpeed;
    w6_cfg_info[CFGP_IDX_PicRcMaxDqp].lval                  = &p_w6_enc_cfg->PicRcMaxDqp;
    w6_cfg_info[CFGP_IDX_MinQp_AV1].lval                    = &p_w6_enc_cfg->MinQp;
    w6_cfg_info[CFGP_IDX_MinQp_AVC_HEVC].lval               = &p_w6_enc_cfg->MinQp;
    w6_cfg_info[CFGP_IDX_MaxQp_AV1].lval                    = &p_w6_enc_cfg->MaxQp;
    w6_cfg_info[CFGP_IDX_MaxQp_AVC_HEVC].lval               = &p_w6_enc_cfg->MaxQp;
    w6_cfg_info[CFGP_IDX_MaxBitrate].lval                   = &p_w6_enc_cfg->MaxBitrate;
    w6_cfg_info[CFGP_IDX_RcInitLevel].lval                  = &p_w6_enc_cfg->RcInitLevel;
    w6_cfg_info[CFGP_IDX_EnBgDetect].lval                   = &p_w6_enc_cfg->EnBgDetect;
    w6_cfg_info[CFGP_IDX_BgThDiff].lval                     = &p_w6_enc_cfg->BgThDiff;
    w6_cfg_info[CFGP_IDX_BgThMeanDiff].lval                 = &p_w6_enc_cfg->BgThMeanDiff;
    w6_cfg_info[CFGP_IDX_BgDeltaQp].lval                    = &p_w6_enc_cfg->BgDeltaQp;
    w6_cfg_info[CFGP_IDX_StrongIntraSmoothing].lval         = &p_w6_enc_cfg->StrongIntraSmoothing;
    w6_cfg_info[CFGP_IDX_ConstrainedIntraPred].lval         = &p_w6_enc_cfg->ConstrainedIntraPred;
    w6_cfg_info[CFGP_IDX_IntraNxN_HEVC].lval                = &p_w6_enc_cfg->IntraNxN;
    w6_cfg_info[CFGP_IDX_IntraNxN_AV1].lval                 = &p_w6_enc_cfg->IntraNxN;
    w6_cfg_info[CFGP_IDX_IntraTransSkip].lval               = &p_w6_enc_cfg->IntraTransSkip;
    w6_cfg_info[CFGP_IDX_IntraRefreshMode].lval             = &p_w6_enc_cfg->IntraRefreshMode;
    w6_cfg_info[CFGP_IDX_IntraRefreshArg].lval              = &p_w6_enc_cfg->IntraRefreshArg;
    w6_cfg_info[CFGP_IDX_EnTemporalMVP].lval                = &p_w6_enc_cfg->EnTemporalMVP;
    w6_cfg_info[CFGP_IDX_MeCenter].lval                     = &p_w6_enc_cfg->MeCenter;
    w6_cfg_info[CFGP_IDX_CABAC].lval                        = &p_w6_enc_cfg->CABAC;
    w6_cfg_info[CFGP_IDX_Transform8x8].lval                 = &p_w6_enc_cfg->Transform8x8;
    w6_cfg_info[CFGP_IDX_LFCrossSliceBoundaryFlag].lval     = &p_w6_enc_cfg->LFCrossSliceBoundaryFlag;
    w6_cfg_info[CFGP_IDX_EnDBK].lval                        = &p_w6_enc_cfg->EnDBK;
    w6_cfg_info[CFGP_IDX_BetaOffsetDiv2].lval               = &p_w6_enc_cfg->BetaOffsetDiv2;
    w6_cfg_info[CFGP_IDX_TcOffsetDiv2].lval                 = &p_w6_enc_cfg->TcOffsetDiv2;
    w6_cfg_info[CFGP_IDX_LfSharpness].lval                  = &p_w6_enc_cfg->LfSharpness;
    w6_cfg_info[CFGP_IDX_EnSAO].lval                        = &p_w6_enc_cfg->EnSAO;
    w6_cfg_info[CFGP_IDX_EnCdef].lval                       = &p_w6_enc_cfg->EnCdef;
    w6_cfg_info[CFGP_IDX_EnWiener].lval                     = &p_w6_enc_cfg->EnWiener;
    w6_cfg_info[CFGP_IDX_QP].lval                           = &p_w6_enc_cfg->QP;
    w6_cfg_info[CFGP_IDX_CbQpOffset].lval                   = &p_w6_enc_cfg->CbQpOffset;
    w6_cfg_info[CFGP_IDX_CrQpOffset].lval                   = &p_w6_enc_cfg->CrQpOffset;
    w6_cfg_info[CFGP_IDX_YDcQpDelta].lval                   = &p_w6_enc_cfg->YDcQpDelta;
    w6_cfg_info[CFGP_IDX_UDcQpDelta].lval                   = &p_w6_enc_cfg->UDcQpDelta;
    w6_cfg_info[CFGP_IDX_VDcQpDelta].lval                   = &p_w6_enc_cfg->VDcQpDelta;
    w6_cfg_info[CFGP_IDX_UAcQpDelta].lval                   = &p_w6_enc_cfg->UAcQpDelta;
    w6_cfg_info[CFGP_IDX_VAcQpDelta].lval                   = &p_w6_enc_cfg->VAcQpDelta;
    w6_cfg_info[CFGP_IDX_ScalingList].lval                  = &p_w6_enc_cfg->ScalingList;
    w6_cfg_info[CFGP_IDX_AdaptiveRound].lval                = &p_w6_enc_cfg->AdaptiveRound;
    w6_cfg_info[CFGP_IDX_QRoundIntra].lval                  = &p_w6_enc_cfg->QRoundIntra;
    w6_cfg_info[CFGP_IDX_QRoundInter].lval                  = &p_w6_enc_cfg->QRoundInter;
    w6_cfg_info[CFGP_IDX_DisableCoefClear].lval             = &p_w6_enc_cfg->DisableCoefClear;
    w6_cfg_info[CFGP_IDX_LambdaDqpIntra].lval               = &p_w6_enc_cfg->LambdaDqpIntra;
    w6_cfg_info[CFGP_IDX_LambdaDqpInter].lval               = &p_w6_enc_cfg->LambdaDqpInter;
    w6_cfg_info[CFGP_IDX_SliceMode].lval                    = &p_w6_enc_cfg->SliceMode;
    w6_cfg_info[CFGP_IDX_SliceArg].lval                     = &p_w6_enc_cfg->SliceArg;
    w6_cfg_info[CFGP_IDX_NumColTiles].lval                  = &p_w6_enc_cfg->NumColTiles;
    w6_cfg_info[CFGP_IDX_NumRowTiles].lval                  = &p_w6_enc_cfg->NumRowTiles;
    w6_cfg_info[CFGP_IDX_EnQpMap].lval                      = &p_w6_enc_cfg->EnQpMap;
    w6_cfg_info[CFGP_IDX_EnModeMap].lval                    = &p_w6_enc_cfg->EnModeMap;
    w6_cfg_info[CFGP_IDX_CustomMapFile].lval                = p_w6_enc_cfg->CustomMapFile;
    w6_cfg_info[CFGP_IDX_EnCustomLambda].lval               = &p_w6_enc_cfg->EnCustomLambda;
    w6_cfg_info[CFGP_IDX_CustomLambdaFile].lval             = p_w6_enc_cfg->CustomLambdaFile;
    w6_cfg_info[CFGP_IDX_ForcePicSkipStart].lval            = &p_w6_enc_cfg->ForcePicSkipStart;
    w6_cfg_info[CFGP_IDX_ForcePicSkipEnd].lval              = &p_w6_enc_cfg->ForcePicSkipEnd;
    w6_cfg_info[CFGP_IDX_ForceIdrPicIdx].lval               = &p_w6_enc_cfg->ForceIdrPicIdx;
    //w6_cfg_info[CFGP_IDX_RdoBiasBlk4].lval                  = &p_w6_enc_cfg->RdoBiasBlk4;
    //w6_cfg_info[CFGP_IDX_RdoBiasBlk8].lval                  = &p_w6_enc_cfg->RdoBiasBlk8;
    //w6_cfg_info[CFGP_IDX_RdoBiasBlk16].lval                 = &p_w6_enc_cfg->RdoBiasBlk16;
    //w6_cfg_info[CFGP_IDX_RdoBiasBlk32].lval                 = &p_w6_enc_cfg->RdoBiasBlk32;
    //w6_cfg_info[CFGP_IDX_RdoBiasIntra].lval                 = &p_w6_enc_cfg->RdoBiasIntra;
    //w6_cfg_info[CFGP_IDX_RdoBiasInter].lval                 = &p_w6_enc_cfg->RdoBiasInter;
    //w6_cfg_info[CFGP_IDX_RdoBiasMerge].lval                 = &p_w6_enc_cfg->RdoBiasMerge;
    w6_cfg_info[CFGP_IDX_NumUnitsInTick].lval               = &p_w6_enc_cfg->NumUnitsInTick;
    w6_cfg_info[CFGP_IDX_TimeScale].lval                    = &p_w6_enc_cfg->TimeScale;
    w6_cfg_info[CFGP_IDX_ConfWindSizeTop].lval              = &p_w6_enc_cfg->ConfWindSizeTop;
    w6_cfg_info[CFGP_IDX_ConfWindSizeBot].lval              = &p_w6_enc_cfg->ConfWindSizeBot;
    w6_cfg_info[CFGP_IDX_ConfWindSizeRight].lval            = &p_w6_enc_cfg->ConfWindSizeRight;
    w6_cfg_info[CFGP_IDX_ConfWindSizeLeft].lval             = &p_w6_enc_cfg->ConfWindSizeLeft;
    w6_cfg_info[CFGP_IDX_NoLastFrameCheck].lval             = &p_w6_enc_cfg->NoLastFrameCheck;
    w6_cfg_info[CFGP_IDX_EnPrefixSeiData].lval              = &p_w6_enc_cfg->EnPrefixSeiData;
    w6_cfg_info[CFGP_IDX_PrefixSeiDataFile].lval            = p_w6_enc_cfg->PrefixSeiDataFile;
    w6_cfg_info[CFGP_IDX_PrefixSeiSize].lval                = &p_w6_enc_cfg->PrefixSeiSize;
    w6_cfg_info[CFGP_IDX_EnSuffixSeiData].lval              = &p_w6_enc_cfg->EnSuffixSeiData;
    w6_cfg_info[CFGP_IDX_SuffixSeiDataFile].lval            = p_w6_enc_cfg->SuffixSeiDataFile;
    w6_cfg_info[CFGP_IDX_SuffixSeiSize].lval                = &p_w6_enc_cfg->SuffixSeiSize;
    w6_cfg_info[CFGP_IDX_EncodeRbspHrd].lval                = &p_w6_enc_cfg->EncodeRbspHrd;
    w6_cfg_info[CFGP_IDX_RbspHrdFile].lval                  = p_w6_enc_cfg->RbspHrdFile;
    w6_cfg_info[CFGP_IDX_RbspHrdSize].lval                  = &p_w6_enc_cfg->RbspHrdSize;
    w6_cfg_info[CFGP_IDX_EncodeRbspVui].lval                = &p_w6_enc_cfg->EncodeRbspVui;
    w6_cfg_info[CFGP_IDX_RbspVuiFile].lval                  = p_w6_enc_cfg->RbspVuiFile;
    w6_cfg_info[CFGP_IDX_RbspVuiSize].lval                  = &p_w6_enc_cfg->RbspVuiSize;
    w6_cfg_info[CFGP_IDX_EncAUD].lval                       = &p_w6_enc_cfg->EncAUD;
    w6_cfg_info[CFGP_IDX_EncEOS].lval                       = &p_w6_enc_cfg->EncEOS;
    w6_cfg_info[CFGP_IDX_EncEOB].lval                       = &p_w6_enc_cfg->EncEOB;
    w6_cfg_info[CFGP_IDX_NumTicksPocDiffOne].lval           = &p_w6_enc_cfg->NumTicksPocDiffOne;
    w6_cfg_info[CFGP_IDX_MinQpI_AV1].lval                   = &p_w6_enc_cfg->minQpI;
    w6_cfg_info[CFGP_IDX_MinQpI_AVC_HEVC].lval              = &p_w6_enc_cfg->minQpI;
    w6_cfg_info[CFGP_IDX_MaxQpI_AV1].lval                   = &p_w6_enc_cfg->maxQpI;
    w6_cfg_info[CFGP_IDX_MaxQpI_AVC_HEVC].lval              = &p_w6_enc_cfg->maxQpI;
    w6_cfg_info[CFGP_IDX_MinQpP_AV1].lval                   = &p_w6_enc_cfg->minQpP;
    w6_cfg_info[CFGP_IDX_MinQpP_AVC_HEVC].lval              = &p_w6_enc_cfg->minQpP;
    w6_cfg_info[CFGP_IDX_MaxQpP_AV1].lval                   = &p_w6_enc_cfg->maxQpP;
    w6_cfg_info[CFGP_IDX_MaxQpP_AVC_HEVC].lval              = &p_w6_enc_cfg->maxQpP;
    w6_cfg_info[CFGP_IDX_MinQpB_AV1].lval                   = &p_w6_enc_cfg->minQpB;
    w6_cfg_info[CFGP_IDX_MinQpB_AVC_HEVC].lval              = &p_w6_enc_cfg->minQpB;
    w6_cfg_info[CFGP_IDX_MaxQpB_AV1].lval                   = &p_w6_enc_cfg->maxQpB;
    w6_cfg_info[CFGP_IDX_MaxQpB_AVC_HEVC].lval              = &p_w6_enc_cfg->maxQpB;
    w6_cfg_info[CFGP_IDX_Tid0_QpI_AV1].lval                 = &p_w6_enc_cfg->temporal_layer_qp[0].qpI;
    w6_cfg_info[CFGP_IDX_Tid0_QpI_AVC_HEVC].lval            = &p_w6_enc_cfg->temporal_layer_qp[0].qpI;
    w6_cfg_info[CFGP_IDX_Tid0_QpP_AV1].lval                 = &p_w6_enc_cfg->temporal_layer_qp[0].qpP;
    w6_cfg_info[CFGP_IDX_Tid0_QpP_AVC_HEVC].lval            = &p_w6_enc_cfg->temporal_layer_qp[0].qpP;
    w6_cfg_info[CFGP_IDX_Tid0_QpB_AV1].lval                 = &p_w6_enc_cfg->temporal_layer_qp[0].qpB;
    w6_cfg_info[CFGP_IDX_Tid0_QpB_AVC_HEVC].lval            = &p_w6_enc_cfg->temporal_layer_qp[0].qpB;
    w6_cfg_info[CFGP_IDX_Tid1_QpI_AV1].lval                 = &p_w6_enc_cfg->temporal_layer_qp[1].qpI;
    w6_cfg_info[CFGP_IDX_Tid1_QpI_AVC_HEVC].lval            = &p_w6_enc_cfg->temporal_layer_qp[1].qpI;
    w6_cfg_info[CFGP_IDX_Tid1_QpP_AV1].lval                 = &p_w6_enc_cfg->temporal_layer_qp[1].qpP;
    w6_cfg_info[CFGP_IDX_Tid1_QpP_AVC_HEVC].lval            = &p_w6_enc_cfg->temporal_layer_qp[1].qpP;
    w6_cfg_info[CFGP_IDX_Tid1_QpB_AV1].lval                 = &p_w6_enc_cfg->temporal_layer_qp[1].qpB;
    w6_cfg_info[CFGP_IDX_Tid1_QpB_AVC_HEVC].lval            = &p_w6_enc_cfg->temporal_layer_qp[1].qpB;
    w6_cfg_info[CFGP_IDX_Tid2_QpI_AV1].lval                 = &p_w6_enc_cfg->temporal_layer_qp[2].qpI;
    w6_cfg_info[CFGP_IDX_Tid2_QpI_AVC_HEVC].lval            = &p_w6_enc_cfg->temporal_layer_qp[2].qpI;
    w6_cfg_info[CFGP_IDX_Tid2_QpP_AV1].lval                 = &p_w6_enc_cfg->temporal_layer_qp[2].qpP;
    w6_cfg_info[CFGP_IDX_Tid2_QpP_AVC_HEVC].lval            = &p_w6_enc_cfg->temporal_layer_qp[2].qpP;
    w6_cfg_info[CFGP_IDX_Tid2_QpB_AV1].lval                 = &p_w6_enc_cfg->temporal_layer_qp[2].qpB;
    w6_cfg_info[CFGP_IDX_Tid2_QpB_AVC_HEVC].lval            = &p_w6_enc_cfg->temporal_layer_qp[2].qpB;
    w6_cfg_info[CFGP_IDX_Tid3_QpI_AV1].lval                 = &p_w6_enc_cfg->temporal_layer_qp[3].qpI;
    w6_cfg_info[CFGP_IDX_Tid3_QpI_AVC_HEVC].lval            = &p_w6_enc_cfg->temporal_layer_qp[3].qpI;
    w6_cfg_info[CFGP_IDX_Tid3_QpP_AV1].lval                 = &p_w6_enc_cfg->temporal_layer_qp[3].qpP;
    w6_cfg_info[CFGP_IDX_Tid3_QpP_AVC_HEVC].lval            = &p_w6_enc_cfg->temporal_layer_qp[3].qpP;
    w6_cfg_info[CFGP_IDX_Tid3_QpB_AV1].lval                 = &p_w6_enc_cfg->temporal_layer_qp[3].qpB;
    w6_cfg_info[CFGP_IDX_Tid3_QpB_AVC_HEVC].lval            = &p_w6_enc_cfg->temporal_layer_qp[3].qpB;
    w6_cfg_info[CFGP_IDX_ForcePicQpPicIdx].lval             = &p_w6_enc_cfg->ForcePicQpPicIdx;
    w6_cfg_info[CFGP_IDX_ForcePicQpValue_AV1].lval          = &p_w6_enc_cfg->ForcePicQpValue;
    w6_cfg_info[CFGP_IDX_ForcePicQpValue_AVC_HEVC].lval     = &p_w6_enc_cfg->ForcePicQpValue;
    w6_cfg_info[CFGP_IDX_ForcePicTypePicIdx].lval           = &p_w6_enc_cfg->ForcePicTypePicIdx;
    w6_cfg_info[CFGP_IDX_ForcePicTypeValue].lval            = &p_w6_enc_cfg->ForcePicTypeValue;
    w6_cfg_info[CFGP_IDX_TemporalLayerCount].lval           = &p_w6_enc_cfg->temporal_layer_count;
    w6_cfg_info[CFGP_IDX_ApiScenario].lval                  = &p_w6_enc_cfg->ApiScenario;
    w6_cfg_info[CFGP_IDX_SPChN].lval                        = &p_w6_enc_cfg->chgParamInfo;
    w6_cfg_info[CFGP_IDX_PPChN].lval                        = &p_w6_enc_cfg->chgPicParamInfo;

    if (init_flag == TRUE) {
        for (i=0; i < max_num; i++) {
            if((w6_cfg_info[i].dtype == CFG_DTYPE_INT) && ((w6_cfg_info[i].codec & cd_std) == cd_std)) {
                int *pilval = w6_cfg_info[i].lval;
                if(pilval){
                    *pilval=w6_cfg_info[i].def;
                }
            }
        }
    }

    return 0;
}

int w6_get_enc_cfg(char *FileName, CodStd cd_idx, WAVE6_ENC_CFG *p_w6_enc_cfg, BOOL init_flag)
{
    osal_file_t     fp = NULL;
    CFG_CODEC       cd_std=(CFG_CODEC)(1 << (unsigned long long)cd_idx);

    if((FileName == NULL) || (p_w6_enc_cfg == NULL)) {
        VLOG_EX(ERR,"[%s:%d]Error! the arguments is NULL!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    fp = osal_fopen(FileName, "r");
    if (fp == NULL) {
        VLOG_EX(ERR,"[%s:%d]Error! fp is NULL!, %s\n", __FUNCTION__, __LINE__, FileName);
        return -2;
    }

#ifdef CHECK_2_PARSE_INVALID_PARAM
    if(w6_check_invalid_param(fp) < 0) {
        VLOG_EX(ERR,"[%s:%d]Error! the cfg file involve invalid parameter!\n", __FUNCTION__, __LINE__);
        osal_fclose(fp);
        return -3;
    }
#endif

    if(w6_init_cfg_info(cd_std, p_w6_enc_cfg, init_flag) < 0) {
        VLOG_EX(ERR,"[%s:%d]Error! Failed to parse the cfg file\n", __FUNCTION__, __LINE__);
        osal_fclose(fp);
        return -4;
    }

    if(w6_parse_cfg_file(fp, cd_std) < 0) {
        VLOG_EX(ERR,"[%s:%d]Error! Failed to parse the cfg file\n", __FUNCTION__, __LINE__);
        osal_fclose(fp);
        return -5;
    }

    osal_fclose(fp);
    return 0;
}

int ParseChangeParamCfgFile(ParamChange* pEncCfg, char* FileName)
{
    FILE *fp;
    char sValue[1024];

    /* Initialize enable flags */
    pEncCfg->paraEnable = 0;

    fp = osal_fopen(FileName, "rt");
    if (fp == NULL)
    {
        fprintf(stderr, "File not exist <%s>\n", FileName);
        return 0;
    }

    /*
        CFG Change Param Enable
        [0]  : GOP_NUM
        [1]  : INTRA_QP
        [2]  : BIT_RATE
        [3]  : FRAME_RATE
        [4]  : INTRA_REFRESH
        [5]  : MIN_MAX_QP
        [14] : PIC_PARAM
    */
    if (GetValue(fp, "PARAM_CHANGE_ENABLE", sValue)!=0)
        pEncCfg->paraEnable                 = atoi(sValue);
    else
        pEncCfg->paraEnable                 = 0;

    // Check MIN_MAX_QP CFG enable bit, and convert to C9EncChangeParam enable bit.
    // because, CFG enable bit is 5 and C9EncChangeParam enable bit is 6.
    if (pEncCfg->paraEnable & (1<<5)) {
        // disable bit 5
        pEncCfg->paraEnable &= ~(1<<5);
        // enable bit 6
        pEncCfg->paraEnable |=  (1<<6);
    }

    /* Change GOP NUM */
    if (pEncCfg->paraEnable & C9_ENC_CHANGE_PARAM_GOP_NUM)
    {
        if (GetValue(fp, "PARAM_CHANGE_GOP_NUM", sValue)!=0)
            pEncCfg->NewGopNum              = atoi(sValue);
        else
            pEncCfg->NewGopNum              = 30;

    }
    /* Change Constant Intra QP */
    if (pEncCfg->paraEnable & C9_ENC_CHANGE_PARAM_INTRA_QP)
    {
        if (GetValue(fp, "PARAM_CHANGE_INTRA_QP", sValue)!=0)
            pEncCfg->NewIntraQp             = atoi(sValue);
        else
            pEncCfg->NewIntraQp             = 25;

    }
    /* Change Bitrate */
    if (pEncCfg->paraEnable & C9_ENC_CHANGE_PARAM_BIT_RATE)
    {
        if (GetValue(fp, "PARAM_CHANGE_BIT_RATE", sValue)!=0)
            pEncCfg->NewBitrate             = atoi(sValue);
        else
            pEncCfg->NewBitrate             = 0;            // RC will be disabled
    }
    /* Change Frame Rate */
    if (pEncCfg->paraEnable & C9_ENC_CHANGE_PARAM_FRAME_RATE)
    {
        if (GetValue(fp, "PARAM_CHANGE_FRAME_RATE", sValue)!=0)
            pEncCfg->NewFrameRate           = atoi(sValue);
        else
            pEncCfg->NewFrameRate			= 30;
    }
    /* Change Intra Refresh */
    if (pEncCfg->paraEnable & C9_ENC_CHANGE_PARAM_INTRA_REFRESH)
    {
        if (GetValue(fp, "PARAM_CHANGE_INTRA_REFRESH", sValue)!=0)
            pEncCfg->NewIntraRefresh        = atoi(sValue);
        else
            pEncCfg->NewIntraRefresh        = 0;
    }

    if (pEncCfg->paraEnable & C9_ENC_CHANGE_PARAM_MIN_MAX_QP)
    {
        if (GetValue(fp, "PARAM_CHANGE_MAX_QP_I", sValue)!=0)
        {
            pEncCfg->minMaxQpParam.maxQpI = atoi(sValue);
            pEncCfg->minMaxQpParam.maxQpIEnable = 1;
        }
        else
        {
            pEncCfg->minMaxQpParam.maxQpI = 51;
            pEncCfg->minMaxQpParam.maxQpIEnable = 0;
        }

        if (GetValue(fp, "PARAM_CHANGE_MIN_QP_I", sValue)!=0)
        {
            pEncCfg->minMaxQpParam.minQpI = atoi(sValue);
            pEncCfg->minMaxQpParam.minQpIEnable = 1;
        }
        else
        {
            pEncCfg->minMaxQpParam.minQpI = 12;
            pEncCfg->minMaxQpParam.minQpIEnable = 0;
        }


        if (GetValue(fp, "PARAM_CHANGE_MAX_QP_P", sValue)!=0)
        {
            pEncCfg->minMaxQpParam.maxQpP = atoi(sValue);
            pEncCfg->minMaxQpParam.maxQpPEnable = 1;
        }
        else
        {
            pEncCfg->minMaxQpParam.maxQpP = 51;
            pEncCfg->minMaxQpParam.maxQpPEnable = 0;
        }


        if (GetValue(fp, "PARAM_CHANGE_MIN_QP_P", sValue)!=0)
        {
            pEncCfg->minMaxQpParam.minQpP = atoi(sValue);
            pEncCfg->minMaxQpParam.minQpPEnable = 1;
        }
        else
        {
            pEncCfg->minMaxQpParam.minQpP = 12;
            pEncCfg->minMaxQpParam.minQpPEnable = 0;
        }
    }

    if (pEncCfg->paraEnable & C9_ENC_CHANGE_PARAM_PIC_PARAM)
    {
        if(GetValue(fp, "PARAM_CHANGE_MAX_DELTA_QP", sValue)!=0)
            pEncCfg->changePicParam.MaxdeltaQp = atoi(sValue);
        else
            pEncCfg->changePicParam.MaxdeltaQp = 51;

        if(GetValue(fp, "PARAM_CHANGE_MIN_DELTA_QP", sValue)!=0)
            pEncCfg->changePicParam.MindeltaQp = atoi(sValue);
        else
            pEncCfg->changePicParam.MindeltaQp = 0;

        if(GetValue(fp, "PARAM_CHANGE_HVS_QP_SCALE_DIV2", sValue) == 0)
            pEncCfg->changePicParam.HvsQpScaleDiv2 = 4;
        else
            pEncCfg->changePicParam.HvsQpScaleDiv2 = atoi(sValue);

        if(GetValue(fp, "PARAM_CHANGE_EN_HVS_QP", sValue) == 0)
            pEncCfg->changePicParam.EnHvsQp = 1;
        else
            pEncCfg->changePicParam.EnHvsQp = atoi(sValue);

        if(pEncCfg->changePicParam.EnHvsQp ==1 && pEncCfg->changePicParam.HvsQpScaleDiv2 == 0)
        {
            printf(" HVS_QP_SCALE_DIV2 is larger than zero");
            return 0;
        }

        if(GetValue(fp, "PARAM_CHANGE_ROW_LEVEL_RC", sValue) == 0)
            pEncCfg->changePicParam.EnRowLevelRc = 1;
        else
            pEncCfg->changePicParam.EnRowLevelRc = atoi(sValue);

        if(GetValue(fp,"PARAM_CHANGE_HVS_MAX_DELTA_QP", sValue) == 0)
            pEncCfg->changePicParam.RcHvsMaxDeltaQp = 10;
        else
            pEncCfg->changePicParam.RcHvsMaxDeltaQp = atoi(sValue);

        if (GetValue(fp, "PARAM_CHANGE_GAMMA_SET_ENABLE", sValue) ==0)
            pEncCfg->changePicParam.GammaEn = 0;
        else
            pEncCfg->changePicParam.GammaEn = atoi(sValue);

        if (GetValue(fp, "PARAM_CHANGE_GAMMA", sValue) == 0)
            pEncCfg->changePicParam.Gamma = 0;
        else
            pEncCfg->changePicParam.Gamma = atoi(sValue);

        if(pEncCfg->changePicParam.GammaEn == 0)
            pEncCfg->changePicParam.Gamma = -1;

        if(GetValue(fp, "PARAM_CHANGE_DELAY_IN_MS", sValue) == 0)
            pEncCfg->changePicParam.RcInitDelay = 1000;
        else
            pEncCfg->changePicParam.RcInitDelay = atoi(sValue);

        if (GetValue(fp, "PARAM_CHANGE_GOP_I_QP_OFFSET_EN", sValue) ==0) {
            pEncCfg->changePicParam.RcGopIQpOffsetEn = 0;
            pEncCfg->changePicParam.RcGopIQpOffset = 0;
        }
        else {
            pEncCfg->changePicParam.RcGopIQpOffsetEn = atoi(sValue);

            if (GetValue(fp, "PARAM_CHANGE_GOP_I_QP_OFFSET", sValue) == 0)
                pEncCfg->changePicParam.RcGopIQpOffset = 0;
            else
                pEncCfg->changePicParam.RcGopIQpOffset = atoi(sValue);

            if(pEncCfg->changePicParam.RcGopIQpOffset < -4)
                pEncCfg->changePicParam.RcGopIQpOffset = -4;
            else if(pEncCfg->changePicParam.RcGopIQpOffset > 4)
                pEncCfg->changePicParam.RcGopIQpOffset = 4;
        }

        if(GetValue(fp, "PARAM_CHANGE_SET_DPQ_PIC_NUM", sValue) == 0)
            pEncCfg->changePicParam.SetDqpNum = 1;
        else
            pEncCfg->changePicParam.SetDqpNum = atoi(sValue);

        if(pEncCfg->changePicParam.SetDqpNum == 0)
        {
            printf("PARAM_CHANGE_SET_DPQ_PIC_NUM must larger than 0");
            return 0;
        }
        if(pEncCfg->changePicParam.SetDqpNum > 1)
        {
            int i;
            char name[1024];
            for(i=0 ; i< pEncCfg->changePicParam.SetDqpNum; i++)
            {
                sprintf(name,"Frame%d", i);
                if(GetValue(fp, name, sValue) != 0)
                {
                    pEncCfg->changePicParam.dqp[i] = atoi(sValue);
                }
                else
                {
                    printf( "dqp Error");
                    return 0;
                }
            }
        }

        if(GetValue(fp, "PARAM_CHANGE_RC_WEIGHT_UPDATE_FACTOR", sValue) == 0)
            pEncCfg->changePicParam.rcWeightFactor = 1;
        else
            pEncCfg->changePicParam.rcWeightFactor = atoi(sValue);
    }

    /* Read next config for another param change, if exist */
    if (GetValue(fp, "CHANGE_PARAM_CONFIG_FILE", sValue)!=0)
    {
        strcpy(pEncCfg->pchChangeCfgFileName, sValue);
        if (GetValue(fp, "CHANGE_PARAM_FRAME_NUM", sValue)!=0)
            pEncCfg->ChangeFrameNum = atoi(sValue);
        else
            pEncCfg->ChangeFrameNum = 0;
    }
    else
    {
        /* Disable parameter change */
        strcpy(pEncCfg->pchChangeCfgFileName, "");
        pEncCfg->ChangeFrameNum = 0;
    }

    osal_fclose(fp);
    return 1;
}

