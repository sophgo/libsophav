bmcv_ive_ord_stat_filter
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建 3x3 模板顺序统计量滤波任务，可进行 Median、 Max、 Min 滤波。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_ord_stat_filter(
    bm_handle_t               handle,
    bm_image                  input,
    bm_image                  output,
    bmcv_ive_ord_stat_filter_mode mode);

| 【参数】

.. list-table:: bmcv_ive_ord_stat_filter 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - input1
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - output
      - 输出
      - 输出 bm_image 对象结构体, 不能为空, 宽、高同 input。
    * - mode
      - 输入
      - 控制顺序统计量滤波的模式。

.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - output
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input1

| 【数据类型说明】

【说明】定义顺序统计量滤波模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    _bmcv_ord_stat_filter_mode_e{
        BM_IVE_ORD_STAT_FILTER_MEDIAN = 0x0,
        BM_IVE_ORD_STAT_FILTER_MAX    = 0x1,
        BM_IVE_ORD_STAT_FILTER_MIN    = 0x2,
    } bmcv_ive_ord_stat_filter_mode;

.. list-table::
    :widths: 120 80

    * - **成员名称**
      - **描述**
    * - BM_IVE_ORD_STAT_FILTER_MEDIAN
      - 中值滤波
    * - BM_IVE_ORD_STAT_FILTER_MAX
      - 最大值滤波, 等价于灰度图的膨胀。
    * - BM_IVE_ORD_STAT_FILTER_MIN
      - 最小值滤波, 等价于灰度图的腐蚀。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 可配置 3 种滤波模式, 参考 bmcv_ive_ord_stat_filter_mode 说明。

3. 计算公式如下：

- BM_IVE_ORD_STAT_FILTER_MEDIAN:

    .. math::
       \begin{aligned}
        & & I_{\text{out}}(x, y) = \text{median}\{\substack{-1 < i < 1 \\ -1 < j < 1} I(x+i, y+j)\}
      \end{aligned}

- BM_IVE_ORD_STAT_FILTER_MAX:

    .. math::

      \begin{aligned}
        & I_{\text{out}}(x, y) = \text{max}\{\substack{-1 < i < 1 \\ -1 < j < 1} I(x+i, y+j)\}
      \end{aligned}

- BM_IVE_ORD_STAT_FILTER_MIN:

    .. math::

      \begin{aligned}
       & I_{\text{out}}(x, y) = \text{min}\{\substack{-1 < i < 1 \\ -1 < j < 1} I(x+i, y+j)\} \\
      \end{aligned}

  其中, :math:`I(x, y)` 对应 input, :math:`I_{\text{out}(x, y)}` 对应 output。


**示例代码**

    .. code-block:: c

      #include <stdio.h>
      #include <stdlib.h>
      #include <string.h>
      #include <pthread.h>
      #include <math.h>
      #include <sys/time.h>
      #include "bmcv_api_ext_c.h"
      #include <unistd.h>
      extern void bm_ive_read_bin(bm_image src, const char *input_name);
      extern void bm_ive_write_bin(bm_image dst, const char *output_name);
      extern bm_status_t bm_ive_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
          bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride);
      int main(){
        int dev_id = 0;int height = 288, width = 352;
        bm_image_format_ext fmt = FORMAT_GRAY;
        bmcv_ive_ord_stat_filter_mode ordStatFilterMode = BM_IVE_ORD_STAT_FILTER_MEDIAN;
        char *src_name = "./data/00_352x288_y.yuv", *dst_name = "ive_ordStatFilter.yuv";
        bm_handle_t handle = NULL;
        int ret = (int)bm_dev_request(&handle, dev_id);
        if (ret != 0) {
            printf("Create bm handle failed. ret = %d\n", ret);
            exit(-1);
        }
        bm_image src, dst;
        int stride[4];
        unsigned int i = 0, loop_time = 0;
        unsigned long long time_single, time_total = 0, time_avg = 0;
        unsigned long long time_max = 0, time_min = 10000, fps_actual = 0;
        struct timeval tv_start;
        struct timeval tv_end;
        struct timeval timediff;

        // calc ive image stride && create bm image struct
        bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, stride);

        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, stride);

        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("src bm_image_alloc_dev_mem_src. ret = %d\n", ret);
            exit(-1);
        }

        ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("dst bm_image_alloc_dev_mem_src. ret = %d\n", ret);
            exit(-1);
        }
        bm_ive_read_bin(src, src_name);

        for (i = 0; i < loop_time; i++) {
            gettimeofday(&tv_start, NULL);
            ret = bmcv_ive_ord_stat_filter(handle, src, dst, ordStatFilterMode);
            gettimeofday(&tv_end, NULL);
            timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
            timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
            time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

            if(time_single>time_max){time_max = time_single;}
            if(time_single<time_min){time_min = time_single;}
            time_total = time_total + time_single;

            if(ret != BM_SUCCESS){
                printf("bmcv_ive_ordStatFilter failed. ret = %d\n", ret);
                exit(-1);
            }
        }

        time_avg = time_total / loop_time;
        fps_actual = 1000000 / time_avg;
        bm_image_destroy(&src);
        bm_image_destroy(&dst);

        printf("bmcv_ive_ordstatfilter: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
                loop_time, time_max, time_avg, fps_actual);
        printf("bmcv ive ordStatFilter test successful \n");
        return 0;
      }





























