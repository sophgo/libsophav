bmcv_distance
=============

计算多维空间下多个点与特定一个点的欧式距离，前者坐标存放在连续的device memory中，而特定一个点的坐标通过参数传入。坐标值为float类型。


**接口形式：**

    .. code-block:: c

        bm_status_t bmcv_distance(
                    bm_handle_t handle,
                    bm_device_mem_t input,
                    bm_device_mem_t output,
                    int dim,
                    const float* pnt,
                    int len);


**参数说明：**

* bm_handle_t handle

  输入参数。bm_handle 句柄

* bm_device_mem_t input

  输入参数。存放len个点坐标的 device 空间。其大小为len*dim*sizeof(float)。

* bm_device_mem_t output

  输出参数。存放len个距离的 device 空间。其大小为len*sizeof(float)。

* int dim

  输入参数。空间维度大小。

* const float\* pnt

  输入参数。特定一个点的坐标，长度为dim。

* int len

  输入参数。待求坐标的数量。


**返回值说明:**

* BM_SUCCESS: 成功

* 其他: 失败


**示例代码**

    .. code-block:: c

      #include "bmcv_api_ext_c.h"
      #include <math.h>
      #include <stdio.h>
      #include <stdlib.h>
      #include <string.h>
      #include "test_misc.h"

      #define DIM_MAX 8

      enum op {
        FP32 = 0,
        FP16 = 1,
      };

      int main() {
        int dim = 1 + rand() % 8;
        int len = 1 + rand() % 40960;
        int ret = 0;
        enum op op = 0;
        bm_handle_t handle;
        ret = bm_dev_request(&handle, 0);
        if (ret != BM_SUCCESS) {
          printf("bm_dev_request failed. ret = %d\n", ret);
          return -1;
        }

        float pnt32[DIM_MAX] = {0};
        float* XHost = (float*)malloc(len * dim * sizeof(float));
        float* tpu_out = (float*)malloc(len * sizeof(float));
        fp16* XHost16 = (fp16*)malloc(len * dim * sizeof(fp16));
        fp16* tpu_out_fp16 = (fp16*)malloc(len * sizeof(fp16));
        fp16 pnt16[DIM_MAX] = {0};
        int round = 1;
        int i;

        printf("len = %d, dim = %d, op = %d\n", len, dim, op);

        for (i = 0; i < dim; ++i) {
          pnt32[i] = (rand() % 2 ? 1.f : -1.f) * (rand() % 100 + (rand() % 100) * 0.01);
          pnt16[i] = fp32tofp16(pnt32[i], round);
        }

        for (i = 0; i < len * dim; ++i) {
          XHost[i] = (rand() % 2 ? 1.f : -1.f) * (rand() % 100 + (rand() % 100) * 0.01);
          XHost16[i] = fp32tofp16(XHost[i], round);
        }
        if (op == FP32) {
          enum bm_data_type_t dtype = DT_FP32;
          bm_device_mem_t XDev, YDev;

          ret = bm_malloc_device_byte(handle, &XDev, len * dim * sizeof(float));
          ret = bm_malloc_device_byte(handle, &YDev, len * sizeof(float));
          ret = bm_memcpy_s2d(handle, XDev, XHost);
          ret = bmcv_distance(handle, XDev, YDev, dim, pnt32, len, dtype);
          ret = bm_memcpy_d2s(handle, tpu_out, YDev);

          bm_free_device(handle, XDev);
          bm_free_device(handle, YDev);
        } else {
          enum bm_data_type_t dtype = DT_FP16;
          bm_device_mem_t XDev, YDev;

          ret = bm_malloc_device_byte(handle, &XDev, len * dim * sizeof(float) / 2);
          ret = bm_malloc_device_byte(handle, &YDev, len * sizeof(float) / 2);
          ret = bm_memcpy_s2d(handle, XDev, XHost16);
          ret = bmcv_distance(handle, XDev, YDev, dim, (const void *)pnt16, len, dtype);
          ret = bm_memcpy_d2s(handle, tpu_out_fp16, YDev);

          bm_free_device(handle, XDev);
          bm_free_device(handle, YDev);

          for (i = 0; i < len; ++i) {
            tpu_out[i] = fp16tofp32(tpu_out_fp16[i]);
          }
        }

        free(XHost16);
        free(tpu_out_fp16);
        free(XHost);
        free(tpu_out);

        bm_dev_free(handle);
        return ret;
      }