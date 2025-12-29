#include "bmcv_api_ext_c.h"
#include "bmcv_internal.h"
#include "bmcv_common.h"

static int param_check_adc(int database_vecs_num, int query_vecs_num, int sort_cnt, int vec_dims, int centroids_num){
    if(sort_cnt > database_vecs_num) {
        printf("sort_cnt cannot be greater than database_vecs_num!\n");
        return -1;
    }
    if(query_vecs_num > database_vecs_num) {
        printf("query_vecs_num cannot be greater than database_vecs_num!\n");
        return -1;
    }
    if(vec_dims > 512) {
        printf("vec_dims cannot be greater than 512!\n");
        return -1;
    }
    if(database_vecs_num < centroids_num) {
        printf("database_vecs_num must be greater than centroids_num!\n");
        return -1;
    }
    return 0;
}

static int param_check_sdc(int database_vecs_num, int query_vecs_num, int sort_cnt, int centroids_num){
    if(sort_cnt > database_vecs_num) {
        printf("sort_cnt cannot be greater than database_vecs_num!\n");
        return -1;
    }
    if(query_vecs_num > database_vecs_num) {
        printf("query_vecs_num cannot be greater than database_vecs_num!\n");
        return -1;
    }
    if(database_vecs_num < centroids_num) {
        printf("database_vecs_num must be greater than centroids_num!\n");
        return -1;
    }
    return 0;
}

static int param_check_encode(int database_vecs_num, int centroids_num){
    if(database_vecs_num < centroids_num) {
        printf("database_vecs_num must be greater than centroids_num!\n");
        return -1;
    }
    return 0;
}

bm_status_t bmcv_faiss_indexPQ_ADC_ext(bm_handle_t handle,
                                   bm_device_mem_t centroids_input_dev,
                                   bm_device_mem_t nxquery_input_dev,
                                   bm_device_mem_t nycodes_input_dev,
                                   bm_device_mem_t distance_output_dev,
                                   bm_device_mem_t index_output_dev,
                                   int vec_dims,
                                   int slice_num,
                                   int centroids_num,
                                   int database_num,
                                   int query_num,
                                   int sort_cnt,
                                   int IP_metric,
                                   int in_dtype,
                                   int out_dtype) {
    bm_status_t ret = BM_SUCCESS;
    ret = param_check_adc(database_num, query_num, sort_cnt, vec_dims, centroids_num);
    if (BM_SUCCESS != ret) {
        printf("faiss_api_indexPQ_ADCsearch param_check failed!\n");
        return ret;
    }
    faiss_api_indexPQ_ADC_t api;
    bm_device_mem_t distance_output_dev_fp32;

    if (bm_mem_get_type(distance_output_dev) == BM_MEM_TYPE_DEVICE) {
        if (BM_SUCCESS !=
            bm_malloc_device_byte(handle, &distance_output_dev_fp32, query_num * database_num * sizeof(float))) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "faiss_indexPQ_ADC bm_malloc_device_byte error!, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
        }
    }

    api.centroids_global_addr = bm_mem_get_device_addr(centroids_input_dev);
    api.nxquery_global_addr = bm_mem_get_device_addr(nxquery_input_dev);
    api.nycodes_global_addr = bm_mem_get_device_addr(nycodes_input_dev);
    api.output_distance_global_addr = bm_mem_get_device_addr(distance_output_dev);
    api.output_distance_global_tmp_addr = bm_mem_get_device_addr(distance_output_dev_fp32);
    api.output_index_global_addr = bm_mem_get_device_addr(index_output_dev);
    api.d = vec_dims;
    api.m = slice_num;
    api.ksub = centroids_num;
    api.ny = database_num;
    api.nx = query_num;
    api.k = sort_cnt;
    api.IP_metric = IP_metric;
    api.input_dtype = in_dtype;
    api.output_dtype = out_dtype;
    unsigned int chipid;
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret) {
        printf("faiss_api_indexPQ_ADCsearch bm_get_chipid failed!\n");
        if (bm_mem_get_type(distance_output_dev) == BM_MEM_TYPE_DEVICE) {
            bm_free_device(handle, distance_output_dev_fp32);
        }
        return ret;
    }
    int core_id = 0;

    switch (chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "faiss_api_indexPQ_ADCsearch", (u8 *)&api, sizeof(api), core_id);
            if (BM_SUCCESS != ret) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "faiss_indexPQ_ADC launch_sync api error!, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
            }
            break;
        default:
            printf("unknown chip_id!\n");
            ret = BM_NOT_SUPPORTED;
            break;
    }
    if (bm_mem_get_type(distance_output_dev) == BM_MEM_TYPE_DEVICE) {
        bm_free_device(handle, distance_output_dev_fp32);
    }
    return ret;
}

