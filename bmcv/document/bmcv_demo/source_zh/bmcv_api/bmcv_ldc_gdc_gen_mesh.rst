bmcv_ldc_gen_mesh
-------------------

| 【描述】
| 镜头畸变校正(LDC)模块的几何畸变校正功能，通过校正镜头引起的图像畸变（针对桶形畸变 (Barrel Distortion) 及枕形畸变 (Pincushion Distortion) ），使图像中的直线变得更加准确和几何正确，提高图像的质量和可视化效果。
  在这个过程中，需要通过CPU计算 MESH 表用于后续的畸变校正，因此可以通过该函数计算并保存 MESH 表用于后续的畸变校正。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ldc_gdc_gen_mesh(bm_handle_t          handle,
                                    bm_image             in_image,
                                    bm_image             out_image,
                                    bmcv_gdc_attr        ldc_attr,
                                    bm_device_mem_t      dmem);

| 【参数】

.. list-table:: bmcv_ldc_gdc_gen_mesh 参数表
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
    * - ldc_attr
      - 输入
      - GDC功能的参数配置列表，包括bAspect XRatio YRatio XYRatio CenterXOffset CenterYOffset DistortionRatio
    * - dmem
      - 输入
      - 用于存储 MESH 表的设备内存，通过调用 bm_malloc_device_byte 创建

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
|  1  | FORMAT_NV12                   |
+-----+-------------------------------+
|  2  | FORMAT_NV21                   |
+-----+-------------------------------+
|  3  | FORMAT_GRAY                   |
+-----+-------------------------------+

| 【注意】

1. 输入输出所有 bm_image 结构必须提前创建，否则返回失败。

2. 支持图像的分辨率为64x64~4608x4608，且要求64位对齐。

