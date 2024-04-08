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
// File         : host_dec.h
// Author       : Jeff Oh
// Description  : 
//
// Revision     : 0.9 11/1, 2020        initial
//-----------------------------------------------------------------------------

#ifndef _HOST_DEC_H_
#define _HOST_DEC_H_

typedef struct {
    //common
    int host_std_idx;
    int stream_format; //VP9 : ivf, AV1 : obu, annexb, ivf
    int init_flag;
    int end_of_stream;    
    //av1 parsing
    int data;
    int remain_bits;
    int remain_size;
    int temporal_id;
    int spatial_id;
    int reduced_still_pic_header;
    int operatingPointIdc;
    //vp9 parsing
    int vp9_superframe_en;
    int vp9_superframe_idx;
    int vp9_superframe_num;
    int vp9_superframe_size[8];
    int consume_superframe_header_flag;
    int super_frame_header_size;
} host_dec_internal_data;

typedef struct {
    int  pic_size_x;
    int  pic_size_y;
    int  bit_depth;
} host_dec_seq_info_t;

typedef struct {
    int addr_base;
    int size;
    int rd_ptr;
    int wr_ptr;
    int bb_end_ptr;

    int explicit_end_flag;   // Stream size == 1 AU
    int stream_end_flag;     // Host check the end of bitstream 
    int mode;
    int stream_format;
} cpb_t;

typedef enum
{
    INT_MODE,
    NAL_END_MODE,
    PIC_END_MODE,
    NAL_INTRV_MODE,
} stream_mode_t;

enum {
    // Too big memory allocated
#ifdef  GAUDI_HEVC_DEC_CODEC_FW_TRACE
    DEC_DRAM_SIZE = 1200 * 1024 * 1024,       // 1GB
#else //GAUDI_HEVC_DEC_CODEC_FW_TRACE
#ifdef  __linux__
    DEC_DRAM_SIZE = 0x7FFFFFFF, // 2GB
#else //__linux__
    DEC_DRAM_SIZE = 1024 * 1024 * 1024,       // 1GB
#endif//__linux__
#endif//GAUDI_HEVC_DEC_CODEC_FW_TRACE
	ENC_DRAM_SIZE       = 128 * 10 * 1024 * 1024,           // 1024 MB
    CODE_SIZE       =     2 * 1024 * 1024, //    2 MB
    TEMP_SIZE       =    10 * 1024 * 1024, //   10 MB
    ENC_WORK_SIZE   =   128        * 1024, //  128 KB
#ifdef GAUDI_HEVC_DEC_CODEC_FW_TRACE
    DEC_WORK_SIZE   =     20  * 1024 * 1024, //   4 MB  Only for C HOST model
#else
    DEC_WORK_SIZE   =     4  * 1024 * 1024 * 4, //    4 MB
#endif

    CPB_SIZE        =    64 * 1024 * 1024, //   64 MB
    CDF_SIZE        =    12        * 1024, // Total(CDF Table 4)  4 * 12KB, CDF TAble 1 size => (329 * 16 * 2byte = 10528 byte )
    Q_MATRIX_SIZE   =    53        * 1024, // Total(Q Matrix 16) 16 * 3344 = 53504 byte, Q Matrix 1 size => 3344byte
};

int host_dec_start_code(host_dec_internal_data* dec_data, BYTE frame_cpb_buf[], int base_addr_cpb, FILE *fp);

#endif  // _HOST_DEC_H_

