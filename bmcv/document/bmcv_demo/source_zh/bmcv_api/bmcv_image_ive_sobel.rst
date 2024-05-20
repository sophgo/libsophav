bmcv_ive_sobel
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建 5x5 模板 Sobel-like 梯度计算任务，计算图像梯度方向的近似灰度值，计算过程与mag_and_ang完全相同。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_sobel(
          bm_handle_t            handle,
          bm_image *             input,
          bm_image *             output_h,
          bm_image *             output_v,
          bmcv_ive_sobel_ctrl    sobel_attr);

| 【参数】

.. list-table:: bmcv_ive_sobel 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \*input
      - 输入
      - 输入 bm_image 对象指针, 不能为空。
    * - \*output_h
      - 输出
      - 输出 bm_image 对象指针, 由模板直接滤波得到的梯度分量图像 H 指针, 根据 sobel_attr->enOutCtrl, 若需要输出则不能为空, 宽、高同 input。
    * - \*output_v
      - 输出
      - 输出 bm_image 对象指针, 由模板直接滤波得到的梯度分量图像 V 指针, 根据 sobel_attr->enOutCtrl, 若需要输出则不能为空, 宽、高同 input。
    * - sobel_attr
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
      - DATA_TYPE_EXT_1N_BYTE
      - 16x16~1920x1080
    * - hOutput
      - GRAY
      - DATA_TYPE_EXT_S16
      - 同 input
    * - vOutput
      - GRAY
      - DATA_TYPE_EXT_S16
      - 同 input

| 【数据类型说明】

【说明】定义 Sobel 输出控制信息。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_sobel_out_mode_e{
        BM_IVE_SOBEL_OUT_MODE_BOTH = 0x0,
        BM_IVE_SOBEL_OUT_MODE_HOR  = 0x1,
        BM_IVE_SOBEL_OUT_MODE_VER  = 0X2,
    } bmcv_ive_sobel_out_mode;

.. list-table::
    :widths: 100 100

    * - **成员名称**
      - **描述**
    * - BM_IVE_SOBEL_OUT_MODE_BOTH
      - 同时输出用模板和转置模板滤波的结果。
    * - BM_IVE_SOBEL_OUT_MODE_HOR
      - 仅输出用模板直接滤波的结果。
    * - BM_IVE_SOBEL_OUT_MODE_VER
      - 仅输出用转置模板滤波的结果。

【说明】定义 Sobel-like 梯度计算控制信息。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_sobel_ctrl_s{
        bmcv_ive_sobel_out_mode sobel_mode;
        signed char as8_mask[25];
    } bmcv_ive_sobel_ctrl;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - sobel_mode
      - 出控制枚举参数。
    * - as8_mask
      - 5x5 模板系数。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入输出图像的 width 都需要16对齐。
2. 可配置 3 种滤波模式, 参考 bmcv_ive_sobel_out_mode 说明。

3. 当输出模式为 BM_IVE_SOBEL_OUT_MODE_BOTH 时, 要求 output_h 与 output_v 跨度一致。

4. 计算公式如下：

    .. math::
       \begin{aligned}
        & Hout(x, y) = \sum_{-2 < i < 2} \sum_{-2 < j < 2} I(x+i, y+j) \cdot \text{coef}(i, j) \\
        & Vout(x, y) = \sum_{-2 < j < 2} \sum_{-2 < i < 2} I(x+i, y+j) \cdot \text{coef}(j, i)
      \end{aligned}

  其中, :math:`I(x, y)` 对应 input, :math:`Hout(x, y)` 对应 output_h, :math:`Vout(x, y)` 对应 output_v, :math:`\text{coef}(mask)` 是 sobel_attr.as8_mask。

5. 模板

