#include "bmcv_api_ext_c.h"
#include "bmcv_internal.h"
#include "bmcv_common.h"
static bm_status_t bmcv_quantify_check(
        bm_handle_t handle,
        bm_image input,
        bm_image output) {
    if (handle == NULL) {
        bmlib_log("QUANTIFY", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
        return BM_ERR_PARAM;
    }
    bm_image_format_ext src_format = input.image_format;
    bm_image_data_format_ext src_type = input.data_type;
    bm_image_format_ext dst_format = output.image_format;
    bm_image_data_format_ext dst_type = output.data_type;
    int src_h = input.height;
    int src_w = input.width;
    int dst_h = output.height;
    int dst_w = output.width;
    if (src_format != dst_format) {
        bmlib_log("QUANTIFY", BMLIB_LOG_ERROR, "input and output image format is NOT same");
        return BM_ERR_PARAM;
    }
    if (src_format == FORMAT_RGBYP_PLANAR || src_format == FORMAT_COMPRESSED) {
        bmlib_log("Quantify", BMLIB_LOG_ERROR, "do not support image format");
        return BM_NOT_SUPPORTED;
    }
    if (src_type != DATA_TYPE_EXT_FLOAT32 ||
        dst_type != DATA_TYPE_EXT_1N_BYTE) {
        bmlib_log("QUANTIFY", BMLIB_LOG_ERROR, "Not supported image data type");
        return BM_NOT_SUPPORTED;
    }
    if (src_h != dst_h || src_w != dst_w) {
        bmlib_log("QUANTIFY", BMLIB_LOG_ERROR, "inputs and output image size should be same");
        return BM_ERR_PARAM;
    }
    if (src_h > 4096 || src_w > 4096) {
        bmlib_log("QUANTIFY", BMLIB_LOG_ERROR, "Unsupported size : size_max = 4096 x 4096 \n");
        return BM_ERR_PARAM;
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_image_quantify(
        bm_handle_t handle,
        bm_image input,
        bm_image output) {
    int core_id = 0;
    bm_status_t ret = BM_SUCCESS;
    bm_api_cv_quantify_t api;
    sg_api_cv_quantify_dual_core_t api_dual_core;
    tpu_launch_param_t launch_params[BM1688_MAX_CORES];
    int if_core0 = 1;
    int if_core1 = 0;
    const char* tpu_env = getenv("TPU_CORES");
    if (tpu_env == NULL) {
        printf("Using the default TPU core configuration: core0\n");
    } else {
        if (strcmp(tpu_env, "0") == 0) {
            printf("Use TPU core0\n");
        } else if (strcmp(tpu_env, "1") == 0) {
            printf("Use TPU core1\n");
            if_core0 = 0;
            if_core1 = 1;
        } else if (strcmp(tpu_env, "2") == 0 || strcmp(tpu_env, "both") == 0) {
            printf("Use all TPU cores (0 and 1))\n");
            if_core1 = 1;
        } else {
            fprintf(stderr, "Invalid TPU_CORES value: %s\n", tpu_env);
            fprintf(stderr, "Available options: 0, 1, 2/both\n");
            exit(EXIT_FAILURE);
        }
    }


    ret = bmcv_quantify_check(handle, input, output);
    if (BM_SUCCESS != ret) {
        return ret;
    }
    unsigned int chipid;
    ret = bm_get_chipid(handle, &chipid);
    if (ret) {
        printf("bm_get_chipid error !\n");
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
    int channel = bm_image_get_plane_num(input);
    int input_str[3], output_str[3];
    bm_image_get_stride(input, input_str);
    bm_image_get_stride(output, output_str);

    bm_device_mem_t input_mem[3];
    bm_image_get_device_mem(input, input_mem);
    bm_device_mem_t output_mem[3];
    bm_image_get_device_mem(output, output_mem);

    if (if_core0 == 1 && if_core1 == 1) {
        int base_msg_id = 0;
        int core_list[BM1688_MAX_CORES];
        int core_nums = 0;
        if (if_core0) core_list[core_nums++] = 0;
        if (if_core1) core_list[core_nums++] = 1;
        api_dual_core.channel = channel;
        for (int i = 0; i < channel; i++) {
            api_dual_core.input_addr[i] = bm_mem_get_device_addr(input_mem[i]);
            api_dual_core.output_addr[i] = bm_mem_get_device_addr(output_mem[i]);
            api_dual_core.width[i] = input.image_private->memory_layout[i].W;
            api_dual_core.height[i] = input.image_private->memory_layout[i].H;
            api_dual_core.input_str[i] = input_str[i];
            api_dual_core.output_str[i] = output_str[i];
        }
        // rgb-planar format's channel is 1
        if (input.image_format == FORMAT_RGB_PLANAR ||
            input.image_format == FORMAT_BGR_PLANAR) {
            api_dual_core.height[0] *= 3;
        }
        for (int i = 0; i < core_nums; i++) {
            base_msg_id |= 1 << core_list[i];
        }
        api_dual_core.base_msg_id = base_msg_id;
        api_dual_core.core_num = core_nums;
        sg_api_cv_quantify_dual_core_t params[BM1688_MAX_CORES];
        for (int core_idx = 0; core_idx < core_nums; core_idx++) {
            api_dual_core.core_id = core_idx;
            params[core_idx] = api_dual_core;
            launch_params[core_idx].core_id = core_list[core_idx];
            launch_params[core_idx].param_data = &params[core_idx];
            launch_params[core_idx].param_size = sizeof(sg_api_cv_quantify_dual_core_t);
        }
        switch (chipid) {
            case BM1688_PREV:
            case BM1688:
                ret = bm_tpu_kernel_launch_dual_core(handle, "cv_quantify_bm1688", launch_params, core_list, core_nums);
                if(BM_SUCCESS != ret) {
                    bmlib_log("QUANTIFY", BMLIB_LOG_ERROR, "quantify sync api error\n");
                    return ret;
                }
                break;
            default:
                ret = BM_NOT_SUPPORTED;
                printf("BM_NOT_SUPPORTED!\n");
                break;
        }
    } else {
        api.channel = channel;
        for (int i = 0; i < channel; i++) {
            api.input_addr[i] = bm_mem_get_device_addr(input_mem[i]);
            api.output_addr[i] = bm_mem_get_device_addr(output_mem[i]);
            api.width[i] = input.image_private->memory_layout[i].W;
            api.height[i] = input.image_private->memory_layout[i].H;
            api.input_str[i] = input_str[i];
            api.output_str[i] = output_str[i];
        }
        // rgb-planar format's channel is 1
        if (input.image_format == FORMAT_RGB_PLANAR ||
            input.image_format == FORMAT_BGR_PLANAR) {
            api.height[0] *= 3;
        }
        switch (chipid) {
            case BM1688_PREV:
            case BM1688:
                ret = bm_tpu_kernel_launch(handle, "cv_quantify", (u8*)&api, sizeof(api), core_id);
                if(BM_SUCCESS != ret){
                    bmlib_log("QUANTIFY", BMLIB_LOG_ERROR, "quantify sync api error\n");
                    if (output_alloc_flag) {
                        for (int i = 0; i < channel; i++) {
                            bm_free_device(handle, output_mem[i]);
                        }
                    }
                    return ret;
                }
                break;

            default:
                printf("ChipID is NOT supported\n");
                break;
        }
    }

    return ret;
}