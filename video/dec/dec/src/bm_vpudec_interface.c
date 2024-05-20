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
#include "linux/cvi_buffer.h"

//for dump frame for debuging lasi..
#ifndef BM_PCIE_MODE
#ifdef __linux__
static __thread int dump_frame_num = 0;
#elif _WIN32
static  __declspec(thread) int dump_frame_num = 0;
#endif
#endif


#define VDEC_MAX_CHN_NUM    64
#define SOPH_VC_DRV_DECODER_DEV_NAME "soph_vc_dec"

typedef struct _BM_VDEC_CTX{
    int is_used;
    int chn_fd;
    int chn_id;
} BM_VDEC_CTX;

BM_VDEC_CTX vpu_dec_chn[VDEC_MAX_CHN_NUM] = {0};
static CVI_U32 u32ChannelCreatedCnt = 0;
static pthread_mutex_t VdecChn_Mutex = PTHREAD_MUTEX_INITIALIZER;

int bmvpu_dec_dump_stream(BMVidCodHandle vidCodHandle, BMVidStream vidStream);

int bmvpu_dec_get_core_idx(BMVidCodHandle handle){
    int ret;
    int coreIdx = 0;
    int VdChn = *((int *)handle);
    VDEC_CHN_STATUS_S stDecStatus = {0};
    if(vpu_dec_chn[VdChn].chn_fd < 0){
        BMVPU_DEC_ERROR("Vdec device fd error.");
        return BM_ERR_VDEC_FAILURE;
    }

    ret = bmdec_ioctl_query_chn_status(vpu_dec_chn[VdChn].chn_fd, &stDecStatus);
    if(ret != BM_SUCCESS){
        BMVPU_DEC_ERROR("Vdec query channel status failed. error: %d", ret);
        return ret;
    }
    coreIdx = stDecStatus.stSeqinitalInfo.u8CoreIdx;

    return coreIdx;
}

int bmvpu_dec_get_inst_idx(BMVidCodHandle vidCodHandle)
{
    int VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        return BM_ERR_VDEC_FAILURE;
    }

    return VdChn;
}

int bmvpu_dec_get_device_fd(BMVidCodHandle vidCodHandle)
{
    int VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        return BM_ERR_VDEC_FAILURE;
    }

    return vpu_dec_chn[VdChn].chn_fd;
}

/**
 * @brief This function create a decoder instance.
 * @param pop [Input] this is open decoder parameter
 * @param pHandle [Output] decoder instance.
 * @return error code.
 */
int bmvpu_dec_create(BMVidCodHandle *pVidCodHandle, BMVidDecParam decParam)
{
    int ret = 0;
    int VdChn_id = -1;
    int device_fd = 0;
    int is_jpu = 0;
    char devName[255];

    VDEC_CHN_ATTR_S stAttr = {0};
    VDEC_CHN_PARAM_S stChnParam;

    BMVPU_DEC_TRACE("enter bmvpu_dec_create\n");

    bm_vpu_set_logging_threshold(BM_VPU_LOG_LEVEL_INFO);

    if(decParam.streamFormat == 0)
        stAttr.enType = PT_H264;
    else if(decParam.streamFormat == 12)
        stAttr.enType = PT_H265;
    stAttr.u32StreamBufSize = decParam.streamBufferSize == 0 ? STREAM_BUF_SIZE : decParam.streamBufferSize;
    stAttr.enMode = decParam.bsMode == 0 ? VIDEO_MODE_STREAM : VIDEO_MODE_FRAME;
    stAttr.u32FrameBufCnt = (decParam.extraFrameBufferNum >= 0) ? decParam.extraFrameBufferNum : 2;
    stAttr.enCompressMode = (decParam.wtlFormat == BMDEC_OUTPUT_COMPRESSED) ? COMPRESS_MODE_FRAME : COMPRESS_MODE_NONE;
    stAttr.u8CommandQueueDepth = decParam.cmd_queue_depth;

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
        BMVPU_DEC_ERROR("init dec error: dec open is more than the max ctreate num(%d).\n", VDEC_MAX_CHN_NUM);
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
    pthread_mutex_unlock(&VdecChn_Mutex);

    bmdec_ioctl_get_chn_param(vpu_dec_chn[VdChn_id].chn_fd, &stChnParam);
    if(decParam.cbcrInterleave == 1)
        stChnParam.enPixelFormat = (decParam.nv21 == 1) ? PIXEL_FORMAT_NV21 : PIXEL_FORMAT_NV12;
    else if(decParam.cbcrInterleave == 2)
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
        pthread_mutex_lock(&VdecChn_Mutex);
        u32ChannelCreatedCnt -= 1;
        goto ERR_DEC_INIT;
    }

    return ret;

