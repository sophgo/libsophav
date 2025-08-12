#include "bmcv_common.h"
#include "bmcv_internal.h"
bm_status_t bmcv_cmulp_check(bm_handle_t handle, int batch, int len, int if_core0, int if_core1) {
    if (handle == NULL) {
        bmlib_log("CMULP", BMLIB_LOG_ERROR, "Can not get handle!\n");
        return BM_ERR_PARAM;
    }
    if (if_core0 == 1 && if_core1 == 1 && batch < 2) {
        bmlib_log("CMULP", BMLIB_LOG_ERROR, "When use dual core process, batch size should be greater than 1!\n");
        return BM_ERR_PARAM;
    }
    if (len > 4096) {
        bmlib_log("CMULP", BMLIB_LOG_ERROR, "len should not be greater than 4096!\n");
        return BM_ERR_PARAM;
    }
    if (batch > 1980) {
        bmlib_log("CMULP", BMLIB_LOG_ERROR, "batch should not be greater than 1980!\n");
        return BM_ERR_PARAM;
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_cmulp(bm_handle_t handle, bm_device_mem_t inputReal, bm_device_mem_t inputImag,
                       bm_device_mem_t pointReal, bm_device_mem_t pointImag,
                       bm_device_mem_t outputReal, bm_device_mem_t outputImag,
                       int batch, int len) {
    bm_status_t ret = BM_SUCCESS;
    bm_api_cv_cmulp_t api;
    sg_api_cv_cmulp_dual_core_t api_dual_core;
    unsigned int chipid;
    if (bm_get_chipid(handle, &chipid) != BM_SUCCESS) {
        bmlib_log("CMULP", BMLIB_LOG_ERROR, "get chipid error!\n");
        return BM_ERR_FAILURE;
    }
    int core_id = 0;
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
    ret = bmcv_cmulp_check(handle, batch, len, if_core0, if_core1);
    if (ret != BM_SUCCESS) {
        bmlib_log("CMULP", BMLIB_LOG_ERROR, "bmcv_cmulp_check failed!\r\n");
    }
    if (if_core0 == 1 && if_core1 == 1) {
        api_dual_core.XR = bm_mem_get_device_addr(inputReal);
        api_dual_core.XI = bm_mem_get_device_addr(inputImag);
        api_dual_core.PR = bm_mem_get_device_addr(pointReal);
        api_dual_core.PI = bm_mem_get_device_addr(pointImag);
        api_dual_core.YR = bm_mem_get_device_addr(outputReal);
        api_dual_core.YI = bm_mem_get_device_addr(outputImag);
        api_dual_core.batch = batch;
        api_dual_core.len = len;
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
        sg_api_cv_cmulp_dual_core_t params[BM1688_MAX_CORES];
        for (int core_idx = 0; core_idx < core_nums; core_idx++) {
            api_dual_core.core_id = core_idx;
            params[core_idx] = api_dual_core;
            launch_params[core_idx].core_id = core_list[core_idx];
            launch_params[core_idx].param_data = &params[core_idx];
            launch_params[core_idx].param_size = sizeof(sg_api_cv_cmulp_dual_core_t);
        }
        switch(chipid) {
            case BM1688_PREV:
            case BM1688:
                ret = bm_tpu_kernel_launch_dual_core(handle, "cv_cmulp_bm1688", launch_params, core_list, core_nums);
                if(ret != BM_SUCCESS){
                    bmlib_log("CMULP", BMLIB_LOG_ERROR, "cv_cmulp sync api error\n");
                    return ret;
                }
                break;
            default:
                printf("BM_NOT_SUPPORTED!\n");
                ret = BM_NOT_SUPPORTED;
                break;
        }
    } else {
        api.XR = bm_mem_get_device_addr(inputReal);
        api.XI = bm_mem_get_device_addr(inputImag);
        api.PR = bm_mem_get_device_addr(pointReal);
        api.PI = bm_mem_get_device_addr(pointImag);
        api.YR = bm_mem_get_device_addr(outputReal);
        api.YI = bm_mem_get_device_addr(outputImag);
        api.batch = batch;
        api.len = len;
        switch(chipid) {
            case BM1688_PREV:
            case BM1688:
                ret = bm_tpu_kernel_launch(handle, "cv_cmulp", (u8*)&api, sizeof(api), core_id);
                if(ret != BM_SUCCESS){
                    bmlib_log("CMULP", BMLIB_LOG_ERROR, "cv_cmulp sync api error\n");
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