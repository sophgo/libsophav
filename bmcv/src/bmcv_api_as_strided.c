#include "bmcv_common.h"
#include "bmcv_internal.h"

bm_status_t bmcv_as_strided(bm_handle_t handle, bm_device_mem_t input, bm_device_mem_t output,
                            int input_row, int input_col, int output_row,
                            int output_col, int row_stride, int col_stride) {
    bm_status_t ret = BM_SUCCESS;
    bm_api_cv_as_strided_t api;
    unsigned int chipid;
    ret = bm_get_chipid(handle, &chipid);
    if (ret != BM_SUCCESS) {
        bmlib_log("AS_STRIDED", BMLIB_LOG_ERROR, "Can not get chipid!\n");
        return BM_ERR_FAILURE;
    }
    int core_id = 0;
    sg_api_cv_as_strided_dual_core_t api_dual_core;
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
            core_id = 1;
        } else if (strcmp(tpu_env, "2") == 0 || strcmp(tpu_env, "both") == 0) {
            printf("Use all TPU cores (0 and 1))\n");
            if_core1 = 1;
        } else {
            fprintf(stderr, "Invalid TPU_CORES value: %s\n", tpu_env);
            fprintf(stderr, "Available options: 0, 1, 2/both\n");
            exit(EXIT_FAILURE);
        }
    }
    if (handle == NULL) {
        bmlib_log("AS_STRIDED", BMLIB_LOG_ERROR, "Can not get handle!\n");
        return BM_ERR_FAILURE;
    }
    if (if_core0 == 1 && if_core1 == 1) {
        api_dual_core.input_addr = bm_mem_get_device_addr(input);
        api_dual_core.output_addr = bm_mem_get_device_addr(output);
        api_dual_core.input_row = input_row;
        api_dual_core.input_col = input_col;
        api_dual_core.output_row = output_row;
        api_dual_core.output_col = output_col;
        api_dual_core.row_stride = row_stride;
        api_dual_core.col_stride = col_stride;
        int base_msg_id = 0;
        int core_list[BM1688_MAX_CORES];
        int core_nums = 0;
        if (if_core0) core_list[core_nums++] = 0;
        if (if_core1) core_list[core_nums++] = 1;
        for (int i = 0; i < core_nums; i++) {
            base_msg_id |= 1 << core_list[i];
        }
        api_dual_core.base_msg_id = base_msg_id;
        api_dual_core.core_num = core_nums;
        sg_api_cv_as_strided_dual_core_t params[BM1688_MAX_CORES];
        for (int core_idx = 0; core_idx < core_nums; core_idx++) {
            api_dual_core.core_id = core_idx;
            params[core_idx] = api_dual_core;
            launch_params[core_idx].core_id = core_list[core_idx];
            launch_params[core_idx].param_data = &params[core_idx];
            launch_params[core_idx].param_size = sizeof(sg_api_cv_as_strided_dual_core_t);
        }
        switch(chipid) {
            case BM1688_PREV:
            case BM1688:
                ret = bm_tpu_kernel_launch_dual_core(handle, "cv_as_strided_bm1688", launch_params, core_list, core_nums);
                if (ret != BM_SUCCESS) {
                    bmlib_log("AS_STRIDED", BMLIB_LOG_ERROR, "as_strided sync api error\n");
                    return ret;
                }
                break;
            default:
                printf("BM_NOT_SUPPORTED!\n");
                ret = BM_NOT_SUPPORTED;
                break;
        }
    } else {
        api.input_addr = bm_mem_get_device_addr(input);
        api.output_addr = bm_mem_get_device_addr(output);
        api.input_row = input_row;
        api.input_col = input_col;
        api.output_row = output_row;
        api.output_col = output_col;
        api.row_stride = row_stride;
        api.col_stride = col_stride;
        switch(chipid) {
            case BM1688_PREV:
            case BM1688:
                ret = bm_tpu_kernel_launch(handle, "cv_as_strided", (u8 *)&api, sizeof(api), core_id);
                if (ret != BM_SUCCESS) {
                    bmlib_log("AS_STRIDED", BMLIB_LOG_ERROR, "as_strided sync api error\n");
                    return ret;
                }
                break;
            default:
                printf("BM_NOT_SUPPORTED!\n");
                ret = BM_NOT_SUPPORTED;
                break;
        }
    }
    return ret;
}
