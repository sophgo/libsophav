/*****************************************************************************
 *
 *    Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
 *
 *    bmvid is licensed under the 2-Clause BSD License except for the
 *    third-party components.
 *
 *****************************************************************************/
/* This library provides a high-level interface for controlling the BitMain
 * Sophon VPU en/decoder.
 */


#ifdef _WIN32
#include <windows.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include  < MMSystem.h >
#pragma comment(lib, "winmm.lib")
#elif __linux__
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <semaphore.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h> // gettimeofday()
#include <fcntl.h>
#include <time.h>
#endif
#include <stdint.h>
#include <string.h>

#include "bm_vpudec_interface.h"
#include "bm_ioctl.h"
#include "bmlib_runtime.h"
#include "linux/comm_buffer.h"

//for dump frame for debuging lasi..
#ifndef BM_PCIE_MODE
#ifdef __linux__
//static __thread int dump_frame_num = 0;
#elif _WIN32
static  __declspec(thread) int dump_frame_num = 0;
#endif
#endif


#define VDEC_MAX_CHN_NUM_INF    64
#define SOPH_VC_DRV_DECODER_DEV_NAME "soph_vc_dec"

typedef struct _BM_VDEC_CTX{
    int is_used;
    int chn_fd;
    int chn_id;
    pthread_rwlock_t process_lock;
} BM_VDEC_CTX;

BM_VDEC_CTX vpu_dec_chn[VDEC_MAX_CHN_NUM_INF] = {0};
static unsigned int u32ChannelCreatedCnt = 0;
static pthread_mutex_t VdecChn_Mutex = PTHREAD_MUTEX_INITIALIZER;

void bmvpu_dec_set_logging_threshold(BmVpuDecLogLevel log_level)
{
    bm_vpu_set_logging_threshold(log_level);
}

BMVidDecRetStatus bmvpu_dec_dump_stream(BMVidCodHandle vidCodHandle, unsigned char *p_stream, int size)
{
    static int stream_count = 0;
    static int file_flag = 0;
    int core_idx, inst_idx;
    char filename[128];
    static FILE* stream_fp = NULL;

    if(stream_fp == NULL) {
        core_idx = bmvpu_dec_get_core_idx(vidCodHandle);
        inst_idx = bmvpu_dec_get_inst_idx(vidCodHandle);
        sprintf(filename, "core%d_inst%d_ref_stream%d.bin", core_idx, inst_idx, file_flag);

        stream_fp = fopen(filename, "wb");
        if(stream_fp == NULL) {
            BMVPU_DEC_ERROR("can not open dump file.\n");
            return BM_ERR_VDEC_FAILURE;
        }
    }

    if(stream_fp != NULL) {
        fwrite(p_stream, 1, size, stream_fp);
        stream_count += 1;
    }

    if(stream_count == atoi(getenv("BMVPU_DEC_DUMP_NUM"))) {
        fclose(stream_fp);
        stream_fp = NULL;
        stream_count = 0;
        file_flag = 1 - file_flag;
    }

    return BM_SUCCESS;
}

