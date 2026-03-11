
bmcv_tde_line
------------------------------

| 【描述】

| 利用 TDE 硬件在 bm_image 的指定位置划线。


| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_tde_line(
      bm_handle_t      handle,
      bm_image         image,
      bmcv_point_t     start,
      bmcv_point_t     end,
      bmcv_color_ext   color,
      int              thick);

| 【参数】

.. list-table:: bmcv_tde_line 参数表
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
    * - \start
      - 输入
      - 划线左上坐标。
    * - \end
      - 输入
      - 划线右下坐标。
    * - \color
      - 输入
      - 划线颜色，包含 A/R/G/B 四个值。可调节 A 来改变填充封闭图形的透明度。
    * - \thick
      - 输入
      - 划线宽度。

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
        bmcv_point_t start = {.x = 0, .y = 0};
        bmcv_point_t end = {.x = 200, .y = 200};
        int h = 1080, w = 1920;
        bm_image_format_ext fmt = FORMAT_RGB_PACKED;
        int thick = 2;

        bm_dev_request(&handle, 0);
        bm_image_create(handle, h, w, fmt, DATA_TYPE_EXT_1N_BYTE, &image, NULL);
        bm_image_alloc_dev_mem(image, BMCV_HEAP1_ID);
        bm_read_bin(image, "/opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin");

        bmcv_tde_line(handle, image, start, end, color, thick);

        bm_write_bin(image, "out.raw");
        bm_image_destroy(&image);
        bm_dev_free(handle);
        return 0;
      }