- Sobel 模板

  .. raw:: latex

   \begin{align*}
   \hspace{2em} % 控制间隔
   \begin{bmatrix}
      0 & 0 & 0 & 0 & 0 \\
      0 & -1 & 0 & 1 & 0 \\
      0 & -2 & 0 & 2 & 0 \\
      0 & -1 & 0 & 1 & 0 \\
      0 & 0 & 0 & 0 & 0 \\
   \end{bmatrix}
   \hspace{5em} % 控制间隔
   \begin{bmatrix}
      0 & 0 & 0 & 0 & 0 \\
      0 & -1 & -2 & -1 &  \\
      0 & 0 & 0 & 0 & 0 \\
      0 & 1 & 2 & 1 & 0 \\
      0 & 0 & 0 & 0 & 0 \\
   \end{bmatrix}
   \end{align*}

  .. raw:: latex

   \begin{align*}
   \hspace{2em} % 控制间隔
   \begin{bmatrix}
      -1 & -2 & 0 & 2 & 1 \\
      -4 & -8 & 0 & 8 & 4 \\
      -6 & -12 & 0 & 12 & 6 \\
      -4 & -8 & 0 & 8 & 4 \\
      -1 & -2 & 0 & 2 & 1 \\
   \end{bmatrix}
   \hspace{5em} % 控制间隔
   \begin{bmatrix}
      -1 & -4 & -6 & -4 & -1 \\
      -2 & -8 & -12 & -8 & -2 \\
      0 & 0 & 0 & 0 & 0 \\
      2 & 8 & 12 & 8 & 2 \\
      1 & 4 & 6 & 4 & 1 \\
   \end{bmatrix}
   \end{align*}

- Scharr 模板

  .. raw:: latex

   \begin{align*}
   \hspace{2em} % 控制间隔
   \begin{bmatrix}
      0 & 0 & 0 & 0 & 0 \\
      0 & -3 & 0 & 3 & 0 \\
      0 & -10 & 0 & 10 & 0 \\
      0 & -3 & 0 & 3 & 0 \\
      0 & 0 & 0 & 0 & 0 \\
   \end{bmatrix}
   \hspace{5em} % 控制间隔
   \begin{bmatrix}
      0 & 0 & 0 & 0 & 0 \\
      0 & -3 & -10 & -3 &  \\
      0 & 0 & 0 & 0 & 0 \\
      0 & 3 & 10 & 3 & 0 \\
      0 & 0 & 0 & 0 & 0 \\
   \end{bmatrix}
   \end{align*}

