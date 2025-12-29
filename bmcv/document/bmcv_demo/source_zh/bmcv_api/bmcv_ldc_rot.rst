bmcv_ldc_rot
---------------

| 【描述】
| 镜头畸变校正(LDC)模块的旋转功能，通过围绕固定点旋转图像，改变其方向与角度，从而使其在平面上发生旋转。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ldc_rot(bm_handle_t          handle,
                             bm_image             in_image,
                             bm_image             out_image,
                             bmcv_rot_mode        rot_mode);

| 【参数】

.. list-table:: bmcv_ldc_rot 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取
    * - in_image
      - 输入
      - 输入的待旋转图像，通过调用 bm_image_create 创建
    * - out_image
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
|  1  | FORMAT_NV12                   |
+-----+-------------------------------+
|  2  | FORMAT_NV21                   |
+-----+-------------------------------+
|  3  | FORMAT_GRAY                   |
+-----+-------------------------------+

| 【注意】

1. 输入输出所有 bm_image 结构必须提前创建，否则返回失败。

2. 支持图像的分辨率为64x64~4608x4608，且要求64位对齐。

3. 当前ldc模块固件原生支持90°与270°旋转，所以rot_mode选择0，1，3。

| 【代码示例】

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "bmcv_api_ext_c.h"
    #include <unistd.h>

    #define LDC_ALIGN 64
    #define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

    int main() {
        int dev_id = 0;
        int height = 1080, width = 1920;
        bm_image_format_ext src_fmt = FORMAT_GRAY, dst_fmt = FORMAT_GRAY;
        bmcv_rot_mode rot_mode = BMCV_ROTATION_0;
        char *src_name = "path/to/src", *dst_name = "path/to/dst";
        bm_handle_t handle = NULL;
        int ret = (int)bm_dev_request(&handle, dev_id);
        if (ret != 0) {
            printf("Create bm handle failed. ret = %d\n", ret);
            exit(-1);
        }

        bm_image src, dst;
        int src_stride[4];
        int dst_stride[4];

        // align
        int align_width  = (width  + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);

        int data_size = 1;
        src_stride[0] = align_up(width, 16) * data_size;
        dst_stride[0] = align_up(align_width, 16) * data_size;
        // create bm image
        bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
        bm_image_create(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);

        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
        ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

        int image_byte_size[4] = {0};
        bm_image_get_byte_size(src, image_byte_size);
        int byte_size  = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
        unsigned char *input_data = (unsigned char *)malloc(byte_size);
        FILE *fp_src = fopen(src_name, "rb");
        if (fread((void *)input_data, 1, byte_size, fp_src) < (unsigned int)byte_size) {
            printf("file size is less than required bytes%d\n", byte_size);
        };
        fclose(fp_src);
        void* in_ptr[4] = {(void *)input_data,
                            (void *)((unsigned char*)input_data + image_byte_size[0]),
                            (void *)((unsigned char*)input_data + image_byte_size[0] + image_byte_size[1]),
                            (void *)((unsigned char*)input_data + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
        bm_image_copy_host_to_device(src, in_ptr);

        ret = bmcv_ldc_rot(handle, src, dst, rot_mode);

        bm_image_get_byte_size(dst, image_byte_size);
        byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
        unsigned char* output_ptr = (unsigned char*)malloc(byte_size);
        void* out_ptr[4] = {(void*)output_ptr,
                            (void*)((unsigned char*)output_ptr + image_byte_size[0]),
                            (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                            (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
        bm_image_copy_device_to_host(src, (void **)out_ptr);

        FILE *fp_dst = fopen(dst_name, "wb");
        if (fwrite((void *)input_data, 1, byte_size, fp_dst) < (unsigned int)byte_size){
            printf("file size is less than %d required bytes\n", byte_size);
        };
        fclose(fp_dst);

        free(input_data);
        free(output_ptr);

        bm_image_destroy(&src);
        bm_image_destroy(&dst);

        bm_dev_free(handle);
        return 0;
    }
