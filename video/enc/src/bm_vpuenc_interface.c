/* bmvpuapi API library for the BitMain Sophon SoC
 *
 * Copyright (C) 2018 Solan Shang
 * Copyright (C) 2015 Carlos Rafael Giani
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */


#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <stdatomic.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#elif _WIN32
#include <windows.h>
#include  < MMSystem.h >
#pragma comment(lib, "winmm.lib")
#endif

#include <signal.h>     /* SIG_SETMASK */

#include "bm_vpuenc_interface.h"
#include "bm_ioctl.h"
#include "bm_vpu_logging.h"

#define MAX_SOC_NUM 64
typedef struct _BMLIB_HANDLE{
    bm_handle_t bm_handle;
    unsigned int count;
} BMLIB_HANDLE;
static BMLIB_HANDLE g_bm_handle[MAX_SOC_NUM] = { {0, 0} };
#define HEAP_MASK 0x06
#define VPU_ENC_GET_STREAM_TIMEOUT 3*1000 // 3*1000 ms

#define VPU_MEMORY_ALIGNMENT         0x8

#define DRV_ENCODER_DEV_NAME      "soph_vc_enc"
#define MAX_NUM_VPU_CORE_CHIP     5
#define VPU_ENC_CORE_IDX          4
#define VENC_MAX_SOC_NUM          64

#define  HANDLE int
#define ALIGN(value,base)   (((value) + (base) - 1) & ~((base)-1))

enum {
    BM_VPU_ENC_REC_IDX_END           = -1,
    BM_VPU_ENC_REC_IDX_DELAY         = -2,
    BM_VPU_ENC_REC_IDX_HEADER_ONLY   = -3,
    BM_VPU_ENC_REC_IDX_CHANGE_PARAM  = -4
};

#define VPU_ENC_BITSTREAM_BUFFER_SIZE (1024*1024*1)
int g_venc_mem_fd = -1;
static pthread_mutex_t enc_chn_mutex  = PTHREAD_MUTEX_INITIALIZER;

typedef struct _BM_AVPKT {
    BmVpuEncodedFrame pkt_data;
    int pkt_is_used;  // 0:idel  1:used(nal data)
    struct _BM_AVPKT *pnext;
} BM_AVPKT;

typedef struct _BM_VPUENC_CTX {
    int is_used;
    int chn_id;
    unsigned int max_pic_width;
    unsigned int max_pic_height;
    unsigned int pic_width;
    unsigned int pic_height;
    unsigned int y_stride_width;
    unsigned int c_stride_width;
    int chn_fd;

    venc_stream_s stStream;

    BmVpuColorFormat color_format;


    BM_AVPKT *p_pkt_list;  // fot pkt data
    BM_AVPKT  p_pkt_header;  // fot pkt (nal pps sps)
} BM_VPUENC_CTX;

BM_VPUENC_CTX g_enc_chn[VENC_MAX_SOC_NUM] = {0};  // 0: no use  1: is using
unsigned int g_vpu_ext_addr = 0x0;

static inline void get_pic_buffer_config_internal(unsigned int width, unsigned int height,
        pixel_format_e enPixelFormat, data_bitwidth_e enBitWidth,
        compress_mode_e enCmpMode, unsigned int u32Align, vb_cal_config_s *pstCalConfig)
{
    unsigned char  u8BitWidth = 0;
    unsigned int u32VBSize = 0;
    unsigned int u32AlignHeight = 0;
    unsigned int main_stride = 0;
    unsigned int c_stride = 0;
    unsigned int main_size = 0;
    unsigned int u32YSize = 0;
    unsigned int u32CSize = 0;

    /* u32Align: 0 is automatic mode, alignment size following system. Non-0 for specified alignment size */
    if (u32Align == 0)
        u32Align = DEFAULT_ALIGN;
    else if (u32Align > MAX_ALIGN)
        u32Align = MAX_ALIGN;

    switch (enBitWidth) {
        case DATA_BITWIDTH_8: {
            u8BitWidth = 8;
            break;
        }
        case DATA_BITWIDTH_10: {
            u8BitWidth = 10;
            break;
        }
        case DATA_BITWIDTH_12: {
            u8BitWidth = 12;
            break;
        }
        case DATA_BITWIDTH_14: {
            u8BitWidth = 14;
            break;
        }
        case DATA_BITWIDTH_16: {
            u8BitWidth = 16;
            break;
        }
        default: {
            u8BitWidth = 0;
            break;
        }
    }

    if ((enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_420)
     || (enPixelFormat == PIXEL_FORMAT_NV12)
     || (enPixelFormat == PIXEL_FORMAT_NV21)) {
        u32AlignHeight = ALIGN(height, 2);
    } else
         u32AlignHeight = height;

    if (enCmpMode == COMPRESS_MODE_NONE) {
        main_stride = ALIGN((width * u8BitWidth + 7) >> 3, u32Align);
        u32YSize = main_stride * u32AlignHeight;

        if (enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_420) {
            c_stride = ALIGN(((width >> 1) * u8BitWidth + 7) >> 3, u32Align);
            u32CSize = (c_stride * u32AlignHeight) >> 1;

            main_stride = c_stride * 2;
            u32YSize = main_stride * u32AlignHeight;
            main_size = u32YSize + (u32CSize << 1);
            pstCalConfig->plane_num = 3;
        } else if (enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_422) {
            c_stride = ALIGN(((width >> 1) * u8BitWidth + 7) >> 3, u32Align);
            u32CSize = c_stride * u32AlignHeight;

            main_size = u32YSize + (u32CSize << 1);
            pstCalConfig->plane_num = 3;
        } else if (enPixelFormat == PIXEL_FORMAT_RGB_888_PLANAR ||
                   enPixelFormat == PIXEL_FORMAT_BGR_888_PLANAR ||
                   enPixelFormat == PIXEL_FORMAT_HSV_888_PLANAR ||
                   enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_444) {
            c_stride = main_stride;
            u32CSize = u32YSize;

            main_size = u32YSize + (u32CSize << 1);
            pstCalConfig->plane_num = 3;
        } else if (enPixelFormat == PIXEL_FORMAT_RGB_BAYER_12BPP) {
            main_size = u32YSize;
            pstCalConfig->plane_num = 1;
        } else if (enPixelFormat == PIXEL_FORMAT_YUV_400) {
            main_size = u32YSize;
            pstCalConfig->plane_num = 1;
        } else if (enPixelFormat == PIXEL_FORMAT_NV12 || enPixelFormat == PIXEL_FORMAT_NV21) {
             c_stride = ALIGN((width * u8BitWidth + 7) >> 3, u32Align);
            u32CSize = (c_stride * u32AlignHeight) >> 1;

            main_size = u32YSize + u32CSize;
            pstCalConfig->plane_num = 2;
        } else if (enPixelFormat == PIXEL_FORMAT_NV16 || enPixelFormat == PIXEL_FORMAT_NV61) {
            c_stride = ALIGN((width * u8BitWidth + 7) >> 3, u32Align);
            u32CSize = c_stride * u32AlignHeight;

            main_size = u32YSize + u32CSize;
            pstCalConfig->plane_num = 2;
        } else if (enPixelFormat == PIXEL_FORMAT_YUYV || enPixelFormat == PIXEL_FORMAT_YVYU ||
                   enPixelFormat == PIXEL_FORMAT_UYVY || enPixelFormat == PIXEL_FORMAT_VYUY) {
            main_stride = ALIGN(((width * u8BitWidth + 7) >> 3) * 2, u32Align);
            u32YSize = main_stride * u32AlignHeight;
            main_size = u32YSize;
            pstCalConfig->plane_num = 1;
        } else if (enPixelFormat == PIXEL_FORMAT_ARGB_1555 || enPixelFormat == PIXEL_FORMAT_ARGB_4444) {
            // packed format
            main_stride = ALIGN((width * 16 + 7) >> 3, u32Align);
            u32YSize = main_stride * u32AlignHeight;
            main_size = u32YSize;
            pstCalConfig->plane_num = 1;
        } else if (enPixelFormat == PIXEL_FORMAT_ARGB_8888) {
            // packed format
            main_stride = ALIGN((width * 32 + 7) >> 3, u32Align);
            u32YSize = main_stride * u32AlignHeight;
            main_size = u32YSize;
            pstCalConfig->plane_num = 1;
        } else if (enPixelFormat == PIXEL_FORMAT_FP32_C3_PLANAR) {
            main_stride = ALIGN(((width * u8BitWidth + 7) >> 3) * 4, u32Align);
            u32YSize = main_stride * u32AlignHeight;
            c_stride = main_stride;
            u32CSize = u32YSize;
            main_size = u32YSize + (u32CSize << 1);
            pstCalConfig->plane_num = 3;
        } else if (enPixelFormat == PIXEL_FORMAT_FP16_C3_PLANAR ||
                        enPixelFormat == PIXEL_FORMAT_BF16_C3_PLANAR) {
            main_stride = ALIGN(((width * u8BitWidth + 7) >> 3) * 2, u32Align);
            u32YSize = main_stride * u32AlignHeight;
            c_stride = main_stride;
            u32CSize = u32YSize;
            main_size = u32YSize + (u32CSize << 1);
            pstCalConfig->plane_num = 3;
        } else if (enPixelFormat == PIXEL_FORMAT_INT8_C3_PLANAR ||
                   enPixelFormat == PIXEL_FORMAT_UINT8_C3_PLANAR) {
            c_stride = main_stride;
            u32CSize = u32YSize;
            main_size = u32YSize + (u32CSize << 1);
            pstCalConfig->plane_num = 3;
        } else {
            // packed format
            main_stride = ALIGN(((width * u8BitWidth + 7) >> 3) * 3, u32Align);
            u32YSize = main_stride * u32AlignHeight;
            main_size = u32YSize;
            pstCalConfig->plane_num = 1;
        }

        u32VBSize = main_size;
    } else {
        // TODO: compression mode
        pstCalConfig->plane_num = 0;
    }

    pstCalConfig->vb_size     = u32VBSize;
    pstCalConfig->main_stride = main_stride;
    pstCalConfig->c_stride    = c_stride;
    pstCalConfig->main_y_size  = u32YSize;
    pstCalConfig->main_c_size  = u32CSize;
    pstCalConfig->main_size   = main_size;
    pstCalConfig->addr_align  = u32Align;

}



