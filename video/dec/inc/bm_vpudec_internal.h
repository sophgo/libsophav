#ifndef BM_VIDEO_INTERNAL_H
#define BM_VIDEO_INTERNAL_H

#include <stdio.h>

typedef void* DecHandle;

#define VDEC_MAX_CHN_NUM_INF    64

typedef struct {
    FILE* stream_fp;
    int stream_count;
    int file_flag;
} dump_info_t;

typedef struct _BM_VDEC_CTX{
    int chn_fd;
    int chn_id;
    pthread_rwlock_t process_lock;
    dump_info_t dump_info;
} BM_VDEC_CTX;


typedef struct BMVidCodInst {
    DecHandle vdchn_id;
    unsigned int  soc_idx;
    BM_VDEC_CTX vpu_dec_chn;
//     vpu_buffer_t vbStream;
//     vpu_buffer_t vbUserData;
//     volatile Uint32 seqInitFlag;
//     volatile Uint32 isStreamBufFilled;
//     volatile BMDecStatus decStatus;
//     BMVidDecConfig decConfig;
//     Queue* ppuQ;
//     Queue* displayQ;
//     Queue* freeQ;
//     vpu_buffer_t        pFbMem[MAX_REG_FRAME];
//     vpu_buffer_t        pPPUFbMem[MAX_REG_FRAME];
//     vpu_buffer_t        pYtabMem[MAX_REG_FRAME];
//     vpu_buffer_t        pCtabMem[MAX_REG_FRAME];
//     volatile int endof_flag;
//     Queue* inputQ;
//     osal_cond_t inputCond;
//     osal_cond_t outputCond;
//     BOOL enablePPU;
//     volatile int frameInBuffer;
//     Uint32 sizeInWord;
//     Uint16 *pusBitCode;
//     Queue* inputQ2;
//     Queue* sequenceQ;
//     PhysicalAddress streamWrAddr;
// #ifdef __linux__
//     sem_t* vid_open_sem;
// #elif _WIN32
//     HANDLE* vid_open_sem;
// #endif
//     osal_thread_t processThread;
//     Uint32 remainedSize;
//     FrameBuffer pPPUFrame[MAX_REG_FRAME];
//     int PPUFrameNum;
//     int extraFrameBufferNum;
//     FILE *fp_stream;
//     int dump_frame_num;
//     int file_flag;
//     int no_reorder_flag;
//     int enable_cache;
//     BMVidExtraInfo extraInfo;
//     //bm_handle_t devHandles;
//     int64_t total_time;
//     int64_t max_time;
//     int64_t min_time;
//     int64_t dec_idx;
//     int perf;
//     BMVidFrame cache_bmframe[32];
//     int enable_decode_order;
//     int decode_index_map[MAX_REG_FRAME];
//     int timeout;
//     int timeout_count;
// 
//     int bitstream_from_user;
//     int framebuf_from_user;
//     int min_framebuf_cnt;
//     int framebuf_delay;
} BMVidCodInst;


typedef struct BMVidCodInst* BMVidHandle;


#endif//BM_VIDEO_INTERNAL_H
