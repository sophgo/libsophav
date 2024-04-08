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

IVE_HANDLE BM_IVE_CreateHandle()
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

void BM_IVE_DestroyHandle(IVE_HANDLE pIveHandle)
{
    // pIveHandle is not used here
    close_ive_dev();
    return;
}

IVE_IMAGE_TYPE_E bm_image_type_convert_to_ive_image_type(bm_image_format_ext image_format, bm_image_data_format_ext data_type)
{
    IVE_IMAGE_TYPE_E enType = IVE_IMAGE_TYPE_BUTT;

    switch (image_format) {
        case FORMAT_GRAY:
            if (data_type == DATA_TYPE_EXT_1N_BYTE) {
                enType = IVE_IMAGE_TYPE_U8C1;
            } else if (data_type == DATA_TYPE_EXT_1N_BYTE_SIGNED) {
                enType = IVE_IMAGE_TYPE_S8C1;
            } else if (data_type == DATA_TYPE_EXT_U16) {
                enType = IVE_IMAGE_TYPE_U16C1;
            } else if (data_type == DATA_TYPE_EXT_S16) {
                enType = IVE_IMAGE_TYPE_S16C1;
            } else if (data_type == DATA_TYPE_EXT_U32){
                enType = IVE_IMAGE_TYPE_U32C1;
            } else {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_WARNING,
                        "unsupported image format for ive, image_format = %d, data_type = %d, %s: %s: %d\n",
                        image_format, data_type, filename(__FILE__), __func__, __LINE__);
                return IVE_IMAGE_TYPE_BUTT;
            }
            break;
        case FORMAT_YUV420P:
            enType = IVE_IMAGE_TYPE_YUV420P;
            break;
        case FORMAT_NV21:
            enType = IVE_IMAGE_TYPE_YUV420SP;
            break;
        case FORMAT_RGBP_SEPARATE:
        case FORMAT_BGRP_SEPARATE:
        case FORMAT_HSV_PLANAR:
        case FORMAT_RGB_PLANAR:
        case FORMAT_BGR_PLANAR:
        case FORMAT_YUV444P:
            enType = IVE_IMAGE_TYPE_U8C3_PLANAR;
            break;
        case FORMAT_NV61:
            enType = IVE_IMAGE_TYPE_YUV422SP;
            break;
        case FORMAT_RGB_PACKED:
        case FORMAT_BGR_PACKED:
        case FORMAT_YUV444_PACKED:
        case FORMAT_YVU444_PACKED:
            enType = IVE_IMAGE_TYPE_U8C3_PACKAGE;
            break;
        default:
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_WARNING,
                    "unsupported image format for ive, image_format = %d, data_type = %d, %s: %s: %d\n",
                    image_format, data_type, filename(__FILE__), __func__, __LINE__);
            return IVE_IMAGE_TYPE_BUTT;
    }

    return enType;
}

bm_status_t bm_image_convert_to_ive_image(bm_handle_t handle, bm_image *bm_img, IVE_IMAGE_S *ive_img)
{
     int plane_num = 0;
    int stride[4];
    bm_device_mem_t device_mem[4];
    bm_image_format_ext image_format;
    bm_image_data_format_ext data_type;
    IVE_IMAGE_TYPE_E enType;
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
    enType = bm_image_type_convert_to_ive_image_type(image_format, data_type);

    ive_img->enType = enType;
    ive_img->u32Width = bm_img->width;
    ive_img->u32Height = bm_img->height;

    for(i = 0; i < plane_num; i++){
        ive_img->u32Stride[i] = stride[i];
        ive_img->u64PhyAddr[i] = device_mem[i].u.device.device_addr;
    }

    if(planar_to_seperate){
        for(int i = 0; i < 3; ++i){
            ive_img->u32Stride[i] = stride[0];
            ive_img->u64PhyAddr[i] = device_mem[0].u.device.device_addr + (stride[0] * bm_img->height * i);
        }
    }

    return BM_SUCCESS;
}

bm_status_t bm_image_convert_to_ive_data(bm_handle_t handle, int i, bm_image *bm_img, IVE_DATA_S *ive_data)
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

    ive_data->u32Width = bm_img->image_private->memory_layout[i].W;
    ive_data->u32Height = bm_img->image_private->memory_layout[i].H;
    ive_data->u64PhyAddr = device_mem[i].u.device.device_addr;
    ive_data->u32Stride = stride[i];

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

bm_status_t BM_IVE_Add(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
            IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
            IVE_ADD_CTRL_S *pstCtrl, bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_add_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    // check if ive device is opened
    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc1 = *pstSrc1;
    ive_arg.stSrc2 = *pstSrc2;
    ive_arg.stDst = *pstDst;
    ive_arg.pstCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    // invoke ive api in driver
    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    if (ioctl(p->dev_fd, CVI_IVE_IOC_Add, &ioctl_arg) < 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "CVI_IVE_IOC_Add failed %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t BM_IVE_And(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
            IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst, bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_and_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    // check if ive device is opened
    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc1 = *pstSrc1;
    ive_arg.stSrc2 = *pstSrc2;
    ive_arg.stDst = *pstDst;
    ive_arg.bInstant = bInstant;

    // invoke ive api in driver
    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    if (ioctl(p->dev_fd, CVI_IVE_IOC_And, &ioctl_arg) < 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "CVI_IVE_IOC_And failed %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t BM_IVE_Sub(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
            IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
            IVE_SUB_CTRL_S *pstCtrl, bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_sub_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    // check if ive device is opened
    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc1 = *pstSrc1;
    ive_arg.stSrc2 = *pstSrc2;
    ive_arg.stDst = *pstDst;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    // invoke ive api in driver
    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    if (ioctl(p->dev_fd, CVI_IVE_IOC_Sub, &ioctl_arg) < 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "CVI_IVE_IOC_Sub failed %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t BM_IVE_Or(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
            IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst, bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_or_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    // check if ive device is opened
    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc1 = *pstSrc1;
    ive_arg.stSrc2 = *pstSrc2;
    ive_arg.stDst = *pstDst;
    ive_arg.bInstant = bInstant;

    // invoke ive api in driver
    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    if (ioctl(p->dev_fd, CVI_IVE_IOC_Or, &ioctl_arg) < 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "CVI_IVE_IOC_Or failed %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t BM_IVE_Xor(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
            IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst, bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_xor_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    // check if ive device is opened
    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc1 = *pstSrc1;
    ive_arg.stSrc2 = *pstSrc2;
    ive_arg.stDst = *pstDst;
    ive_arg.bInstant = bInstant;

    // invoke ive api in driver
    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    if (ioctl(p->dev_fd, CVI_IVE_IOC_Xor, &ioctl_arg) < 0) {
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
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S stSrc1, stSrc2;
    IVE_DST_IMAGE_S stDst;
    IVE_ADD_CTRL_S stAddCtrl;

    memset(&stSrc1, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&stSrc2, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&stDst, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&stAddCtrl, 0, sizeof(IVE_ADD_CTRL_S));

    // transfer bm struct to ive struct
    ret = bm_image_convert_to_ive_image(handle, &input1, &stSrc1);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &input2, &stSrc2);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &output, &stDst);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    memcpy(&stAddCtrl, (void*)&attr, sizeof(IVE_ADD_CTRL_S));
    ret = BM_IVE_Add(pIveHandle, &stSrc1, &stSrc2, &stDst, &stAddCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "bm_ive_add error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }
    BM_IVE_DestroyHandle(pIveHandle);

    return ret;
}

bm_status_t bm_ive_and(
    bm_handle_t         handle,
    bm_image            input1,
    bm_image            input2,
    bm_image            output)
{
    bm_status_t ret = BM_SUCCESS;
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S stSrc1, stSrc2;
    IVE_DST_IMAGE_S stDst;

    memset(&stSrc1, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&stSrc2, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&stDst, 0, sizeof(IVE_DST_IMAGE_S));

    // transfer bm struct to ive struct
    ret = bm_image_convert_to_ive_image(handle, &input1, &stSrc1);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &input2, &stSrc2);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &output, &stDst);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_And(pIveHandle, &stSrc1, &stSrc2, &stDst, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "bm_ive_and error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }
    BM_IVE_DestroyHandle(pIveHandle);

    return ret;
}

bm_status_t bm_ive_or(
    bm_handle_t          handle,
    bm_image             input1,
    bm_image             input2,
    bm_image             output)
{
    bm_status_t ret = BM_SUCCESS;
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S stSrc1, stSrc2;
    IVE_DST_IMAGE_S stDst;

    memset(&stSrc1, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&stSrc2, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&stDst, 0, sizeof(IVE_DST_IMAGE_S));

    // transfer bm struct to ive struct
    ret = bm_image_convert_to_ive_image(handle, &input1, &stSrc1);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &input2, &stSrc2);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &output, &stDst);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_Or(pIveHandle, &stSrc1, &stSrc2, &stDst, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "bm_ive_and error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }
    BM_IVE_DestroyHandle(pIveHandle);

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
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S stSrc1, stSrc2;
    IVE_DST_IMAGE_S stDst;
    IVE_SUB_CTRL_S stSubCtrl;

    memset(&stSrc1, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&stSrc2, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&stDst, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&stSubCtrl, 0, sizeof(IVE_SUB_CTRL_S));

    // transfer bm struct to ive struct
    ret = bm_image_convert_to_ive_image(handle, &input1, &stSrc1);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &input2, &stSrc2);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &output, &stDst);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    memcpy(&stSubCtrl, (void*)&attr, sizeof(IVE_SUB_CTRL_S));
    ret = BM_IVE_Sub(pIveHandle, &stSrc1, &stSrc2, &stDst, &stSubCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "bm_ive_sub error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }
    BM_IVE_DestroyHandle(pIveHandle);

    return ret;
}


bm_status_t bm_ive_xor(
    bm_handle_t          handle,
    bm_image             input1,
    bm_image             input2,
    bm_image             output)
{
    bm_status_t ret = BM_SUCCESS;
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S stSrc1, stSrc2;
    IVE_DST_IMAGE_S stDst;

    memset(&stSrc1, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&stSrc2, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&stDst, 0, sizeof(IVE_DST_IMAGE_S));

    // transfer bm struct to ive struct
    ret = bm_image_convert_to_ive_image(handle, &input1, &stSrc1);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &input2, &stSrc2);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &output, &stDst);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_Xor(pIveHandle, &stSrc1, &stSrc2, &stDst, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "bm_ive_xor error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }
    BM_IVE_DestroyHandle(pIveHandle);

    return ret;
}

