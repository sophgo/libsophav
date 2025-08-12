

#include <stdint.h>
#include <stdio.h>
#include "bmcv_internal.h"
#include "bmcv_common.h"

static bm_status_t bmcv_sort_dual_check_check(bm_handle_t handle, int data_cnt, int sort_cnt) {
    if(data_cnt < (sort_cnt * 2)) {
        bmlib_log("BMCV", BMLIB_LOG_ERROR, "When using dual-core computation, data_cnt should be greater than (sort_cnt * 2)\n");
        return BM_ERR_PARAM;
    }
    return BM_SUCCESS;
}
static bm_status_t bmcv_sort_check(bm_handle_t handle, int data_cnt, int sort_cnt) {
    if (handle == NULL) {
        bmlib_log("SORT", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
        return BM_ERR_PARAM;
    }
    if (data_cnt > 1000000) {
        bmlib_log("SORT", BMLIB_LOG_ERROR, "data_cnt should be less than 1000000!\n");
        return BM_ERR_PARAM;
    }
    if (sort_cnt > data_cnt) {
        bmlib_log("SORT", BMLIB_LOG_ERROR, "sort_cnt should be less than data_cnt!\n");
        return BM_ERR_PARAM;
    }
    return BM_SUCCESS;
}

bm_status_t bmcv_sort(bm_handle_t handle, bm_device_mem_t src_index_addr, bm_device_mem_t src_data_addr,
                      int data_cnt, bm_device_mem_t dst_index_addr, bm_device_mem_t dst_data_addr,
                      int sort_cnt, int order, bool index_enable, bool auto_index) {
    bool use_index_i = index_enable && (!auto_index);
    bool use_index_o = index_enable;
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid;
    int core_id = 0;
    int if_core0 = 1;
    int if_core1 = 0;
    bm_api_cv_sort_t api;
    sg_api_cv_sort_dual_core_t api_dual_core;
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
    ret = bmcv_sort_check(handle, data_cnt, sort_cnt);
    if (ret != BM_SUCCESS) {
        printf("bmcv_sort_check error !\n");
        return ret;
    }
    bm_device_mem_t src_index_buf_device;
    bm_device_mem_t src_data_buf_device;
    bm_device_mem_t dst_index_buf_device;
    bm_device_mem_t dst_data_buf_device;
    bm_device_mem_t dst_core0_index_buf_device;
    bm_device_mem_t dst_core0_data_buf_device;
    bm_device_mem_t dst_core1_index_buf_device;
    bm_device_mem_t dst_core1_data_buf_device;
    if (bm_mem_get_type(src_index_addr) == BM_MEM_TYPE_SYSTEM && use_index_i) {
        BM_CHECK_RET(bm_mem_convert_system_to_device_coeff(handle, &src_index_buf_device, src_index_addr,
                                                           true, data_cnt));
    } else {
        src_index_buf_device = src_index_addr;
    }
    if (bm_mem_get_type(src_data_addr) == BM_MEM_TYPE_SYSTEM) {
        BM_CHECK_RET(bm_mem_convert_system_to_device_coeff(handle, &src_data_buf_device, src_data_addr,
                                                           true, data_cnt));
    } else {
        src_data_buf_device = src_data_addr;
    }
    if (bm_mem_get_type(dst_index_addr) == BM_MEM_TYPE_SYSTEM && use_index_o) {
        BM_CHECK_RET(bm_mem_convert_system_to_device_coeff(handle, &dst_index_buf_device, dst_index_addr,
                                                           false, data_cnt));
    } else {
        dst_index_buf_device = dst_index_addr;
    }
    if (bm_mem_get_type(dst_data_addr) == BM_MEM_TYPE_SYSTEM) {
        BM_CHECK_RET(bm_mem_convert_system_to_device_coeff(handle, &dst_data_buf_device, dst_data_addr,
                                                           false, data_cnt));
    } else {
        dst_data_buf_device = dst_data_addr;
    }
    ret = bm_get_chipid(handle, &chipid);
    if (ret != BM_SUCCESS) {
        printf("get chipid is error !\n");
        return BM_ERR_FAILURE;
    }
    if (if_core0 == 1 && if_core1 == 1) {
        ret = bmcv_sort_dual_check_check(handle, data_cnt, sort_cnt);
        if (ret != BM_SUCCESS) {
            printf("bmcv_sort_dual_check_check error!\n");
            return ret;
        }
        if (sort_cnt > 128) {
            BM_CHECK_RET(bm_malloc_device_byte(handle, &dst_core0_data_buf_device, data_cnt * sizeof(float)));
            BM_CHECK_RET(bm_malloc_device_byte(handle, &dst_core0_index_buf_device, data_cnt * sizeof(int)));
            BM_CHECK_RET(bm_malloc_device_byte(handle, &dst_core1_data_buf_device, data_cnt * sizeof(float)));
            BM_CHECK_RET(bm_malloc_device_byte(handle, &dst_core1_index_buf_device, data_cnt * sizeof(int)));
        } else {
            BM_CHECK_RET(bm_malloc_device_byte(handle, &dst_core0_data_buf_device, sort_cnt * sizeof(float)));
            BM_CHECK_RET(bm_malloc_device_byte(handle, &dst_core0_index_buf_device, sort_cnt * sizeof(int)));
            BM_CHECK_RET(bm_malloc_device_byte(handle, &dst_core1_data_buf_device, sort_cnt * sizeof(float)));
            BM_CHECK_RET(bm_malloc_device_byte(handle, &dst_core1_index_buf_device, sort_cnt * sizeof(int)));
        }
        int base_msg_id = 0;
        int core_list[BM1688_MAX_CORES];
        int core_nums = 0;
        if (if_core0) core_list[core_nums++] = 0;
        if (if_core1) core_list[core_nums++] = 1;
        api_dual_core.src_index_addr = use_index_i ? bm_mem_get_device_addr(src_index_buf_device) : 0;
        api_dual_core.src_data_addr  = bm_mem_get_device_addr(src_data_buf_device);
        api_dual_core.dst_core0_data_addr = bm_mem_get_device_addr(dst_core0_data_buf_device);
        api_dual_core.dst_core0_index_addr = bm_mem_get_device_addr(dst_core0_index_buf_device);
        api_dual_core.dst_core1_data_addr = bm_mem_get_device_addr(dst_core1_data_buf_device);
        api_dual_core.dst_core1_index_addr = bm_mem_get_device_addr(dst_core1_index_buf_device);
        api_dual_core.order          = order;
        api_dual_core.index_enable   = index_enable ? 1 : 0;
        api_dual_core.auto_index     = auto_index ? 1 : 0;
        api_dual_core.data_cnt       = data_cnt;
        api_dual_core.sort_cnt       = sort_cnt;
        for (int i = 0; i < core_nums; i++) {
            base_msg_id |= 1 << core_list[i];
        }
        api_dual_core.base_msg_id = base_msg_id;
        api_dual_core.core_num = core_nums;
        sg_api_cv_sort_dual_core_t sort_params[BM1688_MAX_CORES];
        for (int core_idx = 0; core_idx < core_nums; core_idx++) {
            api_dual_core.core_id = core_idx;
            sort_params[core_idx] = api_dual_core;
            launch_params[core_idx].core_id = core_list[core_idx];
            launch_params[core_idx].param_data = &sort_params[core_idx];
            launch_params[core_idx].param_size = sizeof(sg_api_cv_sort_dual_core_t);
        }

        switch (chipid) {
            case BM1688_PREV:
            case BM1688:
                ret = bm_tpu_kernel_launch_dual_core(handle, "cv_sort_bm1688", launch_params, core_list, core_nums);
                if(BM_SUCCESS != ret) {
                    bmlib_log("SORT", BMLIB_LOG_ERROR, "sort sync api error\n");
                    return ret;
                }
                break;
            default:
                ret = BM_NOT_SUPPORTED;
                printf("BM_NOT_SUPPORTED!\n");
                break;
        }
    } else {
        api.src_index_addr = use_index_i ? bm_mem_get_device_addr(src_index_buf_device) : 0;
        api.src_data_addr  = bm_mem_get_device_addr(src_data_buf_device);
        api.dst_index_addr = use_index_o ? bm_mem_get_device_addr(dst_index_buf_device) : 0;
        api.dst_data_addr  = bm_mem_get_device_addr(dst_data_buf_device);
        api.order          = order;
        api.index_enable   = index_enable ? 1 : 0;
        api.auto_index     = auto_index ? 1 : 0;
        api.data_cnt       = data_cnt;
        api.sort_cnt       = sort_cnt;
        if(if_core1 == 1) {
            core_id = 1;
        }

        switch (chipid) {
            case BM1688_PREV:
            case BM1688:
                ret = bm_tpu_kernel_launch(handle, "cv_sort", (u8 *)&api, sizeof(api), core_id);
                if(BM_SUCCESS != ret) {
                    bmlib_log("SORT", BMLIB_LOG_ERROR, "sort sync api error\n");
                    return ret;
                }
                break;
            default:
                ret = BM_NOT_SUPPORTED;
                printf("BM_NOT_SUPPORTED!\n");
                break;
        }
    }

    // copy and free
    if (if_core0 == 1 && if_core1 == 1) {
        float *core0_buffer_topk_data, *core1_buffer_topk_data, *result_topk_data, *dual_core_buffer_data = NULL;
        int *core0_buffer_topk_idx, *core1_buffer_topk_idx, *result_topk_idx, *dual_core_buffer_idx = NULL;
        if(sort_cnt > 128) {
            core0_buffer_topk_data = (float*)malloc(data_cnt * sizeof(float));
            core1_buffer_topk_data = (float*)malloc(data_cnt * sizeof(float));
            core0_buffer_topk_idx = (int*)malloc(data_cnt * sizeof(int));
            core1_buffer_topk_idx = (int*)malloc(data_cnt * sizeof(int));
        } else {
            core0_buffer_topk_data = (float*)malloc(sort_cnt * sizeof(float));
            core1_buffer_topk_data = (float*)malloc(sort_cnt * sizeof(float));
            core0_buffer_topk_idx = (int*)malloc(sort_cnt * sizeof(int));
            core1_buffer_topk_idx = (int*)malloc(sort_cnt * sizeof(int));
        }
        result_topk_data = (float*)malloc(sort_cnt * sizeof(float));
        result_topk_idx = (int*)malloc(sort_cnt * sizeof(int));
        dual_core_buffer_data = (float*)malloc(2 * sort_cnt * sizeof(float));
        dual_core_buffer_idx = (int*)malloc(2 * sort_cnt * sizeof(int));
        BM_CHECK_RET(bm_memcpy_d2s(handle, core0_buffer_topk_data, dst_core0_data_buf_device));
        BM_CHECK_RET(bm_memcpy_d2s(handle, core1_buffer_topk_data, dst_core1_data_buf_device));
        BM_CHECK_RET(bm_memcpy_d2s(handle, core0_buffer_topk_idx, dst_core0_index_buf_device));
        BM_CHECK_RET(bm_memcpy_d2s(handle, core1_buffer_topk_idx, dst_core1_index_buf_device));
        memcpy(dual_core_buffer_data, core0_buffer_topk_data, sort_cnt * sizeof(float));
        memcpy(dual_core_buffer_data + sort_cnt, core1_buffer_topk_data, sort_cnt * sizeof(float));
        memcpy(dual_core_buffer_idx, core0_buffer_topk_idx, sort_cnt * sizeof(int));
        memcpy(dual_core_buffer_idx + sort_cnt, core1_buffer_topk_idx, sort_cnt * sizeof(int));
        find_topk_fp32(dual_core_buffer_data, dual_core_buffer_idx, result_topk_data, result_topk_idx, sort_cnt, 2, order);
        memcpy(bm_mem_get_system_addr(dst_data_addr), result_topk_data, sort_cnt * sizeof(float));
        memcpy(bm_mem_get_system_addr(dst_index_addr), result_topk_idx, sort_cnt * sizeof(int));
        if (bm_mem_get_type(dst_data_addr) == BM_MEM_TYPE_SYSTEM) {
            bm_free_device(handle, dst_data_buf_device);
        }
        if (bm_mem_get_type(dst_index_addr) == BM_MEM_TYPE_SYSTEM && use_index_o) {
            bm_free_device(handle, dst_index_buf_device);
        }
        if (bm_mem_get_type(src_data_addr) == BM_MEM_TYPE_SYSTEM) {
            bm_free_device(handle, src_data_buf_device);
        }
        if (bm_mem_get_type(src_index_addr) == BM_MEM_TYPE_SYSTEM && use_index_i) {
            bm_free_device(handle, src_index_buf_device);
        }
        bm_free_device(handle, dst_core0_index_buf_device);
        bm_free_device(handle, dst_core0_data_buf_device);
        bm_free_device(handle, dst_core1_index_buf_device);
        bm_free_device(handle, dst_core1_data_buf_device);
        free(core0_buffer_topk_data);
        free(core1_buffer_topk_data);
        free(result_topk_data);
        free(dual_core_buffer_data);
        free(core0_buffer_topk_idx);
        free(core1_buffer_topk_idx);
        free(result_topk_idx);
        free(dual_core_buffer_idx);
    } else {
        if (bm_mem_get_type(dst_data_addr) == BM_MEM_TYPE_SYSTEM) {
            bm_mem_set_device_size(&dst_data_buf_device, sort_cnt * 4);
            if (BM_SUCCESS != bm_memcpy_d2s(handle, bm_mem_get_system_addr(dst_data_addr),
                                            dst_data_buf_device)) {
                bm_mem_set_device_size(&dst_data_buf_device, data_cnt * 4);
                BMCV_ERR_LOG("bm_memcpy_d2s error\r\n");
                goto err2;
            }
            bm_mem_set_device_size(&dst_data_buf_device, data_cnt * 4);
            bm_free_device(handle, dst_data_buf_device);
        }
        if (bm_mem_get_type(dst_index_addr) == BM_MEM_TYPE_SYSTEM && use_index_o) {
            bm_mem_set_device_size(&dst_index_buf_device, sort_cnt * 4);
            if (BM_SUCCESS != bm_memcpy_d2s(handle, bm_mem_get_system_addr(dst_index_addr),
                                            dst_index_buf_device)) {
                bm_mem_set_device_size(&dst_index_buf_device, data_cnt * 4);
                BMCV_ERR_LOG("bm_memcpy_d2s error\r\n");
                goto err1;
            }
            bm_mem_set_device_size(&dst_index_buf_device, data_cnt * 4);
            bm_free_device(handle, dst_index_buf_device);
        }
        if (bm_mem_get_type(src_index_addr) == BM_MEM_TYPE_SYSTEM && use_index_i) {
            bm_free_device(handle, src_index_buf_device);
        }
        if (bm_mem_get_type(src_data_addr) == BM_MEM_TYPE_SYSTEM) {
            bm_free_device(handle, src_data_buf_device);
        }
    }
    return BM_SUCCESS;

err2:
    if (bm_mem_get_type(dst_data_addr) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, dst_data_buf_device);
    }
err1:
    if (bm_mem_get_type(dst_index_addr) == BM_MEM_TYPE_SYSTEM && use_index_o) {
        bm_free_device(handle, dst_index_buf_device);
    }
    if (bm_mem_get_type(src_data_addr) == BM_MEM_TYPE_SYSTEM) {
        bm_free_device(handle, src_data_buf_device);
    }
    if (bm_mem_get_type(src_index_addr) == BM_MEM_TYPE_SYSTEM && use_index_i) {
        bm_free_device(handle, src_index_buf_device);
    }
    return BM_ERR_FAILURE;
}