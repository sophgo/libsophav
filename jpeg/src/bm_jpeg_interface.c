#ifdef _WIN32
#include <windows.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <MMSystem.h>
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

#include <string.h>
#include <errno.h>
#include "bm_ioctl.h"
#include "bm_jpeg_interface.h"
#include "bm_jpeg_internal.h"

#define UNUSED(x)    ((void)(x))
#define ALIGN(LENGTH, ALIGN_SIZE)   ( (((LENGTH) + (ALIGN_SIZE) - 1) / (ALIGN_SIZE)) * (ALIGN_SIZE) )
#define HEAP_MASK_1_2 0x06
#define FRAME_WIDTH_ALIGN 16
#define FRAME_HEIGHT_ALIGN 8
#define MAX_NUM_DEV 64
#define JPEG_CHN_START 64
#define VDEC_MAX_CHN_NUM_INF    64
typedef struct _BMLIB_HANDLE{
    bm_handle_t bm_handle;
    unsigned int count;
} BMLIB_HANDLE;

BmJpuDecReturnCodes bm_jpu_calc_framebuffer_sizes(unsigned int frame_width,
                                   unsigned int frame_height,
                                   unsigned int framebuffer_alignment,
                                   BmJpuImageFormat image_format,
                                   BmJpuFramebufferSizes *calculated_sizes)
{
    int alignment;
    int is_interleave = 0;

    if ((calculated_sizes == NULL) || (frame_width <= 0) || ((frame_height <= 0))) {
        BM_JPU_ERROR("bm_jpu_calc_framebuffer_sizes params err: calculated_sizes(0X%lx), frame_width(%d), frame_height(%d).", calculated_sizes, frame_width, frame_height);
        return BM_JPU_DEC_RETURN_CODE_INVALID_PARAMS;
    }

    calculated_sizes->aligned_frame_width  = ALIGN(frame_width, FRAME_WIDTH_ALIGN);
    calculated_sizes->y_stride = calculated_sizes->aligned_frame_width;

    switch (image_format)
    {
        case BM_JPU_IMAGE_FORMAT_YUV420P:
            calculated_sizes->aligned_frame_height = ALIGN(frame_height, 2);
            calculated_sizes->y_size = calculated_sizes->y_stride * calculated_sizes->aligned_frame_height;
            calculated_sizes->cbcr_stride = calculated_sizes->y_stride / 2;
            calculated_sizes->cbcr_size = calculated_sizes->y_size / 4;
            break;
        case BM_JPU_IMAGE_FORMAT_NV12:
        case BM_JPU_IMAGE_FORMAT_NV21:
            calculated_sizes->aligned_frame_height = ALIGN(frame_height, 2);
            calculated_sizes->y_size = calculated_sizes->y_stride * calculated_sizes->aligned_frame_height;
            calculated_sizes->cbcr_stride = calculated_sizes->y_stride;
            calculated_sizes->cbcr_size = calculated_sizes->y_size / 2;
            is_interleave = 1;
            break;
        case BM_JPU_IMAGE_FORMAT_YUV422P:
            calculated_sizes->aligned_frame_height = frame_height;
            calculated_sizes->y_size = calculated_sizes->y_stride * calculated_sizes->aligned_frame_height;
            calculated_sizes->cbcr_stride = calculated_sizes->y_stride / 2;
            calculated_sizes->cbcr_size = calculated_sizes->y_size / 2;
            break;
        case BM_JPU_IMAGE_FORMAT_NV16:
        case BM_JPU_IMAGE_FORMAT_NV61:
            calculated_sizes->aligned_frame_height = frame_height;
            calculated_sizes->y_size = calculated_sizes->y_stride * calculated_sizes->aligned_frame_height;
            calculated_sizes->cbcr_stride = calculated_sizes->y_stride;
            calculated_sizes->cbcr_size = calculated_sizes->y_size;
            is_interleave = 1;
            break;
        case BM_JPU_IMAGE_FORMAT_YUV444P:
            calculated_sizes->aligned_frame_height = frame_height;
            calculated_sizes->y_size = calculated_sizes->y_stride * calculated_sizes->aligned_frame_height;
            calculated_sizes->cbcr_stride = calculated_sizes->y_stride;
            calculated_sizes->cbcr_size = calculated_sizes->y_size;
            break;
        case BM_JPU_IMAGE_FORMAT_GRAY:
            calculated_sizes->aligned_frame_height = frame_height;
            calculated_sizes->y_size = calculated_sizes->y_stride * calculated_sizes->aligned_frame_height;
            calculated_sizes->cbcr_stride = 0;
            calculated_sizes->cbcr_size = 0;
            break;
        default:
            BM_JPU_ERROR("bm_jpu_calc_framebuffer_sizes image_format(%d) err.", image_format);
            return BM_JPU_DEC_RETURN_CODE_ERROR;
    }

    alignment = framebuffer_alignment;
    if (alignment > 1)
    {
        calculated_sizes->y_size    = ALIGN(calculated_sizes->y_size, alignment);
        calculated_sizes->cbcr_size = ALIGN(calculated_sizes->cbcr_size, alignment);
    }

    /* cbcr_size is added twice if chroma_interleave is 0, since in that case,
     * there are *two* separate planes for Cb and Cr, each one with cbcr_size bytes,
     * while in the chroma_interleave == 1 case, there is one shared chroma plane
     * for both Cb and Cr data, with cbcr_size bytes */
    calculated_sizes->total_size = calculated_sizes->y_size
                                 + (is_interleave ? calculated_sizes->cbcr_size : (calculated_sizes->cbcr_size * 2))
                                 + (alignment > 1 ? alignment : 0);  // add alignment, why???

    calculated_sizes->image_format = image_format;

    return BM_JPU_DEC_RETURN_CODE_OK;
}

void* bm_jpu_devm_map(uint64_t phys_addr, size_t len) {
    return bmjpeg_devm_map(phys_addr, len);
}

void bm_jpu_devm_unmap(void *virt_addr, size_t len) {
    bmjpeg_devm_unmap(virt_addr, len);
}


typedef struct _BM_JPEG_CTX {
    int is_used;    /* 0 is free, 1 is using */
    int chn_fd;
    int chn_id;
} BM_JPEG_CTX;

/* decode */
#define VC_DRV_DECODER_DEV_NAME "soph_vc_dec"
static pthread_mutex_t g_jpeg_dec_lock = PTHREAD_MUTEX_INITIALIZER;
static BM_JPEG_CTX g_jpeg_dec_chn[VDEC_MAX_CHN_NUM_INF] = { 0 };
static BMLIB_HANDLE g_jpeg_dec_bm_handle[MAX_NUM_DEV] = { 0 };

/* framebuffer list api */
FramebufferList* create_fb_node(int chn_id, int chn_fd, BmJpuFramebuffer *fb)
{
    FramebufferList *node = (FramebufferList *)malloc(sizeof(FramebufferList));
    node->next = NULL;
    node->fb = fb;
    node->chn_id = chn_id;
    node->chn_fd = chn_fd;

    return node;
}

void release_fb_node(FramebufferList *node)
{
    if (node != NULL) {
        if (node->fb != NULL) {
            if (node->fb->context != NULL) {
                video_frame_info_s *pstFrameInfo = (video_frame_info_s *)node->fb->context;
                BM_JPU_DEBUG("pstFrameInfo: %p\n", pstFrameInfo);
                BM_JPU_DEBUG("frame_flag = %d\n", pstFrameInfo->video_frame.frame_flag);
                BM_JPU_DEBUG("phyaddr[0] = %#lx\n", pstFrameInfo->video_frame.phyaddr[0]);
                BM_JPU_DEBUG("phyaddr[1] = %#lx\n", pstFrameInfo->video_frame.phyaddr[1]);
                BM_JPU_DEBUG("phyaddr[2] = %#lx\n", pstFrameInfo->video_frame.phyaddr[2]);

                BmJpuDecReturnCodes ret = bmjpeg_dec_ioctl_release_frame(node->chn_fd, pstFrameInfo);
                if (ret != BM_JPU_DEC_RETURN_CODE_OK) {
                    BM_JPU_WARNING("bmjpeg_dec_ioctl_release_frame failed, ret = %d", ret);
                }

                free(node->fb->context);
                node->fb->context = NULL;
            }

            if (node->fb->dma_buffer != NULL) {
                free(node->fb->dma_buffer);
                node->fb->dma_buffer = NULL;
            }

            free(node->fb);
            node->fb = NULL;
        }
        free(node);
        node = NULL;
    }

    return;
}

FramebufferList* add_fb_list(int chn_id, int chn_fd, FramebufferList *list, BmJpuFramebuffer *fb)
{
    FramebufferList *node = NULL;

    if (list == NULL) {
        return NULL;
    }

    node = create_fb_node(chn_id, chn_fd, fb);

    list->next = node;

    return node;
}

