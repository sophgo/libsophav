bmcv_image_put_text
===================

可以实现在一张图像上写字的功能，并支持指定字的颜色、大小和宽度。


**接口形式：**

    .. code-block:: c

        typedef struct {
            int x;
            int y;
        } bmcv_point_t;

        typedef struct {
            unsigned char r;
            unsigned char g;
            unsigned char b;
        } bmcv_color_t;

        bm_status_t bmcv_image_put_text(
                    bm_handle_t handle,
                    bm_image image,
                    const char* text,
                    bmcv_point_t org,
                    bmcv_color_t color,
                    float fontScale,
                    int thickness);


**参数说明：**

* bm_handle_t handle

  输入参数。 bm_handle 句柄。

* bm_image image

  输入/输出参数。需处理图像的 bm_image，bm_image 需要外部调用 bmcv_image_create 创建。image 内存可以使用 bm_image_alloc_dev_mem 或者 bm_image_copy_host_to_device 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。

* const char* text

  输入参数。待写入的文本内容。当文本内容中有中文时，thickness请设置为0。

* bmcv_point_t org

  输入参数。第一个字符左下角的坐标位置。图像左上角为原点，向右延伸为x方向，向下延伸为y方向。

* bmcv_color_t color

  输入参数。画线的颜色，分别为RGB三个通道的值。

* float fontScale

  输入参数。字体大小。

* int thickness

  输入参数。画线的宽度，对于YUV格式的图像建议设置为偶数。开启中文字库请将该参数设置为0。


**返回值说明：**

* BM_SUCCESS: 成功

* 其他: 失败


**格式支持：**

1. 该接口目前支持以下 image_format:

+-----+------------------------+
| num | image_format           |
+=====+========================+
| 1   | FORMAT_GRAY            |
+-----+------------------------+
| 2   | FORMAT_YUV420P         |
+-----+------------------------+
| 3   | FORMAT_YUV422P         |
+-----+------------------------+
| 4   | FORMAT_YUV444P         |
+-----+------------------------+
| 5   | FORMAT_NV12            |
+-----+------------------------+
| 6   | FORMAT_NV21            |
+-----+------------------------+
| 7   | FORMAT_NV16            |
+-----+------------------------+
| 8   | FORMAT_NV61            |
+-----+------------------------+

thickness参数配置为0，即开启中文字库后，image_format 扩展支持 bmcv_image_overlay API 底图支持的格式。

2. 目前支持以下 data_type:

+-----+--------------------------------+
| num | data_type                      |
+=====+================================+
| 1   | DATA_TYPE_EXT_1N_BYTE          |
+-----+--------------------------------+

3. 若文字内容不变，推荐使用 bmcv_gen_text_watermask 与 bmcv_image_overlay 搭配的文字绘制方式，文字生成水印图，重复使用水印图进行osd叠加，以提高处理效率。示例参见bmcv_image_overlay接口文档。

**示例代码**

    .. code-block:: c

      #include "bmcv_api_ext_c.h"
      #include <stdio.h>
      #include <stdlib.h>

      #define IMAGE_CHN_NUM_MAX 3
      #define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
      #define ALIGN(x, a) __ALIGN_MASK(x, (__typeof__(x))(a)-1)

      static bmcv_point_t org = {500, 500};
      static int fontScale = 5;
      static const char text[30] = "Hello, world!";
      static unsigned char color[3] = {255, 0, 0};
      static int thickness = 2;


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

      static int readBin(const char* path, void* input_data)
      {
          int len;
          int size;
          FILE* fp_src = fopen(path, "rb");

          if (fp_src == NULL) {
              perror("Error opening file\n");
              return -1;
          }

          fseek(fp_src, 0, SEEK_END);
          size = ftell(fp_src);
          fseek(fp_src, 0, SEEK_SET);

          len = fread((void*)input_data, 1, size, fp_src);
          if (len < size) {
              printf("file size = %d is less than required bytes = %d\n", len, size);
              return -1;
          }

          fclose(fp_src);
          return 0;
      }

      int main()
      {
          int width = 1920;
          int height = 1080;
          int format = FORMAT_YUV420P;
          int ret = 0;
          char* input_path = "path/to/input";
          char* output_path = "path/to/output";
          int i;
          bm_handle_t handle;
          ret = bm_dev_request(&handle, 0);

          int offset_list[IMAGE_CHN_NUM_MAX] = {0};
          offset_list[0] = width * height;
          offset_list[1] = ALIGN(width, 2) * ALIGN(height, 2) >> 2;
          offset_list[2] = ALIGN(width, 2) * ALIGN(height, 2) >> 2;

          int total_size = 0;
          unsigned char* data_tpu = (unsigned char*)malloc(width * height * IMAGE_CHN_NUM_MAX * sizeof(unsigned char));

          ret = readBin(input_path, data_tpu);

          bm_image input_img;
          unsigned char* in_ptr[IMAGE_CHN_NUM_MAX] = {0};
          bmcv_color_t rgb = {color[0], color[1], color[2]};

          ret = bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input_img, NULL);
          ret = bm_image_alloc_dev_mem(input_img, BMCV_HEAP1_ID);

          in_ptr[0] = data_tpu;
          in_ptr[1] = data_tpu + offset_list[0];
          in_ptr[2] = data_tpu + offset_list[0] + offset_list[1];

          ret = bm_image_copy_host_to_device(input_img, (void**)in_ptr);
          ret = bmcv_image_put_text(handle, input_img, text, org, rgb, fontScale, thickness);
          ret = bm_image_copy_device_to_host(input_img, (void**)in_ptr);

          bm_image_destroy(&input_img);

          for (i = 0; i < IMAGE_CHN_NUM_MAX; ++i) {
              total_size += offset_list[i];
          }
          ret = writeBin(output_path, data_tpu, total_size);

          free(data_tpu);
          bm_dev_free(handle);
          return ret;
      }