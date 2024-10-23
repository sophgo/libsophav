#include "bmcv_common.h"
#include "bmcv_internal.h"

static bm_status_t bmcv_add_weighted_check(bm_handle_t handle, bm_image input1,
                                            bm_image input2, bm_image output)
{
    if (handle == NULL) {
        bmlib_log("ADD_WEIGHTED", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
        return BM_ERR_PARAM;
    }

    bm_image_format_ext src1_format = input1.image_format;
    bm_image_data_format_ext src1_type = input1.data_type;
    bm_image_format_ext src2_format = input2.image_format;
    bm_image_data_format_ext src2_type = input2.data_type;
    bm_image_format_ext dst_format = output.image_format;
    bm_image_data_format_ext dst_type = output.data_type;
    int src1_h = input1.height;
    int src1_w = input1.width;
    int src2_h = input2.height;
    int src2_w = input2.width;
    int dst_h = output.height;
    int dst_w = output.width;
    if (src1_format != src2_format && src1_format != dst_format) {
        bmlib_log("ADD_WEIGHTED", BMLIB_LOG_ERROR, "input and output image format is NOT same");
        return BM_ERR_PARAM;
    }
    if (src1_format != FORMAT_YUV420P &&
        src1_format != FORMAT_YUV422P &&
        src1_format != FORMAT_YUV444P &&
        src1_format != FORMAT_NV12 &&
        src1_format != FORMAT_NV21 &&
        src1_format != FORMAT_NV61 &&
        src1_format != FORMAT_NV16 &&
        src1_format != FORMAT_NV24 &&
        src1_format != FORMAT_GRAY &&
        src1_format != FORMAT_RGB_PLANAR &&
        src1_format != FORMAT_BGR_PLANAR &&
        src1_format != FORMAT_RGB_PACKED &&
        src1_format != FORMAT_BGR_PACKED &&
        src1_format != FORMAT_RGBP_SEPARATE &&
        src1_format != FORMAT_BGRP_SEPARATE) {
        bmlib_log("ADD_WEIGHTED", BMLIB_LOG_ERROR, "Not supported image format");
        return BM_NOT_SUPPORTED;
    }
    if (src1_type != DATA_TYPE_EXT_1N_BYTE ||
        src2_type != DATA_TYPE_EXT_1N_BYTE ||
        dst_type != DATA_TYPE_EXT_1N_BYTE) {
        bmlib_log("ADD_WEIGHTED", BMLIB_LOG_ERROR, "Not supported image data type");
        return BM_NOT_SUPPORTED;
    }
    if (src1_h != src2_h || src1_w != src2_w ||
        src1_h != dst_h || src1_w != dst_w) {
        bmlib_log("ADD_WEIGHTED", BMLIB_LOG_ERROR, "inputs and output image size should be same");
        return BM_ERR_PARAM;
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_image_add_weighted(bm_handle_t handle, bm_image input1, float alpha,
                                    bm_image input2, float beta,
                                    float gamma, bm_image output)
{
    bm_status_t ret = BM_SUCCESS;
    ret = bmcv_add_weighted_check(handle, input1, input2, output);
    if (BM_SUCCESS != ret) {
        return ret;
    }
    bool output_alloc_flag = false;
    if (!bm_image_is_attached(output)) {
        ret = bm_image_alloc_dev_mem(output, BMCV_HEAP1_ID);
        if (ret != BM_SUCCESS) {
            return ret;
        }
        output_alloc_flag = true;
    }
    /* construct and send api*/
    int channel = bm_image_get_plane_num(input1);
    int input1_str[3], input2_str[3], output_str[3];
    ret = bm_image_get_stride(input1, input1_str);
    if (ret) {
        printf("bm_img_get_stride input1 failed!\n");
        return ret;
    }
    ret = bm_image_get_stride(input2, input2_str);
    if (ret) {
        printf("bm_img_get_stride input1 failed!\n");
        return ret;
    }
    ret = bm_image_get_stride(output, output_str);
    if (ret) {
        printf("bm_img_get_stride input1 failed!\n");
        return ret;
    }

    bm_device_mem_t input1_mem[3];
    if (bm_image_get_device_mem(input1, input1_mem) != BM_SUCCESS) {
        printf("get input1 device mem error \n");
    }
    bm_device_mem_t input2_mem[3];
    if (bm_image_get_device_mem(input2, input2_mem) != BM_SUCCESS) {
        printf("get input2 device mem error \n");
    }
    bm_device_mem_t output_mem[3];
    if (bm_image_get_device_mem(output, output_mem) != BM_SUCCESS) {
        printf("get output device mem error \n");
    }
    bm_api_cv_add_weighted_t api;
    api.channel = channel;
    for (int i = 0; i < channel; i++) {
        api.input1_addr[i] = bm_mem_get_device_addr(input1_mem[i]);
        api.input2_addr[i] = bm_mem_get_device_addr(input2_mem[i]);
        api.output_addr[i] = bm_mem_get_device_addr(output_mem[i]);
        api.width[i] = input1.image_private->memory_layout[i].W;
        api.height[i] = input1.image_private->memory_layout[i].H;
        api.input1_str[i] = input1_str[i];
        api.input2_str[i] = input2_str[i];
        api.output_str[i] = output_str[i];
    }
    /*if rgb-planar, its channel is 3 */
    if (input1.image_format == FORMAT_RGB_PLANAR ||
        input1.image_format == FORMAT_BGR_PLANAR) {
        api.height[0] *= 3;
    }
    //prevent packed images from being judeged by assert
    if (input1.image_format == FORMAT_RGB_PACKED ||
        input1.image_format == FORMAT_BGR_PACKED) {
        api.height[0] *= 3;
        api.width[0] /= 3;
        api.input1_str[0] /= 3;
        api.input2_str[0] /= 3;
        api.output_str[0] /= 3;
    }
    api.alpha = alpha;
    api.beta = beta;
    api.gamma = gamma;

    unsigned int chipid;
    if (bm_get_chipid(handle, &chipid) != BM_SUCCESS) {
        printf("get chipid is error !\n");
        if (output_alloc_flag) {
            for (int i = 0; i < channel; i++) {
                bm_free_device(handle, output_mem[i]);
            }
        }
        return BM_ERR_FAILURE;
    }

    int core_id = 0;
    switch(chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "cv_add_weighted", (u8 *)&api, sizeof(api), core_id);
            if(BM_SUCCESS != ret){
                bmlib_log("ADD_WEIGHTED", BMLIB_LOG_ERROR, "add_weighted sync api error\n");
                if (output_alloc_flag) {
                    for (int i = 0; i < channel; i++) {
                        bm_free_device(handle, output_mem[i]);
                    }
                }
                return ret;
            }
            break;
        default:
            printf("BM_NOT_SUPPORTED!\n");
            ret = BM_NOT_SUPPORTED;
            break;
    }
    return ret;
}