FramebufferList* del_fb_list(FramebufferList *list, BmJpuFramebuffer *fb, FramebufferList *list_curr)
{
    FramebufferList *prev = list;
    FramebufferList *curr = list;
    FramebufferList *temp = NULL;
    FramebufferList *ret = list_curr;

    if (list == NULL || fb == NULL) {
        return NULL;
    }

    while (curr != NULL) {
        if (curr->fb == fb) {
            BM_JPU_DEBUG("found framebuffer %p in list", fb);
            temp = curr->next;
            curr->next = NULL;
            prev->next = temp;

            if (list_curr == curr) {
                ret = temp == NULL ? prev : temp;
                BM_JPU_DEBUG("list_curr changed to %p", ret);
            }

            release_fb_node(curr);
            return ret;
        }
        prev = curr;
        curr = curr->next;
    }

    return list_curr;
}

void empty_fb_list(FramebufferList *node)
{
    if (node == NULL) {
        return;
    }

    empty_fb_list(node->next);
    release_fb_node(node);
}

void bm_jpu_dec_set_interrupt_timeout(BmJpuDecoder *decoder, int timeout)
{
    if (decoder == NULL) {
        BM_JPU_ERROR("bm_jpu_dec_set_interrupt_timeout params: decoder(0X%lx)", decoder);
        return;
    }
    /*
    decoder->timeout is 0  first time set timeout
        timeout > 0   set timeout
        timeout <= 0  set deafult value
    decoder->timeout not 0
        timeout > 0   set timeout
        timeout <= 0  use  lasttime value
    */
    if (decoder->timeout <= 0) {
        decoder->timeout = timeout > 0 ? timeout : 0;
    } else {
        if (timeout > 0) {
            decoder->timeout = timeout;
        }
    }
}

int bm_jpu_dec_get_channel_fd(int chn_id)
{
    int chn_fd = -1;

    pthread_mutex_lock(&g_jpeg_dec_lock);
    chn_fd = g_jpeg_dec_chn[chn_id].chn_fd;
    pthread_mutex_unlock(&g_jpeg_dec_lock);

    return chn_fd;
}

bm_handle_t bm_jpu_dec_get_bm_handle(int device_index)
{
    if (device_index >= MAX_NUM_DEV)
    {
      BM_JPU_ERROR("device_index excess MAX_NUM_DEV!\n");
      exit(0);
    }

    bm_handle_t handle = 0;
    pthread_mutex_lock(&g_jpeg_dec_lock);
    if(g_jpeg_dec_bm_handle[device_index].bm_handle) {
        handle = g_jpeg_dec_bm_handle[device_index].bm_handle;
    } else {
        BM_JPU_ERROR("Bm_handle for JPEG decode on soc %d not exist \n", device_index);
    }
    pthread_mutex_unlock(&g_jpeg_dec_lock);

    return handle;
}

BmJpuDecReturnCodes bm_jpu_dec_load(int device_index)
{
    if (device_index >= MAX_NUM_DEV)
    {
      BM_JPU_ERROR("device_index excess MAX_NUM_DEV!\n");
      exit(0);
    }
    pthread_mutex_lock(&g_jpeg_dec_lock);
#ifdef BOARD_FPGA
    bmjpeg_devm_open();
#endif

    if (g_jpeg_dec_bm_handle[device_index].bm_handle) {
        g_jpeg_dec_bm_handle[device_index].count++;
        pthread_mutex_unlock(&g_jpeg_dec_lock);
        return BM_JPU_DEC_RETURN_CODE_OK;
    } else {
        bm_handle_t handle;
        bm_status_t ret = bm_dev_request(&handle, device_index);
        if (ret != BM_SUCCESS) {
            pthread_mutex_unlock(&g_jpeg_dec_lock);
            BM_JPU_ERROR("request dev %d failed, ret: %d", device_index, ret);
            return BM_JPU_DEC_RETURN_CODE_ERROR;
        }
        g_jpeg_dec_bm_handle[device_index].bm_handle = handle;
        g_jpeg_dec_bm_handle[device_index].count = 1;
    }
    pthread_mutex_unlock(&g_jpeg_dec_lock);

    return BM_JPU_DEC_RETURN_CODE_OK;
}

BmJpuDecReturnCodes bm_jpu_dec_unload(int device_index)
{
    if (device_index >= MAX_NUM_DEV)
    {
      BM_JPU_ERROR("device_index excess MAX_NUM_DEV!\n");
      exit(0);
    }
#ifdef BOARD_FPGA
    bmjpeg_devm_close();
#endif
    pthread_mutex_lock(&g_jpeg_dec_lock);
    if (g_jpeg_dec_bm_handle[device_index].bm_handle) {
        if(g_jpeg_dec_bm_handle[device_index].count <= 1)
        {
            bm_dev_free(g_jpeg_dec_bm_handle[device_index].bm_handle);
            g_jpeg_dec_bm_handle[device_index].count = 0;
            g_jpeg_dec_bm_handle[device_index].bm_handle = 0;
        } else {
            g_jpeg_dec_bm_handle[device_index].count--;
        }
    } else {
        BM_JPU_WARNING("Bm_handle for JPEG decode on soc %d not exist \n", device_index);
    }
    pthread_mutex_unlock(&g_jpeg_dec_lock);
    return BM_JPU_DEC_RETURN_CODE_OK;
}

