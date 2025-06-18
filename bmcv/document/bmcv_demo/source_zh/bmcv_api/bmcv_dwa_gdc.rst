bmcv_dwa_gdc
---------------

| 【描述】
| 去畸变仿射(DWA)模块的几何畸变校正功能，通过校正镜头引起的图像畸变（针对桶形畸变 (Barrel Distortion) 及枕形畸变 (Pincushion Distortion) ），使图像中的直线变得更加准确和几何正确，提高图像的质量和可视化效果。
| 其中，提供两种畸变校正的方式供用户选择，分别为：1. 用户根据图像畸变的类型及校正强度输入配置参数列表对图像进行校正；2. 用户使用 Grid_Info(输入输出图像坐标映射关系描述)文件校正图像，以获得更好的图像校正效果。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_dwa_gdc(bm_handle_t          handle,
                             bm_image             input_image,
                             bm_image             output_image,
                             bmcv_gdc_attr        ldc_attr);

| 【参数】

.. list-table:: bmcv_dwa_gdc 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取
    * - input_image
      - 输入
      - 输入的畸变图像，通过调用 bm_image_create 创建
    * - output_image
      - 输出
      - 输出的畸变校正后的图像，通过调用 bm_image_create 创建
    * - ldc_attr
      - 输入
      - GDC功能的参数配置列表，包括bAspect s32XRatio s32YRatio s32XYRatio s32CenterXOffset s32CenterYOffset s32DistortionRatio \*grid_info

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【数据类型说明】

.. list-table:: bmcv_gdc_attr 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - bAspect
      - 输入
      - 畸变校正的过程中是否保持幅型比，0：不保持，1：保持
    * - s32XRatio
      - 输入
      - 配置范围:[0, 100]，水平方向视野大小参数，bAspect=0 时有效
    * - s32YRatio
      - 输入
      - 配置范围:[0, 100]，垂直方向视野大小参数，bAspect=0 时有效
    * - s32XYRatio
      - 输入
      - 配置范围:[0, 100]，视野大小参数，bAspect=1 时有效
    * - s32CenterXOffset
      - 输入
      - 配置范围:[-511, 511]，畸变中心点相对于图像中心点的水平偏移
    * - s32CenterYOffset
      - 输入
      - 配置范围:[-511, 511]，畸变中心点相对于图像中心点的垂直偏移
    * - s32DistortionRatio
      - 输入
      - 配置范围:[-300, 500]，畸变校正强度参数，畸变类型为桶形时配置为负，畸变类型为枕形时配置为负
    * - grid_info
      - 输入
      - 用于存储 grid_info 的信息，包含 grid_info 的大小和数据

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

2. 支持图像的分辨率为32x32~4096x4096，且要求宽 64 stride对齐。

3. 若用户决定使用第一种方式进行图像校正，需根据图像畸变的类型及校正强度自行输入配置参数列表 ldc_attr，此时要将 grid_info 设置为空。

4. 若用户决定使用第二种方式进行图像校正，需提供 Grid_Info 文件，具体使用方式请参考下面的代码示例。

| 【代码示例】

