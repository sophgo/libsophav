#include <stddef.h>
#include "bmcv_internal.h"
#include "bmcv_common.h"
bm_status_t bmcv_faiss_indexflatL2_check(int database_vecs_num, int sort_cnt, int query_vecs_num, int input_dtype, int output_dtype, int is_transpose) {
    switch (input_dtype) {
        case 5:
            if (output_dtype != 3 && output_dtype != 5) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "faiss_api_indexflatL2 when input_dtype = fp32, output_dtype should be fp16/fp32! %s: %s: %d\n",
                        filename(__FILE__), __func__, __LINE__);
                return BM_ERR_PARAM;
            }
            break;
        case 3:
            if (output_dtype != 3 && output_dtype != 5) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "faiss_api_indexflatL2 when input_dtype = fp16, output_dtype should be fp16/fp32! %s: %s: %d\n",
                        filename(__FILE__), __func__, __LINE__);
                return BM_ERR_PARAM;
            }
            break;
        default:
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "faiss_api_indexflatL2 input_dtype should be fp32/fp16! %s: %s: %d\n",
                        filename(__FILE__), __func__, __LINE__);
            return BM_ERR_PARAM;
    }
    if (database_vecs_num < sort_cnt){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "faiss_api_indexflatL2 database_vecs_num(%d) < sort_cnt(%d), %s: %s: %d\n",
                                        database_vecs_num, sort_cnt, filename(__FILE__), __func__, __LINE__);
        return BM_ERR_PARAM;
    }
    if (database_vecs_num < query_vecs_num){
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "faiss_api_indexflatL2 database_vecs_num(%d) < query_vecs_num(%d), %s: %s: %d\n",
                                        database_vecs_num, sort_cnt, filename(__FILE__), __func__, __LINE__);
        return BM_ERR_PARAM;
    }
    if (input_dtype == DT_FP16 && is_transpose == 1) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "faiss_api_indexflatL2 when input_dtype is DT_FP16, database matrix should not be transposed, %s: %s: %d\n",
                  filename(__FILE__), __func__, __LINE__);
        return BM_ERR_PARAM;
    }
    return BM_SUCCESS;
}
bm_status_t bmcv_faiss_indexflatL2(bm_handle_t handle,
                                   bm_device_mem_t input_data_global_addr,
                                   bm_device_mem_t db_data_global_addr,
                                   bm_device_mem_t query_L2norm_global_addr,
                                   bm_device_mem_t db_L2norm_global_addr,
                                   bm_device_mem_t buffer_global_addr,
                                   bm_device_mem_t output_sorted_similarity_global_addr,
                                   bm_device_mem_t output_sorted_index_global_addr,
                                   int vec_dims,
                                   int query_vecs_num,
                                   int database_vecs_num,
                                   int sort_cnt,
                                   int is_transpose,
                                   int input_dtype,
                                   int output_dtype) {
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid;
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "faiss_api_indexflatL2 bm_get_chipid failed!, %s: %s: %d\n",
                                        filename(__FILE__), __func__, __LINE__);
        return ret;
    }
    faiss_api_indexflatL2_t api;
    sg_api_indexflatL2_dual_core_t api_dual_core;
    tpu_launch_param_t launch_params[BM1688_MAX_CORES];
    bm_device_mem_t output_dual_core_sorted_similarity_global_addr, output_dual_core_sorted_index_global_addr;
    int if_core0 = 1;
    int if_core1 = 0;
    int core_id = 0;
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
    ret = bmcv_faiss_indexflatL2_check(database_vecs_num, sort_cnt, query_vecs_num, input_dtype, output_dtype, is_transpose);
    if (ret != BM_SUCCESS) {
        printf("bmcv_faiss_indexflatIP_check failed!\n");
        return ret;
    }
    if (if_core0 == 1 && if_core1 == 1) {
        if (query_vecs_num == 1 && ((database_vecs_num / 2) < sort_cnt)) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "faiss_api_indexflatL2 when using dual cores for calculations, database_vecs_num / 2 should be greater than sort_cnt! %s: %s: %d\n",
                    filename(__FILE__), __func__, __LINE__);
            return BM_ERR_PARAM;
        }
        BM_CHECK_RET(bm_malloc_device_byte(handle, &output_dual_core_sorted_similarity_global_addr, 2 * query_vecs_num * sort_cnt * dtype_size((enum bm_data_type_t)output_dtype)));
        BM_CHECK_RET(bm_malloc_device_byte(handle, &output_dual_core_sorted_index_global_addr, 2 * query_vecs_num * sort_cnt * sizeof(int)));
        int base_msg_id = 0;
        int core_list[BM1688_MAX_CORES];
        int core_nums = 0;
        if (if_core0) core_list[core_nums++] = 0;
        if (if_core1) core_list[core_nums++] = 1;
        api_dual_core.input_query_global_addr = bm_mem_get_device_addr(input_data_global_addr);
        api_dual_core.database_global_addr = bm_mem_get_device_addr(db_data_global_addr);
        api_dual_core.query_L2norm_global_addr = bm_mem_get_device_addr(query_L2norm_global_addr);
        api_dual_core.db_L2norm_global_addr = bm_mem_get_device_addr(db_L2norm_global_addr);
        api_dual_core.buffer_global_addr = bm_mem_get_device_addr(buffer_global_addr);
        api_dual_core.output_sorted_similarity_global_addr = bm_mem_get_device_addr(output_sorted_similarity_global_addr);
        api_dual_core.output_sorted_index_global_addr = bm_mem_get_device_addr(output_sorted_index_global_addr);
        api_dual_core.output_dual_core_sorted_similarity_global_addr = bm_mem_get_device_addr(output_dual_core_sorted_similarity_global_addr);
        api_dual_core.output_dual_core_sorted_index_global_addr = bm_mem_get_device_addr(output_dual_core_sorted_index_global_addr);
        api_dual_core.vec_dims = vec_dims;
        api_dual_core.query_vecs_num = query_vecs_num;
        api_dual_core.database_vecs_num = database_vecs_num;
        api_dual_core.k = sort_cnt;
        api_dual_core.transpose = is_transpose;
        api_dual_core.input_dtype = input_dtype;
        api_dual_core.output_dtype = output_dtype;
        for (int i = 0; i < core_nums; i++) {
            base_msg_id |= 1 << core_list[i];
        }
        api_dual_core.base_msg_id = base_msg_id;
        api_dual_core.core_num = core_nums;
        sg_api_indexflatL2_dual_core_t params[BM1688_MAX_CORES];
        for (int core_idx = 0; core_idx < core_nums; core_idx++) {
            api_dual_core.core_id = core_idx;
            params[core_idx] = api_dual_core;
            launch_params[core_idx].core_id = core_list[core_idx];
            launch_params[core_idx].param_data = &params[core_idx];
            launch_params[core_idx].param_size = sizeof(sg_api_indexflatL2_dual_core_t);
        }
        switch (chipid) {
            case BM1688_PREV:
            case BM1688:
                ret = bm_tpu_kernel_launch_dual_core(handle, "faiss_api_indexflatL2_bm1688", launch_params, core_list, core_nums);
                if (BM_SUCCESS != ret) {
                    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "faiss_api_indexflatL2 launch_sync api error!, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
                }
                break;
            default:
                ret = BM_NOT_SUPPORTED;
                break;
        }
        if(query_vecs_num == 1) {
            float *dual_core_similarity_host_fp32, *similarity_host_fp32;
            fp16 *dual_core_similarity_host_fp16, *similarity_host_fp16;
            int *dual_core_index_host = (int *)malloc(sizeof(int) * query_vecs_num * sort_cnt * 2);
            int *index_host = (int *)malloc(sizeof(int) * query_vecs_num * sort_cnt * 2);
            BM_CHECK_RET(bm_memcpy_d2s(handle, bm_mem_get_system_addr(bm_mem_from_system(dual_core_index_host)),
                                        output_dual_core_sorted_index_global_addr));
            if (output_dtype == DT_FP32) {
                dual_core_similarity_host_fp32 = (float *)malloc(sizeof(float) * query_vecs_num * sort_cnt * 2);
                similarity_host_fp32 = (float *)malloc(sizeof(float) * query_vecs_num * sort_cnt);
                BM_CHECK_RET(bm_memcpy_d2s(handle, bm_mem_get_system_addr(bm_mem_from_system(dual_core_similarity_host_fp32)),
                                output_dual_core_sorted_similarity_global_addr));
                find_topk_fp32(dual_core_similarity_host_fp32, dual_core_index_host,
                                similarity_host_fp32, index_host, sort_cnt, 2, 0);
                BM_CHECK_RET(bm_memcpy_s2d(handle, output_sorted_similarity_global_addr,
                                bm_mem_get_system_addr(bm_mem_from_system(similarity_host_fp32))));
                free(dual_core_similarity_host_fp32);
                free(similarity_host_fp32);
            } else {
                dual_core_similarity_host_fp16 = (fp16 *)malloc(sizeof(fp16) * query_vecs_num * sort_cnt * 2);
                similarity_host_fp16 = (fp16 *)malloc(sizeof(fp16) * query_vecs_num * sort_cnt);
                BM_CHECK_RET(bm_memcpy_d2s(handle, bm_mem_get_system_addr(bm_mem_from_system(dual_core_similarity_host_fp16)),
                                output_dual_core_sorted_similarity_global_addr));
                find_topk_fp16(dual_core_similarity_host_fp16, dual_core_index_host,
                                similarity_host_fp16, index_host, sort_cnt, 2, 0);
                BM_CHECK_RET(bm_memcpy_s2d(handle, output_sorted_similarity_global_addr,
                                bm_mem_get_system_addr(bm_mem_from_system(similarity_host_fp16))));
                free(dual_core_similarity_host_fp16);
                free(similarity_host_fp16);
            }
            BM_CHECK_RET(bm_memcpy_s2d(handle, output_sorted_index_global_addr,
                            bm_mem_get_system_addr(bm_mem_from_system(index_host))));
            free(dual_core_index_host);
            free(index_host);
        }
        bm_free_device(handle, output_dual_core_sorted_similarity_global_addr);
        bm_free_device(handle, output_dual_core_sorted_index_global_addr);
    } else {
        api.input_query_global_addr = bm_mem_get_device_addr(input_data_global_addr);
        api.database_global_addr = bm_mem_get_device_addr(db_data_global_addr);
        api.query_L2norm_global_addr = bm_mem_get_device_addr(query_L2norm_global_addr);
        api.db_L2norm_global_addr = bm_mem_get_device_addr(db_L2norm_global_addr);
        api.buffer_global_addr = bm_mem_get_device_addr(buffer_global_addr);
        api.output_sorted_similarity_global_addr = bm_mem_get_device_addr(output_sorted_similarity_global_addr);
        api.output_sorted_index_global_addr = bm_mem_get_device_addr(output_sorted_index_global_addr);
        api.vec_dims = vec_dims;
        api.query_vecs_num = query_vecs_num;
        api.database_vecs_num = database_vecs_num;
        api.k = sort_cnt;
        api.transpose = is_transpose;
        api.input_dtype = input_dtype;
        api.output_dtype = output_dtype;
        switch (chipid) {
            case BM1688_PREV:
            case BM1688:
                ret = bm_tpu_kernel_launch(handle, "faiss_api_indexflatL2", (u8 *)&api, sizeof(api), core_id);
                if (BM_SUCCESS != ret) {
                    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "faiss_api_indexflatL2 launch_sync api error!, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
                }
                break;
            default:
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "faiss_api_indexflatL2 chip_id not support, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
                ret = BM_NOT_SUPPORTED;
                break;
        }
    }
    return ret;
}

