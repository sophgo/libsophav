bmcv_image_rotate
-----------------

| 【描述】
| 实现图像顺时针旋转90度，180度和270度

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_rotate(
        bm_handle_t handle,
        bm_image input,
        bm_image output,
        int rotation_angle);


| 【参数】

.. list-table:: bmcv_image_rotate 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取
    * - input
      - 输出
      - 输入的待旋转图像，通过调用 bm_image_create 创建
    * - output
      - 输出
      - 输出的旋转后图像，通过调用 bm_image_create 创建
    * - rotation_angle
      - 输入
      - 顺时针旋转角度。可选角度90度，180度，270度

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【格式支持】

1. 输入和输出的数据类型：

+-----+-------------------------------+
| num | data_type                     |
+=====+===============================+
|  1  | DATA_TYPE_EXT_1N_BYTE         |
+-----+-------------------------------+

2. 输入和输出的色彩格式必须保持一致，可支持：

+-----+------------------------+
| num | image_format           |
+=====+========================+
| 1   | FORMAT_YUV444P         |
+-----+------------------------+
| 2   | FORMAT_NV12            |
+-----+------------------------+
| 3   | FORMAT_NV21            |
+-----+------------------------+
| 4   | FORMAT_RGB_PLANAR      |
+-----+------------------------+
| 5   | FORMAT_BGR_PLANAR      |
+-----+------------------------+
| 6   | FORMAT_RGBP_SEPARATE   |
+-----+------------------------+
| 7   | FORMAT_BGRP_SEPARATE   |
+-----+------------------------+
| 8   | FORMAT_GRAY            |
+-----+------------------------+

| 【注意】

1. 输入输出所有 bm_image 结构必须提前创建，否则返回失败。

#. 输入输出图像的data_type、image_format必须相同。

#. 支持图像的分辨率为64x64~4608x4608，且要求64位对齐。

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

    #define LDC_ALIGN 64

    extern void bm_read_bin(bm_image src, const char *input_name);
    extern void bm_write_bin(bm_image dst, const char *output_name);

    int main(int argc, char **argv) {
        bm_status_t ret = BM_SUCCESS;
        bm_handle_t handle = NULL;
        int dev_id = 0;
        char *src_name = "1920x1088_nv21.bin";
        char *dst_name = "out_1920x1088_rot90.yuv";
        int width = 1920;
        int height = 1088;
        int rot_angle = 90;
        bm_image src, dst;
        bm_image_format_ext src_fmt = FORMAT_NV21;
        bm_image_format_ext dst_fmt = FORMAT_NV21;
        bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
        bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
        if (ret != BM_SUCCESS) {
            printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
            exit(-1);
        }
        ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);
        if (ret != BM_SUCCESS) {
            printf("bm_image_alloc_dev_mem_dst. ret = %d\n", ret);
            exit(-1);
        }
        bm_read_bin(src, src_name);
        bmcv_image_rotate(handle, src, dst, rot_angle);
        bm_write_bin(dst, dst_name);

        return 0;
    }
