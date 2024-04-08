//-----------------------------------------------------------------------------
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//                    (C) COPYRIGHT 2003 - 2020 CHIPS&MEDIA INC.
//                           ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//     copies.
//
// File         : com.h
// Author       : Jeff Oh
// Description  :
//
// Revision     : 0.9 11/1, 2020        initial
//-----------------------------------------------------------------------------

#ifndef _COM_H_
#define _COM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

//#define CNM_ENT_SIM
#ifdef CNM_ENT_SIM
extern void hw_trace_fio(char *file_name, char *format, ...);
#endif // CNM_ENT_SIM

//#define VCPU_STANDALONE

//[+]DONE
//#define HOST_MODEL_INT_MODE
//[-]DONE

#define HW_TRACE_MATCH_W_RTL // trace match w/ RTL output
//#define VP9_HW_PROB_UPDATE
//#define VP9_PROB_DIFF_OPT
//#define VP9_HW_PROB_FIO

//#define ERR_CONCEAL_CHECK

#ifdef ERR_CONCEAL_CHECK
extern int  g_ctu_dec_cnt[8192/16][8192/16];
#endif

//#define DISABLE_RUN_HW

//#define GAUDI_DEC_CODEC_FW_TRACE

#ifndef FW_BUILD
//#define GAUDI_HEVC_DEC_CODEC_FW_TRACE
#endif

//#define GAUDI_HEVC_DEC_DBG_TRACE

#define AV1_QMATRIX_WRITE

//#define VP9_TEMP_SUPPORT_RESOLUTION_CHANGE

#ifdef FW_BUILD
#define CRC32
#endif

#ifdef _WIN32
#define CU_RC_V2    1
#endif // _WIN32


//#define DUAL_ENC
#ifdef DUAL_ENC
#if defined SIM_UART || defined WIN32
#define DBG_DUAL
#endif
#endif

#define PIC_RC_V4

#define AV1_TILE_SPEC_CHECK_DISABLE

// Change Param Implement
#define CHANGE_PARAM_DEFINE                           (1)
#if CHANGE_PARAM_DEFINE
//#define SET_GOP_PARAM
#endif

#define NON_VCL
#ifdef NON_VCL
#define EOS_EOB
#define AUD
#define VUI_RBSP
#define HRD_RBSP
#define SEI_NAL
#define TIMING_INFO
#endif

// Default CDF Buffer Info
#define AV1_DEC_MAX_CDF_WORDS 1316
#define AV1_DEC_CDF_WORD_SIZE 16



//typedef int             BOOL;
typedef unsigned char   BYTE;
typedef unsigned short  UI16;
typedef unsigned int    UINT;
typedef unsigned short  PIXL;               // for 10 bit

#ifdef _WIN32
typedef          _int64      I64;
typedef unsigned _int64     UI64;
#define INLINE  _inline
#else
typedef          long long   I64;
typedef unsigned long long  UI64;
#define INLINE   inline
#endif

enum {
    CDF_NUM     = 328,
    MAX_TILE    = 16*2,                     // col 2, row 16
};
enum {
    MAX_DPB               = 17 + 1,
};

void print_msg(char *format, ...);
void print_err(char *format, ...);
void msg(char *format, ...);

static INLINE int I_ABS(int a)
{
    return (a > 0) ? a : -a;
}
static INLINE int I_MIN(int a, int b)
{
    return (a < b) ? a : b;
}
static INLINE int I_MAX(int a, int b)
{
    return (a > b) ? a : b;
}
static INLINE int I_CLIP3(int min, int max, int val)
{
    return (val < min) ? min :
           (val > max) ? max : val;
}
static INLINE int I_I64_CLIP3(int min, int max, I64 val)
{
    return (val < min) ? min :
        (val > max) ? max : (int)val;
}
static INLINE int I_ALIGN(int a, int align)
{
    return (a + align - 1) / align * align;
}
static INLINE int I_NUM_BIT(int a)
{
    int  num_bit;

    num_bit = 0;
    while (1) {
        if ((1 << num_bit) > a)
            break;
        num_bit++;
    }
    return num_bit;
}

typedef struct
{
#ifdef FW_BUILD
    int16_t blk32_luma[4][4];
    int16_t blk16_chroma[4][4];
    int16_t blk16_luma[4][4];
    int16_t blk8_chroma[4][4];
    int16_t blk8_luma[4][4];
    int16_t blk4_chroma[4][4];
    int16_t blk4_luma[4][4];
    int16_t blk4_chroma_nxn[4][4]; //nxn chroma has seperate transform
#else
    int blk4_luma[4][4];
    int blk8_luma[4][4];
    int blk16_luma[4][4];
    int blk32_luma[4][4];
    int blk4_chroma[4][4];
    int blk4_chroma_nxn[4][4]; //nxn chroma has seperate transform
    int blk8_chroma[4][4];
    int blk16_chroma[4][4];
#endif
} com_round_t;

typedef enum
{
    STREAM_FORMAT_IVF,
    STREAM_FORMAT_OBU,
    STREAM_FORMAT_AV1_ANNEXB,
    NUM_STREAM_FORMAT,
} stream_format_t;

enum {
    AR_TYPE_I16,
    AR_TYPE_I4,
    AR_TYPE_I8,
    AR_TYPE_CHROMA_INTRA,
    AR_TYPE_CHROMA_SKIPDIRECT,
    AR_TYPE_CHROMA_INTER16,
    AR_TYPE_CHROMA_INTER8,
    AR_TYPE_P4_SKIPDIRECT,
    AR_TYPE_P4_INTER16,
    AR_TYPE_P4_INTER8,
    AR_TYPE_P8_SKIPDIRECT,
    AR_TYPE_P8_INTER16,
    AR_TYPE_P8_INTER8,
    AR_TYPE_NUM
};

