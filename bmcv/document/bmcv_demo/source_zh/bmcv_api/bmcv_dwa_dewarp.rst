bmcv_dwa_dewarp
-----------------

| 【描述】
| 去畸变仿射(DWA)模块的去畸变校正功能，通过Grid_Info(输入输出图像坐标映射关系描述)文件对畸变图像进行校正，使图像中的直线变得更加准确和几何正确，提高图像的质量和可视化效果。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_dwa_dewarp(bm_handle_t          handle,
                                bm_image             input_image,
                                bm_image             output_image,
                                char                 *grid_info);

| 【参数】

.. list-table:: bmcv_dwa_dewarp 参数表
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
    * - \*grid_info
      - 输入
      - 输入的 Grid_Info 的对象指针

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

2. 支持图像的分辨率为32x32~4096x4096，且要求32对齐。

3. 用户需提供 Grid_Info 文件以进行DEWARP功能，具体使用方式请参考下面的代码示例。


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

    typedef unsigned int u32;

    int main() {
        int src_h = 1080, src_w = 1920, dst_w = 1920, dst_h = 1080;
        int dev_id = 0;
        bm_image_format_ext fmt = FORMAT_GRAY;
        char *src_name = "path/to/src";
        char *dst_name = "path/to/dst";
        char *grid_name = "path/to/grid_info";
        bm_handle_t handle = NULL;
        u32 grid_size = 0;
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
        char *grid_data = (char *)malloc(grid_size);
        fread(grid_data, 1, grid_size, fp);

        fclose(fp);

        bm_image src, dst;

        dst_w = src_w;
        dst_h = src_h;
        bm_image_create(handle, src_h, src_w, fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
        bm_image_create(handle, dst_h, dst_w, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

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

        bm_device_mem_t dmem;
        dmem.u.system.system_addr = (void *)grid_data;
        dmem.size = grid_size;

        bmcv_dwa_dewarp(handle, src, dst, dmem);

        bm_image_get_byte_size(dst, image_byte_size);
        byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
        unsigned char* output_ptr = (unsigned char*)malloc(byte_size);
        void* out_ptr[4] = {(void*)output_ptr,
                            (void*)((unsigned char*)output_ptr + image_byte_size[0]),
                            (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                            (void*)((unsigned char*)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
        bm_image_copy_device_to_host(dst, (void **)out_ptr);

        FILE *fp_dst = fopen(dst_name, "wb");
        if (fwrite((void *)output_ptr, 1, byte_size, fp_dst) < (unsigned int)byte_size){
            printf("file size is less than %d required bytes\n", byte_size);
        };
        fclose(fp_dst);

        free(grid_data);
        free(input_data);
        free(output_ptr);
        bm_image_destroy(&src);
        bm_image_destroy(&dst);

        bm_dev_free(handle);

        return 0;
    }

