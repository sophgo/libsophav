#include <stdio.h>
#include "bmcv_internal.h"
#include "bmcv_common.h"

static bm_status_t bmcv_hamming_distance_check(bm_handle_t handle, int bits_len, int input1_num, int input2_num, int if_core0, int if_core1) {
    if (handle == NULL) {
        bmlib_log("HAMMING_DISTANCE", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
        return BM_ERR_PARAM;
    }
    if (bits_len != 4 && bits_len != 8 && bits_len != 16 && bits_len != 32) {
        bmlib_log("HAMMING_DISTANCE", BMLIB_LOG_ERROR, "bits_len should be 4, 8, 16 or 32!\n");
        return BM_ERR_PARAM;
    }
    if (input1_num <= 0 || input1_num > 16) {
        bmlib_log("HAMMING_DISTANCE", BMLIB_LOG_ERROR, "input1_num should be 1~16!\n");
        return BM_ERR_PARAM;
    }
    if (input2_num <= 1 || input2_num > 50000000) {
        bmlib_log("HAMMING_DISTANCE", BMLIB_LOG_ERROR, "input2_num should be 2~50000000!\n");
        return BM_ERR_PARAM;
    }
    if (if_core0 == 0 && if_core1 == 0) {
        bmlib_log("HAMMING_DISTANCE", BMLIB_LOG_ERROR, "TPU core0 and core1 at least one should be used!\n");
        return BM_ERR_PARAM;
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_hamming_distance(bm_handle_t handle, bm_device_mem_t input1, bm_device_mem_t input2,
                                  bm_device_mem_t output, int bits_len, int input1_num, int input2_num) {
    bm_status_t ret = BM_SUCCESS;
    int if_core0 = 1;
    int if_core1 = 0;
    bm_api_cv_hamming_distance_t api;
    sg_api_hamming_distance_dual_core_t api_dual_core;
    tpu_launch_param_t launch_params[BM1688_MAX_CORES];

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
    unsigned int chipid;
    if (bm_get_chipid(handle, &chipid) != BM_SUCCESS) {
        printf("get chipid is error !\n");
        return BM_ERR_FAILURE;
    }

    ret = bmcv_hamming_distance_check(handle, bits_len, input1_num, input2_num, if_core0, if_core1);
    if (BM_SUCCESS != ret) {
        printf("bmcv_hamming_distance_check error !\n");
        return ret;
    }
    if (if_core0 == 1 && if_core1 == 1) {
        int base_msg_id = 0;
        int core_list[BM1688_MAX_CORES];
        int core_nums = 0;
        if (if_core0) core_list[core_nums++] = 0;
        if (if_core1) core_list[core_nums++] = 1;
        api_dual_core.input_query_global_offset = bm_mem_get_device_addr(input1);
        api_dual_core.input_database_global_offset = bm_mem_get_device_addr(input2);
        api_dual_core.output_global_offset = bm_mem_get_device_addr(output);
        api_dual_core.bits_len = bits_len;
        api_dual_core.left_row = input1_num;
        api_dual_core.right_row = input2_num;
        api_dual_core.core_num = core_nums;
        for (int i = 0; i < core_nums; i++) {
            base_msg_id |= 1 << core_list[i];
        }
        api_dual_core.base_msg_id = base_msg_id;
        api_dual_core.core_num = core_nums;
        sg_api_hamming_distance_dual_core_t hamming_params[BM1688_MAX_CORES];
        for (int core_idx = 0; core_idx < core_nums; core_idx++) {
            api_dual_core.core_id = core_idx;
            hamming_params[core_idx] = api_dual_core;
            launch_params[core_idx].core_id = core_list[core_idx];
            launch_params[core_idx].param_data = &hamming_params[core_idx];
            launch_params[core_idx].param_size = sizeof(sg_api_hamming_distance_dual_core_t);
        }
        switch (chipid) {
            case BM1688_PREV:
            case BM1688:
                ret = bm_tpu_kernel_launch_dual_core(handle, "cv_hamming_distance_bm1688", launch_params, core_list, core_nums);
                if(BM_SUCCESS != ret) {
                    bmlib_log("HM_DISTANCE", BMLIB_LOG_ERROR, "hm_distance sync api error\n");
                    return ret;
                }
                break;
            default:
                ret = BM_NOT_SUPPORTED;
                printf("BM_NOT_SUPPORTED!\n");
                break;
        }
    } else {
        int core_id = 0;
        api.input_query_global_offset = bm_mem_get_device_addr(input1);
        api.input_database_global_offset = bm_mem_get_device_addr(input2);
        api.output_global_offset = bm_mem_get_device_addr(output);
        api.bits_len = bits_len;
        api.left_row = input1_num;
        api.right_row = input2_num;
        if(if_core1 == 1) {
            core_id = 1;
        }
        switch (chipid) {
            case BM1688_PREV:
            case BM1688:
                ret = bm_tpu_kernel_launch(handle, "cv_hanming_distance_1684x", (u8 *)&api, sizeof(api), core_id);
                if(BM_SUCCESS != ret) {
                    bmlib_log("HM_DISTANCE", BMLIB_LOG_ERROR, "hm_distance sync api error\n");
                    return ret;
                }
                break;
            default:
                ret = BM_NOT_SUPPORTED;
                printf("BM_NOT_SUPPORTED!\n");
                break;
        }
    }
    return ret;
}
