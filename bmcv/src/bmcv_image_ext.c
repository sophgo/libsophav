#include <stdint.h>
#include <unistd.h>
#include "bmcv_internal.h"
// #include <fcntl.h>
#ifdef _FPGA
#include <sys/mman.h>
// #ifdef __linux__
//     #define PAGE_SIZE ((u64)getpagesize())
// #else
//     #define PAGE_SIZE ((u64)bm_getpagesize())
// #endif
#endif

#ifdef __linux__
#define __USE_GNU
#include <dlfcn.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

bm_status_t fill_image_private(bm_image *res, int *stride) {
    bm_status_t ret = BM_SUCCESS;
    int data_size = 1;
    switch (res->data_type) {
        case DATA_TYPE_EXT_FLOAT32:
        case DATA_TYPE_EXT_U32:
            data_size = 4;
            break;
        case DATA_TYPE_EXT_FP16:
        case DATA_TYPE_EXT_BF16:
        case DATA_TYPE_EXT_U16:
        case DATA_TYPE_EXT_S16:
            data_size = 2;
            break;
        default:
            data_size = 1;
            break;
    }
    bool              use_default_stride = stride ? false : true;
    bm_image_private *image_private      = res->image_private;
    int               H                  = res->height;
    int               W                  = res->width;
    switch (res->image_format) {
        case FORMAT_YUV420P: {
            image_private->plane_num = 3;
            image_private->memory_layout[0] = set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[1] = set_plane_layout(1, 1, ALIGN(H, 2) >> 1,
                                                ALIGN(W, 2) >> 1, data_size);
            image_private->memory_layout[2] = set_plane_layout(1, 1, ALIGN(H, 2) >> 1,
                                                ALIGN(W, 2) >> 1, data_size);
            break;
        }
        case FORMAT_YUV422P: {
            image_private->plane_num = 3;
            image_private->memory_layout[0] = set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[1] = set_plane_layout(1, 1, H, ALIGN(W, 2) >> 1, data_size);
            image_private->memory_layout[2] = set_plane_layout(1, 1, H, ALIGN(W, 2) >> 1, data_size);
            break;
        }
        case FORMAT_YUV444P: {
            image_private->plane_num = 3;
            image_private->memory_layout[0] = set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[1] = set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[2] = set_plane_layout(1, 1, H, W, data_size);
            break;
        }
        case FORMAT_NV24: {
            image_private->plane_num = 2;
            image_private->memory_layout[0] = set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[1] = set_plane_layout(1, 1, H, ALIGN(W, 2), data_size);
            break;
        }
        case FORMAT_NV12:
        case FORMAT_NV21: {
            image_private->plane_num = 2;
            image_private->memory_layout[0] = set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[1] = set_plane_layout(1, 1, ALIGN(H, 2) >> 1,
                                               ALIGN(W, 2), data_size);
            break;
        }
        case FORMAT_NV16:
        case FORMAT_NV61: {
            image_private->plane_num = 2;
            image_private->memory_layout[0] = set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[1] = set_plane_layout(1, 1, H, ALIGN(W, 2), data_size);
            break;
        }
        case FORMAT_GRAY: {
            image_private->plane_num = 1;
            image_private->memory_layout[0] = set_plane_layout(1, 1, H, W, data_size);
            break;
        }
        case FORMAT_COMPRESSED: {
            image_private->plane_num        = 4;
            image_private->memory_layout[0] = image_private->memory_layout[1] =
                image_private->memory_layout[2] =
                    image_private->memory_layout[3] =
                        set_plane_layout(1, 1, 1, 1, 1);
            break;
        }
        case FORMAT_YUV444_PACKED:
        case FORMAT_YVU444_PACKED:
        case FORMAT_HSV180_PACKED:
        case FORMAT_HSV256_PACKED:
        case FORMAT_BGR_PACKED:
        case FORMAT_RGB_PACKED: {
            image_private->plane_num = 1;
            image_private->memory_layout[0] = set_plane_layout(1, 1, H, W * 3, data_size);
            break;
        }
        case FORMAT_ABGR_PACKED:
        case FORMAT_ARGB_PACKED: {
            image_private->plane_num = 1;
            image_private->memory_layout[0] = set_plane_layout(1, 1, H, W * 4, data_size);
            break;
        }
        case FORMAT_BGR_PLANAR:
        case FORMAT_RGB_PLANAR: {
            image_private->plane_num = 1;
            image_private->memory_layout[0] = set_plane_layout(1, 3, H, W, data_size);
            break;
        }
        case FORMAT_BGRP_SEPARATE:
        case FORMAT_RGBP_SEPARATE: {
            image_private->plane_num = 3;
            image_private->memory_layout[0] = set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[1] = set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[2] = set_plane_layout(1, 1, H, W, data_size);
            break;
        }
        case FORMAT_HSV_PLANAR: {
            image_private->plane_num = 3;
            image_private->memory_layout[0] = set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[1] = set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[2] = set_plane_layout(1, 1, H, W, data_size);
            break;
        }
        case FORMAT_RGBYP_PLANAR: {
            image_private->plane_num = 4;
            image_private->memory_layout[0] = set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[1] = set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[2] = set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[3] = set_plane_layout(1, 1, H, W, data_size);
            break;
        }
        case FORMAT_YUV422_YUYV:
        case FORMAT_YUV422_YVYU:
        case FORMAT_YUV422_UYVY:
        case FORMAT_YUV422_VYUY: {
            image_private->plane_num = 1;
            image_private->memory_layout[0] = set_plane_layout(1, 1, H, W * 2, data_size);
            break;
        }
        case FORMAT_BAYER: {
            image_private->plane_num = 1;
            image_private->memory_layout[0] = set_plane_layout(1, 1, H, W, data_size);
            break;
        }
        case FORMAT_BAYER_RG8: {
            image_private->plane_num = 1;
            image_private->memory_layout[0] = set_plane_layout(1, 1, H, W, data_size);
            break;
        }
        case FORMAT_ARGB4444_PACKED:
        case FORMAT_ABGR4444_PACKED:
        case FORMAT_ARGB1555_PACKED:
        case FORMAT_ABGR1555_PACKED:{
            image_private->plane_num = 1;
            image_private->memory_layout[0] = set_plane_layout(1, 1, H, W * 2, data_size);
            break;
        }
        default:
            printf("UNKONW IMAGE FORMAT! \n");
            ret = BM_ERR_DATA;
            break;

    }

    if (!use_default_stride) {
        for(int p = 0; p < res->image_private->plane_num; p++){
            image_private->memory_layout[p] =
                stride_width(image_private->memory_layout[p], stride[p]);
        }
    }

    res->image_private->default_stride = use_default_stride;
    return ret;
}

bm_status_t bm_image_get_device_mem(bm_image image, bm_device_mem_t *mem) {
    if (!image.image_private)
        return BM_ERR_DATA;
    for (int i = 0; i < image.image_private->plane_num; i++) {
        mem[i] = image.image_private->data[i];
    }
    return BM_SUCCESS;
}

