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

2. 支持图像的分辨率为32x32~4096x4096，且要求32位对齐。

3. 用户需提供 Grid_Info 文件以进行DEWARP功能，具体使用方式请参考下面的代码示例。


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

    typedef unsigned int u32;

    extern void bm_read_bin(bm_image src, const char *input_name);
    extern void bm_write_bin(bm_image dst, const char *output_name);
    extern int md5_cmp(unsigned char* got, unsigned char* exp ,int size);

    int main(int argc, char **argv) {
        bm_status_t ret = BM_SUCCESS;
        bm_handle_t handle = NULL;
        int dev_id = 0;
        char *src_name = "imgL_1280X720.yonly.yuv";
        char *dst_name = "out_dewarp_grid_L.yuv";
        char *grid_name = "grid_info_79_43_3397_80_45_1280x720.dat";
        int src_h = 720, src_w = 1280;
        int dst_h = 720, dst_w = 1280;
        bm_image src, dst;
        bm_image_format_ext fmt = FORMAT_YUV420P;
        int ret = (int)bm_dev_request(&handle, dev_id);
        bm_device_mem_t dmem;
        dmem.size = 328480;     // 注意：用户需根据实际的Grid_Info文件大小（字节数）进行输入设置
        // create bm image
        bm_image_create(handle, src_h, src_w, fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
        bm_image_create(handle, dst_h, dst_w, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);
        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
        ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP_ANY);
        // read image data from input files
        bm_read_bin(src, src_name);
        // read grid_info data
        char *buffer = (char *)malloc(dmem.size);
        if (buffer == NULL) {
            printf("malloc buffer for grid_info failed!\n");
            goto fail;
        }
        memset(buffer, 0, dmem.size);

        FILE *fp = fopen(grid_name, "rb");
        if (!fp) {
            printf("open file:%s failed.\n", grid_name);
            goto fail;
        }
        fread(buffer, 1, dmem.size, fp);
        fclose(fp);
        dmem.u.system.system_addr = (void *)buffer;

        bmcv_dwa_dewarp(handle, src, dst, dmem);
        bm_write_bin(dst, dst_name);
        free(buffer);

        return 0;
    }
