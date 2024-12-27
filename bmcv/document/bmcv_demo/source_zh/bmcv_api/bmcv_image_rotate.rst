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

#. 支持图像的分辨率为64x64~4608x4608。

| 【代码示例】

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    #include "bmcv_api_ext_c.h"
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>

    static int writeBin(const char* path, void* output_data, int size)
    {
        int len = 0;
        FILE* fp_dst = fopen(path, "wb+");

        if (fp_dst == NULL) {
            perror("Error opening file\n");
            return -1;
        }

        len = fwrite((void*)output_data, 1, size, fp_dst);
        if (len < size) {
            printf("file size = %d is less than required bytes = %d\n", len, size);
            return -1;
        }

        fclose(fp_dst);
        return 0;
    }

    static int readBin(const char* path, void* input_data)
    {
        int len;
        int size;
        FILE* fp_src = fopen(path, "rb");

        if (fp_src == NULL) {
            perror("Error opening file\n");
            return -1;
        }

        fseek(fp_src, 0, SEEK_END);
        size = ftell(fp_src);
        fseek(fp_src, 0, SEEK_SET);

        len = fread((void*)input_data, 1, size, fp_src);
        if (len < size) {
            printf("file size = %d is less than required bytes = %d\n", len, size);
            return -1;
        }

        fclose(fp_src);
        return 0;
    }




    int main() {
        int rot_angle = 90;
        int src_w = 1920, src_h = 1080, dst_w = 1080, dst_h = 1920, dev_id = 0;
        bm_image_format_ext src_fmt = FORMAT_RGB_PLANAR, dst_fmt = FORMAT_RGB_PLANAR;
        char *src_name = "/path/to/src";
        char *dst_name = "path/to/dst";
        bm_handle_t handle = NULL;
        bm_status_t ret;
        ret = bm_dev_request(&handle, dev_id);
        bm_image src, dst;

        bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
        bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
        ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

        unsigned char* input_data = malloc(src_w * src_h * 3);
        unsigned char* in_ptr[3] = {input_data, input_data + src_w * src_h, input_data + src_w * src_h * 2};
        readBin(src_name, input_data);
        bm_image_copy_host_to_device(src, (void **)in_ptr);
        bmcv_image_rotate(handle, src, dst, rot_angle);
        bm_image_copy_device_to_host(dst, (void **)in_ptr);

        writeBin(dst_name, input_data, src_w * src_h * 3);

        bm_image_destroy(&src);
        bm_image_destroy(&dst);

        free(input_data);
        bm_dev_free(handle);

        return ret;
    }
