#include "bmcv_api_ext_c.h"
#include "bmcv_internal.h"
#include "bmcv_a2_ive_internal.h"
#include "bmcv_a2_ive_ext.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/ioctl.h>
#endif

#define IVE_DEV_NODE "/dev/soph-ive"

struct IVE_HANDLE_CTX {
    int dev_fd;
    int used_time;
};

static struct IVE_HANDLE_CTX g_handle_ctx;
static pthread_mutex_t g_handle_ctx_lock = PTHREAD_MUTEX_INITIALIZER;

int open_ive_dev()
{
    int dev_fd = -1;

    pthread_mutex_lock(&g_handle_ctx_lock);
    if (g_handle_ctx.dev_fd <= 0) {
        dev_fd = open(IVE_DEV_NODE, O_RDWR);
        if (dev_fd < 0) {
            pthread_mutex_unlock(&g_handle_ctx_lock);
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                    "Can't open %s %s: %s: %d\n",
                    IVE_DEV_NODE, filename(__FILE__), __func__, __LINE__);
            return -1;
        }
        else {
            g_handle_ctx.dev_fd = dev_fd;
            g_handle_ctx.used_time = 1;
        }
    }
    else {
        dev_fd = g_handle_ctx.dev_fd;
        g_handle_ctx.used_time++;
    }
    pthread_mutex_unlock(&g_handle_ctx_lock);

    return dev_fd;
}

ive_handle BM_IVE_CreateHandle()
{
    int ret = -1;

    ret = open_ive_dev();
    if (ret < 0) {
        return NULL;
    }

    return &g_handle_ctx;
}

void close_ive_dev()
{
    pthread_mutex_lock(&g_handle_ctx_lock);
    if (g_handle_ctx.dev_fd <= 0) {
        pthread_mutex_unlock(&g_handle_ctx_lock);
        // ive device is not opened, do nothing
        return;
    }
    else {
        g_handle_ctx.used_time--;
        if (g_handle_ctx.used_time <= 0 && g_handle_ctx.dev_fd > 0) {
            close(g_handle_ctx.dev_fd);
            g_handle_ctx.dev_fd = -1;
            g_handle_ctx.used_time = 0;
        }
    }
    pthread_mutex_unlock(&g_handle_ctx_lock);
    return;
}

void BM_IVE_DestroyHandle(ive_handle ive_handle)
{
    // ive_handle is not used here
    close_ive_dev();
    return;
}

ive_image_type_e bm_image_type_convert_to_ive_image_type(bm_image_format_ext image_format, bm_image_data_format_ext data_type)
{
    ive_image_type_e type = IVE_IMAGE_TYPE_BUTT;

    switch (image_format) {
        case FORMAT_GRAY:
            if (data_type == DATA_TYPE_EXT_1N_BYTE) {
                type = IVE_IMAGE_TYPE_U8C1;
            } else if (data_type == DATA_TYPE_EXT_1N_BYTE_SIGNED) {
                type = IVE_IMAGE_TYPE_S8C1;
            } else if (data_type == DATA_TYPE_EXT_U16) {
                type = IVE_IMAGE_TYPE_U16C1;
            } else if (data_type == DATA_TYPE_EXT_S16) {
                type = IVE_IMAGE_TYPE_S16C1;
            } else if (data_type == DATA_TYPE_EXT_U32){
                type = IVE_IMAGE_TYPE_U32C1;
            } else {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_WARNING,
                        "unsupported image format for ive, image_format = %d, data_type = %d, %s: %s: %d\n",
                        image_format, data_type, filename(__FILE__), __func__, __LINE__);
                return IVE_IMAGE_TYPE_BUTT;
            }
            break;
        case FORMAT_YUV420P:
            type = IVE_IMAGE_TYPE_YUV420P;
            break;
        case FORMAT_NV21:
            type = IVE_IMAGE_TYPE_YUV420SP;
            break;
        case FORMAT_RGBP_SEPARATE:
        case FORMAT_BGRP_SEPARATE:
        case FORMAT_HSV_PLANAR:
        case FORMAT_RGB_PLANAR:
        case FORMAT_BGR_PLANAR:
        case FORMAT_YUV444P:
            type = IVE_IMAGE_TYPE_U8C3_PLANAR;
            break;
        case FORMAT_NV61:
            type = IVE_IMAGE_TYPE_YUV422SP;
            break;
        case FORMAT_RGB_PACKED:
        case FORMAT_BGR_PACKED:
        case FORMAT_YUV444_PACKED:
        case FORMAT_YVU444_PACKED:
            type = IVE_IMAGE_TYPE_U8C3_PACKAGE;
            break;
        default:
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_WARNING,
                    "unsupported image format for ive, image_format = %d, data_type = %d, %s: %s: %d\n",
                    image_format, data_type, filename(__FILE__), __func__, __LINE__);
            return IVE_IMAGE_TYPE_BUTT;
    }

    return type;
}

bm_status_t bm_image_convert_to_ive_image(bm_handle_t handle, bm_image *bm_img, ive_image_s *ive_img)
{
     int plane_num = 0;
    int stride[4];
    bm_device_mem_t device_mem[4];
    bm_image_format_ext image_format;
    bm_image_data_format_ext data_type;
    ive_image_type_e type;
    int i;
    int planar_to_seperate = 0;
    if((bm_img->image_format == FORMAT_RGB_PLANAR || bm_img->image_format == FORMAT_BGR_PLANAR))
        planar_to_seperate = 1;

    plane_num = bm_image_get_plane_num(*bm_img);
    if(plane_num == 0 || bm_image_get_device_mem(*bm_img, device_mem) != BM_SUCCESS)
    {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "not correctly create input bm_image, get plane num or device mem err %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    if((bm_image_get_stride(*bm_img, stride) != BM_SUCCESS)) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "not correctly create input bm_image, input get stride err %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // transfer image format depends on image format and data type
    image_format = bm_img->image_format;
    data_type = bm_img->data_type;
    type = bm_image_type_convert_to_ive_image_type(image_format, data_type);

    ive_img->type = type;
    ive_img->width = bm_img->width;
    ive_img->height = bm_img->height;

    for(i = 0; i < plane_num; i++){
        ive_img->stride[i] = stride[i];
        ive_img->phy_addr[i] = device_mem[i].u.device.device_addr;
    }

    if(planar_to_seperate){
        for(int i = 0; i < 3; ++i){
            ive_img->stride[i] = stride[0];
            ive_img->phy_addr[i] = device_mem[0].u.device.device_addr + (stride[0] * bm_img->height * i);
        }
    }

    return BM_SUCCESS;
}

bm_status_t bm_image_convert_to_ive_data(bm_handle_t handle, int i, bm_image *bm_img, ive_data_s *ive_data)
{
    int plane_num = 0;
    int stride[4];
    bm_device_mem_t device_mem[4];

    plane_num = bm_image_get_plane_num(*bm_img);
    if (plane_num == 0 || bm_image_get_device_mem(*bm_img, device_mem) != BM_SUCCESS)
    {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "not correctly create input bm_image, get plane num or device mem err %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    if ((bm_image_get_stride(*bm_img, stride) != BM_SUCCESS)) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "not correctly create input bm_image, input get stride err %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ive_data->width = bm_img->image_private->memory_layout[i].W;
    ive_data->height = bm_img->image_private->memory_layout[i].H;
    ive_data->phy_addr = device_mem[i].u.device.device_addr;
    ive_data->stride = stride[i];

    return BM_SUCCESS;
}

int bm_image_convert_to_map_mode(bm_image src, bm_image dst) {
    if (src.data_type == DATA_TYPE_EXT_1N_BYTE) {
        if (dst.data_type == DATA_TYPE_EXT_1N_BYTE) {
            return IVE_MAP_MODE_U8;
        } else if (dst.data_type == DATA_TYPE_EXT_S16)
        {
            return IVE_MAP_MODE_S16;
        } else if (dst.data_type == DATA_TYPE_EXT_U16)
        {
            return IVE_MAP_MODE_U16;
        } else {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "Map dst datatype is illegal, please check it %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
            return IVE_MAP_MODE_BUTT;
        }
    } else {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Map src datatype is illegal, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return IVE_MAP_MODE_BUTT;
    }
}

bm_status_t BM_IVE_Add(ive_handle ive_handle, ive_src_image_s *psrc1,
            ive_src_image_s *psrc2, ive_dst_image_s *pdst,
            ive_add_ctrl_s *pctrl, bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_add_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    // check if ive device is opened
    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src1 = *psrc1;
    ive_arg.src2 = *psrc2;
    ive_arg.dst = *pdst;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    // invoke ive api in driver
    ioctl_arg.input_data = (uint64_t)&ive_arg;
    if (ioctl(p->dev_fd, IVE_IOC_ADD, &ioctl_arg) < 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "IVE_IOC_Add failed %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t BM_IVE_And(ive_handle ive_handle, ive_src_image_s *psrc1,
            ive_src_image_s *psrc2, ive_dst_image_s *pdst, bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_and_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    // check if ive device is opened
    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src1 = *psrc1;
    ive_arg.src2 = *psrc2;
    ive_arg.dst = *pdst;
    ive_arg.instant = instant;

    // invoke ive api in driver
    ioctl_arg.input_data = (uint64_t)&ive_arg;
    if (ioctl(p->dev_fd, IVE_IOC_AND, &ioctl_arg) < 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "IVE_IOC_And failed %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t BM_IVE_Sub(ive_handle ive_handle, ive_src_image_s *psrc1,
            ive_src_image_s *psrc2, ive_dst_image_s *pdst,
            ive_sub_ctrl_s *pctrl, bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_sub_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    // check if ive device is opened
    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src1 = *psrc1;
    ive_arg.src2 = *psrc2;
    ive_arg.dst = *pdst;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    // invoke ive api in driver
    ioctl_arg.input_data = (uint64_t)&ive_arg;
    if (ioctl(p->dev_fd, IVE_IOC_SUB, &ioctl_arg) < 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "CVI_IVE_IOC_Sub failed %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t BM_IVE_Or(ive_handle ive_handle, ive_src_image_s *psrc1,
            ive_src_image_s *psrc2, ive_dst_image_s *pdst, bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_or_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    // check if ive device is opened
    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src1 = *psrc1;
    ive_arg.src2 = *psrc2;
    ive_arg.dst = *pdst;
    ive_arg.instant = instant;

    // invoke ive api in driver
    ioctl_arg.input_data = (uint64_t)&ive_arg;
    if (ioctl(p->dev_fd, IVE_IOC_OR, &ioctl_arg) < 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "CVI_IVE_IOC_Or failed %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t BM_IVE_Xor(ive_handle ive_handle, ive_src_image_s *psrc1,
            ive_src_image_s *psrc2, ive_dst_image_s *pdst, bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_xor_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    // check if ive device is opened
    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src1 = *psrc1;
    ive_arg.src2 = *psrc2;
    ive_arg.dst = *pdst;
    ive_arg.instant = instant;

    // invoke ive api in driver
    ioctl_arg.input_data = (uint64_t)&ive_arg;
    if (ioctl(p->dev_fd, IVE_IOC_XOR, &ioctl_arg) < 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "CVI_IVE_IOC_Xor failed %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t bm_ive_add(
    bm_handle_t          handle,
    bm_image             input1,
    bm_image             input2,
    bm_image             output,
    bmcv_ive_add_attr    attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src1, src2;
    ive_dst_image_s dst;
    ive_add_ctrl_s stAddCtrl;

    memset(&src1, 0, sizeof(ive_src_image_s));
    memset(&src2, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_image_s));
    memset(&stAddCtrl, 0, sizeof(ive_add_ctrl_s));

    // transfer bm struct to ive struct
    ret = bm_image_convert_to_ive_image(handle, &input1, &src1);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &input2, &src2);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &output, &dst);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    memcpy(&stAddCtrl, (void*)&attr, sizeof(ive_add_ctrl_s));
    ret = BM_IVE_Add(ive_handle, &src1, &src2, &dst, &stAddCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "bm_ive_add error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }
    BM_IVE_DestroyHandle(ive_handle);

    return ret;
}

bm_status_t bm_ive_and(
    bm_handle_t         handle,
    bm_image            input1,
    bm_image            input2,
    bm_image            output)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src1, src2;
    ive_dst_image_s dst;

    memset(&src1, 0, sizeof(ive_src_image_s));
    memset(&src2, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_image_s));

    // transfer bm struct to ive struct
    ret = bm_image_convert_to_ive_image(handle, &input1, &src1);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &input2, &src2);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &output, &dst);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_And(ive_handle, &src1, &src2, &dst, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "bm_ive_and error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }
    BM_IVE_DestroyHandle(ive_handle);

    return ret;
}

bm_status_t bm_ive_or(
    bm_handle_t          handle,
    bm_image             input1,
    bm_image             input2,
    bm_image             output)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src1, src2;
    ive_dst_image_s dst;

    memset(&src1, 0, sizeof(ive_src_image_s));
    memset(&src2, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_image_s));

    // transfer bm struct to ive struct
    ret = bm_image_convert_to_ive_image(handle, &input1, &src1);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &input2, &src2);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &output, &dst);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_Or(ive_handle, &src1, &src2, &dst, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "bm_ive_and error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }
    BM_IVE_DestroyHandle(ive_handle);

    return ret;
}