bm_status_t bm_image_format_check(int                      img_h,
                                  int                      img_w,
                                  bm_image_format_ext      image_format,
                                  bm_image_data_format_ext data_type) {
    if (img_h <= 0 || img_w <= 0) {
        bmlib_log("BMCV",
                  BMLIB_LOG_ERROR,
                  "invalid width or height %s: %s: %d\n",
                  __FILE__,
                  __func__,
                  __LINE__);
        return BM_ERR_PARAM;
    }

    if (FORMAT_COMPRESSED == image_format) {
        if (!(data_type == DATA_TYPE_EXT_1N_BYTE)) {
            bmlib_log("BMCV",
                      BMLIB_LOG_ERROR,
                      "FORMAT_COMPRESSED only support  data  type "
                      "DATA_TYPE_EXT_1N_BYTE %s: %s: %d\n",
                      filename(__FILE__),
                      __func__,
                      __LINE__);
            return BM_ERR_PARAM;
        }
    }
    return BM_SUCCESS;
}

bm_status_t bm_image_create(bm_handle_t              handle,
                            int                      img_h,
                            int                      img_w,
                            bm_image_format_ext      image_format,
                            bm_image_data_format_ext data_type,
                            bm_image *               res,
                            int *                    stride){
    res->width         = img_w;
    res->height        = img_h;
    res->image_format  = image_format;
    res->data_type     = data_type;
    res->image_private = (bm_image_private *)malloc(sizeof(bm_image_private));
    if (!res->image_private) {
        bmlib_log("BMCV",
                  BMLIB_LOG_ERROR,
                  "host memory alloc failed %s: %s: %d\n",
                  filename(__FILE__),
                  __func__,
                  __LINE__);
        return BM_ERR_NOMEM;
    }

    res->image_private->plane_num = 0;
    res->image_private->internal_alloc_plane = 0;
    res->image_private->attached = false;
    res->image_private->data_owned = false;
    res->image_private->default_stride = false;
    res->image_private->owned_mem = true;
#ifndef USING_CMODEL
    res->image_private->decoder = NULL;
#endif
    memset(res->image_private->data, 0, sizeof(bm_device_mem_t) * MAX_bm_image_CHANNEL);
    memset(res->image_private->memory_layout, 0, sizeof(plane_layout) * MAX_bm_image_CHANNEL);
    pthread_mutex_init(&res->image_private->memory_lock, NULL);

    if (bm_image_format_check(img_h, img_w, image_format, data_type) !=
        BM_SUCCESS) {
        bmlib_log("BMCV",
                  BMLIB_LOG_ERROR,
                  "illegal format or size %s: %s: %d\n",
                  filename(__FILE__),
                  __func__,
                  __LINE__);
        free(res->image_private);
        res->image_private = NULL;
        return BM_ERR_PARAM;
    }

    if (fill_image_private(res, stride) != BM_SUCCESS) {
        bmlib_log("BMCV",
                  BMLIB_LOG_ERROR,
                  "illegal stride %s: %s: %d\n",
                  filename(__FILE__),
                  __func__,
                  __LINE__);
        free(res->image_private);
        res->image_private = NULL;
        return BM_ERR_DATA;
    }
    res->image_private->handle = handle;
    return BM_SUCCESS;
}

bm_status_t bm_image_update(bm_handle_t              handle,
                            int                      img_h,
                            int                      img_w,
                            bm_image_format_ext      image_format,
                            bm_image_data_format_ext data_type,
                            bm_image *               res,
                            int *                    stride) {
    res->width         = img_w;
    res->height        = img_h;
    res->image_format  = image_format;
    res->data_type     = data_type;
    if (!res->image_private) {
        bmlib_log("BMCV",BMLIB_LOG_ERROR, "bm_image image_private is NULL %s: %s: %d\n",
                  filename(__FILE__),__func__,__LINE__);
        return BM_ERR_NOMEM;
    }

    if (bm_image_format_check(img_h, img_w, image_format, data_type) !=
        BM_SUCCESS) {
        bmlib_log("BMCV",
                  BMLIB_LOG_ERROR,
                  "illegal format or size %s: %s: %d\n",
                  filename(__FILE__),
                  __func__,
                  __LINE__);
        return BM_ERR_PARAM;
    }
    if (fill_image_private(res, stride) != BM_SUCCESS) {
        bmlib_log("BMCV", BMLIB_LOG_ERROR, "illegal stride %s: %s: %d\n",
                  filename(__FILE__),  __func__, __LINE__);
        return BM_ERR_PARAM;
    }
    res->image_private->handle = handle;
    return BM_SUCCESS;
}

size_t bmcv_get_private_size(void)
{
    return sizeof(struct bm_image_private);
}

bm_status_t bm_image_create_private(bm_handle_t              handle,
                                    int                      img_h,
                                    int                      img_w,
                                    bm_image_format_ext      image_format,
                                    bm_image_data_format_ext data_type,
                                    bm_image *               res,
                                    int *                    stride,
                                    void*                    bm_private) {

    if (NULL == bm_private) {
        bmlib_log("BMCV",
                  BMLIB_LOG_ERROR,
                  "host memory alloc failed %s: %s: %d\n",
                  filename(__FILE__),
                  __func__,
                  __LINE__);
        return BM_ERR_NOMEM;
    }

    res->width         = img_w;
    res->height        = img_h;
    res->image_format  = image_format;
    res->data_type     = data_type;
    res->image_private = (struct bm_image_private*)bm_private;
    res->image_private->owned_mem = false;

    memset(res->image_private->data, 0,
           sizeof(bm_device_mem_t) * MAX_bm_image_CHANNEL);
    memset(res->image_private->memory_layout, 0,
           sizeof(plane_layout) * MAX_bm_image_CHANNEL);
    if (bm_image_format_check(img_h, img_w, image_format, data_type) !=
        BM_SUCCESS) {
        bmlib_log("BMCV",
                  BMLIB_LOG_ERROR,
                  "illegal format or size %s: %s: %d\n",
                  filename(__FILE__),
                  __func__,
                  __LINE__);
        res->image_private = NULL;
        return BM_ERR_PARAM;
    }
    if (fill_image_private(res, stride) != BM_SUCCESS) {
        bmlib_log("BMCV",
                  BMLIB_LOG_ERROR,
                  "illegal stride %s: %s: %d\n",
                  filename(__FILE__),
                  __func__,
                  __LINE__);
        res->image_private = NULL;
        return BM_ERR_PARAM;
    }
    res->image_private->handle = handle;
    return BM_SUCCESS;
}

bool bm_image_is_attached(bm_image image) {
    if (!image.image_private)
        return false;
    return image.image_private->attached;
}

bm_status_t bm_image_attach(bm_image image, bm_device_mem_t *device_memory) {
    if (!image.image_private)
        return BM_ERR_DATA;
    if (image.image_private->data_owned) {
        pthread_mutex_lock(&image.image_private->memory_lock);
        int                         total_size = 0;
        for (int i = 0; i < image.image_private->internal_alloc_plane; i++) {
            total_size += image.image_private->data[i].size;
        }

        bm_device_mem_t dmem = image.image_private->data[0];
        dmem.size            = total_size;

        bm_free_device(image.image_private->handle, dmem);
        image.image_private->internal_alloc_plane = 0;
        image.image_private->data_owned           = false;
        pthread_mutex_unlock(&image.image_private->memory_lock);
    }
    for (int i = 0; i < image.image_private->plane_num; i++) {
        image.image_private->data[i]               = device_memory[i];
        image.image_private->memory_layout[i].size = device_memory[i].size;
        if (image.image_format == FORMAT_COMPRESSED) {
            image.image_private->memory_layout[i] =
                set_plane_layout(1, 1, 1, device_memory[i].size, 1);
        }
    }
    image.image_private->attached = true;
    return BM_SUCCESS;
}