| 1. 通过配置参数列表进行图像校正

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "bmcv_api_ext_c.h"
    #include <unistd.h>

    int main() {
        int src_h = 1080, src_w = 1920, dev_id = 0;
        bm_image_format_ext fmt = FORMAT_YUV420P;
        char *src_name = "path/to/src", *dst_name = "path/to/dst";
        bm_handle_t handle = NULL;
        bmcv_gdc_attr ldc_attr = {true, 0, 0, 0, 0, 0, -200, };
        fmt = FORMAT_RGB_PLANAR;
        ldc_attr.grid_info.size = 0;
        ldc_attr.grid_info.u.system.system_addr = NULL;
        int ret = (int)bm_dev_request(&handle, dev_id);
        if (ret != 0) {
            printf("Create bm handle failed. ret = %d\n", ret);
            exit(-1);
        }

        bm_image src, dst;
        int dst_w, dst_h;

        bm_image_create(handle, src_h, src_w, fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);

        dst_w = src_w;
        dst_h = src_h;
        bm_image_create(handle, dst_h, dst_w, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
        ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

        int image_byte_size[4] = {0};
        bm_image_get_byte_size(src, image_byte_size);
        for (int i = 0; i < 4; i++) {
            printf("image_byte_size[%d] is : %d\n", i, image_byte_size[i]);
        }
        int byte_size  = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
        // int byte_size = src_w * src_h * 3 / 2;
        unsigned char *input_data = (unsigned char *)malloc(byte_size);
        FILE *fp_src = fopen(src_name, "rb");
        if (fread((void *)input_data, 1, byte_size, fp_src) < (unsigned int)byte_size) {
        printf("file size is less than required bytes%d\n", byte_size);
        };
        fclose(fp_src);
        bm_image_copy_host_to_device(src, (void *)&input_data);

        bmcv_dwa_gdc(handle, src, dst, ldc_attr);

        unsigned char* output_ptr = (unsigned char*)malloc(byte_size);
        void* out_ptr[4] = {(void*)output_ptr,
                            (void*)((unsigned char*)output_ptr + dst_w * dst_h),
                            (void*)((unsigned char*)output_ptr + 5 / 4 * dst_w * dst_w)};
        bm_image_copy_device_to_host(dst, (void **)out_ptr);

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

| 2. 通过 Grid_Info 文件进行图像校正

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "bmcv_api_ext_c.h"
    #include <unistd.h>

    int main() {
        int src_h = 1080, src_w = 1920, dst_h = 1080, dst_w = 1920, dev_id = 0;
        bm_image_format_ext fmt = FORMAT_YUV420P;
        char *src_name = "path/to/src", *dst_name = "path/to/dst", *grid_name = "path/to/grid_info";
        bm_handle_t handle = NULL;
        bmcv_gdc_attr ldc_attr = {true, 0, 0, 0, 0, 0, -200, };
        fmt = FORMAT_RGB_PLANAR;
        ldc_attr.grid_info.size = 0;
        ldc_attr.grid_info.u.system.system_addr = NULL;
        int ret = (int)bm_dev_request(&handle, dev_id);
        if (ret != 0) {
            printf("Create bm handle failed. ret = %d\n", ret);
            exit(-1);
        }

        FILE *fp = fopen(grid_name, "rb");
        if (!fp) {
            printf("open file:%s failed.\n", grid_name);
            exit(-1);
        }
        u32 grid_size = 32768;    // grid_info文件的字节数
        char *grid_data = (char *)malloc(grid_size);
        fread(grid_data, 1, grid_size, fp);

        fclose(fp);

        bm_image src, dst;
        bm_image_create(handle, src_h, src_w, fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
        bm_image_create(handle, dst_h, dst_w, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
        ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

        int image_byte_size[4] = {0};
        bm_image_get_byte_size(src, image_byte_size);
        for (int i = 0; i < 4; i++) {
            printf("image_byte_size[%d] is : %d\n", i, image_byte_size[i]);
        }
        int byte_size  = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
        // int byte_size = src_w * src_h * 3 / 2;
        unsigned char *input_data = (unsigned char *)malloc(byte_size);
        FILE *fp_src = fopen(src_name, "rb");
        if (fread((void *)input_data, 1, byte_size, fp_src) < (unsigned int)byte_size) {
        printf("file size is less than required bytes%d\n", byte_size);
        };
        fclose(fp_src);
        bm_image_copy_host_to_device(src, (void *)&input_data);

        ldc_attr.grid_info.u.system.system_addr = (void *)grid_data;
        ldc_attr.grid_info.size = grid_size;

        bmcv_dwa_gdc(handle, src, dst, ldc_attr);

        unsigned char* output_ptr = (unsigned char*)malloc(byte_size);
        void* out_ptr[4] = {(void*)output_ptr,
                            (void*)((unsigned char*)output_ptr + dst_w * dst_h),
                            (void*)((unsigned char*)output_ptr + 5 / 4 * dst_w * dst_w)};
        bm_image_copy_device_to_host(dst, (void **)out_ptr);

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