bm_status_t BM_IVE_Thresh(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
            IVE_DST_IMAGE_S *pstDst, IVE_THRESH_CTRL_S *pstCtrl, bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_thresh_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    // check if ive device is opened
    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    ive_arg.stDst = *pstDst;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    // invoke ive api in driver
    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    if (ioctl(p->dev_fd, CVI_IVE_IOC_Thresh, &ioctl_arg) < 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "CVI_IVE_IOC_Thresh failed %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t BM_IVE_Thresh_U16(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
            IVE_DST_IMAGE_S *pstDst, IVE_THRESH_U16_CTRL_S *pstCtrl, bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_thres_su16_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    // check if ive device is opened
    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    ive_arg.stDst = *pstDst;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    // invoke ive api in driver
    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    if (ioctl(p->dev_fd, CVI_IVE_IOC_Thresh_U16, &ioctl_arg) < 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "CVI_IVE_IOC_Thresh_U16 failed %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    return BM_SUCCESS;
}

bm_status_t BM_IVE_Thresh_S16(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
            IVE_DST_IMAGE_S *pstDst, IVE_THRESH_S16_CTRL_S *pstCtrl, bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_thresh_s16_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    // check if ive device is opened
    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    ive_arg.stDst = *pstDst;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    // invoke ive api in driver
    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    if (ioctl(p->dev_fd, CVI_IVE_IOC_Thresh_S16, &ioctl_arg) < 0) {
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
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S stSrc;
    IVE_DST_IMAGE_S stDst;
    IVE_THRESH_CTRL_S stThreshCtrl;
    IVE_THRESH_U16_CTRL_S stThreshU16Ctrl;
    IVE_THRESH_S16_CTRL_S stThreshS16Ctrl;

    memset(&stSrc, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&stDst, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&stThreshCtrl, 0, sizeof(IVE_THRESH_CTRL_S));
    memset(&stThreshU16Ctrl, 0, sizeof(IVE_THRESH_U16_CTRL_S));
    memset(&stThreshS16Ctrl, 0, sizeof(IVE_THRESH_S16_CTRL_S));

    // transfer bm struct to ive struct
    ret = bm_image_convert_to_ive_image(handle, &input, &stSrc);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &output, &stDst);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    int opType = bm_image_convert_to_thresh_opType(input, output);
    switch (opType) {
        case MOD_U8:
            stThreshCtrl.enMode = (IVE_THRESH_MODE_E)thransform_pattern(thresh_mode);
            if (stThreshCtrl.enMode < 0 || stThreshCtrl.enMode >= IVE_THRESH_MODE_BUTT) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                        "unknown thresh mode %d, %s: %s: %d\n",
                        thresh_mode, filename(__FILE__), __func__, __LINE__);
                break;
            }

            stThreshCtrl.u8LowThr = (u8)(attr.low_thr & 0xff);
            stThreshCtrl.u8HighThr = (u8)(attr.high_thr & 0xff);
            stThreshCtrl.u8MinVal = (u8)(attr.min_val & 0xff);
            stThreshCtrl.u8MidVal = (u8)(attr.mid_val & 0xff);
            stThreshCtrl.u8MaxVal = (u8)(attr.max_val & 0xff);
            ret = BM_IVE_Thresh(pIveHandle, &stSrc, &stDst, &stThreshCtrl, true);
            break;
        case MOD_U16:
            stThreshU16Ctrl.enMode = (IVE_THRESH_U16_MODE_E)thransform_pattern(thresh_mode);
            if (stThreshU16Ctrl.enMode < 0 || stThreshU16Ctrl.enMode >= IVE_THRESH_U16_MODE_BUTT) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                        "unknown thresh mode %d, %s: %s: %d\n",
                        thresh_mode, filename(__FILE__), __func__, __LINE__);
                break;
            }

            stThreshU16Ctrl.u16LowThr = (u16)(attr.low_thr & 0xffff);
            stThreshU16Ctrl.u16HighThr = (u16)(attr.high_thr & 0xffff);
            stThreshU16Ctrl.u8MinVal = (u8)(attr.min_val & 0xff);
            stThreshU16Ctrl.u8MidVal = (u8)(attr.mid_val & 0xff);
            stThreshU16Ctrl.u8MaxVal = (u8)(attr.max_val & 0xff);
            ret = BM_IVE_Thresh_U16(pIveHandle, &stSrc, &stDst, &stThreshU16Ctrl, true);
            break;
        case MOD_S16:
            stThreshS16Ctrl.enMode = (IVE_THRESH_S16_MODE_E)thransform_pattern(thresh_mode);
            if (stThreshS16Ctrl.enMode < 0 || stThreshS16Ctrl.enMode >= IVE_THRESH_S16_MODE_BUTT) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                        "unknown thresh mode %d, %s: %s: %d\n",
                        thresh_mode, filename(__FILE__), __func__, __LINE__);
                break;
            }

            stThreshS16Ctrl.s16LowThr = (s16)(attr.low_thr & 0xffff);
            stThreshS16Ctrl.s16HighThr = (s16)(attr.high_thr & 0xffff);
            if (stThreshS16Ctrl.enMode == IVE_THRESH_S16_MODE_S16_TO_S8_MIN_MID_MAX ||
                stThreshS16Ctrl.enMode == IVE_THRESH_S16_MODE_S16_TO_S8_MIN_ORI_MAX) {
                    stThreshS16Ctrl.un8MinVal.s8Val = (s8)(attr.min_val & 0xff);
                    stThreshS16Ctrl.un8MidVal.s8Val = (s8)(attr.mid_val & 0xff);
                    stThreshS16Ctrl.un8MaxVal.s8Val = (s8)(attr.max_val & 0xff);
            } else {
                    stThreshS16Ctrl.un8MinVal.u8Val = (u8)(attr.min_val & 0xff);
                    stThreshS16Ctrl.un8MidVal.u8Val = (u8)(attr.mid_val & 0xff);
                    stThreshS16Ctrl.un8MaxVal.u8Val = (u8)(attr.max_val & 0xff);
            }
            ret = BM_IVE_Thresh_S16(pIveHandle, &stSrc, &stDst, &stThreshS16Ctrl, true);
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
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }
    BM_IVE_DestroyHandle(pIveHandle);

    return ret;
}