bm_status_t bm_image_detach(bm_image image){
    if (!image.image_private)
        return BM_ERR_DATA;
    if (image.image_private->data_owned == true) {
        pthread_mutex_lock(&image.image_private->memory_lock);
        int                         total_size = 0;
        for (int i = 0; i < image.image_private->internal_alloc_plane; i++){
            if(image.image_format == FORMAT_COMPRESSED)
                total_size += ALIGN(image.image_private->data[i].size, 64);
            else
                total_size += image.image_private->data[i].size;
        }

        bm_device_mem_t dmem = image.image_private->data[0];
        dmem.size            = total_size;

        bm_free_device(image.image_private->handle, dmem);
        image.image_private->internal_alloc_plane = 0;
        image.image_private->data_owned           = false;
        memset(image.image_private->data,
               0,
               MAX_bm_image_CHANNEL * sizeof(bm_device_mem_t));
        memset(image.image_private->memory_layout,
               0,
               sizeof(plane_layout) * MAX_bm_image_CHANNEL);
        pthread_mutex_unlock(&image.image_private->memory_lock);
    }
    image.image_private->attached = false;
    return BM_SUCCESS;

}

bm_handle_t bm_image_get_handle(bm_image *image) {
    if (image->image_private) {
        return image->image_private->handle;
    } else {
        bmlib_log(
            "BMCV",
            BMLIB_LOG_ERROR,
            "Attempting to get handle from a non-existing image %s: %s: %d\n",
            filename(__FILE__),
            __func__,
            __LINE__);
        return 0;
    }
}

bm_status_t bm_image_get_format_info(bm_image *              src,
                                     bm_image_format_info_t *info) {
    info->height         = src->height;
    info->width          = src->width;
    info->image_format   = src->image_format;
    info->data_type      = src->data_type;
    info->default_stride = src->image_private->default_stride;
    if (src->image_private == NULL) {
        bmlib_log(BMCV_LOG_TAG,
                  BMLIB_LOG_ERROR,
                  "image is not created \n  %s: %s: %d\n",
                  filename(__FILE__),
                  __func__,
                  __LINE__);
        return BM_ERR_DATA;
    }
    if (src->image_format == FORMAT_COMPRESSED &&
        !src->image_private->attached) {
        bmlib_log(BMCV_LOG_TAG,
                  BMLIB_LOG_ERROR,
                  "for compressed format, if device memory is not attached, "
                  "the image info is not intact\n  %s: %s: %d\n",
                  filename(__FILE__),
                  __func__,
                  __LINE__);
        return BM_ERR_DATA;
    }
    info->plane_nb = src->image_private->plane_num;
    for (int i = 0; i < info->plane_nb; ++i) {
        info->plane_data[i] = src->image_private->data[i];
        info->stride[i]     = src->image_private->memory_layout[i].pitch_stride;
    }

    return BM_SUCCESS;
}

bm_status_t bm_image_get_byte_size(bm_image image, int *size) {
    if (!image.image_private)
        return BM_ERR_DATA;
    if (image.image_format == FORMAT_COMPRESSED &&
        !image.image_private->attached) {
        bmlib_log("BMCV",
                  BMLIB_LOG_ERROR,
                  "compressed format plane size is variable, please attached "
                  "device memory first  %s: %s: %d\n",
                  filename(__FILE__),
                  __func__,
                  __LINE__);
        return BM_ERR_DATA;
    }
    pthread_mutex_lock(&image.image_private->memory_lock);
    for (int i = 0; i < image.image_private->plane_num; i++) {
        size[i] = image.image_private->memory_layout[i].size;
    }
    pthread_mutex_unlock(&image.image_private->memory_lock);
    return BM_SUCCESS;
}

bm_status_t bm_image_destroy(bm_image *image){
    if(!image){
        return BM_ERR_PARAM;
    }

    if(!image->image_private){
        return BM_ERR_DATA;
    }

    if (bm_image_detach(*image) != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG,
                  BMLIB_LOG_ERROR,
                  "detach dev mem failed %s %s %d\n",
                  filename(__FILE__),
                  __func__,
                  __LINE__);
        return BM_ERR_DATA;
    }

    if (image->image_private->decoder != NULL) {
        bm_jpu_jpeg_dec_close(image->image_private->decoder);
    }

    pthread_mutex_destroy(&image->image_private->memory_lock);
    if (true == image->image_private->owned_mem) {
        free(image->image_private);
    }
    image->image_private = NULL;
    return BM_SUCCESS;
}

bm_status_t bm_image_alloc_dev_mem(bm_image image, int heap_id) {
    if (!image.image_private)
        return BM_ERR_DATA;
    if (image.image_format == FORMAT_COMPRESSED && image.image_private->memory_layout[0].pitch_stride == 1) {
        bmlib_log(BMCV_LOG_TAG,
                  BMLIB_LOG_ERROR,
                  "compressed format only support attached device memory, not "
                  "host pointer\n");
        return BM_ERR_DATA;
    }
    if (image.image_private->data_owned == true)
        return BM_SUCCESS;
    pthread_mutex_lock(&image.image_private->memory_lock);

    // malloc continuious memory for acceleration
    int             total_size = 0;
    bm_device_mem_t dmem;
    for (int i = 0; i < image.image_private->plane_num; ++i) {
        if(image.image_format == FORMAT_COMPRESSED)
            total_size += ALIGN(image.image_private->memory_layout[i].size, 64);
        else
            total_size += image.image_private->memory_layout[i].size;
    }

    if (heap_id != BMCV_HEAP_ANY) {
        if (BM_SUCCESS !=
            bm_malloc_device_byte_heap(
                image.image_private->handle, &dmem, heap_id, total_size)) {
            BMCV_ERR_LOG("bm_malloc_device_byte_heap error\r\n");
            pthread_mutex_unlock(&image.image_private->memory_lock);
            return BM_ERR_NOMEM;
        }
    } else {
        if (BM_SUCCESS != bm_malloc_device_byte(
                              image.image_private->handle, &dmem, total_size)) {
            BMCV_ERR_LOG("bm_malloc_device_byte error\r\n");
            pthread_mutex_unlock(&image.image_private->memory_lock);
            return BM_ERR_NOMEM;
        }
    }

    void * system_addr = dmem.u.system.system_addr;
    unsigned long long base_addr = dmem.u.device.device_addr;
    for (int i = 0; i < image.image_private->plane_num; i++) {
        image.image_private->data[i] = bm_mem_from_device(
            base_addr, image.image_private->memory_layout[i].size);
        if (i == 0) {
            image.image_private->data[0].flags.u.gmem_heapid =
                dmem.flags.u.gmem_heapid;
            image.image_private->data[0].u.device.dmabuf_fd =
                dmem.u.device.dmabuf_fd;
        }
        image.image_private->data[i].u.system.system_addr = system_addr;
        if(image.image_format == FORMAT_COMPRESSED){
            base_addr += ALIGN(image.image_private->memory_layout[i].size, 64);
            system_addr += ALIGN(image.image_private->memory_layout[i].size, 64);
        }
        else{
            base_addr += image.image_private->memory_layout[i].size;
            system_addr += image.image_private->memory_layout[i].size;
        }
        image.image_private->internal_alloc_plane++;
    }

    image.image_private->data_owned = true;
    image.image_private->attached   = true;
    pthread_mutex_unlock(&image.image_private->memory_lock);
    return BM_SUCCESS;
}