int bmvpu_enc_get_core_idx(int soc_idx)
{
    // todo
    return soc_idx;
}



char const * bmvpu_enc_error_string(BmVpuEncReturnCodes code)
{
    switch (code)
    {
    case BM_VPU_ENC_RETURN_CODE_OK:                        return "ok";
    case BM_VPU_ENC_RETURN_CODE_ERROR:                     return "unspecified error";
    case BM_VPU_ENC_RETURN_CODE_INVALID_PARAMS:            return "invalid params";
    case BM_VPU_ENC_RETURN_CODE_INVALID_HANDLE:            return "invalid handle";
    case BM_VPU_ENC_RETURN_CODE_INVALID_FRAMEBUFFER:       return "invalid framebuffer";
    case BM_VPU_ENC_RETURN_CODE_INSUFFICIENT_FRAMEBUFFERS: return "insufficient framebuffers";
    case BM_VPU_ENC_RETURN_CODE_INVALID_STRIDE:            return "invalid stride";
    case BM_VPU_ENC_RETURN_CODE_WRONG_CALL_SEQUENCE:       return "wrong call sequence";
    case BM_VPU_ENC_RETURN_CODE_TIMEOUT:                   return "timeout";
    case BM_VPU_ENC_RETURN_CODE_RESEND_FRAME:              return "resend frame";
    case BM_VPU_ENC_RETURN_CODE_ENC_END:                   return "enc end";
    default: return "<unknown>";
    }
}

#ifdef __linux__
static int bmve_atomic_lock = 0;
static int bmhandle_atomic_lock = 0; /* atomic lock for bmlib_handle */
#elif _WIN32
static  volatile long bmve_atomic_lock = 0;
static  volatile long bmhandle_atomic_lock = 0;
#endif


/* atomic lock for bmlib_handle operations*/
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


static void bmvpu_enc_load_bmlib_handle(int soc_idx){

    if (soc_idx > MAX_SOC_NUM)
    {
        BMVPU_ENC_ERROR("soc_idx excess MAX_SOC_NUM!\n");
        exit(0);
    }

    bm_handle_lock();
    if (g_bm_handle[soc_idx].bm_handle)
    {
        g_bm_handle[soc_idx].count += 1;
        bm_handle_unlock();
        return ;
    }

    bm_handle_t handle;
    bm_status_t ret = bm_dev_request(&handle, soc_idx);

    if (ret != BM_SUCCESS) {
      BMVPU_ENC_ERROR("Create Bm Handle Failed\n");
      bm_handle_unlock();
      exit(0);
    }
    g_bm_handle[soc_idx].count = 1;
    g_bm_handle[soc_idx].bm_handle = handle;
    bm_handle_unlock();
    return ;
}


static void bmvpu_enc_unload_bmlib_handle(int soc_idx){
    if (soc_idx > MAX_SOC_NUM)
    {
      BMVPU_ENC_ERROR("soc_idx excess MAX_SOC_NUM!\n");
      exit(0);
    }

    if (g_bm_handle[soc_idx].bm_handle)
    {
        bm_handle_lock();
        if (g_bm_handle[soc_idx].count <= 1)
        {
            bm_dev_free(g_bm_handle[soc_idx].bm_handle);
            g_bm_handle[soc_idx].count = 0;
            g_bm_handle[soc_idx].bm_handle = 0;
        }
        else
        {
            g_bm_handle[soc_idx].count -= 1;
        }
        bm_handle_unlock();
    }
    else {
        BMVPU_ENC_ERROR("Bm_handle for encode on soc %d not exist \n", soc_idx);
    }
}


bm_handle_t bmvpu_enc_get_bmlib_handle(int soc_idx)
{
    bm_handle_t handle = 0;
    if (soc_idx > MAX_SOC_NUM)
    {
        BMVPU_ENC_ERROR("soc_idx excess MAX_SOC_NUM!\n");
        exit(0);
    }

    bm_handle_lock();
    if (g_bm_handle[soc_idx].bm_handle)
    {
        handle = g_bm_handle[soc_idx].bm_handle;
        bm_handle_unlock();
        return handle;
    }
    else
    {
        bm_handle_unlock();
        return handle;
    }

    return BM_VPU_ENC_RETURN_CODE_OK;
}


int bmvpu_malloc_device_byte_heap(bm_handle_t bm_handle,
                      bm_device_mem_t *pmem, unsigned int size,
                      int heap_id_mask, int high_bit_first)
{
    int ret = 0;
    int i = 0;
    int heap_num = 0;
    ret = bm_get_gmem_total_heap_num(bm_handle, &heap_num);
    if (ret != 0)
    {
        BMVPU_ENC_ERROR("bmvpu_malloc_device_byte_heap failed!\n");
        return -1;
    }

    int available_heap_mask = 0;
    for (i=0; i<heap_num; i++){
        available_heap_mask = available_heap_mask | (0x1 << i);
    }

    int enable_heap_mask = available_heap_mask & heap_id_mask;
    if (enable_heap_mask == 0x0)
    {
        BMVPU_ENC_ERROR("bmvpu_malloc_device_byte_heap failed! \n");
        return -1;
    }
    if (high_bit_first)
    {
        for (i=(heap_num-1); i>=0; i--)
        {
            if ((enable_heap_mask & (0x1<<i)))
            {
                ret = bm_malloc_device_byte_heap(bm_handle, pmem, i, size);
                if (ret != 0)
                {
                    BMVPU_ENC_ERROR("bm_malloc_device_byte_heap failed \n");
                }
                return ret;
            }
        }
    }
    else
    {
        for (i=0; i<heap_num; i++)
        {
            if ((enable_heap_mask & (0x1<<i)))
            {
                ret = bm_malloc_device_byte_heap(bm_handle, pmem, i, size);
                if (ret != 0)
                {
                    BMVPU_ENC_ERROR("bm_malloc_device_byte_heap failed \n");
                }
                return ret;
            }
        }
    }

    return BM_VPU_ENC_RETURN_CODE_OK;
}

static void* bmvpu_enc_bmlib_mmap(int soc_idx, uint64_t phy_addr, size_t len)
{
    unsigned long long vmem;
    bm_device_mem_t wrapped_dmem;
    wrapped_dmem.u.device.dmabuf_fd = 1;
    wrapped_dmem.u.device.device_addr = (unsigned long)(phy_addr);
    wrapped_dmem.flags.u.mem_type = BM_MEM_TYPE_DEVICE;
    wrapped_dmem.size = len;

    int ret = bm_mem_mmap_device_mem_no_cache(bmvpu_enc_get_bmlib_handle(soc_idx), &wrapped_dmem, &vmem);
    if (ret != BM_SUCCESS) {
        BMVPU_ENC_ERROR("bm_mem_mmap_device_mem_no_cache failed: 0x%x\n", ret);
    }
    return vmem;
}

static void bmvpu_enc_bmlib_munmap(int soc_idx, uint64_t vir_addr, size_t len)
{
    bm_mem_unmap_device_mem(bmvpu_enc_get_bmlib_handle(soc_idx), vir_addr, len);
    return;
}

int bmvpu_enc_load(int soc_idx)
{
    bmvpu_enc_load_bmlib_handle(soc_idx);

    return BM_VPU_ENC_RETURN_CODE_OK;
}

int bmvpu_enc_unload(int soc_idx)
{
    bmvpu_enc_unload_bmlib_handle(soc_idx);

    return BM_VPU_ENC_RETURN_CODE_OK;
}

