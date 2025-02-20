bmcv_gemm_ext
=============

该接口可以实现 fp32/fp16 类型矩阵的通用乘法计算，如下公式：

  .. math::

      Y = \alpha\times A\times B + \beta\times C

其中，A、B、C、Y均为矩阵，:math:`\alpha` 和 :math:`\beta` 均为常系数


**接口形式：**

    .. code-block:: c

        bm_status_t bmcv_gemm_ext(
                    bm_handle_t handle,
                    bool is_A_trans,
                    bool is_B_trans,
                    int M,
                    int N,
                    int K,
                    float alpha,
                    bm_device_mem_t A,
                    bm_device_mem_t B,
                    float beta,
                    bm_device_mem_t C,
                    bm_device_mem_t Y,
                    bm_image_data_format_ext input_dtype,
                    bm_image_data_format_ext output_dtype);


**参数说明：**

* bm_handle_t handle

  输入参数。bm_handle 句柄

* bool is_A_trans

  输入参数。设定矩阵 A 是否转置

* bool is_B_trans

  输入参数。设定矩阵 B 是否转置

* int M

  输入参数。矩阵 A、C、Y 的行数

* int N

  输入参数。矩阵 B、C、Y 的列数

* int K

  输入参数。矩阵 A 的列数和矩阵 B 的行数

* float alpha

  输入参数。数乘系数

* bm_device_mem_t A

  输入参数。根据数据存放位置保存左矩阵 A 数据的 device 地址，需在使用前完成数据s2d搬运。

* bm_device_mem_t B

  输入参数。根据数据存放位置保存右矩阵 B 数据的 device 地址，需在使用前完成数据s2d搬运。

* float beta

  输入参数。数乘系数。

* bm_device_mem_t C

  输入参数。根据数据存放位置保存矩阵 C 数据的 device 地址，需在使用前完成数据s2d搬运。

* bm_device_mem_t Y

  输出参数。矩阵 Y 数据的 device 地址，保存输出结果。

* bm_image_data_format_ext input_dtype

  输入参数。输入矩阵A、B、C的数据类型。支持输入FP16-输出FP16或FP32，输入FP32-输出FP32。

* bm_image_data_format_ext output_dtype

  输入参数。输出矩阵Y的数据类型。


**返回值说明:**

* BM_SUCCESS: 成功

* 其他: 失败


**注意事项：**

1. 该接口在 FP16 输入、A 矩阵转置的情况下，M 仅支持小于等于 64 的取值。

2. 该接口不支持 FP32 输入且 FP16 输出。


**示例代码**

    .. code-block:: c

      #include "bmcv_api_ext_c.h"
      #include <stdio.h>
      #include <stdint.h>
      #include <stdlib.h>
      #include <string.h>
      #include <math.h>

      #define GEMM_INPUT_NUM 3
      #define GEMM_OUTPUT_NUM 1


      static int assign_values_to_matrix(float* matrix, int size)
      {
          int i;

          if (matrix == NULL || size <= 0) {
              printf("the assign_values_to_matrix func error!\n");
              return -1;
          }

          for (i = 0; i < size; ++i) {
              matrix[i] = (rand() % 100) * 0.01f;
          }

          return 0;
      }


      int main()
      {
          int M = 1 + rand() % 800;
          int N = 1 + rand() % 800;
          int K = 1 + rand() % 800;
          int rand_sign_a = (rand() % 2 == 0) ? 1 : -1;
          int rand_sign_b = (rand() % 2 == 0) ? 1 : -1;
          float alpha = rand_sign_a * (rand() % 100) * 0.05;
          float beta  = rand_sign_b * (rand() % 100) * 0.05;
          bool is_A_trans = rand() % 2;
          bool is_B_trans = rand () % 2;
          int ret = 0;
          bm_handle_t handle;

          if (is_A_trans) {
              is_B_trans = true;
          }

          ret = bm_dev_request(&handle, 0);
          if (ret) {
              printf("bm_dev_request failed. ret = %d\n", ret);
              return ret;
          }


          float* A = (float*)malloc(M * K * sizeof(float));
          float* B = (float*)malloc(N * K * sizeof(float));
          float* C = (float*)malloc(M * N * sizeof(float));
          float* tpu_C = (float*)malloc(M * N * sizeof(float));
          bm_image_data_format_ext in_dtype, out_dtype;

          ret = assign_values_to_matrix(A, M * K);
          ret = assign_values_to_matrix(B, N * K);
          ret = assign_values_to_matrix(C, M * N);
          memset(tpu_C, 0.f, sizeof(float) * M * N);

          in_dtype = DATA_TYPE_EXT_FLOAT32;
          out_dtype = DATA_TYPE_EXT_FLOAT32;
          memset(tpu_C, 0.f, sizeof(float) * M * N);

          if (in_dtype == DATA_TYPE_EXT_FP16 && is_A_trans && M > 64) {
              printf("Error! It only support M <= 64 when A is trans and input_dtype is FP16\n");
              return -1;
          }

          unsigned short* A_fp16 = (unsigned short*)malloc(M * K * sizeof(unsigned short));
          unsigned short* B_fp16 = (unsigned short*)malloc(N * K * sizeof(unsigned short));
          unsigned short* C_fp16 = (unsigned short*)malloc(M * N * sizeof(unsigned short));
          unsigned short* Y_fp16 = (unsigned short*)malloc(M * N * sizeof(unsigned short));
          bm_device_mem_t input_dev_buffer[GEMM_INPUT_NUM];
          bm_device_mem_t output_dev_buffer[GEMM_OUTPUT_NUM];

          ret = bm_malloc_device_byte(handle, &input_dev_buffer[0], M * K * sizeof(float));
          ret = bm_malloc_device_byte(handle, &input_dev_buffer[1], N * K * sizeof(float));
          ret = bm_malloc_device_byte(handle, &input_dev_buffer[2], M * N * sizeof(float));
          ret = bm_memcpy_s2d(handle, input_dev_buffer[0], (void*)A);
          ret = bm_memcpy_s2d(handle, input_dev_buffer[1], (void*)B);
          ret = bm_memcpy_s2d(handle, input_dev_buffer[2], (void*)C);

          ret = bm_malloc_device_byte(handle, &output_dev_buffer[0], M * N * sizeof(float));

          ret = bmcv_gemm_ext(handle, is_A_trans, is_B_trans, M, N, K, alpha, input_dev_buffer[0],
                              input_dev_buffer[1], beta, input_dev_buffer[2], output_dev_buffer[0],
                              in_dtype, out_dtype);

          ret = bm_memcpy_d2s(handle, (void*)tpu_C, output_dev_buffer[0]);

          free(A_fp16);
          free(B_fp16);
          free(C_fp16);
          free(Y_fp16);

          free(A);
          free(B);
          free(C);
          free(tpu_C);

          bm_dev_free(handle);
          return ret;
      }