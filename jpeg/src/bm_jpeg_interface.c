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
#include "bm_jpeg_logging.h"

#define UNUSED(x)    ((void)(x))
#define ALIGN(LENGTH, ALIGN_SIZE)   ( (((LENGTH) + (ALIGN_SIZE) - 1) / (ALIGN_SIZE)) * (ALIGN_SIZE) )
#define HEAP_MASK_1_2 0x06
#define FRAME_WIDTH_ALIGN 16
#define FRAME_HEIGHT_ALIGN 8
#define MAX_NUM_DEV 64
#define JPEG_CHN_START 64


int bm_jpu_calc_framebuffer_sizes(BmJpuColorFormat color_format, unsigned int frame_width, unsigned int frame_height, unsigned int framebuffer_alignment, int chroma_interleave, BmJpuFramebufferSizes *calculated_sizes)
{
    int alignment;

    if ((calculated_sizes == NULL) || (frame_width <= 0) || ((frame_height <= 0))) {
        BM_JPU_ERROR("bm_jpu_calc_framebuffer_sizes params err: calculated_sizes(0X%lx), frame_width(%d), frame_height(%d).", calculated_sizes, frame_width, frame_height);
        return BM_JPU_DEC_RETURN_CODE_INVALID_PARAMS;
    }

    // calculated_sizes->aligned_frame_width  = BM_JPU_ALIGN_VAL_TO(frame_width, FRAME_WIDTH_ALIGN);
    // calculated_sizes->aligned_frame_height = BM_JPU_ALIGN_VAL_TO(frame_height, FRAME_HEIGHT_ALIGN);
    // calculated_sizes->aligned_frame_width  = ALIGN(frame_width, JPEGD_ALIGN_W);
    // calculated_sizes->aligned_frame_height = ALIGN(frame_height, JPEGD_ALIGN_H);

    calculated_sizes->aligned_frame_width  = ALIGN(frame_width, FRAME_WIDTH_ALIGN);
    if (color_format == BM_JPU_COLOR_FORMAT_YUV420) {
        calculated_sizes->aligned_frame_height = ALIGN(frame_height, 2);
    } else {
        calculated_sizes->aligned_frame_height = frame_height;
    }

    calculated_sizes->y_stride = calculated_sizes->aligned_frame_width;
    calculated_sizes->y_size = calculated_sizes->y_stride * calculated_sizes->aligned_frame_height;

    switch (color_format)
    {
        case BM_JPU_COLOR_FORMAT_YUV420:
            calculated_sizes->cbcr_stride = calculated_sizes->y_stride / 2;
            calculated_sizes->cbcr_size = calculated_sizes->y_size / 4;
            break;
        case BM_JPU_COLOR_FORMAT_YUV422_HORIZONTAL:
        case BM_JPU_COLOR_FORMAT_YUV422_VERTICAL:
            calculated_sizes->cbcr_stride = calculated_sizes->y_stride / 2;
            calculated_sizes->cbcr_size = calculated_sizes->y_size / 2;
            break;
        case BM_JPU_COLOR_FORMAT_YUV444:
            calculated_sizes->cbcr_stride = calculated_sizes->y_stride;
            calculated_sizes->cbcr_size = calculated_sizes->y_size;
            break;
        case BM_JPU_COLOR_FORMAT_YUV400:
            calculated_sizes->cbcr_stride = 0;
            calculated_sizes->cbcr_size = 0;
            break;
        default:
          {
            BM_JPU_ERROR("bm_jpu_calc_framebuffer_sizes color_format(%d) err.", color_format);
            return 1;
          }
    }

    if (chroma_interleave)
    {
        /* chroma_interleave != 0 means the Cb and Cr values are interleaved
         * and share one plane. The stride values are doubled compared to
         * the chroma_interleave == 0 case because the interleaving happens
         * horizontally, meaning 2 bytes in the shared chroma plane for the
         * chroma information of one pixel. */

        calculated_sizes->cbcr_stride *= 2;
        calculated_sizes->cbcr_size *= 2;
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
                                 + (chroma_interleave ? calculated_sizes->cbcr_size : (calculated_sizes->cbcr_size * 2))
                                 + (alignment > 1 ? alignment : 0);  // add alignment, why???

    calculated_sizes->chroma_interleave = chroma_interleave;

    return 0;
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
static BM_JPEG_CTX g_jpeg_dec_chn[VDEC_MAX_CHN_NUM] = { 0 };
static bm_handle_t g_jpeg_dec_bm_handle[MAX_NUM_DEV] = { 0 };
static int g_jpeg_dec_load_cnt[MAX_NUM_DEV] = { 0 };

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
                VIDEO_FRAME_INFO_S *pstFrameInfo = (VIDEO_FRAME_INFO_S *)node->fb->context;
                BM_JPU_DEBUG("pstFrameInfo: %p\n", pstFrameInfo);
                BM_JPU_DEBUG("u32FrameFlag = %d\n", pstFrameInfo->stVFrame.u32FrameFlag);
                BM_JPU_DEBUG("u64PhyAddr[0] = %#lx\n", pstFrameInfo->stVFrame.u64PhyAddr[0]);
                BM_JPU_DEBUG("u64PhyAddr[1] = %#lx\n", pstFrameInfo->stVFrame.u64PhyAddr[1]);
                BM_JPU_DEBUG("u64PhyAddr[2] = %#lx\n", pstFrameInfo->stVFrame.u64PhyAddr[2]);

                bm_status_t ret = bmjpeg_dec_ioctl_release_frame(node->chn_fd, pstFrameInfo);
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
    bm_handle_t handle = 0;

    pthread_mutex_lock(&g_jpeg_dec_lock);
    handle = g_jpeg_dec_bm_handle[device_index];
    pthread_mutex_unlock(&g_jpeg_dec_lock);

    return handle;
}

BmJpuDecReturnCodes bm_jpu_dec_load(int device_index)
{
    pthread_mutex_lock(&g_jpeg_dec_lock);
#ifdef BOARD_FPGA
    bmjpeg_devm_open();
#endif

    if (g_jpeg_dec_bm_handle[device_index] == 0) {
        bm_handle_t handle;
        bm_status_t ret = bm_dev_request(&handle, device_index);
        if (ret != BM_SUCCESS) {
            pthread_mutex_unlock(&g_jpeg_dec_lock);
            BM_JPU_ERROR("request dev %d failed, ret: %d", device_index, ret);
            return BM_JPU_DEC_RETURN_CODE_ERROR;
        }
        g_jpeg_dec_bm_handle[device_index] = handle;
        g_jpeg_dec_load_cnt[device_index] = 1;
    } else {
        g_jpeg_dec_load_cnt[device_index]++;
    }
    pthread_mutex_unlock(&g_jpeg_dec_lock);

    return BM_JPU_DEC_RETURN_CODE_OK;
}

BmJpuDecReturnCodes bm_jpu_dec_unload(int device_index)
{
    pthread_mutex_lock(&g_jpeg_dec_lock);
#ifdef BOARD_FPGA
    bmjpeg_devm_close();
#endif

    if (g_jpeg_dec_bm_handle[device_index] != 0) {
        g_jpeg_dec_load_cnt[device_index]--;
        if (g_jpeg_dec_load_cnt[device_index] <= 0) {
            bm_dev_free(g_jpeg_dec_bm_handle[device_index]);
            g_jpeg_dec_bm_handle[device_index] = 0;
            g_jpeg_dec_load_cnt[device_index] = 0;
        }
    }
    pthread_mutex_unlock(&g_jpeg_dec_lock);

    return BM_JPU_DEC_RETURN_CODE_OK;
}

BmJpuDecReturnCodes bm_jpu_jpeg_dec_open(BmJpuJPEGDecoder **jpeg_decoder,
                                         BmJpuDecOpenParams *open_params,
                                         unsigned int num_extra_framebuffers)
{
    BmJpuDecReturnCodes ret = BM_JPU_DEC_RETURN_CODE_OK;
    VDEC_CHN_ATTR_S stAttr;
    VDEC_CHN_PARAM_S stParam;
    char dev_name[255];
    int chn_fd;
    int chn_id;

    UNUSED(num_extra_framebuffers);

    sprintf(dev_name, "/dev/%s", VC_DRV_DECODER_DEV_NAME);

    pthread_mutex_lock(&g_jpeg_dec_lock);
    chn_fd = open(dev_name, O_RDWR | O_DSYNC | O_CLOEXEC);
    if (chn_fd < 0) {
        pthread_mutex_unlock(&g_jpeg_dec_lock);
        BM_JPU_ERROR("open device %s failed, errno: %d, %s", dev_name, errno, strerror(errno));
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
    if (chn_id < 0 || chn_id >= VDEC_MAX_CHN_NUM) {
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

    memset(&stAttr, 0, sizeof(VDEC_CHN_ATTR_S));
    stAttr.enType = PT_JPEG;
    stAttr.u32PicWidth = open_params->frame_width;
    stAttr.u32PicHeight = open_params->frame_height;
    stAttr.u32StreamBufSize = ALIGN(open_params->bs_buffer_size, 0x4000);  // align to 16K
    stAttr.u32FrameBufCnt = 1;
    BM_JPU_DEBUG("u32PicWidth = %u, u32PicHeight = %u, u32StreamBufSize = %#lx", stAttr.u32PicWidth, stAttr.u32PicHeight, stAttr.u32StreamBufSize);

    ret = bmjpeg_dec_ioctl_create_chn(chn_fd, &stAttr);
    if (ret != BM_JPU_DEC_RETURN_CODE_OK) {
        BM_JPU_ERROR("bmjpeg_dec_ioctl_create_chn failed, ret = %d", ret);
        goto ERR_DEC_OPEN_1;
    }

    memset(&stParam, 0, sizeof(VDEC_CHN_PARAM_S));
    ret = bmjpeg_dec_ioctl_get_chn_param(chn_fd, &stParam);
    if (ret != BM_JPU_DEC_RETURN_CODE_OK) {
        BM_JPU_ERROR("bmjpeg_dec_ioctl_get_chn_param failed, ret = %d", ret);
        goto ERR_DEC_OPEN_2;
    }

    if (open_params->color_format == BM_JPU_COLOR_FORMAT_YUV420) {
        if (open_params->chroma_interleave == CBCR_INTERLEAVE) {
            stParam.enPixelFormat = PIXEL_FORMAT_NV12;
        } else if (open_params->chroma_interleave == CRCB_INTERLEAVE) {
            stParam.enPixelFormat = PIXEL_FORMAT_NV21;
        } else {
            stParam.enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;
        }
    } else if (open_params->color_format == BM_JPU_COLOR_FORMAT_YUV422_HORIZONTAL) {
        if (open_params->chroma_interleave == CBCR_INTERLEAVE) {
            stParam.enPixelFormat = PIXEL_FORMAT_NV16;
        } else if (open_params->chroma_interleave == CRCB_INTERLEAVE) {
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

    stParam.stVdecPictureParam.u32HDownSampling = open_params->scale_ratio;
    stParam.stVdecPictureParam.u32VDownSampling = open_params->scale_ratio;
    stParam.stVdecPictureParam.s32ROIEnable = open_params->roiEnable;
    stParam.stVdecPictureParam.s32ROIOffsetX = open_params->roiOffsetX;
    stParam.stVdecPictureParam.s32ROIOffsetY = open_params->roiOffsetY;
    stParam.stVdecPictureParam.s32ROIWidth = open_params->roiWidth;
    stParam.stVdecPictureParam.s32ROIHeight = open_params->roiHeight;
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

    FramebufferList *head = create_fb_node(chn_id, chn_fd, NULL);  // head's framebuffer is NULL, only used to record position
    (*jpeg_decoder)->decoder->fb_list_head = head;
    (*jpeg_decoder)->decoder->fb_list_curr = head;

    return BM_JPU_DEC_RETURN_CODE_OK;

ERR_DEC_OPEN_2:
    bmjpeg_dec_ioctl_destroy_chn(chn_fd);
ERR_DEC_OPEN_1:
    close(chn_fd);
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
    if (chn_id < 0 || chn_id >= VDEC_MAX_CHN_NUM) {
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

BmJpuDecReturnCodes bm_jpu_jpeg_dec_decode(BmJpuJPEGDecoder *jpeg_decoder, uint8_t const *jpeg_data, size_t const jpeg_data_size)
{
    BmJpuDecReturnCodes ret = BM_JPU_DEC_RETURN_CODE_OK;
    int chn_id = 0;
    int chn_fd = -1;
    unsigned int jpeg_width = 0;
    unsigned int jpeg_height = 0;
    BmJpuColorFormat jpeg_color_format = BM_JPU_COLOR_FORMAT_YUV420;
    VDEC_CHN_ATTR_S stAttr;
    VDEC_STREAM_S stStream;
    VDEC_STREAM_EX_S stStreamEx;

    if (jpeg_decoder == NULL) {
        BM_JPU_ERROR("bm_jpu_jpeg_dec_decode params err: jpeg_decoder is NULL");
        return BM_JPU_DEC_RETURN_CODE_INVALID_PARAMS;
    }

    chn_id = jpeg_decoder->decoder->channel_id;
    if (chn_id < 0 || chn_id >= VDEC_MAX_CHN_NUM) {
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
    stStream.bEndOfFrame = CVI_FALSE;
    stStream.bEndOfStream = CVI_FALSE;
    stStream.bDisplay = 1;

    stStreamEx.pstStream = &stStream;
    stStreamEx.s32MilliSec = -1;

    BM_JPU_DEBUG("send stream addr: %p, len: %d", stStream.pu8Addr, stStream.u32Len);
    ret = bmjpeg_dec_ioctl_send_stream(chn_fd, &stStreamEx);
    if (ret != BM_JPU_DEC_RETURN_CODE_OK) {
        BM_JPU_ERROR("bmjpeg_dec_ioctl_send_stream failed, ret = %d", ret);
        return ret;
    }

    return ret;
}

void bm_jpu_jpeg_dec_get_info(BmJpuJPEGDecoder *jpeg_decoder, BmJpuJPEGDecInfo *info)
{
    int ret = BM_SUCCESS;
    int chn_id = 0;
    int chn_fd = -1;
    VIDEO_FRAME_INFO_S stFrameInfo;
    VIDEO_FRAME_INFO_EX_S stFrameInfoEx;
    PIXEL_FORMAT_E ePixelFormat;
    FramebufferList *list_curr = NULL;

    if (jpeg_decoder == NULL) {
        BM_JPU_ERROR("bm_jpu_jpeg_dec_get_info params err: jpeg_decoder is NULL");
        return;
    }

    chn_id = jpeg_decoder->decoder->channel_id;
    if (chn_id < 0 || chn_id >= VDEC_MAX_CHN_NUM) {
        BM_JPU_ERROR("invalid channel id: %d", chn_id);
        return;
    }

    pthread_mutex_lock(&g_jpeg_dec_lock);
    if (!g_jpeg_dec_chn[chn_id].is_used) {
        pthread_mutex_unlock(&g_jpeg_dec_lock);
        BM_JPU_ERROR("channel %d is not using", chn_id);
        return;
    }
    chn_fd = g_jpeg_dec_chn[chn_id].chn_fd;
    pthread_mutex_unlock(&g_jpeg_dec_lock);

    stFrameInfoEx.pstFrame = &stFrameInfo;
    stFrameInfoEx.s32MilliSec = -1;

    ret = bmjpeg_dec_ioctl_get_frame(chn_fd, &stFrameInfoEx);
    if (ret != BM_JPU_DEC_RETURN_CODE_OK) {
        BM_JPU_ERROR("bmjpeg_dec_ioctl_get_frame failed, ret = %d", ret);
        return;
    }

    BM_JPU_DEBUG("u32FrameFlag = %d\n", stFrameInfoEx.pstFrame->stVFrame.u32FrameFlag);
    BM_JPU_DEBUG("enPixelFormat = %d\n", stFrameInfoEx.pstFrame->stVFrame.enPixelFormat);
    BM_JPU_DEBUG("u64PhyAddr[0] = %#lx\n", stFrameInfoEx.pstFrame->stVFrame.u64PhyAddr[0]);
    BM_JPU_DEBUG("u64PhyAddr[1] = %#lx\n", stFrameInfoEx.pstFrame->stVFrame.u64PhyAddr[1]);
    BM_JPU_DEBUG("u64PhyAddr[2] = %#lx\n", stFrameInfoEx.pstFrame->stVFrame.u64PhyAddr[2]);
    BM_JPU_DEBUG("u32Stride[0] = %u\n", stFrameInfoEx.pstFrame->stVFrame.u32Stride[0]);
    BM_JPU_DEBUG("u32Stride[1] = %u\n", stFrameInfoEx.pstFrame->stVFrame.u32Stride[1]);
    BM_JPU_DEBUG("u32Stride[2] = %u\n", stFrameInfoEx.pstFrame->stVFrame.u32Stride[2]);
    BM_JPU_DEBUG("u32Length[0] = %u\n", stFrameInfoEx.pstFrame->stVFrame.u32Length[0]);
    BM_JPU_DEBUG("u32Length[1] = %u\n", stFrameInfoEx.pstFrame->stVFrame.u32Length[1]);
    BM_JPU_DEBUG("u32Length[2] = %u\n", stFrameInfoEx.pstFrame->stVFrame.u32Length[2]);

    memset(info, 0, sizeof(BmJpuJPEGDecInfo));
    ePixelFormat = stFrameInfoEx.pstFrame->stVFrame.enPixelFormat;
    switch (ePixelFormat) {
        case PIXEL_FORMAT_YUV_400:
            info->color_format = BM_JPU_COLOR_FORMAT_YUV400;
            info->chroma_interleave = CBCR_SEPARATED;
            break;
        case PIXEL_FORMAT_NV16:
            info->color_format = BM_JPU_COLOR_FORMAT_YUV422_HORIZONTAL;
            info->chroma_interleave = CBCR_INTERLEAVE;
            break;
        case PIXEL_FORMAT_NV61:
            info->color_format = BM_JPU_COLOR_FORMAT_YUV422_HORIZONTAL;
            info->chroma_interleave = CRCB_INTERLEAVE;
            break;
        case PIXEL_FORMAT_YUV_PLANAR_422:
            info->color_format = BM_JPU_COLOR_FORMAT_YUV422_HORIZONTAL;
            info->chroma_interleave = CBCR_SEPARATED;
            break;
        case PIXEL_FORMAT_YUV_PLANAR_444:
            info->color_format = BM_JPU_COLOR_FORMAT_YUV444;
            info->chroma_interleave = CBCR_SEPARATED;
            break;
        case PIXEL_FORMAT_NV12:
            info->color_format = BM_JPU_COLOR_FORMAT_YUV420;
            info->chroma_interleave = CBCR_INTERLEAVE;
            break;
        case PIXEL_FORMAT_NV21:
            info->color_format = BM_JPU_COLOR_FORMAT_YUV420;
            info->chroma_interleave = CRCB_INTERLEAVE;
            break;
        case PIXEL_FORMAT_YUV_PLANAR_420:
            info->color_format = BM_JPU_COLOR_FORMAT_YUV420;
            info->chroma_interleave = CBCR_SEPARATED;
            break;
        default:
            BM_JPU_ERROR("unknown pixel format: %d", ePixelFormat);
            return;
    }

    info->actual_frame_width = stFrameInfoEx.pstFrame->stVFrame.u32Width;
    info->actual_frame_height = stFrameInfoEx.pstFrame->stVFrame.u32Height;
    info->y_stride = stFrameInfoEx.pstFrame->stVFrame.u32Stride[0];
    info->cbcr_stride = stFrameInfoEx.pstFrame->stVFrame.u32Stride[1];
    info->y_size = stFrameInfoEx.pstFrame->stVFrame.u32Length[0];
    info->cbcr_size = stFrameInfoEx.pstFrame->stVFrame.u32Length[1];
    info->y_offset = 0;
    info->cb_offset = stFrameInfoEx.pstFrame->stVFrame.u64PhyAddr[1] - stFrameInfoEx.pstFrame->stVFrame.u64PhyAddr[0];
    info->cr_offset = stFrameInfoEx.pstFrame->stVFrame.u64PhyAddr[2] - stFrameInfoEx.pstFrame->stVFrame.u64PhyAddr[0];
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
    info->framebuffer->dma_buffer->u.device.device_addr = stFrameInfoEx.pstFrame->stVFrame.u64PhyAddr[0];
    BM_JPU_DEBUG("info->framebuffer->dma_buffer->u.device.device_addr: %#lx\n", info->framebuffer->dma_buffer->u.device.device_addr);
    if (info->color_format == BM_JPU_COLOR_FORMAT_YUV400) {
        info->framebuffer->dma_buffer->size = stFrameInfoEx.pstFrame->stVFrame.u32Length[0];
    } else if (info->color_format == BM_JPU_COLOR_FORMAT_YUV444) {
        info->framebuffer->dma_buffer->size = stFrameInfoEx.pstFrame->stVFrame.u32Length[0] + stFrameInfoEx.pstFrame->stVFrame.u32Length[1] + stFrameInfoEx.pstFrame->stVFrame.u32Length[2];
    } else {
        info->framebuffer->dma_buffer->size = stFrameInfoEx.pstFrame->stVFrame.u32Length[0] + stFrameInfoEx.pstFrame->stVFrame.u32Length[1] + (info->chroma_interleave ? 0 : stFrameInfoEx.pstFrame->stVFrame.u32Length[2]);
    }
    BM_JPU_DEBUG("info->framebuffer->dma_buffer->size: %u\n", info->framebuffer->dma_buffer->size);
    info->framebuffer->dma_buffer->flags.u.mem_type = BM_MEM_TYPE_DEVICE;

    // save to release later
    info->framebuffer->context = malloc(sizeof(VIDEO_FRAME_INFO_S));
    memcpy(info->framebuffer->context, stFrameInfoEx.pstFrame, sizeof(VIDEO_FRAME_INFO_S));
    BM_JPU_DEBUG("info->framebuffer->context: %p\n", info->framebuffer->context);

    list_curr = add_fb_list(chn_id, chn_fd, jpeg_decoder->decoder->fb_list_curr, info->framebuffer);
    if (list_curr == NULL) {
        BM_JPU_ERROR("add_fb_list failed, fb_list_curr is NULL");
        bmjpeg_dec_ioctl_release_frame(chn_fd, info->framebuffer->context);
        free(info->framebuffer->context);
        free(info->framebuffer->dma_buffer);
        free(info->framebuffer);
        return;
    }
    jpeg_decoder->decoder->fb_list_curr = list_curr;

    return;
}

BmJpuDecReturnCodes bm_jpu_jpeg_dec_frame_finished(BmJpuJPEGDecoder *jpeg_decoder, BmJpuFramebuffer *framebuffer)
{
    BmJpuDecReturnCodes ret = BM_JPU_DEC_RETURN_CODE_OK;
    int chn_id = 0;
    int chn_fd = -1;
    VIDEO_FRAME_INFO_S *pstFrameInfo;
    FramebufferList *list_curr = NULL;

    chn_id = jpeg_decoder->decoder->channel_id;
    if (chn_id < 0 || chn_id >= VDEC_MAX_CHN_NUM) {
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
static bm_handle_t g_jpeg_enc_bm_handle[MAX_NUM_DEV] = { 0 };
static int g_jpeg_enc_load_cnt[MAX_NUM_DEV] = { 0 };
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
    bm_handle_t handle = 0;

    pthread_mutex_lock(&g_jpeg_enc_lock);
    handle = g_jpeg_enc_bm_handle[device_index];
    pthread_mutex_unlock(&g_jpeg_enc_lock);

    return handle;
}

BmJpuEncReturnCodes bm_jpu_enc_load(int device_index)
{
    pthread_mutex_lock(&g_jpeg_enc_lock);
#ifdef BOARD_FPGA
    bmjpeg_devm_open();
#endif

    if (g_jpeg_enc_bm_handle[device_index] == 0) {
        bm_handle_t handle;
        bm_status_t ret = bm_dev_request(&handle, device_index);
        if (ret != BM_SUCCESS) {
            pthread_mutex_unlock(&g_jpeg_enc_lock);
            BM_JPU_ERROR("request dev %d failed, ret: %d", device_index, ret);
            return BM_JPU_ENC_RETURN_CODE_ERROR;
        }
        g_jpeg_enc_bm_handle[device_index] = handle;
        g_jpeg_enc_load_cnt[device_index] = 1;
    } else {
        g_jpeg_enc_load_cnt[device_index]++;
    }
    pthread_mutex_unlock(&g_jpeg_enc_lock);

    return BM_JPU_ENC_RETURN_CODE_OK;
}

BmJpuEncReturnCodes bm_jpu_enc_unload(int device_index)
{
    pthread_mutex_lock(&g_jpeg_enc_lock);
#ifdef BOARD_FPGA
    bmjpeg_devm_close();
#endif

    if (g_jpeg_enc_bm_handle[device_index] != 0) {
        g_jpeg_enc_load_cnt[device_index]--;
        if (g_jpeg_enc_load_cnt[device_index] <= 0) {
            bm_dev_free(g_jpeg_enc_bm_handle[device_index]);
            g_jpeg_enc_bm_handle[device_index] = 0;
            g_jpeg_enc_load_cnt[device_index] = 0;
        }
    }
    pthread_mutex_unlock(&g_jpeg_enc_lock);

    return BM_JPU_ENC_RETURN_CODE_OK;
}

BmJpuEncReturnCodes bm_jpu_jpeg_enc_open(BmJpuJPEGEncoder **jpeg_encoder,
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
    if (chn_fd < 0) {
        pthread_mutex_unlock(&g_jpeg_enc_lock);
        BM_JPU_ERROR("open device %s failed, errno: %d, %s", dev_name, errno, strerror(errno));
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
    (*jpeg_encoder)->device_index = device_index;  // soc_idx
    (*jpeg_encoder)->bitstream_buffer_size = bs_buffer_size;
    (*jpeg_encoder)->bitstream_buffer_alignment = 16;
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
    VENC_CHN_ATTR_S stAttr;
    VENC_ATTR_JPEG_S *pstJpegAttr;
    VENC_RECV_PIC_PARAM_S stRecvParam;
    VIDEO_FRAME_INFO_EX_S stFrameEx;
    VENC_STREAM_EX_S stStreamEx;
    VIDEO_FRAME_INFO_S stFrame;
    VENC_STREAM_S stStream;
    VENC_CHN_STATUS_S stStat;
    VENC_PACK_S *pPack;
    bm_handle_t bm_handle;
    bm_device_mem_t bm_mem;
    bm_status_t bm_ret = BM_SUCCESS;
    int i = 0;
    size_t total_size = 0;
    unsigned long long vaddr = 0;
    int map_count = 0;

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

    bm_handle = g_jpeg_enc_bm_handle[jpeg_encoder->device_index];
    bm_mem_flush_device_mem(bm_handle, framebuffer->dma_buffer);

    unsigned long long base_addr = bm_mem_get_device_addr(*framebuffer->dma_buffer);
    memset(&stAttr, 0, sizeof(VENC_CHN_ATTR_S));
    memset(&stFrame, 0, sizeof(VIDEO_FRAME_INFO_S));

    stFrame.stVFrame.u32Stride[0] = framebuffer->y_stride;
    stFrame.stVFrame.u32Stride[1] = framebuffer->cbcr_stride;
    stFrame.stVFrame.u32Stride[2] = framebuffer->cbcr_stride;

    stFrame.stVFrame.u64PhyAddr[0] = base_addr + framebuffer->y_offset;
    stFrame.stVFrame.u64PhyAddr[1] = base_addr + framebuffer->cb_offset;
    stFrame.stVFrame.u64PhyAddr[2] = base_addr + framebuffer->cr_offset;

    switch (params->color_format) {
        case BM_JPU_COLOR_FORMAT_YUV420:
            if (params->chroma_interleave == CBCR_SEPARATED) {
                stAttr.stVencAttr.enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;
                stFrame.stVFrame.enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;
            } else if (params->chroma_interleave == CBCR_INTERLEAVE) {
                stAttr.stVencAttr.enPixelFormat = PIXEL_FORMAT_NV12;
                stFrame.stVFrame.enPixelFormat = PIXEL_FORMAT_NV12;
                stFrame.stVFrame.u32Stride[2] = 0;
                stFrame.stVFrame.u64PhyAddr[2] = 0;
            } else if (params->chroma_interleave == CRCB_INTERLEAVE) {
                stAttr.stVencAttr.enPixelFormat = PIXEL_FORMAT_NV21;
                stFrame.stVFrame.enPixelFormat = PIXEL_FORMAT_NV21;
                stFrame.stVFrame.u32Stride[2] = 0;
                stFrame.stVFrame.u64PhyAddr[2] = 0;
            } else {
                BM_JPU_ERROR("unsupported chroma interleave value: %d", params->chroma_interleave);
                return BM_JPU_ENC_RETURN_CODE_INVALID_PARAMS;
            }
            break;
        case BM_JPU_COLOR_FORMAT_YUV422_HORIZONTAL:
            if (params->chroma_interleave == CBCR_SEPARATED) {
                stAttr.stVencAttr.enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_422;
                stFrame.stVFrame.enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_422;
            } else if (params->chroma_interleave == CBCR_INTERLEAVE) {
                stAttr.stVencAttr.enPixelFormat = PIXEL_FORMAT_NV16;
                stFrame.stVFrame.enPixelFormat = PIXEL_FORMAT_NV16;
                stFrame.stVFrame.u32Stride[2] = 0;
                stFrame.stVFrame.u64PhyAddr[2] = 0;
            } else if (params->chroma_interleave == CRCB_INTERLEAVE) {
                stAttr.stVencAttr.enPixelFormat = PIXEL_FORMAT_NV61;
                stFrame.stVFrame.enPixelFormat = PIXEL_FORMAT_NV61;
                stFrame.stVFrame.u32Stride[2] = 0;
                stFrame.stVFrame.u64PhyAddr[2] = 0;
            } else {
                BM_JPU_ERROR("unsupported chroma interleave value: %d", params->chroma_interleave);
                return BM_JPU_ENC_RETURN_CODE_INVALID_PARAMS;
            }
            break;
        case BM_JPU_COLOR_FORMAT_YUV444:
            if (params->chroma_interleave == CBCR_SEPARATED) {
                stAttr.stVencAttr.enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_444;
                stFrame.stVFrame.enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_444;
            } else {
                BM_JPU_ERROR("unsupported chroma interleave value: %d", params->chroma_interleave);
                return BM_JPU_ENC_RETURN_CODE_INVALID_PARAMS;
            }
            break;
        case BM_JPU_COLOR_FORMAT_YUV400:
            if (params->chroma_interleave == CBCR_SEPARATED) {
                stAttr.stVencAttr.enPixelFormat = PIXEL_FORMAT_YUV_400;
                stFrame.stVFrame.enPixelFormat = PIXEL_FORMAT_YUV_400;
                stFrame.stVFrame.u32Stride[1] = 0;
                stFrame.stVFrame.u32Stride[2] = 0;
                stFrame.stVFrame.u64PhyAddr[1] = 0;
                stFrame.stVFrame.u64PhyAddr[2] = 0;
            } else {
                BM_JPU_ERROR("unsupported chroma interleave value: %d", params->chroma_interleave);
                return BM_JPU_ENC_RETURN_CODE_INVALID_PARAMS;
            }
            break;
        default:
            BM_JPU_ERROR("unsupported color format: %d", params->color_format);
            return BM_JPU_ENC_RETURN_CODE_INVALID_PARAMS;
    }

    BM_JPU_DEBUG("stFrame.stVFrame.u32Stride[0] = %u", stFrame.stVFrame.u32Stride[0]);
    BM_JPU_DEBUG("stFrame.stVFrame.u32Stride[1] = %u", stFrame.stVFrame.u32Stride[1]);
    BM_JPU_DEBUG("stFrame.stVFrame.u32Stride[2] = %u", stFrame.stVFrame.u32Stride[2]);
    BM_JPU_DEBUG("stFrame.stVFrame.u64PhyAddr[0] = %#lx", stFrame.stVFrame.u64PhyAddr[0]);
    BM_JPU_DEBUG("stFrame.stVFrame.u64PhyAddr[1] = %#lx", stFrame.stVFrame.u64PhyAddr[1]);
    BM_JPU_DEBUG("stFrame.stVFrame.u64PhyAddr[2] = %#lx", stFrame.stVFrame.u64PhyAddr[2]);

    stAttr.stVencAttr.enType = PT_JPEG;
    stAttr.stVencAttr.u32MaxPicWidth = params->frame_width;
    stAttr.stVencAttr.u32MaxPicHeight = params->frame_height;
    stAttr.stVencAttr.u32PicWidth = params->frame_width;
    stAttr.stVencAttr.u32PicHeight = params->frame_height;
    stAttr.stVencAttr.u32BufSize = jpeg_encoder->bitstream_buffer_size;
    stAttr.stVencAttr.bEsBufQueueEn = 1; // CVI_H26X_ES_BUFFER_QUEUE_DEFAULT
    stAttr.stVencAttr.bIsoSendFrmEn = 1; // CVI_H26X_ISO_SEND_FRAME_DEFAUL

    stAttr.stVencAttr.u32Profile = 0;
    stAttr.stVencAttr.bByFrame = CVI_TRUE; // get stream mode is slice mode or frame mode ?

    pstJpegAttr = &stAttr.stVencAttr.stAttrJpege;
    pstJpegAttr->bSupportDCF = CVI_FALSE;
    pstJpegAttr->stMPFCfg.u8LargeThumbNailNum = 0;
    pstJpegAttr->enReceiveMode = VENC_PIC_RECEIVE_SINGLE;

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

    stFrame.stVFrame.u32Width = params->frame_width;
    stFrame.stVFrame.u32Height = params->frame_height;
    stFrame.stVFrame.s32FrameIdx = -1;
    stFrame.stVFrame.u64PTS = 0;

    stFrameEx.pstFrame = &stFrame;
    stFrameEx.s32MilliSec = -1;
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

    memset(&stStream, 0, sizeof(VENC_STREAM_S));
    stStream.pstPack = malloc(stStat.u32CurPacks * sizeof(VENC_PACK_S));
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
            g_stream_pack_array[chn_id][i] = pPack->pu8Addr;
            BM_JPU_DEBUG("origin pu8Addr: %p", pPack->pu8Addr);
            // get total output size
        #ifdef BOARD_FPGA
            pPack->pu8Addr = bmjpeg_ioctl_mmap(chn_fd, pPack->u64PhyAddr, pPack->u32Len);
        #else
            bm_mem = bm_mem_from_device(pPack->u64PhyAddr, pPack->u32Len);
            bm_ret = bm_mem_mmap_device_mem(bm_handle, &bm_mem, &vaddr);
            if (bm_ret != BM_SUCCESS) {
                BM_JPU_ERROR("bm_mem_mmap_device_mem failed, stream: %d, u64PhyAddr: %#lx, u32Len: %u", i, pPack->u64PhyAddr, pPack->u32Len);
                goto ERR_ENC_ENCODE_3;
            }
            pPack->pu8Addr = (CVI_U8 *)vaddr;
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
            goto ERR_ENC_ENCODE_3;
        }
        free(ptr_start);
    }

ERR_ENC_ENCODE_3:
    {
        for (i = 0; i < map_count; i++) {
            pPack = &stStream.pstPack[i];
            if (pPack->u64PhyAddr && pPack->u32Len) {
            #ifdef BOARD_FPGA
                bmjpeg_ioctl_munmap(pPack->pu8Addr, pPack->u32Len);
            #else
                bm_mem_unmap_device_mem(bm_handle, pPack->pu8Addr, pPack->u32Len);
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