bm_status_t bm_image_alloc_dev_mem_heap_mask(bm_image image, int heap_mask) {
    if (!image.image_private)
        return BM_ERR_DATA;
    if (image.image_format == FORMAT_COMPRESSED && image.image_private->memory_layout[0].pitch_stride == 1) {
        bmlib_log(BMCV_LOG_TAG,
                  BMLIB_LOG_ERROR,
                  "compressed format only support attached device memory, not "
                  "host pointer\n");
        return BM_ERR_DATA;
    }
    if (image.image_private->data_owned == true) {
        return BM_SUCCESS;
    }
    pthread_mutex_lock(&image.image_private->memory_lock);
    // malloc continuious memory for acceleration
    int             total_size = 0;
    bm_device_mem_t dmem;
    for (int i = 0; i < image.image_private->plane_num; ++i) {
        total_size += image.image_private->memory_layout[i].size;
    }

    if (BM_SUCCESS !=
        bm_malloc_device_byte_heap_mask(
            image.image_private->handle, &dmem, heap_mask, total_size)) {
        BMCV_ERR_LOG("bm_malloc_device_byte_heap error\r\n");
        pthread_mutex_unlock(&image.image_private->memory_lock);
        return BM_ERR_NOMEM;
    }

    #ifdef __linux__
    unsigned long base_addr = dmem.u.device.device_addr;
    #else
    unsigned long long base_addr = dmem.u.device.device_addr;
    #endif
    for (int i = 0; i < image.image_private->plane_num; i++) {
        image.image_private->data[i] = bm_mem_from_device(
            base_addr, image.image_private->memory_layout[i].size);
        if (i == 0) {
            image.image_private->data[0].flags.u.gmem_heapid =
                dmem.flags.u.gmem_heapid;
            image.image_private->data[0].u.device.dmabuf_fd =
                dmem.u.device.dmabuf_fd;
        }
        base_addr += image.image_private->memory_layout[i].size;

        image.image_private->internal_alloc_plane++;
    }

    image.image_private->data_owned = true;
    image.image_private->attached   = true;
    pthread_mutex_unlock(&image.image_private->memory_lock);
    return BM_SUCCESS;
}

bm_status_t bm_image_alloc_contiguous_mem_heap_mask(int       image_num,
                                                    bm_image *images,
                                                    int       heap_mask) {
    if (0 == image_num) {
        BMCV_ERR_LOG("image_num can not be set as 0\n");
        return BM_ERR_PARAM;
    }
    for (int i = 0; i < image_num - 1; i++) {
        for (int idx = 0; idx < images[i].image_private->plane_num; idx++) {
            if (images[i].image_private->memory_layout[idx].size !=
                images[i + 1].image_private->memory_layout[idx].size) {
                BMCV_ERR_LOG("all image must have same size\n");
                return BM_ERR_DATA;
            }
        }
        if (images[i].image_private->attached) {
            BMCV_ERR_LOG("image has been attached memory\n");
            return BM_ERR_DATA;
        }
    }
    int single_image_sz = 0;
    for (int idx = 0; idx < images[0].image_private->plane_num; idx++) {
        single_image_sz += images[0].image_private->memory_layout[idx].size;
    }
    int             total_image_size = image_num * single_image_sz;
    bm_device_mem_t dmem;
    if (BM_SUCCESS !=
        bm_malloc_device_byte_heap_mask(images[0].image_private->handle,
                                   &dmem,
                                   heap_mask,
                                   total_image_size)) {
        BMCV_ERR_LOG("bm_malloc_device_byte_heap error\r\n");

        return BM_ERR_NOMEM;
    }
    int           dmabuf_fd = dmem.u.device.dmabuf_fd;
    #ifdef __linux__
    unsigned long base_addr = dmem.u.device.device_addr;
    #else
    unsigned long long base_addr = dmem.u.device.device_addr;
    #endif
    for (int i = 0; i < image_num; i++) {
        pthread_mutex_lock(&images[i].image_private->memory_lock);
        bm_device_mem_t             tmp_dmem[16];
        memset(tmp_dmem, 0x0, sizeof(tmp_dmem));
        for (int idx = 0; idx < images[i].image_private->plane_num; idx++) {
            tmp_dmem[idx] = bm_mem_from_device(
                base_addr, images[i].image_private->memory_layout[idx].size);
            tmp_dmem[idx].flags.u.gmem_heapid = dmem.flags.u.gmem_heapid;
            tmp_dmem[idx].u.device.dmabuf_fd = dmabuf_fd;
            base_addr += images[i].image_private->memory_layout[idx].size;
        }
        bm_image_attach(images[i], tmp_dmem);
        pthread_mutex_unlock(&images[i].image_private->memory_lock);
    }

    return BM_SUCCESS;
}

#ifdef _FPGA
static void *devm_map(int fd, unsigned long long phy_addr, unsigned int len)
{
	off_t offset;
	void *map_base;
	unsigned int padding;
	offset = phy_addr & ~(sysconf(_SC_PAGE_SIZE) - 1);
	padding = phy_addr - offset;
	map_base = mmap(NULL, len + padding, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, offset);
	if (map_base == MAP_FAILED) {
		perror("mmap failed\n");
		printf("phy_addr = %llx, length = %d\t, mapped memory length = %d\t\n"
			, phy_addr, len, len + padding);
		goto mmap_err;
	}
	return map_base + padding;

mmap_err:
	return NULL;
}

static void devm_unmap(void *virt_addr, size_t len)
{
	unsigned long long addr;
	/* page align */
	addr = (((unsigned long long)(uintptr_t)virt_addr) & ~(sysconf(_SC_PAGE_SIZE) - 1));
	munmap((void *)(uintptr_t)addr, len + (unsigned long) virt_addr - addr);
}

bm_status_t bm_memcpy_d2s_fpga(bm_handle_t handle, void *dst, bm_device_mem_t src){
    void *system_addr = 0;
    bm_status_t ret = BM_SUCCESS;
    int fd = -1, size = src.size;
    ret = bm_get_mem_fd(&fd);
    if(ret != BM_SUCCESS)
        return ret;
    system_addr = devm_map(fd, (unsigned long long)src.u.device.device_addr, size);
    if (NULL != system_addr) {
        memcpy(dst, system_addr, size);
        devm_unmap(system_addr, size);
        ret = BM_SUCCESS;
    } else {
        ret = BM_ERR_NOMEM;
    }
    bm_destroy_mem_fd();
    return ret;
}

