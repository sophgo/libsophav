
bmcv_tde_draw
------------------------------

| 【描述】

| 利用 TDE 硬件在 bm_image 的指定位置填充不规则图形。输入各顶点坐标，按顺序连接顶点，最后一个顶点则连接首顶点，填充围成的所有封闭部分。


| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_tde_draw(
      bm_handle_t      handle,
      bm_image         image,
      bmcv_point_t    *point,
      int              path_num,
      bmcv_color_ext   color)

| 【参数】

.. list-table:: bmcv_tde_draw 参数表
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
    * - \point
      - 输入
      - 多边形顶点坐标指针。
    * - \path_num
      - 输入
      - 多边形顶点数。
    * - \color
      - 输入
      - 填充图形颜色，包含 A/R/G/B 四个值。可调节 A 来改变填充封闭图形的透明度。

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
        bmcv_point_t point[5] =
          {{.x = 100, .y = 70},
            {.x = 128, .y = 90},
            {.x = 117, .y = 124},
            {.x = 82, .y = 124},
            {.x = 71, .y = 90}}; //五边形
        int h = 1080, w = 1920;
        bm_image_format_ext fmt = FORMAT_RGB_PACKED;
        int point_num = 5;

        bm_dev_request(&handle, 0);
        bm_image_create(handle, h, w, fmt, DATA_TYPE_EXT_1N_BYTE, &image, NULL);
        bm_image_alloc_dev_mem(image, BMCV_HEAP1_ID);
        bm_read_bin(image, "/opt/sophon/libsophon-current/bin/res/1920x1080_rgb.bin");

        bmcv_tde_draw(handle, image, point, point_num, color);

        bm_write_bin(image, "out.raw");
        bm_image_destroy(&image);
        bm_dev_free(handle);
        return 0;
      }