bm_status_t BM_IVE_DMA(IVE_HANDLE pIveHandle, IVE_DATA_S *pstSrc,
            IVE_DST_DATA_S *pstDst, IVE_DMA_CTRL_S *pstCtrl,
            bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_dma_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    ive_arg.stDst = *pstDst;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    // invoke ive api in driver
    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    if (ioctl(p->dev_fd, CVI_IVE_IOC_DMA, &ioctl_arg) < 0) {
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
    IVE_HANDLE pIveHandle = NULL;
    IVE_DATA_S stSrc;
    IVE_DST_DATA_S stDst;
    IVE_DMA_CTRL_S stDmaCtrl;

    memset(&stSrc, 0, sizeof(IVE_DATA_S));
    memset(&stDst, 0, sizeof(IVE_DATA_S));
    memset(&stDmaCtrl, 0, sizeof(IVE_DMA_CTRL_S));

    // transfer bm struct to ive struct
    // TODO: only support one channel now
    ret = bm_image_convert_to_ive_data(handle, 0, &image, &stSrc);
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
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    stDmaCtrl.enMode = (IVE_DMA_MODE_E)(dma_set_mode + 2);
    printf("stDmaCtrl.enMode = %d \n", stDmaCtrl.enMode);
    stDmaCtrl.u64Val = val;
    stDmaCtrl.u8HorSegSize = 0;
    stDmaCtrl.u8ElemSize = 0;
    stDmaCtrl.u8VerSegRows = 0;

    ret = BM_IVE_DMA(pIveHandle, &stSrc, &stDst, &stDmaCtrl, true);

    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "bm_ive_dma_set error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);

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
    IVE_HANDLE pIveHandle = NULL;
    IVE_DATA_S stSrc;
    IVE_DST_DATA_S stDst;
    IVE_DMA_CTRL_S stDmaCtrl;

    memset(&stSrc, 0, sizeof(IVE_DATA_S));
    memset(&stDst, 0, sizeof(IVE_DATA_S));
    memset(&stDmaCtrl, 0, sizeof(IVE_DMA_CTRL_S));

    // transfer bm struct to ive struct
    // TODO: only support one channel now
    ret = bm_image_convert_to_ive_data(handle, 0, &input, &stSrc);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_data %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_data(handle, 0, &output, &stDst);
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
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }
    stDmaCtrl.enMode = (IVE_DMA_MODE_E)dma_mode;
    if (dma_mode == 0) {
        stDmaCtrl.u64Val = 0;
        stDmaCtrl.u8HorSegSize = 0;
        stDmaCtrl.u8ElemSize = 0;
        stDmaCtrl.u8VerSegRows = 0;
    } else if (dma_mode == 1){
        stDmaCtrl.u64Val = 0;
        stDmaCtrl.u8HorSegSize = (u8)attr->hor_seg_size;
        stDmaCtrl.u8ElemSize = (u8)attr->elem_size;
        stDmaCtrl.u8VerSegRows = (u8)attr->ver_seg_rows;
    }

    ret = BM_IVE_DMA(pIveHandle, &stSrc, &stDst, &stDmaCtrl, true);

    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "bm_ive_dma error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);

    return ret;
}

bm_status_t BM_IVE_MAP(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
            IVE_SRC_MEM_INFO_S *pstMap, IVE_DST_IMAGE_S *pstDst,
            IVE_MAP_CTRL_S *pstCtrl, bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_map_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    ive_arg.stMap = *pstMap;
    ive_arg.stDst = *pstDst;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    // invoke ive api in driver
    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    ioctl_arg.buffer = (void *)pstMap->u64VirAddr;
    ioctl_arg.u32Size = pstMap->u32Size;
    if (ioctl(p->dev_fd, CVI_IVE_IOC_Map, &ioctl_arg) < 0) {
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
    IVE_HANDLE pIveHandle = NULL;
    IVE_IMAGE_S stSrc;
    IVE_SRC_MEM_INFO_S stTable;
    IVE_DST_IMAGE_S stDst;
    IVE_MAP_CTRL_S stMapCtrl;

    memset(&stSrc, 0, sizeof(IVE_IMAGE_S));
    memset(&stTable, 0, sizeof(IVE_SRC_MEM_INFO_S));
    memset(&stDst, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&stMapCtrl, 0, sizeof(IVE_MAP_CTRL_S));

    // transfer bm struct to ive struct
    ret = bm_image_convert_to_ive_image(handle, &input, &stSrc);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, &output, &stDst);
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
        stMapCtrl.enMode = (IVE_MAP_MODE_E)mapMode;
    }
    stTable.u32Size = map_table.size;
    stTable.u64PhyAddr = map_table.u.device.device_addr;
    stTable.u64VirAddr = virt_addr;

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_MAP(pIveHandle, &stSrc, &stTable, &stDst, &stMapCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "BM_IVE_MAP failed %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);

        bm_mem_unmap_device_mem(handle, (void *)virt_addr, MAP_TABLE_SIZE);
        return ret;
    }

    BM_IVE_DestroyHandle(pIveHandle);

    bm_mem_unmap_device_mem(handle, (void *)virt_addr, MAP_TABLE_SIZE);

    return ret;
}

bm_status_t BM_IVE_Hist(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
              IVE_DST_MEM_INFO_S *pstDst, bool bInstant){
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_hist_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }
    ive_arg.pIveHandle = pIveHandle;
    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    ive_arg.stDst = *pstDst;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_Hist, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_hist(
    bm_handle_t      handle,
    bm_image         image,
    bm_device_mem_t  output)
{
    bm_status_t ret = BM_SUCCESS;
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src;
    IVE_DST_MEM_INFO_S dst;

    memset(&src, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dst, 0, sizeof(IVE_DST_MEM_INFO_S));
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

    dst.u64VirAddr = vir_addr;
    dst.u64PhyAddr = bm_mem_get_device_addr(output);
    dst.u32Size = output.size;

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // bInstant: true is interrupt mode, false is polling mode
    ret = BM_IVE_Hist(pIveHandle, &src, &dst, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_hist error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        bm_mem_unmap_device_mem(handle, (void *)vir_addr, output.size);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);
    bm_mem_unmap_device_mem(handle, (void *)vir_addr, output.size);

    return ret;
}

