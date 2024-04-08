bmcv_ldc_load_mesh
-------------------

| 【描述】
| 镜头畸变校正(LDC)模块的几何畸变校正功能，通过校正镜头引起的图像畸变（针对桶形畸变 (Barrel Distortion) 及枕形畸变 (Pincushion Distortion) ），使图像中的直线变得更加准确和几何正确，提高图像的质量和可视化效果。
  在这个过程中，需要通过CPU计算 MESH 表用于后续的畸变校正，由于该过程较为耗时，因此可以通过该函数直接加载已有的 MESH 表进行图像的畸变矫正。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ldc_gdc_load_mesh(bm_handle_t          handle,
                                       bm_image             in_image,
                                       bm_image             out_image,
                                       bm_device_mem_t      dmem);

| 【参数】

.. list-table:: bmcv_ldc_gdc_load_mesh 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取
    * - in_image
      - 输入
      - 输入的畸变图像，通过调用 bm_image_create 创建
    * - out_image
      - 输出
      - 输出的畸变校正后的图像，通过调用 bm_image_create 创建
    * - dmem
      - 输入
      - 用于存储 MESH 表的设备内存，通过调用 bm_malloc_device_byte 创建

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
|  1  | FORMAT_NV12                   |
+-----+-------------------------------+
|  2  | FORMAT_NV21                   |
+-----+-------------------------------+
|  3  | FORMAT_GRAY                   |
+-----+-------------------------------+

| 【注意】

1. 输入输出所有 bm_image 结构必须提前创建，否则返回失败。

2. 支持图像的分辨率为64x64~4608x4608，且要求64位对齐。

3. 若要保证几何畸变校正的效果，用户需确认 MESH 表的正确性。

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
    typedef unsigned int         u32;

    extern void bm_read_bin(bm_image src, const char *input_name);
    extern void bm_write_bin(bm_image dst, const char *output_name);
    extern int md5_cmp(unsigned char* got, unsigned char* exp ,int size);
    extern bm_status_t bm_ldc_image_calc_stride(bm_handle_t handle,
                                                int img_h,
                                                int img_w,
                                                bm_image_format_ext image_format,
                                                bm_image_data_format_ext data_type,
                                                int *stride);
    extern void test_mesh_gen_get_size(u32 u32Width,
                                       u32 u32Height,
                                       u32 *mesh_1st_size,
                                       u32 *mesh_2nd_size);

    int main(int argc, char **argv) {
        bm_status_t ret = BM_SUCCESS;
        bm_handle_t handle = NULL;
        int dev_id = 0;
        char *src_name = "1920x1080_barrel_0.3.yuv";
        char *dst_name = "out_barrel_0.yuv";
        char *mesh_name = "1920x1080_barrel_0.3_r0_ofst_0_0_d-200.mesh";
        int width = 1920;
        int height = 1080;
        bm_image src, dst;
        bm_image_format_ext src_fmt = FORMAT_NV21;
        bm_image_format_ext dst_fmt = FORMAT_NV21;
        int src_stride[4];
        int dst_stride[4];
        int ret = (int)bm_dev_request(&handle, dev_id);

        // align
        int align_height = (height + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);
        int align_width  = (width  + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);

        // calc image stride
        bm_ldc_image_calc_stride(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, src_stride);
        bm_ldc_image_calc_stride(handle, align_height, align_width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, dst_stride);

        // create bm image
        bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
        bm_image_create(handle, align_height, align_width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);

        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
        ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP_ANY);

        bm_device_mem_t dmem;
        // unsigned int mesh_size = 783360;
        u32 mesh_1st_size = 0, mesh_2nd_size = 0;
        test_mesh_gen_get_size(width, height, &mesh_1st_size, &mesh_2nd_size);
        u32 mesh_size = mesh_1st_size + mesh_2nd_size;
        ret = bm_malloc_device_byte(handle, &dmem, mesh_size);
        if (ret != BM_SUCCESS) {
            printf("bm_malloc_device_byte failed: %s\n", strerror(errno));
        }

        unsigned char *buffer = (unsigned char *)malloc(mesh_size);
        if (buffer == NULL) {
            printf("malloc buffer for mesh failed!\n");
        }
        memset(buffer, 0, mesh_size);

        FILE *fp = fopen(mesh_name, "rb");
        if (!fp) {
            printf("open file:%s failed.\n", mesh_name);
        }

        fseek(fp, 0, SEEK_END);
        int fileSize = ftell(fp);

        if (mesh_size != (unsigned int)fileSize) {
            printf("loadmesh file:(%s) size is not match.\n", mesh_name);
            fclose(fp);
        }
        rewind(fp);

        fread(buffer, mesh_size, 1, fp);
        ret = bm_memcpy_s2d(handle, dmem, (void*)buffer);
        fclose(fp);
        free(buffer);

        // read image data from input files
        bm_read_bin(src, src_name);
        bmcv_ldc_gdc_load_mesh(handle, src, dst, dmem);
        bm_write_bin(dst, dst_name);

        return 0;
    }