BmJpuDecReturnCodes bm_jpu_jpeg_dec_open(BmJpuJPEGDecoder **jpeg_decoder,
                                         BmJpuDecOpenParams *open_params,
                                         unsigned int num_extra_framebuffers)
{
    BmJpuDecReturnCodes ret = BM_JPU_DEC_RETURN_CODE_OK;
    vdec_chn_attr_s stAttr;
    vdec_chn_param_s  stParam;
    char dev_name[255];
    int chn_fd;
    int chn_id;

    UNUSED(num_extra_framebuffers);

    sprintf(dev_name, "/dev/%s", VC_DRV_DECODER_DEV_NAME);

    pthread_mutex_lock(&g_jpeg_dec_lock);
    chn_fd = open(dev_name, O_RDWR | O_DSYNC | O_CLOEXEC);
    if (chn_fd <= 0) {
        pthread_mutex_unlock(&g_jpeg_dec_lock);
        BM_JPU_ERROR("open device %s failed, chn_fd = %d, errno: %d, %s", dev_name, chn_fd, errno, strerror(errno));
        return BM_JPU_DEC_RETURN_CODE_ERROR;
    }

    ret = bmjpeg_ioctl_get_chn(chn_fd, &chn_id);
    if (ret != 0) {
        close(chn_fd);
        pthread_mutex_unlock(&g_jpeg_dec_lock);
        BM_JPU_ERROR("get chn id failed, errno: %d, %s", errno, strerror(errno));
        return BM_JPU_DEC_RETURN_CODE_ERROR;
    }

    ret = bmjpeg_ioctl_set_chn(chn_fd, &chn_id);
    if (ret != 0) {
        close(chn_fd);
        pthread_mutex_unlock(&g_jpeg_dec_lock);
        BM_JPU_ERROR("set chn id failed, errno: %d, %s", errno, strerror(errno));
        return BM_JPU_ENC_RETURN_CODE_ERROR;
    }

    chn_id -= JPEG_CHN_START;
    if (chn_id < 0 || chn_id >= VDEC_MAX_CHN_NUM_INF) {
        close(chn_fd);
        pthread_mutex_unlock(&g_jpeg_dec_lock);
        BM_JPU_ERROR("invalid chn id %d", chn_id);
        return BM_JPU_DEC_RETURN_CODE_ERROR;
    }

    if (g_jpeg_dec_chn[chn_id].is_used) {
        close(chn_fd);
        BM_JPU_ERROR("decoder channel id conflict, request: %d, using: %d", chn_id, g_jpeg_dec_chn[chn_id].chn_id);
        pthread_mutex_unlock(&g_jpeg_dec_lock);
        return BM_JPU_DEC_RETURN_CODE_ERROR;
    }

    g_jpeg_dec_chn[chn_id].is_used = 1;
    g_jpeg_dec_chn[chn_id].chn_id = chn_id;
    g_jpeg_dec_chn[chn_id].chn_fd = chn_fd;
    pthread_mutex_unlock(&g_jpeg_dec_lock);

    memset(&stAttr, 0, sizeof(vdec_chn_attr_s));
    stAttr.enType = PT_JPEG;
    stAttr.u32StreamBufSize = ALIGN(open_params->bs_buffer_size, 0x4000);  // align to 16K
    stAttr.u32FrameBufCnt = 1;
    BM_JPU_DEBUG("u32PicWidth = %u, u32PicHeight = %u, u32StreamBufSize = %#lx", stAttr.u32PicWidth, stAttr.u32PicHeight, stAttr.u32StreamBufSize);
    if(open_params->bitstream_from_user) {
        stAttr.stBufferInfo.bitstream_buffer = (buffer_info_s*)malloc(sizeof(buffer_info_s));
        stAttr.stBufferInfo.bitstream_buffer->phys_addr = open_params->bs_buffer_phys_addr;
        stAttr.stBufferInfo.bitstream_buffer->virt_addr = 0;
        stAttr.stBufferInfo.bitstream_buffer->size      = open_params->bs_buffer_size;
    }
    if(open_params->framebuffer_from_user) {
        stAttr.stBufferInfo.frame_buffer = (buffer_info_s*)malloc(sizeof(buffer_info_s));
        stAttr.stBufferInfo.frame_buffer->phys_addr     = *(open_params->framebuffer_phys_addrs);
        stAttr.stBufferInfo.frame_buffer->virt_addr     = 0;
        stAttr.stBufferInfo.frame_buffer->size          = open_params->framebuffer_size;
    }

    ret = bmjpeg_dec_ioctl_create_chn(chn_fd, &stAttr);
    if (ret != BM_JPU_DEC_RETURN_CODE_OK) {
        BM_JPU_ERROR("bmjpeg_dec_ioctl_create_chn failed, ret = %d", ret);
        goto ERR_DEC_OPEN_1;
    }

    memset(&stParam, 0, sizeof(vdec_chn_param_s));
    ret = bmjpeg_dec_ioctl_get_chn_param(chn_fd, &stParam);
    if (ret != BM_JPU_DEC_RETURN_CODE_OK) {
        BM_JPU_ERROR("bmjpeg_dec_ioctl_get_chn_param failed, ret = %d", ret);
        goto ERR_DEC_OPEN_2;
    }

    if (open_params->color_format == BM_JPU_COLOR_FORMAT_YUV420) {
        if (open_params->chroma_interleave == BM_JPU_CHROMA_FORMAT_CBCR_INTERLEAVE) {
            stParam.enPixelFormat = PIXEL_FORMAT_NV12;
        } else if (open_params->chroma_interleave == BM_JPU_CHROMA_FORMAT_CRCB_INTERLEAVE) {
            stParam.enPixelFormat = PIXEL_FORMAT_NV21;
        } else {
            stParam.enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;
        }
    } else if (open_params->color_format == BM_JPU_COLOR_FORMAT_YUV422_HORIZONTAL) {
        if (open_params->chroma_interleave == BM_JPU_CHROMA_FORMAT_CBCR_INTERLEAVE) {
            stParam.enPixelFormat = PIXEL_FORMAT_NV16;
        } else if (open_params->chroma_interleave == BM_JPU_CHROMA_FORMAT_CRCB_INTERLEAVE) {
            stParam.enPixelFormat = PIXEL_FORMAT_NV61;
        } else {
            stParam.enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_422;
        }
    } else if (open_params->color_format == BM_JPU_COLOR_FORMAT_YUV444) {
        stParam.enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_444;
    } else if (open_params->color_format == BM_JPU_COLOR_FORMAT_YUV400) {
        stParam.enPixelFormat = PIXEL_FORMAT_YUV_400;
    } else {
        BM_JPU_DEBUG("enPixelFormat not set, decided by driver");
    }

    stParam.stVdecPictureParam.u32HDownSampling     = open_params->scale_ratio;
    stParam.stVdecPictureParam.u32VDownSampling     = open_params->scale_ratio;
    stParam.stVdecPictureParam.s32ROIEnable         = open_params->roiEnable;
    stParam.stVdecPictureParam.s32ROIOffsetX        = open_params->roiOffsetX;
    stParam.stVdecPictureParam.s32ROIOffsetY        = open_params->roiOffsetY;
    stParam.stVdecPictureParam.s32ROIWidth          = open_params->roiWidth;
    stParam.stVdecPictureParam.s32ROIHeight         = open_params->roiHeight;
    stParam.stVdecPictureParam.s32MaxFrameWidth     = open_params->max_frame_width;
    stParam.stVdecPictureParam.s32MaxFrameHeight    = open_params->max_frame_height;
    stParam.stVdecPictureParam.s32MinFrameWidth     = open_params->min_frame_width;
    stParam.stVdecPictureParam.s32MinFrameHeight    = open_params->min_frame_height;
    if (open_params->rotationEnable) {
        stParam.stVdecPictureParam.s32RotAngle = open_params->rotationAngle;
    } else {
        stParam.stVdecPictureParam.s32RotAngle = 0;
    }
    if (open_params->mirrorEnable) {
        stParam.stVdecPictureParam.s32MirDir = open_params->mirrorDirection;
    } else {
        stParam.stVdecPictureParam.s32MirDir = 0;
    }
    BM_JPU_DEBUG("scale: u32HDownSampling = %u, u32VDownSampling = %u", stParam.stVdecPictureParam.u32HDownSampling, stParam.stVdecPictureParam.u32VDownSampling);
    BM_JPU_DEBUG("roi: s32ROIEnable = %d, s32ROIOffsetX = %d, s32ROIOffsetY = %d, s32ROIWidth = %d, s32ROIHeight = %d", stParam.stVdecPictureParam.s32ROIEnable, stParam.stVdecPictureParam.s32ROIOffsetX, stParam.stVdecPictureParam.s32ROIOffsetY, stParam.stVdecPictureParam.s32ROIWidth, stParam.stVdecPictureParam.s32ROIHeight);

    ret = bmjpeg_dec_ioctl_set_chn_param(chn_fd, &stParam);
    if (ret != BM_JPU_DEC_RETURN_CODE_OK) {
        BM_JPU_ERROR("bmjpeg_dec_ioctl_set_chn_param failed, ret = %d", ret);
        goto ERR_DEC_OPEN_2;
    }

    ret = bmjpeg_dec_ioctl_start_recv_stream(chn_fd);
    if (ret != BM_JPU_DEC_RETURN_CODE_OK) {
        BM_JPU_ERROR("bmjpeg_dec_ioctl_start_recv_stream failed, ret = %d", ret);
        goto ERR_DEC_OPEN_2;
    }

    *jpeg_decoder = (BmJpuJPEGDecoder *)malloc(sizeof(BmJpuJPEGDecoder));
    (*jpeg_decoder)->device_index = open_params->device_index;  // soc_idx
    (*jpeg_decoder)->decoder = (BmJpuDecoder *)malloc(sizeof(BmJpuDecoder));
    (*jpeg_decoder)->decoder->device_index = open_params->device_index;
    (*jpeg_decoder)->decoder->channel_id = chn_id;
    bm_jpu_dec_set_interrupt_timeout((*jpeg_decoder)->decoder, open_params->timeout);

    FramebufferList *head = create_fb_node(chn_id, chn_fd, NULL);  // head's framebuffer is NULL, only used to record position
    (*jpeg_decoder)->decoder->fb_list_head = head;
    (*jpeg_decoder)->decoder->fb_list_curr = head;

    return BM_JPU_DEC_RETURN_CODE_OK;

ERR_DEC_OPEN_2:
    bmjpeg_dec_ioctl_destroy_chn(chn_fd);
ERR_DEC_OPEN_1:
    close(chn_fd);

    if(open_params->bitstream_from_user) {
        free(stAttr.stBufferInfo.bitstream_buffer);
        stAttr.stBufferInfo.bitstream_buffer = NULL;
    }
    if(open_params->framebuffer_from_user) {
        free(stAttr.stBufferInfo.frame_buffer);
        stAttr.stBufferInfo.frame_buffer = NULL;
    }

    return BM_JPU_DEC_RETURN_CODE_ERROR;
}

