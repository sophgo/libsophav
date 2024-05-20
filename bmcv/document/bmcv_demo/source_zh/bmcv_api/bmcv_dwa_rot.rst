bmcv_dwa_rot
---------------

| 【描述】
| 去畸变仿射(DWA)模块的旋转功能，通过围绕固定点旋转图像，改变其方向与角度，从而使其在平面上发生旋转。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_dwa_rot( bm_handle_t          handle,
                              bm_image             input_image,
                              bm_image             output_image,
                              bmcv_rot_mode        rot_mode);

| 【参数】

.. list-table:: bmcv_dwa_rot 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取
    * - input_image
      - 输入
      - 输入的待旋转图像，通过调用 bm_image_create 创建
    * - output_image
      - 输出
      - 输出的旋转后图像，通过调用 bm_image_create 创建
    * - rot_mode
      - 输入
      - 旋转类型，取值为[0, 4]内的整数，其中0为0°，1为90°，2为180°，3为270°，4为xy flip

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【数据类型说明】

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_rot_mode_ {
        BMCV_ROTATION_0 = 0,
        BMCV_ROTATION_90,
        BMCV_ROTATION_180,
        BMCV_ROTATION_270,
        BMCV_ROTATION_XY_FLIP,
        BMCV_ROTATION_MAX
    } bmcv_rot_mode;

* BMCV_ROTATION_0 代表0度的旋转，即图像不进行旋转，保持原状。
* BMCV_ROTATION_90 代表90度的旋转，即图像顺时针旋转90度。
* BMCV_ROTATION_180 代表180度的旋转，即图像顺时针旋转180度。
* BMCV_ROTATION_270 代表270度的旋转，即图像顺时针旋转270度。
* BMCV_ROTATION_XY_FLIP 代表XY翻转，即图像在X轴和Y轴上都进行翻转（镜像翻转）。
* BMCV_ROTATION_MAX 代表枚举最大值，用于指示枚举的结束或作为范围检查的标记。

| 【格式支持】

1. 输入和输出的数据类型：

+-----+-------------------------------+
| num | data_type                     |
+=====+===============================+
|  1  | DATA_TYPE_EXT_1N_BYTE         |
+-----+-------------------------------+

2. 输入和输出的色彩格式必须保持一致，可支持：

+-----+-------------------------------+
| num | image_format                  |
+=====+===============================+
|  1  | FORMAT_RGB_PLANAR             |
+-----+-------------------------------+
|  2  | FORMAT_YUV420P                |
+-----+-------------------------------+
|  3  | FORMAT_YUV444P                |
+-----+-------------------------------+
|  4  | FORMAT_GRAY                   |
+-----+-------------------------------+

| 【注意】

1. 输入输出所有 bm_image 结构必须提前创建，否则返回失败。

2. 支持图像的分辨率为32x32~4096x4096，且要求对齐。

| 【代码示例】

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <pthread.h>
    #include <sys/time.h>
    #include "bmcv_api_ext_c.h"
    #include <unistd.h>

    extern void bm_read_bin(bm_image src, const char *input_name);
    extern void bm_write_bin(bm_image dst, const char *output_name);
    extern int md5_cmp(unsigned char* got, unsigned char* exp ,int size);

    int main(int argc, char **argv) {
        bm_status_t ret = BM_SUCCESS;
        bm_handle_t handle = NULL;
        int dev_id = 0;
        char *src_name = "1920x1080_yuv420.bin";
        char *dst_name = "dst.bin";
        int src_h = 1080, src_w = 1920;
        int dst_w, dst_h;
        bm_image src, dst;
        bmcv_rot_mode rot_mode = BMCV_ROTATION_0;
        bm_image_format_ext fmt = FORMAT_YUV420P;
        int ret = (int)bm_dev_request(&handle, dev_id);
        bm_image_create(handle, src_h, src_w, fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
        dst_w = src_w;
        dst_h = src_h;
        bm_image_create(handle, dst_h, dst_w, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
        ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP_ANY);
        // read image data from input files
        bm_read_bin(src, src_name);
        bmcv_dwa_rot(handle, src, dst, rot_mode);
        bm_write_bin(dst, dst_name);

        return 0;
    }