bm_status_t bm_ive_sub(
    bm_handle_t          handle,
    bm_image             input1,
    bm_image             input2,
    bm_image             output,
    bmcv_ive_sub_attr    attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src1, src2;
    ive_dst_image_s dst;
    ive_sub_ctrl_s stSubCtrl;

    memset(&src1, 0, sizeof(ive_src_image_s));
    memset(&src2, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_image_s));
    memset(&stSubCtrl, 0, sizeof(ive_sub_ctrl_s));

    // transfer bm struct to ive struct
    ret = bm_image_convert_to_ive_image(handle, &input1, &src1);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &input2, &src2);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &output, &dst);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    memcpy(&stSubCtrl, (void*)&attr, sizeof(ive_sub_ctrl_s));
    ret = BM_IVE_Sub(ive_handle, &src1, &src2, &dst, &stSubCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "bm_ive_sub error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }
    BM_IVE_DestroyHandle(ive_handle);

    return ret;
}


bm_status_t bm_ive_xor(
    bm_handle_t          handle,
    bm_image             input1,
    bm_image             input2,
    bm_image             output)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src1, src2;
    ive_dst_image_s dst;

    memset(&src1, 0, sizeof(ive_src_image_s));
    memset(&src2, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_image_s));

    // transfer bm struct to ive struct
    ret = bm_image_convert_to_ive_image(handle, &input1, &src1);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &input2, &src2);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &output, &dst);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_Xor(ive_handle, &src1, &src2, &dst, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "bm_ive_xor error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }
    BM_IVE_DestroyHandle(ive_handle);

    return ret;
}

bm_status_t BM_IVE_Thresh(ive_handle ive_handle, ive_src_image_s *psrc,
            ive_dst_image_s *pdst, ive_thresh_ctrl_s *pctrl, bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_thresh_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    // check if ive device is opened
    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    ive_arg.dst = *pdst;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    // invoke ive api in driver
    ioctl_arg.input_data = (uint64_t)&ive_arg;
    if (ioctl(p->dev_fd, IVE_IOC_THRESH, &ioctl_arg) < 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "CVI_IVE_IOC_Thresh failed %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t BM_IVE_Thresh_U16(ive_handle ive_handle, ive_src_image_s *psrc,
            ive_dst_image_s *pdst, ive_thresh_u16_ctrl_s *pctrl, bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_thresh_u16_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    // check if ive device is opened
    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    ive_arg.dst = *pdst;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    // invoke ive api in driver
    ioctl_arg.input_data = (uint64_t)&ive_arg;
    if (ioctl(p->dev_fd, IVE_IOC_THRESH_U16, &ioctl_arg) < 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "CVI_IVE_IOC_Thresh_U16 failed %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t BM_IVE_Thresh_S16(ive_handle ive_handle, ive_src_image_s *psrc,
            ive_dst_image_s *pdst, ive_thresh_s16_ctrl_s *pctrl, bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_thresh_s16_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    // check if ive device is opened
    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    ive_arg.dst = *pdst;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    // invoke ive api in driver
    ioctl_arg.input_data = (uint64_t)&ive_arg;
    if (ioctl(p->dev_fd, IVE_IOC_THRESH_S16, &ioctl_arg) < 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "CVI_IVE_IOC_Thresh_S16 failed %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

// threshMode to
int thransform_pattern(bmcv_ive_thresh_mode thresh_mode)
{
    int mode = 0;
    switch(thresh_mode)
    {
        // transform IVE_THRESH_MODE_E
        case IVE_THRESH_BINARY:
            mode = IVE_THRESH_MODE_BINARY;
            break;
        case IVE_THRESH_TRUNC:
            mode = IVE_THRESH_MODE_TRUNC;
            break;
        case IVE_THRESH_TO_MINAL:
            mode = IVE_THRESH_MODE_TO_MINVAL;
            break;
        case IVE_THRESH_MIN_MID_MAX:
            mode = IVE_THRESH_MODE_MIN_MID_MAX;
            break;
        case IVE_THRESH_ORI_MID_MAX:
            mode = IVE_THRESH_MODE_ORI_MID_MAX;
            break;
        case IVE_THRESH_MIN_MID_ORI:
            mode = IVE_THRESH_MODE_MIN_MID_ORI;
            break;
        case IVE_THRESH_MIN_ORI_MAX:
            mode = IVE_THRESH_MODE_MIN_ORI_MAX;
            break;
        case IVE_THRESH_ORI_MID_ORI:
            mode = IVE_THRESH_MODE_ORI_MID_ORI;
            break;

        // transform IVE_THRESH_S16_MODE_E
        case IVE_THRESH_S16_TO_S8_MIN_MID_MAX:
            mode = IVE_THRESH_S16_MODE_S16_TO_S8_MIN_MID_MAX;
            break;
        case IVE_THRESH_S16_TO_S8_MIN_ORI_MAX:
            mode = IVE_THRESH_S16_MODE_S16_TO_S8_MIN_MID_MAX;
            break;
        case IVE_THRESH_S16_TO_U8_MIN_MID_MAX:
            mode = IVE_THRESH_S16_MODE_S16_TO_U8_MIN_MID_MAX;
            break;
        case IVE_THRESH_S16_TO_U8_MIN_ORI_MAX:
            mode = IVE_THRESH_S16_MODE_S16_TO_U8_MIN_ORI_MAX;
            break;

        // transform IVE_THRESH_U16_MODE_E
        case IVE_THRESH_U16_TO_U8_MIN_MID_MAX:
            mode = IVE_THRESH_U16_MODE_U16_TO_U8_MIN_MID_MAX;
            break;
        case IVE_THRESH_U16_TO_U8_MIN_ORI_MAX:
            mode = IVE_THRESH_U16_MODE_U16_TO_U8_MIN_ORI_MAX;
            break;
        default:
            printf("thresh mode not supported, thresh_mode is %d \n", thresh_mode);
            break;
    }
    return mode;
}

int bm_image_convert_to_thresh_opType(bm_image src, bm_image dst)
{
    switch (src.data_type) {
        case DATA_TYPE_EXT_1N_BYTE:
            if (dst.data_type == DATA_TYPE_EXT_1N_BYTE){
                return MOD_U8;
            }
            break;
        case DATA_TYPE_EXT_U16:
            if (dst.data_type == DATA_TYPE_EXT_1N_BYTE){
                return MOD_U16;
            }
            break;
        case DATA_TYPE_EXT_S16:
            if (dst.data_type == DATA_TYPE_EXT_1N_BYTE || dst.data_type == DATA_TYPE_EXT_1N_BYTE_SIGNED){
                return MOD_S16;
            }
            break;
        default:
            break;
    }
    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
              "Data_type is invalid, please check it, %s: %s: %d\n",
              filename(__FILE__), __func__, __LINE__);
    return -1;
}

bm_status_t bm_ive_thresh(
    bm_handle_t             handle,
    bm_image                input,
    bm_image                output,
    bmcv_ive_thresh_mode    thresh_mode,
    bmcv_ive_thresh_attr    attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src;
    ive_dst_image_s dst;
    ive_thresh_ctrl_s stThreshCtrl;
    ive_thresh_u16_ctrl_s stThreshU16Ctrl;
    ive_thresh_s16_ctrl_s stThreshS16Ctrl;

    memset(&src, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_image_s));
    memset(&stThreshCtrl, 0, sizeof(ive_thresh_ctrl_s));
    memset(&stThreshU16Ctrl, 0, sizeof(ive_thresh_u16_ctrl_s));
    memset(&stThreshS16Ctrl, 0, sizeof(ive_thresh_s16_ctrl_s));

    // transfer bm struct to ive struct
    ret = bm_image_convert_to_ive_image(handle, &input, &src);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &output, &dst);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    int opType = bm_image_convert_to_thresh_opType(input, output);
    switch (opType) {
        case MOD_U8:
            stThreshCtrl.mode = (ive_thresh_mode_e)thransform_pattern(thresh_mode);
            if (stThreshCtrl.mode < 0 || stThreshCtrl.mode >= IVE_THRESH_MODE_BUTT) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                        "unknown thresh mode %d, %s: %s: %d\n",
                        thresh_mode, filename(__FILE__), __func__, __LINE__);
                break;
            }

            stThreshCtrl.low_thr = (u8)(attr.low_thr & 0xff);
            stThreshCtrl.high_thr = (u8)(attr.high_thr & 0xff);
            stThreshCtrl.min_val = (u8)(attr.min_val & 0xff);
            stThreshCtrl.mid_val = (u8)(attr.mid_val & 0xff);
            stThreshCtrl.max_val = (u8)(attr.max_val & 0xff);
            ret = BM_IVE_Thresh(ive_handle, &src, &dst, &stThreshCtrl, true);
            break;
        case MOD_U16:
            stThreshU16Ctrl.mode = (ive_thresh_u16_mode_e)thransform_pattern(thresh_mode);
            if (stThreshU16Ctrl.mode < 0 || stThreshU16Ctrl.mode >= IVE_THRESH_U16_MODE_BUTT) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                        "unknown thresh mode %d, %s: %s: %d\n",
                        thresh_mode, filename(__FILE__), __func__, __LINE__);
                break;
            }

            stThreshU16Ctrl.low_thr = (u16)(attr.low_thr & 0xffff);
            stThreshU16Ctrl.high_thr = (u16)(attr.high_thr & 0xffff);
            stThreshU16Ctrl.min_val = (u8)(attr.min_val & 0xff);
            stThreshU16Ctrl.mid_val = (u8)(attr.mid_val & 0xff);
            stThreshU16Ctrl.max_val = (u8)(attr.max_val & 0xff);
            ret = BM_IVE_Thresh_U16(ive_handle, &src, &dst, &stThreshU16Ctrl, true);
            break;
        case MOD_S16:
            stThreshS16Ctrl.mode = (ive_thresh_s16_mode_e)thransform_pattern(thresh_mode);
            if (stThreshS16Ctrl.mode < 0 || stThreshS16Ctrl.mode >= IVE_THRESH_S16_MODE_BUTT) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                        "unknown thresh mode %d, %s: %s: %d\n",
                        thresh_mode, filename(__FILE__), __func__, __LINE__);
                break;
            }

            stThreshS16Ctrl.low_thr = (s16)(attr.low_thr & 0xffff);
            stThreshS16Ctrl.high_thr = (s16)(attr.high_thr & 0xffff);
            if (stThreshS16Ctrl.mode == IVE_THRESH_S16_MODE_S16_TO_S8_MIN_MID_MAX ||
                stThreshS16Ctrl.mode == IVE_THRESH_S16_MODE_S16_TO_S8_MIN_ORI_MAX) {
                    stThreshS16Ctrl.min_val.s8_val = (s8)(attr.min_val & 0xff);
                    stThreshS16Ctrl.mid_val.s8_val = (s8)(attr.mid_val & 0xff);
                    stThreshS16Ctrl.max_val.s8_val = (s8)(attr.max_val & 0xff);
            } else {
                    stThreshS16Ctrl.min_val.u8_val = (u8)(attr.min_val & 0xff);
                    stThreshS16Ctrl.mid_val.u8_val = (u8)(attr.mid_val & 0xff);
                    stThreshS16Ctrl.max_val.u8_val = (u8)(attr.max_val & 0xff);
            }
            ret = BM_IVE_Thresh_S16(ive_handle, &src, &dst, &stThreshS16Ctrl, true);
            break;
        default:
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                    "unknown opType %d, %s: %s: %d\n",
                    opType, filename(__FILE__), __func__, __LINE__);
            break;
    }

    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "bm_ive_thresh error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }
    BM_IVE_DestroyHandle(ive_handle);

    return ret;
}