bm_status_t BM_IVE_Integ(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
              IVE_DST_MEM_INFO_S *pstDst, IVE_INTEG_CTRL_S *pstCtrl,
              bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_integ_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;
    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    ive_arg.stDst = *pstDst;
    ive_arg.stCtrl = *pstCtrl;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_Integ, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_integ(
    bm_handle_t              handle,
    bm_image                 input,
    bm_device_mem_t          output,
    bmcv_ive_integ_ctrl_s    integ_attr)
{
    bm_status_t ret = BM_SUCCESS;
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src;
    IVE_DST_MEM_INFO_S dst;
    IVE_INTEG_CTRL_S pstIntegCtrl;

    memset(&src, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dst, 0, sizeof(IVE_DST_MEM_INFO_S));
    memset(&pstIntegCtrl, 0, sizeof(IVE_INTEG_CTRL_S));

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

    dst.u64VirAddr = vir_addr;
    dst.u32Size = output.size;
    dst.u64PhyAddr = bm_mem_get_device_addr(output);

    // set ive integ params
    pstIntegCtrl.enOutCtrl = integ_attr.en_out_ctrl;

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // bInstant: true is interrupt mode, false is polling mode
    ret = BM_IVE_Integ(pIveHandle, &src, &dst, &pstIntegCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_intge error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        bm_mem_unmap_device_mem(handle, (void *)vir_addr, output.size);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);
    bm_mem_unmap_device_mem(handle, (void *)vir_addr, output.size);
    return ret;
}

bm_status_t BM_IVE_NCC(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
            IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_MEM_INFO_S *pstDst,
            bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_ncc_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc1 = *pstSrc1;
    ive_arg.stSrc2 = *pstSrc2;
    ive_arg.stDst = *pstDst;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    ioctl_arg.buffer = (void *)pstDst->u64VirAddr;
    ioctl_arg.u32Size = sizeof(IVE_NCC_DST_MEM_S);
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_NCC, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_ncc(
    bm_handle_t         handle,
    bm_image            input1,
    bm_image            input2,
    bm_device_mem_t     output)
{
    bm_status_t ret = BM_SUCCESS;
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src1;
    IVE_SRC_IMAGE_S src2;
    IVE_DST_MEM_INFO_S dst;

    memset(&src1, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&src2, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dst, 0, sizeof(IVE_DST_MEM_INFO_S));

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

    dst.u64VirAddr = vir_addr;
    dst.u64PhyAddr = bm_mem_get_device_addr(output);
    dst.u32Size = output.size;

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // bInstant: true is interrupt mode, false is polling mode
    ret = BM_IVE_NCC(pIveHandle, &src1, &src2, &dst, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_ncc error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        bm_mem_unmap_device_mem(handle, (void *)vir_addr, output.size);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);
    bm_mem_unmap_device_mem(handle, (void *)vir_addr, output.size);
    return ret;
}

bm_status_t BM_IVE_OrdStatFilter(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
                  IVE_DST_IMAGE_S *pstDst,
                  IVE_ORD_STAT_FILTER_CTRL_S *pstCtrl,
                  bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_ord_stat_filter_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    ive_arg.stDst = *pstDst;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_OrdStatFilter, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_ord_stat_filter(
    bm_handle_t                   handle,
    bm_image *                    input,
    bm_image *                    output,
    bmcv_ive_ord_stat_filter_mode mode)
{
    bm_status_t ret = BM_SUCCESS;
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src;
    IVE_DST_IMAGE_S dst;
    IVE_ORD_STAT_FILTER_CTRL_S pstCtrl;

    memset(&src, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dst, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&pstCtrl, 0, sizeof(IVE_ORD_STAT_FILTER_CTRL_S));

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

    pstCtrl.enMode = mode;

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // bInstant: true is interrupt mode, false is polling mode
    ret = BM_IVE_OrdStatFilter(pIveHandle, &src, &dst, &pstCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_ordStatFilter error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);
    return ret;
}

bm_status_t BM_IVE_LBP(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
            IVE_DST_IMAGE_S *pstDst, IVE_LBP_CTRL_S *pstCtrl,
            bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_lbp_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.stSrc = *pstSrc;
    ive_arg.stDst = *pstDst;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_LBP, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_lbp(
    bm_handle_t              handle,
    bm_image *               input,
    bm_image *               output,
    bmcv_ive_lbp_ctrl_attr * lbp_attr)
{
    bm_status_t ret = BM_SUCCESS;
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src;
    IVE_DST_IMAGE_S dst;
    IVE_LBP_CTRL_S pstlbpCtrl;

    memset(&src, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dst, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&pstlbpCtrl, 0, sizeof(IVE_LBP_CTRL_S));

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

    memcpy(&pstlbpCtrl, lbp_attr, sizeof(IVE_LBP_CTRL_S));

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // bInstant: true is interrupt mode, false is polling mode
    ret = BM_IVE_LBP(pIveHandle, &src, &dst, &pstlbpCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_lbp error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);
    return ret;
}


bm_status_t BM_IVE_Dilate(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
               IVE_DST_IMAGE_S *pstDst, IVE_DILATE_CTRL_S *pstctrl,
               bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_dilate_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    ive_arg.stDst = *pstDst;
    ive_arg.stCtrl = *pstctrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_Dilate, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_dilate(
    bm_handle_t           handle,
    bm_image *            input,
    bm_image *            output,
    unsigned char*        dilate_mask)
{
    bm_status_t ret = BM_SUCCESS;
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src;
    IVE_DST_IMAGE_S dst;
    IVE_DILATE_CTRL_S dilateCtrl;

    memset(&src, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dst, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&dilateCtrl, 0, sizeof(IVE_DILATE_CTRL_S));

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

    memcpy(&dilateCtrl.au8Mask, dilate_mask, sizeof(IVE_DILATE_CTRL_S));

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // bInstant: true is interrupt mode, false is polling mode
    ret = BM_IVE_Dilate(pIveHandle, &src, &dst, &dilateCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_dilate error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);
    return ret;
}

bm_status_t BM_IVE_Erode(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
              IVE_DST_IMAGE_S *pstDst, IVE_ERODE_CTRL_S *pstCtrl,
              bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_erode_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    ive_arg.stDst = *pstDst;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_Erode, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_erode(
    bm_handle_t           handle,
    bm_image *            input,
    bm_image *            output,
    unsigned char *       erode_mask)
{
    bm_status_t ret = BM_SUCCESS;
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src;
    IVE_DST_IMAGE_S dst;
    IVE_ERODE_CTRL_S pstErodeCtrl;

    memset(&src, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dst, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&pstErodeCtrl, 0, sizeof(IVE_ERODE_CTRL_S));

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

    // memcpy(&pstErodeCtrl, erode_attr, sizeof(IVE_ERODE_CTRL_S));
    memcpy(&pstErodeCtrl.au8Mask, erode_mask, sizeof(IVE_ERODE_CTRL_S));
    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // bInstant: true is interrupt mode, false is polling mode
    ret = BM_IVE_Erode(pIveHandle, &src, &dst, &pstErodeCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_erode error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);
    return ret;
}

bm_status_t BM_IVE_MagAndAng(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
              IVE_DST_IMAGE_S *pstDstMag,
              IVE_DST_IMAGE_S *pstDstAng,
              IVE_MAG_AND_ANG_CTRL_S *pstCtrl, bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_maganang_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;
    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    if (pstDstMag != NULL)
        ive_arg.stDstMag = *pstDstMag;
    if (pstDstAng != NULL)
        ive_arg.stDstAng = *pstDstAng;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_MagAndAng, &ioctl_arg);
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
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src;
    IVE_DST_IMAGE_S dstMag;
    IVE_DST_IMAGE_S dstAng;
    IVE_MAG_AND_ANG_CTRL_S magAndAng_outCtrl;

    memset(&src, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dstMag, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&dstAng, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&magAndAng_outCtrl, 0, sizeof(IVE_MAG_AND_ANG_CTRL_S));

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

    memcpy(&magAndAng_outCtrl, attr, sizeof(IVE_MAG_AND_ANG_CTRL_S));

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // bInstant: true is interrupt mode, false is polling mode
    ret = BM_IVE_MagAndAng(pIveHandle, &src, &dstMag, &dstAng, &magAndAng_outCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_magAndAng error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);
    return ret;
}

bm_status_t BM_IVE_Sobel(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
              IVE_DST_IMAGE_S *pstDstH, IVE_DST_IMAGE_S *pstDstV,
              IVE_SOBEL_CTRL_S *pstCtrl, bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_sobel_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;
    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    if(pstDstH != NULL)
        ive_arg.stDstH = *pstDstH;
    if(pstDstV != NULL)
        ive_arg.stDstV = *pstDstV;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_Sobel, &ioctl_arg);
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
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src;
    IVE_DST_IMAGE_S dstH;
    IVE_DST_IMAGE_S dstV;
    IVE_SOBEL_CTRL_S sobelCtrl;

    memset(&src, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dstH, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&dstV, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&sobelCtrl, 0, sizeof(IVE_SOBEL_CTRL_S));

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

    memcpy(&sobelCtrl, sobel_attr, sizeof(IVE_SOBEL_CTRL_S));

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // bInstant: true is interrupt mode, false is polling mode
    ret = BM_IVE_Sobel(pIveHandle, &src, &dstH, &dstV, &sobelCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_sobel error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);
    return ret;
}

bm_status_t BM_IVE_NormGrad(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
             IVE_DST_IMAGE_S *pstDstH, IVE_DST_IMAGE_S *pstDstV,
             IVE_DST_IMAGE_S *pstDstHV,
             IVE_NORM_GRAD_CTRL_S *pstCtrl, bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_norm_grad_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;
    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    if(pstDstH != NULL)
        ive_arg.stDstH = *pstDstH;
    if(pstDstV != NULL)
        ive_arg.stDstV = *pstDstV;
    if(pstDstHV != NULL)
        ive_arg.stDstHV = *pstDstHV;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_NormGrad, &ioctl_arg);
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
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src;
    IVE_DST_IMAGE_S dstH;
    IVE_DST_IMAGE_S dstV;
    IVE_DST_IMAGE_S dstHV;
    IVE_NORM_GRAD_CTRL_S normGradCtrl;

    memset(&src, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dstH, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&dstV, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&dstHV, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&normGradCtrl, 0, sizeof(IVE_NORM_GRAD_CTRL_S));


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

    memcpy(&normGradCtrl, normgrad_attr, sizeof(IVE_NORM_GRAD_CTRL_S));

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // bInstant: true is interrupt mode, false is polling mode
    ret = BM_IVE_NormGrad(pIveHandle, &src, &dstH, &dstV, &dstHV, &normGradCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_normgrad error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);
    return ret;
}

bm_status_t BM_IVE_GMM(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
            IVE_DST_IMAGE_S *pstFg, IVE_DST_IMAGE_S *pstBg,
            IVE_MEM_INFO_S *pstModel, IVE_GMM_CTRL_S *pstCtrl,
            bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_gmm_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;
    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    ive_arg.stFg = *pstFg;
    ive_arg.stBg = *pstBg;
    ive_arg.stModel = *pstModel;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_GMM, &ioctl_arg);
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
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src;
    IVE_DST_IMAGE_S dstFg;
    IVE_DST_IMAGE_S dstBg;
    IVE_MEM_INFO_S  dstModel;
    IVE_GMM_CTRL_S gmmCtrl;

    memset(&src, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dstFg, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&dstBg, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&dstModel, 0, sizeof(IVE_MEM_INFO_S));
    memset(&gmmCtrl, 0, sizeof(IVE_GMM_CTRL_S));

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

    dstModel.u64VirAddr = vir_addr;
    dstModel.u64PhyAddr = bm_mem_get_device_addr(*output_model);
    dstModel.u32Size = output_model->size;

    memcpy(&gmmCtrl, gmm_attr, sizeof(IVE_GMM_CTRL_S));

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // bInstant: true is interrupt mode, false is polling mode
    ret = BM_IVE_GMM(pIveHandle, &src, &dstFg, &dstBg, &dstModel, &gmmCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_gmm error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        bm_mem_unmap_device_mem(handle, (void *)vir_addr, output_model->size);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);
    bm_mem_unmap_device_mem(handle, (void *)vir_addr, output_model->size);

    return ret;
}

bm_status_t BM_IVE_GMM2(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
             IVE_SRC_IMAGE_S *pstFactor, IVE_DST_IMAGE_S *pstFg,
             IVE_DST_IMAGE_S *pstBg, IVE_DST_IMAGE_S *pstMatchModelInfo,
             IVE_MEM_INFO_S *pstModel, IVE_GMM2_CTRL_S *pstCtrl,
             bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_gmm2_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;
    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    ive_arg.stFactor = *pstFactor;
    ive_arg.stFg = *pstFg;
    ive_arg.stBg = *pstBg;
    ive_arg.stInfo = *pstMatchModelInfo;
    ive_arg.stModel = *pstModel;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_GMM2, &ioctl_arg);
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
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src;
    IVE_SRC_IMAGE_S srcFactor;
    IVE_DST_IMAGE_S dstFg;
    IVE_DST_IMAGE_S dstBg;
    IVE_DST_IMAGE_S dstMatchModelInfo;
    IVE_MEM_INFO_S  dstModel;
    IVE_GMM2_CTRL_S gmm2Ctrl;

    memset(&src, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&srcFactor, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dstFg, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&dstBg, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&dstMatchModelInfo, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&dstModel, 0, sizeof(IVE_MEM_INFO_S));
    memset(&gmm2Ctrl, 0, sizeof(IVE_GMM2_CTRL_S));

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

    dstModel.u64VirAddr = vir_addr;
    dstModel.u64PhyAddr = bm_mem_get_device_addr(*output_model);
    dstModel.u32Size = output_model->size;

    memcpy(&gmm2Ctrl, gmm2_attr, sizeof(IVE_GMM2_CTRL_S));

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // bInstant: true is interrupt mode, false is polling mode
    ret = BM_IVE_GMM2(pIveHandle, &src, &srcFactor, &dstFg, &dstBg,
                           &dstMatchModelInfo, &dstModel, &gmm2Ctrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_gmm2 error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        bm_mem_unmap_device_mem(handle, (void *)vir_addr, output_model->size);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);
    bm_mem_unmap_device_mem(handle, (void *)vir_addr, output_model->size);

    return ret;
}

bm_status_t BM_IVE_CanngHysEdge(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
             IVE_DST_IMAGE_S *pstEdge, IVE_DST_MEM_INFO_S *pstStack,
             IVE_CANNY_HYS_EDGE_CTRL_S *pstCtrl, bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_canny_hys_edge_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;
    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    ive_arg.stDst = *pstEdge;
    ive_arg.stStack = *pstStack;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_CannyHysEdge, &ioctl_arg);
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
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src;
    IVE_DST_IMAGE_S dstEdge;
    IVE_DST_MEM_INFO_S dstStack;
    IVE_CANNY_HYS_EDGE_CTRL_S cannyHysEdgeCtrl;

    memset(&src, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dstEdge, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&dstStack, 0, sizeof(IVE_DST_MEM_INFO_S));
    memset(&cannyHysEdgeCtrl, 0, sizeof(IVE_CANNY_HYS_EDGE_CTRL_S));

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
    dstStack.u64VirAddr = vir_addr;
    dstStack.u64PhyAddr = bm_mem_get_device_addr(*output_stack);
    dstStack.u32Size = output_stack->size;

    cannyHysEdgeCtrl.u16LowThr = canny_hys_edge_attr->u16_low_thr;
    cannyHysEdgeCtrl.u16HighThr = canny_hys_edge_attr->u16_high_thr;
    memcpy(cannyHysEdgeCtrl.as8Mask, canny_hys_edge_attr->as8_mask, 5 * 5 * sizeof(signed char));

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // bInstant: true is interrupt mode, false is polling mode
    ret = BM_IVE_CanngHysEdge(pIveHandle, &src, &dstEdge, &dstStack, &cannyHysEdgeCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_cannyHysEdge error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        bm_mem_unmap_device_mem(handle, (void *)vir_addr, output_stack->size);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);
    bm_mem_unmap_device_mem(handle, (void *)vir_addr, output_stack->size);
    return ret;
}

