#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __linux__
#include <sys/time.h>
#else
#include <windows.h>
#include "time.h"
#endif
#include <stdint.h>
#include "bmcv_api_ext_c.h"
#include "test_misc.h"
#include <pthread.h>
#include "string.h"

typedef unsigned int u32;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long long u64;

typedef struct {
    int L_row_num;
    int L_col_num;
    int R_col_num;
    int transpose;
    enum bm_data_type_t L_dtype;
    enum bm_data_type_t R_dtype;
    enum bm_data_type_t Y_dtype;
} matmul_param_t;

typedef struct {
    int loop;
    int database_vecs_num;
    int query_vecs_num;
    int sort_cnt;
    int vec_dims;
    int is_transpose;
    int input_dtype;
    int output_dtype;
    bm_handle_t handle;
} faiss_indexflatL2_thread_arg_t;

typedef struct {
    int index;
    float val;
} topk_t;

void fvec_norm_L2sqr_ref(float* vec, float* matrix, int row_num, int col_num) {
    for (int i = 0; i < row_num; i++) {
        float data_tmp[col_num];
        float l2_norm = 0.0;
        for (int j = 0; j < col_num; j++) {
            data_tmp[j] = ((float)rand() / (float)RAND_MAX);
            l2_norm += data_tmp[j] * data_tmp[j];
        }
        float norm = sqrt(l2_norm);
        for (int j = 0; j < col_num; j++) {
            matrix[i * col_num + j] = data_tmp[j] / norm;
        }
        vec[i] = 1.0f;
    }
}

void matrix_trans(void* src, void* dst, int row_num, int col_num, enum bm_data_type_t dtype) {
    for (int i = 0; i < row_num; i++) {
        for (int j = 0; j < col_num; j++) {
            if (dtype == DT_INT8 || dtype == DT_UINT8) {
                ((u8*)dst)[j * row_num + i] = ((u8*)src)[i * col_num + j];
            } else if (dtype == DT_INT16 || dtype == DT_UINT16) {
                ((u16*)dst)[j * row_num + i] = ((u16*)src)[i * col_num + j];
            } else if (dtype == DT_FP32) {
                ((float*)dst)[j * row_num + i] = ((float*)src)[i * col_num + j];
            } else if (dtype == DT_INT32 || dtype == DT_UINT32) {
                ((u32*)dst)[j * row_num + i] = ((u32*)src)[i * col_num + j];
            } else if (dtype == DT_FP16) {
                ((fp16*)dst)[j * row_num + i] = ((fp16*)src)[i * col_num + j];
            }
        }
    }
}

void native_calc_L2distance_matrix_fp32(const matmul_param_t* param,
                                   float* L_ptr,
                                   float* R_ptr,
                                   float* Y_ptr,
                                   float* vec_query,
                                   float* vec_db) {
    memset(Y_ptr, 0, sizeof(float) * param->L_row_num * param->R_col_num);
    for (int kidx = 0; kidx < param->L_col_num; kidx++) {
        float* cur_R = &R_ptr[kidx * param->R_col_num];
        for (int ridx = 0; ridx < param->L_row_num; ridx++) {
            float L_val = (float)L_ptr[ridx * param->L_col_num + kidx];
            for (int cidx = 0; cidx < param->R_col_num; cidx++) {
                Y_ptr[ridx * param->R_col_num + cidx] += L_val * (float)cur_R[cidx];
            }
        }
    }

    for (int i = 0; i < param->L_row_num; i++) {
        for (int j = 0; j < param->R_col_num; j++) {
            Y_ptr[i * param->R_col_num + j] = (-2) * Y_ptr[i * param->R_col_num + j];
            Y_ptr[i * param->R_col_num + j] += vec_query[i];
            Y_ptr[i * param->R_col_num + j] += vec_db[j];
        }
    }
}

static void merge_ascend(topk_t ref_res[], int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    topk_t L[n1], R[n2];

    for (int i = 0; i < n1; i++) {
        L[i] = ref_res[left + i];
    }
    for (int j = 0; j < n2; j++) {
        R[j] = ref_res[mid + 1 + j];
    }

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (L[i].val <= R[j].val) {
            ref_res[k] = L[i];
            i++;
        } else {
        ref_res[k] = R[j];
        j++;
        }
        k++;
    }
    while (i < n1) {
        ref_res[k] = L[i];
        i++;
        k++;
    }
    while (j < n2) {
        ref_res[k] = R[j];
        j++;
        k++;
    }
}

static void merge_descend(topk_t ref_res[], int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    topk_t L[n1], R[n2];

    for (int i = 0; i < n1; i++) {
        L[i] = ref_res[left + i];
    }
    for (int j = 0; j < n2; j++) {
        R[j] = ref_res[mid + 1 + j];
    }

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (L[i].val >= R[j].val) {
            ref_res[k] = L[i];
            i++;
        } else {
            ref_res[k] = R[j];
            j++;
        }
        k++;
    }
    while (i < n1) {
        ref_res[k] = L[i];
        i++;
        k++;
    }
    while (j < n2) {
        ref_res[k] = R[j];
        j++;
        k++;
    }
}

