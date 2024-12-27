bmcv_image_watermark_superpose
------------------------------

| 【描述】

| 以下接口用于在图像上叠加一个或多个水印。

| 【描述接口一】

| 接口一可实现在不同的输入图的指定位置，叠加不同的水印。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_watermark_superpose(
                      bm_handle_t handle,
                      bm_image * image,
                      bm_device_mem_t * bitmap_mem,
                      int bitmap_num,
                      int bitmap_type,
                      int pitch,
                      bmcv_rect_t * rects,
                      bmcv_color_t color);

| 【描述接口二】

| 此接口为接口一的简化版本，可在一张图中的不同位置重复叠加一种水印。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_watermark_repeat_superpose(
                      bm_handle_t handle,
                      bm_image image,
                      bm_device_mem_t bitmap_mem,
                      int bitmap_num,
                      int bitmap_type,
                      int pitch,
                      bmcv_rect_t * rects,
                      bmcv_color_t color);

**传入参数说明:**

  * bm_handle_t handle

    输入参数。设备环境句柄，通过调用 bm_dev_request 获取。

  * bm_image\* image

    输入参数。需要打水印的 bm_image 对象指针。

  * bm_device_mem_t\* bitmap_mem

    输入参数。水印的 bm_device_mem_t 对象指针。

  * int bitmap_num

    输入参数。水印数量，指 rects 指针中所包含的 bmcv_rect_t 对象个数、 也是 image 指针中所包含的 bm_image 对象个数、 也是 bitmap_mem 指针中所包含的 bm_device_mem_t 对象个数。

  * int bitmap_type

    输入参数。水印类型, 值0表示水印为8bit数据类型(有透明度信息), 值1表示水印为1bit数据类型(无透明度信息)。

  * int pitch

    输入参数。水印文件每行的byte数, 可理解为水印的宽。

  * bmcv_rect_t\* rects

    输入参数。水印位置指针，包含每个水印起始点和宽高。具体内容参考下面的数据类型说明。

  * bmcv_color_t color

    输入参数。水印的颜色。具体内容参考下面的数据类型说明。


**返回值说明:**

  * BM_SUCCESS: 成功

  * 其他:失败


**数据类型说明：**


    .. code-block:: c

        typedef struct bmcv_rect {
            int start_x;
            int start_y;
            int crop_w;
            int crop_h;
        } bmcv_rect_t;

        typedef struct {
            unsigned char r;
            unsigned char g;
            unsigned char b;
        } bmcv_color_t;


* start_x 描述了水印在原图中所在的起始横坐标。自左而右从 0 开始，取值范围 [0, width)。

* start_y 描述了水印在原图中所在的起始纵坐标。自上而下从 0 开始，取值范围 [0, height)。

* crop_w 描述的水印的宽度。

* crop_h 描述的水印的高度。

* r 颜色的r分量。

* g 颜色的g分量。

* b 颜色的b分量。


**注意事项:**

1. 该API要求如下：

- 输入和输出的数据类型必须为：

+-----+-------------------------------+
| num | data_type                     |
+=====+===============================+
|  1  | DATA_TYPE_EXT_1N_BYTE         |
+-----+-------------------------------+

- 输入的色彩格式可支持：

+-----+-------------------------------+
| num | image_format                  |
+=====+===============================+
|  1  | FORMAT_YUV420P                |
+-----+-------------------------------+
|  2  | FORMAT_YUV444P                |
+-----+-------------------------------+
|  3  | FORMAT_NV12                   |
+-----+-------------------------------+
|  4  | FORMAT_NV21                   |
+-----+-------------------------------+
|  5  | FORMAT_RGB_PLANAR             |
+-----+-------------------------------+
|  6  | FORMAT_BGR_PLANAR             |
+-----+-------------------------------+
|  7  | FORMAT_RGB_PACKED             |
+-----+-------------------------------+
|  8  | FORMAT_BGR_PACKED             |
+-----+-------------------------------+
|  9  | FORMAT_RGBP_SEPARATE          |
+-----+-------------------------------+
|  10 | FORMAT_BGRP_SEPARATE          |
+-----+-------------------------------+
|  11 | FORMAT_GRAY                   |
+-----+-------------------------------+

如果不满足输入输出格式要求，则返回失败。

2. 输入输出所有 bm_image 结构必须提前创建，否则返回失败。

