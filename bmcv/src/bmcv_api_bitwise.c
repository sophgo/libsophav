#include "bmcv_common.h"
#include "bmcv_internal.h"

enum operation{
    AND = 7,
    OR = 8,
    XOR = 9,
};

static bm_status_t bmcv_bitwise_check(bm_handle_t handle, bm_image input1,
                                    bm_image input2, bm_image output)
{
    if (handle == NULL) {
        bmlib_log("BITWISE", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
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
        bmlib_log("BITWISE", BMLIB_LOG_ERROR, "input and output image format is NOT same");
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
        bmlib_log("BITWISE", BMLIB_LOG_ERROR, "Not supported image format");
        return BM_NOT_SUPPORTED;
    }
    if (src1_type != DATA_TYPE_EXT_1N_BYTE ||
        src2_type != DATA_TYPE_EXT_1N_BYTE ||
        dst_type != DATA_TYPE_EXT_1N_BYTE) {
        bmlib_log("BITWISE", BMLIB_LOG_ERROR, "Not supported image data type");
        return BM_NOT_SUPPORTED;
    }
    if (src1_h != src2_h || src1_w != src2_w ||
        src1_h != dst_h || src1_w != dst_w) {
        bmlib_log("BITWISE", BMLIB_LOG_ERROR, "inputs and output image size should be same");
        return BM_ERR_PARAM;
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_image_bitwise(bm_handle_t handle, bm_image input1,
                                bm_image input2, bm_image output, int op)
{
    bm_status_t ret = BM_SUCCESS;
    int if_core0 = 1, if_core1 = 0;
    const char *tpu_env = NULL;

    ret = bmcv_bitwise_check(handle, input1, input2, output);
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
    // construct and send api
    int channel = bm_image_get_plane_num(input1);
    int input1_str[3], input2_str[3], output_str[3];
    bm_image_get_stride(input1, input1_str);
    bm_image_get_stride(input2, input2_str);
    bm_image_get_stride(output, output_str);
    bm_device_mem_t input1_mem[3];
    bm_image_get_device_mem(input1, input1_mem);
    bm_device_mem_t input2_mem[3];
    bm_image_get_device_mem(input2, input2_mem);
    bm_device_mem_t output_mem[3];
    bm_image_get_device_mem(output, output_mem);
    bm_api_cv_bitwise_t api;
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
    // if rgb-planar, its channel is 3
    if (input1.image_format == FORMAT_RGB_PLANAR ||
        input1.image_format == FORMAT_BGR_PLANAR) {
        api.height[0] *= 3;
    }
    api.op = op;
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

    switch(chipid) {
        case BM1688_PREV:
        case BM1688:
            tpu_env = getenv("TPU_CORES");
            if (tpu_env) {
                if (strcmp(tpu_env, "0") == 0) {
                } else if (strcmp(tpu_env, "1") == 0) {
                    if_core0 = 0;
                    if_core1 = 1;
                } else if (strcmp(tpu_env, "2") == 0 || strcmp(tpu_env, "both") == 0) {
                    if_core1 = 1;
                } else {
                    fprintf(stderr, "Invalid TPU_CORES value: %s\n", tpu_env);
                    fprintf(stderr, "Available options: 0, 1, 2/both\n");
                    exit(EXIT_FAILURE);
                }
            }

            if (if_core0 && if_core1) {
                int core_list[BM1688_MAX_CORES];
                int core_nums = 0;
                int base_msg_id = 0;

                sg_api_cv_bitwise_dual_core_t dual_api;
                sg_api_cv_bitwise_dual_core_t dual_params[BM1688_MAX_CORES];
                tpu_launch_param_t tpu_params[BM1688_MAX_CORES];

                if (if_core0) core_list[core_nums++] = 0;
                if (if_core1) core_list[core_nums++] = 1;

                dual_api.channel = api.channel;
                dual_api.op = api.op;

                for (int c = 0; c < api.channel; c++) {
                    dual_api.input1_addr[c] = api.input1_addr[c];
                    dual_api.input1_str[c] = api.input1_str[c];
                    dual_api.input2_addr[c] = api.input2_addr[c];
                    dual_api.input2_str[c] = api.input2_str[c];
                    dual_api.output_addr[c] = api.output_addr[c];
                    dual_api.output_str[c] = api.output_str[c];

                    dual_api.width[c] = api.width[c];
                    dual_api.height[c] = api.height[c];
                }

                for (int n = 0; n < core_nums; n++)
                    base_msg_id |= 1 << core_list[n];

                dual_api.core_num = core_nums;
                dual_api.base_msg_id = base_msg_id;

                for (int n = 0; n < core_nums; n++) {
                    dual_api.core_id = n;

                    dual_params[n] = dual_api;

                    tpu_params[n].core_id = n;
                    tpu_params[n].param_data = &dual_params[n];
                    tpu_params[n].param_size = sizeof(sg_api_cv_bitwise_dual_core_t);
                }

                ret = bm_tpu_kernel_launch_dual_core(handle, "cv_bitwise_dual_core", tpu_params, core_list, core_nums);
                if (ret != BM_SUCCESS) {
                    bmlib_log("BITWISE", BMLIB_LOG_ERROR, "bitwise sync api error\n");
                    return ret;
                }

            } else {
                int core_id = if_core1 == 1 ? 1 : 0;
                ret = bm_tpu_kernel_launch(handle, "cv_bitwise", (u8*)&api, sizeof(api), core_id);
                if(BM_SUCCESS != ret){
                    bmlib_log("BITWISE", BMLIB_LOG_ERROR, "bitwise sync api error\n");
                    if (output_alloc_flag) {
                        for (int i = 0; i < channel; i++) {
                            bm_free_device(handle, output_mem[i]);
                        }
                    }
                    return ret;
                }
            }
            break;
        default:
            printf("BM_NOT_SUPPORTED!\n");
            ret = BM_NOT_SUPPORTED;
            break;
    }
    return ret;
}

bm_status_t bmcv_image_bitwise_and(bm_handle_t handle, bm_image input1,
                                    bm_image input2, bm_image output)
{
    return bmcv_image_bitwise(handle, input1, input2, output, AND);
}

bm_status_t bmcv_image_bitwise_or(bm_handle_t handle, bm_image input1,
                                    bm_image input2, bm_image output)
{
    return bmcv_image_bitwise(handle, input1, input2, output, OR);
}

bm_status_t bmcv_image_bitwise_xor(bm_handle_t handle, bm_image input1,
                                    bm_image input2, bm_image output)
{
    return bmcv_image_bitwise(handle, input1, input2, output, XOR);
}