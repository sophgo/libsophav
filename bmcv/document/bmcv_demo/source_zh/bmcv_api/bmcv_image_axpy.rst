bmcv_image_axpy
==================

该接口实现 F = A * X + Y，其中 A 是常数，大小为 n * c ，F 、X 、Y 都是大小为n * c * h * w的矩阵。


**接口形式：**

    .. code-block:: c

        bm_status_t bmcv_image_axpy(
                    bm_handle_t handle,
                    bm_device_mem_t tensor_A,
                    bm_device_mem_t tensor_X,
                    bm_device_mem_t tensor_Y,
                    bm_device_mem_t tensor_F,
                    int input_n,
                    int input_c,
                    int input_h,
                    int input_w);


**参数说明：**

* bm_handle_t handle

  输入参数。 bm_handle 句柄。

* bm_device_mem_t tensor_A

  输入参数。存放常数 A 的设备内存地址。

* bm_device_mem_t tensor_X

  输入参数。存放矩阵X的设备内存地址。

* bm_device_mem_t tensor_Y

  输入参数。存放矩阵Y的设备内存地址。

* bm_device_mem_t tensor_F

  输出参数。存放结果矩阵F的设备内存地址。

* int input_n

  输入参数。n 维度大小。

* int input_c

  输入参数。c 维度大小。

* int input_h

  输入参数。h 维度大小。

* int input_w

  输入参数。w 维度大小。


**返回值说明：**

* BM_SUCCESS: 成功

* 其他: 失败


**示例代码**

    .. code-block:: c

      #include <stdlib.h>
      #include <stdint.h>
      #include <stdio.h>
      #include <string.h>
      #include <math.h>
      #include "bmcv_api_ext_c.h"

      #define N (10)
      #define C 256 //(64 * 2 + (64 >> 1))
      #define H 8
      #define W 8
      #define TENSOR_SIZE (N * C * H * W)

      int main(){
        int trials = 1;
        int ret = 0;
        bm_handle_t handle;
        ret = bm_dev_request(&handle, 0);
        if (ret != BM_SUCCESS) {
          printf("bm_dev_request failed. ret = %d\n", ret);
          return -1;
        }

        float *tensor_X = malloc(TENSOR_SIZE * sizeof(float));
        float *tensor_A = malloc(N * C * sizeof(float));
        float *tensor_Y = malloc(TENSOR_SIZE * sizeof(float));
        float *tensor_F = malloc(TENSOR_SIZE * sizeof(float));
        int idx_trial;

        for (idx_trial = 0; idx_trial < trials; idx_trial++) {
          for (int idx = 0; idx < TENSOR_SIZE; idx++) {
            tensor_X[idx] = (float)idx - 5.0f;
            tensor_Y[idx] = (float)idx/3.0f - 8.2f;  //y
          }

          for (int idx = 0; idx < N*C; idx++) {
            tensor_A[idx] = (float)idx * 1.5f + 1.0f;
          }

          ret = bmcv_image_axpy(handle,
                              bm_mem_from_system((void *)tensor_A),
                              bm_mem_from_system((void *)tensor_X),
                              bm_mem_from_system((void *)tensor_Y),
                              bm_mem_from_system((void *)tensor_F),
                              N, C, H, W);
          }
        free(tensor_X);
        free(tensor_A);
        free(tensor_Y);
        free(tensor_F);

        bm_dev_free(handle);
        return ret;
      }