ERR_DEC_INIT:
    vpu_dec_chn[VdChn_id].chn_fd = -1;
    vpu_dec_chn[VdChn_id].chn_id = 0;
    vpu_dec_chn[VdChn_id].is_used = 0;
    *pVidCodHandle = NULL;
ERR_DEC_INIT2:
    pthread_mutex_unlock(&VdecChn_Mutex);
    if(u32ChannelCreatedCnt == 0)
        bmdec_chn_close(0);

    return ret;
}


int bmvpu_dec_decode(BMVidCodHandle vidCodHandle, BMVidStream vidStream)
{
    int ret = BM_SUCCESS;
    char *dump_num;
    CVI_BOOL bEndOfStream = CVI_FALSE;
    VDEC_STREAM_EX_S stStreamEx;
    VDEC_STREAM_S stStream;
    VDEC_CHN_ATTR_S stAttr;
    uint8_t *total_buf = NULL;
    int total_size;

    int VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    BMVPU_DEC_TRACE("enter bmvpu_dec_decode\n");

    ret = bmdec_ioctl_get_chn_attr(vpu_dec_chn[VdChn].chn_fd, &stAttr);
    if(ret != BM_SUCCESS){
        BMVPU_DEC_ERROR("VDEC_GET_CHN_ATTR error. ret = %d");
        return ret;
    }

    if(stAttr.enMode == VIDEO_MODE_FRAME){
        stStream.bEndOfFrame = CVI_TRUE;
        stStream.bDisplay = CVI_TRUE;
    }
    else{
        stStream.bEndOfFrame = CVI_FALSE;
    }

    if(vidStream.header_size != 0 && vidStream.header_buf != NULL) {
        total_size = vidStream.header_size + vidStream.length;
        total_buf = (uint8_t*)malloc(total_size);
        if(total_buf == NULL) {
            BMVPU_DEC_ERROR("malloc input buffer error.");
            return BM_ERR_VDEC_NOBUF;
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

    if(total_buf != NULL) {
        free(total_buf);
        total_buf = NULL;
    }

    if((dump_num = getenv("BMVPU_DEC_DUMP_NUM")) != NULL)
        bmvpu_dec_dump_stream(vidCodHandle, vidStream);

    return ret;
}

int bmvpu_dec_get_output(BMVidCodHandle vidCodHandle, BMVidFrame *bmFrame)
{
    int ret;
    int dump_num;
    int VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    if(bmFrame == NULL) {
        BMVPU_DEC_ERROR("the frame buffer invalid");
        return BM_ERR_VDEC_NOBUF;
    }

    BMVPU_DEC_TRACE("enter bmvpu_dec_get_output");

    VIDEO_FRAME_INFO_S stFrameInfo = {0};
    VIDEO_FRAME_INFO_EX_S stFrameInfoEx = {0};
    VDEC_CHN_STATUS_S stChnStatus = {0};
    stFrameInfoEx.pstFrame = &stFrameInfo;
    stFrameInfoEx.s32MilliSec = 0;

    ret = bmdec_ioctl_get_frame(vpu_dec_chn[VdChn].chn_fd, &stFrameInfoEx, &stChnStatus);
    if (ret != BM_SUCCESS)
        return ret;
    else
        BMVPU_DEC_LOG("get frame success");

    if(stFrameInfo.stVFrame.enPixelFormat == PIXEL_FORMAT_NV12){
        bmFrame->frameFormat = 0;
        bmFrame->cbcrInterleave = 1;
        bmFrame->nv21 = 0;
    }
    else if(stFrameInfo.stVFrame.enPixelFormat == PIXEL_FORMAT_NV21){
        bmFrame->frameFormat = 0;
        bmFrame->cbcrInterleave = 1;
        bmFrame->nv21 = 1;
    }
    else{
        bmFrame->frameFormat = 0;
        bmFrame->cbcrInterleave = 0;
        bmFrame->nv21 = 0;
    }
    bmFrame->compressed_mode = stFrameInfo.stVFrame.enCompressMode;

    /* BMVidFrame.buf
    < 0: Y virt addr, 1: Cb virt addr: 2, Cr virt addr. 4: Y phy addr, 5: Cb phy addr, 6: Cr phy addr */
    bmFrame->buf[0]   = stFrameInfo.stVFrame.pu8VirAddr[0];
    bmFrame->buf[1]   = stFrameInfo.stVFrame.pu8VirAddr[1];
    bmFrame->buf[2]   = stFrameInfo.stVFrame.pu8VirAddr[2];
    bmFrame->buf[4]   = stFrameInfo.stVFrame.u64PhyAddr[0];
    bmFrame->buf[5]   = stFrameInfo.stVFrame.u64PhyAddr[1];
    bmFrame->buf[6]   = stFrameInfo.stVFrame.u64PhyAddr[2];

    bmFrame->stride[0] = stFrameInfo.stVFrame.u32Stride[0];
    bmFrame->stride[4] = stFrameInfo.stVFrame.u32Stride[0];
    bmFrame->stride[1] = stFrameInfo.stVFrame.u32Stride[1];
    bmFrame->stride[5] = stFrameInfo.stVFrame.u32Stride[1];
    if(stFrameInfo.stVFrame.enCompressMode != COMPRESS_MODE_FRAME) {
        bmFrame->stride[2] = stFrameInfo.stVFrame.u32Stride[2];
        bmFrame->stride[6] = stFrameInfo.stVFrame.u32Stride[2];
    }
    else {
        bmFrame->buf[3] = stFrameInfo.stVFrame.pu8ExtVirtAddr;
        bmFrame->buf[7] = stFrameInfo.stVFrame.u64ExtPhyAddr;

        bmFrame->stride[2] = stFrameInfo.stVFrame.u32Length[2];
        bmFrame->stride[6] = stFrameInfo.stVFrame.u32Length[2];
        bmFrame->stride[3] = stFrameInfo.stVFrame.u32ExtLength;
        bmFrame->stride[7] = stFrameInfo.stVFrame.u32ExtLength;
    }

    bmFrame->width    = stFrameInfo.stVFrame.u32Width;
    bmFrame->height   = stFrameInfo.stVFrame.u32Height;
    bmFrame->frameIdx = (uintptr_t)stFrameInfo.stVFrame.pPrivateData;
    bmFrame->pts      = stFrameInfo.stVFrame.u64PTS;
    bmFrame->dts      = stFrameInfo.stVFrame.u64DTS;
    bmFrame->endian   = stFrameInfo.stVFrame.u8Endian;
    bmFrame->sequenceNo = stFrameInfo.stVFrame.u32SeqenceNo;
    bmFrame->picType  = stFrameInfo.stVFrame.u8PicType;
    bmFrame->interlacedFrame = stFrameInfo.stVFrame.u8InterlacedFrame;
    bmFrame->size = (stFrameInfo.stVFrame.u64PhyAddr[1] - stFrameInfo.stVFrame.u64PhyAddr[0]) * 3 / 2;
    bmFrame->lumaBitDepth = stChnStatus.stSeqinitalInfo.s32LumaBitdepth;
    bmFrame->chromaBitDepth = stChnStatus.stSeqinitalInfo.s32ChromaBitdepth;
    bmFrame->coded_width    = stChnStatus.stSeqinitalInfo.s32PicWidth;
    bmFrame->coded_height   = stChnStatus.stSeqinitalInfo.s32PicHeight;

    // frame->colorPrimaries = stFrameInfo.stVFrame.
    // frame->colorTransferCharacteristic = stFrameInfo.stVFrame.
    // frame->colorSpace = stFrameInfo.stVFrame.
    // frame->colorRange = stFrameInfo.stVFrame.
    // frame->chromaLocation = stFrameInfo.stVFrame.

    if(stFrameInfo.stVFrame.enCompressMode == COMPRESS_MODE_FRAME){
        if(getenv("BMVPU_DEC_DUMP_FBC_NUM") != NULL) {
            int core_num = bmvpu_dec_get_core_idx(vidCodHandle);
            int inst_num = bmvpu_dec_get_inst_idx(vidCodHandle);

            dump_num = atoi(getenv("BMVPU_DEC_DUMP_FBC_NUM"));
            static int dump_cnt[64] = {0};
            if(dump_cnt[inst_num] >= dump_num)
                return BM_SUCCESS;

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

    return BM_SUCCESS;
}

int bmvpu_dec_clear_output(BMVidCodHandle vidCodHandle, BMVidFrame *frame)
{
    int ret;
    int VdChn = *((int *)vidCodHandle);
    VIDEO_FRAME_INFO_S stFrameInfo = {0};
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    BMVPU_DEC_TRACE("enter bmvpu_dec_clear_output\n");

    stFrameInfo.stVFrame.u32Width        = frame->width;
    stFrameInfo.stVFrame.u32Height       = frame->height;
    stFrameInfo.stVFrame.u32Stride[0]    = frame->stride[0];
    stFrameInfo.stVFrame.u32Stride[1]    = frame->stride[1];
    stFrameInfo.stVFrame.u32Stride[2]    = frame->stride[2];

    stFrameInfo.stVFrame.pu8VirAddr[0]   = frame->buf[0];
    stFrameInfo.stVFrame.pu8VirAddr[1]   = frame->buf[1];
    stFrameInfo.stVFrame.pu8VirAddr[2]   = frame->buf[2];
    stFrameInfo.stVFrame.u64PhyAddr[0]   = frame->buf[4];
    stFrameInfo.stVFrame.u64PhyAddr[1]   = frame->buf[5];
    stFrameInfo.stVFrame.u64PhyAddr[2]   = frame->buf[6];

    stFrameInfo.stVFrame.enCompressMode = frame->compressed_mode;
    stFrameInfo.stVFrame.pPrivateData  = (void *)frame->frameIdx;

    stFrameInfo.stVFrame.u32Length[0]   = frame->stride[0] * frame->height;
    stFrameInfo.stVFrame.u32Length[1]   = frame->stride[1] * frame->height / 2;
    if(stFrameInfo.stVFrame.enCompressMode != COMPRESS_MODE_FRAME) {
        stFrameInfo.stVFrame.u32Length[2]   = frame->stride[2] * frame->height / 2;
    }
    else{
        stFrameInfo.stVFrame.u32Length[2]      = frame->stride[2];
        stFrameInfo.stVFrame.u32ExtLength      = frame->stride[3];

        stFrameInfo.stVFrame.pu8ExtVirtAddr    = frame->buf[3];
        stFrameInfo.stVFrame.u64ExtPhyAddr     = frame->buf[7];
    }

    if(frame->cbcrInterleave == 1)
        stFrameInfo.stVFrame.enPixelFormat = (frame->nv21 == 1) ? PIXEL_FORMAT_NV21 : PIXEL_FORMAT_NV12;
    else
        stFrameInfo.stVFrame.enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;

    ret = bmdec_ioctl_release_frame(vpu_dec_chn[VdChn].chn_fd, &stFrameInfo, frame->size);
    if(ret != BM_SUCCESS) {
        BMVPU_DEC_ERROR("realease frame failed %d.\n", ret);
        return ret;
    }

    return BM_SUCCESS;
}


int bmvpu_dec_flush(BMVidCodHandle vidCodHandle)
{
    int ret;
    int VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    VDEC_STREAM_S stStream;
    VDEC_STREAM_EX_S stStreamEx;

    memset(&stStream, 0 ,sizeof(VDEC_STREAM_S));
    stStream.bEndOfStream = CVI_TRUE;
    stStreamEx.pstStream = &stStream;
    stStreamEx.s32MilliSec = -1;
    while(bmdec_ioctl_send_stream(vpu_dec_chn[VdChn].chn_fd, &stStreamEx) != CVI_SUCCESS)
    {
        usleep(1000);
    }

    return BM_SUCCESS;
}

int bmvpu_dec_delete(BMVidCodHandle vidCodHandle)
{
    int ret;
    int VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    BMVPU_DEC_TRACE("enter bmvpu_dec_delete chn id = %d\n", VdChn);

    ret = bmdec_ioctl_stop_recv_stream(vpu_dec_chn[VdChn].chn_fd);
    if(ret != CVI_SUCCESS)
    {
        BMVPU_DEC_ERROR("ioctl CVI_VC_VDEC_STOP_RECV_STREAM fail with %d", ret);
        return ret;
    }

    ret = bmdec_ioctl_destory_chn(vpu_dec_chn[VdChn].chn_fd);
    if(ret != CVI_SUCCESS)
    {
        BMVPU_DEC_ERROR("ioctl CVI_VC_VDEC_DESTROY_CHN fail with %d", ret);
        return ret;
    }

    pthread_mutex_lock(&VdecChn_Mutex);
    close(vpu_dec_chn[VdChn].chn_fd);
    BMVPU_DEC_TRACE("the chn %d is deleted\n", VdChn);
    vpu_dec_chn[VdChn].chn_fd = 0;
    vpu_dec_chn[VdChn].chn_id = 0;
    vpu_dec_chn[VdChn].is_used = 0;
    u32ChannelCreatedCnt -= 1;
    pthread_mutex_unlock(&VdecChn_Mutex);

    if(u32ChannelCreatedCnt == 0)
    {
        bmdec_chn_close(0);
    }

    return ret;
}


BMDecStatus bmvpu_dec_get_status(BMVidCodHandle vidCodHandle)
{
    int ret;
    VDEC_CHN_STATUS_S stDecStatus = {0};
    BMDecStatus state;
    int VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    ret = bmdec_ioctl_query_chn_status(vpu_dec_chn[VdChn].chn_fd, &stDecStatus);
    if(ret != BM_SUCCESS)
    {
        BMVPU_DEC_ERROR("Vdec query channel status failed.");
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

    return state;
}


int bmvpu_dec_get_caps(BMVidCodHandle vidCodHandle, BMVidStreamInfo *streamInfo)
{
    int ret;
    VDEC_CHN_STATUS_S stDecStatus = {0};
    int VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    ret = bmdec_ioctl_query_chn_status(vpu_dec_chn[VdChn].chn_fd, &stDecStatus);
    if(ret != BM_SUCCESS)
    {
        BMVPU_DEC_ERROR("Vdec query channel status failed.");
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

    return BM_SUCCESS;
}


int bmvpu_dec_get_all_empty_input_buf_cnt(BMVidCodHandle vidCodHandle)
{
    int ret;
    VDEC_CHN_STATUS_S stDecStatus = {0};
    int VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    ret = bmdec_ioctl_query_chn_status(vpu_dec_chn[VdChn].chn_fd, &stDecStatus);
    if(ret != BM_SUCCESS)
    {
        BMVPU_DEC_ERROR("Vdec query channel status failed.");
        return ret;
    }

    return (int)stDecStatus.u8FreeSrcBuffer;
}


int bmvpu_dec_get_pkt_in_buf_cnt(BMVidCodHandle vidCodHandle)
{
    int ret;
    VDEC_CHN_STATUS_S stDecStatus = {0};
    int VdChn = *((int *)vidCodHandle);
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    ret = bmdec_ioctl_query_chn_status(vpu_dec_chn[VdChn].chn_fd, &stDecStatus);
    if(ret != BM_SUCCESS)
    {
        BMVPU_DEC_ERROR("Vdec query channel status failed.");
        return ret;
    }

    return (int)stDecStatus.u8BusySrcBuffer;
}


int bmvpu_dec_get_all_frame_in_buffer(BMVidCodHandle vidCodHandle)
{
    int ret;
    int VdChn = *((int *)vidCodHandle);
    VDEC_STREAM_S stStream;
    VDEC_STREAM_EX_S stStreamEx;
    if(vpu_dec_chn[VdChn].chn_fd < 0)
    {
        BMVPU_DEC_ERROR("Vdec device fd error.");
        return BM_ERR_VDEC_INVALID_CHNID;
    }

    memset(&stStream, 0 ,sizeof(VDEC_STREAM_S));
    stStream.bEndOfStream = CVI_TRUE;
    stStreamEx.pstStream = &stStream;
    stStreamEx.s32MilliSec = -1;
    ret = bmdec_ioctl_send_stream(vpu_dec_chn[VdChn].chn_fd, &stStreamEx);
    if(ret != BM_SUCCESS)
        return ret;
    return BMDEC_FLUSH_SUCCESS;
}

int bmvpu_dec_reset(int devIdx, int coreIdx)
{
    int VdChn;

    //CVI_VDEC_ResetChn(VdChn);
    return BM_SUCCESS;
}


int bmvpu_dec_dump_stream(BMVidCodHandle vidCodHandle, BMVidStream vidStream)
{
    int ret = 0;
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
        if(vidStream.header_buf != NULL && vidStream.header_size != 0) {
            fwrite(vidStream.header_buf, 1, vidStream.header_size, stream_fp);
        }
        fwrite(vidStream.buf, 1, vidStream.length, stream_fp);
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
