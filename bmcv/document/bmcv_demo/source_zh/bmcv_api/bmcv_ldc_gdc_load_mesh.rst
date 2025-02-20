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
    #include "bmcv_api_ext_c.h"
    #include <unistd.h>
    #include <errno.h>

    #define TILESIZE 64 // HW: data Tile Size
    #define LDC_ALIGN 64
    #define HW_MESH_SIZE 8
    #define MESH_NUM_ATILE (TILESIZE / HW_MESH_SIZE) // how many mesh in A TILE

    typedef unsigned int         u32;
    #define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

    typedef struct COORD2D_INT_HW {
        unsigned char xcor[3]; // s13.10, 24bit
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
        int align_height = (height + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);
        int align_width  = (width  + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);

        // calc image stride
        int data_size = 1;
        src_stride[0] = align_up(width, 16) * data_size;
        dst_stride[0] = align_up(align_width, 16) * data_size;
        // create bm image
        bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
        bm_image_create(handle, align_height, align_width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);

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
        u32 mesh_1st_size = 0, mesh_2nd_size = 0;
        test_mesh_gen_get_size(width, height, &mesh_1st_size, &mesh_2nd_size);
        u32 mesh_size = mesh_1st_size + mesh_2nd_size;
        ret = bm_malloc_device_byte_heap(handle, &dmem, BMCV_HEAP1_ID, mesh_size);

        bm_image_get_byte_size(src, image_byte_size);
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