- 拉普拉斯模板

  .. raw:: latex

   \begin{align*}
   \hspace{2em} % 控制间隔
   \begin{bmatrix}
      0 & 0 & 0 & 0 & 0 \\
      0 & 0 & 1 & 0 & 0 \\
      0 & 1 & -4 & 1 & 0 \\
      0 & 0 & 1 & 0 & 0 \\
      0 & 0 & 0 & 0 & 0 \\
   \end{bmatrix}
   \hspace{5em} % 控制间隔
   \begin{bmatrix}
      0 & 0 & 0 & 0 & 0 \\
      0 & 0 & -1 & 0 & 0 \\
      0 & -1 & 4 & -1 & 0 \\
      0 & 0 & -1 & 0 & 0 \\
      0 & 0 & 0 & 0 & 0 \\
   \end{bmatrix}
   \end{align*}

  .. raw:: latex

   \begin{align*}
   \hspace{2em} % 控制间隔
   \begin{bmatrix}
      0 & 0 & 0 & 0 & 0 \\
      0 & 1 & 1 & 1 & 0 \\
      0 & 1 & -8 & 1 & 0 \\
      0 & 1 & 1 & 1 & 0 \\
      0 & 0 & 0 & 0 & 0 \\
   \end{bmatrix}
   \hspace{5em} % 控制间隔
   \begin{bmatrix}
      0 & 0 & 0 & 0 & 0 \\
      0 & -1 & -1 & -1 & 0 \\
      0 & -1 & 8 & -1 & 0 \\
      0 & -1 & -1 & -1 & 0 \\
      0 & 0 & 0 & 0 & 0 \\
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
        int height = 288, width = 352;
        int thrSize = 0; // 0 -> 3x3  1-> 5x5
        bmcv_ive_sobel_out_mode enMode = BM_IVE_SOBEL_OUT_MODE_BOTH;
        bm_image_format_ext fmt = FORMAT_GRAY;
        char *src_name = "./data/00_352x288_y.yuv";
        char *sobel_hName = "iveSobelH_res.yuv", *sobel_vName = "iveSobelV_res.yuv";
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
        bm_image dst_H, dst_V;
        int src_stride[4];
        int dst_stride[4];
        unsigned int i = 0, loop_time = 0;
        unsigned long long time_single, time_total = 0, time_avg = 0;
        unsigned long long time_max = 0, time_min = 10000, fps_actual = 0;
        struct timeval tv_start;
        struct timeval tv_end;
        struct timeval timediff;
        bmcv_ive_sobel_ctrl sobelAtt;
        sobelAtt.sobel_mode = enMode;
        (thrSize == 0) ? (memcpy(sobelAtt.as8_mask, arr3by3, 5 * 5 * sizeof(signed char))) :
                          (memcpy(sobelAtt.as8_mask, arr5by5, 5 * 5 * sizeof(signed char)));

        // calc ive image stride && create bm image struct
        bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, src_stride);
        bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            printf("src bm_image_alloc_dev_mem_src. ret = %d\n", ret);
            exit(-1);
        }
        bm_ive_read_bin(src, src_name);

        bm_ive_image_calc_stride(handle, height, width, fmt, DATA_TYPE_EXT_S16, dst_stride);
        if(enMode == BM_IVE_SOBEL_OUT_MODE_BOTH || enMode == BM_IVE_SOBEL_OUT_MODE_HOR){
            bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_S16, &dst_H, dst_stride);
            ret = bm_image_alloc_dev_mem(dst_H, BMCV_HEAP_ANY);
            if (ret != BM_SUCCESS) {
                printf("dst_H bm_image_alloc_dev_mem_src. ret = %d\n", ret);
                exit(-1);
            }
        }

        if(enMode == BM_IVE_SOBEL_OUT_MODE_BOTH || enMode == BM_IVE_SOBEL_OUT_MODE_VER){
            bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_S16, &dst_V, dst_stride);
            ret = bm_image_alloc_dev_mem(dst_V, BMCV_HEAP_ANY);
            if (ret != BM_SUCCESS) {
                printf("dst_V bm_image_alloc_dev_mem_src. ret = %d\n", ret);
                exit(-1);
            }
        }

        for (i = 0; i < loop_time; i++) {
            gettimeofday(&tv_start, NULL);
            ret = bmcv_ive_sobel(handle, &src, &dst_H, &dst_V, sobelAtt);
            gettimeofday(&tv_end, NULL);
            timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
            timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
            time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);

            if(time_single>time_max){time_max = time_single;}
            if(time_single<time_min){time_min = time_single;}
            time_total = time_total + time_single;

            if(ret != BM_SUCCESS){
                printf("bmcv_ive_sobel failed, ret is %d \n", ret);
                exit(-1);
            }
        }

        time_avg = time_total / loop_time;
        fps_actual = 1000000 / time_avg;
        bm_image_destroy(&src);
        if(enMode == BM_IVE_SOBEL_OUT_MODE_BOTH || enMode == BM_IVE_SOBEL_OUT_MODE_HOR)
            bm_image_destroy(&dst_H);

        if(enMode == BM_IVE_SOBEL_OUT_MODE_BOTH || enMode == BM_IVE_SOBEL_OUT_MODE_VER)
            bm_image_destroy(&dst_V);
        printf("idx:%d, bmcv_ive_sobel: loop %d cycles, time_max = %llu, time_avg = %llu, fps %llu \n",
                ctx.i, loop_time, time_max, time_avg, fps_actual);
        printf("bmcv ive sobel test successful \n");
        return 0;
      }












