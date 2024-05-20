bmcv_ive_resize
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建图像缩放任务, 支持 双线性插值、区域插值 缩放, 支持多张 GRAY 与 RGB PLANAR 图像同时输入做一种类型的缩放。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_resize(
        bm_handle_t              handle,
        bm_image                 input,
        bm_image                 output,
        bmcv_resize_algorithm     resize_mode);

| 【参数】

.. list-table:: bmcv_ive_resize 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input
      - 输入
      - 输入 bm_image 对象结构体，不能为空。
    * - \output
      - 输出
      - 输出 bm_image 对象结构体，不能为空, 每张图像类型同input。
    * - resize_mode
      - 输入
      - 输入 bmcv_resize_algorithm 对象, 枚举控制信息。

.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY

        RGB_PLANAR
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - output
      - GRAY

        RGB_PLANAR
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080

| 【数据类型说明】

【说明】定义 Resize 模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_resize_algorithm_ {
        BMCV_INTER_NEAREST = 0,
        BMCV_INTER_LINEAR  = 1,
        BMCV_INTER_BICUBIC = 2,
        BMCV_INTER_AREA = 3,
    } bmcv_resize_algorithm;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - BMCV_INTER_NEAREST
      - 最近邻插值模式。
    * - IVE_RESIZE_AREA
      - 双线性插值缩放模式。
    * - IVE_RESIZE_LINEAR
      - 双三次插值模式。
    * - IVE_RESIZE_AREA
      - 区域插值缩放模式。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 支持 GRAY、RGB_PLANAR 混合图像数组输入, 但所有图像的缩放模式相同，同时模式只支持双线性插值和区域插值缩放模式。

2. 最大支持 16 倍缩放。

3. 输入输出图像的 width 都需要16对齐。


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
      #define IMG_NUM 3
      extern void bm_ive_read_bin(bm_image src, const char *input_name);
      extern bm_status_t bm_ive_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
          bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride);
      int main(){
        int dev_id = 0;
        bmcv_resize_algorithm resize_mode = BMCV_INTER_AREA;
        bm_image_format_ext format[IMG_NUM] = {FORMAT_RGBP_SEPARATE, FORMAT_GRAY, FORMAT_RGB_PLANAR};
        int src_widht = 352, src_height = 288;
        int dst_width[IMG_NUM] = {176, 400, 320};
        int dst_height[IMG_NUM] = {144, 300, 240};
        char *src_name[IMG_NUM] = {"./ive_data/campus_352x288.rgb", "./ive_data/campus_352x288.gray", "./ive_data/campus_352x288.rgb"};
        bm_handle_t handle = NULL;
        int ret = (int)bm_dev_request(&handle, dev_id);
        if (ret != 0) {
            printf("Create bm handle failed. ret = %d\n", ret);
            exit(-1);
        }
        bm_image src[IMG_NUM], dst[IMG_NUM];
        int src_stride[IMG_NUM][4], dst_stride[IMG_NUM][4];
        unsigned int i = 0, loop_time = 0;
        unsigned long long time_single, time_total = 0, time_avg = 0;
        unsigned long long time_max = 0, time_min = 10000, fps_actual = 0;
        struct timeval tv_start;
        struct timeval tv_end;
        struct timeval timediff;

        // calc ive image stride && create bm image struct
        for(int img_idx = 0; img_idx < IMG_NUM; img_idx++){
            bm_ive_image_calc_stride(handle, src_height, src_widht,
                      format[img_idx], DATA_TYPE_EXT_1N_BYTE, src_stride[img_idx]);
            bm_image_create(handle, src_height, src_widht, format[img_idx],
                      DATA_TYPE_EXT_1N_BYTE, &src[img_idx], src_stride[img_idx]);

            bm_ive_image_calc_stride(handle, dst_height[img_idx], dst_width[img_idx],
                      format[img_idx], DATA_TYPE_EXT_1N_BYTE, dst_stride[img_idx]);
            bm_image_create(handle, dst_height[img_idx], dst_width[img_idx], format[img_idx],
                      DATA_TYPE_EXT_1N_BYTE, &dst[img_idx], dst_stride[img_idx]);

            ret = bm_image_alloc_dev_mem(src[img_idx], BMCV_HEAP_ANY);
            if (ret != BM_SUCCESS) {
                printf("src[%d] bm_image_alloc_dev_mem failed. ret = %d\n", img_idx, ret);
                for(int idx = 0; idx < img_idx; idx++){
                    bm_image_destroy(&src[idx]);
                    bm_image_destroy(&dst[idx]);
                }
                exit(-1);
            }

            ret = bm_image_alloc_dev_mem(dst[img_idx], BMCV_HEAP_ANY);
            if (ret != BM_SUCCESS) {
                printf("dst[%d] bm_image_alloc_dev_mem failed. ret = %d\n", img_idx, ret);
                for(int idx = 0; idx < img_idx; idx++){
                    bm_image_destroy(&src[idx]);
                    bm_image_destroy(&dst[idx]);
                }
                exit(-1);
            }

            bm_ive_read_bin(src[img_idx], src_name[img_idx]);
        }

        for (i = 0; i < loop_time; i++) {
            gettimeofday(&tv_start, NULL);
            for (int idx=0; idx<IMG_NUM; idx++){
                ret = bmcv_ive_resize(handle, src[idx], dst[idx], resize_mode);
            }
            gettimeofday(&tv_end, NULL);
            timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
            timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
            time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

            if(time_single>time_max){time_max = time_single;}
            if(time_single<time_min){time_min = time_single;}
            time_total = time_total + time_single;
            if(ret != BM_SUCCESS){
                printf("bmcv_ive_resize failed ,ret is %d \n", ret);
                for(int idx = 0; idx < IMG_NUM; idx++){
                    bm_image_destroy(&src[idx]);
                    bm_image_destroy(&dst[idx]);
                }
                exit(-1);
            }
        }

        time_avg = time_total / loop_time;
        fps_actual = 1000000 / time_avg;
        for(int idx = 0; idx < IMG_NUM; idx++){
            bm_image_destroy(&src[idx]);
            bm_image_destroy(&dst[idx]);
        }
        printf("bmcv_ive_resize: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
                loop_time, time_max, time_avg, fps_actual);
        printf("bmcv ive resize test successful \n");

        return 0;
      }
























