bmcv_gemm
============

该接口可以实现 float32 类型矩阵的通用乘法计算，如下公式：

  .. math::

      C = \alpha\times A\times B + \beta\times C

其中，A、B、C均为矩阵，:math:`\alpha` 和 :math:`\beta` 均为常系数


**接口形式：**

    .. code-block:: c

        bm_status_t bmcv_gemm(
                    bm_handle_t handle,
                    bool is_A_trans,
                    bool is_B_trans,
                    int M,
                    int N,
                    int K,
                    float alpha,
                    bm_device_mem_t A,
                    int lda,
                    bm_device_mem_t B,
                    int ldb,
                    float beta,
                    bm_device_mem_t C,
                    int ldc);


**参数说明：**

* bm_handle_t handle

  输入参数。bm_handle 句柄

* bool is_A_trans

  输入参数。设定矩阵 A 是否转置

* bool is_B_trans

  输入参数。设定矩阵 B 是否转置

* int M

  输入参数。矩阵 A 和矩阵 C 的行数

* int N

  输入参数。矩阵 B 和矩阵 C 的列数

* int K

  输入参数。矩阵 A 的列数和矩阵 B 的行数

* float alpha

  输入参数。数乘系数

* bm_device_mem_t A

  输入参数。根据数据存放位置保存左矩阵 A 数据的 device 地址或者 host 地址。如果数据存放于 host 空间则内部会自动完成 s2d 的搬运

* int lda

  输入参数。矩阵 A 的 leading dimension, 即第一维度的大小，在行与行之间没有stride的情况下即为 A 的列数（不做转置）或行数（做转置）

* bm_device_mem_t B

  输入参数。根据数据存放位置保存右矩阵 B 数据的 device 地址或者 host 地址。如果数据存放于 host 空间则内部会自动完成 s2d 的搬运。

* int ldb

  输入参数。矩阵 C 的 leading dimension, 即第一维度的大小，在行与行之间没有stride的情况下即为 B 的列数（不做转置）或行数（做转置。

* float beta

  输入参数。数乘系数。

* bm_device_mem_t C

  输出参数。根据数据存放位置保存矩阵 C 数据的 device 地址或者 host 地址。如果是 host 地址，则当beta不为0时，计算前内部会自动完成 s2d 的搬运，计算后再自动完成 d2s 的搬运。

* int ldc

  输入参数。矩阵 C 的 leading dimension, 即第一维度的大小，在行与行之间没有stride的情况下即为 C 的列数。


**返回值说明:**

* BM_SUCCESS: 成功

* 其他: 失败


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
          bool if_A_trans = rand() % 2;
          bool if_B_trans = rand () % 2;
          int ret = 0;
          bm_handle_t handle;

          if (if_A_trans) {
              if_B_trans = true;
          }

          ret = bm_dev_request(&handle, 0);
          if (ret) {
              printf("bm_dev_request failed. ret = %d\n", ret);
              return ret;
          }

          float* src_A = (float*)malloc(M * K * sizeof(float));
          float* src_B = (float*)malloc(N * K * sizeof(float));
          float* src_C = (float*)malloc(M * N * sizeof(float));
          int lda = if_A_trans ? M : K;
          int ldb = if_B_trans ? K : N;

          ret = assign_values_to_matrix(src_A, M * K);
          ret = assign_values_to_matrix(src_B, N * K);
          ret = assign_values_to_matrix(src_C, M * N);


          ret= bmcv_gemm(handle, if_A_trans, if_B_trans, M, N, K, alpha, bm_mem_from_system((void *)src_A),
                          lda, bm_mem_from_system((void *)src_B), ldb, beta,
                          bm_mem_from_system((void *)src_C), N);

          free(src_A);
          free(src_B);
          free(src_C);

          bm_dev_free(handle);
          return ret;
      }