static void mergeSort(topk_t ref_res[], int left, int right, bool is_ascend) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        mergeSort(ref_res, left, mid, is_ascend);
        mergeSort(ref_res, mid + 1, right, is_ascend);
        if (is_ascend) {
            merge_ascend(ref_res, left, mid, right);
        }else{
            merge_descend(ref_res, left, mid, right);
        }
    }
}

void gen_topk_reference_fp32(float* input_data, float* top_data_ref, int* top_index_ref, int query_vecs_num, int database_vecs_num, int sort_cnt, int descending) {
    topk_t *bottom_vec = (topk_t*)malloc(database_vecs_num * sizeof(topk_t));
    if (!bottom_vec) {
        perror("Memory allocation failed");
        return;
    }

    for (int b = 0; b < query_vecs_num; b++) {
        for (int i = 0; i < database_vecs_num; i++) {
            int offset = b * database_vecs_num + i;
            bottom_vec[i].index = i;
            bottom_vec[i].val = input_data[offset];
        }
        mergeSort(bottom_vec, 0, database_vecs_num - 1, !descending);
        for (int i = 0; i < sort_cnt; i++) {
            top_index_ref[b * sort_cnt + i] = bottom_vec[i].index;
            top_data_ref[b * sort_cnt + i] = bottom_vec[i].val;
        }
    }

    free(bottom_vec);
}

void gen_topk_reference_fp16(float* input_data,
                        fp16* top_data_ref,
                        int* top_index_ref,
                        int query_vecs_num,
                        int database_vecs_num,
                        int sort_cnt,
                        int descending) {
    topk_t *bottom_vec = (topk_t*)malloc(database_vecs_num * sizeof(topk_t));
    if (!bottom_vec) {
        perror("Memory allocation failed");
        return;
    }

    for (int b = 0; b < query_vecs_num; b++) {
        for (int i = 0; i < database_vecs_num; i++) {
            int offset = b * database_vecs_num + i;
            bottom_vec[i].index = i;
            bottom_vec[i].val = input_data[offset];
        }
        mergeSort(bottom_vec, 0, database_vecs_num - 1, !descending);
        for (int i = 0; i < sort_cnt; i++) {
            top_index_ref[b * sort_cnt + i] = bottom_vec[i].index;
            top_data_ref[b * sort_cnt + i] = fp32tofp16(bottom_vec[i].val, 1);
        }
    }

    free(bottom_vec);
}

void matrix_gen_data_norm(fp16* data, fp16* vec, int row_num, int col_num) {
    for (int i = 0; i < row_num; i++) {
        float data_tmp[col_num];
        float l2_norm = 0.0;
        for (int j = 0; j < col_num; j++) {
            data_tmp[j] = ((float)rand() / (float)RAND_MAX);
            l2_norm += data_tmp[j] * data_tmp[j];
        }
        float norm = sqrt(l2_norm);
        for (int j = 0; j < col_num; j++) {
            data[i * col_num + j] = fp32tofp16(data_tmp[j] / norm, 1);
        }
        vec[i] = fp32tofp16(1.0f, 1);
    }
}

void native_calc_L2distance_matrix_fp16(const matmul_param_t* param,
                                   fp16* L_ptr,
                                   fp16* R_ptr,
                                   float* Y_ptr,
                                   fp16* vec_query,
                                   fp16* vec_db) {
    memset(Y_ptr, 0, sizeof(float) * param->L_row_num * param->R_col_num);
    for (int kidx = 0; kidx < param->L_col_num; kidx++) {
        fp16* cur_R = &R_ptr[kidx * param->R_col_num];
        for (int ridx = 0; ridx < param->L_row_num; ridx++) {
            float L_val = (float)fp16tofp32(L_ptr[ridx * param->L_col_num + kidx]);
            for (int cidx = 0; cidx < param->R_col_num; cidx++) {
                Y_ptr[ridx * param->R_col_num + cidx] += L_val * fp16tofp32(cur_R[cidx]);
            }
        }
    }

    for (int i = 0; i < param->L_row_num; i++) {
        for (int j = 0; j < param->R_col_num; j++) {
            Y_ptr[i * param->R_col_num + j] = (-2) * Y_ptr[i * param->R_col_num + j];
            Y_ptr[i * param->R_col_num + j] += fp16tofp32(vec_query[i]);
            Y_ptr[i * param->R_col_num + j] += fp16tofp32(vec_db[j]);
        }
    }
}

