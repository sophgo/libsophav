bmcv_ive_filter_and_csc
------------------------------

| 【描述】

| 该 API 使用ive硬件资源,  创建 5x5 模板滤波和 YUV2RGB 色彩空间转换复合任务，通过一次创建完成两种功能。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_filter_and_csc(
        bm_handle_t             handle,
        bm_image                input,
        bm_image                output,
        bmcv_ive_filter_ctrl    filter_attr,
        csc_type_t              csc_type);

| 【参数】

.. list-table:: bmcv_ive_filter_and_csc 参数表
    :widths: 20 15 35

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
      - 输出 bm_image 对象结构体，不能为空，宽、高同 input。
    * - \filter_attr
      - 输入
      - 滤波控制参数结构体，不能为空。
    * - \csc_type
      - 输入
      - 色彩转换模式选择，不能为空。

.. list-table::
    :widths: 20 40 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - NV21

        NV61
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - output
      - RGB_PLANAR

        RGB_PACKAGE
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input

| 【数据类型说明】

【说明】定义 模板滤波加色彩空间转换符合功能控制信息。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_filter_ctrl_s{
        signed char as8_mask[25];
        unsigned char u8_norm;
    } bmcv_ive_filter_ctrl;

.. list-table::
    :widths: 58 100

    * - **成员名称**
      - **描述**
    * - as8_mask
      - 5x5 模板系数。
    * - u8_norm
      - 归一化参数，取值范围：[0， 13]。


【说明】定义色彩空间转换控制模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum csc_type {
        CSC_YCbCr2RGB_BT601 = 0,
        CSC_YPbPr2RGB_BT601,
        CSC_RGB2YCbCr_BT601,
        CSC_YCbCr2RGB_BT709,
        CSC_RGB2YCbCr_BT709,
        CSC_RGB2YPbPr_BT601,
        CSC_YPbPr2RGB_BT709,
        CSC_RGB2YPbPr_BT709,
        CSC_YCbCr2HSV_BT601,
        CSC_YCbCr2HSV_BT709,
        CSC_YCbCr2LAB_BT601,
        CSC_YCbCr2LAB_BT709,
        CSC_RGB2HSV,
        CSC_RGB2GRAY,
        CSC_FANCY_PbPr_BT601 = 100,
        CSC_FANCY_PbPr_BT709,
        CSC_USER_DEFINED_MATRIX = 1000,
        CSC_MAX_ENUM
    } csc_type_t;


| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 仅支持 YUV2RGB 的 4 种工作模式，具体参见 bmcv_ive_csc。

2. 输入输出图像的 width 都需要16对齐。


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
        int thrSize = 0; //0 -> 3x3; 1 -> 5x5
        unsigned char u8Norm = 4;
        int height = 288, width = 352;
        csc_type_t csc_type = CSC_MAX_ENUM;
        bm_image_format_ext src_fmt = FORMAT_NV21, dst_fmt = FORMAT_RGB_PLANAR;
        char *src_name = "ive_data/00_352x288_SP420.yuv";
        bm_handle_t handle = NULL;
        int ret = (int)bm_dev_request(&handle, dev_id);
        if (ret != 0) {
            printf("Create bm handle failed. ret = %d\n", ret);
            exit(-1);
        }
        bm_image src, dst;
        int src_stride[4], dst_stride[4];
        unsigned int i = 0, loop_time = 0;
        unsigned long long time_single, time_total = 0, time_avg = 0;
        unsigned long long time_max = 0, time_min = 10000, fps_actual = 0, pixel_per_sec = 0;
        struct timeval tv_start;
        struct timeval tv_end;
        struct timeval timediff;
        // filter data
        signed char arr3by3[25] = {0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0, 2,
                            4, 2, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0};

        signed char arr5by5[25] = {1, 2, 3, 2, 1, 2, 5, 6, 5, 2, 3, 6,
                            8, 6, 3, 2, 5, 6, 5, 2, 1, 2, 3, 2, 1};

        // config setting
        bmcv_ive_filter_ctrl filterAttr;

        filterAttr.u8_norm = u8Norm;
        (thrSize == 0) ? memcpy(filterAttr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char)) :
                          memcpy(filterAttr.as8_mask, arr5by5, 5 * 5 * sizeof(signed char)) ;

        // calc ive image stride && create bm image struct
        bm_ive_image_calc_stride(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, src_stride);
        bm_ive_image_calc_stride(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, dst_stride);

        bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
        bm_image_create(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);

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
            ret = bmcv_ive_filter_and_csc(handle, src, dst, filterAttr, csc_type);
            gettimeofday(&tv_end, NULL);
            timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
            timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
            time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

            if(time_single>time_max){time_max = time_single;}
            if(time_single<time_min){time_min = time_single;}
            time_total = time_total + time_single;

            if(ret != BM_SUCCESS){
                printf("bmcv_image_ive_filterAndCsc is failed \n");
                exit(-1);
            }
        }

        time_avg = time_total / loop_time;
        fps_actual = 1000000 / time_avg;
        pixel_per_sec = width * height * fps_actual/1024/1024;

        bm_image_destroy(&src);
        bm_image_destroy(&dst);
        printf("bm_ive_filterAndCsc: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu, %lluM pps\n",
            loop_time, time_max, time_avg, fps_actual, pixel_per_sec);


        return 0;
      }




