3. 用户需根据图像畸变的类型及校正强度自行输入配置GDC的参数配置列表ldc_attr，此时要将 grid_info 设置为空。

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
    #include <inttypes.h>

    #define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))
    #define HW_MESH_SIZE 8

    #define TILESIZE 64 // HW: data Tile Size
    #define MESH_NUM_ATILE (TILESIZE / HW_MESH_SIZE) // how many mesh in A TILE
    #define LDC_ALIGN 64
    typedef unsigned int         u32;
    typedef unsigned char        u8;

    typedef struct COORD2D_INT_HW {
        u8 xcor[3]; // s13.10, 24bit
    } __attribute__((packed)) COORD2D_INT_HW;

    void test_mesh_gen_get_1st_size(u32 u32Width, u32 u32Height, u32 *mesh_1st_size)
    {
        if (!mesh_1st_size)
            return;

        u32 ori_src_width = u32Width;
        u32 ori_src_height = u32Height;

        // In LDC Processing, width & height aligned to TILESIZE
        u32 src_width_s1 = ((ori_src_width + TILESIZE - 1) / TILESIZE) * TILESIZE;
        u32 src_height_s1 = ((ori_src_height + TILESIZE - 1) / TILESIZE) * TILESIZE;

        // modify frame size
        u32 dst_height_s1 = src_height_s1;
        u32 dst_width_s1 = src_width_s1;
        u32 num_tilex_s1 = dst_width_s1 / TILESIZE;
        u32 num_tiley_s1 = dst_height_s1 / TILESIZE;

        // 4 = 4 knots in a mesh
        *mesh_1st_size = sizeof(struct COORD2D_INT_HW) * MESH_NUM_ATILE * MESH_NUM_ATILE * num_tilex_s1 * num_tiley_s1 * 4;
    }

    void test_mesh_gen_get_2nd_size(u32 u32Width, u32 u32Height, u32 *mesh_2nd_size)
    {
        if (!mesh_2nd_size)
            return;

        u32 ori_src_width = u32Width;
        u32 ori_src_height = u32Height;

        // In LDC Processing, width & height aligned to TILESIZE
        u32 src_width_s1 = ((ori_src_width + TILESIZE - 1) / TILESIZE) * TILESIZE;
        u32 src_height_s1 = ((ori_src_height + TILESIZE - 1) / TILESIZE) * TILESIZE;

        // modify frame size
        u32 dst_height_s1 = src_height_s1;
        u32 dst_width_s1 = src_width_s1;
        u32 src_height_s2 = dst_width_s1;
        u32 src_width_s2 = dst_height_s1;
        u32 dst_height_s2 = src_height_s2;
        u32 dst_width_s2 = src_width_s2;
        u32 num_tilex_s2 = dst_width_s2 / TILESIZE;
        u32 num_tiley_s2 = dst_height_s2 / TILESIZE;

        // 4 = 4 knots in a mesh
        *mesh_2nd_size = sizeof(struct COORD2D_INT_HW) * MESH_NUM_ATILE * MESH_NUM_ATILE * num_tilex_s2 * num_tiley_s2 * 4;
    }

    void test_mesh_gen_get_size(u32 u32Width,
                                u32 u32Height,
                                u32 *mesh_1st_size,
                                u32 *mesh_2nd_size)
    {
        if (!mesh_1st_size || !mesh_2nd_size)
            return;

        test_mesh_gen_get_1st_size(u32Width, u32Height, mesh_1st_size);
        test_mesh_gen_get_2nd_size(u32Width, u32Height, mesh_2nd_size);
    }


    int main() {
        int dev_id = 0;
        int height = 1080, width = 1920;
        bm_image_format_ext src_fmt = FORMAT_GRAY, dst_fmt = FORMAT_GRAY;
        bmcv_gdc_attr stLDCAttr = {0};
        char *src_name = "path/to/src";
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
        int align_height = (height + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);
        int align_width  = (width  + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);

        // calc image stride
        int data_size = 1;
        src_stride[0] = align_up(width, 64) * data_size;
        dst_stride[0] = align_up(width, 64) * data_size;
        // create bm image
        bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
        bm_image_create(handle, align_height, align_width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);

        ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
        ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

        int byte_size = width * height;
        unsigned char *input_data = (unsigned char *)malloc(byte_size);
        FILE *fp_src = fopen(src_name, "rb");
        if (fread((void *)input_data, 1, byte_size, fp_src) < (unsigned int)byte_size) {
          printf("file size is less than required bytes%d\n", byte_size);
        };
        fclose(fp_src);
        void *in_ptr[1] = {(void *)input_data};
        bm_image_copy_host_to_device(src, in_ptr);

        bm_device_mem_t dmem;
        u32 mesh_1st_size = 1, mesh_2nd_size = 1;
        test_mesh_gen_get_size(width, height, &mesh_1st_size, &mesh_2nd_size);
        u32 mesh_size = mesh_1st_size + mesh_2nd_size;
        ret = bm_malloc_device_byte(handle, &dmem, mesh_size);

        ret = bmcv_ldc_gdc_gen_mesh(handle, src, dst, stLDCAttr, dmem);
        unsigned char *buffer = (unsigned char *)malloc(mesh_size);
        memset(buffer, 0, mesh_size);
        ret = bm_memcpy_d2s(handle, (void*)buffer, dmem);

        char mesh_name[128];
        snprintf(mesh_name, 128, "./test_mesh_%dx%d_%d_%d_%d_%d_%d_%d_%d.mesh"
            , width, height, stLDCAttr.bAspect, stLDCAttr.s32XRatio, stLDCAttr.s32YRatio
            , stLDCAttr.s32XYRatio, stLDCAttr.s32CenterXOffset, stLDCAttr.s32CenterYOffset, stLDCAttr.s32DistortionRatio);

        FILE *fp = fopen(mesh_name, "wb");
        if (fwrite((void *)(unsigned long int)buffer, mesh_size, 1, fp) != 1) {
            printf("fwrite mesh data error\n");
            free(buffer);
        }
        fclose(fp);
        free(buffer);

        bm_image_destroy(&src);
        bm_image_destroy(&dst);

        bm_dev_free(handle);

        return 0;
    }
