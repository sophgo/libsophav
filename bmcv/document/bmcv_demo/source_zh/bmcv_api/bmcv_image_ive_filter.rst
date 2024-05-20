bmcv_ive_filter
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建 5x5 模板滤波任务, 通过配置不同的模板系数, 可以实现不同的滤波。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_filter(
        bm_handle_t            handle,
        bm_image               input,
        bm_image               output,
        bmcv_ive_filter_ctrl   filter_attr);

| 【参数】

.. list-table:: bmcv_ive_filter 参数表
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
      - 输出 bm_image 对象结构体，不能为空, 宽、高同 input。
    * - filter_attr
      - 输入
      - 控制信息结构体, 不能为空。

.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY

        NV21

        NV61
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - output
      - 同input
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input

| 【数据类型说明】

【说明】定义模板滤波控制信息。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_filter_ctrl_s{
        signed char as8_mask[25];
        unsigned char u8_norm;
    } bmcv_ive_filter_ctrl;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - as8_mask
      - 5x5 模板系数，外围系数设为0，可实现 3x3 模板滤波。
    * - u8_norm
      - 归一化参数，取值范围: [0, 13]。

【注意】

* 通过配置不同的模板系数可以达到不同的滤波效果。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 当输入数据 为 FORMAT_NV21、FORMAT_NV61 类型时, 要求输出数据跨度一致。

3. Filter 计算公式:

  .. math::

    I_{\text{out}}(x, y) = \left\{ \sum_{-2 < j < 2} \sum_{-2 < i < 2} I(x+i, y+j) \cdot \text{coef}(i, j) \right\} \gg \text{norm}(x)

4. 经典高斯模板如下， 其中 norm_u8 分别等于4、8、8:

  .. raw:: latex

   \begin{align*}
   \hspace{2em} % 控制间隔
   \begin{bmatrix}
      0 & 0 & 0 & 0 & 0 \\
      0 & 1 & 2 & 1 & 0 \\
      0 & 2 & 4 & 2 & 0 \\
      0 & 1 & 2 & 1 & 0 \\
      0 & 0 & 0 & 0 & 0 \\
   \end{bmatrix}
   \hspace{5em} % 控制间隔
   \begin{bmatrix}
      1 & 2 & 3 & 2 & 1 \\
      2 & 5 & 6 & 5 & 2 \\
      3 & 6 & 8 & 6 & 3 \\
      2 & 5 & 6 & 5 & 2 \\
      1 & 2 & 3 & 2 & 1 \\
   \end{bmatrix}
   \times 3
   \hspace{3.5em} % 控制间隔
   \begin{bmatrix}
      1 & 4 & 7 & 4 & 1 \\
      4 & 16 & 26 & 16 & 4 \\
      7 & 26 & 41 & 26 & 7 \\
      4 & 16 & 26 & 16 & 4 \\
      1 & 4 & 7 & 4 & 1 \\
   \end{bmatrix}
   \end{align*}


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
      extern bm_status_t bm_ive_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
          bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride);
      int main(){
        int dev_id = 0;
        int thrSize = 0; //0 -> 3x3; 1 -> 5x5
        unsigned char u8Norm = 4;
        int height = 288, width = 352;
        bm_image_format_ext fmt = FORMAT_GRAY; // 14 4 6
        char *src_name = "./data/00_352x288_y.yuv";
        char *dst_name = "ive_filter_res.yuv";
        bm_handle_t handle = NULL;
        int ret = (int)bm_dev_request(&handle, dev_id);
        if (ret != 0) {
            printf("Create bm handle failed. ret = %d\n", ret);
            exit(-1);
        }
        signed char arr5by5[25] = { 1, 2, 3, 2, 1, 2, 5, 6, 5, 2, 3, 6, 8,
                            6, 3, 2, 5, 6, 5, 2, 1, 2, 3, 2, 1 };
        signed char arr3by3[25] = { 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0, 2, 4,
                            2, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0 };
        bm_image src, dst;
        int stride[4];
        unsigned int i = 0, loop_time = 0;
        unsigned long long time_single, time_total = 0, time_avg = 0;
        unsigned long long time_max = 0, time_min = 10000, fps_actual = 0;
        struct timeval tv_start;
        struct timeval tv_end;
        struct timeval timediff;

        bmcv_ive_filter_ctrl filterAttr;
        (thrSize == 0) ? memcpy(filterAttr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char)) :
                        memcpy(filterAttr.as8_mask, arr5by5, 5 * 5 * sizeof(signed char));
        filterAttr.u8_norm = u8Norm;

        // calc ive image stride && create bm image struct
        bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, stride);
        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, stride);

        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("src bm_image_alloc_dev_mem failed. ret = %d\n", ret);
            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            exit(-1);
        }

        ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("src bm_image_alloc_dev_mem failed. ret = %d\n", ret);
            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            exit(-1);
        }

        bm_ive_read_bin(src, src_name);

        for (i = 0; i < loop_time; i++) {
            gettimeofday(&tv_start, NULL);
            ret = bmcv_ive_filter(handle, src, dst, filterAttr);
            gettimeofday(&tv_end, NULL);
            timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
            timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
            time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

            if(time_single>time_max){time_max = time_single;}
            if(time_single<time_min){time_min = time_single;}
            time_total = time_total + time_single;

            if(ret != BM_SUCCESS){
                printf("bmcv_ive_filter failed. ret = %d\n", ret);
                bm_image_destroy(&src);
                bm_image_destroy(&dst);
                exit(-1);
            }
        }

        time_avg = time_total / loop_time;
        fps_actual = 1000000 / time_avg;
        bm_image_destroy(&src);
        bm_image_destroy(&dst);
        printf("bmcv_ive_filter: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
                loop_time, time_max, time_avg, fps_actual);
        printf("bmcv ive filter test successful \n");
        return 0;
      }