BmJpuDecReturnCodes bm_jpu_jpeg_dec_close(BmJpuJPEGDecoder *jpeg_decoder)
{
    BmJpuDecReturnCodes ret = BM_JPU_DEC_RETURN_CODE_OK;
    int chn_id = 0;
    int chn_fd = -1;

    if (jpeg_decoder == NULL) {
        BM_JPU_ERROR("bm_jpu_jpeg_dec_close params err: jpeg_decoder is NULL");
        return BM_JPU_DEC_RETURN_CODE_INVALID_PARAMS;
    }

    empty_fb_list(jpeg_decoder->decoder->fb_list_head->next);
    release_fb_node(jpeg_decoder->decoder->fb_list_head);

    chn_id = jpeg_decoder->decoder->channel_id;
    if (chn_id < 0 || chn_id >= VDEC_MAX_CHN_NUM_INF) {
        BM_JPU_ERROR("invalid channel id: %d", chn_id);
        return BM_JPU_DEC_RETURN_CODE_ERROR;
    }

    pthread_mutex_lock(&g_jpeg_dec_lock);
    if (!g_jpeg_dec_chn[chn_id].is_used) {
        pthread_mutex_unlock(&g_jpeg_dec_lock);
        BM_JPU_ERROR("channel %d is not using", chn_id);
        return BM_JPU_DEC_RETURN_CODE_ERROR;
    }

    chn_fd = g_jpeg_dec_chn[chn_id].chn_fd;
    if (chn_fd > 0) {
        ret = bmjpeg_dec_ioctl_stop_recv_stream(chn_fd);
        if (ret != BM_JPU_DEC_RETURN_CODE_OK) {
            pthread_mutex_unlock(&g_jpeg_dec_lock);
            BM_JPU_ERROR("bmjpeg_dec_ioctl_stop_recv_stream failed, ret = %d", ret);
            return ret;
        }

        ret = bmjpeg_dec_ioctl_destroy_chn(chn_fd);
        if (ret != BM_JPU_DEC_RETURN_CODE_OK) {
            pthread_mutex_unlock(&g_jpeg_dec_lock);
            BM_JPU_ERROR("bmjpeg_dec_ioctl_destroy_chn failed, ret = %d", ret);
            return ret;
        }
        close(chn_fd);
        g_jpeg_dec_chn[chn_id].chn_fd = -1;
        g_jpeg_dec_chn[chn_id].is_used = 0;
    }
    pthread_mutex_unlock(&g_jpeg_dec_lock);

    free(jpeg_decoder->decoder);
    free(jpeg_decoder);

    return ret;
}

BmJpuDecReturnCodes bm_jpu_jpeg_dec_decode(BmJpuJPEGDecoder *jpeg_decoder, uint8_t const *jpeg_data, size_t const jpeg_data_size, int timeout, int timeout_count)
{
    BmJpuDecReturnCodes ret = BM_JPU_DEC_RETURN_CODE_OK;
    int chn_id = 0;
    int chn_fd = -1;
    vdec_stream_s stStream;
    vdec_stream_ex_s stStreamEx;

    if (jpeg_decoder == NULL) {
        BM_JPU_ERROR("bm_jpu_jpeg_dec_decode params err: jpeg_decoder is NULL");
        return BM_JPU_DEC_RETURN_CODE_INVALID_PARAMS;
    }

    bm_jpu_dec_set_interrupt_timeout(jpeg_decoder->decoder, timeout);

    chn_id = jpeg_decoder->decoder->channel_id;
    if (chn_id < 0 || chn_id >= VDEC_MAX_CHN_NUM_INF) {
        BM_JPU_ERROR("invalid channel id: %d", chn_id);
        return BM_JPU_DEC_RETURN_CODE_ERROR;
    }

    if (!g_jpeg_dec_chn[chn_id].is_used) {
        BM_JPU_ERROR("channel %d is not using", chn_id);
        return BM_JPU_DEC_RETURN_CODE_ERROR;
    }
    chn_fd = g_jpeg_dec_chn[chn_id].chn_fd;

    // set vdec stream
    stStream.u64PTS = 0;
    stStream.pu8Addr = (uint8_t *)jpeg_data;
    stStream.u32Len = jpeg_data_size;
    stStream.bEndOfFrame = 0;
    stStream.bEndOfStream = 0;
    stStream.bDisplay = 1;

    stStreamEx.pstStream = &stStream;
    stStreamEx.s32MilliSec = jpeg_decoder->decoder->timeout;

    BM_JPU_DEBUG("send stream addr: %p, len: %d", stStream.pu8Addr, stStream.u32Len);
    ret = bmjpeg_dec_ioctl_send_stream(chn_fd, &stStreamEx);
    if (ret != BM_JPU_DEC_RETURN_CODE_OK) {
        BM_JPU_ERROR("bmjpeg_dec_ioctl_send_stream failed, ret = %d", ret);
        return ret;
    }

    return ret;
}

BmJpuDecReturnCodes bm_jpu_jpeg_dec_get_info(BmJpuJPEGDecoder *jpeg_decoder, BmJpuJPEGDecInfo *info)
{
    int ret = BM_SUCCESS;
    int chn_id = 0;
    int chn_fd = -1;
    video_frame_info_s stFrameInfo;
    video_frame_info_ex_s stFrameInfoEx;
    pixel_format_e ePixelFormat;
    int chroma_interleave = 0;
    FramebufferList *list_curr = NULL;

    if (jpeg_decoder == NULL) {
        BM_JPU_ERROR("bm_jpu_jpeg_dec_get_info params err: jpeg_decoder is NULL");
        return BM_JPU_DEC_RETURN_CODE_INVALID_PARAMS;
    }

    chn_id = jpeg_decoder->decoder->channel_id;
    if (chn_id < 0 || chn_id >= VDEC_MAX_CHN_NUM_INF) {
        BM_JPU_ERROR("invalid channel id: %d", chn_id);
        return BM_JPU_DEC_RETURN_CODE_INVALID_PARAMS;
    }

    pthread_mutex_lock(&g_jpeg_dec_lock);
    if (!g_jpeg_dec_chn[chn_id].is_used) {
        pthread_mutex_unlock(&g_jpeg_dec_lock);
        BM_JPU_ERROR("channel %d is not using", chn_id);
        return BM_JPU_DEC_RETURN_CODE_INVALID_PARAMS;
    }
    chn_fd = g_jpeg_dec_chn[chn_id].chn_fd;
    pthread_mutex_unlock(&g_jpeg_dec_lock);

    stFrameInfoEx.pstFrame = &stFrameInfo;
    stFrameInfoEx.s32MilliSec = -1;

    ret = bmjpeg_dec_ioctl_get_frame(chn_fd, &stFrameInfoEx);
    if (ret != BM_JPU_DEC_RETURN_CODE_OK) {
        BM_JPU_ERROR("bmjpeg_dec_ioctl_get_frame failed, ret = %d", ret);
        return BM_JPU_DEC_RETURN_CODE_INVALID_FRAMEBUFFER;
    }

    BM_JPU_DEBUG("frame_flag = %d\n", stFrameInfoEx.pstFrame->video_frame.frame_flag);
    BM_JPU_DEBUG("pixel_format = %d\n", stFrameInfoEx.pstFrame->video_frame.pixel_format);
    BM_JPU_DEBUG("phyaddr[0] = %#lx\n", stFrameInfoEx.pstFrame->video_frame.phyaddr[0]);
    BM_JPU_DEBUG("phyaddr[1] = %#lx\n", stFrameInfoEx.pstFrame->video_frame.phyaddr[1]);
    BM_JPU_DEBUG("phyaddr[2] = %#lx\n", stFrameInfoEx.pstFrame->video_frame.phyaddr[2]);
    BM_JPU_DEBUG("stride[0] = %u\n", stFrameInfoEx.pstFrame->video_frame.stride[0]);
    BM_JPU_DEBUG("stride[1] = %u\n", stFrameInfoEx.pstFrame->video_frame.stride[1]);
    BM_JPU_DEBUG("stride[2] = %u\n", stFrameInfoEx.pstFrame->video_frame.stride[2]);
    BM_JPU_DEBUG("length[0] = %u\n", stFrameInfoEx.pstFrame->video_frame.length[0]);
    BM_JPU_DEBUG("length[1] = %u\n", stFrameInfoEx.pstFrame->video_frame.length[1]);
    BM_JPU_DEBUG("length[2] = %u\n", stFrameInfoEx.pstFrame->video_frame.length[2]);

    memset(info, 0, sizeof(BmJpuJPEGDecInfo));
    ePixelFormat = stFrameInfoEx.pstFrame->video_frame.pixel_format;
    switch (ePixelFormat) {
        case PIXEL_FORMAT_YUV_400:
            info->image_format = BM_JPU_IMAGE_FORMAT_GRAY;
            break;
        case PIXEL_FORMAT_NV16:
            info->image_format = BM_JPU_IMAGE_FORMAT_NV16;
            chroma_interleave = 1;
            break;
        case PIXEL_FORMAT_NV61:
            info->image_format = BM_JPU_IMAGE_FORMAT_NV61;
            chroma_interleave = 1;
            break;
        case PIXEL_FORMAT_YUV_PLANAR_422:
            info->image_format = BM_JPU_IMAGE_FORMAT_YUV422P;
            break;
        case PIXEL_FORMAT_YUV_PLANAR_444:
            info->image_format = BM_JPU_IMAGE_FORMAT_YUV444P;
            break;
        case PIXEL_FORMAT_NV12:
            info->image_format = BM_JPU_IMAGE_FORMAT_NV12;
            chroma_interleave = 1;
            break;
        case PIXEL_FORMAT_NV21:
            info->image_format = BM_JPU_IMAGE_FORMAT_NV21;
            chroma_interleave = 1;
            break;
        case PIXEL_FORMAT_YUV_PLANAR_420:
            info->image_format = BM_JPU_IMAGE_FORMAT_YUV420P;
            break;
        default:
            BM_JPU_ERROR("unknown pixel format: %d", ePixelFormat);
            return BM_JPU_DEC_RETURN_CODE_INVALID_PARAMS;
    }

    info->actual_frame_width = stFrameInfoEx.pstFrame->video_frame.width;
    info->actual_frame_height = stFrameInfoEx.pstFrame->video_frame.height;
    info->y_stride = stFrameInfoEx.pstFrame->video_frame.stride[0];
    info->cbcr_stride = stFrameInfoEx.pstFrame->video_frame.stride[1];
    info->y_size = stFrameInfoEx.pstFrame->video_frame.length[0];
    info->cbcr_size = stFrameInfoEx.pstFrame->video_frame.length[1];
    info->y_offset = 0;
    info->cb_offset = stFrameInfoEx.pstFrame->video_frame.phyaddr[1] - stFrameInfoEx.pstFrame->video_frame.phyaddr[0];
    info->cr_offset = stFrameInfoEx.pstFrame->video_frame.phyaddr[2] - stFrameInfoEx.pstFrame->video_frame.phyaddr[0];
    info->aligned_frame_width = info->y_stride;
    info->aligned_frame_height = info->y_size / info->y_stride;

    /* set framebuffer */
    info->framebuffer = malloc(sizeof(BmJpuFramebuffer));
    memset(info->framebuffer, 0, sizeof(BmJpuFramebuffer));

    info->framebuffer->y_stride = info->y_stride;
    info->framebuffer->cbcr_stride = info->cbcr_stride;
    info->framebuffer->y_offset = info->y_offset;
    info->framebuffer->cb_offset = info->cb_offset;
    info->framebuffer->cr_offset = info->cr_offset;

    info->framebuffer->dma_buffer = malloc(sizeof(bm_device_mem_t));
    info->framebuffer->dma_buffer->u.device.device_addr = stFrameInfoEx.pstFrame->video_frame.phyaddr[0];
    BM_JPU_DEBUG("info->framebuffer->dma_buffer->u.device.device_addr: %#lx\n", info->framebuffer->dma_buffer->u.device.device_addr);

    if (info->image_format == BM_JPU_IMAGE_FORMAT_GRAY) {
        info->framebuffer->dma_buffer->size = stFrameInfoEx.pstFrame->video_frame.length[0];
    } else if (info->image_format == BM_JPU_IMAGE_FORMAT_YUV444P) {
        info->framebuffer->dma_buffer->size = stFrameInfoEx.pstFrame->video_frame.length[0] + stFrameInfoEx.pstFrame->video_frame.length[1] + stFrameInfoEx.pstFrame->video_frame.length[2];
    } else {
        info->framebuffer->dma_buffer->size = stFrameInfoEx.pstFrame->video_frame.length[0] + stFrameInfoEx.pstFrame->video_frame.length[1] + (chroma_interleave ? 0 : stFrameInfoEx.pstFrame->video_frame.length[2]);
    }
    BM_JPU_DEBUG("info->framebuffer->dma_buffer->size: %u\n", info->framebuffer->dma_buffer->size);
    info->framebuffer->dma_buffer->flags.u.mem_type = BM_MEM_TYPE_DEVICE;

    // save to release later
    info->framebuffer->context = malloc(sizeof(video_frame_info_s));
    memcpy(info->framebuffer->context, stFrameInfoEx.pstFrame, sizeof(video_frame_info_s));
    BM_JPU_DEBUG("info->framebuffer->context: %p\n", info->framebuffer->context);

    list_curr = add_fb_list(chn_id, chn_fd, jpeg_decoder->decoder->fb_list_curr, info->framebuffer);
    if (list_curr == NULL) {
        BM_JPU_ERROR("add_fb_list failed, fb_list_curr is NULL");
        bmjpeg_dec_ioctl_release_frame(chn_fd, info->framebuffer->context);
        free(info->framebuffer->context);
        free(info->framebuffer->dma_buffer);
        free(info->framebuffer);
        return BM_JPU_DEC_RETURN_CODE_INVALID_PARAMS;
    }
    jpeg_decoder->decoder->fb_list_curr = list_curr;

    return BM_JPU_DEC_RETURN_CODE_OK;
}

