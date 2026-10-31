#include "bmcv_common.h"
#include "bmcv_internal.h"
#include "stdlib.h"

#define ONLY_MIN    0
#define ONLY_MAX    1
#define BOTH        2

static bm_status_t bmcv_min_max_check(bm_handle_t handle, int len, float *minVal, float *maxVal) {
    if (handle == NULL) {
        bmlib_log("MIN_MAX", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
        return BM_ERR_PARAM;
    }
    if (len < 0) {
        bmlib_log("min_max", BMLIB_LOG_ERROR, "Unsupported size! len_size must larger than 0!\n");
        return BM_ERR_PARAM;
    }
    if (minVal == NULL && maxVal == NULL) {
        bmlib_log("min_max", BMLIB_LOG_ERROR, "The maximum and minimum values cannot both be empty\n");
        return BM_ERR_PARAM;
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_min_max(bm_handle_t handle, bm_device_mem_t input, float* minVal, float* maxVal, int len) {
    bm_status_t ret = BM_SUCCESS;
    bm_device_mem_t minDev, maxDev, buffer_dual_core_min_addr, buffer_dual_core_max_addr;
    int core_id = 0;
    float* min_npu = NULL;
    float* max_npu = NULL;
    float* buffer_dual_core_min_npu = NULL;
    float* buffer_dual_core_max_npu = NULL;
    bm_api_cv_min_max_t api;
    sg_api_cv_min_max_dual_core_t api_dual_core;
    tpu_launch_param_t launch_params[BM1688_MAX_CORES];
    int if_core0 = 1;
    int if_core1 = 0;
    const char* tpu_env = getenv("TPU_CORES");
    if (tpu_env == NULL) {
        bmlib_log("MIN_MAX", BMLIB_LOG_DEBUG, "Use TPU core0\n");
    } else {
        if (strcmp(tpu_env, "0") == 0) {
            bmlib_log("MIN_MAX", BMLIB_LOG_DEBUG, "Use TPU core0\n");
        } else if (strcmp(tpu_env, "1") == 0) {
            bmlib_log("MIN_MAX", BMLIB_LOG_DEBUG, "Use TPU core1\n");
            if_core0 = 0;
            if_core1 = 1;
        } else if (strcmp(tpu_env, "2") == 0 || strcmp(tpu_env, "both") == 0) {
            bmlib_log("MIN_MAX", BMLIB_LOG_DEBUG, "Use all TPU cores (0 and 1)\n");
#ifndef MEDIA_V3
            if_core1 = 1;
#endif
        } else {
            bmlib_log("MIN_MAX", BMLIB_LOG_ERROR, "Invalid TPU_CORES value: %s\n", tpu_env);
            bmlib_log("MIN_MAX", BMLIB_LOG_ERROR, "Available options: 0, 1, 2/both\n");
            exit(EXIT_FAILURE);
        }
    }

    ret = bmcv_min_max_check(handle, len, minVal, maxVal);
    if(ret != BM_SUCCESS) {
        BMCV_ERR_LOG("min_max check error\r\n");
        return ret;
    }

    if (if_core0 == 1 && if_core1 == 1) {
        if (maxVal != NULL) {
            buffer_dual_core_max_npu = (float*)malloc(2 * sizeof(float));
            if (buffer_dual_core_max_npu == NULL) {
                ret = BM_ERR_NOMEM;
                goto exit;
            }
            ret = bm_malloc_device_byte(handle, &buffer_dual_core_max_addr, 2 * sizeof(float));
            if (ret != BM_SUCCESS) {
                goto exit;
            }
        }
        if (minVal != NULL) {
            buffer_dual_core_min_npu = (float*)malloc(2 * sizeof(float));
            if (buffer_dual_core_min_npu == NULL) {
                ret = BM_ERR_NOMEM;
                goto exit;
            }
            ret = bm_malloc_device_byte(handle, &buffer_dual_core_min_addr, 2 * sizeof(float));
            if (ret != BM_SUCCESS) {
                goto exit;
            }
        }
        int base_msg_id = 0;
        int core_list[BM1688_MAX_CORES];
        int core_nums = 0;
        if (if_core0) core_list[core_nums++] = 0;
        if (if_core1) core_list[core_nums++] = 1;
        api_dual_core.inputAddr = bm_mem_get_device_addr(input);
        api_dual_core.buffer_dual_core_min_addr = bm_mem_get_device_addr(minVal != NULL ? buffer_dual_core_min_addr : buffer_dual_core_max_addr);
        api_dual_core.buffer_dual_core_max_addr = bm_mem_get_device_addr(maxVal != NULL ? buffer_dual_core_max_addr : buffer_dual_core_min_addr);
        api_dual_core.len = len;
        api_dual_core.mode = maxVal == NULL ? ONLY_MIN : (minVal == NULL ? ONLY_MAX : BOTH);
        for (int i = 0; i < core_nums; i++) {
            base_msg_id |= 1 << core_list[i];
        }
        api_dual_core.base_msg_id = base_msg_id;
        api_dual_core.core_num = core_nums;
        sg_api_cv_min_max_dual_core_t params[BM1688_MAX_CORES];
        for (int core_idx = 0; core_idx < core_nums; core_idx++) {
            api_dual_core.core_id = core_idx;
            params[core_idx] = api_dual_core;
            launch_params[core_idx].core_id = core_list[core_idx];
            launch_params[core_idx].param_data = &params[core_idx];
            launch_params[core_idx].param_size = sizeof(sg_api_cv_min_max_dual_core_t);
        }
        ret = bm_tpu_kernel_launch_dual_core(handle, "cv_min_max_bm1688", launch_params, core_list, core_nums);
        if(BM_SUCCESS != ret) {
            bmlib_log("MIN_MAX", BMLIB_LOG_ERROR, "min_max sync api error\n");
            goto exit;
        }
        if (maxVal != NULL) {
            ret = bm_memcpy_d2s(handle, buffer_dual_core_max_npu, buffer_dual_core_max_addr);
            if (ret != BM_SUCCESS) {
                goto exit;
            }
            *maxVal = buffer_dual_core_max_npu[0] > buffer_dual_core_max_npu[1] ? buffer_dual_core_max_npu[0] : buffer_dual_core_max_npu[1];
        }
        if (minVal != NULL) {
            ret = bm_memcpy_d2s(handle, buffer_dual_core_min_npu, buffer_dual_core_min_addr);
            if (ret != BM_SUCCESS) {
                goto exit;
            }
            *minVal = buffer_dual_core_min_npu[0] < buffer_dual_core_min_npu[1] ? buffer_dual_core_min_npu[0] : buffer_dual_core_min_npu[1];
        }
    } else {
        if (maxVal != NULL) {
            max_npu = (float*)malloc(sizeof(float));
            if (max_npu == NULL) {
                ret = BM_ERR_NOMEM;
                goto exit;
            }
            ret = bm_malloc_device_byte(handle, &maxDev, sizeof(float));
            if (ret != BM_SUCCESS) {
                goto exit;
            }
        }
        if (minVal != NULL) {
            min_npu = (float*)malloc(sizeof(float));
            if (min_npu == NULL) {
                ret = BM_ERR_NOMEM;
                goto exit;
            }
            ret = bm_malloc_device_byte(handle, &minDev, sizeof(float));
            if (ret != BM_SUCCESS) {
                goto exit;
            }
        }
        api.inputAddr = bm_mem_get_device_addr(input);
        api.minAddr = bm_mem_get_device_addr(minVal != NULL ? minDev : maxDev);
        api.maxAddr = bm_mem_get_device_addr(maxVal != NULL ? maxDev : minDev);
        api.len = len;
        api.mode = maxVal == NULL ? ONLY_MIN : (minVal == NULL ? ONLY_MAX : BOTH);
        ret = bm_tpu_kernel_launch(handle, "cv_min_max", (u8*)&api, sizeof(api), core_id);
        if(BM_SUCCESS != ret){
            bmlib_log("MIN_MAX", BMLIB_LOG_ERROR, "min_max sync api error\n");
            goto exit;
        }
        if (maxVal != NULL) {
            ret = bm_memcpy_d2s(handle, max_npu, maxDev);
            if (ret != BM_SUCCESS) {
                goto exit;
            }
            *maxVal = *max_npu;
        }
        if (minVal != NULL) {
            ret = bm_memcpy_d2s(handle, min_npu, minDev);
            if (ret != BM_SUCCESS) {
                goto exit;
            }
            *minVal = *min_npu;
        }
    }

exit:
    if (buffer_dual_core_max_npu != NULL) {
        bm_free_device(handle, buffer_dual_core_max_addr);
        free(buffer_dual_core_max_npu);
    }
    if (buffer_dual_core_min_npu != NULL) {
        bm_free_device(handle, buffer_dual_core_min_addr);
        free(buffer_dual_core_min_npu);
    }
    if (max_npu != NULL) {
        bm_free_device(handle, maxDev);
        free(max_npu);
    }
    if (min_npu != NULL) {
        bm_free_device(handle, minDev);
        free(min_npu);
    }
    return ret;
}

bm_status_t bmcv_min_max_u64(bm_handle_t handle, bm_device_mem_u64_t input, float* minVal,
    float* maxVal, int len)
{
    bm_status_t ret = BM_SUCCESS;
    bm_device_mem_u64_t minDev, maxDev;
    uint8_t npu_num;
    int core_id = 0;
    float* min_npu = NULL;
    float* max_npu = NULL;
    bm_api_cv_min_max_t api;

    ret = bmcv_min_max_check(handle, len, minVal, maxVal);
    if(ret != BM_SUCCESS) {
        BMCV_ERR_LOG("min_max check error\r\n");
        return ret;
    }

    npu_num = 32;

    if (maxVal != NULL) {
        max_npu = (float*)malloc(npu_num * sizeof(float));
        if (max_npu == NULL) {
            ret = BM_ERR_NOMEM;
            goto exit;
        }
        ret = bm_malloc_device_byte_u64(handle, &maxDev, npu_num * sizeof(float));
        if (ret != BM_SUCCESS) {
            goto exit;
        }
    }

    if (minVal != NULL) {
        min_npu = (float*)malloc(npu_num * sizeof(float));
        if (min_npu == NULL) {
            ret = BM_ERR_NOMEM;
            goto exit;
        }
        ret = bm_malloc_device_byte_u64(handle, &minDev, npu_num * sizeof(float));
        if (ret != BM_SUCCESS) {
            goto exit;
        }
    }

    api.inputAddr = bm_mem_get_device_addr_u64(input);
    api.minAddr = bm_mem_get_device_addr_u64(minVal != NULL ? minDev : maxDev);
    api.maxAddr = bm_mem_get_device_addr_u64(maxVal != NULL ? maxDev : minDev);
    api.len = len;
    api.mode = maxVal == NULL ? ONLY_MIN : (minVal == NULL ? ONLY_MAX : BOTH);

    ret = bm_tpu_kernel_launch(handle, "cv_min_max", (u8*)&api,
                                sizeof(api), core_id);
    if (BM_SUCCESS != ret){
        bmlib_log("MIN_MAX", BMLIB_LOG_ERROR, "min_max sync api error\n");
        goto exit;
    }

    if (maxVal != NULL) {
        ret = bm_memcpy_d2s_u64(handle, max_npu, maxDev);
        if (ret != BM_SUCCESS) {
            goto exit;
        }
        *maxVal = max_npu[0];
        for (int i = 1; i < npu_num; ++i)
            *maxVal = *maxVal > max_npu[i] ? *maxVal : max_npu[i];
    }

    if (minVal != NULL) {
        ret = bm_memcpy_d2s_u64(handle, min_npu, minDev);
        if (ret != BM_SUCCESS) {
            goto exit;
        }
        *minVal = min_npu[0];
        for (int i = 1; i < npu_num; ++i)
            *minVal = *minVal < min_npu[i] ? *minVal : min_npu[i];
    }

exit:
    if (max_npu != NULL) {
        bm_free_device_u64(handle, maxDev);
        free(max_npu);
    }
    if (min_npu != NULL) {
        bm_free_device_u64(handle, minDev);
        free(min_npu);
    }
    return ret;
}