static bm_status_t result_compare_fp32(float* tpu_result_similarity,
                          int* tpu_result_index,
                          float* ref_similarity,
                          int* ref_index,
                          int sort_cnt,
                          int query_vecs_num) {
    for (int query_cnt = 0; query_cnt < query_vecs_num; query_cnt++) {
        for (int sort_indx = 0; sort_indx < sort_cnt; sort_indx++) {
            if (fabs((float)tpu_result_similarity[query_cnt * sort_cnt + sort_indx] - ref_similarity[query_cnt * sort_cnt + sort_indx]) > (1e-1)) {
                printf("faiss_indexflatL2 fp32 cpu && tpu result compare failed!\n");
                printf("tpu_res[%d][%d][%d] %f ref_result[%d][%d][%d] %f\n",
                    query_cnt, sort_indx, tpu_result_index[query_cnt * sort_cnt + sort_indx],
                    tpu_result_similarity[query_cnt * sort_cnt + sort_indx],
                    query_cnt, sort_indx, ref_index[query_cnt * sort_cnt + sort_indx],
                    ref_similarity[query_cnt * sort_cnt + sort_indx]);
                return BM_ERR_FAILURE;
            }
        }
    }
    printf("-------------faiss_indexflatL2 fp32 compare succeed-----------\n");
    return BM_SUCCESS;
}

static bm_status_t result_compare_fp16(fp16* tpu_result_similarity,
                          int* tpu_result_index,
                          fp16* ref_similarity,
                          int* ref_index,
                          int sort_cnt,
                          int query_vecs_num) {
    for (int query_cnt = 0; query_cnt < query_vecs_num; query_cnt++) {
        for (int sort_indx = 0; sort_indx < sort_cnt; sort_indx++) {
            if (fabs(fp16tofp32(tpu_result_similarity[query_cnt * sort_cnt + sort_indx]) - fp16tofp32(ref_similarity[query_cnt * sort_cnt + sort_indx])) > (1e-1)) {
                printf("faiss_indexflatL2 fp16 cpu && tpu result compare failed!\n");
                printf("tpu_res[%d][%d][%d] %f ref_result[%d][%d][%d] %f\n",
                    query_cnt, sort_indx, tpu_result_index[query_cnt * sort_cnt + sort_indx],
                    fp16tofp32(tpu_result_similarity[query_cnt * sort_cnt + sort_indx]),
                    query_cnt, sort_indx, ref_index[query_cnt * sort_cnt + sort_indx],
                    fp16tofp32(ref_similarity[query_cnt * sort_cnt + sort_indx]));
                return BM_ERR_FAILURE;
            }
        }
    }
    printf("-------------faiss_indexflatL2 fp16 compare succeed-----------\n");
    return BM_SUCCESS;
}

