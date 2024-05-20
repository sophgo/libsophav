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

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <linux/types.h>
#include <linux/cvi_vc_drv_ioctl.h>
// #include "devmem.h"
#include "bm_ioctl.h"
#include "bmlib_runtime.h"

#define MAX_SOC_NUM 64
typedef struct _BMLIB_HANDLE{
    bm_handle_t bm_handle;
    unsigned int count;
} BMLIB_HANDLE;
static BMLIB_HANDLE g_bmlib_handle[MAX_SOC_NUM] = { {0, 0} };

#ifdef __linux__
static int bmve_atomic_lock = 0;
static int bmhandle_atomic_lock = 0; /* atomic lock for bmlib_handle */
#elif _WIN32
static  volatile long bmve_atomic_lock = 0;
static  volatile long bmhandle_atomic_lock = 0;
#endif
static int bmlib_init_flag = 0;
BmVpuLogLevel bm_vpu_log_level_threshold = BM_VPU_LOG_LEVEL_ERROR;

void bm_vpu_set_logging_threshold(BmVpuLogLevel threshold)
{
    bm_vpu_log_level_threshold = threshold;
}

void logging_fn(BmVpuLogLevel level, char const *file, int const line, char const *fn, const char *format, ...)
{
    va_list args;

    char const *lvlstr = "";
    switch (level)
    {
        case BM_VPU_LOG_LEVEL_ERROR:   lvlstr = "ERROR";   break;
        case BM_VPU_LOG_LEVEL_WARNING: lvlstr = "WARNING"; break;
        case BM_VPU_LOG_LEVEL_INFO:    lvlstr = "INFO";    break;
        case BM_VPU_LOG_LEVEL_DEBUG:   lvlstr = "DEBUG";   break;
        case BM_VPU_LOG_LEVEL_TRACE:   lvlstr = "TRACE";   break;
        case BM_VPU_LOG_LEVEL_LOG:     lvlstr = "LOG";     break;
        default: break;
    }

    fprintf(stderr, "%s:%d (%s)   %s: ", file, line, fn, lvlstr);

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");
}

static int bm_get_ret(int cvi_ret)
{
    int bm_ret = BM_SUCCESS;
    switch (cvi_ret)
    {
    case CVI_SUCCESS:
        bm_ret = BM_SUCCESS;
        break;
    case CVI_FAILURE:
        bm_ret = BM_ERR_VDEC_FAILURE;
        break;
    case CVI_ERR_VDEC_INVALID_CHNID:
        bm_ret = BM_ERR_VDEC_INVALID_CHNID;
        break;
    case CVI_ERR_VDEC_ILLEGAL_PARAM:
        bm_ret = BM_ERR_VDEC_ILLEGAL_PARAM;
        break;
    case CVI_ERR_VDEC_EXIST:
        bm_ret = BM_ERR_VDEC_EXIST;
        break;
    case CVI_ERR_VDEC_UNEXIST:
        bm_ret = BM_ERR_VDEC_UNEXIST;
        break;
    case CVI_ERR_VDEC_NULL_PTR:
        bm_ret = BM_ERR_VDEC_NULL_PTR;
        break;
    case CVI_ERR_VDEC_NOT_CONFIG:
        bm_ret = BM_ERR_VDEC_NOT_CONFIG;
        break;
    case CVI_ERR_VDEC_NOT_SUPPORT:
        bm_ret = BM_ERR_VDEC_NOT_SUPPORT;
        break;
    case CVI_ERR_VDEC_NOT_PERM:
        bm_ret = BM_ERR_VDEC_NOT_PERM;
        break;
    case CVI_ERR_VDEC_INVALID_PIPEID:
        bm_ret = BM_ERR_VDEC_INVALID_PIPEID;
        break;
    case CVI_ERR_VDEC_INVALID_GRPID:
        bm_ret = BM_ERR_VDEC_INVALID_GRPID;
        break;
    case CVI_ERR_VDEC_NOMEM:
        bm_ret = BM_ERR_VDEC_NOMEM;
        break;
    case CVI_ERR_VDEC_NOBUF:
        bm_ret = BM_ERR_VDEC_NOBUF;
        break;
    case CVI_ERR_VDEC_BUF_EMPTY:
        bm_ret = BM_ERR_VDEC_BUF_EMPTY;
        break;
    case CVI_ERR_VDEC_BUF_FULL:
        bm_ret = BM_ERR_VDEC_BUF_FULL;
        break;
    case CVI_ERR_VDEC_SYS_NOTREADY:
        bm_ret = BM_ERR_VDEC_SYS_NOTREADY;
        break;
    case CVI_ERR_VDEC_BADADDR:
        bm_ret = BM_ERR_VDEC_BADADDR;
        break;
    case CVI_ERR_VDEC_BUSY:
        bm_ret = BM_ERR_VDEC_BUSY;
        break;
    case CVI_ERR_VDEC_SIZE_NOT_ENOUGH:
        bm_ret = BM_ERR_VDEC_SIZE_NOT_ENOUGH;
        break;
    case CVI_ERR_VDEC_INVALID_VB:
        bm_ret = BM_ERR_VDEC_INVALID_VB;
        break;
    case CVI_ERR_VDEC_ERR_INIT:
        bm_ret = BM_ERR_VDEC_ERR_INIT;
        break;
    case CVI_ERR_VDEC_ERR_INVALID_RET:
        bm_ret = BM_ERR_VDEC_ERR_INVALID_RET;
        break;
    case CVI_ERR_VDEC_ERR_SEQ_OPER:
        bm_ret = BM_ERR_VDEC_ERR_SEQ_OPER;
        break;
    case CVI_ERR_VDEC_ERR_VDEC_MUTEX:
        bm_ret = BM_ERR_VDEC_ERR_VDEC_MUTEX;
        break;
    case CVI_ERR_VDEC_ERR_SEND_FAILED:
        bm_ret = BM_ERR_VDEC_ERR_SEND_FAILED;
        break;
    case CVI_ERR_VDEC_ERR_GET_FAILED:
        bm_ret = BM_ERR_VDEC_ERR_GET_FAILED;
        break;
    case CVI_ERR_VDEC_BUTT:
        bm_ret = BM_ERR_VDEC_BUTT;
        break;
    default:
        bm_ret = cvi_ret;
        break;
    }

    return bm_ret;
}

