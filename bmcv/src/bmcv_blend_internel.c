#ifndef BM_PCIE_MODE
#include <stdio.h>
#include "bmcv_internal.h"
#include "bmcv_a2_blend_ext.h"
#define BLEND_STRIDE_ALIGN 16

bm_status_t bm_blend_image_calc_stride(bm_handle_t handle,
                                     int img_h,
                                     int img_w,
                                     bm_image_format_ext image_format,
                                     bm_image_data_format_ext data_type,
                                     int *stride)
{
    bm_status_t ret = BM_SUCCESS;
    bm_image_private image_private;
    ret = fill_default_image_private(&image_private, img_h, img_w, image_format, data_type);
    for(int i = 0; i < image_private.plane_num; i++)
        stride[i] = ALIGN(image_private.memory_layout[i].pitch_stride, BLEND_STRIDE_ALIGN);
    if(image_format == FORMAT_YUV420P || image_format == FORMAT_YUV422P)
        stride[0] = stride[1] * 2;
    return ret;
}

bm_status_t bmcv_blending(
    bm_handle_t handle,
    int input_num,
    bm_image* input,
    bm_image output,
    struct stitch_param stitch_config)
{
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid = BM1688;
    ret = bm_get_chipid(handle, &chipid);

    if(ret != BM_SUCCESS) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "Blend get chipid error! \n");
        return ret;
    }

    switch(chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bmcv_blending_internal(handle, input_num, input, output, stitch_config);
            if(ret != BM_SUCCESS){
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bmcv_blending execution failed! \n");
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
#endif