bm_status_t test_faiss_indexflatL2_fp16(bm_handle_t handle,
                                        int vec_dims,
                                        int query_vecs_num,
                                        int database_vecs_num,
                                        int sort_cnt,
                                        int is_transpose,
                                        int input_dtype,
                                        int output_dtype) {
    printf("database_vecs_num: %d\n", database_vecs_num);
    printf("query_num:         %d\n", query_vecs_num);
    printf("sort_count:        %d\n", sort_cnt);
    printf("data_dims:         %d\n", vec_dims);
    printf("transpose:         %d\n", is_transpose);
    printf("input_dtype:       %d\n", input_dtype);
    printf("output_dtype:      %d\n", output_dtype);
    bm_status_t ret = BM_SUCCESS;
    fp16* input_data = (fp16*)malloc(query_vecs_num * vec_dims * dtype_size((enum bm_data_type_t)input_dtype));
    fp16* db_data = (fp16*)malloc(database_vecs_num * vec_dims * dtype_size((enum bm_data_type_t)input_dtype));
    fp16* db_data_trans = (fp16*)malloc(vec_dims * database_vecs_num * dtype_size((enum bm_data_type_t)input_dtype));
    fp16* vec_query = (fp16*)malloc(1 * query_vecs_num * dtype_size((enum bm_data_type_t)input_dtype));
    fp16* vec_db = (fp16*)malloc(1 * database_vecs_num * dtype_size((enum bm_data_type_t)input_dtype));

    unsigned char* output_dis = (unsigned char*)malloc(query_vecs_num * sort_cnt * dtype_size((enum bm_data_type_t)output_dtype));
    int* output_idx = (int*)malloc(query_vecs_num * sort_cnt * dtype_size(DT_INT32));

    float* blob_Y_ref = (float*)malloc(query_vecs_num * database_vecs_num * sizeof(float));
    unsigned char* blob_dis_ref = (unsigned char*)malloc(query_vecs_num * sort_cnt * dtype_size((enum bm_data_type_t)output_dtype)); //???
    int* blob_inx_ref = (int*)malloc(query_vecs_num * sort_cnt * sizeof(int));

    matrix_gen_data_norm(input_data, vec_query, query_vecs_num, vec_dims);
    matrix_gen_data_norm(db_data, vec_db, database_vecs_num, vec_dims);
    matrix_trans(db_data, db_data_trans, database_vecs_num, vec_dims, (enum bm_data_type_t)input_dtype);
    bm_device_mem_t query_data_dev_mem,
                    db_data_dev_mem,
                    query_L2norm_dev_mem,
                    db_L2norm_dev_mem,
                    buffer_dev_mem,
                    sorted_similarity_dev_mem,
                    sorted_index_dev_mem;

    ret = bm_malloc_device_byte(handle,
                          &query_data_dev_mem,
                          dtype_size((enum bm_data_type_t)input_dtype) * query_vecs_num * vec_dims);
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte query_data_dev_mem failed!\n");
        query_data_dev_mem = BM_MEM_NULL;
        goto free_mem1;
    }
    ret = bm_malloc_device_byte(handle,
                          &db_data_dev_mem,
                          dtype_size((enum bm_data_type_t)input_dtype) * database_vecs_num * vec_dims);
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte db_data_dev_mem failed!\n");
        bm_free_device(handle, query_data_dev_mem);
        db_data_dev_mem = BM_MEM_NULL;
        goto free_mem1;
    }
    ret = bm_malloc_device_byte(handle,
                          &query_L2norm_dev_mem,
                          dtype_size((enum bm_data_type_t)input_dtype) * query_vecs_num * 1);
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte query_L2norm_dev_mem failed!\n");
        bm_free_device(handle, query_data_dev_mem);
        bm_free_device(handle, db_data_dev_mem);
        query_L2norm_dev_mem = BM_MEM_NULL;
        goto free_mem1;
    }
    ret = bm_malloc_device_byte(handle,
                          &db_L2norm_dev_mem,
                          dtype_size((enum bm_data_type_t)input_dtype) * database_vecs_num * 1);
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte db_L2norm_dev_mem failed!\n");
        bm_free_device(handle, query_data_dev_mem);
        bm_free_device(handle, db_data_dev_mem);
        bm_free_device(handle, query_L2norm_dev_mem);
        db_L2norm_dev_mem = BM_MEM_NULL;
        goto free_mem1;
    }
    ret = bm_malloc_device_byte(handle,
                          &buffer_dev_mem,
                          dtype_size((enum bm_data_type_t)DT_FP32) * query_vecs_num * database_vecs_num);
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte buffer_dev_mem failed!\n");
        bm_free_device(handle, query_data_dev_mem);
        bm_free_device(handle, db_data_dev_mem);
        bm_free_device(handle, query_L2norm_dev_mem);
        bm_free_device(handle, db_L2norm_dev_mem);
        buffer_dev_mem = BM_MEM_NULL;
        goto free_mem1;
    }
    ret = bm_malloc_device_byte(handle,
                          &sorted_similarity_dev_mem,
                          dtype_size((enum bm_data_type_t)output_dtype) * query_vecs_num * sort_cnt);
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte sorted_similarity_dev_mem failed!\n");
        bm_free_device(handle, query_data_dev_mem);
        bm_free_device(handle, db_data_dev_mem);
        bm_free_device(handle, query_L2norm_dev_mem);
        bm_free_device(handle, db_L2norm_dev_mem);
        bm_free_device(handle, buffer_dev_mem);
        sorted_similarity_dev_mem = BM_MEM_NULL;
        goto free_mem1;
    }
    ret = bm_malloc_device_byte(handle,
                          &sorted_index_dev_mem,
                          dtype_size((enum bm_data_type_t)DT_INT32) * query_vecs_num * sort_cnt);
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte sorted_index_dev_mem failed!\n");
        bm_free_device(handle, query_data_dev_mem);
        bm_free_device(handle, db_data_dev_mem);
        bm_free_device(handle, query_L2norm_dev_mem);
        bm_free_device(handle, db_L2norm_dev_mem);
        bm_free_device(handle, buffer_dev_mem);
        bm_free_device(handle, sorted_similarity_dev_mem);
        sorted_index_dev_mem = BM_MEM_NULL;
        goto free_mem1;
    }
    ret = bm_memcpy_s2d(handle, query_data_dev_mem, bm_mem_get_system_addr(bm_mem_from_system(input_data)));
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d query_data_dev_mem failed!\n");
        goto free_mem;
    }
    ret = bm_memcpy_s2d(handle, db_data_dev_mem, bm_mem_get_system_addr(bm_mem_from_system(db_data)));
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d db_data_dev_mem failed!\n");
        goto free_mem;
    }
    ret = bm_memcpy_s2d(handle, query_L2norm_dev_mem, bm_mem_get_system_addr(bm_mem_from_system(vec_query)));
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d query_L2norm_dev_mem failed!\n");
        goto free_mem;
    }
    ret = bm_memcpy_s2d(handle, db_L2norm_dev_mem, bm_mem_get_system_addr(bm_mem_from_system(vec_db)));
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d db_L2norm_dev_mem failed!\n");
        goto free_mem;
    }
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);
    ret = bmcv_faiss_indexflatL2(handle,
                           query_data_dev_mem,
                           db_data_dev_mem,
                           query_L2norm_dev_mem,
                           db_L2norm_dev_mem,
                           buffer_dev_mem,
                           sorted_similarity_dev_mem,
                           sorted_index_dev_mem,
                           vec_dims,
                           query_vecs_num,
                           database_vecs_num,
                           sort_cnt,
                           is_transpose,
                           input_dtype,
                           output_dtype);
    gettimeofday(&t2, NULL);
    printf("TPU using time: %ld(us)\n", ((t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec));
    if (BM_SUCCESS != ret) {
        printf("bmcv_faiss_indexflatL2 api error\n");
        goto free_mem;
    }
    ret = bm_memcpy_d2s(handle, bm_mem_get_system_addr(bm_mem_from_system(output_dis)), sorted_similarity_dev_mem);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_d2s sorted_similarity_dev_mem failed!\n");
        goto free_mem;
    }
    ret = bm_memcpy_d2s(handle, bm_mem_get_system_addr(bm_mem_from_system(output_idx)), sorted_index_dev_mem);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_d2s sorted_index_dev_mem failed!\n");
        goto free_mem;
    }
    matmul_param_t param;
    memset(&param, 0, sizeof(matmul_param_t));

    param.L_row_num = query_vecs_num,
    param.L_col_num = vec_dims;
    param.R_col_num = database_vecs_num;
    param.transpose = is_transpose;
    param.L_dtype = (enum bm_data_type_t)input_dtype;
    param.R_dtype = (enum bm_data_type_t)input_dtype;
    param.Y_dtype = (enum bm_data_type_t)output_dtype;

    gettimeofday(&t1, NULL);
    native_calc_L2distance_matrix_fp16(&param, input_data, is_transpose ? db_data_trans : db_data, (float*)blob_Y_ref, vec_query, vec_db);
    if (output_dtype == DT_FP32) {
        gen_topk_reference_fp32(blob_Y_ref, (float *)blob_dis_ref, blob_inx_ref, query_vecs_num, database_vecs_num, sort_cnt, 0);
    } else {
        gen_topk_reference_fp16(blob_Y_ref, (fp16 *)blob_dis_ref, blob_inx_ref, query_vecs_num, database_vecs_num, sort_cnt, 0);
    }
    gettimeofday(&t2, NULL);
    printf("CPU using time: %ld(us)\n", ((t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec));
    if (output_dtype == DT_FP32) {
        ret = result_compare_fp32((float*)output_dis, output_idx, (float*)blob_dis_ref, blob_inx_ref, sort_cnt, query_vecs_num);
    } else {
        ret = result_compare_fp16((fp16 *)output_dis, output_idx, (fp16 *)blob_dis_ref, blob_inx_ref, sort_cnt, query_vecs_num);
    }
free_mem:
    bm_free_device(handle, query_data_dev_mem);
    bm_free_device(handle, db_data_dev_mem);
    bm_free_device(handle, query_L2norm_dev_mem);
    bm_free_device(handle, db_L2norm_dev_mem);
    bm_free_device(handle, buffer_dev_mem);
    bm_free_device(handle, sorted_similarity_dev_mem);
    bm_free_device(handle, sorted_index_dev_mem);
free_mem1:
    free(input_data);
    free(db_data);
    free(db_data_trans);
    free(vec_query);
    free(vec_db);
    free(output_dis);
    free(output_idx);
    free(blob_Y_ref);
    free(blob_dis_ref);
    free(blob_inx_ref);
    return ret;
}