// 需要用到的接口
void bmvpu_enc_set_default_open_params(BmVpuEncOpenParams *open_params,
                                       BmVpuCodecFormat codec_format)
{
    if (open_params == NULL) {
        BMVPU_ENC_ERROR("bmvpu_enc_set_default_open_params failed: open_params is null.");
        return;
    }


    memset(open_params, 0, sizeof(BmVpuEncOpenParams));

    open_params->codec_format = codec_format;
    open_params->color_format = BM_VPU_COLOR_FORMAT_YUV420;
    open_params->frame_width = 0;
    open_params->frame_height = 0;
    open_params->fps_num = 1;
    open_params->fps_den = 25;

    open_params->cqp = -1;
    open_params->bitrate = 1000*1000; // defaule 1000kbps
    open_params->vbv_buffer_size = 0;

    open_params->bg_detection    = 0;
    open_params->enable_nr       = 1;

    open_params->mb_rc           = 1;
    open_params->delta_qp        = 5;
    open_params->min_qp          = 8;
    open_params->max_qp          = 51;

    open_params->chroma_interleave = 0;

    open_params->soc_idx = 0;

    open_params->gop_preset = 5;
    open_params->intra_period = 60;

    open_params->enc_mode = 1;

    open_params->roi_enable = 0;

    open_params->max_num_merge = 2;

    open_params->enable_constrained_intra_prediction = 0;

    open_params->enable_wp = 1;

    open_params->disable_deblocking = 0;
    open_params->offset_tc          = 0;
    open_params->offset_beta        = 0;
    open_params->enable_cross_slice_boundary = 0;
    open_params->cmd_queue_depth = 4;


    if (codec_format == BM_VPU_CODEC_FORMAT_H264)
    {
        BmVpuEncH264Params* par = &(open_params->h264_params);
        par->enable_transform8x8 = 1;
    }
    else if (codec_format == BM_VPU_CODEC_FORMAT_H265)
    {
        BmVpuEncH265Params* par = &(open_params->h265_params);

        par->enable_intra_trans_skip = 1;
        par->enable_strong_intra_smoothing = 1;
        par->enable_tmvp = 1;
        par->enable_wpp = 0;
        par->enable_sao = 1;

        par->enable_intraNxN = 1;
    }




    return;
}

int bmvpu_enc_param_parse(BmVpuEncOpenParams *p, const char *name, const char *value)
{
    char *name_buf = NULL;
    int b_error = 0;
    int name_was_bool;
    int value_was_null = !value;

    if (!name)
        return BM_VPU_ENC_RETURN_CODE_INVALID_PARAMS;
    if (!value)
        value = "1";

    if (value[0] == '=')
        value++;

    if (strchr(name, '_')) // s/_/-/g
    {
        char *c;
        name_buf = strdup(name);
        if (!name_buf)
            return BM_VPU_ENC_RETURN_CODE_INVALID_PARAMS;
        while ((c = strchr(name_buf, '_')))
            *c = '-';
        name = name_buf;
    }

    if (!strncmp(name, "no", 2))
    {
        name += 2;
        if (name[0] == '-')
            name++;
        value = (!!atoi(value)) ? "0" : "1";
    }
    name_was_bool = 0;

#define OPT(STR) else if (!strcmp(name, STR))
    if (0);
    OPT("bg")
    {
        p->bg_detection    = !!atoi(value);
        BMVPU_ENC_DEBUG("bg=%d", p->bg_detection);
    }
    OPT("nr")
    {
        p->enable_nr       = !!atoi(value);
        BMVPU_ENC_DEBUG("nr=%d", p->enable_nr);
    }
    OPT("deblock")
    {
        int offset_tc   = 0;
        int offset_beta = 0;
        int ret = sscanf(value, "%d,%d", &offset_tc, &offset_beta);
        if (ret == 2) {
            p->disable_deblocking = 0;
            p->offset_tc   = offset_tc;
            p->offset_beta = offset_beta;
            BMVPU_ENC_DEBUG("deblock=%d,%d", p->offset_tc, p->offset_beta);
        } else {
            p->disable_deblocking = !atoi(value);;
            BMVPU_ENC_DEBUG("%s deblock", p->disable_deblocking?"disable":"enable");
        }
    }
    OPT("gop-preset")
    {
        p->gop_preset      = atoi(value);
        BMVPU_ENC_DEBUG("gop_preset=%d", p->gop_preset);
    }
    OPT("preset")
    {
        BMVPU_ENC_DEBUG("preset=%s", value);

        /* 0 : Custom mode, TODO
         * 1 : recommended encoder parameters (slow encoding speed, highest picture quality)
         * 2 : Boost mode (normal encoding speed, normal picture quality),
         * 3 : Fast mode (high encoding speed, low picture quality) */
        if (!strcmp(value, "fast") || !strcmp(value, "0"))
            p->enc_mode = 3;
        else if (!strcmp(value, "medium") || !strcmp(value, "1"))
            p->enc_mode = 2;
        else if (!strcmp(value, "slow") || !strcmp(value, "2"))
            p->enc_mode = 1;
        else {
            p->enc_mode = 2; /* TODO change to custom after lots of cfg parameters are added. */
            BMVPU_ENC_DEBUG("Invalid preset:%s. Use slow encoding preset instead.", value);
        }
    }
    OPT("mb-rc")
    {
        p->mb_rc = !!atoi(value);
        BMVPU_ENC_DEBUG("mb_rc=%d", p->mb_rc);
    }
    OPT("delta-qp")
    {
        p->delta_qp = atoi(value);
        BMVPU_ENC_DEBUG("delta_qp=%d", p->delta_qp);
    }
    OPT("min-qp")
    {
        p->min_qp = atoi(value);
        BMVPU_ENC_DEBUG("min_qp=%d", p->min_qp);
    }
    OPT("max-qp")
    {
        p->max_qp = atoi(value);
        BMVPU_ENC_DEBUG("max_qp=%d", p->max_qp);
    }
    OPT("qp")
    {
        int cqp = atoi(value);
        if (cqp < 0 || cqp > 51) {
            BMVPU_ENC_ERROR("Invalid qp %d", cqp);
            b_error = 1;
            goto END;
        }
        p->bitrate = 0;
        p->cqp = cqp;
        BMVPU_ENC_DEBUG("const quality mode. qp=%d", p->cqp);
    }
    OPT("bitrate")
    {
        int bitrate = atoi(value);
        if (bitrate < 0) {
            BMVPU_ENC_ERROR("Invalid bitrate %dKbps", bitrate);
            b_error = 1;
            goto END;
        }
        p->bitrate = bitrate*1000;
        BMVPU_ENC_DEBUG("bitrate=%dKbps", bitrate);
    }
    OPT("weightp")
    {
        p->enable_wp = !!atoi(value);
        BMVPU_ENC_DEBUG("weightp=%d", p->enable_wp);
    }
    OPT("roi_enable")
    {
        p->roi_enable = atoi(value);
        BMVPU_ENC_DEBUG("weightp=%d", p->roi_enable);
    }
    else
    {
        b_error = 1;
    }
#undef OPT

END:
    if (name_buf)
        free(name_buf);

    b_error |= value_was_null && !name_was_bool;
    return b_error ? -1 : 0;
}

int bmvpu_get_ext_addr()
{
    return g_vpu_ext_addr;
}