int bmvpu_dec_get_core_idx(BMVidCodHandle vidCodHandle){
    int ret;
    int coreIdx = 0;
    int VdChn;
    vdec_chn_status_s stDecStatus = {0};

    if(vidCodHandle == NULL)
    {
        BMVPU_DEC_ERROR("invalid vdec handle.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].is_used != 1)
    {
        BMVPU_DEC_ERROR("invalid vdec chn.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    pthread_rwlock_rdlock(&vpu_dec_chn[VdChn].process_lock);
    if(vpu_dec_chn[VdChn].chn_fd < 0){
        BMVPU_DEC_ERROR("Vdec device fd error.");
        pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
        return BM_ERR_VDEC_FAILURE;
    }

    ret = bmdec_ioctl_query_chn_status(vpu_dec_chn[VdChn].chn_fd, &stDecStatus);
    if(ret != BM_SUCCESS){
        BMVPU_DEC_ERROR("Vdec query channel status failed. error: %d", ret);
        pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
        return ret;
    }
    coreIdx = stDecStatus.stSeqinitalInfo.u8CoreIdx;

    pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
    return coreIdx;
}

int bmvpu_dec_get_inst_idx(BMVidCodHandle vidCodHandle)
{
    int VdChn;

    if(vidCodHandle == NULL)
    {
        BMVPU_DEC_ERROR("invalid vdec handle.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].is_used != 1)
    {
        BMVPU_DEC_ERROR("invalid vdec chn.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    return VdChn;
}
#ifdef BM_PCIE_MODE
BMVidDecRetStatus bmvpu_dec_read_memory(BMVidCodHandle vidCodHandle, u64 src_addr, u64 dst_addr, int size)
{
    return 0;
}
#endif

int bmvpu_dec_get_device_fd(BMVidCodHandle vidCodHandle)
{
    int VdChn;

    if(vidCodHandle == NULL)
    {
        BMVPU_DEC_ERROR("invalid vdec handle.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].is_used != 1)
    {
        BMVPU_DEC_ERROR("invalid vdec chn.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    return vpu_dec_chn[VdChn].chn_fd;
}

/**
 * @brief This function create a decoder instance.
 * @param pop [Input] this is open decoder parameter
 * @param pHandle [Output] decoder instance.
 * @return error code.
 */
BMVidDecRetStatus bmvpu_dec_create(BMVidCodHandle *pVidCodHandle, BMVidDecParam decParam)
{
    int ret = 0;
    int VdChn_id = -1;
    int device_fd = 0;
    int is_jpu = 0;
    char devName[255];

    vdec_chn_attr_s stAttr = {0};
    vdec_chn_param_s stChnParam;

    BMVPU_DEC_TRACE("enter bmvpu_dec_create\n");

    bm_vpu_set_logging_threshold(BMVPU_DEC_LOG_LEVEL_INFO);

    if(decParam.streamFormat == BMDEC_AVC)
        stAttr.enType = PT_H264;
    else if(decParam.streamFormat == BMDEC_HEVC)
        stAttr.enType = PT_H265;
    else {
        BMVPU_DEC_ERROR("init dec error: invalid streamFormat\n");
        return BM_ERR_VDEC_FAILURE;
    }
    stAttr.u32StreamBufSize = (decParam.streamBufferSize == 0) ? STREAM_BUF_SIZE : decParam.streamBufferSize;
    stAttr.enMode = (decParam.bsMode == BMDEC_BS_MODE_INTERRUPT) ? VIDEO_MODE_STREAM : VIDEO_MODE_FRAME;
    stAttr.u32FrameBufCnt = (decParam.extraFrameBufferNum >= 0) ? decParam.extraFrameBufferNum : 2;
    stAttr.enCompressMode = (decParam.wtlFormat == BMDEC_OUTPUT_COMPRESSED) ? COMPRESS_MODE_FRAME : COMPRESS_MODE_NONE;
    stAttr.u8CommandQueueDepth = decParam.cmd_queue_depth;
    if (decParam.decode_order)
        stAttr.u8ReorderEnable = 0;
    else
        stAttr.u8ReorderEnable = 1; //default
    if(decParam.picWidth != 0 && decParam.picHeight != 0) {
        stAttr.u32PicWidth = decParam.picWidth;
        stAttr.u32PicHeight = decParam.picHeight;
    }

    if(decParam.bitstream_buffer != NULL) {
        if(stAttr.u8CommandQueueDepth <= 0) {
            BMVPU_DEC_ERROR("Invalid command queue depth: %d\n", stAttr.u8CommandQueueDepth);
            return BM_ERR_VDEC_ILLEGAL_PARAM;
        }
        stAttr.stBufferInfo.bitstream_buffer = (buffer_info_s *)decParam.bitstream_buffer;
    }
    if(decParam.frame_buffer != NULL && decParam.Ytable_buffer != NULL && decParam.Ctable_buffer != NULL) {
        if(decParam.extraFrameBufferNum <= 0 || decParam.min_framebuf_cnt <= 0 ||
            decParam.framebuf_delay < 0 || decParam.cmd_queue_depth <= 0 ) {
            BMVPU_DEC_ERROR("Invalid frame buffer count: extra frame buffer:%d mini frame buffer:%d frame delay:%d command queue depth:%d\n",
                decParam.extraFrameBufferNum, decParam.min_framebuf_cnt, decParam.framebuf_delay, decParam.cmd_queue_depth);
            return BM_ERR_VDEC_ILLEGAL_PARAM;
        }
        if(decParam.wtlFormat != BMDEC_OUTPUT_COMPRESSED)
            stAttr.stBufferInfo.numOfDecwtl = decParam.framebuf_delay + decParam.extraFrameBufferNum + decParam.cmd_queue_depth;
        stAttr.stBufferInfo.numOfDecFbc = decParam.min_framebuf_cnt + decParam.extraFrameBufferNum + decParam.cmd_queue_depth;
        stAttr.stBufferInfo.frame_buffer = (buffer_info_s *)decParam.frame_buffer;
        stAttr.stBufferInfo.Ytable_buffer = (buffer_info_s *)decParam.Ytable_buffer;
        stAttr.stBufferInfo.Ctable_buffer = (buffer_info_s *)decParam.Ctable_buffer;
    }

    if(getenv("NO_FRAMEBUFFER")!=NULL && strcmp(getenv("NO_FRAMEBUFFER"),"1")==0)
    {
        BMVPU_DEC_INFO("Capture the environment variable <NO_FRAMEBUFFER>!\n");
        stAttr.u8ReorderEnable = 0;
    }

    pthread_mutex_lock(&VdecChn_Mutex);
    /* create the device fd */
    sprintf(devName, "/dev/%s", SOPH_VC_DRV_DECODER_DEV_NAME);
    device_fd = bmdec_chn_open(devName, 0);
    if(device_fd <= 0) {
        ret = BM_ERR_VDEC_FAILURE;
        goto ERR_DEC_INIT2;
    }

    /* get the chn id */
    ret = bmdec_ioctl_get_chn(device_fd, &is_jpu, &VdChn_id);
    if(ret != BM_SUCCESS) {
        BMVPU_DEC_ERROR("init dec error: get dec chn id failed.\n");
        close(device_fd);
        goto ERR_DEC_INIT2;
    }
    if(VdChn_id >= 2*VC_MAX_CHN_NUM) {
        BMVPU_DEC_ERROR("init dec error: dec open is more than the max ctreate num(%d).\n", VDEC_MAX_CHN_NUM_INF);
        close(device_fd);
        goto ERR_DEC_INIT2;
    }

    ret = bmdec_ioctl_set_chn(device_fd, &VdChn_id);
    if (ret != 0) {
        bmdec_chn_close(device_fd);
        BMVPU_DEC_ERROR("set chn id  %d failed\n", VdChn_id);
        goto ERR_DEC_INIT2;
    }

    if(vpu_dec_chn[VdChn_id].is_used == 1) {
        BMVPU_DEC_ERROR("init dec error: the chn id %d is occupied.\n", VdChn_id);
        close(device_fd);
        goto ERR_DEC_INIT2;
    }

    if (pthread_rwlock_init(&vpu_dec_chn[VdChn_id].process_lock, NULL) != 0) {
        BMVPU_DEC_ERROR("pthread_rwlock_init error");
        close(device_fd);
        goto ERR_DEC_INIT2;
    }

    pthread_rwlock_wrlock(&vpu_dec_chn[VdChn_id].process_lock);
    vpu_dec_chn[VdChn_id].chn_fd = device_fd;
    vpu_dec_chn[VdChn_id].chn_id = VdChn_id;
    vpu_dec_chn[VdChn_id].is_used = 1;
    *pVidCodHandle = (BMVidCodHandle)&vpu_dec_chn[VdChn_id].chn_id;
    u32ChannelCreatedCnt += 1;

    ret = bmdec_ioctl_create_chn(vpu_dec_chn[VdChn_id].chn_fd, &stAttr);
    if (ret != BM_SUCCESS) {
        BMVPU_DEC_ERROR("ioctl CVI_VC_VDEC_CREATE_CHN fail with %d\n", ret);
        close(vpu_dec_chn[VdChn_id].chn_fd);
        u32ChannelCreatedCnt -= 1;
        goto ERR_DEC_INIT;
    }

    bmdec_ioctl_get_chn_param(vpu_dec_chn[VdChn_id].chn_fd, &stChnParam);
    if(decParam.pixel_format == BM_VPU_DEC_PIX_FORMAT_NV12)
        stChnParam.enPixelFormat = PIXEL_FORMAT_NV12;
    else if(decParam.pixel_format == BM_VPU_DEC_PIX_FORMAT_NV21)
        stChnParam.enPixelFormat = PIXEL_FORMAT_NV21;
    else
        stChnParam.enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;
    bmdec_ioctl_set_chn_param(vpu_dec_chn[VdChn_id].chn_fd, &stChnParam);

    ret = bmdec_ioctl_start_recv_stream(vpu_dec_chn[VdChn_id].chn_fd);
    if (ret != BM_SUCCESS) {
        BMVPU_DEC_ERROR("CVI_VDEC_StartRecvStream failed. ret = %d", ret);
        ret = BM_ERR_VDEC_FAILURE;
        bmdec_ioctl_destory_chn(vpu_dec_chn[VdChn_id].chn_fd);
        close(vpu_dec_chn[VdChn_id].chn_fd);
        u32ChannelCreatedCnt -= 1;
        goto ERR_DEC_INIT;
    }

    pthread_rwlock_unlock(&vpu_dec_chn[VdChn_id].process_lock);
    pthread_mutex_unlock(&VdecChn_Mutex);
    return ret;

ERR_DEC_INIT:
    vpu_dec_chn[VdChn_id].chn_fd = -1;
    vpu_dec_chn[VdChn_id].chn_id = 0;
    vpu_dec_chn[VdChn_id].is_used = 0;
    *pVidCodHandle = NULL;
    pthread_rwlock_unlock(&vpu_dec_chn[VdChn_id].process_lock);
ERR_DEC_INIT2:
    pthread_mutex_unlock(&VdecChn_Mutex);
    if(u32ChannelCreatedCnt == 0)
        bmdec_chn_close(0);

    return ret;
}


BMVidDecRetStatus bmvpu_dec_decode(BMVidCodHandle vidCodHandle, BMVidStream vidStream)
{
    int ret = BM_SUCCESS;
    char *dump_num;
    vdec_stream_ex_s stStreamEx;
    vdec_stream_s stStream;
    vdec_chn_attr_s stAttr;
    vdec_chn_status_s stDecStatus = {0};
    uint8_t *total_buf = NULL;
    int total_size;

    int VdChn;

    if(vidCodHandle == NULL)
    {
        BMVPU_DEC_ERROR("invalid vdec handle.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].is_used != 1)
    {
        BMVPU_DEC_ERROR("invalid vdec chn.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    pthread_rwlock_rdlock(&vpu_dec_chn[VdChn].process_lock);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        ret = BM_ERR_VDEC_INVALID_CHNID;
        goto RET;
    }

    BMVPU_DEC_TRACE("enter bmvpu_dec_decode\n");

    ret = bmdec_ioctl_query_chn_status(vpu_dec_chn[VdChn].chn_fd, &stDecStatus);
    if(ret != BM_SUCCESS)
    {
        BMVPU_DEC_ERROR("Vdec query channel status failed.");
        goto RET;
    }

    if(stDecStatus.u8Status == SEQ_CHANGE){
        ret = BM_ERR_VDEC_SEQ_CHANGE;
        goto RET;
    }

    ret = bmdec_ioctl_get_chn_attr(vpu_dec_chn[VdChn].chn_fd, &stAttr);
    if(ret != BM_SUCCESS){
        BMVPU_DEC_ERROR("VDEC_GET_CHN_ATTR error. ret = %d");
        goto RET;
    }

    if(stAttr.enMode == VIDEO_MODE_FRAME){
        stStream.bEndOfFrame = 1;
        stStream.bDisplay = 1;
    }
    else{
        stStream.bEndOfFrame = 0;
    }

    if(vidStream.header_size != 0 && vidStream.header_buf != NULL) {
        total_size = vidStream.header_size + vidStream.length;
        total_buf = (uint8_t*)malloc(total_size);
        if(total_buf == NULL) {
            BMVPU_DEC_ERROR("malloc input buffer error.");
            ret = BM_ERR_VDEC_NOBUF;
            goto RET;
        }

        memcpy(total_buf, vidStream.header_buf, vidStream.header_size);
        memcpy(total_buf + vidStream.header_size, vidStream.buf, vidStream.length);

        stStream.pu8Addr = total_buf;
        stStream.u32Len = total_size;
    }
    else {
        stStream.pu8Addr = vidStream.buf;
        stStream.u32Len = vidStream.length;
    }
    stStream.bEndOfStream = vidStream.end_of_stream;
    stStreamEx.pstStream = &stStream;
    stStreamEx.s32MilliSec = -1;
    stStream.u64DTS = vidStream.dts;
    stStream.u64PTS = vidStream.pts;

    /* send bitstream and decode */
    ret = bmdec_ioctl_send_stream(vpu_dec_chn[VdChn].chn_fd, &stStreamEx);
    if(ret == BM_ERR_VDEC_ILLEGAL_PARAM)
        BMVPU_DEC_ERROR("bmdec_ioctl_send_stream failed. REASON:%d\n", ret);

    if(total_buf != NULL) {
        free(total_buf);
        total_buf = NULL;
    }

    if((dump_num = getenv("BMVPU_DEC_DUMP_NUM")) != NULL) {
        if(vidStream.header_buf != NULL && vidStream.header_size != 0)
            bmvpu_dec_dump_stream(vidCodHandle, vidStream.header_buf, vidStream.header_size);
        bmvpu_dec_dump_stream(vidCodHandle, vidStream.buf, vidStream.length);
    }

RET:
    pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
    return ret;
}

BMVidDecRetStatus bmvpu_dec_get_output(BMVidCodHandle vidCodHandle, BMVidFrame *bmFrame)
{
    int ret;
    int dump_num;
    int VdChn;

    if(vidCodHandle == NULL)
    {
        BMVPU_DEC_ERROR("invalid vdec handle.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].is_used != 1)
    {
        BMVPU_DEC_ERROR("invalid vdec chn.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    pthread_rwlock_rdlock(&vpu_dec_chn[VdChn].process_lock);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        ret = BM_ERR_VDEC_INVALID_CHNID;
        goto RET;
    }

    if(bmFrame == NULL) {
        BMVPU_DEC_ERROR("the frame buffer invalid");
        ret = BM_ERR_VDEC_NOBUF;
        goto RET;
    }

    BMVPU_DEC_TRACE("enter bmvpu_dec_get_output");

    video_frame_info_s stFrameInfo = {0};
    video_frame_info_ex_s stFrameInfoEx = {0};
    vdec_chn_status_s stChnStatus = {0};
    stFrameInfoEx.pstFrame = &stFrameInfo;
    stFrameInfoEx.s32MilliSec = 0;

    ret = bmdec_ioctl_get_frame(vpu_dec_chn[VdChn].chn_fd, &stFrameInfoEx, &stChnStatus);
    if (ret != BM_SUCCESS){
        BMVPU_DEC_TRACE("get frame failed ret=%d\n", ret);
        goto RET;
    }
    else
    BMVPU_DEC_TRACE("get frame success");

    if(stFrameInfo.video_frame.pixel_format == PIXEL_FORMAT_NV12)
        bmFrame->pixel_format = BM_VPU_DEC_PIX_FORMAT_NV12;
    else if(stFrameInfo.video_frame.pixel_format == PIXEL_FORMAT_NV21)
        bmFrame->pixel_format = BM_VPU_DEC_PIX_FORMAT_NV21;
    else if(stFrameInfo.video_frame.compress_mode == COMPRESS_MODE_FRAME)
        bmFrame->pixel_format = BM_VPU_DEC_PIX_FORMAT_COMPRESSED;
    else
        bmFrame->pixel_format = BM_VPU_DEC_PIX_FORMAT_YUV420P;

    /* BMVidFrame.buf
    < 0: Y virt addr, 1: Cb virt addr: 2, Cr virt addr. 4: Y phy addr, 5: Cb phy addr, 6: Cr phy addr */
    bmFrame->buf[0]   = stFrameInfo.video_frame.viraddr[0];
    bmFrame->buf[1]   = stFrameInfo.video_frame.viraddr[1];
    bmFrame->buf[2]   = stFrameInfo.video_frame.viraddr[2];
    bmFrame->buf[4]   = (unsigned char *)stFrameInfo.video_frame.phyaddr[0];
    bmFrame->buf[5]   = (unsigned char *)stFrameInfo.video_frame.phyaddr[1];
    bmFrame->buf[6]   = (unsigned char *)stFrameInfo.video_frame.phyaddr[2];

    bmFrame->stride[0] = stFrameInfo.video_frame.stride[0];
    bmFrame->stride[4] = stFrameInfo.video_frame.stride[0];
    bmFrame->stride[1] = stFrameInfo.video_frame.stride[1];
    bmFrame->stride[5] = stFrameInfo.video_frame.stride[1];
    if(stFrameInfo.video_frame.compress_mode != COMPRESS_MODE_FRAME) {
        bmFrame->stride[2] = stFrameInfo.video_frame.stride[2];
        bmFrame->stride[6] = stFrameInfo.video_frame.stride[2];
    }
    else {
        bmFrame->buf[3] = stFrameInfo.video_frame.ext_virt_addr;
        bmFrame->buf[7] = (unsigned char *)stFrameInfo.video_frame.ext_phy_addr;

        bmFrame->stride[2] = stFrameInfo.video_frame.length[2];
        bmFrame->stride[6] = stFrameInfo.video_frame.length[2];
        bmFrame->stride[3] = stFrameInfo.video_frame.ext_length;
        bmFrame->stride[7] = stFrameInfo.video_frame.ext_length;
    }

    bmFrame->width    = stFrameInfo.video_frame.width;
    bmFrame->height   = stFrameInfo.video_frame.height;
    bmFrame->frameIdx = (uintptr_t)stFrameInfo.video_frame.private_data;
    bmFrame->pts      = stFrameInfo.video_frame.pts;
    bmFrame->dts      = stFrameInfo.video_frame.dts;
    bmFrame->endian   = stFrameInfo.video_frame.endian;
    bmFrame->sequenceNo = stFrameInfo.video_frame.seqenceno;
    bmFrame->picType  = stFrameInfo.video_frame.pic_type;
    bmFrame->interlacedFrame = stFrameInfo.video_frame.interl_aced_frame;
    bmFrame->size = (stFrameInfo.video_frame.phyaddr[1] - stFrameInfo.video_frame.phyaddr[0]) * 3 / 2;
    bmFrame->lumaBitDepth = stChnStatus.stSeqinitalInfo.s32LumaBitdepth;
    bmFrame->chromaBitDepth = stChnStatus.stSeqinitalInfo.s32ChromaBitdepth;
    bmFrame->coded_width    = stChnStatus.stSeqinitalInfo.s32PicWidth;
    bmFrame->coded_height   = stChnStatus.stSeqinitalInfo.s32PicHeight;

    // frame->colorPrimaries = stFrameInfo.stVFrame.
    // frame->colorTransferCharacteristic = stFrameInfo.stVFrame.
    // frame->colorSpace = stFrameInfo.stVFrame.
    // frame->colorRange = stFrameInfo.stVFrame.
    // frame->chromaLocation = stFrameInfo.stVFrame.

    if(stFrameInfo.video_frame.compress_mode == COMPRESS_MODE_FRAME){
        if(getenv("BMVPU_DEC_DUMP_FBC_NUM") != NULL) {
            int core_num = bmvpu_dec_get_core_idx(vidCodHandle);
            int inst_num = bmvpu_dec_get_inst_idx(vidCodHandle);

            dump_num = atoi(getenv("BMVPU_DEC_DUMP_FBC_NUM"));
            static int dump_cnt[64] = {0};
            if(dump_cnt[inst_num] >= dump_num) {
                ret = BM_SUCCESS;
                goto RET;
            }

            FILE* fbc_fp[4] = {0};
            char fbc_filename[4][128] = {0};

            sprintf(fbc_filename[0], "core%d_inst%d_frame%d_Y", core_num, inst_num, dump_cnt[inst_num]);
            sprintf(fbc_filename[1], "core%d_inst%d_frame%d_Cb", core_num, inst_num, dump_cnt[inst_num]);
            sprintf(fbc_filename[2], "core%d_inst%d_frame%d_YTb", core_num, inst_num, dump_cnt[inst_num]);
            sprintf(fbc_filename[3], "core%d_inst%d_frame%d_CbTb", core_num, inst_num, dump_cnt[inst_num]);

            for(int i=0; i<4; i++) {
                fbc_fp[i] = fopen(fbc_filename[i], "wb+");
                if(fbc_fp[i] == NULL) {
                    BMVPU_DEC_ERROR("open fbc dump file failed");
                    break;
                }

                if(i == 0)
                    fwrite(bmFrame->buf[i], 1, bmFrame->stride[i]*bmFrame->height, fbc_fp[i]);
                else if(i == 1)
                    fwrite(bmFrame->buf[i], 1, bmFrame->stride[i]*bmFrame->height/2, fbc_fp[i]);
                else
                    fwrite(bmFrame->buf[i], 1, bmFrame->stride[i], fbc_fp[i]);
                fclose(fbc_fp[i]);
            }
            dump_cnt[inst_num] += 1;
        }
    }

RET:
    pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
    return ret;
}

BMVidDecRetStatus bmvpu_dec_clear_output(BMVidCodHandle vidCodHandle, BMVidFrame *frame)
{
    int ret;
    int VdChn;
    video_frame_info_s stFrameInfo = {0};

    if(vidCodHandle == NULL)
    {
        BMVPU_DEC_ERROR("invalid vdec handle.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].is_used != 1)
    {
        BMVPU_DEC_ERROR("invalid vdec chn.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    pthread_rwlock_rdlock(&vpu_dec_chn[VdChn].process_lock);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    BMVPU_DEC_TRACE("enter bmvpu_dec_clear_output\n");

    stFrameInfo.video_frame.width        = frame->width;
    stFrameInfo.video_frame.height       = frame->height;
    stFrameInfo.video_frame.stride[0]    = frame->stride[0];
    stFrameInfo.video_frame.stride[1]    = frame->stride[1];
    stFrameInfo.video_frame.stride[2]    = frame->stride[2];

    stFrameInfo.video_frame.viraddr[0]   = frame->buf[0];
    stFrameInfo.video_frame.viraddr[1]   = frame->buf[1];
    stFrameInfo.video_frame.viraddr[2]   = frame->buf[2];
    stFrameInfo.video_frame.phyaddr[0]   = (long long unsigned int)frame->buf[4];
    stFrameInfo.video_frame.phyaddr[1]   = (long long unsigned int)frame->buf[5];
    stFrameInfo.video_frame.phyaddr[2]   = (long long unsigned int)frame->buf[6];

    if(frame->pixel_format == BM_VPU_DEC_PIX_FORMAT_COMPRESSED)
        stFrameInfo.video_frame.compress_mode = COMPRESS_MODE_FRAME;
    else
        stFrameInfo.video_frame.compress_mode = COMPRESS_MODE_NONE;
    stFrameInfo.video_frame.private_data  = (void *)(uintptr_t)frame->frameIdx;

    stFrameInfo.video_frame.length[0]   = frame->stride[0] * frame->height;
    stFrameInfo.video_frame.length[1]   = frame->stride[1] * frame->height / 2;
    if(stFrameInfo.video_frame.compress_mode != COMPRESS_MODE_FRAME) {
        stFrameInfo.video_frame.length[2]   = frame->stride[2] * frame->height / 2;
    }
    else{
        stFrameInfo.video_frame.length[2]      = frame->stride[2];
        stFrameInfo.video_frame.ext_length      = frame->stride[3];

        stFrameInfo.video_frame.ext_virt_addr    = frame->buf[3];
        stFrameInfo.video_frame.ext_phy_addr     = (long long unsigned int)frame->buf[7];
    }

    if(frame->pixel_format == BM_VPU_DEC_PIX_FORMAT_YUV420P)
        stFrameInfo.video_frame.pixel_format = PIXEL_FORMAT_YUV_PLANAR_420;
    else if(frame->pixel_format == BM_VPU_DEC_PIX_FORMAT_NV21)
        stFrameInfo.video_frame.pixel_format = PIXEL_FORMAT_NV21;
    else
        stFrameInfo.video_frame.pixel_format = PIXEL_FORMAT_NV12;

    ret = bmdec_ioctl_release_frame(vpu_dec_chn[VdChn].chn_fd, &stFrameInfo, frame->size);
    if(ret != BM_SUCCESS) {
        BMVPU_DEC_ERROR("realease frame failed %d.\n", ret);
    }

    pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
    return ret;
}


BMVidDecRetStatus bmvpu_dec_flush(BMVidCodHandle vidCodHandle)
{
    int ret;
    int VdChn;

    if(vidCodHandle == NULL)
    {
        BMVPU_DEC_ERROR("invalid vdec handle.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].is_used != 1)
    {
        BMVPU_DEC_ERROR("invalid vdec chn.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    vdec_stream_s stStream;
    vdec_stream_ex_s stStreamEx;

    memset(&stStream, 0 ,sizeof(vdec_stream_s));
    stStream.bEndOfStream = 1;
    stStreamEx.pstStream = &stStream;
    stStreamEx.s32MilliSec = -1;
    while(1)
    {
        pthread_rwlock_rdlock(&vpu_dec_chn[VdChn].process_lock);
        if(vpu_dec_chn[VdChn].chn_fd < 0)
        {
            BMVPU_DEC_ERROR("Vdec device fd error.");
            pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
            return BM_ERR_VDEC_INVALID_CHNID;
        }

        ret = bmdec_ioctl_send_stream(vpu_dec_chn[VdChn].chn_fd, &stStreamEx);
        pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
        if(ret == BM_SUCCESS || ret == BM_ERR_VDEC_ILLEGAL_PARAM)
            break;
        usleep(1000);
    }

    return ret;
}

BMVidDecRetStatus bmvpu_dec_delete(BMVidCodHandle vidCodHandle)
{
    int ret;
    int VdChn;

    if(vidCodHandle == NULL)
    {
        BMVPU_DEC_ERROR("invalid vdec handle.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].is_used != 1)
    {
        BMVPU_DEC_ERROR("invalid vdec chn.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    pthread_mutex_lock(&VdecChn_Mutex);
    pthread_rwlock_wrlock(&vpu_dec_chn[VdChn].process_lock);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        ret = BM_ERR_VDEC_INVALID_CHNID;
        goto ERR_RET;
    }

    BMVPU_DEC_TRACE("enter bmvpu_dec_delete chn id = %d\n", VdChn);

    ret = bmdec_ioctl_stop_recv_stream(vpu_dec_chn[VdChn].chn_fd);
    if(ret != 0)
    {
        BMVPU_DEC_ERROR("ioctl CVI_VC_VDEC_STOP_RECV_STREAM fail with %d", ret);
        goto ERR_RET;
    }

    ret = bmdec_ioctl_destory_chn(vpu_dec_chn[VdChn].chn_fd);
    if(ret != 0)
    {
        BMVPU_DEC_ERROR("ioctl CVI_VC_VDEC_DESTROY_CHN fail with %d", ret);
        goto ERR_RET;
    }

    close(vpu_dec_chn[VdChn].chn_fd);
    BMVPU_DEC_TRACE("the chn %d is deleted\n", VdChn);
    vpu_dec_chn[VdChn].chn_fd = 0;
    vpu_dec_chn[VdChn].chn_id = 0;
    vpu_dec_chn[VdChn].is_used = 0;
    u32ChannelCreatedCnt -= 1;

    pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);

    if(u32ChannelCreatedCnt == 0)
    {
        bmdec_chn_close(0);
    }
    pthread_mutex_unlock(&VdecChn_Mutex);

    if (pthread_rwlock_destroy(&vpu_dec_chn[VdChn].process_lock) != 0) {
        BMVPU_DEC_ERROR("pthread_rwlock_destroy error");
    }

    return ret;

ERR_RET:
    pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
    return ret;
}


BMDecStatus bmvpu_dec_get_status(BMVidCodHandle vidCodHandle)
{
    int ret;
    vdec_chn_status_s stDecStatus = {0};
    BMDecStatus state = 0;
    int VdChn;

    if(vidCodHandle == NULL)
    {
        BMVPU_DEC_ERROR("invalid vdec handle.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].is_used != 1)
    {
        BMVPU_DEC_ERROR("invalid vdec chn.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    pthread_rwlock_rdlock(&vpu_dec_chn[VdChn].process_lock);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    ret = bmdec_ioctl_query_chn_status(vpu_dec_chn[VdChn].chn_fd, &stDecStatus);
    if(ret != BM_SUCCESS)
    {
        BMVPU_DEC_ERROR("Vdec query channel status failed.");
        pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
        return ret;
    }

    switch (stDecStatus.u8Status){
    case SEQ_INIT_NON:
        state = BMDEC_UNINIT;
        break;
    case SEQ_INIT_START:
        state = BMDEC_INITING;
        break;
    case SEQ_DECODE_START:
        state = BMDEC_DECODING;
        break;
    case SEQ_DECODE_WRONG_RESOLUTION:
        state = BMDEC_WRONG_RESOLUTION;
        break;
    // case SEQ_DECODE_HANG:
    //     state = BMDEC_HUNG;
    //     break;
    // case SEQ_DECODE_BUF_FULL:
    //     state = BMDEC_FRAME_BUF_FULL;
    //     break;
    case SEQ_DECODE_FINISH:
        state = BMDEC_STOP;
        break;
    default:
        break;
    }

    pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
    return state;
}

BMVidDecRetStatus bmvpu_dec_get_caps(BMVidCodHandle vidCodHandle, BMVidStreamInfo *streamInfo)
{
    int ret;
    vdec_chn_status_s stDecStatus = {0};
    int VdChn;

    if(vidCodHandle == NULL)
    {
        BMVPU_DEC_ERROR("invalid vdec handle.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].is_used != 1)
    {
        BMVPU_DEC_ERROR("invalid vdec chn.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    pthread_rwlock_rdlock(&vpu_dec_chn[VdChn].process_lock);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    ret = bmdec_ioctl_query_chn_status(vpu_dec_chn[VdChn].chn_fd, &stDecStatus);
    if(ret != BM_SUCCESS)
    {
        BMVPU_DEC_ERROR("Vdec query channel status failed.");
        pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
        return ret;
    }

    streamInfo->picWidth    = stDecStatus.stSeqinitalInfo.s32PicWidth;
    streamInfo->picHeight   = stDecStatus.stSeqinitalInfo.s32PicHeight;
    streamInfo->bitRate     = stDecStatus.stSeqinitalInfo.s32BitRate;
    streamInfo->profile     = stDecStatus.stSeqinitalInfo.s32Profile;
    streamInfo->level       = stDecStatus.stSeqinitalInfo.s32Level;
    streamInfo->interlace   = stDecStatus.stSeqinitalInfo.s32Interlace;
    streamInfo->aspectRateInfo  = stDecStatus.stSeqinitalInfo.s32AspectRateInfo;
    streamInfo->frameBufDelay   = stDecStatus.stSeqinitalInfo.s32FrameBufDelay;
    streamInfo->fRateNumerator  = stDecStatus.stSeqinitalInfo.s32FRateNumerator;
    streamInfo->fRateDenominator    = stDecStatus.stSeqinitalInfo.s32FRateDenominator;
    streamInfo->chromaBitdepth  = stDecStatus.stSeqinitalInfo.s32ChromaBitdepth;
    streamInfo->lumaBitdepth    = stDecStatus.stSeqinitalInfo.s32LumaBitdepth;
    streamInfo->maxNumRefFrm    = stDecStatus.stSeqinitalInfo.s32MaxNumRefFrm;
    streamInfo->maxNumRefFrmFlag    = stDecStatus.stSeqinitalInfo.s32MaxNumRefFrmFlag;
    streamInfo->minFrameBufferCount = stDecStatus.stSeqinitalInfo.s32MinFrameBufferCount;
    streamInfo->picCropRect.bottom  = stDecStatus.stSeqinitalInfo.stPicCropRect.bottom;
    streamInfo->picCropRect.top     = stDecStatus.stSeqinitalInfo.stPicCropRect.top;
    streamInfo->picCropRect.left    = stDecStatus.stSeqinitalInfo.stPicCropRect.left;
    streamInfo->picCropRect.right   = stDecStatus.stSeqinitalInfo.stPicCropRect.right;

    pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
    return BM_SUCCESS;
}

BMVidDecRetStatus bmvpu_dec_get_stream_info(BMVidCodHandle vidCodHandle, int* width, int* height, int* mini_fb, int* frame_delay)
{
    int ret;
    vdec_chn_status_s stDecStatus = {0};
    int VdChn;

    if(vidCodHandle == NULL)
    {
        BMVPU_DEC_ERROR("invalid vdec handle.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].is_used != 1)
    {
        BMVPU_DEC_ERROR("invalid vdec chn.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    pthread_rwlock_rdlock(&vpu_dec_chn[VdChn].process_lock);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    ret = bmdec_ioctl_query_chn_status(vpu_dec_chn[VdChn].chn_fd, &stDecStatus);
    if(ret != BM_SUCCESS)
    {
        BMVPU_DEC_ERROR("Vdec query channel status failed.");
        pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
        return ret;
    }

    *width = stDecStatus.stSeqinitalInfo.s32PicWidth;
    *height = stDecStatus.stSeqinitalInfo.s32PicHeight;
    *mini_fb = stDecStatus.stSeqinitalInfo.s32MinFrameBufferCount;
    *frame_delay = stDecStatus.stSeqinitalInfo.s32FrameBufDelay;

    pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
    return ret;
}


int bmvpu_dec_get_all_empty_input_buf_cnt(BMVidCodHandle vidCodHandle)
{
    int ret;
    vdec_chn_status_s stDecStatus = {0};
    int VdChn;

    if(vidCodHandle == NULL)
    {
        BMVPU_DEC_ERROR("invalid vdec handle.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].is_used != 1)
    {
        BMVPU_DEC_ERROR("invalid vdec chn.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    pthread_rwlock_rdlock(&vpu_dec_chn[VdChn].process_lock);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    ret = bmdec_ioctl_query_chn_status(vpu_dec_chn[VdChn].chn_fd, &stDecStatus);
    if(ret != BM_SUCCESS)
    {
        BMVPU_DEC_ERROR("Vdec query channel status failed.");
        pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
        return ret;
    }

    pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
    return (int)stDecStatus.u8FreeSrcBuffer;
}

int bmvpu_dec_get_stream_buffer_empty_size(BMVidCodHandle vidCodHandle)
{
    int ret;
    vdec_chn_status_s stDecStatus = {0};
    int VdChn = *((int *)vidCodHandle);

    if(vpu_dec_chn[VdChn].is_used != 1)
    {
        BMVPU_DEC_ERROR("invalid vdec chn.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    pthread_rwlock_rdlock(&vpu_dec_chn[VdChn].process_lock);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    ret = bmdec_ioctl_query_chn_status(vpu_dec_chn[VdChn].chn_fd, &stDecStatus);
    if(ret != BM_SUCCESS)
    {
        BMVPU_DEC_ERROR("Vdec query channel status failed.");
        pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
        return ret;
    }

    pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
    return (int)stDecStatus.u32EmptyStreamBufSzie;
}


int bmvpu_dec_get_pkt_in_buf_cnt(BMVidCodHandle vidCodHandle)
{
    int ret;
    vdec_chn_status_s stDecStatus = {0};
    int VdChn;

    if(vidCodHandle == NULL)
    {
        BMVPU_DEC_ERROR("invalid vdec handle.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].is_used != 1)
    {
        BMVPU_DEC_ERROR("invalid vdec chn.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    pthread_rwlock_rdlock(&vpu_dec_chn[VdChn].process_lock);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    ret = bmdec_ioctl_query_chn_status(vpu_dec_chn[VdChn].chn_fd, &stDecStatus);
    if(ret != BM_SUCCESS)
    {
        BMVPU_DEC_ERROR("Vdec query channel status failed.");
        pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
        return ret;
    }

    pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
    return (int)stDecStatus.u8BusySrcBuffer;
}


BMVidDecRetStatus bmvpu_dec_get_all_frame_in_buffer(BMVidCodHandle vidCodHandle)
{
    int ret;
    int VdChn;
    vdec_stream_s stStream;
    vdec_stream_ex_s stStreamEx;

    if(vidCodHandle == NULL)
    {
        BMVPU_DEC_ERROR("invalid vdec handle.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].is_used != 1)
    {
        BMVPU_DEC_ERROR("invalid vdec chn.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    pthread_rwlock_rdlock(&vpu_dec_chn[VdChn].process_lock);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    memset(&stStream, 0 ,sizeof(vdec_stream_s));
    stStream.bEndOfStream = 1;
    stStreamEx.pstStream = &stStream;
    stStreamEx.s32MilliSec = -1;
    ret = bmdec_ioctl_send_stream(vpu_dec_chn[VdChn].chn_fd, &stStreamEx);
    if(ret != BM_SUCCESS) {
        pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
        return ret;
    }
    pthread_rwlock_unlock(&vpu_dec_chn[VdChn].process_lock);
    return BMDEC_FLUSH_SUCCESS;
}

BMVidDecRetStatus bmvpu_dec_reset(int devIdx, int coreIdx)
{
    //int VdChn;

    //CVI_VDEC_ResetChn(VdChn);
    return BM_SUCCESS;
}

// Need to set decParam->streamFormat before use it.
BMVidDecRetStatus bmvpu_dec_get_param(BMVidStream vidStream, BMVidDecParam *decParam)
{
    BMVidCodHandle vidHandle;
    int ret = 0;
    BMVidFrame pFrame;
    BMVidStreamInfo streamInfo;
    int retry = 0;

    decParam->extraFrameBufferNum = 1;
    decParam->streamBufferSize = 0x500000;
    decParam->enable_cache = 0;
    decParam->bsMode = 0;
    decParam->core_idx = -1;
    decParam->cmd_queue_depth = 4;

    if(vidStream.buf == NULL || vidStream.length == 0){
        BMVPU_DEC_ERROR("Input buffer error\n");
        return BM_ERR_VDEC_ILLEGAL_PARAM;
    }

    ret = bmvpu_dec_create(&vidHandle, *decParam);
    if(ret != BM_SUCCESS){
        BMVPU_DEC_ERROR("Create decoder failed, ret=%d\n", ret);
        return ret;
    }

    while((ret = bmvpu_dec_decode(vidHandle, vidStream)) != BM_SUCCESS){
        usleep(1000);
        if(++retry >= 1000){
            BMVPU_DEC_ERROR("Send one frame failed after %d retries (%.3f sec), aborting.\n", retry, retry * 0.001);
            goto OUT2;
        }
    }

    bmvpu_dec_get_all_frame_in_buffer(vidHandle);

    retry = 0;
    while((ret = bmvpu_dec_get_output(vidHandle, &pFrame)) != BM_SUCCESS){
        usleep(1000);
        if(++retry >= 1000){
            BMVPU_DEC_ERROR("Get one frame failed after %d retries (%.3f sec), aborting.\n", retry, retry * 0.001);
            goto OUT2;
        }
    }

    memset(&streamInfo, 0 ,sizeof(BMVidStreamInfo));
    ret = bmvpu_dec_get_caps(vidHandle, &streamInfo);
    if(ret != BM_SUCCESS){
        BMVPU_DEC_ERROR("Get caps failed, ret=%d\n", ret);
        goto OUT1;
    }
    decParam->picWidth = streamInfo.picWidth;
    decParam->picHeight = streamInfo.picHeight;
    decParam->min_framebuf_cnt = streamInfo.minFrameBufferCount;
    decParam->framebuf_delay = streamInfo.frameBufDelay;

OUT1:
    if(bmvpu_dec_clear_output(vidHandle, &pFrame) != BM_SUCCESS){
        BMVPU_DEC_ERROR("Clear one frame failed.\n");
    }
OUT2:
    bmvpu_dec_delete(vidHandle);
    return ret;
}