bm_status_t BM_IVE_DMA(ive_handle ive_handle, ive_data_s *psrc,
            ive_dst_data_s *pdst, ive_dma_ctrl_s *pctrl,
            bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_dma_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    ive_arg.dst = *pdst;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    // invoke ive api in driver
    ioctl_arg.input_data = (uint64_t)&ive_arg;
    if (ioctl(p->dev_fd, IVE_IOC_DMA, &ioctl_arg) < 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "CVI_IVE_IOC_DMA failed %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t bm_ive_dma_set(
    bm_handle_t                      handle,
    bm_image                         image,
    bmcv_ive_dma_set_mode            dma_set_mode,
    unsigned long long               val)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_data_s src;
    ive_dst_data_s dst;
    ive_dma_ctrl_s stDmaCtrl;

    memset(&src, 0, sizeof(ive_data_s));
    memset(&dst, 0, sizeof(ive_data_s));
    memset(&stDmaCtrl, 0, sizeof(ive_dma_ctrl_s));

    // transfer bm struct to ive struct
    // TODO: only support one channel now
    ret = bm_image_convert_to_ive_data(handle, 0, &image, &src);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_data %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    if (dma_set_mode < 0 || dma_set_mode > IVE_DMA_SET_8BYTE) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "unknown dma mode %d, %s: %s: %d\n",
                dma_set_mode, filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    stDmaCtrl.mode = (ive_dma_mode_e)(dma_set_mode + 2);
    printf("stDmaCtrl.mode = %d \n", stDmaCtrl.mode);
    stDmaCtrl.val = val;
    stDmaCtrl.hor_seg_size = 0;
    stDmaCtrl.elem_size = 0;
    stDmaCtrl.ver_seg_rows = 0;

    ret = BM_IVE_DMA(ive_handle, &src, &dst, &stDmaCtrl, true);

    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "bm_ive_dma_set error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);

    return ret;
}

bm_status_t bm_ive_dma(
    bm_handle_t                     handle,
    bm_image                        input,
    bm_image                        output,
    bmcv_ive_dma_mode               dma_mode,
    bmcv_ive_interval_dma_attr *    attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_data_s src;
    ive_dst_data_s dst;
    ive_dma_ctrl_s stDmaCtrl;

    memset(&src, 0, sizeof(ive_data_s));
    memset(&dst, 0, sizeof(ive_data_s));
    memset(&stDmaCtrl, 0, sizeof(ive_dma_ctrl_s));

    // transfer bm struct to ive struct
    // TODO: only support one channel now
    ret = bm_image_convert_to_ive_data(handle, 0, &input, &src);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_data %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_data(handle, 0, &output, &dst);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_data %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    if (dma_mode < 0 || dma_mode > IVE_DMA_INTERVAL_COPY) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "unknown dma mode %d, %s: %s: %d\n",
                dma_mode, filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    stDmaCtrl.mode = (ive_dma_mode_e)dma_mode;
    if (dma_mode == 0) {
        stDmaCtrl.val = 0;
        stDmaCtrl.hor_seg_size = 0;
        stDmaCtrl.elem_size = 0;
        stDmaCtrl.ver_seg_rows = 0;
    } else if (dma_mode == 1){
        stDmaCtrl.val = 0;
        stDmaCtrl.hor_seg_size = (u8)attr->hor_seg_size;
        stDmaCtrl.elem_size = (u8)attr->elem_size;
        stDmaCtrl.ver_seg_rows = (u8)attr->ver_seg_rows;
    }

    ret = BM_IVE_DMA(ive_handle, &src, &dst, &stDmaCtrl, true);

    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "bm_ive_dma error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);

    return ret;
}

bm_status_t BM_IVE_MAP(ive_handle ive_handle, ive_src_image_s *psrc,
            ive_src_mem_info_s *pmap, ive_dst_image_s *pdst,
            ive_map_ctrl_s *pctrl, bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_map_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    ive_arg.map = *pmap;
    ive_arg.dst = *pdst;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    // invoke ive api in driver
    ioctl_arg.input_data = (uint64_t)&ive_arg;
    ioctl_arg.buffer = (void *)pmap->vir_addr;
    ioctl_arg.size = pmap->size;
    if (ioctl(p->dev_fd, IVE_IOC_MAP, &ioctl_arg) < 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "CVI_IVE_IOC_Map failed %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t bm_ive_map(
    bm_handle_t             handle,
    bm_image                input,
    bm_image                output,
    bm_device_mem_t         map_table)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_image_s src;
    ive_src_mem_info_s stTable;
    ive_dst_image_s dst;
    ive_map_ctrl_s mapCtrl;

    memset(&src, 0, sizeof(ive_image_s));
    memset(&stTable, 0, sizeof(ive_src_mem_info_s));
    memset(&dst, 0, sizeof(ive_dst_image_s));
    memset(&mapCtrl, 0, sizeof(ive_map_ctrl_s));

    // transfer bm struct to ive struct
    ret = bm_image_convert_to_ive_image(handle, &input, &src);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &output, &dst);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    unsigned long long virt_addr = 0;
    ret = bm_mem_mmap_device_mem_no_cache(handle, &map_table, &virt_addr);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to mmap device memory %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    int mapMode = bm_image_convert_to_map_mode(input, output);
    if (mapMode < 0 || mapMode >= IVE_MAP_MODE_BUTT) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Out of IVE_MAP_MODE_E mode %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    } else {
        mapCtrl.mode = (ive_map_mode_e)mapMode;
    }
    stTable.size = map_table.size;
    stTable.phy_addr = map_table.u.device.device_addr;
    stTable.vir_addr = virt_addr;

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_MAP(ive_handle, &src, &stTable, &dst, &mapCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "BM_IVE_MAP failed %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);

        bm_mem_unmap_device_mem(handle, (void *)virt_addr, MAP_TABLE_SIZE);
        return ret;
    }

    BM_IVE_DestroyHandle(ive_handle);

    bm_mem_unmap_device_mem(handle, (void *)virt_addr, MAP_TABLE_SIZE);

    return ret;
}