BmJpuDecReturnCodes bm_jpu_jpeg_dec_frame_finished(BmJpuJPEGDecoder *jpeg_decoder, BmJpuFramebuffer *framebuffer)
{
    int chn_id = 0;
    FramebufferList *list_curr = NULL;

    chn_id = jpeg_decoder->decoder->channel_id;
    if (chn_id < 0 || chn_id >= VDEC_MAX_CHN_NUM_INF) {
        BM_JPU_ERROR("invalid channel id: %d", chn_id);
        return BM_JPU_DEC_RETURN_CODE_ERROR;
    }

    pthread_mutex_lock(&g_jpeg_dec_lock);
    if (!g_jpeg_dec_chn[chn_id].is_used) {
        pthread_mutex_unlock(&g_jpeg_dec_lock);
        BM_JPU_ERROR("channel %d is not using", chn_id);
        return BM_JPU_DEC_RETURN_CODE_ERROR;
    }
    pthread_mutex_unlock(&g_jpeg_dec_lock);

    BM_JPU_DEBUG("before del_fb_list: fb_list_head = %p, fb_list_curr = %p", jpeg_decoder->decoder->fb_list_head, jpeg_decoder->decoder->fb_list_curr);
    list_curr = del_fb_list(jpeg_decoder->decoder->fb_list_head, framebuffer, jpeg_decoder->decoder->fb_list_curr);
    jpeg_decoder->decoder->fb_list_curr = list_curr;
    BM_JPU_DEBUG("after del_fb_list: fb_list_head = %p, fb_list_curr = %p", jpeg_decoder->decoder->fb_list_head, jpeg_decoder->decoder->fb_list_curr);

    return BM_JPU_DEC_RETURN_CODE_OK;
}

BmJpuDecReturnCodes bm_jpu_jpeg_dec_flush(BmJpuJPEGDecoder *jpeg_decoder)
{
    empty_fb_list(jpeg_decoder->decoder->fb_list_head->next);
    jpeg_decoder->decoder->fb_list_head->next = NULL;
    jpeg_decoder->decoder->fb_list_curr = jpeg_decoder->decoder->fb_list_head;

    return BM_JPU_DEC_RETURN_CODE_OK;
}


/* encode */
#define DEFAULT_BS_BUFFER_SIZE (5 * 1024 * 1024)
#define BS_MASK (1024 * 16)
#define VC_DRV_ENCODER_DEV_NAME "soph_vc_enc"
static pthread_mutex_t g_jpeg_enc_lock = PTHREAD_MUTEX_INITIALIZER;
static BM_JPEG_CTX g_jpeg_enc_chn[VENC_MAX_CHN_NUM] = { 0 };
static BMLIB_HANDLE g_jpeg_enc_bm_handle[MAX_NUM_DEV] = { 0 };
static uint8_t *g_stream_pack_array[VENC_MAX_CHN_NUM][8] = { NULL };

int bm_jpu_enc_get_channel_fd(int chn_id)
{
    int chn_fd = -1;

    pthread_mutex_lock(&g_jpeg_enc_lock);
    chn_fd = g_jpeg_enc_chn[chn_id].chn_fd;
    pthread_mutex_unlock(&g_jpeg_enc_lock);

    return chn_fd;
}

bm_handle_t bm_jpu_enc_get_bm_handle(int device_index)
{
    if (device_index >= MAX_NUM_DEV)
    {
      BM_JPU_ERROR("device_index excess MAX_NUM_DEV!\n");
      exit(0);
    }
    bm_handle_t handle = 0;

    pthread_mutex_lock(&g_jpeg_enc_lock);
    if (g_jpeg_enc_bm_handle[device_index].bm_handle) {
        handle = g_jpeg_enc_bm_handle[device_index].bm_handle;
    } else {
        BM_JPU_ERROR("Bm_handle for JPEG encode on soc %d not exist \n", device_index);
    }
    pthread_mutex_unlock(&g_jpeg_enc_lock);

    return handle;
}

