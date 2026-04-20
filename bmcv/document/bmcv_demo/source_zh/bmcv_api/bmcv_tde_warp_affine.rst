
bmcv_tde_warp_affine
------------------------------

| 【描述】

| 利用 TDE 硬件对图像执行仿射变换操作。


| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_tde_warp_affine(
      bm_handle_t        handle,
      bmcv_affine_matrix matrix,
      bm_image           input,
      bm_image           output,
      int                use_bilinear);

| 【参数】

.. list-table:: bmcv_tde_warp_affine 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \matrix
      - 输入
      - 仿射变换矩阵。
    * - \input
      - 输入
      - 输入图像的 bm_image。
    * - \output
      - 输出
      - 输出图像的 bm_image。
    * - \use_bilinear
      - 输入
      - 输出是否用 bilinear 插值，设置为 0 则为 nearest 取点。

| 【结构体】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_affine_matrix_s {
        float m[6];
    } bmcv_affine_matrix;

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
        int src_h = 1080, src_w = 1920, dst_h = 500, dst_w = 500;
        bm_image_format_ext src_fmt = FORMAT_RGB_PACKED;
        bm_image_format_ext dst_fmt = FORMAT_RGB_PACKED;
        bmcv_affine_matrix matrix;

        bm_dev_request(&handle, 0);
        bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &input, NULL);
        bm_image_alloc_dev_mem(input, BMCV_HEAP1_ID);
        bm_read_bin(input, "/opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin");

        bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &output, NULL);
        bm_image_alloc_dev_mem(output, BMCV_HEAP1_ID);

        matrix.m[0] = 1.0; matrix.m[1] = 0.0; matrix.m[2] = -100.0;
        matrix.m[3] = 0.0; matrix.m[4] = 1.0; matrix.m[5] = 0.0;

        bmcv_tde_warp_affine(handle, matrix, input, output, 1);

        bm_write_bin(output, "out.raw");
        bm_image_destroy(&input);
        bm_image_destroy(&output);
        bm_dev_free(handle);
        return 0;
      }