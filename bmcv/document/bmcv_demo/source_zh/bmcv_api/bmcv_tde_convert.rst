
bmcv_tde_convert
------------------------------

| 【描述】

| 利用 TDE 硬件完成 bm_image 的 crop + resize + csc + rotate 变换， 并以指定方式融合到目标 bm_image 的指定位置。


| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_tde_convert(
      bm_handle_t    handle,
      bm_image       input,
      bm_image       output,
      int            rot_angle,
      int            global_alpha,
      bmcv_rect_t*   src_rect,
      bmcv_rect_t*   dst_rect);

| 【参数】

.. list-table:: bmcv_tde_convert 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input
      - 输入
      - 待处理图像的 bm_image。
    * - \output
      - 输入
      - 输出图像的 bm_image。
    * - \rot_angle
      - 输入
      - 旋转角度，支持 90/180/270 度旋转，传入 0 表示不旋转。
    * - \global_alpha
      - 输入
      - 全局透明度。取值为 1-255 时，统一调整 Src 的 alpha 通道。取值为 -15-0 时，按指定公式处理。详见下文。
    * - \src_rect
      - 输入
      - crop 位置的指针，传入 NULL 表示不进行 crop。
    * - \dst_rect
      - 输入
      - 输出位置的指针，传入 NULL 表示拉伸到输出图的宽高。

| 【输入格式支持】

+-----+-------------------------------+
| num | input image_format            |
+=====+===============================+
|  1  | FORMAT_YUV420P                |
+-----+-------------------------------+
|  2  | FORMAT_YUV422P                |
+-----+-------------------------------+
|  3  | FORMAT_YUV444P                |
+-----+-------------------------------+
|  4  | FORMAT_NV12                   |
+-----+-------------------------------+
|  5  | FORMAT_NV16                   |
+-----+-------------------------------+
|  6  | FORMAT_YUV422_YUYV            |
+-----+-------------------------------+
|  7  | FORMAT_RGB_PACKED             |
+-----+-------------------------------+
|  8  | FORMAT_BGR_PACKED             |
+-----+-------------------------------+
|  9  | FORMAT_ARGB_PACKED            |
+-----+-------------------------------+
| 10  | FORMAT_ABGR_PACKED            |
+-----+-------------------------------+
| 11  | FORMAT_ARGB1555_PACKED        |
+-----+-------------------------------+
| 12  | FORMAT_ABGR1555_PACKED        |
+-----+-------------------------------+
| 13  | FORMAT_ARGB4444_PACKED        |
+-----+-------------------------------+
| 14  | FORMAT_ABGR4444_PACKED        |
+-----+-------------------------------+

| 【输出格式支持】

+-----+-------------------------------+
| num | input image_format            |
+=====+===============================+
|  1  | FORMAT_RGB_PACKED             |
+-----+-------------------------------+
|  2  | FORMAT_BGR_PACKED             |
+-----+-------------------------------+
|  3  | FORMAT_ARGB_PACKED            |
+-----+-------------------------------+
|  4  | FORMAT_ABGR_PACKED            |
+-----+-------------------------------+
|  5  | FORMAT_ARGB1555_PACKED        |
+-----+-------------------------------+
|  6  | FORMAT_ABGR1555_PACKED        |
+-----+-------------------------------+
|  7  | FORMAT_ARGB4444_PACKED        |
+-----+-------------------------------+
|  8  | FORMAT_ABGR4444_PACKED        |
+-----+-------------------------------+

| 【注意事项】

该接口限制输入的 bm_image 的 stride 为 32 对齐。

global_alpha 取值为 0-255 时，表示统一调整 Src 的 alpha 通道，融合时计算公式为

Src.alpha = Src.alpha * (global_alpha / 255)

Out = (Src.alpha / 255) * Src + ((255 - Src.alpha) / 255) * Dst

当 Src 没有 alpha 通道时， Src.alpha 取255。

global_alpha 取值为 -15-0 时，表示按指定公式处理，如下：

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bm_tde_overlay {
        BM_BLEND_NONE                 = 0,  /*! S, No blend */
        BM_BLEND_SRC_OVER             = -1, /*! S + (1 - S.a) * D */
        BM_BLEND_DST_OVER             = -2, /*! (1 - D.a) * S + D */
        BM_BLEND_SRC_IN               = -3, /*! D.a * S */
        BM_BLEND_DST_IN               = -4, /*! S.a * D */
        BM_BLEND_MULTIPLY             = -5, /*! S * (1 - D.a) + D * (1 - S.a) + S * D */
        BM_BLEND_SCREEN               = -6, /*! S + D - S * D */
        BM_BLEND_DARKEN               = -7, /*! min(SrcOver, DstOver) */
        BM_BLEND_LIGHTEN              = -8, /*! max(SrcOver, DstOver) */
        BM_BLEND_ADDITIVE             = -9, /*! S + D */
        BM_BLEND_SUBTRACT             = -10,/*! D * (1 - S.a) */
        BM_BLEND_SUBTRACT_LVGL        = -11,/*! D - S */
        BM_BLEND_NORMAL_LVGL          = -12,/*! S * S.a + (1 - S.a) * D */
        BM_BLEND_ADDITIVE_LVGL        = -13,/*! (S + D) * S.a + D * (1 - S.a) */
        BM_BLEND_MULTIPLY_LVGL        = -14,/*! (S * D) * S.a + D * (1 - S.a) */
        BM_BLEND_PREMULTIPLY_SRC_OVER = -15,/*! S * S.a + (1 - S.a) * D */
    } bm_tde_overlay_t;

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【代码示例】

    .. code-block:: c

      #include <stdio.h>
      #include <stdlib.h>
      #include <string.h>
      #include <assert.h>
      #include <math.h>
      #include "bmcv_api_ext_c.h"

      int main() {
        bm_handle_t handle;
        bm_image input, output;
        bmcv_rect_t src_rect = {.start_x = 0, .start_y = 0, .crop_w = 1920, .crop_h = 1080};
        bmcv_rect_t dst_rect = {.start_x = 0, .start_y = 0, .crop_w = 2560, .crop_h = 1440};
        int src_h = 1080, src_w = 1920, dst_h = 2560, dst_w = 1440;
        bm_image_format_ext src_fmt = FORMAT_RGB_PACKED;
        bm_image_format_ext dst_fmt = FORMAT_ARGB_PACKED;

        bm_dev_request(&handle, 0);
        bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &input, NULL);
        bm_image_alloc_dev_mem(input, BMCV_HEAP1_ID);
        bm_read_bin(input, "/opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin");

        bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &output, NULL);
        bm_image_alloc_dev_mem(output, BMCV_HEAP1_ID);

        bmcv_tde_convert(handle, input, output, 0, 0, &src_rect, &dst_rect);

        bm_write_bin(output, "out.raw");
        bm_image_destroy(&input);
        bm_image_destroy(&output);
        bm_dev_free(handle);
        return 0;
      }