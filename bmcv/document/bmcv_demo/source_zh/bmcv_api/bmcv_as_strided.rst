bmcv_as_strided
===============

该接口可以根据现有矩阵以及给定的步长来创建一个视图矩阵。


**接口形式：**

    .. code-block:: c

        bm_status_t bmcv_as_strided(
                    bm_handle_t handle,
                    bm_device_mem_t input,
                    bm_device_mem_t output,
                    int input_row,
                    int input_col,
                    int output_row,
                    int output_col,
                    int row_stride,
                    int col_stride);


**参数说明：**

* bm_handle_t handle

  输入参数。bm_handle 句柄。

* bm_device_mem_t input

  输入参数。存放输入矩阵 input 数据的设备内存地址。

* bm_device_mem_t output

  输入参数。存放输出矩阵 output 数据的设备内存地址。

* int input_row

  输入参数。输入矩阵 input 的行数。

* int input_col

  输入参数。输入矩阵 input 的列数。

* int output_row

  输入参数。输出矩阵 output 的行数。

* int output_col

  输入参数。输出矩阵 output 的列数。

* int row_stride

  输入参数。输出矩阵行之间的步长。

* int col_stride

  输入参数。输出矩阵列之间的步长。


**返回值说明:**

* BM_SUCCESS: 成功

* 其他: 失败


**注意事项：**

1. 该接口可通过设置环境变量启用双核计算，运行程序前：export TPU_CORES=2或export TPU_CORES=both即可。


**示例代码**

    .. code-block:: c

      #include "bmcv_api_ext_c.h"
      #include <math.h>
      #include <stdio.h>
      #include <stdlib.h>
      #include <string.h>

      int main()
      {
        int input_row = 5;
        int input_col = 5;
        int output_row = 3;
        int output_col = 3;
        int row_stride = 1;
        int col_stride = 2;
        int input_size = input_row * input_col;
        int output_size = output_row * output_col;
        int ret = 0;
        bm_handle_t handle;
        ret = bm_dev_request(&handle, 0);

        float* input_data = (float*)malloc(input_size  * sizeof(float));
        float* tpu_output = (float*)malloc(output_size * sizeof(float));

        for (int i = 0; i < input_size; i++) {
          input_data[i] = (float)rand() / (float)RAND_MAX * 100;
        }

        memset(tpu_output, 0, output_size * sizeof(float));

        bm_device_mem_t input_dev_mem, output_dev_mem;
        ret = bm_malloc_device_byte(handle, &input_dev_mem, input_size * sizeof(float));
        ret = bm_malloc_device_byte(handle, &output_dev_mem, output_size * sizeof(float));

        ret = bm_memcpy_s2d(handle, input_dev_mem, input_data);

        ret = bmcv_as_strided(handle, input_dev_mem, output_dev_mem, input_row, \
                        input_col, output_row, output_col, row_stride, col_stride);

        ret = bm_memcpy_d2s(handle, tpu_output, output_dev_mem);

        bm_free_device(handle, input_dev_mem);
        bm_free_device(handle, output_dev_mem);

        free(input_data);
        free(tpu_output);
        bm_dev_free(handle);
        return ret;
      }