BmJpuEncReturnCodes bm_jpu_enc_load(int device_index)
{
    if (device_index >= MAX_NUM_DEV)
    {
      BM_JPU_ERROR("device_index excess MAX_NUM_DEV!\n");
      exit(0);
    }
    pthread_mutex_lock(&g_jpeg_enc_lock);
#ifdef BOARD_FPGA
    bmjpeg_devm_open();
#endif

    if (g_jpeg_enc_bm_handle[device_index].bm_handle) {
        g_jpeg_enc_bm_handle[device_index].count++;
        pthread_mutex_unlock(&g_jpeg_enc_lock);
        return BM_JPU_ENC_RETURN_CODE_OK;
    } else {
        bm_handle_t handle;
        bm_status_t ret = bm_dev_request(&handle, device_index);
        if (ret != BM_SUCCESS) {
            pthread_mutex_unlock(&g_jpeg_enc_lock);
            BM_JPU_ERROR("request dev %d failed, ret: %d", device_index, ret);
            return BM_JPU_ENC_RETURN_CODE_ERROR;
        }
        g_jpeg_enc_bm_handle[device_index].bm_handle = handle;
        g_jpeg_enc_bm_handle[device_index].count = 1;
    }
    pthread_mutex_unlock(&g_jpeg_enc_lock);

    return BM_JPU_ENC_RETURN_CODE_OK;
}

BmJpuEncReturnCodes bm_jpu_enc_unload(int device_index)
{
#ifdef BOARD_FPGA
    bmjpeg_devm_close();
#endif
    if (device_index >= MAX_NUM_DEV)
    {
      BM_JPU_ERROR("device_index excess MAX_NUM_DEV!\n");
      exit(0);
    }
    pthread_mutex_lock(&g_jpeg_enc_lock);
    if (g_jpeg_enc_bm_handle[device_index].bm_handle) {
        if(g_jpeg_enc_bm_handle[device_index].count <= 1)
        {
            bm_dev_free(g_jpeg_enc_bm_handle[device_index].bm_handle);
            g_jpeg_enc_bm_handle[device_index].count = 0;
            g_jpeg_enc_bm_handle[device_index].bm_handle = 0;
        } else {
            g_jpeg_enc_bm_handle[device_index].count--;
        }
    } else {
        BM_JPU_WARNING("Bm_handle for JPEG encode on soc %d not exist \n", device_index);
    }
    pthread_mutex_unlock(&g_jpeg_enc_lock);
    return BM_JPU_ENC_RETURN_CODE_OK;
}

BmJpuEncReturnCodes bm_jpu_jpeg_enc_open(BmJpuJPEGEncoder **jpeg_encoder,
                                         bm_jpu_phys_addr_t bs_buffer_phys_addr,
                                         int bs_buffer_size,
                                         int device_index)
{
    int ret = BM_JPU_ENC_RETURN_CODE_OK;
    char dev_name[255];
    int chn_fd;
    int chn_id;

    sprintf(dev_name, "/dev/%s", VC_DRV_ENCODER_DEV_NAME);

    pthread_mutex_lock(&g_jpeg_enc_lock);
    chn_fd = open(dev_name, O_RDWR | O_DSYNC | O_CLOEXEC);
    if (chn_fd <= 0) {
        pthread_mutex_unlock(&g_jpeg_enc_lock);
        BM_JPU_ERROR("open device %s failed, chnfd = %d, errno: %d, %s", dev_name, chn_fd, errno, strerror(errno));
        return BM_JPU_ENC_RETURN_CODE_ERROR;
    }

    ret = bmjpeg_ioctl_get_chn(chn_fd, &chn_id);
    if (ret != 0) {
        close(chn_fd);
        pthread_mutex_unlock(&g_jpeg_enc_lock);
        BM_JPU_ERROR("get chn id failed, errno: %d, %s", errno, strerror(errno));
        return BM_JPU_ENC_RETURN_CODE_ERROR;
    }

    ret = bmjpeg_ioctl_set_chn(chn_fd, &chn_id);
    if (ret != 0) {
        close(chn_fd);
        pthread_mutex_unlock(&g_jpeg_enc_lock);
        BM_JPU_ERROR("set chn id failed, errno: %d, %s", errno, strerror(errno));
        return BM_JPU_ENC_RETURN_CODE_ERROR;
    }

    chn_id -= JPEG_CHN_START;
    if (chn_id < 0 || chn_id >= VENC_MAX_CHN_NUM) {
        close(chn_fd);
        pthread_mutex_unlock(&g_jpeg_enc_lock);
        BM_JPU_ERROR("invalid chn id %d", chn_id);
        return BM_JPU_ENC_RETURN_CODE_ERROR;
    }

    if (g_jpeg_enc_chn[chn_id].is_used) {
        close(chn_fd);
        BM_JPU_ERROR("encoder channel id conflict, request: %d, using: %d", chn_id, g_jpeg_enc_chn[chn_id].chn_id);
        pthread_mutex_unlock(&g_jpeg_enc_lock);
        return BM_JPU_ENC_RETURN_CODE_ERROR;
    }

    g_jpeg_enc_chn[chn_id].is_used = 1;
    g_jpeg_enc_chn[chn_id].chn_id = chn_id;
    g_jpeg_enc_chn[chn_id].chn_fd = chn_fd;
    pthread_mutex_unlock(&g_jpeg_enc_lock);

    if (bs_buffer_size <= 0) {
        bs_buffer_size = DEFAULT_BS_BUFFER_SIZE;
    } else {
        bs_buffer_size = ALIGN(bs_buffer_size, BS_MASK);
    }

    *jpeg_encoder = (BmJpuJPEGEncoder *)malloc(sizeof(BmJpuJPEGEncoder));
    memset(*jpeg_encoder, 0, sizeof(BmJpuJPEGEncoder));
    (*jpeg_encoder)->device_index = device_index;  // soc_idx
    (*jpeg_encoder)->bitstream_buffer_size = bs_buffer_size;
    (*jpeg_encoder)->bitstream_buffer_alignment = 16;
    if(bs_buffer_phys_addr)
        (*jpeg_encoder)->bitstream_buffer_addr = bs_buffer_phys_addr;
    (*jpeg_encoder)->encoder = (BmJpuEncoder *)malloc(sizeof(BmJpuEncoder));
    (*jpeg_encoder)->encoder->device_index = device_index;
    (*jpeg_encoder)->encoder->channel_id = chn_id;

    return ret;
}

BmJpuEncReturnCodes bm_jpu_jpeg_enc_close(BmJpuJPEGEncoder *jpeg_encoder)
{
    int ret = BM_JPU_ENC_RETURN_CODE_OK;
    int chn_id = 0;
    int chn_fd = -1;

    if (jpeg_encoder == NULL) {
        BM_JPU_ERROR("bm_jpu_jpeg_enc_close params err: jpeg_encoder is NULL");
        return BM_JPU_ENC_RETURN_CODE_INVALID_PARAMS;
    }

    chn_id = jpeg_encoder->encoder->channel_id;
    if (chn_id < 0 || chn_id >= VENC_MAX_CHN_NUM) {
        BM_JPU_ERROR("invalid channel id: %d", chn_id);
        return BM_JPU_ENC_RETURN_CODE_ERROR;
    }

    pthread_mutex_lock(&g_jpeg_enc_lock);
    if (!g_jpeg_enc_chn[chn_id].is_used) {
        pthread_mutex_unlock(&g_jpeg_enc_lock);
        BM_JPU_ERROR("channel %d is not using", chn_id);
        return BM_JPU_ENC_RETURN_CODE_ERROR;
    }

    chn_fd = g_jpeg_enc_chn[chn_id].chn_fd;
    if (chn_fd > 0) {
        close(chn_fd);
        g_jpeg_enc_chn[chn_id].chn_fd = -1;
        g_jpeg_enc_chn[chn_id].is_used = 0;
    }
    pthread_mutex_unlock(&g_jpeg_enc_lock);

    if (jpeg_encoder->encoder != NULL) {
        free(jpeg_encoder->encoder);
    }
    free(jpeg_encoder);

    return ret;
}