3. 水印数量最多可设置512个。

4. 如果水印区域超出原图宽高，会返回失败。

5. 不支持对单底图进行位置重叠的多图叠加。

**示例代码：**

    .. code-block:: c

      #include <limits.h>
      #include <stdio.h>
      #include <stdlib.h>
      #include <string.h>

      #include "bmcv_api_ext_c.h"

      static int writeBin(const char* path, void* output_data, int size)
      {
          int len = 0;
          FILE* fp_dst = fopen(path, "wb+");

          if (fp_dst == NULL) {
              perror("Error opening file\n");
              return -1;
          }

          len = fwrite((void*)output_data, 1, size, fp_dst);
          if (len < size) {
              printf("file size = %d is less than required bytes = %d\n", len, size);
              return -1;
          }

          fclose(fp_dst);
          return 0;
      }
      bm_status_t open_water(
          bm_handle_t           handle,
          char *                src_name,
          int                   src_size,
          bm_device_mem_t *     dst)
      {
          bm_status_t ret = BM_SUCCESS;
          unsigned char * src = (unsigned char *)malloc(sizeof(unsigned char) * src_size);
          ret = bm_malloc_device_byte(handle, dst, src_size);
          if(ret != BM_SUCCESS){
              printf("bm_malloc_device_byte fail %s: %s: %d\n", __FILE__, __func__, __LINE__);
              goto fail;
          }

          FILE * fp_src = fopen(src_name, "rb");
          size_t read_size = fread((void *)src, src_size, 1, fp_src);
          printf("fread %ld byte\n", read_size);
          fclose(fp_src);
      #ifdef _FPGA
          ret = bm_memcpy_s2d_fpga(handle, dst[0], (void*)src);
      #else
          ret = bm_memcpy_s2d(handle, dst[0], (void*)src);
      #endif
          if(ret != BM_SUCCESS){
              printf("bm_memcpy_s2d fail %s: %s: %d\n", __FILE__, __func__, __LINE__);
          }
      fail:
          free(src);
          return ret;
      }


      int main() {
          char *filename_src = "path/to/src";
          char *filename_water = "path/to/water_file";
          char *filename_dst = "path/to/dst";
          int in_width = 1920;
          int in_height = 1080;
          int water_width = 800;
          int water_height = 600;
          bm_image_format_ext src_format = FORMAT_RGB_PLANAR;
          bmcv_rect_t rects = {
              .start_x = 200,
              .start_y = 200,
              .crop_w = 800,
              .crop_h = 600};
          bmcv_color_t color = {
              .r = 0,
              .g = 0,
              .b = 0};

          bm_status_t ret = BM_SUCCESS;

          int src_size = in_width * in_height * 3;
          int water_size = water_width * water_height * 3;
          unsigned char *src_data = (unsigned char *)malloc(src_size);
          unsigned char *water_data = (unsigned char *)malloc(water_size);

          FILE *file;
          file = fopen(filename_water, "rb");
          fread(water_data, sizeof(unsigned char), water_size, file);
          fclose(file);

          file = fopen(filename_src, "rb");
          fread(src_data, sizeof(unsigned char), src_size, file);
          fclose(file);

          bm_handle_t handle = NULL;
          int dev_id = 0;
          bm_image src;
          bm_device_mem_t water;
          ret = bm_dev_request(&handle, dev_id);

          open_water(handle, filename_water, water_size, &water);

          bm_image_create(handle, in_height, in_width, src_format, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
          bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);

          int src_image_byte_size[4] = {0};
          bm_image_get_byte_size(src, src_image_byte_size);

          void *src_in_ptr[4] = {(void *)src_data,
                              (void *)((char *)src_data + src_image_byte_size[0]),
                              (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1]),
                              (void *)((char *)src_data + src_image_byte_size[0] + src_image_byte_size[1] + src_image_byte_size[2])};

          bm_image_copy_host_to_device(src, (void **)src_in_ptr);
          ret = bmcv_image_watermark_superpose(handle, &src, &water, 1, 0, water_width, &rects, color);
          bm_image_copy_device_to_host(src, (void **)src_in_ptr);

          writeBin(filename_dst, src_data, src_size);

          bm_image_destroy(&src);
          bm_dev_free(handle);

          free(src_data);
          free(water_data);

          return ret;
      }
