bmcv_hm_distance
------------------------------

**描述：**

该接口用来计算两个向量中各个元素的汉明距离。

**语法：**

.. code-block:: c
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_hamming_distance(
        bm_handle_t handle,
        bm_device_mem_t input1,
        bm_device_mem_t input2,
        bm_device_mem_t output,
        int bits_len,
        int input1_num,
        int input2_num);

**参数：**

.. list-table:: bmcv_hm_distance 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用bm_dev_request获取。
    * - input1
      - 输入
      - 向量1数据的地址信息。
    * - input2
      - 输入
      - 向量2数据的地址信息。
    * - output
      - 输出
      - output向量数据的地址信息。
    * - bits_len
      - 输入
      - 向量中的每个元素的长度。
    * - input1_num
      - 输入
      - 向量1的数据个数。
    * - input2_num
      - 输入
      - 向量2的数据个数。

**返回值：**

该函数成功调用时, 返回BM_SUCCESS。

**注意事项：**

1. bits_len支持到4, 8, 16, 32。

2. input1_num最大支持到16，input2_num最大支持到50000000。

**代码示例：**

.. code-block:: c
    :linenos:
    :lineno-start: 1
    :force:

    #include <math.h>
    #include "stdio.h"
    #include "stdlib.h"
    #include "string.h"
    #include "bmcv_api_ext_c.h"

    int main() {
        int bits_len = 8;
        int input1_num = 1 + rand() % 16;
        int input2_num = 1 + rand() % 10000;
        bm_handle_t handle;
        bm_status_t ret = bm_dev_request(&handle, 0);
        if (ret != BM_SUCCESS) {
            printf("Create bm handle failed. ret = %d\n", ret);
            return -1;
        }

        bm_device_mem_t input1_dev_mem;
        bm_device_mem_t input2_dev_mem;
        bm_device_mem_t output_dev_mem;

        uint32_t* input1_data = (uint32_t*)malloc(input1_num * bits_len * sizeof(uint32_t));
        uint32_t* input2_data = (uint32_t*)malloc(input2_num * bits_len * sizeof(uint32_t));
        uint32_t* output_tpu  = (uint32_t*)malloc(input1_num * input2_num * sizeof(uint32_t));

        printf("bits_len is %u\n", bits_len);
        printf("input1_data len is %u\n", input1_num);
        printf("input2_data len is %u\n", input2_num);
        memset(input1_data, 0, input1_num * bits_len * sizeof(uint32_t));
        memset(input2_data, 0, input2_num * bits_len * sizeof(uint32_t));
        memset(output_tpu,  0,  input1_num * input2_num * sizeof(uint32_t));

        // fill data
        for(int i = 0; i < input1_num * bits_len; i++) {
            input1_data[i] = rand() % 10;
        }
        for(int i = 0; i < input2_num * bits_len; i++) {
            input2_data[i] = rand() % 20 + 1;
        }
        // tpu_cal
        bm_malloc_device_byte(handle, &input1_dev_mem, input1_num * bits_len * sizeof(uint32_t));
        bm_malloc_device_byte(handle, &input2_dev_mem, input2_num * bits_len * sizeof(uint32_t));
        bm_malloc_device_byte(handle, &output_dev_mem, input1_num * input2_num * sizeof(uint32_t));
        bm_memcpy_s2d(handle, input1_dev_mem, input1_data);
        bm_memcpy_s2d(handle, input2_dev_mem, input2_data);

        bmcv_hamming_distance(handle, input1_dev_mem, input2_dev_mem, output_dev_mem, bits_len, input1_num, input2_num);

        bm_memcpy_d2s(handle, output_tpu, output_dev_mem);

        for (int i = 0; i < 8; i++) {
            printf("output_tpu[%d] is: %d\n", i, output_tpu[i]);
        }
        free(input1_data);
        free(input2_data);
        free(output_tpu);
        bm_free_device(handle, input1_dev_mem);
        bm_free_device(handle, input2_dev_mem);
        bm_free_device(handle, output_dev_mem);

        bm_dev_free(handle);
        return ret;
    }