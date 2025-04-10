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
#include <linux/vc_uapi.h>
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
//static int bmve_atomic_lock = 0;
static int bmhandle_atomic_lock = 0; /* atomic lock for bmlib_handle */
#elif _WIN32
static  volatile long bmve_atomic_lock = 0;
static  volatile long bmhandle_atomic_lock = 0;
#endif
static int bmlib_init_flag = 0;
BmVpuDecLogLevel bm_vpu_log_level_threshold = BMVPU_DEC_LOG_LEVEL_ERR;

void bmdec_set_logging_threshold(BmVpuDecLogLevel threshold)
{
    bm_vpu_log_level_threshold = threshold;
    printf("vdec loglevel is %d\n", bm_vpu_log_level_threshold);
}

void bmdec_set_logging_thresholdEx()
{
    char *debug_env = getenv("BMVID_DEBUG");

    if (debug_env) {
        BmVpuDecLogLevel log_level = BMVPU_DEC_LOG_LEVEL_ERR;
        log_level = atoi(debug_env);
        bmdec_set_logging_threshold(log_level);
    }
}

void logging_fn(BmVpuDecLogLevel level, char const *file, int const line, char const *fn, const char *format, ...)
{
    va_list args;

    char const *lvlstr = "";
    switch (level)
    {
        case BMVPU_DEC_LOG_LEVEL_ERR:   lvlstr = "ERROR";   break;
        case BMVPU_DEC_LOG_LEVEL_WARN: lvlstr = "WARNING"; break;
        case BMVPU_DEC_LOG_LEVEL_INFO:    lvlstr = "INFO";    break;
        case BMVPU_DEC_LOG_LEVEL_DEBUG:   lvlstr = "DEBUG";   break;
        case BMVPU_DEC_LOG_LEVEL_TRACE:   lvlstr = "TRACE";   break;
        case BMVPU_DEC_LOG_LEVEL_LOG:     lvlstr = "LOG";     break;
        default: break;
    }

    fprintf(stderr, "%s:%d (%s)   %s: ", file, line, fn, lvlstr);

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");
}