bm_status_t BM_IVE_Filter(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
               IVE_DST_IMAGE_S *pstDst, IVE_FILTER_CTRL_S *pstCtrl,
               bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_filter_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;
    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    ive_arg.stDst = *pstDst;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_Filter, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_filter(
    bm_handle_t                  handle,
    bm_image                     input,
    bm_image                     output,
    bmcv_ive_filter_ctrl         filter_attr)
{
    bm_status_t ret = BM_SUCCESS;
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src;
    IVE_DST_IMAGE_S dst;
    IVE_FILTER_CTRL_S filterAttr;

    memset(&src, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dst, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&filterAttr, 0, sizeof(IVE_FILTER_CTRL_S));

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

    memcpy(&filterAttr, &filter_attr, sizeof(IVE_FILTER_CTRL_S));

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // bInstant: true is interrupt mode, false is polling mode
    ret = BM_IVE_Filter(pIveHandle, &src, &dst, &filterAttr, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_filter error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);

    return ret;
}

bm_status_t BM_IVE_CSC(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
            IVE_DST_IMAGE_S *pstDst, IVE_CSC_CTRL_S *pstCtrl,
            bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_csc_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;
    if(p == NULL || p->dev_fd <= 0){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    ive_arg.stDst = *pstDst;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_CSC, &ioctl_arg);
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

IVE_CSC_MODE_E bm_image_format_to_ive_csc_mode(csc_type_t csc_type, bm_image* input, bm_image* output)
{
    IVE_CSC_MODE_E mode = IVE_CSC_MODE_BUTT;

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
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src;
    IVE_DST_IMAGE_S dst;
    IVE_CSC_CTRL_S cscCtrl;

    memset(&src, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dst, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&cscCtrl, 0, sizeof(IVE_CSC_CTRL_S));

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
    cscCtrl.enMode = (IVE_CSC_MODE_E)csc_mode;

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // bInstant: true is interrupt mode, false is polling mode
    ret = BM_IVE_CSC(pIveHandle, &src, &dst, &cscCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_csc error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);

    return ret;
}

bm_status_t BM_IVE_Resize(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *astSrc,
               IVE_DST_IMAGE_S *astDst, IVE_RESIZE_CTRL_S *pstCtrl,
               bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_resize_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }
    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *astSrc;
    ive_arg.stDst = *astDst;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_Resize, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_resize(
    bm_handle_t               handle,
    bm_image *                input,
    bm_image *                output,
    bmcv_resize_algorithm     resize_mode)
{
    bm_status_t ret = BM_SUCCESS;
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S stSrc;
    IVE_DST_IMAGE_S stDst;
    IVE_RESIZE_CTRL_S stCtrl;

    memset(&stSrc, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&stDst, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&stCtrl, 0, sizeof(IVE_RESIZE_CTRL_S));

    // transfer bm struct to ive struct
    ret = bm_image_convert_to_ive_image(handle, input, &stSrc);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
        "failed to convert bm_image to ive_image %s: %s: %d\n",
        filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, output, &stDst);
    if (ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
        "failed to convert bm_image to ive_image %s: %s: %d\n",
        filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // set ive resize params
    if(resize_mode == BMCV_INTER_LINEAR){
        stCtrl.enMode = resize_mode - 1;
    } else if (resize_mode == BMCV_INTER_AREA)
    {
        stCtrl.enMode = resize_mode - 2;
    } else {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
        "please check, ive no support this resize mode. %s: %s: %d\n",
        filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // bInstant: true is interrupt mode, false is polling mode
    ret = BM_IVE_Resize(pIveHandle, &stSrc, &stDst, &stCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_resize error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }
    BM_IVE_DestroyHandle(pIveHandle);

    return ret;
}

bm_status_t BM_IVE_STCandiCorner(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
                  IVE_DST_IMAGE_S *pstDst, IVE_ST_CANDI_CORNER_CTRL_S *pstCtrl,
                  bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_stcandicorner ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    ive_arg.stDst = *pstDst;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_STCandiCorner, &ioctl_arg);
    return ret;
}

bm_status_t bm_ive_stCandiCorner(
    bm_handle_t                   handle,
    bm_image *                    input,
    bm_image *                    output,
    bmcv_ive_stcandicorner_attr * stcandicorner_attr)
{
    bm_status_t ret = BM_SUCCESS;
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src;
    IVE_DST_IMAGE_S dst;
    IVE_ST_CANDI_CORNER_CTRL_S stCandiCornerAttr;

    memset(&src, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dst, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&stCandiCornerAttr, 0, sizeof(IVE_ST_CANDI_CORNER_CTRL_S));

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
    stCandiCornerAttr.stMem.u64VirAddr = vir_addr;
    stCandiCornerAttr.stMem.u64PhyAddr = bm_mem_get_device_addr(stcandicorner_attr->st_mem);
    stCandiCornerAttr.stMem.u32Size = stcandicorner_attr->st_mem.size;
    stCandiCornerAttr.u0q8QualityLevel = stcandicorner_attr->u0q8_quality_level;

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // bInstant: true is interrupt mode, false is polling mode
    ret = BM_IVE_STCandiCorner(pIveHandle, &src, &dst, &stCandiCornerAttr, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "BM_IVE_STCandiCorner error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        bm_mem_unmap_device_mem(handle, (void *)vir_addr, stcandicorner_attr->st_mem.size);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);
    bm_mem_unmap_device_mem(handle, (void *)vir_addr, stcandicorner_attr->st_mem.size);
    return ret;
}

bm_status_t BM_IVE_GradFg(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstBgDiffFg,
               IVE_SRC_IMAGE_S *pstCurGrad, IVE_SRC_IMAGE_S *pstBgGrad,
               IVE_DST_IMAGE_S *pstGradFg, IVE_GRAD_FG_CTRL_S *pstCtrl,
               bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_grad_fg_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    if(pstBgDiffFg != NULL) ive_arg.stBgDiffFg = *pstBgDiffFg;
    if(pstCurGrad != NULL) ive_arg.stCurGrad = *pstCurGrad;
    ive_arg.stBgGrad = *pstBgGrad;
    ive_arg.stGradFg = *pstGradFg;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_GradFg, &ioctl_arg);

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
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S srcBgDiffFg;
    IVE_SRC_IMAGE_S srcCurGrad;
    IVE_SRC_IMAGE_S srcBgGrad;
    IVE_DST_IMAGE_S dstGradFg;
    IVE_GRAD_FG_CTRL_S gradFgAttr;

    memset(&srcBgDiffFg, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&srcCurGrad, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&srcBgGrad, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dstGradFg, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&gradFgAttr, 0, sizeof(IVE_GRAD_FG_CTRL_S));

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

    memcpy(&gradFgAttr, gradfg_attr, sizeof(IVE_GRAD_FG_CTRL_S));

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // bInstant: true is interrupt mode, false is polling mode
    ret = BM_IVE_GradFg(pIveHandle, &srcBgDiffFg, &srcCurGrad, &srcBgGrad, &dstGradFg, &gradFgAttr, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "BM_IVE_GradFg error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);
    return ret;
}

bm_status_t BM_IVE_SAD(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
            IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstSad,
            IVE_DST_IMAGE_S *pstThr, IVE_SAD_CTRL_S *pstCtrl,
            bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_sad_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc1 = *pstSrc1;
    ive_arg.stSrc2 = *pstSrc2;
    ive_arg.stSad = *pstSad;
    ive_arg.stThr = *pstThr;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_SAD, &ioctl_arg);
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
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src1;
    IVE_SRC_IMAGE_S src2;
    IVE_DST_IMAGE_S dst_sad;
    IVE_DST_IMAGE_S dst_thr;
    IVE_SAD_CTRL_S sadAttr;

    memset(&src1, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&src2, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dst_sad, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&dst_thr, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&sadAttr, 0, sizeof(IVE_SAD_CTRL_S));

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

    sadAttr.enMode = sad_attr->en_mode;
    sadAttr.enOutCtrl = (IVE_SAD_OUT_CTRL_E)bm_image_convert_to_sad_enOutCtrl(sad_attr, output_sad);
    if(thresh_attr == NULL) {
        sadAttr.u16Thr = 0;
        sadAttr.u8MaxVal = 0;
        sadAttr.u8MinVal = 0;
    } else {
        sadAttr.u16Thr = thresh_attr->u16_thr;
        sadAttr.u8MaxVal = thresh_attr->u8_max_val;
        sadAttr.u8MinVal = thresh_attr->u8_min_val;
    }
    // memcpy(&sadAttr, sad_attr, sizeof(IVE_SAD_CTRL_S));

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    // bInstant: true is interrupt mode, false is polling mode
    ret = BM_IVE_SAD(pIveHandle, &src1, &src2, &dst_sad, &dst_thr, &sadAttr, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_sad error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);
    return ret;
}

bm_status_t BM_IVE_MatchBgModel(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstCurImg,
                 IVE_DATA_S *pstBgModel, IVE_IMAGE_S *pstFgFlag,
                 IVE_DST_IMAGE_S *pstDiffFg, IVE_DST_MEM_INFO_S *pstStatData,
                 IVE_MATCH_BG_MODEL_CTRL_S *pstCtrl, bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_match_bgmodel_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stCurImg = *pstCurImg;
    ive_arg.stBgModel = *pstBgModel;
    ive_arg.stFgFlag = *pstFgFlag;
    ive_arg.stDiffFg = *pstDiffFg;
    ive_arg.stStatData = *pstStatData;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    ioctl_arg.buffer = (void *)(uintptr_t)pstStatData->u64VirAddr;
    ioctl_arg.u32Size = sizeof(IVE_BG_STAT_DATA_S);
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_MatchBgModel, &ioctl_arg);
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
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S  pstCurImg;
    IVE_DATA_S pstBgModel;
    IVE_IMAGE_S pstFgFlag;
    IVE_DST_IMAGE_S pstDiffFg;
    IVE_DST_MEM_INFO_S pstStatData;
    IVE_MATCH_BG_MODEL_CTRL_S pstCtrl;

    memset(&pstCurImg, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&pstBgModel, 0, sizeof(IVE_DATA_S));
    memset(&pstFgFlag, 0, sizeof(IVE_IMAGE_S));
    memset(&pstDiffFg, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&pstStatData, 0, sizeof(IVE_DST_MEM_INFO_S));
    memset(&pstCtrl, 0, sizeof(IVE_MATCH_BG_MODEL_CTRL_S));

    ret = bm_image_convert_to_ive_image(handle, cur_img, &pstCurImg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_data(handle, 0, bgmodel_img, &pstBgModel);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_data %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, fgflag_img, &pstFgFlag);
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
    pstStatData.u64VirAddr = vir_addr;
    pstStatData.u64PhyAddr = bm_mem_get_device_addr(*stat_data_mem);
    pstStatData.u32Size = stat_data_mem->size;

    memcpy(&pstCtrl, attr, sizeof(IVE_MATCH_BG_MODEL_CTRL_S));

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_MatchBgModel(pIveHandle, &pstCurImg, &pstBgModel,
                 &pstFgFlag, &pstDiffFg, &pstStatData, &pstCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_match_bgmodel error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        bm_mem_unmap_device_mem(handle, (void *)vir_addr, stat_data_mem->size);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }

    bm_mem_unmap_device_mem(handle, (void *)vir_addr, stat_data_mem->size);
    BM_IVE_DestroyHandle(pIveHandle);
    return ret;
}

bm_status_t BM_IVE_UpdateBgModel(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstCurImg, IVE_DATA_S *pstBgModel,
            IVE_IMAGE_S *pstFgFlag, IVE_DST_IMAGE_S *pstBgImg,
            IVE_DST_IMAGE_S *pstChgSta, IVE_DST_MEM_INFO_S *pstStatData,
            IVE_UPDATE_BG_MODEL_CTRL_S *pstCtrl, bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_update_bgmodel_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }
    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stCurImg = *pstCurImg;
    ive_arg.stBgModel = *pstBgModel;
    ive_arg.stFgFlag = *pstFgFlag;
    ive_arg.stBgImg = *pstBgImg;
    ive_arg.stChgSta = *pstChgSta;
    ive_arg.stStatData = *pstStatData;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    ioctl_arg.buffer = (void *)(uintptr_t)pstStatData->u64VirAddr;
    ioctl_arg.u32Size = sizeof(IVE_BG_STAT_DATA_S);
    bm_status_t ret = (bm_status_t)ioctl(p->dev_fd, CVI_IVE_IOC_UpdateBgModel, &ioctl_arg);
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
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S  pstCurImg;
    IVE_DATA_S pstBgModel;
    IVE_IMAGE_S pstFgFlag;
    IVE_DST_IMAGE_S pstBgImg;
    IVE_DST_IMAGE_S pstChgSta;
    IVE_DST_MEM_INFO_S pstStatData;
    IVE_UPDATE_BG_MODEL_CTRL_S pstCtrl;

    memset(&pstCurImg, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&pstBgModel, 0, sizeof(IVE_DATA_S));
    memset(&pstFgFlag, 0, sizeof(IVE_IMAGE_S));
    memset(&pstBgImg, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&pstChgSta, 0, sizeof(IVE_DST_MEM_INFO_S));
    memset(&pstCtrl, 0, sizeof(IVE_UPDATE_BG_MODEL_CTRL_S));

    ret = bm_image_convert_to_ive_image(handle, cur_img, &pstCurImg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_data %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_data(handle, 0, bgmodel_img, &pstBgModel);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_data %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, fgflag_img, &pstFgFlag);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, bg_img, &pstBgImg);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "failed to convert bm_image to ive_image %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
    }

    ret = bm_image_convert_to_ive_image(handle, chgsta_img, &pstChgSta);
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

    pstStatData.u64VirAddr = vir_addr;
    pstStatData.u64PhyAddr = bm_mem_get_device_addr(*stat_data_mem);
    pstStatData.u32Size = stat_data_mem->size;

    memcpy(&pstCtrl, attr, sizeof(IVE_UPDATE_BG_MODEL_CTRL_S));

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_UpdateBgModel(pIveHandle, &pstCurImg, &pstBgModel, &pstFgFlag,
                       &pstBgImg, &pstChgSta, &pstStatData, &pstCtrl, true);
    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "cvi_ive_update_bgmodel error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        bm_mem_unmap_device_mem(handle, (void *)vir_addr, stat_data_mem->size);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }

    bm_mem_unmap_device_mem(handle, (void *)vir_addr, stat_data_mem->size);
    BM_IVE_DestroyHandle(pIveHandle);
    return ret;
}