bm_status_t bm_memcpy_s2d_fpga(bm_handle_t handle, bm_device_mem_t dst, void *src){
    void *system_addr = 0;
    bm_status_t ret = BM_SUCCESS;
    int fd = -1, size = dst.size;
    ret = bm_get_mem_fd(&fd);
    if(ret != BM_SUCCESS)
        return ret;
    system_addr = devm_map(fd, (unsigned long long)dst.u.device.device_addr, size);
    if (NULL != system_addr) {
        memcpy(system_addr, src, size);
        devm_unmap(system_addr, size);
        ret = BM_SUCCESS;
    } else {
        ret = BM_ERR_NOMEM;
    }
    bm_destroy_mem_fd();
    return ret;
}
#endif

bm_status_t bm_image_copy_host_to_device(bm_image image, void *buffers[]) {
    if (!image.image_private)
        return BM_ERR_DATA;
    if (image.image_format == FORMAT_COMPRESSED && image.image_private->memory_layout[0].pitch_stride == 1) {
        bmlib_log(BMCV_LOG_TAG,
                  BMLIB_LOG_ERROR,
                  "compressed format only support attached device memory, not "
                  "host pointer\n");
        return BM_ERR_DATA;
    }
    // The image didn't attached to a buffer, malloc and own it.
    if (!image.image_private->attached) {
        if (bm_image_alloc_dev_mem(image, BMCV_HEAP1_ID) != BM_SUCCESS) {
            bmlib_log(BMCV_LOG_TAG,
                      BMLIB_LOG_ERROR,
                      "alloc dst dev mem failed %s %s %d\n",
                      filename(__FILE__),
                      __func__,
                      __LINE__);
            return BM_ERR_NOMEM;
        }
    }

    unsigned char **host = (unsigned char **)buffers;
    for (int i = 0; i < image.image_private->plane_num; i++) {
#ifdef _FPGA
        if (BM_SUCCESS != bm_memcpy_s2d_fpga(image.image_private->handle,
                                        image.image_private->data[i],
                                        host[i])) {
#else
        if (BM_SUCCESS != bm_memcpy_s2d(image.image_private->handle,
                                        image.image_private->data[i],
                                        host[i])) {
#endif
            BMCV_ERR_LOG("bm_memcpy_s2d error, src addr %p, dst addr 0x%llx\n",
              host[i], image.image_private->data[i].u.device.device_addr);
            return BM_ERR_NOMEM;
        }
    }
    return BM_SUCCESS;
}

bm_status_t bm_image_copy_device_to_host(bm_image image, void *buffers[]) {
    if (!image.image_private)
        return BM_ERR_DATA;
    if (!image.image_private->attached) {
        return BM_ERR_DATA;
    }
    unsigned char **host = (unsigned char **)buffers;
    for (int i = 0; i < image.image_private->plane_num; i++) {
#ifdef _FPGA
        if (BM_SUCCESS != bm_memcpy_d2s_fpga(image.image_private->handle,
                                        host[i],
                                        image.image_private->data[i])) {
#else
        if (BM_SUCCESS != bm_memcpy_d2s(image.image_private->handle,
                                        host[i],
                                        image.image_private->data[i])) {
#endif
            BMCV_ERR_LOG("bm_memcpy_d2s error, src addr 0x%llx, dst addr %p\n",
                image.image_private->data[i].u.device.device_addr, host[i]);

            return BM_ERR_NOMEM;
        }
        // host = host + image.image_private->plane_byte_size[i];
    }
    return BM_SUCCESS;
}

bm_status_t bm_image_get_stride(bm_image image, int *stride) {
    if (!image.image_private)
        return BM_ERR_DATA;

    for (int i = 0; i < image.image_private->plane_num; i++) {
        stride[i] = image.image_private->memory_layout[i].pitch_stride;
    }
    return BM_SUCCESS;
}

int bm_image_get_plane_num(bm_image image) {
    if (!image.image_private)
        return BM_ERR_DATA;
    return image.image_private->plane_num;
}

bm_status_t bm_image_tensor_get_device_mem(bm_image_tensor  image_tensor,
                                           bm_device_mem_t *mem) {
    return bm_image_get_device_mem(image_tensor.image, mem);
}

bool bm_image_tensor_is_attached(bm_image_tensor image_tensor) {
    return bm_image_is_attached(image_tensor.image);
}

bm_status_t bm_image_tensor_alloc_dev_mem(bm_image_tensor image_tensor,
                                          int             heap_id) {
    return bm_image_alloc_dev_mem(image_tensor.image, heap_id);
}

static void fill_image_private_tensor(bm_image_tensor res) {
    int data_size = 1;
    switch (res.image.data_type) {
        case DATA_TYPE_EXT_FLOAT32:
        case DATA_TYPE_EXT_U32:
            data_size = 4 * res.image_n;
            break;
        case DATA_TYPE_EXT_FP16:
        case DATA_TYPE_EXT_BF16:
        case DATA_TYPE_EXT_U16:
        case DATA_TYPE_EXT_S16:
            data_size = 2 * res.image_n;
            break;
        default:
            data_size = 1 * res.image_n;
            break;
    }

    int  H             = res.image.height;
    int  W             = res.image.width;
    bm_image_private* image_private = res.image.image_private;
    switch (res.image.image_format) {
        case FORMAT_YUV420P: {
            image_private->plane_num = 3;
            image_private->memory_layout[0] = set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[1] = set_plane_layout(
                   1, 1, ALIGN(H, 2) >> 1, ALIGN(W, 2) >> 1, data_size);
            image_private->memory_layout[2] = set_plane_layout(
                   1, 1, ALIGN(H, 2) >> 1, ALIGN(W, 2) >> 1, data_size);
            break;
        }
        case FORMAT_YUV422P: {
            image_private->plane_num = 3;
            image_private->memory_layout[0] =
                set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[1] =
                set_plane_layout(1, 1, H, ALIGN(W, 2) >> 1, data_size);
            image_private->memory_layout[2] =
                set_plane_layout(1, 1, H, ALIGN(W, 2) >> 1, data_size);
            break;
        }
        case FORMAT_YUV444P: {
            image_private->plane_num = 3;
            image_private->memory_layout[0] =
                set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[1] =
                set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[2] =
                set_plane_layout(1, 1, H, W, data_size);
            break;
        }
        case FORMAT_NV24: {
            image_private->plane_num = 2;
            image_private->memory_layout[0] =
                set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[1] =
                set_plane_layout(1, 1, H, ALIGN(W, 2), data_size);
            break;
        }
        case FORMAT_NV12:
        case FORMAT_NV21: {
            image_private->plane_num = 2;
            image_private->memory_layout[0] =
                set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[1] = set_plane_layout(
                1, 1, ALIGN(H, 2) >> 1, ALIGN(W, 2), data_size);
            break;
        }
        case FORMAT_NV16:
        case FORMAT_NV61: {
            image_private->plane_num = 2;
            image_private->memory_layout[0] =
                set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[1] =
                set_plane_layout(1, 1, H, ALIGN(W, 2), data_size);
            break;
        }
        case FORMAT_GRAY: {
            image_private->plane_num = 1;
            image_private->memory_layout[0] =
                set_plane_layout(1, 1, H, W, data_size);
            break;
        }
        case FORMAT_YUV444_PACKED:
        case FORMAT_YVU444_PACKED:
        case FORMAT_HSV180_PACKED:
        case FORMAT_HSV256_PACKED:
        case FORMAT_BGR_PACKED:
        case FORMAT_RGB_PACKED: {
            image_private->plane_num = 1;
            image_private->memory_layout[0] =
                set_plane_layout(1, 1, H, W * 3, data_size);
            break;
        }
        case FORMAT_ABGR_PACKED:
        case FORMAT_ARGB_PACKED: {
            image_private->plane_num = 1;
            image_private->memory_layout[0] =
                set_plane_layout(1, 1, H, W * 4, data_size);
            break;
        }
        case FORMAT_BGR_PLANAR:
        case FORMAT_RGB_PLANAR: {
            image_private->plane_num = 1;
            image_private->memory_layout[0] =
                set_plane_layout(1, 3, H, W, data_size);
            break;
        }
        case FORMAT_BGRP_SEPARATE:
        case FORMAT_RGBP_SEPARATE: {
            image_private->plane_num = 3;
            image_private->memory_layout[0] =
                set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[1] =
                set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[2] =
                set_plane_layout(1, 1, H, W, data_size);
            break;
        }
        case FORMAT_HSV_PLANAR: {
            image_private->plane_num = 3;
            image_private->memory_layout[0] =
                set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[1] =
                set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[2] =
                set_plane_layout(1, 1, H, W, data_size);
            break;
        }
        case FORMAT_RGBYP_PLANAR: {
            image_private->plane_num = 4;
            image_private->memory_layout[0] =
                set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[1] =
                set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[2] =
                set_plane_layout(1, 1, H, W, data_size);
            image_private->memory_layout[3] =
                set_plane_layout(1, 1, H, W, data_size);
            break;
        }
        case FORMAT_YUV422_YUYV:
        case FORMAT_YUV422_YVYU:
        case FORMAT_YUV422_UYVY:
        case FORMAT_YUV422_VYUY: {
            image_private->plane_num = 1;
            image_private->memory_layout[0] =
                set_plane_layout(1, 1, H, W * 2, data_size);
            break;
        }
        default:
            image_private->plane_num = 1;
            image_private->memory_layout[0] =
                set_plane_layout(1, 1, H, W, data_size);
    }
}

bm_status_t bm_image_tensor_attach(bm_image_tensor  image_tensor,
                                   bm_device_mem_t *device_memory) {
    return bm_image_attach(image_tensor.image, device_memory);
}

bm_status_t bm_image_tensor_create(bm_handle_t              handle,
                                   int                      img_n,
                                   int                      img_c,
                                   int                      img_h,
                                   int                      img_w,
                                   bm_image_format_ext      image_format,
                                   bm_image_data_format_ext data_type,
                                   bm_image_tensor *        res) {
    res->image.width         = img_w;
    res->image.height        = img_h;
    res->image.image_format  = image_format;
    res->image.data_type     = data_type;
    res->image_n             = img_n;
    res->image_c             = img_c;
    res->image.image_private = (bm_image_private *) malloc (sizeof(bm_image_private));
    res->image.image_private->plane_num = 0;
    res->image.image_private->internal_alloc_plane = 0;
    res->image.image_private->data_owned = false;
    res->image.image_private->attached = false;
    res->image.image_private->decoder = NULL;
    res->image.image_private->owned_mem = true;
    pthread_mutex_init(&res->image.image_private->memory_lock, NULL);
    if (!res->image.image_private)
        return BM_ERR_DATA;

    fill_image_private_tensor(*res);
    res->image.image_private->handle = handle;
    return BM_SUCCESS;
}

bm_status_t concat_images_to_tensor(bm_handle_t      handle,
                                    int              image_num,
                                    bm_image *       images,
                                    bm_image_tensor *image_tensor) {

    if (bm_image_get_plane_num(images[0]) != 1) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "NOT supported image format\n");
        return BM_ERR_DATA;
    }
    bm_device_mem_t first_dev_mem;
    bm_image_get_device_mem(images[0], &first_dev_mem);
    u64 last_dev_addr = bm_mem_get_device_addr(first_dev_mem);
    for (int i = 0; i < image_num - 1; i++) {
        int last_image_size = 0;
        bm_image_get_byte_size(images[i], &last_image_size);
        bm_device_mem_t dev_mem;
        bm_image_get_device_mem(images[i + 1], &dev_mem);
        u64 dev_addr = bm_mem_get_device_addr(dev_mem);
        if (dev_addr != last_dev_addr + last_image_size) {
            return BM_ERR_DATA;
        }
        if ((images[i].width != images[i + 1].width) ||
            (images[i].height != images[i + 1].height)) {
            return BM_ERR_DATA;
        }
        last_dev_addr = dev_addr;
    }
    int image_channel = (images[0].image_format == FORMAT_GRAY) ? (1) : (3);
    bm_image_tensor_create( \
            handle,
            image_num,
            image_channel,
            images[0].height,
            images[0].width,
            images[0].image_format,
            images[0].data_type,
            image_tensor);
    first_dev_mem.size = first_dev_mem.size * image_num;
    bm_image_tensor_attach(*image_tensor, &first_dev_mem);

    return BM_SUCCESS;
}