bm_status_t BM_IVE_Hist(ive_handle ive_handle, ive_src_image_s *psrc,
              ive_dst_mem_info_s *pdst, bool instant){
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_hist_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }
    ive_arg.ive_handle = ive_handle;
    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    ive_arg.dst = *pdst;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_HIST, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_hist(
    bm_handle_t      handle,
    bm_image         image,
    bm_device_mem_t  output)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src;
    ive_dst_mem_info_s dst;

    memset(&src, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_mem_info_s));
    ret = bm_image_convert_to_ive_image(handle, &image, &src);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    unsigned long long vir_addr = 0;
    ret = bm_mem_mmap_device_mem_no_cache(handle, &output, &vir_addr);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to mmap device memory %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    dst.vir_addr = vir_addr;
    dst.phy_addr = bm_mem_get_device_addr(output);
    dst.size = output.size;

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // instant: true is interrupt mode, false is polling mode
    ret = BM_IVE_Hist(ive_handle, &src, &dst, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_hist error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        bm_mem_unmap_device_mem(handle, (void *)vir_addr, output.size);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);
    bm_mem_unmap_device_mem(handle, (void *)vir_addr, output.size);

    return ret;
}

bm_status_t BM_IVE_Integ(ive_handle ive_handle, ive_src_image_s *psrc,
              ive_dst_mem_info_s *pdst, ive_integ_ctrl_s *pctrl,
              bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_integ_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;
    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    ive_arg.dst = *pdst;
    ive_arg.ctrl = *pctrl;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_INTEG, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_integ(
    bm_handle_t              handle,
    bm_image                 input,
    bm_device_mem_t          output,
    bmcv_ive_integ_ctrl_s    integ_attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src;
    ive_dst_mem_info_s dst;
    ive_integ_ctrl_s pstIntegCtrl;

    memset(&src, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_mem_info_s));
    memset(&pstIntegCtrl, 0, sizeof(ive_integ_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, &input, &src);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    unsigned long long vir_addr = 0;
    ret = bm_mem_mmap_device_mem_no_cache(handle, &output, &vir_addr);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to mmap device memory %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    dst.vir_addr = vir_addr;
    dst.size = output.size;
    dst.phy_addr = bm_mem_get_device_addr(output);

    // set ive integ params
    pstIntegCtrl.out_ctrl = integ_attr.en_out_ctrl;

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // instant: true is interrupt mode, false is polling mode
    ret = BM_IVE_Integ(ive_handle, &src, &dst, &pstIntegCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_intge error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        bm_mem_unmap_device_mem(handle, (void *)vir_addr, output.size);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);
    bm_mem_unmap_device_mem(handle, (void *)vir_addr, output.size);
    return ret;
}

bm_status_t BM_IVE_NCC(ive_handle ive_handle, ive_src_image_s *psrc1,
            ive_src_image_s *psrc2, ive_dst_mem_info_s *pdst,
            bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_ncc_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src1 = *psrc1;
    ive_arg.src2 = *psrc2;
    ive_arg.dst = *pdst;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    ioctl_arg.buffer = (void *)pdst->vir_addr;
    ioctl_arg.size = sizeof(ive_ncc_dst_mem_s);
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_NCC, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_ncc(
    bm_handle_t         handle,
    bm_image            input1,
    bm_image            input2,
    bm_device_mem_t     output)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src1;
    ive_src_image_s src2;
    ive_dst_mem_info_s dst;

    memset(&src1, 0, sizeof(ive_src_image_s));
    memset(&src2, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_mem_info_s));

    ret = bm_image_convert_to_ive_image(handle, &input1, &src1);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &input2, &src2);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    unsigned long long vir_addr = 0;
    ret = bm_mem_mmap_device_mem_no_cache(handle, &output, &vir_addr);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to mmap device memory %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    dst.vir_addr = vir_addr;
    dst.phy_addr = bm_mem_get_device_addr(output);
    dst.size = output.size;

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // instant: true is interrupt mode, false is polling mode
    ret = BM_IVE_NCC(ive_handle, &src1, &src2, &dst, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_ncc error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        bm_mem_unmap_device_mem(handle, (void *)vir_addr, output.size);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);
    bm_mem_unmap_device_mem(handle, (void *)vir_addr, output.size);
    return ret;
}

bm_status_t BM_IVE_OrdStatFilter(ive_handle ive_handle, ive_src_image_s *psrc,
                  ive_dst_image_s *pdst,
                  ive_ord_stat_filter_ctrl_s *pctrl,
                  bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_ord_stat_filter_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    ive_arg.dst = *pdst;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_ORD_STAT_FILTER, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_ord_stat_filter(
    bm_handle_t                   handle,
    bm_image *                    input,
    bm_image *                    output,
    bmcv_ive_ord_stat_filter_mode mode)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src;
    ive_dst_image_s dst;
    ive_ord_stat_filter_ctrl_s pctrl;

    memset(&src, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_image_s));
    memset(&pctrl, 0, sizeof(ive_ord_stat_filter_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, input, &src);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, output, &dst);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    pctrl.mode = mode;

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // instant: true is interrupt mode, false is polling mode
    ret = BM_IVE_OrdStatFilter(ive_handle, &src, &dst, &pctrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_ordStatFilter error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);
    return ret;
}

bm_status_t BM_IVE_LBP(ive_handle ive_handle, ive_src_image_s *psrc,
            ive_dst_image_s *pdst, ive_lbp_ctrl_s *pctrl,
            bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_lbp_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.src = *psrc;
    ive_arg.dst = *pdst;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_LBP, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_lbp(
    bm_handle_t              handle,
    bm_image *               input,
    bm_image *               output,
    bmcv_ive_lbp_ctrl_attr * lbp_attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src;
    ive_dst_image_s dst;
    ive_lbp_ctrl_s pstlbpCtrl;

    memset(&src, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_image_s));
    memset(&pstlbpCtrl, 0, sizeof(ive_lbp_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, input, &src);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, output, &dst);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    memcpy(&pstlbpCtrl, lbp_attr, sizeof(ive_lbp_ctrl_s));

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // instant: true is interrupt mode, false is polling mode
    ret = BM_IVE_LBP(ive_handle, &src, &dst, &pstlbpCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_lbp error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);
    return ret;
}


bm_status_t BM_IVE_Dilate(ive_handle ive_handle, ive_src_image_s *psrc,
               ive_dst_image_s *pdst, ive_dilate_ctrl_s *pctrl,
               bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_dilate_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    ive_arg.dst = *pdst;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_DILATE, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_dilate(
    bm_handle_t           handle,
    bm_image *            input,
    bm_image *            output,
    unsigned char*        dilate_mask)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src;
    ive_dst_image_s dst;
    ive_dilate_ctrl_s dilateCtrl;

    memset(&src, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_image_s));
    memset(&dilateCtrl, 0, sizeof(ive_dilate_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, input, &src);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, output, &dst);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    memcpy(&dilateCtrl.mask, dilate_mask, sizeof(ive_dilate_ctrl_s));

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // instant: true is polling mode, false is interrupt mode
    ret = BM_IVE_Dilate(ive_handle, &src, &dst, &dilateCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_dilate error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);
    return ret;
}

bm_status_t BM_IVE_Erode(ive_handle ive_handle, ive_src_image_s *psrc,
              ive_dst_image_s *pdst, ive_erode_ctrl_s *pctrl,
              bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_erode_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    ive_arg.dst = *pdst;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_ERODE, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_erode(
    bm_handle_t           handle,
    bm_image *            input,
    bm_image *            output,
    unsigned char *       erode_mask)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src;
    ive_dst_image_s dst;
    ive_erode_ctrl_s pstErodeCtrl;

    memset(&src, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_image_s));
    memset(&pstErodeCtrl, 0, sizeof(ive_erode_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, input, &src);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, output, &dst);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    // memcpy(&pstErodeCtrl, erode_attr, sizeof(ive_erode_ctrl_s));
    memcpy(&pstErodeCtrl.mask, erode_mask, sizeof(ive_erode_ctrl_s));
    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // instant: true is interrupt mode, false is polling mode
    ret = BM_IVE_Erode(ive_handle, &src, &dst, &pstErodeCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_erode error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);
    return ret;
}

