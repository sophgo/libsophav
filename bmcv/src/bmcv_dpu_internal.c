#include <stdio.h>
#include "bmcv_internal.h"
#include "bmcv_a2_dpu_ext.h"

#define DPU_STRIDE_ALIGN 16
#define FGS_STRIDE_ALIGN 32
#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

void dpu_read_bin(const char *path, unsigned char *data, int size)
{
    FILE *fp = fopen(path, "rb");
    if (fread((void *)data, 1, size, fp) < (unsigned int)size){
        printf("file size is less than %d required bytes\n", size);
    };

    fclose(fp);
}

bm_status_t bm_dpu_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
    bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride, bool bFgsRes)
{
    bm_status_t ret = BM_SUCCESS;
    int data_size = 1;
    int stride_align = (bFgsRes) ? FGS_STRIDE_ALIGN : DPU_STRIDE_ALIGN;
    switch (data_type) {
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
    switch (image_format) {
        case FORMAT_YUV420P:
        case FORMAT_YUV422P:{
            stride[0] = align_up(img_w, stride_align) * data_size;
            stride[1] = align_up(img_w>>1, stride_align) * data_size;
            stride[2] = align_up(img_w>>1, stride_align) * data_size;
            break;
        }
        case FORMAT_YUV444P:
        case FORMAT_BGRP_SEPARATE:
        case FORMAT_RGBP_SEPARATE:
        case FORMAT_HSV_PLANAR:{
            stride[0] = align_up(img_w, stride_align) * data_size;
            stride[1] = align_up(img_w, stride_align) * data_size;
            stride[2] = align_up(img_w, stride_align) * data_size;
            break;
        }
        case FORMAT_NV24:
        case FORMAT_NV12:
        case FORMAT_NV21:
        case FORMAT_NV16:
        case FORMAT_NV61: {
            stride[0] = align_up(img_w, stride_align) * data_size;
            stride[1] = align_up(img_w, stride_align) * data_size;
            break;
        }
        case FORMAT_GRAY:
        case FORMAT_BGR_PLANAR:
        case FORMAT_RGB_PLANAR:{
            stride[0] = align_up(img_w, stride_align) * data_size;
            break;
        }
        case FORMAT_COMPRESSED:
        case FORMAT_RGBYP_PLANAR:{
            stride[0] = align_up(img_w, stride_align) * data_size;
            stride[1] = align_up(img_w, stride_align) * data_size;
            stride[2] = align_up(img_w, stride_align) * data_size;
            stride[3] = align_up(img_w, stride_align) * data_size;
            break;
        }
        case FORMAT_YUV444_PACKED:
        case FORMAT_YVU444_PACKED:
        case FORMAT_HSV180_PACKED:
        case FORMAT_HSV256_PACKED:
        case FORMAT_BGR_PACKED:
        case FORMAT_RGB_PACKED: {
            stride[0] = align_up(img_w*3, stride_align) * data_size;
            break;
        }
        case FORMAT_ABGR_PACKED:
        case FORMAT_ARGB_PACKED: {
            stride[0] = align_up(img_w*4, stride_align) * data_size;
            break;
        }
        case FORMAT_BAYER:
        case FORMAT_YUV422_YUYV:
        case FORMAT_YUV422_YVYU:
        case FORMAT_YUV422_UYVY:
        case FORMAT_YUV422_VYUY:
        case FORMAT_ARGB4444_PACKED:
        case FORMAT_ABGR4444_PACKED:
        case FORMAT_ARGB1555_PACKED:
        case FORMAT_ABGR1555_PACKED:{
            stride[0] = align_up(img_w*2, stride_align) * data_size;
            break;
        }
        default:
            printf("image format not supported \n");
            ret = BM_NOT_SUPPORTED;
            break;
    }
    return ret;
}

bm_status_t bmcv_dpu_sgbm_disp( bm_handle_t          handle,
                                bm_image             *left_image,
                                bm_image             *right_image,
                                bm_image             *disp_image,
                                bmcv_dpu_sgbm_attrs  *dpu_attr,
                                bmcv_dpu_sgbm_mode   sgbm_mode){
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
    ret = bm_get_chipid(handle, &chipid);

    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dpu get chipid error! \n");
        return ret;
    }

    switch(chipid){
        case BM1688:
            ret = bm_dpu_sgbm_disp_internal(handle, left_image, right_image, disp_image, dpu_attr, sgbm_mode);
            if(ret != BM_SUCCESS){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Bmcv dpu execution failed! \n");
                return ret;
            }
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "not support! \n");
            break;
    }
    return ret;
}

bm_status_t bmcv_dpu_fgs_disp( bm_handle_t           handle,
                               bm_image             *guide_image,
                               bm_image             *smooth_image,
                               bm_image             *disp_image,
                               bmcv_dpu_fgs_attrs   *fgs_attr,
                               bmcv_dpu_fgs_mode    fgs_mode){
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
    ret = bm_get_chipid(handle, &chipid);

    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dpu get chipid error! \n");
        return ret;
    }

    switch(chipid){
        case BM1688:
            ret = bm_dpu_fgs_disp_internal(handle, guide_image, smooth_image, disp_image, fgs_attr, fgs_mode);
            if(ret != BM_SUCCESS){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Bmcv dpu execution failed! \n");
                return ret;
            }
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "not support! \n");
            break;
    }
    return ret;
}

bm_status_t bmcv_dpu_online_disp( bm_handle_t         handle,
                                  bm_image            *left_image,
                                  bm_image            *right_image,
                                  bm_image            *disp_image,
                                  bmcv_dpu_sgbm_attrs *dpu_attr,
                                  bmcv_dpu_fgs_attrs  *fgs_attr,
                                  bmcv_dpu_online_mode online_mode){
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
    ret = bm_get_chipid(handle, &chipid);

    if(ret != BM_SUCCESS){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Dpu get chipid error! \n");
        return ret;
    }

    switch(chipid){
        case BM1688:
            ret = bm_dpu_online_disp_internal(handle, left_image, right_image, disp_image, dpu_attr, fgs_attr, online_mode);
            if(ret != BM_SUCCESS){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Bmcv dpu execution failed! \n");
                return ret;
            }
            break;
        default:
            ret = BM_NOT_SUPPORTED;
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "not support! \n");
            break;
    }

    return ret;
}