static void bm_handle_lock()
{
#ifdef __linux__
    while (__atomic_test_and_set(&bmhandle_atomic_lock, __ATOMIC_SEQ_CST))
    {
        usleep(100);
    }
#endif
#ifdef _WIN32
    while (InterlockedCompareExchange(&bmhandle_atomic_lock, 1, 0)) {
        Sleep(1);
    }
#endif
}

static void bm_handle_unlock()
{
#ifdef __linux__
    __atomic_clear(&bmhandle_atomic_lock, __ATOMIC_SEQ_CST);
#endif
#ifdef _WIN32
    InterlockedExchange(&bmhandle_atomic_lock, 0);
#endif
}

static void bmvpu_dec_load_bmlib_handle(int soc_idx){

    if (soc_idx > MAX_SOC_NUM)
    {
        BMVPU_DEC_ERROR("soc_idx excess MAX_SOC_NUM!\n");
        exit(0);
    }

    bm_handle_lock();
    if (g_bmlib_handle[soc_idx].bm_handle)
    {
        g_bmlib_handle[soc_idx].count += 1;
        bm_handle_unlock();
        return ;
    }

    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, soc_idx);

    if (ret != BM_SUCCESS) {
      BMVPU_DEC_ERROR("Create Bm Handle Failed\n");
      bm_handle_unlock();
      exit(0);
    }
    g_bmlib_handle[soc_idx].count = 1;
    g_bmlib_handle[soc_idx].bm_handle = handle;
    bm_handle_unlock();
    return ;
}


static void bmvpu_dec_unload_bmlib_handle(int soc_idx){
    if (soc_idx > MAX_SOC_NUM)
    {
      BMVPU_DEC_ERROR("soc_idx excess MAX_SOC_NUM!\n");
      exit(0);
    }

    if (g_bmlib_handle[soc_idx].bm_handle)
    {
        bm_handle_lock();
        if (g_bmlib_handle[soc_idx].count <= 1)
        {
            bm_dev_free(g_bmlib_handle[soc_idx].bm_handle);
            g_bmlib_handle[soc_idx].count = 0;
            g_bmlib_handle[soc_idx].bm_handle = 0;
        }
        else
        {
            g_bmlib_handle[soc_idx].count -= 1;
        }
        bm_handle_unlock();
    }
    else {
        BMVPU_DEC_ERROR("Bm_handle for decode on soc %d not exist \n", soc_idx);
    }
}