enum
{
    AR_SKIPDRIECT,
    AR_INTER16,
    AR_INTER8,
    AR_INTRA
};
enum {
    MB_SKIP = 0,    //p_skip or b_skip
    MB_DIRECT16x16 = 1,
    MB_P16x16 = 2,
    MB_P8x8 = 3,
    MB_I4x4 = 4,
    MB_I8x8 = 5,
    MB_I16x16 = 6,
    NUM_MB_MODE = 7
};

typedef struct {
#ifdef FW_BUILD
    short round[AR_TYPE_NUM][4][4];
#else
    int round[AR_TYPE_NUM][4][4];
#endif
} avc_round_t;


//---------------------------------------------------------------------------
// codec standard
//---------------------------------------------------------------------------
enum {
    STD_DEC = 0x00,
    STD_ENC = 0x01,
    STD_CODEC_MASK = 0x01,

    S_HEVC = 0x00,
    S_AVC = 0x02,
    S_VP9 = 0x16,
    S_AVS2 = 0x18,
    S_AV1 = 0x1A,
    S_SVAC = 0x20,
    S_UNKNOWN = 0xFF,

    ///using STD MAGIC NUM for UTEST
    STD_UTEST = 0xFF,

    STD_HEVC_DEC = S_HEVC | STD_DEC,
    STD_HEVC_ENC = S_HEVC | STD_ENC,
    STD_AVC_DEC = S_AVC | STD_DEC,
    STD_AVC_ENC = S_AVC | STD_ENC,
    STD_VP9_DEC = S_VP9 | STD_DEC,
    STD_VP9_ENC = S_VP9 | STD_ENC,
    STD_AVS2_DEC = S_AVS2 | STD_DEC,
    STD_AVS2_ENC = S_AVS2 | STD_ENC,
    STD_AV1_DEC = S_AV1 | STD_DEC,
    STD_AV1_ENC = S_AV1 | STD_ENC,
};

enum
{
    DEC_TEMP_ID_MODE                        = 0x80000,
    SEQ_CHNAGE_ENABLE_FLAG_BITDEPTH         = 0x40000,
    SEQ_CHANGE_ENABLE_FLAG_INTER_RES_CHANGE = 0x20000,
    SEQ_CHANGE_ENABLE_FLAG_SIZE             = 0x10000,
    SEQ_CHANGE_ENABLE_FLAG_CHROMA_FORMAT    = 0x08000,
    SEQ_CHANGE_ENABLE_FLAG_PROFILE          = 0x00020
};


enum
{
    SET_FB_SET_MODE = 0,
    SET_FB_UPDATE_MODE = 1,
    SET_FB_SCALER_MODE = 2,
};

// error definition
enum
{
    ERR_DECODER_FUSE                    = 0x00000002,
    ERR_INSTRUCTION_ACCESS_VIOLATION    = 0x00000004,
    ERR_PRIVILEDGE_VIOLATION            = 0x00000008,
    ERR_DATA_ADDR_ALIGNMENT             = 0x00000010,
    ERR_DATA_ACCESS_VIOLATION           = 0x00000020,
    ERR_GDI_ACCESS_VIOLATION            = 0x00000040,
    ERR_INSTRUCTION_ADDR_ALIGNMENT      = 0x00000080,
    ERR_UNKNOWN                         = 0x00000100,
    ERR_BUS_ERROR                       = 0x00000200,
    ERR_DOUBLE_FAULT                    = 0x00000400,
    ERR_RESULT_NOT_READY                = 0x00000800,
    ERR_FLUSH_FAIL                      = 0x00001000,
    ERR_UNKNOWN_CMD                     = 0x00002000,
    ERR_UNKNOWN_CODEC_STD               = 0x00004000,
    ERR_TIMEOUT                         = 0x00020000,
    ERR_TEMP_SEC_BUF_OVERFLOW           = 0x00200000,
    ERR_TIMEOUT_CODEC_FW                = 0x40000000,
    ERR_SLEEP_FAIL                      = 0x80000000,
    ERR_HOST_SIDE_HW_RESET              = 0xF0000000,

};

enum intra_refresh_mode
{
    INTRA_REFRESH_NONE = 0,
    INTRA_REFRESH_ROW = 1,
    INTRA_REFRESH_COLUMN = 2,
};
enum {
    PSEUDO_MODE = 0,
    INTER_MODE = 1,
    MERGE_MODE = 2,
    INTRA_MODE = 3,
    NUM_RDO_CAND = 4
};
// log generator
void msg(char* str, ...);
void msg_s(char* str);

void delay(int tick);
#ifdef FW_BUILD

#define print_critical_msg(str)  do    \
{                                           \
    int lr;                                 \
    msg(str);                  \
    __asm__ __volatile__ (                  \
        "push %%lr \n"                      \
        "pop  %0   \n"                      \
        : "=r"(lr)                          \
    );                                      \
    msg("%s(%d) - fail, LR: %08x\r\n",      \
                __func__, __LINE__, lr);    \
                                            \
    __asm__ __volatile__ ("halt 1");    \
} while(0)
#else

#define print_critical_msg(str)  do    \
{                                           \
    printf("%s(%d): %s\n",              \
        __FUNCTION__, __LINE__, str);    \
    exit(1);                               \
} while(0)

#endif

#endif  // _COM_H_