bm_status_t BM_IVE_CCL(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstSrcDst,
        IVE_DST_MEM_INFO_S *pstBlob, IVE_CCL_CTRL_S *pstCclCtrl,
        CVI_BOOL bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_ccl_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrcDst = *pstSrcDst;
    ive_arg.stBlob = *pstBlob;
    ive_arg.stCclCtrl = *pstCclCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    ioctl_arg.buffer = (void *)(uintptr_t)pstBlob->u64VirAddr;
    ioctl_arg.u32Size = sizeof(CVI_U16) + sizeof(CVI_S8) + sizeof(CVI_U8);
    bm_status_t ret = (bm_status_t) ioctl(p->dev_fd, CVI_IVE_IOC_CCL, &ioctl_arg);

    return ret;
}

bm_status_t bm_ive_ccl(
    bm_handle_t          handle,
    bm_image *           src_dst_image,
    bm_device_mem_t *    ccblob_output,
    bmcv_ive_ccl_attr *  ccl_attr)
{
    bm_status_t ret = BM_SUCCESS;
    IVE_HANDLE pIveHandle = NULL;
    IVE_IMAGE_S pstSrcDst;
    IVE_DST_MEM_INFO_S pstBlob;
    IVE_CCL_CTRL_S pstCclCtrl;

    memset(&pstSrcDst, 0, sizeof(IVE_IMAGE_S));
    memset(&pstBlob, 0, sizeof(IVE_DST_MEM_INFO_S));
    memset(&pstCclCtrl, 0, sizeof(IVE_CCL_CTRL_S));

    ret = bm_image_convert_to_ive_image(handle, src_dst_image, &pstSrcDst);
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

    pstBlob.u64VirAddr = vir_addr;
    pstBlob.u64PhyAddr = bm_mem_get_device_addr(*ccblob_output);
    pstBlob.u32Size = ccblob_output->size;

    memcpy(&pstCclCtrl, ccl_attr, sizeof(IVE_CCL_CTRL_S));

     // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_CCL(pIveHandle, &pstSrcDst, &pstBlob, &pstCclCtrl, true);
    if(ret != BM_SUCCESS){
         bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "BM_IVE_CCL error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        bm_mem_unmap_device_mem(handle, (void *)vir_addr, ccblob_output->size);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }

    bm_mem_unmap_device_mem(handle, (void *)vir_addr, ccblob_output->size);
    BM_IVE_DestroyHandle(pIveHandle);
    return ret;
}

bm_status_t BM_IVE_BERNSEN(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
                IVE_DST_IMAGE_S *pstDst, IVE_BERNSEN_CTRL_S *pstCtrl,
                CVI_BOOL bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_bernsen_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    ive_arg.stDst = *pstDst;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t) ioctl(p->dev_fd, CVI_IVE_IOC_Bernsen, &ioctl_arg);

    return ret;
}

