
bmcv_tde_fill
------------------------------

| 【描述】

| 利用 TDE 硬件将 bm_image 的指定位置填充指定颜色。


| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_tde_fill(
      bm_handle_t    handle,
      bm_image       image,
      bmcv_color_ext color,
      bmcv_rect_t*   rect);

| 【参数】

.. list-table:: bmcv_tde_fill 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \image
      - 输入
      - 待处理图像的 bm_image。
    * - \color
      - 输入
      - 指定颜色，包含 A/R/G/B 四个值。当底图为非 ARGB 格式时， A 数值不会生效。
    * - \rect
      - 输入
      - 填充位置的指针，传入 NULL 表示全图填充。

| 【结构体】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct {
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;
    } bmcv_color_ext;

| 【格式支持】

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
        bm_image image;
        bmcv_color_ext color = {.a = 255, .r = 255, .g = 0, .b = 0};
        bmcv_rect_t rect = {.start_x = 0, .start_y = 0, .crop_w = 1920, .crop_h = 1080};
        int h = 1080, w = 1920;
        bm_image_format_ext fmt = FORMAT_ARGB_PACKED;

        bm_dev_request(&handle, 0);
        bm_image_create(handle, h, w, fmt, DATA_TYPE_EXT_1N_BYTE, &image, NULL);
        bm_image_alloc_dev_mem(image, BMCV_HEAP1_ID);

        bmcv_tde_fill(handle, image, color, &rect);

        bm_write_bin(image, "out.raw");
        bm_image_destroy(&image);
        bm_dev_free(handle);
        return 0;
      }