int bmvpu_enc_open(BmVpuEncoder **encoder, BmVpuEncOpenParams *open_params)
{
    int ret = 0;
    int i = 0;
    venc_chn VeChn = 0;
    venc_chn_attr_s stAttr = {0};
    venc_recv_pic_param_s stRecvParam = {0};

    pthread_mutex_lock(&enc_chn_mutex);
    // 1. chose an useable channel
    char devName[255];
    sprintf(devName, "/dev/%s", DRV_ENCODER_DEV_NAME);
    int chn_fd = bmenc_chn_open(devName);
    if (chn_fd <= 0) {
        pthread_mutex_unlock(&enc_chn_mutex);
        return BM_VPU_ENC_RETURN_CODE_INVALID_HANDLE;
    }
    // 1.1 get chn by ioctl
    int is_jpu = 0;
    ret = bmenc_ioctl_get_chn(chn_fd, &is_jpu,  &VeChn);
    if (ret != 0) {
        bmenc_chn_close(chn_fd);
        BMVPU_ENC_ERROR("get chn id failed, errno: %d, %s", errno, strerror(errno));
        pthread_mutex_unlock(&enc_chn_mutex);
        return BM_VPU_ENC_RETURN_CODE_ERROR;
    }

    ret = bmenc_ioctl_set_chn(chn_fd, &VeChn);
    if (ret != 0) {
        bmenc_chn_close(chn_fd);
        BMVPU_ENC_ERROR("get chn id failed, errno: %d, %s", errno, strerror(errno));
        pthread_mutex_unlock(&enc_chn_mutex);
        return BM_VPU_ENC_RETURN_CODE_ERROR;
    }

    // 1.2 check chn_id is using
    if (VeChn >= VENC_MAX_SOC_NUM) {
        BMVPU_ENC_ERROR("enc open is more than DRV_ENCODER_DEV_NAME(%s).\n", DRV_ENCODER_DEV_NAME);
        pthread_mutex_unlock(&enc_chn_mutex);
        return BM_VPU_ENC_RETURN_CODE_INVALID_HANDLE;
    }
    if(g_enc_chn[VeChn].is_used == 1) {
        bmenc_chn_close(chn_fd);
        BMVPU_ENC_ERROR("set chn id failed, errno: %d, %s", errno, strerror(errno));
        pthread_mutex_unlock(&enc_chn_mutex);
        return BM_VPU_ENC_RETURN_CODE_ERROR;
    }

    // 1.3 save chn_id in g_enc_chn
    g_enc_chn[VeChn].is_used = 1;
    g_enc_chn[VeChn].chn_fd = chn_fd;
    *encoder = (void*)VeChn;

    // 2. set enc params (stVencAttr)
    if (open_params->codec_format == BM_VPU_CODEC_FORMAT_H264) {
        stAttr.stVencAttr.enType = PT_H264;
    } else if (open_params->codec_format == BM_VPU_CODEC_FORMAT_H265) {
        stAttr.stVencAttr.enType = PT_H265;
    } else {
        BMVPU_ENC_ERROR("params err open_params->codec_format=%d\n", open_params->codec_format);
        pthread_mutex_unlock(&enc_chn_mutex);
        return BM_VPU_ENC_RETURN_CODE_ERROR;
    }
    stAttr.stVencAttr.u32MaxPicWidth     = open_params->frame_width;
    stAttr.stVencAttr.u32MaxPicHeight    = open_params->frame_height;
    stAttr.stVencAttr.u32BufSize         = VPU_ENC_BITSTREAM_BUFFER_SIZE;
    stAttr.stVencAttr.u32PicWidth        = open_params->frame_width;
    stAttr.stVencAttr.u32PicHeight       = open_params->frame_height;
    stAttr.stVencAttr.u32CmdQueueDepth   = open_params->cmd_queue_depth;
    stAttr.stVencAttr.enEncMode          = open_params->enc_mode;

    stAttr.stGopExAttr.u32GopPreset      = open_params->gop_preset;

    // 3. set enc params (stRcAttr)
    if (open_params->cqp >= 0) {
        // cqp
        if (open_params->codec_format == BM_VPU_CODEC_FORMAT_H264) {
            stAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264FIXQP;
        } else if (open_params->codec_format == BM_VPU_CODEC_FORMAT_H265) {
            stAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265FIXQP;
        }

        // stAttr.stRcAttr.stH264FixQp.u32GopPreset     = open_params->gop_preset;  // gop_oreset
        stAttr.stRcAttr.stH264FixQp.u32Gop           = open_params->intra_period;   // gop_size
        stAttr.stRcAttr.stH264FixQp.u32SrcFrameRate  = open_params->fps_num/open_params->fps_den;
        stAttr.stRcAttr.stH264FixQp.fr32DstFrameRate = open_params->fps_num/open_params->fps_den;
        stAttr.stRcAttr.stH264FixQp.bVariFpsEn       = 0;    // variable frame rate
        stAttr.stRcAttr.stH264FixQp.u32IQp           = open_params->cqp;
        stAttr.stRcAttr.stH264FixQp.u32PQp           = open_params->cqp;
    } else {
        if (open_params->bitrate <= 1000) {
            bmenc_chn_close(g_enc_chn[VeChn].chn_fd);
            g_enc_chn[VeChn].is_used = 0;

            BMVPU_ENC_ERROR("bmvpu_enc_open enc params is err(bitrate=%d). \n", open_params->bitrate);
            pthread_mutex_unlock(&enc_chn_mutex);
            return BM_VPU_ENC_RETURN_CODE_INVALID_PARAMS;   // 0 or -1
        }

        // cbr
        if (open_params->codec_format == BM_VPU_CODEC_FORMAT_H264) {
            stAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
        } else if (open_params->codec_format == BM_VPU_CODEC_FORMAT_H265) {
            stAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
        }
        stAttr.stRcAttr.stH264Cbr.u32BitRate         = open_params->bitrate / 1000;
        // stAttr.stRcAttr.stH264Cbr.u32GopPreset       = open_params->gop_preset;
        stAttr.stRcAttr.stH264Cbr.u32Gop             = open_params->intra_period;
        stAttr.stRcAttr.stH264Cbr.u32SrcFrameRate    = open_params->fps_num/open_params->fps_den;
        stAttr.stRcAttr.stH264Cbr.fr32DstFrameRate   = open_params->fps_num/open_params->fps_den;
        stAttr.stRcAttr.stH264Cbr.bVariFpsEn         = 0;
        // stAttr.stRcAttr.stH264Cbr.u32StatTime        = 2;
    }

    // 3. set enc params (stGopAttr)
    stAttr.stGopAttr.enGopMode              = VENC_GOPMODE_NORMALP;
    stAttr.stGopAttr.stNormalP.s32IPQpDelta = 2;


    // 4. call ioctl for create channel
    ret = bmenc_ioctl_create_chn(g_enc_chn[VeChn].chn_fd, &stAttr);
    if (ret != 0) {
        BMVPU_ENC_ERROR("bmenc create chn failed %d\n", ret);
        ret = -1;
        g_enc_chn[VeChn].is_used = 0;
        pthread_mutex_unlock(&enc_chn_mutex);
        return ret;
    }

    // 5. set rc params
    venc_rc_param_s stRcParam;
    stRcParam.u32ThrdLv            = 2;
    stRcParam.bBgEnhanceEn         = 0;
    stRcParam.s32BgDeltaQp         = 0;
    stRcParam.u32RowQpDelta        = 1;
    stRcParam.s32FirstFrameStartQp = (open_params->codec_format == BM_VPU_CODEC_FORMAT_H265)?63:32;
    stRcParam.s32InitialDelay      = 1000;
    if ((open_params->codec_format == BM_VPU_CODEC_FORMAT_H264) && (stAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR)) {
        stRcParam.stParamH264Cbr.u32MaxIprop = open_params->max_qp;
        stRcParam.stParamH264Cbr.u32MinIprop = open_params->min_qp;
        stRcParam.stParamH264Cbr.u32MaxIQp   = open_params->max_qp;
        stRcParam.stParamH264Cbr.u32MinIQp   = open_params->min_qp;
        stRcParam.stParamH264Cbr.u32MaxQp    = open_params->max_qp;
        stRcParam.stParamH264Cbr.u32MinQp    = open_params->min_qp;
        stRcParam.stParamH264Cbr.s32MaxReEncodeTimes = 0;
    } else if ((open_params->codec_format == BM_VPU_CODEC_FORMAT_H265) && (stAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR)) {
        stRcParam.stParamH265Cbr.u32MaxIprop = open_params->max_qp;
        stRcParam.stParamH265Cbr.u32MinIprop = open_params->min_qp;
        stRcParam.stParamH265Cbr.u32MaxIQp   = open_params->max_qp;
        stRcParam.stParamH265Cbr.u32MinIQp   = open_params->min_qp;
        stRcParam.stParamH265Cbr.u32MaxQp    = open_params->max_qp;
        stRcParam.stParamH265Cbr.u32MinQp    = open_params->min_qp;
        stRcParam.stParamH265Cbr.s32MaxReEncodeTimes = 0;
    }
    bmenc_ioctl_set_rc_params(g_enc_chn[VeChn].chn_fd, &stRcParam);

    // 6. set vui
    if (open_params->codec_format == BM_VPU_CODEC_FORMAT_H264) {
        venc_h264_vui_s h264Vui;
        memset(&h264Vui, 0, sizeof(venc_h264_vui_s));
        h264Vui.stVuiAspectRatio.aspect_ratio_info_present_flag = 0;
        h264Vui.stVuiAspectRatio.overscan_info_present_flag = 0;
        h264Vui.stVuiVideoSignal.video_signal_type_present_flag = 0;
        h264Vui.stVuiTimeInfo.timing_info_present_flag = 1;
        if (h264Vui.stVuiTimeInfo.timing_info_present_flag) {
            h264Vui.stVuiTimeInfo.fixed_frame_rate_flag = 1;
            h264Vui.stVuiTimeInfo.num_units_in_tick = open_params->fps_den * 1000;
            h264Vui.stVuiTimeInfo.time_scale = open_params->fps_num * 1000 * 2;  // frame rate is always equal to time_scale / (2 * num_units_in_tick)
        }
        ret = bmenc_ioctl_set_h264VUI(g_enc_chn[VeChn].chn_fd, &h264Vui);
        if (ret != 0) {
            BMVPU_ENC_ERROR("bmenc set h264 vui failed %d\n", ret);
            pthread_mutex_unlock(&enc_chn_mutex);
            bmvpu_enc_close(*encoder);
            ret = -1;
            g_enc_chn[VeChn].is_used = 0;
            return ret;
        }
    } else if (open_params->codec_format == BM_VPU_CODEC_FORMAT_H265) {
        venc_h265_vui_s h265Vui;
        memset(&h265Vui, 0, sizeof(venc_h265_vui_s));
        h265Vui.stVuiAspectRatio.aspect_ratio_info_present_flag = 0;
        h265Vui.stVuiAspectRatio.overscan_info_present_flag = 0;
        h265Vui.stVuiVideoSignal.video_signal_type_present_flag = 0;
        h265Vui.stVuiTimeInfo.timing_info_present_flag = 1;
        if (h265Vui.stVuiTimeInfo.timing_info_present_flag) {
            h265Vui.stVuiTimeInfo.num_units_in_tick = open_params->fps_den * 1000;
            h265Vui.stVuiTimeInfo.time_scale = open_params->fps_num * 1000;
        }
        ret = bmenc_ioctl_set_h265VUI(g_enc_chn[VeChn].chn_fd, &h265Vui);
        if (ret != 0) {
            BMVPU_ENC_ERROR("bmenc set h265 vui failed %d\n", ret);
            pthread_mutex_unlock(&enc_chn_mutex);
            bmvpu_enc_close(*encoder);
            ret = -1;
            g_enc_chn[VeChn].is_used = 0;
            return ret;
        }
    }

    ret = bmenc_ioctl_start_recv_frame(g_enc_chn[VeChn].chn_fd, &stRecvParam);
    if (ret != 0) {
        pthread_mutex_unlock(&enc_chn_mutex);
        return BM_VPU_ENC_RETURN_CODE_ERROR;
    }


    if (g_vpu_ext_addr == 0) {
        bmenc_ioctl_get_ext_addr(g_enc_chn[VeChn].chn_fd, &g_vpu_ext_addr);
    }
    g_enc_chn[VeChn].color_format = open_params->color_format;

    pthread_mutex_unlock(&enc_chn_mutex);
    return BM_VPU_ENC_RETURN_CODE_OK;
}