bm_handle_t bmvpu_dec_get_bmlib_handle(int soc_idx)
{
    bm_handle_t handle = 0;
    if (soc_idx > MAX_SOC_NUM)
    {
        BMVPU_DEC_ERROR("soc_idx excess MAX_SOC_NUM!\n");
        exit(0);
    }

    bm_handle_lock();
    if (g_bmlib_handle[soc_idx].bm_handle)
    {
        handle = g_bmlib_handle[soc_idx].bm_handle;
        bm_handle_unlock();
        return handle;
    }
    else
    {
        bm_handle_unlock();
        return handle;
    }

    return NULL;
}

static void* bmvpu_dec_bmlib_mmap(int soc_idx, uint64_t phy_addr, size_t len)
{
    unsigned long long vmem;
    bm_device_mem_t wrapped_dmem;
    wrapped_dmem.u.device.dmabuf_fd = 1;
    wrapped_dmem.u.device.device_addr = (unsigned long)(phy_addr);
    wrapped_dmem.flags.u.mem_type = BM_MEM_TYPE_DEVICE;
    wrapped_dmem.size = len;

    int ret = bm_mem_mmap_device_mem_no_cache(bmvpu_dec_get_bmlib_handle(soc_idx), &wrapped_dmem, &vmem);
    if (ret != BM_SUCCESS) {
        BMVPU_DEC_ERROR("bm_mem_mmap_device_mem_no_cache failed: 0x%x\n", ret);
    }
    return vmem;
}

static void bmvpu_dec_bmlib_munmap(int soc_idx, uint64_t vir_addr, size_t len)
{
    bm_mem_unmap_device_mem(bmvpu_dec_get_bmlib_handle(soc_idx), vir_addr, len);
    return;
}

int bmvpu_dec_load(int soc_idx)
{
    bmvpu_dec_load_bmlib_handle(soc_idx);

    return BM_SUCCESS;
}

int bmvpu_dec_unload(int soc_idx)
{
    bmvpu_dec_unload_bmlib_handle(soc_idx);

    return BM_SUCCESS;
}

int bmdec_chn_open(char* dev_name, int soc_idx) {

    int fd;

    fd = open(dev_name, O_RDWR | O_DSYNC | O_CLOEXEC);
    if (fd < 0) {
        BMVPU_DEC_ERROR("can not open cvi dec device %s.\n", dev_name);
        return -1;
    }

    if (bmlib_init_flag == 0) {
        if(bmvpu_dec_load(soc_idx) == BM_SUCCESS) {
            bmlib_init_flag = 1;
        }
        else {
            BMVPU_DEC_ERROR("init bmlib failed");
            return -1;
        }
    }

    return fd;
}

int bmdec_chn_close(int soc_idx) {
    int ret;

    if(bmlib_init_flag == 1) {
        ret = bmvpu_dec_unload(soc_idx);
        bmlib_init_flag = 0;
    }

    return ret;
}

///////////////
// DEC IOCTL //
///////////////
int bmdec_ioctl_get_chn(int chn_fd, int *is_jpu, int *chn_id)
{
    int ret = 0;
    *chn_id = ioctl(chn_fd, CVI_VC_VCODEC_GET_CHN, is_jpu);
    if(*chn_id < 0)
        return BM_ERR_VDEC_INVALID_CHNID;

    return ret;
}

int bmdec_ioctl_set_chn(int chn_fd, int* chn_id)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VCODEC_SET_CHN, chn_id);
    return bm_get_ret(ret);
}

int bmdec_ioctl_create_chn(int chn_fd, VDEC_CHN_ATTR_S* pstAttr)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VDEC_CREATE_CHN, pstAttr);
    return bm_get_ret(ret);
}

int bmdec_ioctl_destory_chn(int chn_fd)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VDEC_DESTROY_CHN, NULL);
    return bm_get_ret(ret);
}

int bmdec_ioctl_start_recv_stream(int chn_fd)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VDEC_START_RECV_STREAM, NULL);
    return bm_get_ret(ret);
}