bm_status_t test_faiss_indexflatL2_fp32(bm_handle_t handle,
                                        int vec_dims,
                                        int query_vecs_num,
                                        int database_vecs_num,
                                        int sort_cnt,
                                        int is_transpose,
                                        int input_dtype,
                                        int output_dtype) {
    printf("database_vecs_num: %d\n", database_vecs_num);
    printf("query_num:         %d\n", query_vecs_num);
    printf("sort_count:        %d\n", sort_cnt);
    printf("data_dims:         %d\n", vec_dims);
    printf("transpose:         %d\n", is_transpose);
    printf("input_dtype:       %d\n", input_dtype);
    printf("output_dtype:      %d\n", output_dtype);
    bm_status_t ret = BM_SUCCESS;
    float* input_data = (float*)malloc(query_vecs_num * vec_dims * sizeof(float));
    float* db_data = (float*)malloc(database_vecs_num * vec_dims * sizeof(float));
    float* db_data_trans = (float*)malloc(vec_dims * database_vecs_num * sizeof(float));
    float* vec_query = (float*)malloc(1 * query_vecs_num * sizeof(float));
    float* vec_db = (float*)malloc(1 * database_vecs_num * sizeof(float));

    unsigned char* output_dis = (unsigned char*)malloc(query_vecs_num * sort_cnt * dtype_size((enum bm_data_type_t)output_dtype));
    int* output_idx = (int*)malloc(query_vecs_num * sort_cnt * dtype_size(DT_INT32));

    float* blob_Y_ref = (float*)malloc(query_vecs_num * database_vecs_num * sizeof(float));
    unsigned char *blob_dis_ref = (unsigned char*)malloc(query_vecs_num * sort_cnt * dtype_size((enum bm_data_type_t)output_dtype)); //???
    int *blob_inx_ref = (int*)malloc(query_vecs_num * sort_cnt * sizeof(int));
    fvec_norm_L2sqr_ref(vec_query, input_data, query_vecs_num, vec_dims);
    fvec_norm_L2sqr_ref(vec_db, db_data, database_vecs_num, vec_dims);
    matrix_trans(db_data, db_data_trans, database_vecs_num, vec_dims, (enum bm_data_type_t)input_dtype);
    bm_device_mem_t query_data_dev_mem,
                    db_data_dev_mem,
                    query_L2norm_dev_mem,
                    db_L2norm_dev_mem,
                    buffer_dev_mem,
                    sorted_similarity_dev_mem,
                    sorted_index_dev_mem;

    ret = bm_malloc_device_byte(handle,
                          &query_data_dev_mem,
                          dtype_size((enum bm_data_type_t)input_dtype) * query_vecs_num * vec_dims);
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte query_data_dev_mem failed!\n");
        query_data_dev_mem = BM_MEM_NULL;
        goto free_mem1;
    }
    ret = bm_malloc_device_byte(handle,
                          &db_data_dev_mem,
                          dtype_size((enum bm_data_type_t)input_dtype) * database_vecs_num * vec_dims);
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte db_data_dev_mem failed!\n");
        bm_free_device(handle, query_data_dev_mem);
        db_data_dev_mem = BM_MEM_NULL;
        goto free_mem1;
    }
    ret = bm_malloc_device_byte(handle,
                          &query_L2norm_dev_mem,
                          dtype_size((enum bm_data_type_t)input_dtype) * query_vecs_num * 1);
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte query_L2norm_dev_mem failed!\n");
        bm_free_device(handle, query_data_dev_mem);
        bm_free_device(handle, db_data_dev_mem);
        query_L2norm_dev_mem = BM_MEM_NULL;
        goto free_mem1;
    }
    ret = bm_malloc_device_byte(handle,
                          &db_L2norm_dev_mem,
                          dtype_size((enum bm_data_type_t)input_dtype) * database_vecs_num * 1);
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte db_L2norm_dev_mem failed!\n");
        bm_free_device(handle, query_data_dev_mem);
        bm_free_device(handle, db_data_dev_mem);
        bm_free_device(handle, query_L2norm_dev_mem);
        db_L2norm_dev_mem = BM_MEM_NULL;
        goto free_mem1;
    }
    ret = bm_malloc_device_byte(handle,
                          &buffer_dev_mem,
                          dtype_size((enum bm_data_type_t)DT_FP32) * query_vecs_num * database_vecs_num);
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte buffer_dev_mem failed!\n");
        bm_free_device(handle, query_data_dev_mem);
        bm_free_device(handle, db_data_dev_mem);
        bm_free_device(handle, query_L2norm_dev_mem);
        bm_free_device(handle, db_L2norm_dev_mem);
        buffer_dev_mem = BM_MEM_NULL;
        goto free_mem1;
    }
    ret = bm_malloc_device_byte(handle,
                          &sorted_similarity_dev_mem,
                          dtype_size((enum bm_data_type_t)output_dtype) * query_vecs_num * sort_cnt);
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte sorted_similarity_dev_mem failed!\n");
        bm_free_device(handle, query_data_dev_mem);
        bm_free_device(handle, db_data_dev_mem);
        bm_free_device(handle, query_L2norm_dev_mem);
        bm_free_device(handle, db_L2norm_dev_mem);
        bm_free_device(handle, buffer_dev_mem);
        sorted_similarity_dev_mem = BM_MEM_NULL;
        goto free_mem1;
    }
    ret = bm_malloc_device_byte(handle,
                          &sorted_index_dev_mem,
                          dtype_size((enum bm_data_type_t)DT_INT32) * query_vecs_num * sort_cnt);
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte sorted_index_dev_mem failed!\n");
        bm_free_device(handle, query_data_dev_mem);
        bm_free_device(handle, db_data_dev_mem);
        bm_free_device(handle, query_L2norm_dev_mem);
        bm_free_device(handle, db_L2norm_dev_mem);
        bm_free_device(handle, buffer_dev_mem);
        bm_free_device(handle, sorted_similarity_dev_mem);
        sorted_index_dev_mem = BM_MEM_NULL;
        goto free_mem1;
    }
    ret = bm_memcpy_s2d(handle, query_data_dev_mem, bm_mem_get_system_addr(bm_mem_from_system(input_data)));
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d query_data_dev_mem failed!\n");
        goto free_mem;
    }
    ret = bm_memcpy_s2d(handle, db_data_dev_mem, bm_mem_get_system_addr(bm_mem_from_system(db_data)));
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d db_data_dev_mem failed!\n");
        goto free_mem;
    }
    ret = bm_memcpy_s2d(handle, query_L2norm_dev_mem, bm_mem_get_system_addr(bm_mem_from_system(vec_query)));
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d query_L2norm_dev_mem failed!\n");
        goto free_mem;
    }
    ret = bm_memcpy_s2d(handle, db_L2norm_dev_mem, bm_mem_get_system_addr(bm_mem_from_system(vec_db)));
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_s2d db_L2norm_dev_mem failed!\n");
        goto free_mem;
    }
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);
    ret = bmcv_faiss_indexflatL2(handle,
                           query_data_dev_mem,
                           db_data_dev_mem,
                           query_L2norm_dev_mem,
                           db_L2norm_dev_mem,
                           buffer_dev_mem,
                           sorted_similarity_dev_mem,
                           sorted_index_dev_mem,
                           vec_dims,
                           query_vecs_num,
                           database_vecs_num,
                           sort_cnt,
                           is_transpose,
                           input_dtype,
                           output_dtype);
    gettimeofday(&t2, NULL);
    printf("TPU using time: %ld(us)\n", ((t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec));
    if (BM_SUCCESS != ret) {
        printf("bmcv_faiss_indexflatL2 api error\n");
        goto free_mem;
    }
    ret = bm_memcpy_d2s(handle, bm_mem_get_system_addr(bm_mem_from_system(output_dis)), sorted_similarity_dev_mem);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_d2s sorted_similarity_dev_mem failed!\n");
        goto free_mem;
    }
    ret = bm_memcpy_d2s(handle, bm_mem_get_system_addr(bm_mem_from_system(output_idx)), sorted_index_dev_mem);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_d2s sorted_index_dev_mem failed!\n");
        goto free_mem;
    }
    matmul_param_t param;
    memset(&param, 0, sizeof(matmul_param_t));

    param.L_row_num = query_vecs_num,
    param.L_col_num = vec_dims;
    param.R_col_num = database_vecs_num;
    param.transpose = is_transpose;
    param.L_dtype = (enum bm_data_type_t)input_dtype;
    param.R_dtype = (enum bm_data_type_t)input_dtype;
    param.Y_dtype = (enum bm_data_type_t)output_dtype;

    gettimeofday(&t1, NULL);
    native_calc_L2distance_matrix_fp32(&param, input_data, is_transpose ? db_data_trans : db_data, (float*)blob_Y_ref, vec_query, vec_db);
    if (output_dtype == DT_FP32) {
        gen_topk_reference_fp32(blob_Y_ref, (float *)blob_dis_ref, blob_inx_ref, query_vecs_num, database_vecs_num, sort_cnt, 0);
    } else {
        gen_topk_reference_fp16(blob_Y_ref, (fp16 *)blob_dis_ref, blob_inx_ref, query_vecs_num, database_vecs_num, sort_cnt, 0);
    }
    gettimeofday(&t2, NULL);
    printf("CPU using time: %ld(us)\n", ((t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec));
    if (output_dtype == DT_FP32) {
        ret = result_compare_fp32((float*)output_dis, output_idx, (float*)blob_dis_ref, blob_inx_ref, sort_cnt, query_vecs_num);
    } else {
        ret = result_compare_fp16((fp16 *)output_dis, output_idx, (fp16 *)blob_dis_ref, blob_inx_ref, sort_cnt, query_vecs_num);
    }
free_mem:
    bm_free_device(handle, query_data_dev_mem);
    bm_free_device(handle, db_data_dev_mem);
    bm_free_device(handle, query_L2norm_dev_mem);
    bm_free_device(handle, db_L2norm_dev_mem);
    bm_free_device(handle, buffer_dev_mem);
    bm_free_device(handle, sorted_similarity_dev_mem);
    bm_free_device(handle, sorted_index_dev_mem);
free_mem1:
    free(input_data);
    free(db_data);
    free(db_data_trans);
    free(vec_query);
    free(vec_db);
    free(output_dis);
    free(output_idx);
    free(blob_Y_ref);
    free(blob_dis_ref);
    free(blob_inx_ref);
    return ret;
}

