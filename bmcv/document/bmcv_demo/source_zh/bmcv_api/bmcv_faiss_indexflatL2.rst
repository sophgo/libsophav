bmcv_faiss_indexflatL2
======================

计算查询向量与数据库向量 L2 距离的平方, 输出前 K （sort_cnt）个最匹配的 L2 距离的平方值及其对应的索引。


**接口形式：**

    .. code-block:: c++

        bm_status_t bmcv_faiss_indexflatL2(
                bm_handle_t     handle,
                bm_device_mem_t input_data_global_addr,
                bm_device_mem_t db_data_global_addr,
                bm_device_mem_t query_L2norm_global_addr,
                bm_device_mem_t db_L2norm_global_addr,
                bm_device_mem_t buffer_global_addr,
                bm_device_mem_t output_sorted_similarity_global_addr,
                bm_device_mem_t output_sorted_index_global_addr,
                int             vec_dims,
                int             query_vecs_num,
                int             database_vecs_num,
                int             sort_cnt,
                int             is_transpose,
                int             input_dtype,
                int             output_dtype);


**输入参数说明：**

* bm_handle_t handle

  输入参数。bm_handle 句柄。

* bm_device_mem_t input_data_global_addr

  输入参数。存放查询向量组成的矩阵的 device 空间。

* bm_device_mem_t db_data_global_addr

  输入参数。存放底库向量组成的矩阵的 device 空间。

* bm_device_mem_t query_L2norm_global_addr

  输入参数。存放计算出的内积值的缓存空间。

* bm_device_mem_t db_L2norm_global_addr

  输入参数。Device addr information of the database norm_L2sqr vector.

* bm_device_mem_t buffer_global_addr

  输入参数。Squared L2 values stored in the buffer

* bm_device_mem_t output_sorted_similarity_global_addr

  输出参数。存放排序后的最匹配的内积值的 device 空间。

* bm_device_mem_t output_sorted_index_global_addr

  输出参数。存储输出内积值对应索引的 device 空间。

* int vec_dims

  输入参数。向量维数。

* int query_vecs_num

  输入参数。查询向量的个数。

* int database_vecs_num

  输入参数。底库向量的个数。

* int sort_cnt

  输入参数。输出的前 sort_cnt 个最匹配的 L2 距离的平方值。

* int is_transpose

  输入参数。0 表示底库矩阵不转置; 1 表示底库矩阵转置。

* int input_dtype

  输入参数。输入数据类型，支持 fp32 和 fp16, 5 表示 fp32, 3 表示 fp16。

* int output_dtype

  输出参数。输出数据类型，支持 fp32 和 fp16, 5 表示 fp32, 3 表示 fp16。


**返回值说明：**

* BM_SUCCESS: 成功

* 其他:失败


**注意事项：**

1. 输入数据(查询向量)和底库数据(底库向量)的数据类型为 float。

2. 输出的排序后的相似度结果的数据类型为 float, 相对应的索引的数据类型为 int。

3. 假设输入数据和底库数据的 L2 范数的平方值已提前计算完成, 并存储在处理器上。

4. 底库数据通常以 database_vecs_num * vec_dims 的形式排布在内存中。此时, 参数 is_transpose 需要设置为 1。

5. 查询向量和数据库向量 L2 距离的平方值越小, 表示两者的相似度越高。因此, 在 TopK 过程中对 L2 距离的平方值按升序排序。

6. database_vecs_num与sort_cnt的取值需要满足条件：database_vecs_num > sort_cnt。

7. 输入参数中 sort_cnt 和 query_vecs_num 需要小于或等于 database_vecs_num。

8. 该接口可通过设置环境变量启用双核计算，运行程序前：export TPU_CORES=2或export TPU_CORES=both即可。

**示例代码：**

    .. code-block:: c++

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
          bm_device_mem_t query_data_dev_mem,
                          db_data_dev_mem,
                          query_L2norm_dev_mem,
                          db_L2norm_dev_mem,
                          buffer_dev_mem,
                          sorted_similarity_dev_mem,
                          sorted_index_dev_mem;

          bm_malloc_device_byte(handle,
                              &query_data_dev_mem,
                              dtype_size((enum bm_data_type_t)input_dtype) * query_vecs_num * vec_dims);
          bm_malloc_device_byte(handle,
                              &db_data_dev_mem,
                              dtype_size((enum bm_data_type_t)input_dtype) * database_vecs_num * vec_dims);
          bm_malloc_device_byte(handle,
                              &query_L2norm_dev_mem,
                              dtype_size((enum bm_data_type_t)input_dtype) * query_vecs_num * 1);
          bm_malloc_device_byte(handle,
                              &db_L2norm_dev_mem,
                              dtype_size((enum bm_data_type_t)input_dtype) * database_vecs_num * 1);

          bm_malloc_device_byte(handle,
                              &buffer_dev_mem,
                              dtype_size((enum bm_data_type_t)DT_FP32) * query_vecs_num * database_vecs_num);
          bm_malloc_device_byte(handle,
                              &sorted_similarity_dev_mem,
                              dtype_size((enum bm_data_type_t)output_dtype) * query_vecs_num * sort_cnt);
          bm_malloc_device_byte(handle,
                              &sorted_index_dev_mem,
                              dtype_size((enum bm_data_type_t)DT_INT32) * query_vecs_num * sort_cnt);
          bm_memcpy_s2d(handle,
                      query_data_dev_mem,
                      bm_mem_get_system_addr(bm_mem_from_system(input_data)));
          bm_memcpy_s2d(handle,
                      db_data_dev_mem,
                      bm_mem_get_system_addr(bm_mem_from_system(db_data)));
          bm_memcpy_s2d(handle,
                      query_L2norm_dev_mem,
                      bm_mem_get_system_addr(bm_mem_from_system(vec_query)));
          bm_memcpy_s2d(handle,
                      db_L2norm_dev_mem,
                      bm_mem_get_system_addr(bm_mem_from_system(vec_db)));
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
          bm_memcpy_d2s(handle,
                      bm_mem_get_system_addr(bm_mem_from_system(output_dis)),
                      sorted_similarity_dev_mem);
          bm_memcpy_d2s(handle,
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

          bm_free_device(handle, query_data_dev_mem);
          bm_free_device(handle, db_data_dev_mem);
          bm_free_device(handle, query_L2norm_dev_mem);
          bm_free_device(handle, db_L2norm_dev_mem);
          bm_free_device(handle, buffer_dev_mem);
          bm_free_device(handle, sorted_similarity_dev_mem);
          bm_free_device(handle, sorted_index_dev_mem);

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