int bmdec_ioctl_stop_recv_stream(int chn_fd)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VDEC_STOP_RECV_STREAM, NULL);
    return bm_get_ret(ret);
}

int bmdec_ioctl_send_stream(int chn_fd, VDEC_STREAM_EX_S* pstStreamEx)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VDEC_SEND_STREAM, pstStreamEx);
    return bm_get_ret(ret);
}

int bmdec_ioctl_get_frame(int chn_fd, VIDEO_FRAME_INFO_EX_S* pstFrameInfoEx, VDEC_CHN_STATUS_S* stChnStatus)
{
    int ret = 0;
    int i = 0;
    int align_width, align_height, size = 0;

    ret = ioctl(chn_fd, CVI_VC_VDEC_GET_FRAME, pstFrameInfoEx);
    if (ret == CVI_SUCCESS) {
        if((ret = ioctl(chn_fd, CVI_VC_VDEC_QUERY_STATUS, stChnStatus)) != BM_SUCCESS) {
            BMVPU_DEC_ERROR("ioctl CVI_VC_VDEC_QUERY_STATUS error\n");
            return bm_get_ret(ret);
        }

        if(pstFrameInfoEx->pstFrame->stVFrame.enCompressMode == COMPRESS_MODE_FRAME){
            if(getenv("BMVPU_DEC_DUMP_FBC_NUM") != NULL) {
                if(pstFrameInfoEx->pstFrame->stVFrame.u64PhyAddr[0] && pstFrameInfoEx->pstFrame->stVFrame.u32Length[0]) {
                    pstFrameInfoEx->pstFrame->stVFrame.pu8VirAddr[0] =
                        bmvpu_dec_bmlib_mmap(0, pstFrameInfoEx->pstFrame->stVFrame.u64PhyAddr[0],
                                            pstFrameInfoEx->pstFrame->stVFrame.u32Length[0]);
                }

                if(pstFrameInfoEx->pstFrame->stVFrame.u64PhyAddr[1] && pstFrameInfoEx->pstFrame->stVFrame.u32Length[1]) {
                    pstFrameInfoEx->pstFrame->stVFrame.pu8VirAddr[1] =
                        bmvpu_dec_bmlib_mmap(0, pstFrameInfoEx->pstFrame->stVFrame.u64PhyAddr[1],
                                            pstFrameInfoEx->pstFrame->stVFrame.u32Length[1]);
                }

                if(pstFrameInfoEx->pstFrame->stVFrame.u64PhyAddr[2] && pstFrameInfoEx->pstFrame->stVFrame.u32Length[2]) {
                    pstFrameInfoEx->pstFrame->stVFrame.pu8VirAddr[2] =
                        bmvpu_dec_bmlib_mmap(0, pstFrameInfoEx->pstFrame->stVFrame.u64PhyAddr[2],
                                            pstFrameInfoEx->pstFrame->stVFrame.u32Length[2]);
                }

                if(pstFrameInfoEx->pstFrame->stVFrame.u64ExtPhyAddr && pstFrameInfoEx->pstFrame->stVFrame.u32ExtLength) {
                    pstFrameInfoEx->pstFrame->stVFrame.pu8ExtVirtAddr =
                        bmvpu_dec_bmlib_mmap(0, pstFrameInfoEx->pstFrame->stVFrame.u64ExtPhyAddr,
                                            pstFrameInfoEx->pstFrame->stVFrame.u32ExtLength);
                }
            }
            return ret;
        }

        size = (pstFrameInfoEx->pstFrame->stVFrame.u64PhyAddr[1] - pstFrameInfoEx->pstFrame->stVFrame.u64PhyAddr[0]) * 3 / 2;
        if(pstFrameInfoEx->pstFrame->stVFrame.u64PhyAddr[0] && size) {
            pstFrameInfoEx->pstFrame->stVFrame.pu8VirAddr[0] =
                bmvpu_dec_bmlib_mmap(0, pstFrameInfoEx->pstFrame->stVFrame.u64PhyAddr[0], size);

            pstFrameInfoEx->pstFrame->stVFrame.pu8VirAddr[1] = pstFrameInfoEx->pstFrame->stVFrame.pu8VirAddr[0] +
                                                                pstFrameInfoEx->pstFrame->stVFrame.u64PhyAddr[1] -
                                                                pstFrameInfoEx->pstFrame->stVFrame.u64PhyAddr[0];

            if(pstFrameInfoEx->pstFrame->stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_420) {
                pstFrameInfoEx->pstFrame->stVFrame.pu8VirAddr[2] = pstFrameInfoEx->pstFrame->stVFrame.pu8VirAddr[1] +
                                                                    pstFrameInfoEx->pstFrame->stVFrame.u64PhyAddr[2] -
                                                                    pstFrameInfoEx->pstFrame->stVFrame.u64PhyAddr[1];
            }
        }
    }

    return bm_get_ret(ret);
}

