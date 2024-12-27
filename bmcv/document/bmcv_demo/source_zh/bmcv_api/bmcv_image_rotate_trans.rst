bmcv_image_rotate_trans
-------------------------

| 【描述】
| 实现图像顺时针旋转90度，180度和270度

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_rotate_trans(
        bm_handle_t handle,
        bm_image input,
        bm_image output,
        int rotation_angle) ;


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
      - 输入的待旋转图像，需要外部调用 bmcv_image_create 创建。image 内存可以使用 bm_image_alloc_dev_mem 或者 bm_image_copy_host_to_device 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。
    * - output
      - 输出
      - 输出的旋转后图像，需要外部调用 bmcv_image_create 创建。image 内存可以通过 bm_image_alloc_dev_mem 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。如果不主动分配将在 api 内部进行自行分配
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

1. 在调用 bmcv_image_rotate_trans()之前必须确保输入的 image 内存已经申请。

#. 输入输出图像的data_type、image_format必须相同。

#. 输入输出图像的宽高尺寸支持8*8-8192*8192。

#. 输入输出图像为nv12和nv21图像格式时，因处理过程中会经过多次色彩变换，输出图像像素值将存在误差，但肉眼观察差异不大。

| 【代码示例】

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

        #include <stdio.h>
        #include "bmcv_api_ext_c.h"
        #include "stdlib.h"
        #include "string.h"
        #include <assert.h>
        #include <float.h>

        #define BM1688 0x1686a200
        #define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

        static void read_bin(const char *input_path, unsigned char *input_data, int width, int height, float channel) {
            FILE *fp_src = fopen(input_path, "rb");
            if (fp_src == NULL)
            {
                printf("Can not open file! %s\n", input_path);
                return;
            }
            if(fread(input_data, sizeof(unsigned char), width * height * channel, fp_src) != 0)
                printf("read image success\n");
            fclose(fp_src);
        }

        static void write_bin(const char *output_path, unsigned char *output_data, int width, int height, int channel) {
            FILE *fp_dst = fopen(output_path, "wb");
            if (fp_dst == NULL)
            {
                printf("Can not open file! %s\n", output_path);
                return;
            }
            fwrite(output_data, sizeof(unsigned char), width * height * channel, fp_dst);
            fclose(fp_dst);
        }

        int main() {
            int width = 1920;
            int height = 1080;
            int format = 14;         // FORMAT_GRAY
            int rotation_angle = 90;        // chosen from {90, 180, 270}
            char *input_path = "path/to/input";
            char *output_path = "path/to/output";
            bm_handle_t handle;
            bm_status_t ret = bm_dev_request(&handle, 0);
            if (ret != BM_SUCCESS) {
                printf("Create bm handle failed. ret = %d\n", ret);
                return -1;
            }

            unsigned char* input_data = (unsigned char*)malloc(width * height * 3 * sizeof(unsigned char));
            unsigned char* output_tpu = (unsigned char*)malloc(width * height * 3 * sizeof(unsigned char));
            read_bin(input_path, input_data, width, height, 3);

            bm_image input_img, output_img;
            bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input_img, NULL);
            bm_image_create(handle, width, height, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &output_img, NULL);

            bm_image_alloc_dev_mem(input_img, 2);
            bm_image_alloc_dev_mem(output_img, 2);

            unsigned char *input_addr[3] = {input_data, input_data + height * width, input_data + 2 * height * width};
            bm_image_copy_host_to_device(input_img, (void **)(input_addr));

            ret = bmcv_image_rotate_trans(handle, input_img, output_img, rotation_angle);
            if (ret != BM_SUCCESS) {
                printf("bmcv_image_rotate error!");
                bm_image_destroy(&input_img);
                bm_image_destroy(&output_img);
                bm_dev_free(handle);
                return -1;
            }

            unsigned char *output_addr[3] = {output_tpu, output_tpu + height * width, output_tpu + 2 * height * width};
            bm_image_copy_device_to_host(output_img, (void **)output_addr);

            bm_image_destroy(&input_img);
            bm_image_destroy(&output_img);

            write_bin(output_path, output_tpu, width, height, 1);

            free(input_data);
            free(output_tpu);
            bm_dev_free(handle);
            return ret;
        }
