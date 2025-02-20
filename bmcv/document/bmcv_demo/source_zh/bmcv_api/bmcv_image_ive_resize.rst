bmcv_ive_resize
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建图像缩放任务, 支持 双线性插值、区域插值 缩放, 支持多张 GRAY 与 RGB PLANAR 图像同时输入做一种类型的缩放。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_resize(
        bm_handle_t              handle,
        bm_image                 input,
        bm_image                 output,
        bmcv_resize_algorithm     resize_mode);

| 【参数】

.. list-table:: bmcv_ive_resize 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input
      - 输入
      - 输入 bm_image 对象结构体，不能为空。
    * - \output
      - 输出
      - 输出 bm_image 对象结构体，不能为空, 每张图像类型同input。
    * - resize_mode
      - 输入
      - 输入 bmcv_resize_algorithm 对象, 枚举控制信息。

.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY

        RGB_PLANAR
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - output
      - GRAY

        RGB_PLANAR
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080

| 【数据类型说明】

【说明】定义 Resize 模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_resize_algorithm_ {
        BMCV_INTER_NEAREST = 0,
        BMCV_INTER_LINEAR  = 1,
        BMCV_INTER_BICUBIC = 2,
        BMCV_INTER_AREA = 3,
    } bmcv_resize_algorithm;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - BMCV_INTER_NEAREST
      - 最近邻插值模式。
    * - IVE_RESIZE_AREA
      - 双线性插值缩放模式。
    * - IVE_RESIZE_LINEAR
      - 双三次插值模式。
    * - IVE_RESIZE_AREA
      - 区域插值缩放模式。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 支持 GRAY、RGB_PLANAR 混合图像数组输入, 但所有图像的缩放模式相同，同时模式只支持双线性插值和区域插值缩放模式。

2. 最大支持 16 倍缩放。

3. 输入输出图像的 width 都需要16对齐。


**示例代码**

    .. code-block:: c

      #include <stdio.h>
      #include <stdlib.h>
      #include <string.h>
      #include <math.h>
      #include "bmcv_api_ext_c.h"
      #include <unistd.h>

      #define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

      int main(){
          int dev_id = 0;
          bmcv_resize_algorithm resize_mode = BMCV_INTER_AREA;
          bm_image_format_ext format = FORMAT_GRAY;
          int src_width = 1920, src_height = 1080;
          int dst_width = 400;
          int dst_height = 300;

          char *src_name = "path/to/src";
          char *dst_name = "path/to/dst";
          bm_handle_t handle = NULL;
          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }

          bm_image src, dst;
          int src_stride[4], dst_stride[4];

          int data_size= 1;
          int byte_size;
          int image_byte_size[4] = {0};
          // calc ive image stride && create bm image struct
          src_stride[0] = align_up(src_width, 16) * data_size;
          bm_image_create(handle, src_height, src_width, format, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);

          dst_stride[0] = align_up(dst_width, 16) * data_size;
          bm_image_create(handle, dst_height, dst_width, format, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);

          ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

          bm_image_get_byte_size(src, image_byte_size);
          byte_size  = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
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

          ret = bmcv_ive_resize(handle, src, dst, resize_mode);

          bm_image_get_byte_size(dst, image_byte_size);
          byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
          unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
          memset(output_ptr, 0, sizeof(byte_size));

          void* out_ptr[4] = {(void*)output_ptr,
                          (void*)((char*)output_ptr + image_byte_size[0]),
                          (void*)((char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                          (void*)((char*)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};

          ret = bm_image_copy_device_to_host(dst, (void **)out_ptr);

          FILE *ive_fp = fopen(dst_name, "wb");
          fwrite((void *)output_ptr, 1, byte_size, ive_fp);
          fclose(ive_fp);

          free(input_data);
          free(output_ptr);

          bm_image_destroy(&src);
          bm_image_destroy(&dst);

          bm_dev_free(handle);
          return 0;
      }