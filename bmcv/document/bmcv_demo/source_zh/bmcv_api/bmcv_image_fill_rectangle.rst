bmcv_image_fill_rectangle
-------------------------

**描述：**

该接口用于在图像上填充一个或者多个矩形。

**语法：**

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_fill_rectangle(
            bm_handle_t   handle,
            bm_image      image,
            int           rect_num,
            bmcv_rect_t * rects,
            unsigned char r,
            unsigned char g,
            unsigned char b)

**参数：**

.. list-table:: bmcv_image_fill_rectangle 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - image
      - 输入
      - 需要在其上画矩形框的 bm_image 对象。
    * - rect_num
      - 输入
      - 矩形框数量，指 rects 指针中所包含的 bmcv_rect_t 对象个数。
    * - \*rect
      - 输出
      - 矩形框对象指针，包含矩形起始点和宽高。具体内容参考下面的数据类型说明。
    * - r
      - 输入
      - 矩形框颜色的 r 分量。
    * - g
      - 输入
      - 矩形框颜色的 g 分量。
    * - b
      - 输入
      - 矩形框颜色的 b 分量。

**返回值：**

该函数成功调用时, 返回 BM_SUCCESS。

**数据类型说明：**

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_rect {
        int start_x;
        int start_y;
        int crop_w;
        int crop_h;
    } bmcv_rect_t;

* start_x 描述了 crop 图像在原图中所在的起始横坐标。自左而右从 0 开始，取值范围 [0, width)。
* start_y 描述了 crop 图像在原图中所在的起始纵坐标。自上而下从 0 开始，取值范围 [0, height)。
* crop_w 描述的 crop 图像的宽度，也就是对应输出图像的宽度。
* crop_h 描述的 crop 图像的高度，也就是对应输出图像的高度。

**格式支持：**

1. 输入和输出的数据类型

+-----+-------------------------------+
| num | data_type                     |
+=====+===============================+
|  1  | DATA_TYPE_EXT_1N_BYTE         |
+-----+-------------------------------+

2. 输入和输出的色彩格式必须保持一致，可支持：

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

**注意：**

1. 输入输出所有 bm_image 结构必须提前创建，否则返回失败。

#. 如果rect_num为0，则自动返回成功。

#. 如果line_width小于零，则返回失败。

#. 所有输入矩形对象部分在image之外，则只会画出在image之内的线条，并返回成功。

**代码示例：**

.. code-block:: cpp
    :linenos:
    :lineno-start: 1
    :force:

    #include <limits.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    #include "bmcv_api_ext_c.h"

    int main() {
      char* filename_src = "path/to/src";
      char* filename_dst = "path/to/dst";
      int in_width = 1920;
      int in_height = 1080;
      int rect_num = 1;
      bm_image_format_ext src_format = 8;
      bmcv_rect_t crop_rect = {
          .start_x = 100,
          .start_y = 100,
          .crop_w = 100,
          .crop_h = 100};
      unsigned char r = 0;
      unsigned char g = 0;
      unsigned char b = 0;

      bm_status_t ret = BM_SUCCESS;

      int src_size = in_width * in_height * 3;
      unsigned char *input_data = (unsigned char *)malloc(src_size);

      FILE *file;
      file = fopen(filename_src, "rb");
      fread(input_data, sizeof(unsigned char), src_size, file);
      fclose(file);

      bm_handle_t handle = NULL;
      int dev_id = 0;
      bm_image src;

      ret = bm_dev_request(&handle, dev_id);
      if (ret != BM_SUCCESS) {
          printf("Create bm handle failed. ret = %d\n", ret);
          return ret;
      }

      bm_image_create(handle, in_height, in_width, src_format, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
      bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);

      void *src_in_ptr[3] = {(void *)input_data,
                            (void *)((char *)input_data + in_height * in_width),
                            (void *)((char *)input_data + 2 * in_height * in_width)};

      bm_image_copy_host_to_device(src, (void **)src_in_ptr);
      ret = bmcv_image_fill_rectangle(handle, src, rect_num, &crop_rect, r, g, b);
      bm_image_copy_device_to_host(src, (void **)src_in_ptr);

      bm_image_destroy(&src);
      bm_dev_free(handle);


      file = fopen(filename_dst, "wb");
      fwrite(input_data, sizeof(unsigned char), src_size, file);
      fclose(file);


      free(input_data);
      return ret;
    }