bm_status_t bm_ive_bernsen(
    bm_handle_t           handle,
    bm_image              input,
    bm_image              output,
    bmcv_ive_bernsen_attr attr)
{
    bm_status_t ret = BM_SUCCESS;
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src;
    IVE_DST_IMAGE_S dst;
    IVE_BERNSEN_CTRL_S bernsenAttr;

    memset(&src, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dst, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&bernsenAttr, 0, sizeof(IVE_BERNSEN_CTRL_S));

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

    memcpy(&bernsenAttr, &attr, sizeof(IVE_BERNSEN_CTRL_S));

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_BERNSEN(pIveHandle, &src, &dst, &bernsenAttr, true);
    if(ret != BM_SUCCESS){
         bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "BM_IVE_BERNSEN error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);

    return ret;
}

bm_status_t BM_IVE_FilterAndCsc(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
                    IVE_DST_IMAGE_S *pstDst,
                    IVE_FILTER_AND_CSC_CTRL_S *pstCtrl,
                    CVI_BOOL bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_filter_and_csc_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    ive_arg.stDst = *pstDst;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t) ioctl(p->dev_fd, CVI_IVE_IOC_FilterAndCSC, &ioctl_arg);

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
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src;
    IVE_DST_IMAGE_S dst;
    IVE_FILTER_AND_CSC_CTRL_S ctrl;

    memset(&src, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dst, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&ctrl, 0, sizeof(IVE_FILTER_AND_CSC_CTRL_S));

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

    ctrl.u8Norm = attr.u8_norm;
    memcpy(ctrl.as8Mask, attr.as8_mask, 5 * 5 * sizeof(signed char));
    int csc_mode = bm_image_format_filterandcsc_mode(csc_type);
    if (csc_mode < 0 || csc_mode >= IVE_CSC_MODE_BUTT) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            " csc_type is error, please check it. %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
    }
    ctrl.enMode = (IVE_CSC_MODE_E)csc_mode;

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_FilterAndCsc(pIveHandle, &src, &dst, &ctrl, true);
    if(ret != BM_SUCCESS){
         bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "BM_IVE_FilterAndCsc error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);

    return ret;
}

