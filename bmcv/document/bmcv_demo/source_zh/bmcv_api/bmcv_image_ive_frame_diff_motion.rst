bmcv_ive_frame_diff_motion
---------------------------------

| 【描述】

| 该 API 使用ive硬件资源,  创建背景相减任务。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_frame_diff_motion(
        bm_handle_t                     handle,
        bm_image                        input1,
        bm_image                        input2,
        bm_image                        output,
        bmcv_ive_frame_diff_motion_attr attr);

| 【参数】

.. list-table:: bmcv_ive_frame_diff_motion 参数表
    :widths: 20 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input1
      - 输入
      - 输入 bm_image 对象结构体，不能为空。
    * - \input2
      - 输入
      - 输入 bm_image 对象结构体，不能为空。
    * - \output
      - 输出
      - 输出 bm_image 对象结构体，不能为空。
    * - \attr
      - 输入
      - 控制参数结构体，不能为空。

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
      - 同 input

| 【数据类型说明】

【说明】定义该算子控制信息。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_frame_diff_motion_attr_s{
        bmcv_ive_sub_mode sub_mode;
        bmcv_ive_thresh_mode thr_mode;
        unsigned char u8_thr_low;
        unsigned char u8_thr_high;
        unsigned char u8_thr_min_val;
        unsigned char u8_thr_mid_val;
        unsigned char u8_thr_max_val;
        unsigned char au8_erode_mask[25];
        unsigned char au8_dilate_mask[25];
    } bmcv_ive_frame_diff_motion_attr;

.. list-table::
    :widths: 40 100

    * - **成员名称**
      - **描述**
    * - sub_mode
      - 定义两张图像相减输出格式。
    * - thr_mode
      - 定义阈值化计算方式，仅支持 MOD_U8 模式，即 U8 数据到 U8 数据的阈值化。
    * - u8_thr_low
      - 低阈值，取值范围：[0, 255]。
    * - u8_thr_high
      - 表示高阈值， 0 ≤ u8ThrLow ≤ u8ThrHigh ≤ 255。
    * - u8_thr_min_val
      - 表示最小值，取值范围：[0, 255]。
    * - u8_thr_mid_val
      - 表示中间值，取值范围：[0, 255]。
    * - u8_thr_max_val
      - 表示最大值，取值范围：[0, 255]。
    * - au8_erode_mask[25]
      - 表示腐蚀的 5x5 模板系数，取值范围：0 或 255。
    * - au8_dilate_mask[25]
      - 表示膨胀的 5x5 模板系数，取值范围：0 或 255。

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
        int dev_id = 0;int height = 480, width = 480;bm_image_format_ext fmt = FORMAT_GRAY;
        char *src1_name = "ive_data/md1_480x480.yuv";
        char *src2_name = "ive_data/md2_480x480.yuv";
        bm_handle_t handle = NULL;
        int ret = (int)bm_dev_request(&handle, dev_id);
        if (ret != 0) {
            printf("Create bm handle failed. ret = %d\n", ret);
            exit(-1);
        }
        bm_image src1, src2, dst;
        int stride[4];
        unsigned int i = 0, loop_time = 0;
        unsigned long long time_single, time_total = 0, time_avg = 0;
        unsigned long long time_max = 0, time_min = 10000, fps_actual = 0, pixel_per_sec = 0;
        struct timeval tv_start;
        struct timeval tv_end;
        struct timeval timediff;

        // mask data
        unsigned char arr[25] = {0, 0, 255, 0, 0, 0, 0, 255, 0, 0, 255,
                255, 255, 255, 255, 0, 0, 255, 0, 0, 0, 0, 255, 0, 0};

        // config setting(Sub->threshold->erode->dilate)
        bmcv_ive_frame_diff_motion_attr attr;
        attr.sub_mode = IVE_SUB_ABS;
        attr.thr_mode = IVE_THRESH_BINARY;
        attr.u8_thr_min_val = 0;
        attr.u8_thr_max_val = 255;
        attr.u8_thr_low = 30;

        memcpy(&attr.au8_erode_mask, &arr, 25 * sizeof(unsigned char));
        memcpy(&attr.au8_dilate_mask, &arr, 25 * sizeof(unsigned char));

        // calc ive image stride && create bm image struct
        bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, stride);

        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src1, stride);
        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src2, stride);
        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, stride);

        ret = bm_image_alloc_dev_mem(src1, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("src bm_image_alloc_dev_mem failed. ret = %d\n", ret);
            exit(-1);
        }

        ret = bm_image_alloc_dev_mem(src2, BMCV_HEAP_ANY);
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
        bm_ive_read_bin(src1, src1_name);
        bm_ive_read_bin(src2, src2_name);

        for(i = 0; i < loop_time; i++)
        {
            gettimeofday(&tv_start, NULL);
            ret = bmcv_ive_frame_diff_motion(handle, src1, src2, dst, attr);
            gettimeofday(&tv_end, NULL);
            timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
            timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
            time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

            if(time_single>time_max){time_max = time_single;}
            if(time_single<time_min){time_min = time_single;}
            time_total = time_total + time_single;

            if(ret != BM_SUCCESS){
                printf("bmcv_ive_frameDiffMotion is failed \n");
                exit(-1);
            }
        }

        time_avg = time_total / loop_time;
        fps_actual = 1000000 / time_avg;
        pixel_per_sec = width * height * fps_actual/1024/1024;
        bm_image_destroy(&src1);
        bm_image_destroy(&src2);
        bm_image_destroy(&dst);
        printf("bm_ive_frameDiffMotion: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
            loop_time, time_max, time_avg, fps_actual, pixel_per_sec);
        return 0;
      }