static int bm_get_ret(int ret)
{
    int bm_ret = BM_SUCCESS;
    switch (ret)
    {
    case 0:
        bm_ret = BM_SUCCESS;
        break;
    case (-1):
        bm_ret = BM_ERR_VDEC_FAILURE;
        break;
    case DRV_ERR_VDEC_INVALID_CHNID:
        bm_ret = BM_ERR_VDEC_INVALID_CHNID;
        break;
    case DRV_ERR_VDEC_ILLEGAL_PARAM:
        bm_ret = BM_ERR_VDEC_ILLEGAL_PARAM;
        break;
    case DRV_ERR_VDEC_EXIST:
        bm_ret = BM_ERR_VDEC_EXIST;
        break;
    case DRV_ERR_VDEC_UNEXIST:
        bm_ret = BM_ERR_VDEC_UNEXIST;
        break;
    case DRV_ERR_VDEC_NULL_PTR:
        bm_ret = BM_ERR_VDEC_NULL_PTR;
        break;
    case DRV_ERR_VDEC_NOT_CONFIG:
        bm_ret = BM_ERR_VDEC_NOT_CONFIG;
        break;
    case DRV_ERR_VDEC_NOT_SUPPORT:
        bm_ret = BM_ERR_VDEC_NOT_SUPPORT;
        break;
    case DRV_ERR_VDEC_NOT_PERM:
        bm_ret = BM_ERR_VDEC_NOT_PERM;
        break;
    case DRV_ERR_VDEC_INVALID_PIPEID:
        bm_ret = BM_ERR_VDEC_INVALID_PIPEID;
        break;
    case DRV_ERR_VDEC_INVALID_GRPID:
        bm_ret = BM_ERR_VDEC_INVALID_GRPID;
        break;
    case DRV_ERR_VDEC_NOMEM:
        bm_ret = BM_ERR_VDEC_NOMEM;
        break;
    case DRV_ERR_VDEC_NOBUF:
        bm_ret = BM_ERR_VDEC_NOBUF;
        break;
    case DRV_ERR_VDEC_BUF_EMPTY:
        bm_ret = BM_ERR_VDEC_BUF_EMPTY;
        break;
    case DRV_ERR_VDEC_BUF_FULL:
        bm_ret = BM_ERR_VDEC_BUF_FULL;
        break;
    case DRV_ERR_VDEC_SYS_NOTREADY:
        bm_ret = BM_ERR_VDEC_SYS_NOTREADY;
        break;
    case DRV_ERR_VDEC_BADADDR:
        bm_ret = BM_ERR_VDEC_BADADDR;
        break;
    case DRV_ERR_VDEC_BUSY:
        bm_ret = BM_ERR_VDEC_BUSY;
        break;
    case DRV_ERR_VDEC_SIZE_NOT_ENOUGH:
        bm_ret = BM_ERR_VDEC_SIZE_NOT_ENOUGH;
        break;
    case DRV_ERR_VDEC_INVALID_VB:
        bm_ret = BM_ERR_VDEC_INVALID_VB;
        break;
    case DRV_ERR_VDEC_ERR_INIT:
        bm_ret = BM_ERR_VDEC_ERR_INIT;
        break;
    case DRV_ERR_VDEC_ERR_INVALID_RET:
        bm_ret = BM_ERR_VDEC_ERR_INVALID_RET;
        break;
    case DRV_ERR_VDEC_ERR_SEQ_OPER:
        bm_ret = BM_ERR_VDEC_ERR_SEQ_OPER;
        break;
    case DRV_ERR_VDEC_ERR_VDEC_MUTEX:
        bm_ret = BM_ERR_VDEC_ERR_VDEC_MUTEX;
        break;
    case DRV_ERR_VDEC_ERR_SEND_FAILED:
        bm_ret = BM_ERR_VDEC_ERR_SEND_FAILED;
        break;
    case DRV_ERR_VDEC_ERR_GET_FAILED:
        bm_ret = BM_ERR_VDEC_ERR_GET_FAILED;
        break;
    case DRV_ERR_VDEC_BUTT:
        bm_ret = BM_ERR_VDEC_BUTT;
        break;
    default:
        bm_ret = ret;
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

    if (soc_idx >= MAX_SOC_NUM)
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
    if (soc_idx >= MAX_SOC_NUM)
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
    if (soc_idx >= MAX_SOC_NUM)
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
    return (void*)vmem;
}

static void bmvpu_dec_bmlib_munmap(int soc_idx, uint64_t vir_addr, size_t len)
{
    bm_mem_unmap_device_mem(bmvpu_dec_get_bmlib_handle(soc_idx), (void *)vir_addr, len);
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
    int ret = 0;

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
    *chn_id = ioctl(chn_fd, DRV_VC_VCODEC_GET_CHN, is_jpu);
    if(*chn_id < 0)
        return BM_ERR_VDEC_INVALID_CHNID;

    return ret;
}

int bmdec_ioctl_set_chn(int chn_fd, int* chn_id)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VCODEC_SET_CHN, chn_id);
    return bm_get_ret(ret);
}

int bmdec_ioctl_create_chn(int chn_fd, vdec_chn_attr_s* pstAttr)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VDEC_CREATE_CHN, pstAttr);
    return bm_get_ret(ret);
}

int bmdec_ioctl_destory_chn(int chn_fd)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VDEC_DESTROY_CHN, NULL);
    return bm_get_ret(ret);
}

int bmdec_ioctl_start_recv_stream(int chn_fd)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VDEC_START_RECV_STREAM, NULL);
    return bm_get_ret(ret);
}

int bmdec_ioctl_stop_recv_stream(int chn_fd)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VDEC_STOP_RECV_STREAM, NULL);
    return bm_get_ret(ret);
}

int bmdec_ioctl_send_stream(int chn_fd, vdec_stream_ex_s* pstStreamEx)
{
    int ret = 0;
    int bm_ret;

    ret = ioctl(chn_fd, DRV_VC_VDEC_SEND_STREAM, pstStreamEx);
    bm_ret = bm_get_ret(ret);

    return bm_ret;
}

