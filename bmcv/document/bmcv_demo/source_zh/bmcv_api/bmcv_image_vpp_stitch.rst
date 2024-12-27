bmcv_image_vpp_stitch
------------------------------

| 【描述】

| 该 API 使用vpp硬件资源的 crop 功能，实现图像拼接的效果，对输入 image 可以一次完成 src crop + csc + resize + dst crop操作。dst image 中拼接的小图像数量不能超过256。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_vpp_stitch(
        bm_handle_t           handle,
        int                   input_num,
        bm_image*             input,
        bm_image              output,
        bmcv_rect_t*          dst_crop_rect,
        bmcv_rect_t*          src_crop_rect = NULL,
        bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR);

| 【参数】

.. list-table:: bmcv_image_vpp_stitch 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - input_num
      - 输入
      - 输入 bm_image 数量。
    * - \*input
      - 输入
      - 输入 bm_image 对象指针。
    * - output
      - 输出
      - 输出 bm_image 对象。
    * - \*dst_crop_rect
      - 输入
      - 在dst images上，各个目标小图的坐标和宽高信息。
    * - \*src_crop_rect
      - 输入
      - 在src image上，各个目标小图的坐标和宽高信息。
    * - algorithm
      - 输入
      - resize 算法选择，包括 BMCV_INTER_NEAREST、BMCV_INTER_LINEAR 和 BMCV_INTER_BICUBIC三种，默认情况下是双线性差值。

| 【数据类型说明】

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

1. 该 API 的src image不支持压缩格式的数据。

2. 该 API 所需要满足的格式以及部分要求与 bmcv_image_vpp_basic 一致。

3. 如果对src image做crop操作，一张src image只crop一个目标。

| 【代码示例】

.. code-block:: cpp

  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include "bmcv_api_ext_c.h"
  #include <unistd.h>

  int src_h = 1080, src_w = 1920, dst_w = 1920, dst_h = 2160, dev_id = 0;
  bm_image_format_ext src_fmt = FORMAT_YUV420P, dst_fmt = FORMAT_YUV420P;
  char *src_name = "path/to/src", *dst_name = "/path/to/dst";
  bmcv_rect_t dst_rect0 = {.start_x = 0, .start_y = 0, .crop_w = 1920, .crop_h = 1080};
  bmcv_rect_t dst_rect1 = {.start_x = 0, .start_y = 1080, .crop_w = 1920, .crop_h = 1080};
  bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR;
  bm_handle_t handle = NULL;

  int main() {
      bm_status_t ret;
      bm_image src, dst;
      ret = bm_dev_request(&handle, dev_id);
      bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
      bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

      ret = bm_image_alloc_dev_mem(src,BMCV_HEAP1_ID);
      ret = bm_image_alloc_dev_mem(dst,BMCV_HEAP1_ID);

      int src_size = src_h * src_w * 3 / 2;
      int dst_size = src_h * src_w * 3 / 2;
      unsigned char *src_data = (unsigned char *)malloc(src_size);
      unsigned char *dst_data = (unsigned char *)malloc(dst_size);

      FILE *file;
      file = fopen(src_name, "rb");
      fread(src_data, sizeof(unsigned char), src_size, file);
      fclose(file);

      int src_image_byte_size[4] = {0};
      bm_image_get_byte_size(src, src_image_byte_size);
      void *src_in_ptr[4] = {(void *)src_data,
                            (void *)((char *)src_data + src_image_byte_size[0]),
                            (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1]),
                            (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1] + src_image_byte_size[2])};
      bm_image_copy_host_to_device(src, (void **)src_in_ptr);

      bmcv_rect_t rect = {.start_x = 0, .start_y = 0, .crop_w = src_w, .crop_h = src_h};
      bmcv_rect_t src_rect[2] = {rect, rect};
      bmcv_rect_t dst_rect[2] = {dst_rect0, dst_rect1};

      bm_image input[2] = {src, src};

      bmcv_image_vpp_stitch(handle, 2, input, dst, dst_rect, src_rect, BMCV_INTER_LINEAR);

      int dst_image_byte_size[4] = {0};
      bm_image_get_byte_size(dst, dst_image_byte_size);
      void *dst_in_ptr[4] = {(void *)dst_data,
                            (void *)((char *)dst_data + dst_image_byte_size[0]),
                            (void *)((char *)dst_data + dst_image_byte_size[0] + dst_image_byte_size[1]),
                            (void *)((char *)dst_data + dst_image_byte_size[0] + dst_image_byte_size[1] + dst_image_byte_size[2])};
      bm_image_copy_device_to_host(dst, (void **)dst_in_ptr);

      FILE *fp_dst = fopen(dst_name, "wb");
      if (fwrite((void *)dst_data, 1, dst_size, fp_dst) < (unsigned int)dst_size){
          printf("file size is less than %d required bytes\n", dst_size);
      };
      fclose(fp_dst);


      bm_image_destroy(&src);
      bm_image_destroy(&dst);
      bm_dev_free(handle);

      return ret;
  }