bm_status_t bmcv_faiss_indexPQ_ADC(bm_handle_t handle,
                                   bm_device_mem_t centroids_input_dev,
                                   bm_device_mem_t nxquery_input_dev,
                                   bm_device_mem_t nycodes_input_dev,
                                   bm_device_mem_t distance_output_dev,
                                   bm_device_mem_t index_output_dev,
                                   int vec_dims,
                                   int slice_num,
                                   int centroids_num,
                                   int database_num,
                                   int query_num,
                                   int sort_cnt,
                                   int IP_metric) {
    return bmcv_faiss_indexPQ_ADC_ext(handle,
                                       centroids_input_dev,
                                       nxquery_input_dev,
                                       nycodes_input_dev,
                                       distance_output_dev,
                                       index_output_dev,
                                       vec_dims,
                                       slice_num,
                                       centroids_num,
                                       database_num,
                                       query_num,
                                       sort_cnt,
                                       IP_metric,
                                       DT_FP32,
                                       DT_FP32);
}

bm_status_t bmcv_faiss_indexPQ_SDC_ext(bm_handle_t handle,
                                   bm_device_mem_t sdc_table_input_dev,
                                   bm_device_mem_t nxcodes_input_dev,
                                   bm_device_mem_t nycodes_input_dev,
                                   bm_device_mem_t distance_output_dev,
                                   bm_device_mem_t index_output_dev,
                                   int slice_num,
                                   int centroids_num,
                                   int database_num,
                                   int query_num,
                                   int sort_cnt,
                                   int IP_metric,
                                   int in_dtype,
                                   int out_dtype) {
    bm_status_t ret = BM_SUCCESS;
    ret = param_check_sdc(database_num, query_num, sort_cnt, centroids_num);
    if (BM_SUCCESS != ret) {
        printf("faiss_api_indexPQ_SDCsearch param_check failed!\n");
        return ret;
    }
    faiss_api_indexPQ_SDC_t api;
    bm_device_mem_t distance_output_dev_fp32;

    if (bm_mem_get_type(distance_output_dev) == BM_MEM_TYPE_DEVICE) {
        if (BM_SUCCESS !=
            bm_malloc_device_byte(handle, &distance_output_dev_fp32, query_num * database_num * sizeof(float))) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "faiss_indexPQ_SDC bm_malloc_device_byte error!, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
        }
    }

    api.sdc_table_global_addr = bm_mem_get_device_addr(sdc_table_input_dev);
    api.nxcodes_global_addr = bm_mem_get_device_addr(nxcodes_input_dev);
    api.nycodes_global_addr = bm_mem_get_device_addr(nycodes_input_dev);
    api.output_distance_global_addr = bm_mem_get_device_addr(distance_output_dev);
    api.output_distance_global_tmp_addr = bm_mem_get_device_addr(distance_output_dev_fp32);
    api.output_index_global_addr = bm_mem_get_device_addr(index_output_dev);
    api.m = slice_num;
    api.ksub = centroids_num;
    api.ny = database_num;
    api.nx = query_num;
    api.k = sort_cnt;
    api.IP_metric = IP_metric;
    api.input_dtype = in_dtype;
    api.output_dtype = out_dtype;
    unsigned int chipid;
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret) {
        printf("faiss_api_indexPQ_SDCsearch bm_get_chipid failed!\n");
        if (bm_mem_get_type(distance_output_dev) == BM_MEM_TYPE_DEVICE) {
            bm_free_device(handle, distance_output_dev_fp32);
        }
        return ret;
    }
    int core_id = 0;
    switch (chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "faiss_api_indexPQ_SDCsearch", (u8 *)&api, sizeof(api), core_id);
            if (BM_SUCCESS != ret) {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "faiss_indexPQ_SDC launch_sync api error!, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
            }
            break;
        default:
            printf("unknown chip_id!\n");
            ret = BM_NOT_SUPPORTED;
            break;
    }
    if (bm_mem_get_type(distance_output_dev) == BM_MEM_TYPE_DEVICE) {
        bm_free_device(handle, distance_output_dev_fp32);
    }
    return ret;
}

