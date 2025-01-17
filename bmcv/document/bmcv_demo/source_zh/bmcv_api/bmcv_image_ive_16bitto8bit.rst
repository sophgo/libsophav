bmcv_ive_16bit_to_8bit
------------------------------

| 【描述】

| 该 API 使用ive硬件资源,  创建 16bit 图像数据到 8bit 图像数据的线性转化任务。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_16bit_to_8bit(
        bm_handle_t                 handle,
        bm_image                    input,
        bm_image                    output,
        bmcv_ive_16bit_to_8bit_attr attr);

| 【参数】

.. list-table:: bmcv_ive_16bit_to_8bit 参数表
    :widths: 20 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - \output
      - 输出
      - 输出 bm_image 对象结构体, 不能为空。
    * - \attr
      - 输入
      - 控制参数结构体, 不能为空。

.. list-table::
    :widths: 20 20 74 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY
      - DATA_TYPE_EXT_S16

        DATA_TYPE_EXT_U16
      - 16x16~1920x1080
    * - output
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE

        DATA_TYPE_EXT_1N_BYTE_SIGNED
      - 同 input

| 【数据类型说明】

【说明】定义 16bit 图像数据到 8bit 图像数据的转换模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_16bit_to_8bit_mode_e{
        BM_IVE_S16_TO_S8 = 0x0,
        BM_IVE_S16_TO_U8_ABS = 0x1,
        BM_IVE_S16_TO_U8_BIAS = 0x2,
        BM_IVE_U16_TO_U8 = 0x3,
    } bmcv_ive_16bit_to_8bit_mode;

.. list-table::
    :widths: 58 100

    * - **成员名称**
      - **描述**
    * - BM_IVE_S16_TO_S8
      - S16 数据到 S8 数据的线性变换。
    * - BM_IVE_S16_TO_U8_ABS
      - S16 数据线性变化到 S8 数据后取绝对值得到 S8 数据。
    * - BM_IVE_S16_TO_U8_BIAS
      - S16 数据线性变化到 S8 数据且平移后截断得到 U8 数据。
    * - BM_IVE_U16_TO_U8
      - U16 数据线性变化到 U8 数据。

【说明】定义 16bit 图像数据到 8bit 图像数据的转化控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_16bit_to_8bit_attr_s{
        bmcv_ive_16bit_to_8bit_mode mode;
        unsigned short u16_denominator;
        unsigned char u8_numerator;
        signed char   s8_bias;
    } bmcv_ive_16bit_to_8bit_attr;

.. list-table::
    :widths: 30 100

    * - **成员名称**
      - **描述**
    * - mode
      - 16bit 数据到 8bit 数据的转换模式。
    * - u16_denominator
      - 线性变换中的分母， 取值范围：{max{1, u8Numerator}, 65535}。
    * - u8_numerator
      - 线性变化中的分子，取值范围：[0, 255]。
    * - s8_bias
      - 线性变化中的平移项，取值范围：[-128, 127]。

【注意】

* :math:`u8Numerator \leq u16Denominator \text{ 且 } u16Denominator ≠ 0` ,

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 可配置 4 种模式，具体参考 bmcv_ive_16bit_to_8bit_mode 说明。

3. 计算公式：

   - BM_IVE_S16_TO_S8：

    .. math::
       I_{\text{out}}(x, y) =
       \begin{cases}
        -128 & \ (\frac{a}{b}I(x, y) < -128) \\
        \frac{a}{b}I(x, y) & \ (-128 \leq \frac{a}{b}I(x, y) \leq 127) \\
        127 & \ (\frac{a}{b}I(x, y) > 127) \\
      \end{cases}

   - BM_IVE_S16_TO_U8_ABS：

    .. math::
       I_{\text{out}}(x, y) =
       \begin{cases}
        \left|\frac{a}{b}I(x, y)\right| & \ (\left|\frac{a}{b}I(x, y)\right| \leq 255) \\
        255 & \ (\left|\frac{a}{b}I(x, y)\right| \geq 255) \\
      \end{cases}


   - BM_IVE_S16_TO_U8_BIAS：

    .. math::
       I_{\text{out}}(x, y) =
       \begin{cases}
        0 & \ (\frac{a}{b}I(x, y) + bais < 0) \\
        \frac{a}{b}I(x, y) + bais & \ (0 \leq \frac{a}{b}I(x, y) + bais \leq 255) \\
        255 & \ (\frac{a}{b}I(x, y) + bais > 255) \\
      \end{cases}

   - BM_IVE_U16_TO_U8：

    .. math::
       I_{\text{out}}(x, y) =
       \begin{cases}
        0 & \ (\frac{a}{b}I(x, y)< 0) \\
        \frac{a}{b}I(x, y) & \ (0 \leq \frac{a}{b}I(x, y) \leq 255) \\
        255 & \ (\frac{a}{b}I(x, y) > 255) \\
      \end{cases}