bm_status_t BM_IVE_16BitTo8Bit(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
                IVE_DST_IMAGE_S *pstDst,
                IVE_16BIT_TO_8BIT_CTRL_S *pstCtrl,
                bool bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_16bit_to_8bit_arg ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc = *pstSrc;
    ive_arg.stDst = *pstDst;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;
    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t) ioctl(p->dev_fd, CVI_IVE_IOC_16BitTo8Bit, &ioctl_arg);

    return ret;
}

bm_status_t bm_ive_16bit_to_8bit(
    bm_handle_t                 handle,
    bm_image                    input,
    bm_image                    output,
    bmcv_ive_16bit_to_8bit_attr attr)
{
    bm_status_t ret = BM_SUCCESS;
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src;
    IVE_DST_IMAGE_S dst;
    IVE_16BIT_TO_8BIT_CTRL_S ctrl;

    memset(&src, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dst, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&ctrl, 0, sizeof(IVE_16BIT_TO_8BIT_CTRL_S));

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

    memcpy(&ctrl, &attr, sizeof(IVE_16BIT_TO_8BIT_CTRL_S));

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_16BitTo8Bit(pIveHandle, &src, &dst, &ctrl, true);
    if(ret != BM_SUCCESS){
         bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "BM_IVE_16BitTo8Bit error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);

    return ret;
}

bm_status_t BM_IVE_FrameDiffMotion(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
            IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
            IVE_FRAME_DIFF_MOTION_CTRL_S *pstCtrl,
            CVI_BOOL bInstant)
{
    struct cvi_ive_ioctl_arg ioctl_arg;
    struct cvi_ive_ioctl_md ive_arg;
    struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

    if (p == NULL || p->dev_fd <= 0) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
            "Device ive is not open, please check it %s: %s: %d\n",
            filename(__FILE__), __func__, __LINE__);
        return BM_ERR_DEVNOTREADY;
    }

    ive_arg.pIveHandle = pIveHandle;
    ive_arg.stSrc1 = *pstSrc1;
    ive_arg.stSrc2 = *pstSrc2;
    ive_arg.stDst = *pstDst;
    ive_arg.stCtrl = *pstCtrl;
    ive_arg.bInstant = bInstant;

    ioctl_arg.input_data = (CVI_U64)&ive_arg;
    bm_status_t ret = (bm_status_t) ioctl(p->dev_fd, CVI_IVE_IOC_MD, &ioctl_arg);

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
    IVE_HANDLE pIveHandle = NULL;
    IVE_SRC_IMAGE_S src1, src2;
    IVE_DST_IMAGE_S dst;
    IVE_FRAME_DIFF_MOTION_CTRL_S ctrl;

    memset(&src1, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&src2, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&dst, 0, sizeof(IVE_DST_IMAGE_S));
    memset(&ctrl, 0, sizeof(IVE_FRAME_DIFF_MOTION_CTRL_S));


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
    ctrl.enSubMode = attr.sub_mode;
    ctrl.enThrMode = (IVE_THRESH_MODE_E)thransform_pattern(attr.thr_mode);
    ctrl.u8ThrLow = attr.u8_thr_low;
    ctrl.u8ThrHigh = attr.u8_thr_high;
    ctrl.u8ThrMinVal = attr.u8_thr_min_val;
    ctrl.u8ThrMidVal = attr.u8_thr_mid_val;
    ctrl.u8ThrMaxVal = attr.u8_thr_max_val;

    memcpy(&ctrl.au8ErodeMask, &attr.au8_erode_mask, 25 * sizeof(unsigned char));
    memcpy(&ctrl.au8DilateMask, &attr.au8_dilate_mask, 25 * sizeof(unsigned char));

    // get ive handle and invoke ive funtion
    pIveHandle = BM_IVE_CreateHandle();
    if (pIveHandle == NULL) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "failed to create ive handle %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        return BM_ERR_FAILURE;
    }

    ret = BM_IVE_FrameDiffMotion(pIveHandle, &src1, &src2, &dst, &ctrl, true);
    if(ret != BM_SUCCESS){
         bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR,
                "BM_IVE_FrameDiffMotion error %s: %s: %d\n",
                filename(__FILE__), __func__, __LINE__);
        BM_IVE_DestroyHandle(pIveHandle);
        return BM_ERR_FAILURE;
    }

    BM_IVE_DestroyHandle(pIveHandle);

    return ret;
}