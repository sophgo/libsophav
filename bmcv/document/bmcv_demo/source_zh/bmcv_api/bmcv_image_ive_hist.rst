bmcv_ive_hist
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建灰度图像，遍历图像像素值实现统计图像相同像素值出现的次数，构建直方图。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_hist(
        bm_handle_t      handle,
        bm_image         input,
        bm_device_mem_t  output);

| 【参数】

.. list-table:: bmcv_ive_hist 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - input
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - output
      - 输出
      - 输入 bm_device_mem_t 对象数据结构体, 不能为空, 内存至少配置 1024 字节，输出使用u32类型代表0-255每个数字的统计值。

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

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入图像的 width 都需要16对齐。

2. 计算公式如下:

    .. math::

      I(x) = \sum_{i} \sum_{j} \begin{cases}
         1 & \text{if } I(i, j) = x \\
         0 & \text{otherwise}
      \end{cases} \quad \text{for } x = 0 \ldots 255


**示例代码**

    .. code-block:: c

        #include <stdio.h>
        #include <stdlib.h>
        #include <string.h>
        #include <pthread.h>
        #include <sys/time.h>
        #include "bmcv_api_ext_c.h"
        #include <unistd.h>
        extern void bm_ive_read_bin(bm_image src, const char *input_name);
        extern bm_status_t bm_ive_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
            bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride);
        int dev_id = 0;
        int height = 288, width = 352;
        bm_image_format_ext src_fmt = FORMAT_GRAY;
        char * src_name = "./data/00_352x288_y.yuv", *dst_name = "./hist_result.bin";
        bm_handle_t handle = NULL;
        int ret = (int)bm_dev_request(&handle, dev_id);
        if (ret != 0) {
            printf("Create bm handle failed. ret = %d\n", ret);
            exit(-1);
        }
        bm_image src;
        bm_device_mem_t dst;
        int src_stride[4];
        unsigned int i = 0, loop_time = 0;
        unsigned long long time_single, time_total = 0, time_avg = 0;
        unsigned long long time_max = 0, time_min = 10000, fps_actual = 0;
        struct timeval tv_start;
        struct timeval tv_end;
        struct timeval timediff;

        // calc ive image stride && create bm image struct
        bm_ive_image_calc_stride(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, src_stride);
        bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
            exit(-1);
        }
        bm_ive_read_bin(src, src_name);

        // create result
        ret = bm_malloc_device_byte(handle, &dst, 1024);
        if (ret != BM_SUCCESS) {
            printf("bm_malloc_device_byte failed. ret = %d\n", ret);
            exit(-1);
        }

        for (i = 0; i < loop_time; i++) {
            gettimeofday(&tv_start, NULL);
            ret = bmcv_ive_hist(handle, src, dst);
            gettimeofday(&tv_end, NULL);
            timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
            timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
            time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

            if(time_single>time_max){time_max = time_single;}
            if(time_single<time_min){time_min = time_single;}
            time_total = time_total + time_single;

            if(ret != BM_SUCCESS){
                printf("bmcv_image_ive_hist failed. ret = %d\n", ret);
                exit(-1);
            }
        }
        time_avg = time_total / loop_time;
        fps_actual = 1000000 / time_avg;
        bm_image_destroy(&src);
        bm_free_device(handle, dst);
        printf("bmcv_ive_hist: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
            loop_time, time_max, time_avg, fps_actual);
        printf("bmcv ive hist successful \n");
        return 0;
      }