bm_status_t bm_image_tensor_destroy(bm_image_tensor image_tensor) {
    return bm_image_destroy(&(image_tensor.image));
}

bm_status_t bm_image_alloc_contiguous_mem(int       image_num,
                                          bm_image *images,
                                          int       heap_id) {
    if (0 == image_num) {
        BMCV_ERR_LOG("image_num can not be set as 0\n");
        return BM_ERR_PARAM;
    }
    for (int i = 0; i < image_num - 1; i++) {
        for (int idx = 0; idx < images[i].image_private->plane_num; idx++) {
            if (images[i].image_private->memory_layout[idx].size !=
                images[i + 1].image_private->memory_layout[idx].size) {
                BMCV_ERR_LOG("all image must have same size\n");
                return BM_ERR_DATA;
            }
        }
        if (images[i].image_private->attached) {
            BMCV_ERR_LOG("image has been attached memory\n");
            return BM_ERR_DATA;
        }
    }
    int single_image_sz = 0;
    for (int idx = 0; idx < images[0].image_private->plane_num; idx++) {
        single_image_sz += images[0].image_private->memory_layout[idx].size;
    }
    int             total_image_size = image_num * single_image_sz;
    bm_device_mem_t dmem;
    if (heap_id != BMCV_HEAP_ANY) {
        if (BM_SUCCESS !=
            bm_malloc_device_byte_heap(images[0].image_private->handle,
                                       &dmem,
                                       heap_id,
                                       total_image_size)) {
            BMCV_ERR_LOG("bm_malloc_device_byte_heap error\r\n");

            return BM_ERR_NOMEM;
        }
    } else {
        if (BM_SUCCESS != bm_malloc_device_byte(images[0].image_private->handle,
                                                &dmem,
                                                total_image_size)) {
            BMCV_ERR_LOG("bm_malloc_device_byte error\r\n");

            return BM_ERR_NOMEM;
        }
    }
    int           dmabuf_fd = dmem.u.device.dmabuf_fd;
    #ifdef __linux__
    unsigned long base_addr = dmem.u.device.device_addr;
    #else
    unsigned long long base_addr = dmem.u.device.device_addr;
    #endif
    for (int i = 0; i < image_num; i++) {
        // std::lock_guard<std::mutex> lock(images[i].image_private->memory_lock);
        pthread_mutex_lock(&images[i].image_private->memory_lock);
        bm_device_mem_t             tmp_dmem[16];
        memset(tmp_dmem, 0x0, sizeof(tmp_dmem));
        for (int idx = 0; idx < images[i].image_private->plane_num; idx++) {
            tmp_dmem[idx] = bm_mem_from_device(
                base_addr, images[i].image_private->memory_layout[idx].size);
            tmp_dmem[idx].flags.u.gmem_heapid  = dmem.flags.u.gmem_heapid;
            tmp_dmem[idx].u.device.dmabuf_fd = dmabuf_fd;
            base_addr += images[i].image_private->memory_layout[idx].size;
        }
        bm_image_attach(images[i], tmp_dmem);
        pthread_mutex_unlock(&images[i].image_private->memory_lock);
    }

    return BM_SUCCESS;
}

