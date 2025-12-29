bmcv_faiss_indexflatIP
======================

计算查询向量与数据库向量的内积距离, 输出前 K （sort_cnt） 个最匹配的内积距离值及其对应的索引。


**接口形式：**

    .. code-block:: c++

        bm_status_t bmcv_faiss_indexflatIP(
                bm_handle_t     handle,
                bm_device_mem_t input_data_global_addr,
                bm_device_mem_t db_data_global_addr,
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

* bm_device_mem_t buffer_global_addr

  输入参数。存放计算出的内积值的缓存空间。

* bm_device_mem_t output_sorted_similarity_global_addr

  输出参数。存放排序后的最匹配的内积值的 device 空间。

* bm_device_mem_t output_sorted_index_global_add

  输出参数。存储输出内积值对应索引的 device 空间。

* int vec_dims

  输入参数。向量维数。

* int query_vecs_num

  输入参数。查询向量的个数。

* int database_vecs_num

  输入参数。底库向量的个数。

* int sort_cnt

  输入参数。输出的前 sort_cnt 个最匹配的内积值。

* int is_transpose

  输入参数。0 表示底库矩阵不转置; 1 表示底库矩阵转置。

* int input_dtype

  输入参数。输入数据类型，支持 fp32,fp16,char, 5 表示fp32， 3 表示fp16，1 表示char。

* int output_dtype

  输出参数。输出数据类型，支持 fp32,fp16,char, 5 表示fp32， 3 表示fp16，9 表示char。


**返回值说明：**

* BM_SUCCESS: 成功

* 其他:失败


**注意事项：**

1. 输入数据(查询向量)和底库数据(底库向量)的数据类型为 fp32，fp16 或 char。

2. 输出的排序后的相似度的数据类型为 fp32，fp16或int, 相对应的索引的数据类型为 int。

3. 底库数据通常以 database_vecs_num * vec_dims 的形式排布在内存中。此时, 参数 is_transpose 需要设置为 1。

4. 查询向量和数据库向量内积距离值越大, 表示两者的相似度越高。因此, 在 TopK 过程中对内积距离值按降序排序。

5. 输入参数中 sort_cnt 和 query_vecs_num 需要小于或等于 database_vecs_num。

6. 该接口可通过设置环境变量启用双核计算，运行程序前：export TPU_CORES=2或export TPU_CORES=both即可。


**示例代码：**

    .. code-block:: c++

      #include <math.h>
      #include <stdio.h>
      #include <stdlib.h>
      #include <stdint.h>
      #include "bmcv_api_ext_c.h"
      #include "test_misc.h"

      int main() {
          int sort_cnt = 100;
          int database_vecs_num = 10000;
          int query_vecs_num = 1;
          int vec_dims = 256;
          int is_transpose = 1;
          int ret;

          int input_dtype = 1;
          int output_dtype = 9;
          bm_handle_t handle;
          ret = bm_dev_request(&handle, 0);
          if (BM_SUCCESS != ret) {
              printf("request dev failed\n");
              return BM_ERR_FAILURE;
          }

          int i, j;
          signed char* input_data = (signed char*)malloc(query_vecs_num * vec_dims * sizeof(signed char));
          signed char* db_data = (signed char*)malloc(database_vecs_num * vec_dims * sizeof(signed char));
          int* output_similarity = (int*)malloc(query_vecs_num * sort_cnt * sizeof(int));
          signed char** input_content_vec = (signed char**)malloc(query_vecs_num * sizeof(signed char*));
          for (i = 0; i < query_vecs_num; i++) {
              input_content_vec[i] = (signed char*)malloc(vec_dims * sizeof(signed char));
              for (j = 0; j < vec_dims; j++) {
                  #ifdef __linux__
                  signed char temp_val = random() % 20 - 10;
                  #else
                  signed char temp_val = rand() % 20 - 10;
                  #endif
                  input_content_vec[i][j] = temp_val;
              }
          }
          signed char** db_content_vec = (signed char**)malloc((is_transpose ? database_vecs_num : vec_dims) * sizeof(signed char*));
          signed char** db_content_vec_trans = (signed char**)malloc((is_transpose ? vec_dims : database_vecs_num) * sizeof(signed char*));

          for(i = 0; i < vec_dims; i++) {
              db_content_vec_trans[i] = (signed char*)malloc(database_vecs_num * sizeof(signed char));
          }
          for (i = 0; i < database_vecs_num; i++) {
              db_content_vec[i] = (signed char*)malloc(vec_dims * sizeof(signed char));
              for (j = 0; j < vec_dims; j++) {
                  #ifdef __linux__
                  signed char temp_val = random() % 20 + 1;
                  #else
                  signed char temp_val = rand() % 20 - 10;
                  #endif
                  db_content_vec[i][j] = temp_val;
                  db_content_vec_trans[j][i] = temp_val;
              }
          }

          for (i = 0; i < query_vecs_num; ++i) {
              for (j = 0; j < vec_dims; ++j) {
                  input_data[i * vec_dims + j] = input_content_vec[i][j];
              }
          }

          for (i = 0; i < database_vecs_num; ++i) {
              for (j = 0; j < vec_dims; ++j) {
                  db_data[i * vec_dims + j] = db_content_vec[i][j];
              }
          }

          int* output_index = (int*)malloc(query_vecs_num * sort_cnt * sizeof(int));
          int** ref_result = (int**)calloc(query_vecs_num, sizeof(int*));
          for (i = 0; i < query_vecs_num; i++) {
              ref_result[i] = (int*)calloc(database_vecs_num, sizeof(int));
          }
          bm_device_mem_t input_data_global_addr_device,
                          db_data_global_addr_device,
                          buffer_global_addr_device,
                          output_sorted_similarity_global_addr_device,
                          output_sorted_index_global_addr_device;
          bm_malloc_device_byte(handle,
                                &input_data_global_addr_device,
                                dtype_size((enum bm_data_type_t)input_dtype) * query_vecs_num * vec_dims);
          bm_malloc_device_byte(handle,
                                &db_data_global_addr_device,
                                dtype_size((enum bm_data_type_t)input_dtype) * database_vecs_num * vec_dims);
          bm_malloc_device_byte(handle,
                                &buffer_global_addr_device,
                                dtype_size((enum bm_data_type_t)DT_FP32) * query_vecs_num * database_vecs_num);
          bm_malloc_device_byte(handle,
                                &output_sorted_similarity_global_addr_device,
                                dtype_size((enum bm_data_type_t)output_dtype) * query_vecs_num * sort_cnt);
          bm_malloc_device_byte(handle,
                                &output_sorted_index_global_addr_device,
                                dtype_size(DT_INT32) * query_vecs_num * sort_cnt);
          bm_memcpy_s2d(handle,
                        input_data_global_addr_device,
                        bm_mem_get_system_addr(bm_mem_from_system(input_data)));
          bm_memcpy_s2d(handle,
                        db_data_global_addr_device,
                        bm_mem_get_system_addr(bm_mem_from_system(db_data)));

          ret = bmcv_faiss_indexflatIP(handle,
                                input_data_global_addr_device,
                                db_data_global_addr_device,
                                buffer_global_addr_device,
                                output_sorted_similarity_global_addr_device,
                                output_sorted_index_global_addr_device,
                                vec_dims,
                                query_vecs_num,
                                database_vecs_num,
                                sort_cnt,
                                is_transpose,
                                input_dtype,
                                output_dtype);

          bm_memcpy_d2s(handle,
                        bm_mem_get_system_addr(bm_mem_from_system(output_similarity)),
                        output_sorted_similarity_global_addr_device);
          bm_memcpy_d2s(handle,
                        bm_mem_get_system_addr(bm_mem_from_system(output_index)),
                        output_sorted_index_global_addr_device);

          bm_free_device(handle, input_data_global_addr_device);
          bm_free_device(handle, db_data_global_addr_device);
          bm_free_device(handle, buffer_global_addr_device);
          bm_free_device(handle, output_sorted_similarity_global_addr_device);
          bm_free_device(handle, output_sorted_index_global_addr_device);

          free(input_data);
          free(db_data);
          free(output_similarity);
          for (i = 0; i < query_vecs_num; ++i) {
              free(input_content_vec[i]);
              free(ref_result[i]);
          }
          for(i = 0; i < query_vecs_num; ++i){
              free(db_content_vec[i]);
          }
          free(input_content_vec);
          free(db_content_vec);
          free(output_index);
          free(ref_result);


          bm_dev_free(handle);
          return 0;
      }