int bmvpu_enc_close(BmVpuEncoder *encoder)
{
    int ret = 0;
    HANDLE VeChn = (HANDLE)encoder;
    pthread_mutex_lock(&enc_chn_mutex);
    if (g_enc_chn[VeChn].is_used == 0) {
        pthread_mutex_unlock(&enc_chn_mutex);
        return BM_VPU_ENC_RETURN_CODE_ERROR;
    }

    ret = bmenc_ioctl_destroy_chn(g_enc_chn[VeChn].chn_fd);

    if (g_enc_chn[VeChn].stStream.pstPack != NULL) {
        free(g_enc_chn[VeChn].stStream.pstPack);
        g_enc_chn[VeChn].stStream.pstPack = NULL;
    }

    BM_AVPKT * pPktHeader            = &(g_enc_chn[VeChn].p_pkt_header);
    if (pPktHeader->pkt_data.data_len > 0) {
        free(pPktHeader->pkt_data.data);
    }
    pPktHeader->pkt_data.data = NULL;
    pPktHeader->pkt_data.data_len = 0;

    BM_AVPKT * pPkt = g_enc_chn[VeChn].p_pkt_list;
    while(pPkt != NULL) {
        BM_AVPKT * pPkt_next = pPkt->pnext;
        if (pPkt->pkt_data.data_len > 0) {
            free(pPkt->pkt_data.data);
            pPkt->pkt_data.data = NULL;
            pPkt->pkt_data.data_len = 0;
        }
        free(pPkt);
        pPkt = pPkt_next;
    }
    g_enc_chn[VeChn].p_pkt_list = NULL;


    bmenc_chn_close(g_enc_chn[VeChn].chn_fd);
    g_enc_chn[VeChn].is_used = 0;
    pthread_mutex_unlock(&enc_chn_mutex);
    return BM_VPU_ENC_RETURN_CODE_OK;
}

int bmvpu_enc_get_initial_info(BmVpuEncoder *encoder, BmVpuEncInitialInfo *info)
{
    venc_chn_attr_s stAttr;
    HANDLE VeChn = -1;
    VeChn = (HANDLE)encoder;
    if (g_enc_chn[VeChn].is_used == 0) {
        return BM_VPU_ENC_RETURN_CODE_INVALID_HANDLE;
    }

    venc_initial_info_s pinfo;
    bmenc_ioctl_get_intinal_info(g_enc_chn[VeChn].chn_fd, &pinfo);

    bmenc_ioctl_get_chn_attr(g_enc_chn[VeChn].chn_fd, &stAttr);
    // info->min_num_rec_fb = 2;
    info->min_num_src_fb  = pinfo.min_num_src_fb + 1;
    vb_cal_config_s stCalConfig;
    unsigned int u32AlignWidth  = ALIGN(stAttr.stVencAttr.u32PicWidth, VENC_ALIGN_W);
    unsigned int u32AlignHeight = ALIGN(stAttr.stVencAttr.u32PicHeight, VENC_ALIGN_H);
    unsigned int u32Align       = VENC_ALIGN_W;

    if (g_enc_chn[VeChn].color_format == BM_VPU_COLOR_FORMAT_YUV420) {
        get_pic_buffer_config_internal(u32AlignWidth, u32AlignHeight, PIXEL_FORMAT_YUV_PLANAR_420,
                                      DATA_BITWIDTH_8, COMPRESS_MODE_NONE, u32Align, &stCalConfig);
    } else {
        get_pic_buffer_config_internal(u32AlignWidth, u32AlignHeight, PIXEL_FORMAT_NV12,
                                      DATA_BITWIDTH_8, COMPRESS_MODE_NONE, u32Align, &stCalConfig);
    }
    info->src_fb.width    = stAttr.stVencAttr.u32PicWidth;
    info->src_fb.height   = stAttr.stVencAttr.u32PicHeight;
    info->src_fb.y_stride = stCalConfig.main_stride; // 1920
    info->src_fb.c_stride = stCalConfig.c_stride;    // 1920/2
    info->src_fb.h_stride = u32AlignHeight;    // 1920/2

    info->src_fb.y_size   = stCalConfig.main_y_size;
    info->src_fb.c_size   = stCalConfig.main_c_size;
    info->src_fb.size     = stCalConfig.main_size;



    return BM_VPU_ENC_RETURN_CODE_OK;

}

int bmvpu_fill_framebuffer_params(BmVpuFramebuffer *fb,
                                   BmVpuFbInfo *info,
                                   bm_device_mem_t *fb_dma_buffer,
                                   int fb_id, void* context)
{
    if((fb == NULL) || (info == NULL)){
        BMVPU_ENC_ERROR("bmvpu_fill_framebuffer_params  params is err.\n");
        return -1;
    }

    fb->context = context;
    fb->myIndex = fb_id;

    fb->dma_buffer = fb_dma_buffer;

    fb->y_stride    = info->y_stride;
    fb->cbcr_stride = info->c_stride;

    fb->width  = info->width;
    fb->height = info->height;

    fb->y_offset  = 0;
    fb->cb_offset = info->y_size;
    fb->cr_offset = info->y_size + info->c_size;


    return BM_VPU_ENC_RETURN_CODE_OK;
}


int bmvpu_enc_encode_header(BmVpuEncoder *encoder, uint8_t* header_buf, int* header_len)
{
    int ret = 0;
    HANDLE VeChn = -1;
    venc_encode_header_s stEncodeHeader;

    VeChn = (HANDLE)encoder;
    if (g_enc_chn[VeChn].is_used == 0) {
        BMVPU_ENC_ERROR("bmvpu_enc_encode_header chn is not ready.\n");
        return BM_VPU_ENC_RETURN_CODE_INVALID_PARAMS;
    }
    if ((header_buf == NULL)  || (header_len == NULL)){
        BMVPU_ENC_ERROR("bmvpu_enc_encode_header header_buf or header_len is null.\n");
        return BM_VPU_ENC_RETURN_CODE_INVALID_PARAMS;
    }

    ret = bmenc_ioctl_encode_header(g_enc_chn[VeChn].chn_fd, &stEncodeHeader);
    if (ret != 0) {
        BMVPU_ENC_ERROR("bmvpu_enc_encode_header failed(ret=%x).\n", ret);
        return -1;
    }


    BM_AVPKT * pPktHeader            = &(g_enc_chn[VeChn].p_pkt_header);
    if (stEncodeHeader.u32Len > pPktHeader->pkt_data.data_len) {
        if (pPktHeader->pkt_data.data_len > 0) {
            free(pPktHeader->pkt_data.data);
        }
        pPktHeader->pkt_data.data = NULL;
        pPktHeader->pkt_data.data        = (uint8_t*)malloc(stEncodeHeader.u32Len + 1);
        pPktHeader->pkt_data.data_len    = stEncodeHeader.u32Len + 1;
    }

    memcpy(pPktHeader->pkt_data.data, stEncodeHeader.headerRbsp, stEncodeHeader.u32Len);
    pPktHeader->pkt_data.data_size   = stEncodeHeader.u32Len;
    memcpy(header_buf, stEncodeHeader.headerRbsp, pPktHeader->pkt_data.data_size);
    *header_len = pPktHeader->pkt_data.data_size;
    return 0;
}