bm_status_t bmcv_faiss_indexPQ_SDC(bm_handle_t handle,
                                   bm_device_mem_t sdc_table_input_dev,
                                   bm_device_mem_t nxcodes_input_dev,
                                   bm_device_mem_t nycodes_input_dev,
                                   bm_device_mem_t distance_output_dev,
                                   bm_device_mem_t index_output_dev,
                                   int slice_num,
                                   int centroids_num,
                                   int database_num,
                                   int query_num,
                                   int sort_cnt,
                                   int IP_metric) {
    return bmcv_faiss_indexPQ_SDC_ext(handle,
                                   sdc_table_input_dev,
                                   nxcodes_input_dev,
                                   nycodes_input_dev,
                                   distance_output_dev,
                                   index_output_dev,
                                   slice_num,
                                   centroids_num,
                                   database_num,
                                   query_num,
                                   sort_cnt,
                                   IP_metric,
                                   DT_FP32,
                                   DT_FP32);
}

bm_status_t bmcv_faiss_indexPQ_encode_ext(bm_handle_t handle,
                                      bm_device_mem_t vector_input_dev,
                                      bm_device_mem_t centroids_input_dev,
                                      bm_device_mem_t buffer_table_dev,
                                      bm_device_mem_t nvcodes_output_dev,
                                      int encode_vec_num,
                                      int vec_dims,
                                      int slice_num,
                                      int centroids_num,
                                      int IP_metric,
                                      int input_dtype,
                                      int output_dtype) {
    bm_status_t ret = BM_SUCCESS;
    ret = param_check_encode(encode_vec_num, centroids_num);
    if (BM_SUCCESS != ret) {
        printf("faiss_api_indexPQ_encode param_check failed!\n");
        return ret;
    }
    faiss_api_indexPQ_encode_t api;
    bm_device_mem_t buffer_table_dev_32byte;

    if (bm_mem_get_type(buffer_table_dev) == BM_MEM_TYPE_DEVICE) {
        if (BM_SUCCESS !=
            bm_malloc_device_byte(handle, &buffer_table_dev_32byte, encode_vec_num * slice_num * centroids_num * sizeof(float))) {
            bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "faiss_indexPQ_encode bm_malloc_device_byte error!, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
            return BM_ERR_FAILURE;
        }
    }
    api.vector_input_global_addr = bm_mem_get_device_addr(vector_input_dev);
    api.centroids_global_addr = bm_mem_get_device_addr(centroids_input_dev);
    api.buffer_table_global_addr = bm_mem_get_device_addr(buffer_table_dev);
    api.buffer_table_global_tmp_addr = bm_mem_get_device_addr(buffer_table_dev_32byte);
    api.codes_output_global_addr = bm_mem_get_device_addr(nvcodes_output_dev);
    api.nv = encode_vec_num;
    api.d = vec_dims;
    api.m = slice_num;
    api.ksub = centroids_num;
    api.IP_metric = IP_metric;
    api.input_dtype = input_dtype;
    api.output_dtype = output_dtype;
    unsigned int chipid;
    ret = bm_get_chipid(handle, &chipid);
    if (BM_SUCCESS != ret) {
        printf("faiss_api_indexPQ_SDCsearch bm_get_chipid failed!\n");
        if (bm_mem_get_type(buffer_table_dev) == BM_MEM_TYPE_DEVICE) {
            bm_free_device(handle, buffer_table_dev_32byte);
        }
        return ret;
    }
    int core_id = 0;
    switch (chipid) {
        case BM1688_PREV:
        case BM1688:
            ret = bm_tpu_kernel_launch(handle, "faiss_api_indexPQ_encode", (u8 *)&api, sizeof(api), core_id);
            if (BM_SUCCESS != ret)
            {
                bmlib_log(BMCV_LOG_TAG, BMLIB_LOG_ERROR, "faiss_indexPQ_encode launch_sync api error!, %s: %s: %d\n", filename(__FILE__), __func__, __LINE__);
            }
            break;
        default:
            printf("unknown chip_id!\n");
            ret = BM_NOT_SUPPORTED;
            break;
    }
    if (bm_mem_get_type(buffer_table_dev) == BM_MEM_TYPE_DEVICE) {
        bm_free_device(handle, buffer_table_dev_32byte);
    }
    return ret;
}

bm_status_t bmcv_faiss_indexPQ_encode(bm_handle_t handle,
                                      bm_device_mem_t vector_input_dev,
                                      bm_device_mem_t centroids_input_dev,
                                      bm_device_mem_t buffer_table_dev,
                                      bm_device_mem_t nvcodes_output_dev,
                                      int encode_vec_num,
                                      int vec_dims,
                                      int slice_num,
                                      int centroids_num,
                                      int IP_metric) {
    return bmcv_faiss_indexPQ_encode_ext(handle,
                                      vector_input_dev,
                                      centroids_input_dev,
                                      buffer_table_dev,
                                      nvcodes_output_dev,
                                      encode_vec_num,
                                      vec_dims,
                                      slice_num,
                                      centroids_num,
                                      IP_metric,
                                      DT_FP32,
                                      DT_FP32);
}
