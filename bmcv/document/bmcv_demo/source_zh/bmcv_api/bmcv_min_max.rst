bmcv_min_max
============

对于存储于device memory中连续空间的一组数据，该接口可以获取这组数据中的最大值和最小值。


**接口形式:**

    .. code-block:: c

        bm_status_t bmcv_min_max(
                    bm_handle_t handle,
                    bm_device_mem_t input,
                    float* minVal,
                    float* maxVal,
                    int len);


**参数说明：**

* bm_handle_t handle

  输入参数。bm_handle 句柄

* bm_device_mem_t input

  输入参数。输入数据的 device 地址。

* float\* minVal

  输出参数。运算后得到的最小值结果, 如果为 NULL 则不计算最小值。

* float\* maxVal

  输出参数。运算后得到的最大值结果，如果为 NULL 则不计算最大值。

* int len

  输入参数。输入数据的长度。


**返回值说明:**

* BM_SUCCESS: 成功

* 其他: 失败


**注意事项：**

1. 该接口可通过设置环境变量启用双核计算，运行程序前：export TPU_CORES=2或export TPU_CORES=both即可。


**示例代码**

    .. code-block:: c

      #include "bmcv_api_ext_c.h"
      #include "stdio.h"
      #include "stdlib.h"


      int main() {
          int L = 50 + rand() % 260095;
          int ret = 0;
          bm_handle_t handle;
          ret = bm_dev_request(&handle, 0);

          if (ret != BM_SUCCESS) {
              printf("Create bm handle failed. ret = %d\n", ret);
              return -1;
          }

          float min_tpu = 0, max_tpu = 0;
          float* XHost = (float*)malloc(L * sizeof(float));
          int i;

          for (i = 0; i < L; ++i)
              XHost[i] = (float)((rand() % 2 ? 1 : -1) * (rand() % 1000 + (rand() % 100000) * 0.01));

          bm_device_mem_t XDev;

          ret = bm_malloc_device_byte(handle, &XDev, L * sizeof(float));
          ret = bm_memcpy_s2d(handle, XDev, XHost);
          ret = bmcv_min_max(handle, XDev, &min_tpu, &max_tpu, L);
          if (ret != BM_SUCCESS) {
              printf("Calculate bm min_max failed. ret = %d\n", ret);
              return -1;
          }

          bm_free_device(handle, XDev);

          free(XHost);

          bm_dev_free(handle);
          return ret;
      }