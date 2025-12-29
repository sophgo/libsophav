bmcv_faiss_indexPQ_SDC
======================

该接口通过检索向量编码和底库编码在 sdc_table 中查表并累加，输出前 K (sort_cnt) 个最匹配的向量索引及其对应的距离。


**接口形式：**

    .. code-block:: c++

        bm_status_t bmcv_faiss_indexPQ_SDC(
                bm_handle_t handle,
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
                int IP_metric);


**输入参数说明：**

* bm_handle_t handle

  输入参数。bm_handle 句柄。

* bm_device_mem_t sdc_table_input_dev

  输入参数。存放对称距离表的 device 空间。

* bm_device_mem_t nxcodes_input_dev

  输入参数。存放检索向量编码的 device 空间。

* bm_device_mem_t nycodes_input_dev

  输入参数。存放底库编码的 device 空间。

* bm_device_mem_t distance_output_dev

  输出参数。存放输出距离的 device 空间。

* bm_device_mem_t index_output_dev

  输出参数。存放输出排序的 device 空间。

* int slice_num

  输入参数。原始维度切分数量。

* int centroids_num

  输入参数。聚类中心的数量。

* int database_num

  输入参数。数据底库的数量。

* int query_num

  输入参数。检索向量的数量。

* int sort_cnt

  输入参数。输出的前 sort_cnt 个最匹配底库向量。

* int IP_metric

  输入参数。0 表示L2距离计算; 1 表示IP距离计算。


**返回值说明:**

* BM_SUCCESS: 成功

* 其他:失败


**注意事项：**

1、输入数据 (查询向量) 和对称距离表的数据类型为 float，底库数据 (底库编码)的数据类型为uint8，存储在设备内存上。

2、输出的排序后的相似度结果的数据类型为 float, 相对应的索引的数据类型为 int，存储在设备内存上。

3、SDC检索过程中 metric 的选择没有区别，因为距离在于输入的sdc table，主要区别在于其 topk 结果是降序，L2的结果为升序。

4、查询向量和数据库向量 L2 距离越小, 表示两者的相似度越高。输出 L2 topk距离按升序排序。

5、查询向量和数据库向量 IP 距离越大, 表示两者的相似度越高。输出 IP topk距离按降序排序。

6、faiss系列算子有多个输入参数，每个参数都有一个使用范围限制，超过该范围的入参对应tpu输出会出错，我们选择了三个主要参数做了测试，固定其中两个维度，测试了第三个维度的最大值，测试结果如下表格所示：

+-----------+--------------+-------------------+
| query_num | vec_dims     | max_database_num  |
+===========+==============+===================+
| 1         | 128          | 3500万            |
+-----------+--------------+-------------------+
| 1         | 256          | 3500万            |
+-----------+--------------+-------------------+
| 1         | 512          | 3500万            |
+-----------+--------------+-------------------+
| 64        | 128          | 250万             |
+-----------+--------------+-------------------+
| 64        | 256          | 250万             |
+-----------+--------------+-------------------+
| 64        | 512          | 250万             |
+-----------+--------------+-------------------+
| 128       | 128          | 130万             |
+-----------+--------------+-------------------+
| 128       | 256          | 130万             |
+-----------+--------------+-------------------+
| 128       | 512          | 130万             |
+-----------+--------------+-------------------+
| 256       | 128          | 70万              |
+-----------+--------------+-------------------+
| 256       | 256          | 70万              |
+-----------+--------------+-------------------+
| 256       | 512          | 70万              |
+-----------+--------------+-------------------+

+--------------+--------------+----------------+
| database_num | vec_dims     | max_query_num  |
+==============+==============+================+
| 1000         | 128          | 1000           |
+--------------+--------------+----------------+
| 1000         | 256          | 1000           |
+--------------+--------------+----------------+
| 1000         | 512          | 1000           |
+--------------+--------------+----------------+
| 1万          | 128          | 1万            |
+--------------+--------------+----------------+
| 1万          | 256          | 1万            |
+--------------+--------------+----------------+
| 1万          | 512          | 1万            |
+--------------+--------------+----------------+
| 10万         | 128          | 1875           |
+--------------+--------------+----------------+
| 10万         | 256          | 1872           |
+--------------+--------------+----------------+
| 10万         | 512          | 1869           |
+--------------+--------------+----------------+