int bmdec_ioctl_get_frame(int chn_fd, video_frame_info_ex_s* pstFrameInfoEx, vdec_chn_status_s* stChnStatus)
{
    int ret = 0;
    int size = 0;
    int bm_ret;

    ret = ioctl(chn_fd, DRV_VC_VDEC_GET_FRAME, pstFrameInfoEx);
    if (ret == 0) {
        bm_ret = 0;
        if((ret = ioctl(chn_fd, DRV_VC_VDEC_QUERY_STATUS, stChnStatus)) != BM_SUCCESS) {
            BMVPU_DEC_ERROR("ioctl DRV_VC_VDEC_QUERY_STATUS error\n");
            return bm_get_ret(ret);
        }

        if(pstFrameInfoEx->pstFrame->video_frame.compress_mode == COMPRESS_MODE_FRAME){
#ifndef BM_PCIE_MODE
            if(getenv("BMVPU_DEC_DUMP_FBC_NUM") != NULL) {
                if(pstFrameInfoEx->pstFrame->video_frame.phyaddr[0] && pstFrameInfoEx->pstFrame->video_frame.length[0]) {
                    pstFrameInfoEx->pstFrame->video_frame.viraddr[0] =
                        bmvpu_dec_bmlib_mmap(0, pstFrameInfoEx->pstFrame->video_frame.phyaddr[0],
                                            pstFrameInfoEx->pstFrame->video_frame.length[0]);
                }

                if(pstFrameInfoEx->pstFrame->video_frame.phyaddr[1] && pstFrameInfoEx->pstFrame->video_frame.length[1]) {
                    pstFrameInfoEx->pstFrame->video_frame.viraddr[1] =
                        bmvpu_dec_bmlib_mmap(0, pstFrameInfoEx->pstFrame->video_frame.phyaddr[1],
                                            pstFrameInfoEx->pstFrame->video_frame.length[1]);
                }

                if(pstFrameInfoEx->pstFrame->video_frame.phyaddr[2] && pstFrameInfoEx->pstFrame->video_frame.length[2]) {
                    pstFrameInfoEx->pstFrame->video_frame.viraddr[2] =
                        bmvpu_dec_bmlib_mmap(0, pstFrameInfoEx->pstFrame->video_frame.phyaddr[2],
                                            pstFrameInfoEx->pstFrame->video_frame.length[2]);
                }

                if(pstFrameInfoEx->pstFrame->video_frame.ext_phy_addr && pstFrameInfoEx->pstFrame->video_frame.ext_length) {
                    pstFrameInfoEx->pstFrame->video_frame.ext_virt_addr =
                        bmvpu_dec_bmlib_mmap(0, pstFrameInfoEx->pstFrame->video_frame.ext_phy_addr,
                                            pstFrameInfoEx->pstFrame->video_frame.ext_length);
                }
            }
#endif
            return ret;
        }

#ifndef BM_PCIE_MODE
        size = (pstFrameInfoEx->pstFrame->video_frame.phyaddr[1] - pstFrameInfoEx->pstFrame->video_frame.phyaddr[0]) * 3 / 2;
        if(pstFrameInfoEx->pstFrame->video_frame.phyaddr[0] && size) {
            pstFrameInfoEx->pstFrame->video_frame.viraddr[0] =
                bmvpu_dec_bmlib_mmap(0, pstFrameInfoEx->pstFrame->video_frame.phyaddr[0], size);

            pstFrameInfoEx->pstFrame->video_frame.viraddr[1] = pstFrameInfoEx->pstFrame->video_frame.viraddr[0] +
                                                                pstFrameInfoEx->pstFrame->video_frame.phyaddr[1] -
                                                                pstFrameInfoEx->pstFrame->video_frame.phyaddr[0];

            if(pstFrameInfoEx->pstFrame->video_frame.pixel_format == PIXEL_FORMAT_YUV_PLANAR_420) {
                pstFrameInfoEx->pstFrame->video_frame.viraddr[2] = pstFrameInfoEx->pstFrame->video_frame.viraddr[1] +
                                                                    pstFrameInfoEx->pstFrame->video_frame.phyaddr[2] -
                                                                    pstFrameInfoEx->pstFrame->video_frame.phyaddr[1];
            }
        }
#endif
    }
    else{
        bm_ret = bm_get_ret(ret);
        if(bm_ret != BM_SUCCESS) {
            ret = ioctl(chn_fd, DRV_VC_VDEC_QUERY_STATUS, stChnStatus);
            if(stChnStatus->u8Status == SEQ_DECODE_WRONG_RESOLUTION)
                bm_ret = BM_ERR_VDEC_ILLEGAL_PARAM;
        }
    }

    return bm_ret;
}

