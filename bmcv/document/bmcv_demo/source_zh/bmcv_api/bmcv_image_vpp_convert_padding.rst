bmcv_image_vpp_convert_padding
------------------------------

| 【描述】

| 该 API 使用vpp硬件资源，通过对 dst image 做 memset 操作，实现图像padding的效果。这个效果的实现是利用了vpp的dst crop的功能，通俗的讲是将一张小图填充到大图中。 可以从一张src image上crop多个目标图像，对于每一个目标小图，可以一次性完成csc+resize操作，然后根据其在大图中的offset信息，填充到大图中。 一次crop的数量不能超过256。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_vpp_convert_padding(
        bm_handle_t           handle,
        int                   output_num,
        bm_image              input,
        bm_image *            output,
        bmcv_padding_atrr_t * padding_attr,
        bmcv_rect_t *         crop_rect = NULL,
        bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR);

| 【参数】

.. list-table:: bmcv_image_vpp_convert_padding 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - output_num
      - 输出
      - 输出 bm_image 数量，和src image的crop 数量相等，一个src crop 输出一个dst bm_image。
    * - input
      - 输入
      - 输入 bm_image 对象。
    * - \* output
      - 输出
      - 输出 bm_image 对象指针。
    * - \* padding_attr
      - 输入
      - src crop的目标小图在dst image中的位置信息以及要pdding的各通道像素值。
    * - \* crop_rect
      - 输入
      - 每个输出 bm_image 对象所对应的在输入图像上 crop 的参数。
    * - algorithm
      - 输入
      - resize 算法选择，包括 BMCV_INTER_NEAREST、BMCV_INTER_LINEAR 和 BMCV_INTER_BICUBIC三种，默认情况下是双线性差值。

| 【数据类型说明】

.. code-block:: c

    typedef struct bmcv_padding_atrr_s {
        unsigned int    dst_crop_stx;
        unsigned int    dst_crop_sty;
        unsigned int    dst_crop_w;
        unsigned int    dst_crop_h;
        unsigned char padding_r;
        unsigned char padding_g;
        unsigned char padding_b;
        int           if_memset;
    } bmcv_padding_atrr_t;


1. 目标小图的左上角顶点相对于 dst image 原点（左上角）的offset信息：dst_crop_stx 和 dst_crop_sty；
#. 目标小图经resize后的宽高：dst_crop_w 和 dst_crop_h；
#. dst image 如果是RGB格式，各通道需要padding的像素值信息：padding_r、padding_g、padding_b，当if_memset=1时有效，如果是GRAY图像可以将三个值均设置为同一个值；
#. if_memset表示要不要在该api内部对dst image 按照各个通道的padding值做memset，仅支持RGB和GRAY格式的图像。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_rect {
        int start_x;
        int start_y;
        int crop_w;
        int crop_h;
    } bmcv_rect_t;

start_x、start_y、crop_w、crop_h分别表示每个输出 bm_image 对象所对应的在输入图像上 crop 的参数，包括起始点x坐标、起始点y坐标、crop图像的宽度以及crop图像的高度。图像左上顶点作为坐标原点。如果不使用 crop 功能可填 NULL。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 该 API 的dst image的格式仅支持:

+-----+-------------------------------+
| num | dst image_format              |
+=====+===============================+
|  1  | FORMAT_RGB_PLANAR             |
+-----+-------------------------------+
|  2  | FORMAT_BGR_PLANAR             |
+-----+-------------------------------+
|  3  | FORMAT_RGBP_SEPARATE          |
+-----+-------------------------------+
|  4  | FORMAT_BGRP_SEPARATE          |
+-----+-------------------------------+
|  5  | FORMAT_RGB_PACKED             |
+-----+-------------------------------+
|  6  | FORMAT_BGR_PACKED             |
+-----+-------------------------------+

2. 该 API 所需要满足的格式以及部分要求与 bmcv_image_vpp_basic 一致。


| 【示例代码】

    .. code-block:: c

      #include <limits.h>
      #include <stdio.h>
      #include <stdlib.h>
      #include <string.h>

      #include "bmcv_api_ext_c.h"


      int main() {
          char *filename_src = "path/to/src";
          char *filename_dst = "path/to/dst";
          int in_width = 1920;
          int in_height = 1080;
          int out_width = 1920;
          int out_height = 1080;
          bm_image_format_ext src_format = 0;     // FORMAT_YUV420P
          bm_image_format_ext dst_format = 0;
          bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR;

          bmcv_rect_t crop_rect = {
              .start_x = 100,
              .start_y = 100,
              .crop_w = 500,
              .crop_h = 500
              };

          bmcv_padding_attr_t padding_rect = {
              .dst_crop_stx = 0,
              .dst_crop_sty = 0,
              .dst_crop_w = 1000,
              .dst_crop_h = 1000,
              .padding_r = 155,
              .padding_g = 20,
              .padding_b = 36,
              .if_memset = 1
              };

          bm_status_t ret = BM_SUCCESS;

          int src_size = in_height * in_width * 3 / 2;
          int dst_size = in_height * in_width * 3 / 2;
          unsigned char *src_data = (unsigned char *)malloc(src_size);
          unsigned char *dst_data = (unsigned char *)malloc(dst_size);

          FILE *file;
          file = fopen(filename_src, "rb");
          fread(src_data, sizeof(unsigned char), src_size, file);
          fclose(file);

          bm_handle_t handle = NULL;
          int dev_id = 0;
          bm_image src, dst;

          ret = bm_dev_request(&handle, dev_id);

          bm_image_create(handle, in_height, in_width, src_format, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
          bm_image_create(handle, out_height, out_width, dst_format, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);
          bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
          bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

          int src_image_byte_size[4] = {0};
          bm_image_get_byte_size(src, src_image_byte_size);
          void *src_in_ptr[4] = {(void *)src_data,
                                (void *)((char *)src_data + src_image_byte_size[0]),
                                (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1]),
                                (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1] + src_image_byte_size[2])};

          bm_image_copy_host_to_device(src, (void **)src_in_ptr);
          ret = bmcv_image_vpp_convert_padding(handle, 1, src, &dst, &padding_rect, &crop_rect, algorithm);

          int dst_image_byte_size[4] = {0};
          bm_image_get_byte_size(dst, dst_image_byte_size);
          void *dst_in_ptr[4] = {(void *)dst_data,
                                (void *)((char *)dst_data + dst_image_byte_size[0]),
                                (void *)((char *)dst_data + dst_image_byte_size[0] + dst_image_byte_size[1]),
                                (void *)((char *)dst_data + dst_image_byte_size[0] + dst_image_byte_size[1] + dst_image_byte_size[2])};

          bm_image_copy_device_to_host(dst, (void **)dst_in_ptr);

          FILE *fp_dst = fopen(filename_dst, "wb");
          if (fwrite((void *)dst_data, 1, dst_size, fp_dst) < (unsigned int)dst_size){
              printf("file size is less than %d required bytes\n", dst_size);
          };
          fclose(fp_dst);

          bm_image_destroy(&src);
          bm_image_destroy(&dst);
          bm_dev_free(handle);

          free(src_data);
          free(dst_data);

          return ret;
      }
