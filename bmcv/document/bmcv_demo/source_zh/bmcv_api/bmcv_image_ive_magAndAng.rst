bmcv_ive_mag_and_ang
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建 5x5 模板，计算灰度图的每个像素区域位置灰度值变化梯度的幅值和幅角任务。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_mag_and_ang(
        bm_handle_t            handle,
        bm_image  *             input,
        bm_image  *            mag_output,
        bm_image  *            ang_output,
        bmcv_ive_mag_and_ang_ctrl  attr);

| 【参数】

.. list-table:: bmcv_ive_magandang 参数表
    :widths: 20 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \*input
      - 输入
      - 输入 bm_image 对象指针, 不能为空。
    * - \*mag_output
      - 输出
      - 输出 bm_image 对象数据指针, 输出幅值图像, 不能为空, 宽、高同 input。
    * - \*ang_output
      - 输出
      - 输出 bm_image 对象数据指针, 输出幅角图像。根据 attr.en_out_ctrl, 需要输出则不能为空, 宽、高同 input。
    * - \attr
      - 输入
      - 控制信息结构体, 存放梯度幅值和幅角计算的控制信息，不能为空。


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
    * - mag_output
      - GRAY
      - DATA_TYPE_EXT_U16
      - 同 input
    * - ang_output
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input


| 【数据类型说明】

【说明】定义梯度幅值与角度计算的输出格式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_mag_and_ang_outctrl_e{
        BM_IVE_MAG_AND_ANG_OUT_MAG = 0x0,
        BM_IVE_MAG_AND_ANG_OUT_ALL = 0X1,
    } bmcv_ive_mag_and_ang_outctrl;

.. list-table::
    :widths: 100 100

    * - **成员名称**
      - **描述**
    * - BM_IVE_MAG_AND_ANG_OUT_MAG
      - 仅输出幅值。
    * - BM_IVE_MAG_AND_ANG_OUT_ALL
      - 同时输出幅值和角度值。

【说明】定义梯度幅值和幅角计算的控制信息。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_mag_and_ang_ctrl_s{
        bmcv_ive_mag_and_ang_outctrl     en_out_ctrl;
        unsigned short               u16_thr;
        signed char                  as8_mask[25];
    } bmcv_ive_mag_and_ang_ctrl;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - en_out_ctrl
      - 输出格式。
    * - u16_thr
      - 用于对幅值进行阈值化的阈值。
    * - as8_mask
      - 5x5 模板系数。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 可配置两种输出模式, 详细请参考 bm_ive_mag_and_ang_outctrl。

3. 配置 BM_IVE_MAG_AND_ANG_OUT_ALL 时， 要求 mag_output 与 ang_output 跨度一致。

4. 用户可以通过 attr.u16_thr 对幅值图进行阈值化操作(可用来实现EOH), 计算公式如下:

  .. math::

    \text{Mag}(x, y) =
    \begin{cases}
        \text{0} & \ (Mag(x, y) < u16_thr) \\
        \text{Mag(x, y)} & \ (Mag(x, y) \geq u16_thr) \\
    \end{cases}

  其中, :math:`Mag(x, y)` 对应 mag_output。


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
        int dev_id = 0;int height = 288, width = 352;
        int templateSize = 0; // 0: 3x3 1:5x5
        unsigned short thrValue = 0;
        bm_image_format_ext fmt = FORMAT_GRAY;
        bmcv_ive_mag_and_ang_outctrl enMode = BM_IVE_MAG_AND_ANG_OUT_ALL;
        char *src_name = "./data/00_352x288_y.yuv";
        char *dst_magName = "ive_magAndAng_magResult.yuv", *dst_angName = "ive_magAndAng_angResult.yuv";
        bm_handle_t handle = NULL;
        int ret = (int)bm_dev_request(&handle, dev_id);
        if (ret != 0) {
            printf("Create bm handle failed. ret = %d\n", ret);
            exit(-1);
        }
        /* 5 by 5*/
        signed char arr5by5[25] = { -1, -2, 0,  2,  1, -4, -8, 0,  8,  4, -6, -12, 0,
                          12, 6,  -4, -8, 0, 8,  4,  -1, -2, 0, 2, 1 };
        /* 3 by 3*/
        signed char arr3by3[25] = { 0, 0, 0, 0,  0, 0, -1, 0, 1, 0, 0, -2, 0,
                          2, 0, 0, -1, 0, 1, 0,  0, 0, 0, 0, 0 };
        bm_image src;
        bm_image dst_mag, dst_ang;
        int stride[4];
        int magStride[4];
        unsigned int i = 0, loop_time = 0;
        unsigned long long time_single, time_total = 0, time_avg = 0;
        unsigned long long time_max = 0, time_min = 10000, fps_actual = 0;
        struct timeval tv_start;
        struct timeval tv_end;
        struct timeval timediff;
        bmcv_ive_mag_and_ang_ctrl magAndAng_attr;
        memset(&magAndAng_attr, 0, sizeof(bmcv_ive_mag_and_ang_ctrl));
        magAndAng_attr.en_out_ctrl = enMode;
        magAndAng_attr.u16_thr = thrValue;
        if(templateSize == 0)
            memcpy(magAndAng_attr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char));
        else
            memcpy(magAndAng_attr.as8_mask, arr5by5, 5 * 5 * sizeof(signed char));

        // calc ive image stride && create bm image struct
        bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, stride);
        bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_U16, magStride);

        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_U16, &dst_mag, magStride);

        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("src bm_image_alloc_dev_mem_src. ret = %d\n", ret);
            exit(-1);
        }

        ret = bm_image_alloc_dev_mem(dst_mag, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("dst_mag bm_image_alloc_dev_mem_src. ret = %d\n", ret);
            exit(-1);
        }
        bm_ive_read_bin(src, src_name);

        if(enMode == BM_IVE_MAG_AND_ANG_OUT_ALL){
            bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dst_ang, stride);
            ret = bm_image_alloc_dev_mem(dst_ang, BMCV_HEAP_ANY);
            if (ret != BM_SUCCESS) {
                printf("dst_mag bm_image_alloc_dev_mem_src. ret = %d\n", ret);
                exit(-1);
            }
        }

        for (i = 0; i < loop_time; i++) {
            gettimeofday(&tv_start, NULL);
            ret = bmcv_ive_mag_and_ang(handle, &src, &dst_mag, &dst_ang, magAndAng_attr);
            gettimeofday(&tv_end, NULL);
            timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
            timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
            time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

            if(time_single>time_max){time_max = time_single;}
            if(time_single<time_min){time_min = time_single;}
            time_total = time_total + time_single;

            if(ret != BM_SUCCESS){
                printf("bmcv_ive_magAndAng failed , ret is %d \n", ret);
                exit(-1);
            }
        }

        time_avg = time_total / loop_time;
        fps_actual = 1000000 / time_avg;

        bm_image_destroy(&src);
        bm_image_destroy(&dst_mag);
        if(enMode == BM_IVE_MAG_AND_ANG_OUT_ALL) bm_image_destroy(&dst_ang);
        printf(" bmcv_ive_magAndAng: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
                loop_time, time_max, time_avg, fps_actual);
        printf("bmcv ive magAndAng test successful \n");

        return 0;
      }