void* test_faiss_indexflatL2(void* args) {
    faiss_indexflatL2_thread_arg_t* faiss_indexflatL2_thread_arg = (faiss_indexflatL2_thread_arg_t*)args;
    int loop = faiss_indexflatL2_thread_arg->loop;
    int database_vecs_num = faiss_indexflatL2_thread_arg->database_vecs_num;
    int query_vecs_num = faiss_indexflatL2_thread_arg->query_vecs_num;
    int sort_cnt = faiss_indexflatL2_thread_arg->sort_cnt;
    int vec_dims = faiss_indexflatL2_thread_arg->vec_dims;
    int is_transpose = faiss_indexflatL2_thread_arg->is_transpose;
    int input_dtype = faiss_indexflatL2_thread_arg->input_dtype;
    int output_dtype = faiss_indexflatL2_thread_arg->output_dtype;
    bm_handle_t handle = faiss_indexflatL2_thread_arg->handle;
    for (int i = 0; i < loop; i++) {
        if(loop > 1) {
            query_vecs_num = 2 + rand() % 50;
            sort_cnt = rand() % 30 + 1;
            database_vecs_num = rand() % 100000 + 1 + query_vecs_num + sort_cnt * 2;
            input_dtype = (rand() % 2 == 0) ? 3 : 5;
            output_dtype = (rand() % 2 == 0) ? 3 : 5;
            is_transpose = input_dtype == 3 ? 0 : is_transpose;
        }
        switch (input_dtype) {
            case DT_FP32:
                if (BM_SUCCESS != test_faiss_indexflatL2_fp32(handle, vec_dims, query_vecs_num, database_vecs_num, sort_cnt, is_transpose, input_dtype, output_dtype)) {
                    printf("------faiss_indexflatL2_fp32_single_test failed------\n");
                    exit(-1);
                }
                break;
            case DT_FP16:
                if (BM_SUCCESS != test_faiss_indexflatL2_fp16(handle, vec_dims, query_vecs_num, database_vecs_num, sort_cnt, is_transpose, input_dtype, output_dtype)) {
                    printf("------faiss_indexflatL2_fp16_single_test failed------\n");
                    exit(-1);
                }
                break;
            default:
                printf("input data_type not support!\n");
                exit(-1);
                break;
        }
    }
    return NULL;
}