bm_status_t BM_IVE_MagAndAng(ive_handle ive_handle, ive_src_image_s *psrc,
              ive_dst_image_s *pdstMag,
              ive_dst_image_s *pdstAng,
              ive_mag_and_ang_ctrl_s *pctrl, bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_maganang_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;
    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    if (pdstMag != NULL)
        ive_arg.dst_mag = *pdstMag;
    if (pdstAng != NULL)
        ive_arg.dst_ang = *pdstAng;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_MAG_AND_ANG, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_mag_and_ang(
    bm_handle_t                   handle,
    bm_image *                    input,
    bm_image *                    mag_output,
    bm_image *                    ang_output,
    bmcv_ive_mag_and_ang_ctrl *   attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src;
    ive_dst_image_s dstMag;
    ive_dst_image_s dstAng;
    ive_mag_and_ang_ctrl_s magAndAng_outCtrl;

    memset(&src, 0, sizeof(ive_src_image_s));
    memset(&dstMag, 0, sizeof(ive_dst_image_s));
    memset(&dstAng, 0, sizeof(ive_dst_image_s));
    memset(&magAndAng_outCtrl, 0, sizeof(ive_mag_and_ang_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, input, &src);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, mag_output, &dstMag);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    if(attr->en_out_ctrl == BM_IVE_MAG_AND_ANG_OUT_ALL){
        ret = bm_image_convert_to_ive_image(handle, ang_output, &dstAng);
        if(ret != BM_SUCCESS){
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to convert bm_image to ive_image %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
                return BM_ERR_FAILURE;
        }
    }

    memcpy(&magAndAng_outCtrl, attr, sizeof(ive_mag_and_ang_ctrl_s));

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // instant: true is interrupt mode, false is polling mode
    ret = BM_IVE_MagAndAng(ive_handle, &src, &dstMag, &dstAng, &magAndAng_outCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_magAndAng error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);
    return ret;
}

bm_status_t BM_IVE_Sobel(ive_handle ive_handle, ive_src_image_s *psrc,
              ive_dst_image_s *pdstH, ive_dst_image_s *pdstV,
              ive_sobel_ctrl_s *pctrl, bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_sobel_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;
    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    if(pdstH != NULL)
        ive_arg.dst_h = *pdstH;
    if(pdstV != NULL)
        ive_arg.dst_v = *pdstV;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_SOBEL, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_sobel(
    bm_handle_t           handle,
    bm_image *            input,
    bm_image *            output_h,
    bm_image *            output_v,
    bmcv_ive_sobel_ctrl * sobel_attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src;
    ive_dst_image_s dstH;
    ive_dst_image_s dstV;
    ive_sobel_ctrl_s sobelCtrl;

    memset(&src, 0, sizeof(ive_src_image_s));
    memset(&dstH, 0, sizeof(ive_dst_image_s));
    memset(&dstV, 0, sizeof(ive_dst_image_s));
    memset(&sobelCtrl, 0, sizeof(ive_sobel_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, input, &src);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    if(sobel_attr->sobel_mode == BM_IVE_SOBEL_OUT_MODE_BOTH ||
       sobel_attr->sobel_mode == BM_IVE_SOBEL_OUT_MODE_HOR){
        ret = bm_image_convert_to_ive_image(handle, output_h, &dstH);
        if(ret != BM_SUCCESS){
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to convert bm_image to ive_image %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
                return BM_ERR_FAILURE;
        }
    }

    if(sobel_attr->sobel_mode == BM_IVE_SOBEL_OUT_MODE_BOTH ||
       sobel_attr->sobel_mode == BM_IVE_SOBEL_OUT_MODE_VER){
        ret = bm_image_convert_to_ive_image(handle, output_v, &dstV);
        if(ret != BM_SUCCESS){
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to convert bm_image to ive_image %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
                return BM_ERR_FAILURE;
        }
    }

    memcpy(&sobelCtrl, sobel_attr, sizeof(ive_sobel_ctrl_s));

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // instant: true is interrupt mode, false is polling mode
    ret = BM_IVE_Sobel(ive_handle, &src, &dstH, &dstV, &sobelCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_sobel error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);
    return ret;
}

bm_status_t BM_IVE_NormGrad(ive_handle ive_handle, ive_src_image_s *psrc,
             ive_dst_image_s *pdstH, ive_dst_image_s *pdstV,
             ive_dst_image_s *pdstHV,
             ive_norm_grad_ctrl_s  *pctrl, bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_norm_grad_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;
    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    if(pdstH != NULL)
        ive_arg.dst_h = *pdstH;
    if(pdstV != NULL)
        ive_arg.dst_v = *pdstV;
    if(pdstHV != NULL)
        ive_arg.dst_hv = *pdstHV;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_NORMGRAD, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_normgrad(
    bm_handle_t              handle,
    bm_image *               input,
    bm_image *               output_h,
    bm_image *               output_v,
    bm_image *               output_hv,
    bmcv_ive_normgrad_ctrl * normgrad_attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src;
    ive_dst_image_s dstH;
    ive_dst_image_s dstV;
    ive_dst_image_s dstHV;
    ive_norm_grad_ctrl_s normGradCtrl;

    memset(&src, 0, sizeof(ive_src_image_s));
    memset(&dstH, 0, sizeof(ive_dst_image_s));
    memset(&dstV, 0, sizeof(ive_dst_image_s));
    memset(&dstHV, 0, sizeof(ive_dst_image_s));
    memset(&normGradCtrl, 0, sizeof(ive_norm_grad_ctrl_s));


    ret = bm_image_convert_to_ive_image(handle, input, &src);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    if(normgrad_attr->en_mode == BM_IVE_NORM_GRAD_OUT_HOR_AND_VER ||
       normgrad_attr->en_mode == BM_IVE_NORM_GRAD_OUT_HOR)
    {
        ret = bm_image_convert_to_ive_image(handle, output_h, &dstH);
        if(ret != BM_SUCCESS){
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to convert bm_image to ive_image %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
                return BM_ERR_FAILURE;
        }
    }

    if(normgrad_attr->en_mode == BM_IVE_NORM_GRAD_OUT_HOR_AND_VER ||
       normgrad_attr->en_mode == BM_IVE_NORM_GRAD_OUT_VER)
    {
        ret = bm_image_convert_to_ive_image(handle, output_v, &dstV);
        if(ret != BM_SUCCESS){
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to convert bm_image to ive_image %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
                return BM_ERR_FAILURE;
        }
    }

    if(normgrad_attr->en_mode == BM_IVE_NORM_GRAD_OUT_COMBINE){
        ret = bm_image_convert_to_ive_image(handle, output_hv, &dstHV);
        if(ret != BM_SUCCESS){
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to convert bm_image to ive_image %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
                return BM_ERR_FAILURE;
        }
    }

    memcpy(&normGradCtrl, normgrad_attr, sizeof(ive_norm_grad_ctrl_s));

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // instant: true is interrupt mode, false is polling mode
    ret = BM_IVE_NormGrad(ive_handle, &src, &dstH, &dstV, &dstHV, &normGradCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_normgrad error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);
    return ret;
}

bm_status_t BM_IVE_GMM(ive_handle ive_handle, ive_src_image_s *psrc,
            ive_dst_image_s *pstFg, ive_dst_image_s *pstBg,
            ive_mem_info_s *pstModel, ive_gmm_ctrl_s *pctrl,
            bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_gmm_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;
    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    ive_arg.fg = *pstFg;
    ive_arg.bg = *pstBg;
    ive_arg.model = *pstModel;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_GMM, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_gmm(
    bm_handle_t            handle,
    bm_image *             input,
    bm_image *             output_fg,
    bm_image *             output_bg,
    bm_device_mem_t *      output_model,
    bmcv_ive_gmm_ctrl *    gmm_attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src;
    ive_dst_image_s dstFg;
    ive_dst_image_s dstBg;
    ive_mem_info_s  dstModel;
    ive_gmm_ctrl_s gmmCtrl;

    memset(&src, 0, sizeof(ive_src_image_s));
    memset(&dstFg, 0, sizeof(ive_dst_image_s));
    memset(&dstBg, 0, sizeof(ive_dst_image_s));
    memset(&dstModel, 0, sizeof(ive_mem_info_s));
    memset(&gmmCtrl, 0, sizeof(ive_gmm_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, input, &src);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, output_fg, &dstFg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, output_bg, &dstBg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    unsigned long long vir_addr = 0;
    ret = bm_mem_mmap_device_mem_no_cache(handle, output_model, &vir_addr);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to mmap device memory %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    dstModel.vir_addr = vir_addr;
    dstModel.phy_addr = bm_mem_get_device_addr(*output_model);
    dstModel.size = output_model->size;

    memcpy(&gmmCtrl, gmm_attr, sizeof(ive_gmm_ctrl_s));

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // instant: true is interrupt mode, false is polling mode
    ret = BM_IVE_GMM(ive_handle, &src, &dstFg, &dstBg, &dstModel, &gmmCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_gmm error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        bm_mem_unmap_device_mem(handle, (void *)vir_addr, output_model->size);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);
    bm_mem_unmap_device_mem(handle, (void *)vir_addr, output_model->size);

    return ret;
}

bm_status_t BM_IVE_GMM2(ive_handle ive_handle, ive_src_image_s *psrc,
             ive_src_image_s *pstFactor, ive_dst_image_s *pstFg,
             ive_dst_image_s *pstBg, ive_dst_image_s *pstMatchModelInfo,
             ive_mem_info_s *pstModel, ive_gmm2_ctrl_s *pctrl,
             bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_gmm2_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;
    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    ive_arg.factor = *pstFactor;
    ive_arg.fg = *pstFg;
    ive_arg.bg = *pstBg;
    ive_arg.stInfo = *pstMatchModelInfo;
    ive_arg.model = *pstModel;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_GMM2, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_gmm2(
    bm_handle_t            handle,
    bm_image *             input,
    bm_image *             input_factor,
    bm_image *             output_fg,
    bm_image *             output_bg,
    bm_image *             output_match_model_info,
    bm_device_mem_t *      output_model,
    bmcv_ive_gmm2_ctrl *   gmm2_attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src;
    ive_src_image_s srcFactor;
    ive_dst_image_s dstFg;
    ive_dst_image_s dstBg;
    ive_dst_image_s dstMatchModelInfo;
    ive_mem_info_s  dstModel;
    ive_gmm2_ctrl_s gmm2Ctrl;

    memset(&src, 0, sizeof(ive_src_image_s));
    memset(&srcFactor, 0, sizeof(ive_src_image_s));
    memset(&dstFg, 0, sizeof(ive_dst_image_s));
    memset(&dstBg, 0, sizeof(ive_dst_image_s));
    memset(&dstMatchModelInfo, 0, sizeof(ive_dst_image_s));
    memset(&dstModel, 0, sizeof(ive_mem_info_s));
    memset(&gmm2Ctrl, 0, sizeof(ive_gmm2_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, input, &src);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, input_factor, &srcFactor);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, output_fg, &dstFg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, output_bg, &dstBg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, output_match_model_info, &dstMatchModelInfo);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    unsigned long long vir_addr = 0;
    ret = bm_mem_mmap_device_mem_no_cache(handle, output_model, &vir_addr);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to mmap device memory %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    dstModel.vir_addr = vir_addr;
    dstModel.phy_addr = bm_mem_get_device_addr(*output_model);
    dstModel.size = output_model->size;

    memcpy(&gmm2Ctrl, gmm2_attr, sizeof(ive_gmm2_ctrl_s));

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // instant: true is interrupt mode, false is polling mode
    ret = BM_IVE_GMM2(ive_handle, &src, &srcFactor, &dstFg, &dstBg,
                           &dstMatchModelInfo, &dstModel, &gmm2Ctrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_gmm2 error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        bm_mem_unmap_device_mem(handle, (void *)vir_addr, output_model->size);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);
    bm_mem_unmap_device_mem(handle, (void *)vir_addr, output_model->size);

    return ret;
}

bm_status_t BM_IVE_CanngHysEdge(ive_handle ive_handle, ive_src_image_s *psrc,
             ive_dst_image_s *pstEdge, ive_dst_mem_info_s *pstStack,
             ive_canny_hys_edge_ctrl_s *pctrl, bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_canny_hys_edge_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;
    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    ive_arg.dst = *pstEdge;
    ive_arg.stack = *pstStack;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_CANNYHYSEDGE, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_canny_hsy_edge(
    bm_handle_t                    handle,
    bm_image *                     input,
    bm_image *                     output_edge,
    bm_device_mem_t *              output_stack,
    bmcv_ive_canny_hys_edge_ctrl * canny_hys_edge_attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src;
    ive_dst_image_s dstEdge;
    ive_dst_mem_info_s dstStack;
    ive_canny_hys_edge_ctrl_s cannyHysEdgeCtrl;

    memset(&src, 0, sizeof(ive_src_image_s));
    memset(&dstEdge, 0, sizeof(ive_dst_image_s));
    memset(&dstStack, 0, sizeof(ive_dst_mem_info_s));
    memset(&cannyHysEdgeCtrl, 0, sizeof(ive_canny_hys_edge_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, input, &src);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, output_edge, &dstEdge);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    unsigned long long vir_addr = 0;
    ret = bm_mem_mmap_device_mem_no_cache(handle, output_stack, &vir_addr);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to mmap device memory %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    dstStack.vir_addr = vir_addr;
    dstStack.phy_addr = bm_mem_get_device_addr(*output_stack);
    dstStack.size = output_stack->size;

    cannyHysEdgeCtrl.low_thr = canny_hys_edge_attr->u16_low_thr;
    cannyHysEdgeCtrl.high_thr = canny_hys_edge_attr->u16_high_thr;
    memcpy(cannyHysEdgeCtrl.mask, canny_hys_edge_attr->as8_mask, 5 * 5 * sizeof(signed char));

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // instant: true is interrupt mode, false is polling mode
    ret = BM_IVE_CanngHysEdge(ive_handle, &src, &dstEdge, &dstStack, &cannyHysEdgeCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_cannyHysEdge error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        bm_mem_unmap_device_mem(handle, (void *)vir_addr, output_stack->size);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);
    bm_mem_unmap_device_mem(handle, (void *)vir_addr, output_stack->size);
    return ret;
}

bm_status_t BM_IVE_Filter(ive_handle ive_handle, ive_src_image_s *psrc,
               ive_dst_image_s *pdst, ive_filter_ctrl_s *pctrl,
               bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_filter_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;
    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    ive_arg.dst = *pdst;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_FILTER, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_filter(
    bm_handle_t                  handle,
    bm_image                     input,
    bm_image                     output,
    bmcv_ive_filter_ctrl         filter_attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src;
    ive_dst_image_s dst;
    ive_filter_ctrl_s filterAttr;

    memset(&src, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_image_s));
    memset(&filterAttr, 0, sizeof(ive_filter_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, &input, &src);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &output, &dst);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    memcpy(&filterAttr, &filter_attr, sizeof(ive_filter_ctrl_s));

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // instant: true is interrupt mode, false is polling mode
    ret = BM_IVE_Filter(ive_handle, &src, &dst, &filterAttr, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_filter error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);

    return ret;
}

bm_status_t BM_IVE_CSC(ive_handle ive_handle, ive_src_image_s *psrc,
            ive_dst_image_s *pdst, ive_csc_ctrl_s *pctrl,
            bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_csc_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;
    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    ive_arg.dst = *pdst;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_CSC, &ioctl_arg);
    return ret;
}

bm_status_t bm_output_format_check(bm_image* output)
{
    if (output->image_format == FORMAT_HSV_PLANAR) {
        return BM_SUCCESS;
    }
    return BM_ERR_FAILURE;
}

bm_status_t bm_input_format_check(bm_image* input)
{
    if (input->image_format == FORMAT_RGB_PLANAR \
        || input->image_format == FORMAT_RGB_PACKED \
        || input->image_format == FORMAT_RGBP_SEPARATE \
        || input->image_format == FORMAT_RGBYP_PLANAR) {
        return BM_SUCCESS;
    }
    return BM_ERR_FAILURE;
}

ive_csc_mode_e bm_image_format_to_ive_csc_mode(csc_type_t csc_type, bm_image* input, bm_image* output)
{
    ive_csc_mode_e mode = IVE_CSC_MODE_BUTT;

    switch (csc_type)
    {
    case CSC_YCbCr2RGB_BT601:
        if (bm_output_format_check(output) == BM_SUCCESS){
            mode = IVE_CSC_MODE_PIC_BT601_YUV2HSV;
        } else {
            mode = IVE_CSC_MODE_PIC_BT601_YUV2RGB;
        }
        break;
    case CSC_YPbPr2RGB_BT601:
        mode = IVE_CSC_MODE_VIDEO_BT601_YUV2RGB;
        break;
    case CSC_RGB2YCbCr_BT601:
        mode = IVE_CSC_MODE_PIC_BT601_RGB2YUV;
        break;
    case CSC_YCbCr2RGB_BT709:
        if (bm_output_format_check(output) != BM_SUCCESS){
            mode = IVE_CSC_MODE_PIC_BT709_YUV2RGB;
        } else {
            mode = IVE_CSC_MODE_PIC_BT709_YUV2HSV;
        }
        break;
    case CSC_RGB2YCbCr_BT709:
        mode = IVE_CSC_MODE_PIC_BT709_RGB2YUV;
        break;
    case CSC_RGB2YPbPr_BT601:
        mode = IVE_CSC_MODE_VIDEO_BT601_RGB2YUV;
        break;
    case CSC_YPbPr2RGB_BT709:
        mode = IVE_CSC_MODE_VIDEO_BT709_YUV2RGB;
        break;
    case CSC_RGB2YPbPr_BT709:
        mode = IVE_CSC_MODE_VIDEO_BT709_RGB2YUV;
        break;
    case CSC_MAX_ENUM:
        if (bm_input_format_check(input) == BM_SUCCESS){
            if (bm_output_format_check(output) == BM_SUCCESS){
                mode = IVE_CSC_MODE_PIC_RGB2HSV;
            } else if (output->image_format == FORMAT_GRAY)
            {
                mode = IVE_CSC_MODE_PIC_RGB2GRAY;
            } else {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "bm_image output image_format is error, please check it. %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
            }
        }else {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "bm_image input image_format with csc_type, please check it. %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        }
        break;
    default:
        break;
    }
    return mode;
}

bm_status_t bm_ive_csc(
    bm_handle_t     handle,
    bm_image *      input,
    bm_image *      output,
    csc_type_t      csc_type)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src;
    ive_dst_image_s dst;
    ive_csc_ctrl_s cscCtrl;

    memset(&src, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_image_s));
    memset(&cscCtrl, 0, sizeof(ive_csc_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, input, &src);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, output, &dst);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    int csc_mode = bm_image_format_to_ive_csc_mode(csc_type, input, output);
    if (csc_mode < 0 || csc_mode >= IVE_CSC_MODE_BUTT) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            " csc_type is error, please check it. %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
    }
    cscCtrl.mode = (ive_csc_mode_e)csc_mode;

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // instant: true is interrupt mode, false is polling mode
    ret = BM_IVE_CSC(ive_handle, &src, &dst, &cscCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_csc error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);

    return ret;
}

bm_status_t BM_IVE_Resize(ive_handle ive_handle, ive_src_image_s *asrc,
               ive_dst_image_s *adst, ive_resize_ctrl_s *pctrl,
               bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_resize_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }
    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *asrc;
    ive_arg.dst = *adst;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_RESIZE, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_resize(
    bm_handle_t               handle,
    bm_image *                input,
    bm_image *                output,
    bmcv_resize_algorithm     resize_mode)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src;
    ive_dst_image_s dst;
    ive_resize_ctrl_s ctrl;

    memset(&src, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_image_s));
    memset(&ctrl, 0, sizeof(ive_resize_ctrl_s));

    // transfer bm struct to ive struct
    ret = bm_image_convert_to_ive_image(handle, input, &src);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
        "failed to convert bm_image to ive_image %s: %s: %d\n",
        filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, output, &dst);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
        "failed to convert bm_image to ive_image %s: %s: %d\n",
        filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // set ive resize params
    if(resize_mode == BMCV_INTER_LINEAR){
        ctrl.mode = resize_mode - 1;
    } else if (resize_mode == BMCV_INTER_AREA)
    {
        ctrl.mode = resize_mode - 2;
    } else {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
        "please check, ive no support this resize mode. %s: %s: %d\n",
        filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // instant: true is interrupt mode, false is polling mode
    ret = BM_IVE_Resize(ive_handle, &src, &dst, &ctrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_resize error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }
    BM_IVE_DestroyHandle(ive_handle);

    return ret;
}

bm_status_t BM_IVE_STCandiCorner(ive_handle ive_handle, ive_src_image_s *psrc,
                  ive_dst_image_s *pdst, ive_st_candi_corner_ctrl_s *pctrl,
                  bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_stcandicorner ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    ive_arg.dst = *pdst;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_ST_CANDI_CORNER, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_stCandiCorner(
    bm_handle_t                   handle,
    bm_image *                    input,
    bm_image *                    output,
    bmcv_ive_stcandicorner_attr * stcandicorner_attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src;
    ive_dst_image_s dst;
    ive_st_candi_corner_ctrl_s stCandiCornerAttr;

    memset(&src, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_image_s));
    memset(&stCandiCornerAttr, 0, sizeof(ive_st_candi_corner_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, input, &src);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, output, &dst);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    unsigned long long vir_addr = 0;
    ret = bm_mem_mmap_device_mem_no_cache(handle, &stcandicorner_attr->st_mem, &vir_addr);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to mmap device memory %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    stCandiCornerAttr.mem.vir_addr = vir_addr;
    stCandiCornerAttr.mem.phy_addr = bm_mem_get_device_addr(stcandicorner_attr->st_mem);
    stCandiCornerAttr.mem.size = stcandicorner_attr->st_mem.size;
    stCandiCornerAttr.quality_level = stcandicorner_attr->u0q8_quality_level;

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // instant: true is interrupt mode, false is polling mode
    ret = BM_IVE_STCandiCorner(ive_handle, &src, &dst, &stCandiCornerAttr, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "BM_IVE_STCandiCorner error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        bm_mem_unmap_device_mem(handle, (void *)vir_addr, stcandicorner_attr->st_mem.size);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);
    bm_mem_unmap_device_mem(handle, (void *)vir_addr, stcandicorner_attr->st_mem.size);
    return ret;
}

bm_status_t BM_IVE_GradFg(ive_handle ive_handle, ive_src_image_s *pstBgDiffFg,
               ive_src_image_s *pstCurGrad, ive_src_image_s *pstBgGrad,
               ive_dst_image_s *pstGradFg, ive_grad_fg_ctrl_s *pctrl,
               bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_grad_fg_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    if(pstBgDiffFg != NULL) ive_arg.bg_diff_fg = *pstBgDiffFg;
    if(pstCurGrad != NULL) ive_arg.cur_grad = *pstCurGrad;
    ive_arg.bg_grad = *pstBgGrad;
    ive_arg.grad_fg = *pstGradFg;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_GRADFG, &ioctl_arg);

    return ret;
}

bm_status_t bm_ive_gradFg(
    bm_handle_t             handle,
    bm_image *              input_bgDiffFg,
    bm_image *              input_fgGrad,
    bm_image *              input_bgGrad,
    bm_image *              output_gradFg,
    bmcv_ive_gradfg_attr *  gradfg_attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s srcBgDiffFg;
    ive_src_image_s srcCurGrad;
    ive_src_image_s srcBgGrad;
    ive_dst_image_s dstGradFg;
    ive_grad_fg_ctrl_s gradFgAttr;

    memset(&srcBgDiffFg, 0, sizeof(ive_src_image_s));
    memset(&srcCurGrad, 0, sizeof(ive_src_image_s));
    memset(&srcBgGrad, 0, sizeof(ive_src_image_s));
    memset(&dstGradFg, 0, sizeof(ive_dst_image_s));
    memset(&gradFgAttr, 0, sizeof(ive_grad_fg_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, input_bgDiffFg, &srcBgDiffFg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, input_fgGrad, &srcCurGrad);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, input_bgGrad, &srcBgGrad);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, output_gradFg, &dstGradFg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    memcpy(&gradFgAttr, gradfg_attr, sizeof(ive_grad_fg_ctrl_s));

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // instant: true is interrupt mode, false is polling mode
    ret = BM_IVE_GradFg(ive_handle, &srcBgDiffFg, &srcCurGrad, &srcBgGrad, &dstGradFg, &gradFgAttr, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "BM_IVE_GradFg error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);
    return ret;
}

bm_status_t BM_IVE_SAD(ive_handle ive_handle, ive_src_image_s *psrc1,
            ive_src_image_s *psrc2, ive_dst_image_s *pstSad,
            ive_dst_image_s *pstThr, ive_sad_ctrl_s *pctrl,
            bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_sad_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src1 = *psrc1;
    ive_arg.src2 = *psrc2;
    ive_arg.sad = *pstSad;
    ive_arg.thr = *pstThr;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_SAD, &ioctl_arg);
    return ret;
}

int bm_image_convert_to_sad_enOutCtrl(bmcv_ive_sad_attr * sad_attr, bm_image * output_sad)
{
    if (sad_attr->en_out_ctrl == BM_IVE_SAD_OUT_BOTH) {
        if (output_sad->data_type == DATA_TYPE_EXT_U16){return IVE_SAD_OUT_CTRL_16BIT_BOTH;}
        else if (output_sad->data_type == DATA_TYPE_EXT_1N_BYTE){return IVE_SAD_OUT_CTRL_8BIT_BOTH;}
        else {bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "output_sad.datatype is invailed,please check it. %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return -1;}
    }
    else if (sad_attr->en_out_ctrl == BM_IVE_SAD_OUT_SAD) {
        if (output_sad->data_type == DATA_TYPE_EXT_U16){return IVE_SAD_OUT_CTRL_16BIT_BOTH;}
        else if (output_sad->data_type == DATA_TYPE_EXT_1N_BYTE){return IVE_SAD_OUT_CTRL_8BIT_BOTH;}
        else {bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "output_sad.datatype is invailed,please check it. %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return -1;}
    }
    else if (sad_attr->en_out_ctrl == BM_IVE_SAD_OUT_THRESH) {
        return BM_IVE_SAD_OUT_THRESH;
    }
    else {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "sad_attr->enOutCtrl is invailed,please check it. %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return -1;
    }
}

bm_status_t bm_ive_sad(
    bm_handle_t                handle,
    bm_image *                 input,
    bm_image *                 output_sad,
    bm_image *                 output_thr,
    bmcv_ive_sad_attr *        sad_attr,
    bmcv_ive_sad_thresh_attr*  thresh_attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src1;
    ive_src_image_s src2;
    ive_dst_image_s dst_sad;
    ive_dst_image_s dst_thr;
    ive_sad_ctrl_s sadAttr;

    memset(&src1, 0, sizeof(ive_src_image_s));
    memset(&src2, 0, sizeof(ive_src_image_s));
    memset(&dst_sad, 0, sizeof(ive_dst_image_s));
    memset(&dst_thr, 0, sizeof(ive_dst_image_s));
    memset(&sadAttr, 0, sizeof(ive_sad_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, &input[0], &src1);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &input[1], &src2);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, output_sad, &dst_sad);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, output_thr, &dst_thr);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    sadAttr.mode = sad_attr->en_mode;
    sadAttr.out_ctrl = (ive_sad_out_ctrl_e)bm_image_convert_to_sad_enOutCtrl(sad_attr, output_sad);
    if(thresh_attr == NULL) {
        sadAttr.thr = 0;
        sadAttr.max_val = 0;
        sadAttr.min_val = 0;
    } else {
        sadAttr.thr = thresh_attr->u16_thr;
        sadAttr.max_val = thresh_attr->u8_max_val;
        sadAttr.min_val = thresh_attr->u8_min_val;
    }
    // memcpy(&sadAttr, sad_attr, sizeof(IVE_SAD_CTRL_S));

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // instant: true is interrupt mode, false is polling mode
    ret = BM_IVE_SAD(ive_handle, &src1, &src2, &dst_sad, &dst_thr, &sadAttr, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_sad error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);
    return ret;
}

bm_status_t BM_IVE_MatchBgModel(ive_handle ive_handle, ive_src_image_s *pcur_img,
                 ive_data_s *pbg_model, ive_image_s *pfg_flag,
                 ive_dst_image_s *pstDiffFg, ive_dst_mem_info_s *pstat_data,
                 ive_match_bg_model_ctrl_s *pctrl, bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_match_bgmodel_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.cur_img = *pcur_img;
    ive_arg.bg_model = *pbg_model;
    ive_arg.fg_flag = *pfg_flag;
    ive_arg.stDiffFg = *pstDiffFg;
    ive_arg.stat_data = *pstat_data;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    ioctl_arg.buffer = (void *)(uintptr_t)pstat_data->vir_addr;
    ioctl_arg.size = sizeof(ive_bg_stat_data_s);
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_MATCH_BGMODEM, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_match_bgmodel(
        bm_handle_t                   handle,
        bm_image *                    cur_img,
        bm_image *                    bgmodel_img,
        bm_image *                    fgflag_img,
        bm_image *                    diff_fg_img,
        bm_device_mem_t *             stat_data_mem,
        bmcv_ive_match_bgmodel_attr * attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s  pcur_img;
    ive_data_s pbg_model;
    ive_image_s pfg_flag;
    ive_dst_image_s pstDiffFg;
    ive_dst_mem_info_s pstat_data;
    ive_match_bg_model_ctrl_s  pctrl;

    memset(&pcur_img, 0, sizeof(ive_src_image_s));
    memset(&pbg_model, 0, sizeof(ive_data_s));
    memset(&pfg_flag, 0, sizeof(ive_image_s));
    memset(&pstDiffFg, 0, sizeof(ive_dst_image_s));
    memset(&pstat_data, 0, sizeof(ive_dst_mem_info_s));
    memset(&pctrl, 0, sizeof(ive_match_bg_model_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, cur_img, &pcur_img);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_data(handle, 0, bgmodel_img, &pbg_model);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_data %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, fgflag_img, &pfg_flag);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, diff_fg_img, &pstDiffFg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

   unsigned long long vir_addr = 0;
   ret = bm_mem_mmap_device_mem_no_cache(handle, stat_data_mem, &vir_addr);
   if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to mmap device memory %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    pstat_data.vir_addr = vir_addr;
    pstat_data.phy_addr = bm_mem_get_device_addr(*stat_data_mem);
    pstat_data.size = stat_data_mem->size;

    memcpy(&pctrl, attr, sizeof(ive_match_bg_model_ctrl_s));

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_MatchBgModel(ive_handle, &pcur_img, &pbg_model,
                 &pfg_flag, &pstDiffFg, &pstat_data, &pctrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_match_bgmodel error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        bm_mem_unmap_device_mem(handle, (void *)vir_addr, stat_data_mem->size);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }

    bm_mem_unmap_device_mem(handle, (void *)vir_addr, stat_data_mem->size);
    BM_IVE_DestroyHandle(ive_handle);
    return ret;
}

bm_status_t BM_IVE_UpdateBgModel(ive_handle ive_handle, ive_src_image_s *pcur_img, ive_data_s *pbg_model,
            ive_image_s *pfg_flag, ive_dst_image_s *pbg_img,
            ive_dst_image_s *pchg_sta, ive_dst_mem_info_s *pstat_data,
            ive_update_bg_model_ctrl_s *pctrl, bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_update_bgmodel_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }
    ive_arg.ive_handle = ive_handle;
    ive_arg.cur_img = *pcur_img;
    ive_arg.bg_model = *pbg_model;
    ive_arg.fg_flag = *pfg_flag;
    ive_arg.bg_img = *pbg_img;
    ive_arg.chg_sta = *pchg_sta;
    ive_arg.stat_data = *pstat_data;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    ioctl_arg.buffer = (void *)(uintptr_t)pstat_data->vir_addr;
    ioctl_arg.size = sizeof(ive_bg_stat_data_s);
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, IVE_IOC_UPDATE_BGMODEL, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_update_bgmodel(
    bm_handle_t                    handle,
    bm_image *                     cur_img,
    bm_image *                     bgmodel_img,
    bm_image *                     fgflag_img,
    bm_image *                     bg_img,
    bm_image *                     chgsta_img,
    bm_device_mem_t  *             stat_data_mem,
    bmcv_ive_update_bgmodel_attr * attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s  pcur_img;
    ive_data_s pbg_model;
    ive_image_s pfg_flag;
    ive_dst_image_s pbg_img;
    ive_dst_image_s pchg_sta;
    ive_dst_mem_info_s pstat_data;
    ive_update_bg_model_ctrl_s pctrl;

    memset(&pcur_img, 0, sizeof(ive_src_image_s));
    memset(&pbg_model, 0, sizeof(ive_data_s));
    memset(&pfg_flag, 0, sizeof(ive_image_s));
    memset(&pbg_img, 0, sizeof(ive_dst_image_s));
    memset(&pchg_sta, 0, sizeof(ive_dst_mem_info_s));
    memset(&pctrl, 0, sizeof(ive_update_bg_model_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, cur_img, &pcur_img);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_data %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_data(handle, 0, bgmodel_img, &pbg_model);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_data %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, fgflag_img, &pfg_flag);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, bg_img, &pbg_img);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, chgsta_img, &pchg_sta);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    unsigned long long vir_addr = 0;
    ret = bm_mem_mmap_device_mem_no_cache(handle, stat_data_mem, &vir_addr);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to mmap device memory %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    pstat_data.vir_addr = vir_addr;
    pstat_data.phy_addr = bm_mem_get_device_addr(*stat_data_mem);
    pstat_data.size = stat_data_mem->size;

    memcpy(&pctrl, attr, sizeof(ive_update_bg_model_ctrl_s));

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_UpdateBgModel(ive_handle, &pcur_img, &pbg_model, &pfg_flag,
                       &pbg_img, &pchg_sta, &pstat_data, &pctrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_update_bgmodel error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        bm_mem_unmap_device_mem(handle, (void *)vir_addr, stat_data_mem->size);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }

    bm_mem_unmap_device_mem(handle, (void *)vir_addr, stat_data_mem->size);
    BM_IVE_DestroyHandle(ive_handle);
    return ret;
}

bm_status_t BM_IVE_CCL(ive_handle ive_handle, ive_image_s *psrc_dst,
        ive_dst_mem_info_s *pblob, ive_ccl_ctrl_s *ccl_ctrl,
        unsigned char instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_ccl_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src_dst = *psrc_dst;
    ive_arg.blob = *pblob;
    ive_arg.ccl_ctrl = *ccl_ctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    ioctl_arg.buffer = (void *)(uintptr_t)pblob->vir_addr;
    ioctl_arg.size = sizeof(unsigned short) + sizeof(signed char) + sizeof(unsigned char);
    bm_status_t ret = (bm_status_t) ioctl(p->dev_fd, IVE_IOC_CCL, &ioctl_arg);

    return ret;
}

bm_status_t bm_ive_ccl(
    bm_handle_t          handle,
    bm_image *           src_dst_image,
    bm_device_mem_t *    ccblob_output,
    bmcv_ive_ccl_attr *  ccl_attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_image_s psrc_dst;
    ive_dst_mem_info_s pblob;
    ive_ccl_ctrl_s ccl_ctrl;

    memset(&psrc_dst, 0, sizeof(ive_image_s));
    memset(&pblob, 0, sizeof(ive_dst_mem_info_s));
    memset(&ccl_ctrl, 0, sizeof(ive_ccl_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, src_dst_image, &psrc_dst);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    unsigned long long vir_addr = 0;
    ret = bm_mem_mmap_device_mem_no_cache(handle, ccblob_output, &vir_addr);
    if(ret !=  BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to mmap device memory %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    pblob.vir_addr = vir_addr;
    pblob.phy_addr = bm_mem_get_device_addr(*ccblob_output);
    pblob.size = ccblob_output->size;

    memcpy(&ccl_ctrl, ccl_attr, sizeof(ive_ccl_ctrl_s));

     // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_CCL(ive_handle, &psrc_dst, &pblob, &ccl_ctrl, true);
    if(ret != BM_SUCCESS){
         bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "BM_IVE_CCL error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        bm_mem_unmap_device_mem(handle, (void *)vir_addr, ccblob_output->size);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }

    bm_mem_unmap_device_mem(handle, (void *)vir_addr, ccblob_output->size);
    BM_IVE_DestroyHandle(ive_handle);
    return ret;
}

bm_status_t BM_IVE_BERNSEN(ive_handle ive_handle, ive_src_image_s *psrc,
                ive_dst_image_s *pdst, ive_bernsen_ctrl_s *pctrl,
                unsigned char instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_bernsen_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    ive_arg.dst = *pdst;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t) ioctl(p->dev_fd, IVE_IOC_BERNSEN, &ioctl_arg);

    return ret;
}

bm_status_t bm_ive_bernsen(
    bm_handle_t           handle,
    bm_image              input,
    bm_image              output,
    bmcv_ive_bernsen_attr attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src;
    ive_dst_image_s dst;
    ive_bernsen_ctrl_s bernsenAttr;

    memset(&src, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_image_s));
    memset(&bernsenAttr, 0, sizeof(ive_bernsen_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, &input, &src);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &output, &dst);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    memcpy(&bernsenAttr, &attr, sizeof(ive_bernsen_ctrl_s));

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_BERNSEN(ive_handle, &src, &dst, &bernsenAttr, true);
    if(ret != BM_SUCCESS){
         bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "BM_IVE_BERNSEN error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);

    return ret;
}

bm_status_t BM_IVE_FilterAndCsc(ive_handle ive_handle, ive_src_image_s *psrc,
                    ive_dst_image_s *pdst,
                    ive_filter_and_csc_ctrl_s *pctrl,
                    unsigned char instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_filter_and_csc_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    ive_arg.dst = *pdst;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t) ioctl(p->dev_fd, IVE_IOC_FILTER_AND_CSC, &ioctl_arg);

    return ret;
}

int bm_image_format_filterandcsc_mode(csc_type_t csc_type)
{
    switch (csc_type)
    {
    case CSC_YCbCr2RGB_BT601:
        return IVE_CSC_MODE_PIC_BT601_YUV2RGB;
        break;
    case CSC_YPbPr2RGB_BT601:
        return IVE_CSC_MODE_VIDEO_BT601_YUV2RGB;
        break;
    case CSC_YPbPr2RGB_BT709:
        return IVE_CSC_MODE_VIDEO_BT709_YUV2RGB;
        break;
    case CSC_YCbCr2RGB_BT709:
        return IVE_CSC_MODE_PIC_BT709_YUV2RGB;
        break;
    default:
        break;
    }
    return IVE_CSC_MODE_BUTT;
}

bm_status_t bm_ive_filterAndCsc(
    bm_handle_t             handle,
    bm_image                input,
    bm_image                output,
    bmcv_ive_filter_ctrl    attr,
    csc_type_t              csc_type)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src;
    ive_dst_image_s dst;
    ive_filter_and_csc_ctrl_s ctrl;

    memset(&src, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_image_s));
    memset(&ctrl, 0, sizeof(ive_filter_and_csc_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, &input, &src);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &output, &dst);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ctrl.norm = attr.u8_norm;
    memcpy(ctrl.mask, attr.as8_mask, 5 * 5 * sizeof(signed char));
    int csc_mode = bm_image_format_filterandcsc_mode(csc_type);
    if (csc_mode < 0 || csc_mode >= IVE_CSC_MODE_BUTT) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            " csc_type is error, please check it. %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
    }
    ctrl.mode = (ive_csc_mode_e)csc_mode;

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_FilterAndCsc(ive_handle, &src, &dst, &ctrl, true);
    if(ret != BM_SUCCESS){
         bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "BM_IVE_FilterAndCsc error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);

    return ret;
}