BmJpuEncReturnCodes bm_jpu_jpeg_enc_encode(BmJpuJPEGEncoder *jpeg_encoder,
                                           BmJpuFramebuffer const *framebuffer,
                                           BmJpuJPEGEncParams const *params,
                                           void **acquired_handle,
                                           size_t *output_buffer_size)
{
    int ret = BM_JPU_ENC_RETURN_CODE_OK;
    int chn_id = 0;
    int chn_fd = -1;
    venc_chn_attr_s stAttr;
    venc_attr_jpeg_s *pstJpegAttr;
    venc_recv_pic_param_s stRecvParam;
    video_frame_info_ex_s stFrameEx;
    venc_stream_ex_s stStreamEx;
    video_frame_info_s stFrame;
    venc_stream_s stStream;
    venc_chn_status_s stStat;
    venc_pack_s *pPack;
    bm_handle_t bm_handle;
    bm_device_mem_t bm_mem;
    bm_status_t bm_ret = BM_SUCCESS;
    int i = 0;
    size_t total_size = 0;
    unsigned long long vaddr = 0;
    int map_count = 0;
    bm_jpu_phys_addr_t external_bs_addr = 0;
    size_t external_bs_size = 0;

    if (jpeg_encoder == NULL) {
        BM_JPU_ERROR("bm_jpu_jpeg_enc_encode params err: jpeg_encoder is NULL");
        return BM_JPU_ENC_RETURN_CODE_INVALID_PARAMS;
    }

    chn_id = jpeg_encoder->encoder->channel_id;
    if (chn_id < 0 || chn_id >= VENC_MAX_CHN_NUM) {
        BM_JPU_ERROR("invalid channel id: %d", chn_id);
        return BM_JPU_ENC_RETURN_CODE_ERROR;
    }

    if (!g_jpeg_enc_chn[chn_id].is_used) {
        BM_JPU_ERROR("channel %d is not using", chn_id);
        return BM_JPU_ENC_RETURN_CODE_ERROR;
    }

    if(params->bs_buffer_phys_addr)
        external_bs_addr = params->bs_buffer_phys_addr;
    else
        external_bs_addr = jpeg_encoder->bitstream_buffer_addr;

    if(params->bs_buffer_size)
        external_bs_size = params->bs_buffer_size;
    else
        external_bs_size = jpeg_encoder->bitstream_buffer_size;

    bm_handle = bm_jpu_enc_get_bm_handle(jpeg_encoder->device_index);
#ifndef BM_PCIE_MODE
    bm_mem_flush_device_mem(bm_handle, framebuffer->dma_buffer);
#endif
    unsigned long long base_addr = bm_mem_get_device_addr(*framebuffer->dma_buffer);
    memset(&stAttr, 0, sizeof(venc_chn_attr_s));
    memset(&stFrame, 0, sizeof(video_frame_info_s));
    memset(&stStream, 0, sizeof(venc_stream_s));

    stFrame.video_frame.stride[0] = framebuffer->y_stride;
    stFrame.video_frame.stride[1] = framebuffer->cbcr_stride;
    stFrame.video_frame.stride[2] = framebuffer->cbcr_stride;

    stFrame.video_frame.phyaddr[0] = base_addr + framebuffer->y_offset;
    stFrame.video_frame.phyaddr[1] = base_addr + framebuffer->cb_offset;
    stFrame.video_frame.phyaddr[2] = base_addr + framebuffer->cr_offset;

    switch (params->image_format) {
        case BM_JPU_IMAGE_FORMAT_YUV420P:
            stAttr.stVencAttr.enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;
            stFrame.video_frame.pixel_format = PIXEL_FORMAT_YUV_PLANAR_420;
            break;
        case BM_JPU_IMAGE_FORMAT_NV12:
            stAttr.stVencAttr.enPixelFormat = PIXEL_FORMAT_NV12;
            stFrame.video_frame.pixel_format = PIXEL_FORMAT_NV12;
            stFrame.video_frame.stride[2] = 0;
            stFrame.video_frame.phyaddr[2] = 0;
            break;
        case BM_JPU_IMAGE_FORMAT_NV21:
            stAttr.stVencAttr.enPixelFormat = PIXEL_FORMAT_NV21;
            stFrame.video_frame.pixel_format = PIXEL_FORMAT_NV21;
            stFrame.video_frame.stride[2] = 0;
            stFrame.video_frame.phyaddr[2] = 0;
            break;
        case BM_JPU_IMAGE_FORMAT_YUV422P:
            stAttr.stVencAttr.enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_422;
            stFrame.video_frame.pixel_format = PIXEL_FORMAT_YUV_PLANAR_422;
            break;
        case BM_JPU_IMAGE_FORMAT_NV16:
            stAttr.stVencAttr.enPixelFormat = PIXEL_FORMAT_NV16;
            stFrame.video_frame.pixel_format = PIXEL_FORMAT_NV16;
            stFrame.video_frame.stride[2] = 0;
            stFrame.video_frame.phyaddr[2] = 0;
            break;
        case BM_JPU_IMAGE_FORMAT_NV61:
            stAttr.stVencAttr.enPixelFormat = PIXEL_FORMAT_NV61;
            stFrame.video_frame.pixel_format = PIXEL_FORMAT_NV61;
            stFrame.video_frame.stride[2] = 0;
            stFrame.video_frame.phyaddr[2] = 0;
            break;
        case BM_JPU_IMAGE_FORMAT_YUV444P:
            stAttr.stVencAttr.enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_444;
            stFrame.video_frame.pixel_format = PIXEL_FORMAT_YUV_PLANAR_444;
            break;
        case BM_JPU_IMAGE_FORMAT_GRAY:
            stAttr.stVencAttr.enPixelFormat = PIXEL_FORMAT_YUV_400;
            stFrame.video_frame.pixel_format = PIXEL_FORMAT_YUV_400;
            stFrame.video_frame.stride[1] = 0;
            stFrame.video_frame.stride[2] = 0;
            stFrame.video_frame.phyaddr[1] = 0;
            stFrame.video_frame.phyaddr[2] = 0;
            break;
        default:
            BM_JPU_ERROR("unsupported image format: %d", params->image_format);
            return BM_JPU_ENC_RETURN_CODE_INVALID_PARAMS;
    }

    BM_JPU_DEBUG("stFrame.video_frame.stride[0] = %u", stFrame.video_frame.stride[0]);
    BM_JPU_DEBUG("stFrame.video_frame.stride[1] = %u", stFrame.video_frame.stride[1]);
    BM_JPU_DEBUG("stFrame.video_frame.stride[2] = %u", stFrame.video_frame.stride[2]);
    BM_JPU_DEBUG("stFrame.video_frame.phyaddr[0] = %#lx", stFrame.video_frame.phyaddr[0]);
    BM_JPU_DEBUG("stFrame.video_frame.phyaddr[1] = %#lx", stFrame.video_frame.phyaddr[1]);
    BM_JPU_DEBUG("stFrame.video_frame.phyaddr[2] = %#lx", stFrame.video_frame.phyaddr[2]);

    stAttr.stVencAttr.enType = PT_JPEG;
    stAttr.stVencAttr.u32MaxPicWidth = params->frame_width;
    stAttr.stVencAttr.u32MaxPicHeight = params->frame_height;
    stAttr.stVencAttr.u32PicWidth = params->frame_width;
    stAttr.stVencAttr.u32PicHeight = params->frame_height;
    stAttr.stVencAttr.u64ExternalBufAddr = external_bs_addr;
    stAttr.stVencAttr.u32BufSize = external_bs_size;
    stAttr.stVencAttr.bEsBufQueueEn = 1; // CVI_H26X_ES_BUFFER_QUEUE_DEFAULT
    stAttr.stVencAttr.bIsoSendFrmEn = 1; // CVI_H26X_ISO_SEND_FRAME_DEFAUL

    stAttr.stVencAttr.u32Profile = 0;
    stAttr.stVencAttr.bByFrame = 1; // get stream mode is slice mode or frame mode ?

    pstJpegAttr = &stAttr.stVencAttr.stAttrJpege;
    pstJpegAttr->bSupportDCF = 0;
    pstJpegAttr->stMPFCfg.u8LargeThumbNailNum = 0;
    pstJpegAttr->enReceiveMode = VENC_PIC_RECEIVE_SINGLE;

    if (params->quality_factor > 0) {
        stAttr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGFIXQP;
        stAttr.stRcAttr.stMjpegFixQp.u32Qfactor = params->quality_factor;
    }

    stAttr.stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    stAttr.stGopAttr.stNormalP.s32IPQpDelta = 0;

    if (params->rotationEnable) {
        switch (params->rotationAngle) {
            case 90:
                stAttr.stVencAttr.enRotation = ROTATION_90;
                break;
            case 180:
                stAttr.stVencAttr.enRotation = ROTATION_180;
                break;
            case 270:
                stAttr.stVencAttr.enRotation = ROTATION_270;
                break;
            default:
                BM_JPU_ERROR("unsupported rotation angle: %d", params->rotationAngle);
                return BM_JPU_ENC_RETURN_CODE_INVALID_PARAMS;
        }
    }

    if (params->mirrorEnable) {
        switch (params->mirrorDirection) {
            case 1:
                stAttr.stVencAttr.enMirrorDirextion = MIRDIR_TYPE_VER;
                break;
            case 2:
                stAttr.stVencAttr.enMirrorDirextion = MIRDIR_TYPE_HOR;
                break;
            case 3:
                stAttr.stVencAttr.enMirrorDirextion = MIRDIR_TYPE_HOR_VER;
                break;
            default:
                BM_JPU_ERROR("unsupported mirror direction: %d", params->mirrorDirection);
                return BM_JPU_ENC_RETURN_CODE_INVALID_PARAMS;
        }
    }

    pthread_mutex_lock(&g_jpeg_enc_lock);
    chn_fd = g_jpeg_enc_chn[chn_id].chn_fd;
    pthread_mutex_unlock(&g_jpeg_enc_lock);
    ret = bmjpeg_enc_ioctl_create_chn(chn_fd, &stAttr);
    if (ret != BM_JPU_ENC_RETURN_CODE_OK) {
        BM_JPU_ERROR("bmjpeg_enc_ioctl_create_chn failed, ret = %d", ret);
        return ret;
    }

    stRecvParam.s32RecvPicNum = 1;
    ret = bmjpeg_enc_ioctl_start_recv_frame(chn_fd, &stRecvParam);
    if (ret != BM_JPU_ENC_RETURN_CODE_OK) {
        BM_JPU_ERROR("bmjpeg_enc_ioctl_start_recv_frame failed, ret = %d", ret);
        goto ERR_ENC_ENCODE_1;
    }

    stFrame.video_frame.width = params->frame_width;
    stFrame.video_frame.height = params->frame_height;
    stFrame.video_frame.frame_idx = -1;
    stFrame.video_frame.pts = 0;

    stFrameEx.pstFrame = &stFrame;
    stFrameEx.s32MilliSec = params->timeout;
    ret = bmjpeg_enc_ioctl_send_frame(chn_fd, &stFrameEx);
    if (ret != BM_JPU_ENC_RETURN_CODE_OK) {
        BM_JPU_ERROR("bmjpeg_enc_ioctl_send_frame failed, ret = %d", ret);
        goto ERR_ENC_ENCODE_2;
    }

    ret = bmjpeg_enc_ioctl_query_status(chn_fd, &stStat);
    if (ret != BM_JPU_ENC_RETURN_CODE_OK) {
        BM_JPU_ERROR("bmjpeg_enc_ioctl_query_status failed, ret = %d", ret);
        goto ERR_ENC_ENCODE_2;
    }

    BM_JPU_DEBUG("u32CurPacks = %d", stStat.u32CurPacks);

    stStream.pstPack = malloc(stStat.u32CurPacks * sizeof(venc_pack_s));
    if (stStream.pstPack == NULL) {
        BM_JPU_ERROR("malloc stream packet failed!");
        goto ERR_ENC_ENCODE_2;
    }

    stStreamEx.pstStream = &stStream;
    stStreamEx.s32MilliSec = -1;
    ret = bmjpeg_enc_ioctl_get_stream(chn_fd, &stStreamEx);
    if (ret != BM_JPU_ENC_RETURN_CODE_OK) {
        BM_JPU_ERROR("bmjpeg_enc_ioctl_get_stream failed, ret = %d", ret);
        goto ERR_ENC_ENCODE_2;
    }

    for (i = 0; i < stStreamEx.pstStream->u32PackCount; i++) {
        g_stream_pack_array[chn_id][i] = NULL;
        pPack = &stStreamEx.pstStream->pstPack[i];
        if (pPack->u64PhyAddr && pPack->u32Len) {
            // get total output size
        #ifndef BM_PCIE_MODE
            g_stream_pack_array[chn_id][i] = pPack->pu8Addr;
            BM_JPU_DEBUG("origin pu8Addr: %p", pPack->pu8Addr);
            bm_mem = bm_mem_from_device(pPack->u64PhyAddr, pPack->u32Len);
            bm_ret = bm_mem_mmap_device_mem(bm_handle, &bm_mem, &vaddr);
            if (bm_ret != BM_SUCCESS) {
                BM_JPU_ERROR("bm_mem_mmap_device_mem failed, stream: %d, u64PhyAddr: %#lx, u32Len: %u", i, pPack->u64PhyAddr, pPack->u32Len);
                goto ERR_ENC_ENCODE_3;
            }
            pPack->pu8Addr = (unsigned char *)vaddr;
        #else
            unsigned char *host_va = malloc(pPack->u32Len);
            bm_mem = bm_mem_from_device(pPack->u64PhyAddr, pPack->u32Len);
            bm_memcpy_d2s_partial(bm_handle, host_va, bm_mem, pPack->u32Len);
            pPack->pu8Addr = host_va;
            int j = 0;
            if(i != 0 && pPack->u32Len > 0) {
                for (j = (pPack->u32Len - pPack->u32Offset - 1); j > 0; j--) {
                    unsigned char *tmp_ptr = pPack->pu8Addr + pPack->u32Offset + j;
                    if (tmp_ptr[0] == 0xd9 && tmp_ptr[-1] == 0xff)
                        break;
                }
                pPack->u32Len = pPack->u32Offset + j + 1;
            }
            host_va = NULL;
        #endif
            total_size += pPack->u32Len;
            BM_JPU_DEBUG("get stream %d, u64PhyAddr: %#lx, pu8Addr: %p, u32Len: %u, u32Offset: %u", i, pPack->u64PhyAddr, pPack->pu8Addr + pPack->u32Offset, pPack->u32Len - pPack->u32Offset, pPack->u32Offset);
        }
        map_count++;
    }
    BM_JPU_DEBUG("get %u bytes data", total_size);

    if (acquired_handle != NULL) {
        *output_buffer_size = total_size;
        /* Set this here to ensure that the handle is NULL if an error occurs
         * before acquire_output_buffer() is called */
        *acquired_handle = NULL;

        uint8_t *ptr_start = NULL;

        if (params->acquire_output_buffer == NULL) {
            BM_JPU_ERROR("acquire_output_buffer = 0x%llx\n\t-o need to be configured! e.g. \"-o /dev/null\"\n",
                                    (unsigned long long)params->acquire_output_buffer);
            ret = BM_JPU_ENC_RETURN_CODE_INVALID_PARAMS;
            goto ERR_ENC_ENCODE_3;
        }

        ptr_start = params->acquire_output_buffer(params->output_buffer_context, *output_buffer_size, acquired_handle);
        if (ptr_start == NULL) {
            BM_JPU_ERROR("could not acquire buffer with %llu bytes or encoded frame data", *output_buffer_size);
            goto ERR_ENC_ENCODE_3;
        }

        uint8_t *buf_offset = *acquired_handle;
        for (i = 0; i < stStream.u32PackCount; i++) {
            memcpy(buf_offset, stStream.pstPack[i].pu8Addr + stStream.pstPack[i].u32Offset, stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset);
            BM_JPU_DEBUG("%u bytes copied to acquired_handle", stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset);
            buf_offset = buf_offset + stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset;
        }
    } else {
        uint8_t *ptr_start = malloc(sizeof(uint8_t)*total_size);
        memset(ptr_start, 0, total_size);

        uint8_t *buf_offset = ptr_start;
        for (i = 0; i < stStream.u32PackCount; i++) {
            memcpy(buf_offset, stStream.pstPack[i].pu8Addr + stStream.pstPack[i].u32Offset, stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset);
            BM_JPU_DEBUG("%u bytes copied to acquired_handle", stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset);
            buf_offset = buf_offset + stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset;
        }

        if (params->write_output_data(params->output_buffer_context, ptr_start, total_size, NULL) < 0) {
            BM_JPU_ERROR("could not output encoded data with %llu bytes: write callback reported failure", total_size);
            free(ptr_start);
            ret = BM_JPU_ENC_RETURN_CODE_WRITE_CALLBACK_FAILED;
            goto ERR_ENC_ENCODE_3;
        }
        free(ptr_start);
    }

ERR_ENC_ENCODE_3:
    {
        for (i = 0; i < map_count; i++) {
            pPack = &stStream.pstPack[i];
            if (pPack->u64PhyAddr && pPack->u32Len) {
            #ifndef BM_PCIE_MODE
                bm_mem_unmap_device_mem(bm_handle, pPack->pu8Addr, pPack->u32Len);
            #else
                free(pPack->pu8Addr);
            #endif
                pPack->pu8Addr = g_stream_pack_array[chn_id][i];
            }
        }

        bmjpeg_enc_ioctl_release_stream(chn_fd, &stStream);
    }
ERR_ENC_ENCODE_2:
    if(stStream.pstPack)
        free(stStream.pstPack);

    bmjpeg_enc_ioctl_stop_recv_frame(chn_fd);

    bmjpeg_enc_ioctl_reset_chn(chn_fd);
ERR_ENC_ENCODE_1:
    bmjpeg_enc_ioctl_destroy_chn(chn_fd);

    return ret;
}