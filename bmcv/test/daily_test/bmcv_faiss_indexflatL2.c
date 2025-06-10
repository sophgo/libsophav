#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "bmcv_api_ext_c.h"
#include "test_misc.h"
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

void fvec_norm_L2sqr_ref(float* vec, float* matrix, int row_num, int col_num) {
    for (int i = 0; i < row_num; i++)
        for (int j = 0; j < col_num; j++) {
            vec[i] += matrix[i * col_num + j] * matrix[i * col_num + j];
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

void matrix_gen_data(float* data, u32 len) {
    for (u32 i = 0; i < len; i++) {
        data[i] = ((float)rand() / (float)RAND_MAX);
    }
}

int main() {
    int sort_cnt = 100;
    int database_vecs_num = 20000;
    int query_vecs_num = 1;
    int vec_dims = 256;
    int is_transpose = 1;
    int input_dtype = 5;
    int output_dtype = 5;

    int ret;

    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (BM_SUCCESS != ret) {
        printf("request dev failed\n");
        return BM_ERR_FAILURE;
    }

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

    matrix_gen_data(input_data, query_vecs_num * vec_dims);
    matrix_gen_data(db_data, vec_dims * database_vecs_num);
    matrix_trans(db_data, db_data_trans, database_vecs_num, vec_dims, (enum bm_data_type_t)input_dtype);
    fvec_norm_L2sqr_ref(vec_query, input_data, query_vecs_num, vec_dims);
    fvec_norm_L2sqr_ref(vec_db, db_data, database_vecs_num, vec_dims);
    bm_device_mem_u64_t query_data_dev_mem,
                    db_data_dev_mem,
                    query_L2norm_dev_mem,
                    db_L2norm_dev_mem,
                    buffer_dev_mem,
                    sorted_similarity_dev_mem,
                    sorted_index_dev_mem;

    bm_malloc_device_byte_u64(handle,
                        &query_data_dev_mem,
                        dtype_size((enum bm_data_type_t)input_dtype) * query_vecs_num * vec_dims);
    bm_malloc_device_byte_u64(handle,
                        &db_data_dev_mem,
                        dtype_size((enum bm_data_type_t)input_dtype) * database_vecs_num * vec_dims);
    bm_malloc_device_byte_u64(handle,
                        &query_L2norm_dev_mem,
                        dtype_size((enum bm_data_type_t)input_dtype) * query_vecs_num * 1);
    bm_malloc_device_byte_u64(handle,
                        &db_L2norm_dev_mem,
                        dtype_size((enum bm_data_type_t)input_dtype) * database_vecs_num * 1);

    bm_malloc_device_byte_u64(handle,
                        &buffer_dev_mem,
                        dtype_size((enum bm_data_type_t)DT_FP32) * query_vecs_num * database_vecs_num);
    bm_malloc_device_byte_u64(handle,
                        &sorted_similarity_dev_mem,
                        dtype_size((enum bm_data_type_t)output_dtype) * query_vecs_num * sort_cnt);
    bm_malloc_device_byte_u64(handle,
                        &sorted_index_dev_mem,
                        dtype_size((enum bm_data_type_t)DT_INT32) * query_vecs_num * sort_cnt);
    bm_memcpy_s2d_u64(handle,
                query_data_dev_mem,
                bm_mem_get_system_addr(bm_mem_from_system(input_data)));
    bm_memcpy_s2d_u64(handle,
                db_data_dev_mem,
                bm_mem_get_system_addr(bm_mem_from_system(db_data)));
    bm_memcpy_s2d_u64(handle,
                query_L2norm_dev_mem,
                bm_mem_get_system_addr(bm_mem_from_system(vec_query)));
    bm_memcpy_s2d_u64(handle,
                db_L2norm_dev_mem,
                bm_mem_get_system_addr(bm_mem_from_system(vec_db)));
    ret = bmcv_faiss_indexflatL2_u64(handle,
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
    bm_memcpy_d2s_u64(handle,
                bm_mem_get_system_addr(bm_mem_from_system(output_dis)),
                sorted_similarity_dev_mem);
    bm_memcpy_d2s_u64(handle,
                bm_mem_get_system_addr(bm_mem_from_system(output_idx)),
                sorted_index_dev_mem);
    matmul_param_t param;
    memset(&param, 0, sizeof(matmul_param_t));

    param.L_row_num = query_vecs_num,
    param.L_col_num = vec_dims;
    param.R_col_num = database_vecs_num;
    param.transpose = is_transpose;
    param.L_dtype = (enum bm_data_type_t)input_dtype;
    param.R_dtype = (enum bm_data_type_t)input_dtype;
    param.Y_dtype = (enum bm_data_type_t)output_dtype;

    bm_free_device_u64(handle, query_data_dev_mem);
    bm_free_device_u64(handle, db_data_dev_mem);
    bm_free_device_u64(handle, query_L2norm_dev_mem);
    bm_free_device_u64(handle, db_L2norm_dev_mem);
    bm_free_device_u64(handle, buffer_dev_mem);
    bm_free_device_u64(handle, sorted_similarity_dev_mem);
    bm_free_device_u64(handle, sorted_index_dev_mem);

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

    bm_dev_free(handle);
    return 0;
}