bm_status_t BM_IVE_16BitTo8Bit(ive_handle ive_handle, ive_src_image_s *psrc,
                ive_dst_image_s *pdst,
                ive_16bit_to_8bit_ctrl_s *pctrl,
                bool instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_16bit_to_8bit_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src = *psrc;
    ive_arg.dst = *pdst;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;
    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t) ioctl(p->dev_fd, IVE_IOC_16BIT_TO_8BIT, &ioctl_arg);

    return ret;
}

bm_status_t bm_ive_16bit_to_8bit(
    bm_handle_t                 handle,
    bm_image                    input,
    bm_image                    output,
    bmcv_ive_16bit_to_8bit_attr attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src;
    ive_dst_image_s dst;
    ive_16bit_to_8bit_ctrl_s ctrl;

    memset(&src, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_image_s));
    memset(&ctrl, 0, sizeof(ive_16bit_to_8bit_ctrl_s));

    ret = bm_image_convert_to_ive_image(handle, &input, &src);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &output, &dst);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    memcpy(&ctrl, &attr, sizeof(ive_16bit_to_8bit_ctrl_s));

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_16BitTo8Bit(ive_handle, &src, &dst, &ctrl, true);
    if(ret != BM_SUCCESS){
         bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "BM_IVE_16BitTo8Bit error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);

    return ret;
}