int bmvpu_enc_send_frame(BmVpuEncoder *encoder,
                     BmVpuRawFrame const *raw_frame,
                     bool isframe_end)
{
    int ret = 0;
    HANDLE VeChn = -1;
    int s32MilliSec = 0;
    video_frame_info_s stFrame = {0};
    venc_chn_attr_s stAttr;
    venc_stream_s stStream = {0};

    VeChn = (HANDLE)encoder;
    if (g_enc_chn[VeChn].is_used == 0) {
        BMVPU_ENC_ERROR("bmvpu_enc_send_frame line=%d \n", __LINE__);
        return BM_VPU_ENC_RETURN_CODE_INVALID_PARAMS;   // 0 or -1
    }

    if (raw_frame == NULL) {
        BMVPU_ENC_ERROR("raw_frame is null. line=%d \n", __LINE__);
        return BM_VPU_ENC_RETURN_CODE_INVALID_PARAMS;   // 0 or -1
    }

    if ((raw_frame == NULL) || (raw_frame->framebuffer == NULL)) {
        isframe_end = true;
    }

    bmenc_ioctl_get_chn_attr(g_enc_chn[VeChn].chn_fd, &stAttr);

    stFrame.video_frame.width      = stAttr.stVencAttr.u32PicWidth;
    stFrame.video_frame.height     = stAttr.stVencAttr.u32PicHeight;
    if (g_enc_chn[VeChn].color_format == BM_VPU_COLOR_FORMAT_YUV420) {
        stFrame.video_frame.pixel_format = PIXEL_FORMAT_YUV_PLANAR_420;
    } else if (g_enc_chn[VeChn].color_format == BM_VPU_COLOR_FORMAT_NV12) {
        stFrame.video_frame.pixel_format = PIXEL_FORMAT_NV12;
    } else if (g_enc_chn[VeChn].color_format == BM_VPU_COLOR_FORMAT_NV21) {
        stFrame.video_frame.pixel_format = PIXEL_FORMAT_NV21;
    } else if (g_enc_chn[VeChn].color_format == BM_VPU_COLOR_FORMAT_YUV422) {
        stFrame.video_frame.pixel_format = PIXEL_FORMAT_YUV_PLANAR_422;
    }
    if (isframe_end == false) {
        unsigned int y_stride  = raw_frame->framebuffer->y_stride;
        unsigned int c_stride  = raw_frame->framebuffer->cbcr_stride;
        unsigned int h_stride  = stFrame.video_frame.height;

        if (g_enc_chn[VeChn].color_format == BM_VPU_COLOR_FORMAT_YUV420) {
            stFrame.video_frame.stride[0]  = y_stride;
            stFrame.video_frame.stride[1]  = c_stride;
            stFrame.video_frame.stride[2]  = c_stride;
            stFrame.video_frame.length[0]  = y_stride * h_stride;
            stFrame.video_frame.length[1]  = c_stride * (h_stride/2);
            stFrame.video_frame.length[2]  = c_stride * (h_stride/2);

            stFrame.video_frame.phyaddr[0] = raw_frame->framebuffer->dma_buffer->u.device.device_addr;
            stFrame.video_frame.phyaddr[1] = raw_frame->framebuffer->dma_buffer->u.device.device_addr + \
                                             raw_frame->framebuffer->cb_offset;
            stFrame.video_frame.phyaddr[2] = raw_frame->framebuffer->dma_buffer->u.device.device_addr + \
                                             raw_frame->framebuffer->cr_offset;
            stFrame.video_frame.frame_idx   = raw_frame->framebuffer->myIndex;
        } else if (g_enc_chn[VeChn].color_format == BM_VPU_COLOR_FORMAT_NV12 \
                || g_enc_chn[VeChn].color_format == BM_VPU_COLOR_FORMAT_NV21) {
            stFrame.video_frame.stride[0]  = y_stride;
            stFrame.video_frame.stride[1]  = c_stride;
            stFrame.video_frame.length[0]  = y_stride * h_stride;
            stFrame.video_frame.length[1]  = c_stride * h_stride;

            stFrame.video_frame.phyaddr[0] = raw_frame->framebuffer->dma_buffer->u.device.device_addr;
            stFrame.video_frame.phyaddr[1] = raw_frame->framebuffer->dma_buffer->u.device.device_addr + \
                                             raw_frame->framebuffer->cb_offset;
            stFrame.video_frame.frame_idx   = raw_frame->framebuffer->myIndex;
        }
    }
    stFrame.video_frame.pts        = raw_frame->pts;
    stFrame.video_frame.dts        = raw_frame->dts;
    stFrame.video_frame.srcend       = isframe_end;
    video_frame_info_ex_s stFrameEx;
    stFrameEx.pstFrame = &stFrame;
    stFrameEx.s32MilliSec = s32MilliSec;

#if 0
    unsigned char * va;
    va = devm_map(stFrame.stVFrame.u64PhyAddr[0], 1920*1080*3/2);
    // bmvpu_enc_ioctl_mmap(encoder, stFrame.stVFrame.u64PhyAddr[0], 1920*1080*3/2, &va);
    printf("pa 0X%lx \n", stFrame.stVFrame.u64PhyAddr[0]);
    printf("va 0X%x %x %x %x \n", ((uint8_t*)va)[0], ((uint8_t*)va)[1], ((uint8_t*)va)[2], ((uint8_t*)va)[3]);
    static unsigned int temp = 0;
    char filename[256] = {0};
    sprintf(filename, "mapyuv_%d.yuv", temp++);
    FILE  *fp = fopen(filename, "wb+");
    fwrite((uint8_t*)va, 1920*1080*3/2, 1, fp);
    fclose(fp);
    devm_unmap(va, 1920*1080*3/2);
    // bmvpu_enc_ioctl_unmmap(va, 1920*1080*3/2);
#endif

    if (raw_frame->customMapOpt != NULL) {
        venc_custom_map_s  roiAttr;
        roiAttr.roiAvgQp              = raw_frame->customMapOpt->roiAvgQp;
        roiAttr.customRoiMapEnable    = raw_frame->customMapOpt->customRoiMapEnable;
        roiAttr.customLambdaMapEnable = raw_frame->customMapOpt->customLambdaMapEnable;
        roiAttr.customModeMapEnable   = raw_frame->customMapOpt->customModeMapEnable;
        roiAttr.customCoefDropEnable  = raw_frame->customMapOpt->customCoefDropEnable;
        roiAttr.addrCustomMap         = raw_frame->customMapOpt->addrCustomMap;

        ret = bmenc_ioctl_roi(g_enc_chn[VeChn].chn_fd, &roiAttr);
    }

    ret = bmenc_ioctl_send_frame(g_enc_chn[VeChn].chn_fd, &stFrameEx);
    if (ret != 0) {
        BMVPU_ENC_DEBUG("bmenc send frame FAIL: 0x%x   g_enc_chn[%d].chn_fd=%d \n", ret, VeChn, g_enc_chn[VeChn].chn_fd);
        return BM_VPU_ENC_RETURN_CODE_RESEND_FRAME;
    }
    return BM_VPU_ENC_RETURN_CODE_OK;
}