int bmdec_ioctl_release_frame(int chn_fd, video_frame_info_s *pstFrameInfo, int size)
{
    int ret = 0;
    if(pstFrameInfo->video_frame.compress_mode != COMPRESS_MODE_FRAME) {
#ifndef BM_PCIE_MODE
        if (pstFrameInfo->video_frame.viraddr[0] && pstFrameInfo->video_frame.length[0]) {
            bmvpu_dec_bmlib_munmap(0, (uint64_t)pstFrameInfo->video_frame.viraddr[0], size);
        }
#endif
    }
    else {
        if(getenv("BMVPU_DEC_DUMP_FBC_NUM") != NULL) {
#ifndef BM_PCIE_MODE
            if (pstFrameInfo->video_frame.viraddr[0] && pstFrameInfo->video_frame.length[0]) {
                bmvpu_dec_bmlib_munmap(0, (uint64_t)pstFrameInfo->video_frame.viraddr[0], pstFrameInfo->video_frame.length[0]);
            }

            if (pstFrameInfo->video_frame.viraddr[1] && pstFrameInfo->video_frame.length[1]) {
                bmvpu_dec_bmlib_munmap(0, (uint64_t)pstFrameInfo->video_frame.viraddr[1], pstFrameInfo->video_frame.length[1]);
            }

            if (pstFrameInfo->video_frame.viraddr[2] && pstFrameInfo->video_frame.length[2]) {
                bmvpu_dec_bmlib_munmap(0, (uint64_t)pstFrameInfo->video_frame.viraddr[2], pstFrameInfo->video_frame.length[2]);
            }

            if(pstFrameInfo->video_frame.ext_virt_addr && pstFrameInfo->video_frame.ext_length) {
                bmvpu_dec_bmlib_munmap(0, (uint64_t)pstFrameInfo->video_frame.ext_virt_addr, pstFrameInfo->video_frame.ext_length);
            }
#endif
        }
    }

    ret = ioctl(chn_fd, DRV_VC_VDEC_RELEASE_FRAME, pstFrameInfo);
    if(ret == BM_SUCCESS)
        memset(pstFrameInfo, 0, sizeof(video_frame_info_s));

    return bm_get_ret(ret);
}

int bmdec_ioctl_get_chn_attr(int chn_fd, vdec_chn_attr_s *pstAttr)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VDEC_GET_CHN_ATTR, pstAttr);
    return bm_get_ret(ret);
}

int bmdec_ioctl_set_chn_attr(int chn_fd, vdec_chn_attr_s *pstAttr)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VDEC_SET_CHN_ATTR, pstAttr);
    return bm_get_ret(ret);
}

int bmdec_ioctl_query_chn_status(int chn_fd, vdec_chn_status_s *pstStatus)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VDEC_QUERY_STATUS, pstStatus);
    return bm_get_ret(ret);
}

int bmdec_ioctl_set_chn_param(int chn_fd, const vdec_chn_param_s *pstParam)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VDEC_SET_CHN_PARAM, pstParam);
    return bm_get_ret(ret);
}

int bmdec_ioctl_get_chn_param(int chn_fd, vdec_chn_param_s *pstParam)
{
    int ret = 0;
    ret = ioctl(chn_fd, DRV_VC_VDEC_GET_CHN_PARAM, pstParam);
    return bm_get_ret(ret);
}