bm_status_t bm_image_free_contiguous_mem(int image_num, bm_image *images) {
    if (0 == image_num) {
        BMCV_ERR_LOG("[FREE]image_num can not be set as 0\n");
        return BM_ERR_PARAM;
    }
    for (int i = 0; i < image_num; i++) {
        if (images[i].image_private->data_owned) {
            return BM_ERR_DATA;
        }
        bm_image_detach(images[i]);
    }
    bm_device_mem_t dmem[16];
    bm_image_get_device_mem(images[0], dmem);
    int dmabuf_fd       = dmem->u.device.dmabuf_fd;
    u64 base_addr       = bm_mem_get_device_addr(dmem[0]);
    int single_image_sz = 0;
    for (int idx = 0; idx < images[0].image_private->plane_num; idx++) {
        single_image_sz += images[0].image_private->memory_layout[idx].size;
    }
    int             total_image_size = image_num * single_image_sz;
    bm_device_mem_t release_dmem;
    release_dmem = bm_mem_from_device(base_addr, total_image_size);
    release_dmem.u.device.dmabuf_fd = dmabuf_fd;
    release_dmem.flags.u.gmem_heapid = dmem->flags.u.gmem_heapid;
    bm_free_device(images[0].image_private->handle, release_dmem);

    return BM_SUCCESS;
}


bm_status_t bm_image_attach_contiguous_mem(int             image_num,
                                           bm_image *      images,
                                           bm_device_mem_t dmem) {
    if (0 == image_num) {
        BMCV_ERR_LOG("[ALLOC]image_num can not be set as 0\n");
        return BM_ERR_PARAM;
    }
    for (int i = 0; i < image_num - 1; i++) {
        for (int idx = 0; idx < images[i].image_private->plane_num; idx++) {
            if (images[i].image_private->memory_layout[idx].size !=
                images[i + 1].image_private->memory_layout[idx].size) {
                BMCV_ERR_LOG("all image must have same size\n");
                return BM_ERR_DATA;
            }
        }
    }
    #ifdef __linux__
    unsigned long base_addr = dmem.u.device.device_addr;
    #else
    unsigned long long base_addr = dmem.u.device.device_addr;
    #endif
    int           dmabuf_fd = dmem.u.device.dmabuf_fd;
    for (int i = 0; i < image_num; i++) {
        pthread_mutex_lock(&images[i].image_private->memory_lock);
        bm_device_mem_t             tmp_dmem[16];
        memset(tmp_dmem, 0x0, sizeof(tmp_dmem));
        for (int idx = 0; idx < images[i].image_private->plane_num; idx++) {
            tmp_dmem[idx] = bm_mem_from_device(
                base_addr, images[i].image_private->memory_layout[idx].size);
            tmp_dmem[idx].u.device.dmabuf_fd = dmabuf_fd;
            base_addr += images[i].image_private->memory_layout[idx].size;
        }
        if (images[i].image_private->attached) {
            bm_image_detach(images[i]);
        }
        bm_image_attach(images[i], tmp_dmem);
    }

    return BM_SUCCESS;
}

bm_status_t bm_image_get_contiguous_device_mem(int              image_num,
                                               bm_image *       images,
                                               bm_device_mem_t *mem) {
    if (0 == image_num) {
        BMCV_ERR_LOG("image_num can not be set as 0\n");
        return BM_ERR_PARAM;
    }
    for (int i = 0; i < image_num - 1; i++) {
        for (int idx = 0; idx < images[i].image_private->plane_num; idx++) {
            if (images[i].image_private->memory_layout[idx].size !=
                images[i + 1].image_private->memory_layout[idx].size) {
                BMCV_ERR_LOG("all image must have same size\n");
                return BM_ERR_DATA;
            }
        }
        if (!(images[i].image_private->attached)) {
            BMCV_ERR_LOG("image has not been attached memory\n");
            return BM_ERR_DATA;
        }
    }
    bm_device_mem_t dmem;
    bm_image_get_device_mem(images[0], &dmem);
    int dmabuf_fd       = dmem.u.device.dmabuf_fd;
    u64 base_addr       = bm_mem_get_device_addr(dmem);
    int single_image_sz = 0;
    for (int idx = 0; idx < images[0].image_private->plane_num; idx++) {
        single_image_sz += images[0].image_private->memory_layout[idx].size;
    }
    int total_image_size = image_num * single_image_sz;
    for (int i = 1; i < image_num; i++) {
        bm_device_mem_t tmp_dmem;
        bm_image_get_device_mem(images[i], &tmp_dmem);
        u64 tmp_addr = bm_mem_get_device_addr(tmp_dmem);
        if ((base_addr + i * single_image_sz) != tmp_addr) {
            BMCV_ERR_LOG("images should have continuous mem\r\n");

            return BM_ERR_DATA;
        }
    }
    bm_device_mem_t out_dmem;
    out_dmem = bm_mem_from_device(base_addr, total_image_size);
    out_dmem.u.device.dmabuf_fd = dmabuf_fd;
    memcpy(mem, &out_dmem, sizeof(bm_device_mem_t));

    return BM_SUCCESS;
}

bm_status_t bm_image_detach_contiguous_mem(int image_num, bm_image *images) {
    for (int i = 0; i < image_num; i++) {
        if (images[i].image_private->data_owned) {
            BMCV_ERR_LOG("image mem can not be free\n");
            return BM_ERR_FAILURE;
        }
        bm_image_detach(images[i]);
    }

    return BM_SUCCESS;
}