其中，:math:`I(x, y)` 对应 input，  :math:`I_{\text{out}}(x, y)` 对应output， a、b 和 bais 分别对应 attr 的 u8_numerator、u16_denominator、s8_bias。

**示例代码**

    .. code-block:: c

      #include <stdio.h>
      #include <stdlib.h>
      #include <string.h>
      #include <pthread.h>
      #include <sys/time.h>
      #include "bmcv_api_ext_c.h"
      #include <unistd.h>
      #define SLEEP_ON 0
      #define MAX_THREAD_NUM 20
      int main() {
          int loop_time = 1;
          int dev_id = 0;
          bmcv_ive_16bit_to_8bit_mode mode = BM_IVE_S16_TO_S8;
          unsigned char u8Numerator = 41;
          unsigned short u16Denominator = 18508;
          signed char s8Bias = 0;
          int height = xxxx, width = xxxx;
          bm_image_data_format_ext srcDtype = DATA_TYPE_EXT_S16;
          bm_image_data_format_ext dstDtype = DATA_TYPE_EXT_1N_BYTE_SIGNED;
          char *src_name = "ive_data/xxxxx";
          bm_handle_t handle = NULL;
          bm_image src, dst;
          int srcStride[4], dstStride[4];
          unsigned long long time_single, time_total = 0, time_avg = 0;
          unsigned long long time_max = 0, time_min = 10000, fps_actual = 0, pixel_per_sec = 0;
          struct timeval tv_start;
          struct timeval tv_end;
          struct timeval timediff;

          // config setting
          bmcv_ive_16bit_to_8bit_attr attr;
          attr.mode = mode;
          attr.u16_denominator = u16Denominator;
          attr.u8_numerator = u8Numerator;
          attr.s8_bias = s8Bias;
          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }
          // calc ive image stride && create bm image struct
          bm_ive_image_calc_stride(handle, height, width, FORMAT_GRAY, srcDtype, srcStride);
          bm_ive_image_calc_stride(handle, height, width, FORMAT_GRAY, dstDtype, dstStride);

          bm_image_create(handle, height, width, FORMAT_GRAY, srcDtype, &src, srcStride);
          bm_image_create(handle, height, width, FORMAT_GRAY, dstDtype, &dst, dstStride);

          ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
          if (ret != BM_SUCCESS) {
              printf("src bm_image_alloc_dev_mem failed. ret = %d\n", ret);
              exit(-1);
          }

          ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP_ANY);
          if (ret != BM_SUCCESS) {
              printf("src bm_image_alloc_dev_mem failed. ret = %d\n", ret);
              exit(-1);
          }

          // read image data from input files
          bm_ive_read_bin(src, src_name);

          for(i = 0; i < loop_time; i++)
          {
              gettimeofday(&tv_start, NULL);
              ret = bmcv_ive_16bit_to_8bit(handle, src, dst, attr);
              gettimeofday(&tv_end, NULL);
              timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
              timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
              time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);
              if(time_single>time_max){time_max = time_single;}
              if(time_single<time_min){time_min = time_single;}
              time_total = time_total + time_single;

              if(ret != BM_SUCCESS){
                  printf("bmcv_ive_16bitto8bit is failed \n");
                  exit(-1);
              }
          }

          time_avg = time_total / loop_time;
          fps_actual = 1000000 / time_avg;
          pixel_per_sec = width * height * fps_actual/1024/1024;
          bm_image_destroy(&src);
          bm_image_destroy(&dst);

          printf("ive_16bitTo8bit: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
                loop_time, time_max, time_avg, fps_actual, pixel_per_sec);

      }