int bmvpu_enc_get_stream(BmVpuEncoder *encoder,
                         BmVpuEncodedFrame *encoded_frame)
{
    int ret = 0;
    HANDLE VeChn = -1;
    int s32MilliSec = 0;

    VeChn = (HANDLE)encoder;
    if (g_enc_chn[VeChn].is_used == 0) {
        BMVPU_ENC_ERROR("cur channel is using. \n");
        return -1;
    }

    if (g_enc_chn[VeChn].stStream.pstPack == NULL) {
        int pkt_nums = 0;
        venc_chn_status_s status;
        ret = bmenc_ioctl_query_status(g_enc_chn[VeChn].chn_fd, &status);
        if (ret != 0) {
            pkt_nums = 32;
        } else {
            pkt_nums = status.u32CurPacks + 1;
        }

        g_enc_chn[VeChn].stStream.pstPack = malloc(pkt_nums * sizeof(venc_pack_s));
        if (g_enc_chn[VeChn].stStream.pstPack == NULL) {
            BMVPU_ENC_ERROR("bmenc create chn failed : 0x%x\n", ret);
            return -1;
        }
    }
    encoded_frame->data_size = 0;
    encoded_frame->src_idx   = 0;
    encoded_frame->pts       = 0;
    encoded_frame->pts       = 0;
    encoded_frame->u64CustomMapPhyAddr = 0;

    // 1. already have pkt
    BM_AVPKT * pPkt = g_enc_chn[VeChn].p_pkt_list;
    while (pPkt != NULL) {
        if (pPkt->pkt_is_used == 0) {
            pPkt = pPkt->pnext;
        } else {
            if (pPkt->pkt_data.frame_type == BM_VPU_FRAME_TYPE_I) {
                // 1.1 is I frame  copy header
                BM_AVPKT * pPktHeader = &(g_enc_chn[VeChn].p_pkt_header);
                if (encoded_frame->data_len < (pPkt->pkt_data.data_size + pPktHeader->pkt_data.data_size)) {
                    if (pPkt->pkt_data.data_size > 0)
                        free(encoded_frame->data);
                    encoded_frame->data = NULL;
                    encoded_frame->data = (uint8_t*)malloc(pPkt->pkt_data.data_size + pPktHeader->pkt_data.data_size + 1);
                    encoded_frame->data_len = pPkt->pkt_data.data_size + pPktHeader->pkt_data.data_size + 1;
                }
                memcpy(encoded_frame->data, pPktHeader->pkt_data.data, pPktHeader->pkt_data.data_size);
                memcpy(encoded_frame->data+pPktHeader->pkt_data.data_size, pPkt->pkt_data.data, pPkt->pkt_data.data_size);
                encoded_frame->data_size   = pPktHeader->pkt_data.data_size + pPkt->pkt_data.data_size;
                encoded_frame->frame_type = pPkt->pkt_data.frame_type;
                encoded_frame->pts        = pPkt->pkt_data.pts;
                encoded_frame->dts        = pPkt->pkt_data.dts;
                encoded_frame->src_idx    = pPkt->pkt_data.src_idx;
                encoded_frame->u64CustomMapPhyAddr    = pPkt->pkt_data.u64CustomMapPhyAddr;
                encoded_frame->avg_ctu_qp = pPkt->pkt_data.avg_ctu_qp;

                pPkt->pkt_is_used = 0;
            } else {

                // 1.2 is P/B frame  copy P/B frame data
                if (encoded_frame->data_len < pPkt->pkt_data.data_size) {
                    if (pPkt->pkt_data.data_size > 0)
                        free(encoded_frame->data);
                    encoded_frame->data = NULL;
                    encoded_frame->data = (uint8_t*)malloc(pPkt->pkt_data.data_size + 1);
                    encoded_frame->data_len = pPkt->pkt_data.data_size + 1;
                }
                memcpy(encoded_frame->data, pPkt->pkt_data.data, pPkt->pkt_data.data_size);
                encoded_frame->data_size  = pPkt->pkt_data.data_size;
                encoded_frame->frame_type = pPkt->pkt_data.frame_type;
                encoded_frame->pts        = pPkt->pkt_data.pts;
                encoded_frame->dts        = pPkt->pkt_data.dts;
                encoded_frame->src_idx    = pPkt->pkt_data.src_idx;
                encoded_frame->u64CustomMapPhyAddr    = pPkt->pkt_data.u64CustomMapPhyAddr;
                encoded_frame->avg_ctu_qp = pPkt->pkt_data.avg_ctu_qp;

                pPkt->pkt_is_used = 0;
            }
            return 0;
        }
    }


    // 2. get pkt by drv
    venc_stream_ex_s stStreamEx;
    stStreamEx.pstStream   = &g_enc_chn[VeChn].stStream;
    stStreamEx.s32MilliSec = s32MilliSec;
    ret = bmenc_ioctl_get_stream(g_enc_chn[VeChn].chn_fd, &stStreamEx);
    if (ret == DRV_ERR_VENC_GET_STREAM_END) {
        return BM_VPU_ENC_RETURN_CODE_ENC_END;
    }
    if (ret != 0) {
        BMVPU_ENC_DEBUG("bmenc get stream FAIL: 0x%x\n", ret);
        return -1;
    }

    // 3. return the first pkt, and then reserver othre pkt
    uint8_t *data_dst = encoded_frame->data;
    int first_pkt_data = 0;
    for (int i=0; i < g_enc_chn[VeChn].stStream.u32PackCount; i++) {
        venc_pack_s *pkt_drv;
        pkt_drv = &g_enc_chn[VeChn].stStream.pstPack[i];
        pkt_drv->pu8Addr = bmvpu_enc_bmlib_mmap(0, pkt_drv->u64PhyAddr, pkt_drv->u32Len);
        if (pkt_drv->pu8Addr == NULL) {
            continue;
        }
        // 3.1  sps pps -> copy to p_pkt_header
        if ((pkt_drv->DataType.enH264EType == H264E_NALU_SEI) || (pkt_drv->DataType.enH264EType == H264E_NALU_SPS) || \
            (pkt_drv->DataType.enH264EType == H264E_NALU_PPS) || \
            (pkt_drv->DataType.enH265EType == H265E_NALU_SEI) || (pkt_drv->DataType.enH265EType == H265E_NALU_SPS) || \
            (pkt_drv->DataType.enH265EType == H265E_NALU_PPS) || (pkt_drv->DataType.enH265EType == H265E_NALU_VPS)) {
            BM_AVPKT * pPkt = &(g_enc_chn[VeChn].p_pkt_header);
            if (pkt_drv->u32Len > pPkt->pkt_data.data_len) {
                if (pPkt->pkt_data.data_len > 0)
                    free(pPkt->pkt_data.data);
                pPkt->pkt_data.data = NULL;
                pPkt->pkt_data.data = (uint8_t*)malloc(pkt_drv->u32Len  + 1);
                pPkt->pkt_data.data_len = pkt_drv->u32Len  + 1;
            }

            memcpy(pPkt->pkt_data.data, pkt_drv->pu8Addr, pkt_drv->u32Len);
            pPkt->pkt_data.data_size   = pkt_drv->u32Len;
            // pPkt->pkt_data.frame_type = pkt_drv->frame_type;
            // pPkt->pkt_data.pts        = pkt_drv->u64PTS;
            pPkt->pkt_data.pts        = pkt_drv->u64PTS;
            pPkt->pkt_data.dts        = pkt_drv->u64DTS;
            pPkt->pkt_data.src_idx    = pkt_drv->releasFrameIdx;
            pPkt->pkt_data.u64CustomMapPhyAddr    = pkt_drv->u64CustomMapPhyAddr;
            pPkt->pkt_data.avg_ctu_qp = pkt_drv->u32AvgCtuQp;
        } else if (first_pkt_data == 0) {
        // 3.2  first data pkt  out put

            // 3.2.1 I frame -> out put sps and I frame
            if ((pkt_drv->DataType.enH264EType == H264E_NALU_ISLICE) || (pkt_drv->DataType.enH264EType == H264E_NALU_IDRSLICE) || \
                (pkt_drv->DataType.enH265EType == H265E_NALU_ISLICE) || (pkt_drv->DataType.enH265EType == H265E_NALU_IDRSLICE)) {
                // 1.1 is I frame  copy header
                BM_AVPKT * pPktHeader = &(g_enc_chn[VeChn].p_pkt_header);

                if (encoded_frame->data_len < (pkt_drv->u32Len + pPktHeader->pkt_data.data_size)) {
                    if (encoded_frame->data_len > 0)
                        free(encoded_frame->data);
                    encoded_frame->data = NULL;
                    encoded_frame->data = (uint8_t*)malloc(pkt_drv->u32Len + pPktHeader->pkt_data.data_size + 1);
                    encoded_frame->data_len = pkt_drv->u32Len + pPktHeader->pkt_data.data_size + 1;
                }
                memcpy(encoded_frame->data, pPktHeader->pkt_data.data, pPktHeader->pkt_data.data_size);
                memcpy(encoded_frame->data+pPktHeader->pkt_data.data_size, pkt_drv->pu8Addr, pkt_drv->u32Len);
                encoded_frame->data_size   = pPktHeader->pkt_data.data_size + pkt_drv->u32Len;
                encoded_frame->frame_type = BM_VPU_FRAME_TYPE_I;
                encoded_frame->pts        = pkt_drv->u64PTS;
                encoded_frame->dts        = pkt_drv->u64DTS;
                encoded_frame->src_idx    = pkt_drv->releasFrameIdx;
                encoded_frame->u64CustomMapPhyAddr    = pkt_drv->u64CustomMapPhyAddr;
                encoded_frame->avg_ctu_qp = pkt_drv->u32AvgCtuQp;
            } else {
                // 3.2.2 P Frame & B Frame
                if (encoded_frame->data_len < pkt_drv->u32Len) {
                    if (encoded_frame->data_len > 0)
                        free(encoded_frame->data);
                    encoded_frame->data = NULL;
                    encoded_frame->data = (uint8_t*)malloc(pkt_drv->u32Len + 1);
                    encoded_frame->data_len = pkt_drv->u32Len + 1;
                }
                memcpy(encoded_frame->data, pkt_drv->pu8Addr, pkt_drv->u32Len);
                encoded_frame->data_size   = pkt_drv->u32Len;
                if ((pkt_drv->DataType.enH265EType == H265E_NALU_PSLICE) || (pkt_drv->DataType.enH264EType == H264E_NALU_PSLICE)) {
                    encoded_frame->frame_type = BM_VPU_FRAME_TYPE_P;
                } else if ((pkt_drv->DataType.enH265EType == H265E_NALU_BSLICE) || (pkt_drv->DataType.enH264EType == H264E_NALU_BSLICE)) {
                    encoded_frame->frame_type = BM_VPU_FRAME_TYPE_B;
                }
                encoded_frame->pts        = pkt_drv->u64PTS;
                encoded_frame->dts        = pkt_drv->u64DTS;
                encoded_frame->src_idx    = pkt_drv->releasFrameIdx;
                encoded_frame->u64CustomMapPhyAddr    = pkt_drv->u64CustomMapPhyAddr;
                encoded_frame->avg_ctu_qp = pkt_drv->u32AvgCtuQp;

            }
            first_pkt_data = 1;
        } else {
        // 3.3 find usable node
            BM_AVPKT * pPkt = g_enc_chn[VeChn].p_pkt_list;
            bool isAddNode = true;
            // 3.3.1  find an available node and copy nal data
            while (pPkt != NULL) {
                if (pPkt->pkt_is_used == 1) {
                    // 3.3.1.1 using
                    pPkt = pPkt->pnext;
                } else if (pPkt->pkt_is_used == 0) {
                    // 3.3.1.2 idles
                    if (pkt_drv->u32Len > pPkt->pkt_data.data_len) {
                        if (pPkt->pkt_data.data_len > 0)
                            free(pPkt->pkt_data.data);
                        pPkt->pkt_data.data = NULL;
                        pPkt->pkt_data.data = (uint8_t*)malloc(pkt_drv->u32Len  + 1);
                        pPkt->pkt_data.data_len = pkt_drv->u32Len  + 1;
                    }

                    memcpy(pPkt->pkt_data.data, pkt_drv->pu8Addr, pkt_drv->u32Len);
                    pPkt->pkt_data.data_size   = pkt_drv->u32Len;
                    if ((pkt_drv->DataType.enH264EType == H264E_NALU_SEI) || (pkt_drv->DataType.enH264EType == H264E_NALU_SPS) || \
                        (pkt_drv->DataType.enH264EType == H264E_NALU_PPS) || \
                        (pkt_drv->DataType.enH265EType == H265E_NALU_SEI) || (pkt_drv->DataType.enH265EType == H265E_NALU_SPS) || \
                        (pkt_drv->DataType.enH265EType == H265E_NALU_PPS) || (pkt_drv->DataType.enH265EType == H265E_NALU_VPS)) {
                        pPkt->pkt_data.frame_type = BM_VPU_FRAME_TYPE_UNKNOWN;
                    } else if ((pkt_drv->DataType.enH264EType == H264E_NALU_ISLICE) || (pkt_drv->DataType.enH264EType == H264E_NALU_IDRSLICE) || \
                               (pkt_drv->DataType.enH265EType == H265E_NALU_ISLICE) || (pkt_drv->DataType.enH265EType == H265E_NALU_IDRSLICE)) {
                        pPkt->pkt_data.frame_type = BM_VPU_FRAME_TYPE_I;
                    } else if ((pkt_drv->DataType.enH264EType == H264E_NALU_PSLICE) || (pkt_drv->DataType.enH264EType == H264E_NALU_PSLICE) || \
                               (pkt_drv->DataType.enH265EType == H265E_NALU_PSLICE) || (pkt_drv->DataType.enH265EType == H265E_NALU_PSLICE)) {
                        pPkt->pkt_data.frame_type = BM_VPU_FRAME_TYPE_P;
                    } else if ((pkt_drv->DataType.enH264EType == H264E_NALU_BSLICE) || (pkt_drv->DataType.enH264EType == H264E_NALU_BSLICE) || \
                               (pkt_drv->DataType.enH265EType == H265E_NALU_BSLICE) || (pkt_drv->DataType.enH265EType == H265E_NALU_BSLICE)) {
                        pPkt->pkt_data.frame_type = BM_VPU_FRAME_TYPE_B;
                    }
                    pPkt->pkt_data.pts        = pkt_drv->u64PTS;
                    pPkt->pkt_data.dts        = pkt_drv->u64DTS;
                    pPkt->pkt_data.src_idx    = pkt_drv->releasFrameIdx;
                    pPkt->pkt_data.u64CustomMapPhyAddr    = pkt_drv->u64CustomMapPhyAddr;
                    pPkt->pkt_data.avg_ctu_qp = pkt_drv->u32AvgCtuQp;

                    pPkt->pkt_is_used = 1;
                    isAddNode = false;
                    break;
                }
            }

            // 3.3.2 allocate new nodes to copy data
            if (isAddNode) {
                // 3.3.2.1 new node
                BM_AVPKT *pNodeNew = malloc(sizeof(BM_AVPKT));
                memset(pNodeNew, 0, sizeof(BM_AVPKT));
                pNodeNew->pkt_data.data_len = 0;
                pNodeNew->pkt_is_used = 0;
                pNodeNew->pnext = NULL;

                // 3.3.2.2 add new node to list
                if (g_enc_chn[VeChn].p_pkt_list == NULL) {
                    g_enc_chn[VeChn].p_pkt_list = pNodeNew;
                } else {
                    BM_AVPKT * pPkt = g_enc_chn[VeChn].p_pkt_list;
                    while (pPkt != NULL) {
                        if (pPkt->pnext == NULL) {
                            pPkt->pnext = pNodeNew;
                            break;
                        } else {
                            pPkt = pPkt->pnext;
                        }
                    }
                }

                // 3.3.2.3 copy data
                if (pkt_drv->u32Len > pNodeNew->pkt_data.data_len) {
                    if (pNodeNew->pkt_data.data_len > 0)
                        free(pNodeNew->pkt_data.data);
                    pNodeNew->pkt_data.data = NULL;
                    pNodeNew->pkt_data.data = (uint8_t*)malloc(pkt_drv->u32Len  + 1);
                    pNodeNew->pkt_data.data_len = pkt_drv->u32Len  + 1;
                }

                memcpy(pNodeNew->pkt_data.data, pkt_drv->pu8Addr, pkt_drv->u32Len);
                pNodeNew->pkt_data.data_size   = pkt_drv->u32Len;
                if ((pkt_drv->DataType.enH264EType == H264E_NALU_SEI) || (pkt_drv->DataType.enH264EType == H264E_NALU_SPS) || \
                    (pkt_drv->DataType.enH264EType == H264E_NALU_PPS) || \
                    (pkt_drv->DataType.enH265EType == H265E_NALU_SEI) || (pkt_drv->DataType.enH265EType == H265E_NALU_SPS) || \
                    (pkt_drv->DataType.enH265EType == H265E_NALU_PPS) || (pkt_drv->DataType.enH265EType == H265E_NALU_VPS)) {
                    pNodeNew->pkt_data.frame_type = BM_VPU_FRAME_TYPE_UNKNOWN;
                } else if ((pkt_drv->DataType.enH264EType == H264E_NALU_ISLICE) || (pkt_drv->DataType.enH264EType == H264E_NALU_IDRSLICE) || \
                           (pkt_drv->DataType.enH265EType == H265E_NALU_ISLICE) || (pkt_drv->DataType.enH265EType == H265E_NALU_IDRSLICE)) {
                    pNodeNew->pkt_data.frame_type = BM_VPU_FRAME_TYPE_I;
                } else if ((pkt_drv->DataType.enH264EType == H264E_NALU_PSLICE) || \
                           (pkt_drv->DataType.enH265EType == H265E_NALU_PSLICE) ) {
                    pNodeNew->pkt_data.frame_type = BM_VPU_FRAME_TYPE_P;
                } else if ((pkt_drv->DataType.enH264EType == H264E_NALU_BSLICE) || \
                           (pkt_drv->DataType.enH265EType == H265E_NALU_BSLICE) ) {
                    pNodeNew->pkt_data.frame_type = BM_VPU_FRAME_TYPE_B;
                }
                pNodeNew->pkt_data.pts        = pkt_drv->u64PTS;
                pNodeNew->pkt_data.dts        = pkt_drv->u64DTS;
                pNodeNew->pkt_data.src_idx    = pkt_drv->releasFrameIdx;
                pNodeNew->pkt_data.u64CustomMapPhyAddr    = pkt_drv->u64CustomMapPhyAddr;
                pNodeNew->pkt_data.avg_ctu_qp = pkt_drv->u32AvgCtuQp;

                pNodeNew->pkt_is_used = 1;
            }
        }
    }
    // 4. release resource and unmap
    for (int i = 0; i < g_enc_chn[VeChn].stStream.u32PackCount; i++) {
        venc_pack_s *ppack;
        ppack = &g_enc_chn[VeChn].stStream.pstPack[i];
        if (ppack->u64PhyAddr && ppack->u32Len) {
            bmvpu_enc_bmlib_munmap(0, ppack->pu8Addr, ppack->u32Len);
        }
    }

    // encoded_frame->src_idx   = g_enc_chn[VeChn].stStream.pstPack->releasFrameIdx;
    ret = bmenc_ioctl_release_stream(g_enc_chn[VeChn].chn_fd, &(g_enc_chn[VeChn].stStream));
    if (ret != 0) {
        BMVPU_ENC_ERROR("bmenc release stream FAIL: 0x%x\n", ret);
        return -1;
    }

    if (g_enc_chn[VeChn].stStream.u32PackCount > 0)
        return 0;
    return -1;
}


int bmvpu_enc_encode(BmVpuEncoder *encoder,
                     BmVpuRawFrame const *raw_frame,
                     BmVpuEncodedFrame *encoded_frame,
                     bool isframe_end)
{
    return 0;
}

int bmvpu_enc_set_roiinfo(BmVpuEncoder *encoder)
{
    int ret = 0;
    HANDLE VeChn = -1;
    venc_custom_map_s roiAttr;

    if (encoder == NULL) {
        return -1;
    }
    VeChn = (HANDLE)encoder->handle;
    if (g_enc_chn[VeChn].is_used == 0) {
        return -1;
    }

    ret = bmenc_ioctl_roi(g_enc_chn[VeChn].chn_fd, &roiAttr);
    return ret;
}