bm_status_t bmcv_faiss_indexflatL2_u64(bm_handle_t handle,
    bm_device_mem_u64_t input_data_global_addr,
    bm_device_mem_u64_t db_data_global_addr,
    bm_device_mem_u64_t query_L2norm_global_addr,
    bm_device_mem_u64_t db_L2norm_global_addr,
    bm_device_mem_u64_t buffer_global_addr,
    bm_device_mem_u64_t output_sorted_similarity_global_addr,
    bm_device_mem_u64_t output_sorted_index_global_addr,
    int vec_dims,
    int query_vecs_num,
    int database_vecs_num,
    int sort_cnt,
    int is_transpose,
    int input_dtype,
    int output_dtype) {
    bm_status_t ret = BM_SUCCESS;
    unsigned int chipid;
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret) {
        bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "bmcv_faiss_indexflatL2_u64 bm_get_chipid failed!, %s: %s: %d\n",
                                        filename(__FILE__), __func__, __LINE__);
        return ret;
    }
    faiss_api_indexflatL2_t api;
    sg_api_indexflatL2_dual_core_t api_dual_core;
    tpu_launch_param_t launch_params[BM1688_MAX_CORES];
    bm_device_mem_u64_t output_dual_core_sorted_similarity_global_addr, output_dual_core_sorted_index_global_addr;
    int if_core0 = 1;
    int if_core1 = 0;
    int core_id = 0;
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
    ret = bmcv_faiss_indexflatL2_check(database_vecs_num, sort_cnt, query_vecs_num, input_dtype, output_dtype, is_transpose);
    if (ret != BM_SUCCESS) {
        printf("bmcv_faiss_indexflatIP_check failed!\n");
        return ret;
    }
    if (if_core0 == 1 && if_core1 == 1) {
        if (query_vecs_num == 1 && ((database_vecs_num / 2) < sort_cnt)) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "faiss_api_indexflatL2 when using dual cores for calculations, database_vecs_num / 2 should be greater than sort_cnt! %s: %s: %d\n",
                    filename(__FILE__), __func__, __LINE__);
            return BM_ERR_PARAM;
        }
        BM_CHECK_RET(bm_malloc_device_byte_u64(handle, &output_dual_core_sorted_similarity_global_addr, 2 * query_vecs_num * sort_cnt * dtype_size((enum bm_data_type_t)output_dtype)));
        BM_CHECK_RET(bm_malloc_device_byte_u64(handle, &output_dual_core_sorted_index_global_addr, 2 * query_vecs_num * sort_cnt * sizeof(int)));
        int base_msg_id = 0;
        int core_list[BM1688_MAX_CORES];
        int core_nums = 0;
        if (if_core0) core_list[core_nums++] = 0;
        if (if_core1) core_list[core_nums++] = 1;
        api_dual_core.input_query_global_addr = bm_mem_get_device_addr_u64(input_data_global_addr);
        api_dual_core.database_global_addr = bm_mem_get_device_addr_u64(db_data_global_addr);
        api_dual_core.query_L2norm_global_addr = bm_mem_get_device_addr_u64(query_L2norm_global_addr);
        api_dual_core.db_L2norm_global_addr = bm_mem_get_device_addr_u64(db_L2norm_global_addr);
        api_dual_core.buffer_global_addr = bm_mem_get_device_addr_u64(buffer_global_addr);
        api_dual_core.output_sorted_similarity_global_addr = bm_mem_get_device_addr_u64(output_sorted_similarity_global_addr);
        api_dual_core.output_sorted_index_global_addr = bm_mem_get_device_addr_u64(output_sorted_index_global_addr);
        api_dual_core.output_dual_core_sorted_similarity_global_addr = bm_mem_get_device_addr_u64(output_dual_core_sorted_similarity_global_addr);
        api_dual_core.output_dual_core_sorted_index_global_addr = bm_mem_get_device_addr_u64(output_dual_core_sorted_index_global_addr);
        api_dual_core.vec_dims = vec_dims;
        api_dual_core.query_vecs_num = query_vecs_num;
        api_dual_core.database_vecs_num = database_vecs_num;
        api_dual_core.k = sort_cnt;
        api_dual_core.transpose = is_transpose;
        api_dual_core.input_dtype = input_dtype;
        api_dual_core.output_dtype = output_dtype;
        for (int i = 0; i < core_nums; i++) {
            base_msg_id |= 1 << core_list[i];
        }
        api_dual_core.base_msg_id = base_msg_id;
        api_dual_core.core_num = core_nums;
        sg_api_indexflatL2_dual_core_t params[BM1688_MAX_CORES];
        for (int core_idx = 0; core_idx < core_nums; core_idx++) {
            api_dual_core.core_id = core_idx;
            params[core_idx] = api_dual_core;
            launch_params[core_idx].core_id = core_list[core_idx];
            launch_params[core_idx].param_data = &params[core_idx];
            launch_params[core_idx].param_size = sizeof(sg_api_indexflatL2_dual_core_t);
        }
        switch (chipid) {
            case BM1688_PREV:
            case BM1688:
                ret = bm_tpu_kernel_launch_dual_core(handle, "faiss_api_indexflatL2_bm1688", launch_params, core_list, core_nums);
                if (BM_SUCCESS != ret) {
                    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "faiss_api_indexflatL2 launch_sync api error!, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
                }
                break;
            default:
                ret = BM_NOT_SUPPORTED;
                break;
        }
        if(query_vecs_num == 1) {
            float *dual_core_similarity_host_fp32, *similarity_host_fp32;
            fp16 *dual_core_similarity_host_fp16, *similarity_host_fp16;
            int *dual_core_index_host = (int *)malloc(sizeof(int) * query_vecs_num * sort_cnt * 2);
            int *index_host = (int *)malloc(sizeof(int) * query_vecs_num * sort_cnt * 2);
            BM_CHECK_RET(bm_memcpy_d2s_u64(handle, bm_mem_get_system_addr(bm_mem_from_system(dual_core_index_host)),
                                        output_dual_core_sorted_index_global_addr));
            if (output_dtype == DT_FP32) {
                dual_core_similarity_host_fp32 = (float *)malloc(sizeof(float) * query_vecs_num * sort_cnt * 2);
                similarity_host_fp32 = (float *)malloc(sizeof(float) * query_vecs_num * sort_cnt);
                BM_CHECK_RET(bm_memcpy_d2s_u64(handle, bm_mem_get_system_addr(bm_mem_from_system(dual_core_similarity_host_fp32)),
                                output_dual_core_sorted_similarity_global_addr));
                find_topk_fp32(dual_core_similarity_host_fp32, dual_core_index_host,
                                similarity_host_fp32, index_host, sort_cnt, 2, 0);
                BM_CHECK_RET(bm_memcpy_s2d_u64(handle, output_sorted_similarity_global_addr,
                                bm_mem_get_system_addr(bm_mem_from_system(similarity_host_fp32))));
                free(dual_core_similarity_host_fp32);
                free(similarity_host_fp32);
            } else {
                dual_core_similarity_host_fp16 = (fp16 *)malloc(sizeof(fp16) * query_vecs_num * sort_cnt * 2);
                similarity_host_fp16 = (fp16 *)malloc(sizeof(fp16) * query_vecs_num * sort_cnt);
                BM_CHECK_RET(bm_memcpy_d2s_u64(handle, bm_mem_get_system_addr(bm_mem_from_system(dual_core_similarity_host_fp16)),
                                output_dual_core_sorted_similarity_global_addr));
                find_topk_fp16(dual_core_similarity_host_fp16, dual_core_index_host,
                                similarity_host_fp16, index_host, sort_cnt, 2, 0);
                BM_CHECK_RET(bm_memcpy_s2d_u64(handle, output_sorted_similarity_global_addr,
                                bm_mem_get_system_addr(bm_mem_from_system(similarity_host_fp16))));
                free(dual_core_similarity_host_fp16);
                free(similarity_host_fp16);
            }
            BM_CHECK_RET(bm_memcpy_s2d_u64(handle, output_sorted_index_global_addr,
                            bm_mem_get_system_addr(bm_mem_from_system(index_host))));
            free(dual_core_index_host);
            free(index_host);
        }
        bm_free_device_u64(handle, output_dual_core_sorted_similarity_global_addr);
        bm_free_device_u64(handle, output_dual_core_sorted_index_global_addr);
    } else {
        api.input_query_global_addr = bm_mem_get_device_addr_u64(input_data_global_addr);
        api.database_global_addr = bm_mem_get_device_addr_u64(db_data_global_addr);
        api.query_L2norm_global_addr = bm_mem_get_device_addr_u64(query_L2norm_global_addr);
        api.db_L2norm_global_addr = bm_mem_get_device_addr_u64(db_L2norm_global_addr);
        api.buffer_global_addr = bm_mem_get_device_addr_u64(buffer_global_addr);
        api.output_sorted_similarity_global_addr = bm_mem_get_device_addr_u64(output_sorted_similarity_global_addr);
        api.output_sorted_index_global_addr = bm_mem_get_device_addr_u64(output_sorted_index_global_addr);
        api.vec_dims = vec_dims;
        api.query_vecs_num = query_vecs_num;
        api.database_vecs_num = database_vecs_num;
        api.k = sort_cnt;
        api.transpose = is_transpose;
        api.input_dtype = input_dtype;
        api.output_dtype = output_dtype;
        switch (chipid) {
            case BM1688_PREV:
            case BM1688:
                ret = bm_tpu_kernel_launch(handle, "faiss_api_indexflatL2", (u8 *)&api, sizeof(api), core_id);
                if (BM_SUCCESS != ret) {
                    bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "faiss_api_indexflatL2 launch_sync api error!, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
                }
                break;
            default:
                ret = BM_NOT_SUPPORTED;
                break;
        }
    }
    return ret;
}