bm_status_t bm_image_write_to_bmp(bm_image image, const char *filename) {
    if (!image.image_private || !image.image_private->attached)
        return BM_ERR_DATA;
    int need_format_transform = (image.image_format != FORMAT_RGB_PACKED &&
                                 image.image_format != FORMAT_GRAY) ||
                                image.data_type != DATA_TYPE_EXT_1N_BYTE;
    bm_image image_temp = image;

    if (need_format_transform) {
        bm_status_t ret = bm_image_create(image.image_private->handle,
                                          image.height,
                                          image.width,
                                          FORMAT_RGB_PACKED,
                                          DATA_TYPE_EXT_1N_BYTE,
                                          &image_temp,
                                          NULL);
        if (ret != BM_SUCCESS) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "malloc failed\n");
            return ret;
        }
        ret = bmcv_image_storage_convert(
            image.image_private->handle, 1, &image, &image_temp);
        if (ret != BM_SUCCESS) {
            bmlib_log(BMCV_LOG_TAG,
                      BMLIB_LOG_ERROR,
                      "failed to convert to suitable format\n");
            bm_image_destroy(&image_temp);
            return ret;
        }
    }
    int component = image.image_format == FORMAT_GRAY ? 1 : 3;
    int stride[4] = {0};
    bm_image_get_stride(image_temp, stride);
    int line_size = image_temp.width * component;
    void       *buf_tmp = malloc(stride[0] * image_temp.height);
    void       *buf     = malloc(line_size * image_temp.height);
    bm_status_t ret     = bm_image_copy_device_to_host(image_temp, &buf_tmp);

    if (stride[0] > line_size) {
        for (int i = 0; i < image_temp.height; i++) {
            memcpy((unsigned char *)buf + i * line_size, (unsigned char *)buf_tmp + i * stride[0], line_size);
        }
    } else {
        memcpy((unsigned char *)buf, (unsigned char *)buf_tmp, stride[0] * image_temp.height);
    }
    free(buf_tmp);

    if (ret != BM_SUCCESS) {
        free(buf);
        if (need_format_transform) {
            bm_image_destroy(&image_temp);
        }
        bmlib_log(BMCV_LOG_TAG,
                  BMLIB_LOG_ERROR,
                  "failed to copy from device memory\n");
        return ret;
    }

    if (stbi_write_bmp(filename, image_temp.width, image_temp.height, component, buf) ==
        0) {
        free(buf);
        if (need_format_transform) {
            bm_image_destroy(&image_temp);
        }
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "failed to write to file\n");
        return BM_ERR_FAILURE;
    }


    free(buf);
    if (need_format_transform) {
        bm_image_destroy(&image_temp);
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_width_align(bm_handle_t handle,
                             bm_image    input,
                             bm_image    output) {
    if (input.image_format == FORMAT_COMPRESSED) {
        bmlib_log(BMCV_LOG_TAG,
                  BMLIB_LOG_INFO,
                  "Compressed format not support stride align\n");
        return BM_SUCCESS;
    }

    if (input.width != output.width || input.height != output.height ||
        input.image_format != output.image_format ||
        input.data_type != output.data_type) {
        bmlib_log(BMCV_LOG_TAG,
                  BMLIB_LOG_ERROR,
                  "input image and outuput image have different size or format "
                  " %s: %s: %d\n",
                  filename(__FILE__),
                  __func__,
                  __LINE__);
        return BM_ERR_DATA;
    }

    if (!bm_image_is_attached(input)) {
        bmlib_log(BMCV_LOG_TAG,
                  BMLIB_LOG_ERROR,
                  "input image not attache device memory  %s: %s: %d\n",
                  filename(__FILE__),
                  __func__,
                  __LINE__);
        return BM_ERR_DATA;
    }
    if (!bm_image_is_attached(output)) {
        if (BM_SUCCESS != bm_image_alloc_dev_mem(output, BMCV_HEAP1_ID)) {
            BMCV_ERR_LOG("bm_image_alloc_dev_mem error\r\n");

            return BM_ERR_NOMEM;
        }
    }

    int plane_num = bm_image_get_plane_num(input);
    bm_device_mem_t src_mem[4];
    bm_device_mem_t dst_mem[4];
    bm_image_get_device_mem(input, src_mem);
    bm_image_get_device_mem(output, dst_mem);
    for (int p = 0; p < plane_num; p++) {
        update_memory_layout(handle,
                            src_mem[p],
                            input.image_private->memory_layout[p],
                            dst_mem[p],
                            output.image_private->memory_layout[p]);
    }

    return BM_SUCCESS;
}

/**
 * Abandoned interface, supports compatibility settings
 * Not recommended for use
*/

bm_status_t bmcv_image_crop(
    bm_handle_t         handle,
    int                 crop_num,
    bmcv_rect_t *       rects,
    bm_image            input,
    bm_image *          output){
    return bmcv_image_vpp_convert(handle, crop_num, input, output, rects, BMCV_INTER_LINEAR);
}

bm_status_t bm_image_dev_mem_alloc(bm_image image, int heap_id){
    return bm_image_alloc_dev_mem(image, heap_id);
}

bm_status_t bm_image_dettach_contiguous_mem(int image_num, bm_image *images){
    return bm_image_detach_contiguous_mem(image_num, images);
}

bm_status_t bmcv_image_yuv2bgr_ext(
    bm_handle_t handle,
    int         image_num,
    bm_image *  input,
    bm_image *  output){
    return bmcv_image_storage_convert(handle, image_num, input, output);
}

void bmcv_print_version() {
    const char *env_val = getenv("BMCV_PRINT_VERSION");
    if (env_val == NULL || strcmp(env_val, "1") != 0) {
        return;
    }
    const char *fw_fname = "libbm1688_kernel_module.so";
    static char fw_path[512] = {0};
    static char bmcv_path[512] = {0};
    char cmd[1024] = {0};
    char *ptr;
    int ret = 0;
#ifdef __linux__
    Dl_info dl_info;

    ret = dladdr((void*)bmcv_print_version, &dl_info);
    if (ret == 0){
        printf("dladdr() failed: %s\n", dlerror());
        return;
    }
    if (dl_info.dli_fname == NULL){
        printf("%s is NOT a symbol\n", __FUNCTION__);
        return;
    }

    ptr = (char*)strrchr(dl_info.dli_fname, '/');
    if (!ptr){
        printf("Invalid absolute path name of libbmcv.so\n");
        return;
    }

    int dirname_len = ptr - dl_info.dli_fname + 1;
    if (dirname_len <= 0){
        printf("Invalid length of folder name\n");
        return;
    }

    strncpy(bmcv_path, dl_info.dli_fname, dirname_len);
    strcat(bmcv_path, ptr + 1);
    printf("libbmcv_path:%s\n", bmcv_path);
    sprintf(cmd, "strings %s | grep -E \"libbmcv_version:.*, branch:.*, minor version:.*, commit hash:.*\" | sed -n \'2p\'", bmcv_path);
    ret = system(cmd);
    if (ret != 0) {
        printf("Error print tpu_firmware_version!\n");
    }

    if (0 != find_tpufirmaware_path(fw_path, fw_fname)) {
        printf("libbm1684x_kernel_module.so does not exist\n");
        return;
    }

    printf("tpu_firmware_path:%s\n", fw_path);
    memset (cmd, 0, sizeof(cmd));
    sprintf(cmd, "strings %s | grep -E \"tpu_firmware_version:.*, branch:.*, minor version:.*, commit:.*\"", fw_path);
    ret = system(cmd);
    if (ret != 0) {
        printf("Error print tpu_firmware_version!\n");
    }
#endif
    return;
}