bm_status_t BM_IVE_FrameDiffMotion(ive_handle ive_handle, ive_src_image_s *psrc1,
            ive_src_image_s *psrc2, ive_dst_image_s *pdst,
            ive_frame_diff_motion_ctrl_s *pctrl,
            unsigned char instant)
{
    struct ive_ioctl_arg ioctl_arg;
    struct ive_ioctl_md ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)ive_handle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.ive_handle = ive_handle;
    ive_arg.src1 = *psrc1;
    ive_arg.src2 = *psrc2;
    ive_arg.dst = *pdst;
    ive_arg.ctrl = *pctrl;
    ive_arg.instant = instant;

    ioctl_arg.input_data = (uint64_t)&ive_arg;
    bm_status_t ret = (bm_status_t) ioctl(p->dev_fd, IVE_IOC_MD, &ioctl_arg);

    return ret;
}

bm_status_t bm_ive_frame_diff_motion(
    bm_handle_t                     handle,
    bm_image                        input1,
    bm_image                        input2,
    bm_image                        output,
    bmcv_ive_frame_diff_motion_attr attr)
{
    bm_status_t ret = BM_SUCCESS;
    ive_handle ive_handle = NULL;
    ive_src_image_s src1, src2;
    ive_dst_image_s dst;
    ive_frame_diff_motion_ctrl_s ctrl;

    memset(&src1, 0, sizeof(ive_src_image_s));
    memset(&src2, 0, sizeof(ive_src_image_s));
    memset(&dst, 0, sizeof(ive_dst_image_s));
    memset(&ctrl, 0, sizeof(ive_frame_diff_motion_ctrl_s));


    ret = bm_image_convert_to_ive_image(handle, &input1, &src1);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &input2, &src2);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &output, &dst);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    // config setting
    ctrl.sub_mode = attr.sub_mode;
    ctrl.thr_mode = (ive_thresh_mode_e)thransform_pattern(attr.thr_mode);
    ctrl.thr_low = attr.u8_thr_low;
    ctrl.thr_high = attr.u8_thr_high;
    ctrl.thr_min_val = attr.u8_thr_min_val;
    ctrl.thr_mid_val = attr.u8_thr_mid_val;
    ctrl.thr_max_val = attr.u8_thr_max_val;

    memcpy(&ctrl.erode_mask, &attr.au8_erode_mask, 25 * sizeof(unsigned char));
    memcpy(&ctrl.dilate_mask, &attr.au8_dilate_mask, 25 * sizeof(unsigned char));

    // get ive handle and invoke ive funtion
    ive_handle = BM_IVE_CreateHandle();
    if (ive_handle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_FrameDiffMotion(ive_handle, &src1, &src2, &dst, &ctrl, true);
    if(ret != BM_SUCCESS){
         bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "BM_IVE_FrameDiffMotion error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(ive_handle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(ive_handle);

    return ret;
}