+--------------+-----------------+--------------+
| database_num | query_num       | max_vec_dims |
+==============+=================+==============+
| 1万          | 1               | 512          |
+--------------+-----------------+--------------+
| 1万          | 64              | 512          |
+--------------+-----------------+--------------+
| 1万          | 128             | 512          |
+--------------+-----------------+--------------+
| 1万          | 256             | 512          |
+--------------+-----------------+--------------+
| 10万         | 1               | 512          |
+--------------+-----------------+--------------+
| 10万         | 32              | 512          |
+--------------+-----------------+--------------+
| 10万         | 64              | 512          |
+--------------+-----------------+--------------+
| 10万         | 128             | 512          |
+--------------+-----------------+--------------+
| 10万         | 256             | 512          |
+--------------+-----------------+--------------+
| 100万        | 1               | 512          |
+--------------+-----------------+--------------+
| 100万        | 32              | 512          |
+--------------+-----------------+--------------+
| 100万        | 64              | 512          |
+--------------+-----------------+--------------+
| 100万        | 128             | 512          |
+--------------+-----------------+--------------+


**示例代码**


    .. code-block:: c++

      #include "bmcv_api_ext_c.h"
      #include "test_misc.h"
      #include <stdio.h>
      #include <stdlib.h>
      #include <time.h>
      #include <assert.h>
      #include <sys/time.h>

      int main() {
          int sort_cnt = 100;
          int query_num = 1;
          int slice_m = 32;
          int ksub = 256;
          int database_num = 2000000;
          int input_dtype = 5; // 5: float
          int output_dtype = 5;
          int IP_metric = 0;
          int show_result = 1;
          struct timespec tp;
          clock_gettime(CLOCK_REALTIME, &tp);
          unsigned int seed = tp.tv_nsec;

          bm_handle_t handle;
          bm_status_t ret = bm_dev_request(&handle, 0);
          if (ret != BM_SUCCESS)
          {
              printf("request dev failed\n");
              return BM_ERR_FAILURE;
          }

          srand(seed);
          int round = 1;
          fp16 *sdc_table_input_sys_fp16 = (fp16*)malloc(slice_m * ksub * ksub * sizeof(fp16));
          float *sdc_table_input_sys_fp32 = (float*)malloc(slice_m * ksub * ksub * sizeof(float));

          unsigned char *nxcodes_input_sys = (unsigned char*)malloc(query_num * slice_m);
          unsigned char *nycodes_input_sys = (unsigned char*)malloc(database_num * slice_m);
          unsigned char *distance_output_sys = (unsigned char*)malloc(query_num * database_num * dtype_size((enum bm_data_type_t )output_dtype));
          int *index_output_sys = (int*)malloc(query_num * database_num * sizeof(int));

          for (int i = 0; i < slice_m; i++) {
              for (int j = 0; j < ksub; j++) {
                  for (int n = 0; n < ksub; n++) {
                      float value = (n > j) ? (float)rand() / RAND_MAX * 20.0 : 0.0;
                      sdc_table_input_sys_fp32[i * ksub * ksub + j * ksub + n] = value;
                      sdc_table_input_sys_fp16[i * ksub * ksub + j * ksub + n] = fp32tofp16(value, round);
                  }
              }
          }
          for (int i = 0; i < query_num; i++) {
              for (int j = 0; j < slice_m; j++) {
                  nxcodes_input_sys[i * slice_m + j] = rand() % 256;
              }
          }
          for (int i = 0; i < database_num; i++) {
              for (int j = 0; j < slice_m; j++) {
                  nycodes_input_sys[i * slice_m + j] = rand() % 256;
              }
          }

          int sdc_table_size = slice_m * ksub * ksub * dtype_size((enum bm_data_type_t )input_dtype);
          int nxcodes_size = query_num * slice_m;
          int nycodes_size = database_num * slice_m;
          int output_distance_size = query_num * database_num * dtype_size((enum bm_data_type_t )output_dtype);
          int output_index_size = query_num * database_num * sizeof(int);

          bm_device_mem_t sdc_table_input_dev, nxcodes_input_dev, nycodes_input_dev, distance_output_dev, index_output_dev;

          bm_malloc_device_byte(handle, &sdc_table_input_dev, sdc_table_size);
          bm_malloc_device_byte(handle, &nxcodes_input_dev, nxcodes_size);
          bm_malloc_device_byte(handle, &nycodes_input_dev, nycodes_size);
          bm_malloc_device_byte(handle, &distance_output_dev, output_distance_size);
          bm_malloc_device_byte(handle, &index_output_dev, output_index_size);

          if (input_dtype == DT_FP16) {
              bm_memcpy_s2d(handle, sdc_table_input_dev, sdc_table_input_sys_fp16);
          } else {
              bm_memcpy_s2d(handle, sdc_table_input_dev, sdc_table_input_sys_fp32);
          }
          bm_memcpy_s2d(handle, nxcodes_input_dev, nxcodes_input_sys);
          bm_memcpy_s2d(handle, nycodes_input_dev, nycodes_input_sys);

          struct timeval t1, t2;
          gettimeofday(&t1, NULL);
          ret = bmcv_faiss_indexPQ_SDC_ext(handle,
                                          sdc_table_input_dev,
                                          nxcodes_input_dev,
                                          nycodes_input_dev,
                                          distance_output_dev,
                                          index_output_dev,
                                          slice_m, ksub, database_num, query_num, sort_cnt, IP_metric, input_dtype, output_dtype);
          gettimeofday(&t2, NULL);
          printf("TPU using time(us): %ld(us)\n", (t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec);
          printf("TPU using time(ms): %ld(ms)\n", ((t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec) / 1000);

          if(ret != BM_SUCCESS){
              bm_free_device(handle, sdc_table_input_dev);
              bm_free_device(handle, nxcodes_input_dev);
              bm_free_device(handle, nycodes_input_dev);
              bm_free_device(handle, distance_output_dev);
              bm_free_device(handle, index_output_dev);

              free(sdc_table_input_sys_fp32);
              free(sdc_table_input_sys_fp16);
              free(nxcodes_input_sys);
              free(nycodes_input_sys);
              free(distance_output_sys);
              free(index_output_sys);

              bm_dev_free(handle);
              return BM_ERR_FAILURE;
          }

          bm_memcpy_d2s(handle, distance_output_sys, distance_output_dev);
          bm_memcpy_d2s(handle, index_output_sys, index_output_dev);

          if (show_result) {
              printf("SDCsearch result:\n");
              for (int i = 0; i < sort_cnt; i++) {
                  printf("top: %d\n", i + 1);
                  printf("index: %d\t", index_output_sys[i]);
                  if (output_dtype == DT_FP16) {
                      printf("distance: %f\n", fp16tofp32(((fp16*)distance_output_sys)[i]));
                  } else {
                      printf("distance: %f\n", ((float*)distance_output_sys)[i]);
                  }
              }
          }

          bm_free_device(handle, sdc_table_input_dev);
          bm_free_device(handle, nxcodes_input_dev);
          bm_free_device(handle, nycodes_input_dev);
          bm_free_device(handle, distance_output_dev);
          bm_free_device(handle, index_output_dev);

          free(sdc_table_input_sys_fp32);
          free(sdc_table_input_sys_fp16);
          free(nxcodes_input_sys);
          free(nycodes_input_sys);
          free(distance_output_sys);
          free(index_output_sys);

          bm_dev_free(handle);
          return 0;
      }