int bmdec_ioctl_release_frame(int chn_fd, const VIDEO_FRAME_INFO_S *pstFrameInfo, int size)
{
    int ret = 0;
    if(pstFrameInfo->stVFrame.enCompressMode != COMPRESS_MODE_FRAME) {
        if (pstFrameInfo->stVFrame.pu8VirAddr[0] && pstFrameInfo->stVFrame.u32Length[0]) {
            bmvpu_dec_bmlib_munmap(0, pstFrameInfo->stVFrame.pu8VirAddr[0], size);
        }
    }
    else {
        if(getenv("BMVPU_DEC_DUMP_FBC_NUM") != NULL) {
            if (pstFrameInfo->stVFrame.pu8VirAddr[0] && pstFrameInfo->stVFrame.u32Length[0]) {
                bmvpu_dec_bmlib_munmap(0, pstFrameInfo->stVFrame.pu8VirAddr[0], pstFrameInfo->stVFrame.u32Length[0]);
            }

            if (pstFrameInfo->stVFrame.pu8VirAddr[1] && pstFrameInfo->stVFrame.u32Length[1]) {
                bmvpu_dec_bmlib_munmap(0, pstFrameInfo->stVFrame.pu8VirAddr[1], pstFrameInfo->stVFrame.u32Length[1]);
            }

            if (pstFrameInfo->stVFrame.pu8VirAddr[2] && pstFrameInfo->stVFrame.u32Length[2]) {
                bmvpu_dec_bmlib_munmap(0, pstFrameInfo->stVFrame.pu8VirAddr[2], pstFrameInfo->stVFrame.u32Length[2]);
            }

            if(pstFrameInfo->stVFrame.pu8ExtVirtAddr && pstFrameInfo->stVFrame.u32ExtLength) {
                bmvpu_dec_bmlib_munmap(0, pstFrameInfo->stVFrame.pu8ExtVirtAddr, pstFrameInfo->stVFrame.u32ExtLength);
            }
        }
    }

    ret = ioctl(chn_fd, CVI_VC_VDEC_RELEASE_FRAME, pstFrameInfo);
    if(ret == BM_SUCCESS)
        memset(pstFrameInfo, 0, sizeof(VIDEO_FRAME_INFO_S));

    return bm_get_ret(ret);
}

int bmdec_ioctl_get_chn_attr(int chn_fd, VDEC_CHN_ATTR_S *pstAttr)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VDEC_GET_CHN_ATTR, pstAttr);
    return bm_get_ret(ret);
}

int bmdec_ioctl_set_chn_attr(int chn_fd, VDEC_CHN_ATTR_S *pstAttr)
{
    int ret = 0;
    ret = ioctl(chn_fd, CVI_VC_VDEC_SET_CHN_ATTR, pstAttr);
    return bm_get_ret(ret);
}

int bmdec_ioctl_query_chn_status(int chn_fd, VDEC_CHN_STATUS_S *pstStatus)
{
    int ret = 0;
    ioctl(chn_fd, CVI_VC_VDEC_QUERY_STATUS, pstStatus);
    return bm_get_ret(ret);
}

int bmdec_ioctl_set_chn_param(int chn_fd, const VDEC_CHN_PARAM_S *pstParam)
{
    int ret = 0;
    ioctl(chn_fd, CVI_VC_VDEC_SET_CHN_PARAM, pstParam);
    return bm_get_ret(ret);
}

int bmdec_ioctl_get_chn_param(int chn_fd, VDEC_CHN_PARAM_S *pstParam)
{
    int ret = 0;
    ioctl(chn_fd, CVI_VC_VDEC_GET_CHN_PARAM, pstParam);
    return bm_get_ret(ret);
}