int main(int argc, char *args[]) {
    struct timespec tp;
    clock_gettime(0, &tp);
    unsigned int seed = tp.tv_nsec;
    srand(seed);
    printf("random seed = %u\n", seed);
    int thread_num = 1;
    int loop = 1;
    int sort_cnt = rand() % 30 + 1;
    int query_vecs_num = 1 + rand() % 50;
    int database_vecs_num = rand() % 100000 + 1 + query_vecs_num + sort_cnt * 2;
    int vec_dims = 256;
    int input_dtype = (rand() % 2 == 0) ? 3 : 5;
    int output_dtype = (rand() % 2 == 0) ? 3 : 5;
    int is_transpose = rand() % 2;
    is_transpose = input_dtype == 3 ? 0 : is_transpose;

    if (argc == 2 && atoi(args[1]) == -1) {
        printf("usage: %d\n", argc);
        printf("%s thread_num loop_num database_num query_num sort_count data_dims is_transpose input_dtype output_dtype\n", args[0]);
        return 0;
    }
    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) database_vecs_num = atoi(args[3]);
    if (argc > 4) query_vecs_num = atoi(args[4]);
    if (argc > 5) sort_cnt = atoi(args[5]);
    if (argc > 6) vec_dims = atoi(args[6]);
    if (argc > 7) is_transpose = atoi(args[7]);
    if (argc > 8) input_dtype = atoi(args[8]);
    if (argc > 9) output_dtype = atoi(args[9]);

    printf("thread_num:        %d\n", thread_num);
    printf("loop:              %d\n", loop);
    int ret = 0;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (BM_SUCCESS != ret) {
        printf("request dev failed\n");
        return BM_ERR_FAILURE;
    }
    // test for multi-thread
    pthread_t pid[thread_num];
    faiss_indexflatL2_thread_arg_t faiss_indexflatL2_thread_arg[thread_num];
    for (int i = 0; i < thread_num; i++) {
        faiss_indexflatL2_thread_arg[i].loop = loop;
        faiss_indexflatL2_thread_arg[i].database_vecs_num = database_vecs_num;
        faiss_indexflatL2_thread_arg[i].query_vecs_num = query_vecs_num;
        faiss_indexflatL2_thread_arg[i].sort_cnt = sort_cnt;
        faiss_indexflatL2_thread_arg[i].vec_dims = vec_dims;
        faiss_indexflatL2_thread_arg[i].is_transpose = is_transpose;
        faiss_indexflatL2_thread_arg[i].input_dtype = input_dtype;
        faiss_indexflatL2_thread_arg[i].output_dtype = output_dtype;
        faiss_indexflatL2_thread_arg[i].handle = handle;
        if (pthread_create(pid + i, NULL, test_faiss_indexflatL2, faiss_indexflatL2_thread_arg + i) != 0) {
            printf("create thread failed\n");
            return -1;
        }
    }
    for (int i = 0; i < thread_num; i++) {
        int ret = pthread_join(pid[i], NULL);
        if (ret != 0) {
            printf("Thread join failed\n");
            exit(-1);
        }
    }
    bm_dev_free(handle);
    return 0;
}
