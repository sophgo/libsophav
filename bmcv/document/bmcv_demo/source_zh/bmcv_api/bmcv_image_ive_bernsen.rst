
bmcv_ive_bersen
------------------------------

| 【描述】

| 该 API 使用ive硬件资源,  实现利用 Bernsen 二值法，对图像进行二值化求解，实现图像的二值化。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_bernsen(
        bm_handle_t           handle,
        bm_image              input,
        bm_image              output,
        bmcv_ive_bernsen_attr attr);

| 【参数】

.. list-table:: bmcv_ive_bersen 参数表
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
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 16x16~1920x1080
    * - output
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input

| 【数据类型说明】

【说明】定义 Bernsen 阈值计算的不同方式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_bernsen_mode_e{
        BM_IVE_BERNSEN_NORMAL = 0x0,
        BM_IVE_BERNSEN_THRESH = 0x1,
        BM_IVE_BERNSEN_PAPER = 0x2,
    } bmcv_ive_bernsen_mode;

【说明】定义 Bernsen 二值法的控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_bernsen_attr_s{
        bmcv_ive_bernsen_mode en_mode;
        unsigned char u8_win_size;
        unsigned char u8_thr;
        unsigned char u8_contrast_threshold;
    } bmcv_ive_bernsen_attr;

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 输入输出图像的 width 都需要16对齐。


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
      int main(){

        int dev_id = 0;
        bmcv_ive_bernsen_mode mode = BM_IVE_BERNSEN_NORMAL;
        int winsize = 5;
        int height = 288, width = 352;
        bm_image_format_ext fmt = FORMAT_GRAY;
        char *src_name = "ive_data/00_352x288_y.yuv";
        bm_handle_t handle = NULL;
        int ret = (int)bm_dev_request(&handle, dev_id);
        if (ret != 0) {
            printf("Create bm handle failed. ret = %d\n", ret);
            exit(-1);
        }
        bm_image src, dst;
        int stride[4];
        unsigned int i = 0, loop_time = 1;
        unsigned long long time_single, time_total = 0, time_avg = 0;
        unsigned long long time_max = 0, time_min = 10000, fps_actual = 0, pixel_per_sec = 0;
        struct timeval tv_start;
        struct timeval tv_end;
        struct timeval timediff;
        bmcv_ive_bernsen_attr attr;
        memset(&attr, 0, sizeof(bmcv_ive_bernsen_attr));

        attr.en_mode = mode;
        attr.u8_thr = 128;
        attr.u8_contrast_threshold = 15;
        attr.u8_win_size = winsize;
        bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, stride);

        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, stride);

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
            ret = bmcv_ive_bernsen(handle, src, dst, attr);
            gettimeofday(&tv_end, NULL);
            timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
            timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
            time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

            if(time_single>time_max){time_max = time_single;}
            if(time_single<time_min){time_min = time_single;}
            time_total = time_total + time_single;

            if(ret != BM_SUCCESS){
                printf("bmcv_ive_bersen is failed \n");
                exit(-1);
            }
        }

        time_avg = time_total / loop_time;
        fps_actual = 1000000 / time_avg;
        pixel_per_sec = width * height * fps_actual/1024/1024;
        bm_image_destroy(&src);
        bm_image_destroy(&dst);
        printf(" bm_ive_bersen: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
            loop_time, time_max, time_avg, fps_actual, pixel_per_sec);

        return 0;
      }