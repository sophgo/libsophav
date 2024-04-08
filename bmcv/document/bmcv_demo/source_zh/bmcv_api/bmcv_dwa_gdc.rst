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

2. 支持图像的分辨率为32x32~4096x4096，且要求32位对齐。

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
        int dst_h = 1080, dst_w = 1920;
        bm_image src, dst;
        bm_image_format_ext fmt = FORMAT_YUV420P;
        int ret = (int)bm_dev_request(&handle, dev_id);
        bmcv_gdc_attr ldc_attr = {true, 0, 0, 0, 0, 0, -200, };
        //set ldc_attr for grid_info
        ldc_attr.grid_info.u.system.system_addr = NULL;
        ldc_attr.grid_info.size = 0;

        // create bm image
        bm_image_create(handle, src_h, src_w, fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
        bm_image_create(handle, dst_h, dst_w, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);
        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
        ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP_ANY);
        // read image data from input files
        bm_read_bin(src, src_name);
        bmcv_dwa_gdc(handle, src, dst, ldc_attr);
        bm_write_bin(dst, dst_name);

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
        char *dst_name = "out_gdc_grid_L.yuv";
        char *grid_name = "grid_info_79_43_3397_80_45_1280x720.dat";
        int src_h = 720, src_w = 1280;
        int dst_h = 720, dst_w = 1280;
        bm_image src, dst;
        bm_image_format_ext fmt = FORMAT_GRAY;
        int ret = (int)bm_dev_request(&handle, dev_id);
        bmcv_gdc_attr ldc_attr = {0};
        ldc_attr.grid_info.size = 328480;     // 注意：用户需根据实际的Grid_Info文件大小（字节数）进行输入设置

        // create bm image
        bm_image_create(handle, src_h, src_w, fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
        bm_image_create(handle, dst_h, dst_w, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);
        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
        ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP_ANY);

        // read image data from input files
        bm_read_bin(src, src_name);

        char *buffer = (char *)malloc(ldc_attr.grid_info.size);
        if (buffer == NULL) {
            printf("malloc buffer for grid_info failed!\n");
            goto fail;
        }
        memset(buffer, 0, ldc_attr.grid_info.size);

        FILE *fp = fopen(grid_name, "rb");
        if (!fp) {
            printf("open file:%s failed.\n", grid_name);
            goto fail;
        }
        fread(buffer, 1, ldc_attr.grid_info.size, fp);
        fclose(fp);
        ldc_attr.grid_info.u.system.system_addr = (void *)buffer;

        bmcv_dwa_gdc(handle, src, dst, ldc_attr);
        bm_write_bin(dst, dst_name);

        return 0;
    }