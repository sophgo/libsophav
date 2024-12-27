bmcv_image_draw_lines
======================

可以实现在一张图像上画一条或多条线段，从而可以实现画多边形的功能，并支持指定线的颜色和线的宽度。


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

        bm_status_t bmcv_image_draw_lines(
                    bm_handle_t handle,
                    bm_image img,
                    const bmcv_point_t* start,
                    const bmcv_point_t* end,
                    int line_num,
                    bmcv_color_t color,
                    int thickness);


**参数说明：**

* bm_handle_t handle

  输入参数。 bm_handle 句柄。

* bm_image img

  输入/输出参数。需处理图像的 bm_image，bm_image 需要外部调用 bmcv_image_create 创建。image 内存可以使用 bm_image_alloc_dev_mem 或者 bm_image_copy_host_to_device 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。

* const bmcv_point_t* start

  输入参数。线段起始点的坐标指针，指向的数据长度由 line_num 参数决定。图像左上角为原点，向右延伸为x方向，向下延伸为y方向。

* const bmcv_point_t* end

  输入参数。线段结束点的坐标指针，指向的数据长度由 line_num 参数决定。图像左上角为原点，向右延伸为x方向，向下延伸为y方向。

* int line_num

  输入参数。需要画线的数量。

* bmcv_color_t color

  输入参数。画线的颜色，分别为RGB三个通道的值。

* int thickness

  输入参数。画线的宽度，对于YUV格式的图像建议设置为偶数。


**返回值说明：**

* BM_SUCCESS: 成功

* 其他: 失败


**格式支持：**

该接口目前支持以下 image_format:

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

目前支持以下 data_type:

+-----+--------------------------------+
| num | data_type                      |
+=====+================================+
| 1   | DATA_TYPE_EXT_1N_BYTE          |
+-----+--------------------------------+


**示例代码**

    .. code-block:: c

      #include "bmcv_api_ext_c.h"
      #include <stdio.h>
      #include <stdlib.h>

      #define ALIGN(num, align) (((num) + ((align) - 1)) & ~((align) - 1))
      #define SATURATE(a, s, e) ((a) > (e) ? (e) : ((a) < (s) ? (s) : (a)))
      #define IMAGE_CHN_NUM_MAX 3

      static int line_num = 6;
      static bmcv_point_t start[6] = {{0, 0}, {500, 0}, {500, 500}, {0, 500}, {0, 0}, {0, 500}};
      static bmcv_point_t end[6] = {{500, 0}, {500, 500}, {0, 500}, {0, 0}, {500, 500}, {500, 0}};
      static unsigned char color[3] = {255, 0, 0};
      static int thickness = 4;

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
          int format = 0;
          int ret = 0;
          char *input_path = "path/to/input";
          char *output_path = "path/to/output";
          bm_handle_t handle;
          ret = bm_dev_request(&handle, 0);
          if (ret != BM_SUCCESS) {
              printf("bm_dev_request failed. ret = %d\n", ret);
              return -1;
          }
          unsigned char* data_tpu = (unsigned char*)malloc(width * height * IMAGE_CHN_NUM_MAX * sizeof(unsigned char));
          int offset_list[IMAGE_CHN_NUM_MAX] = {0};
          int total_size = 0;
          ret = readBin(input_path, data_tpu);

          unsigned char* input = data_tpu;
          const bmcv_point_t* p1 = start;
          const bmcv_point_t* p2 = end;

          bm_image input_img;
          unsigned char* in_ptr[IMAGE_CHN_NUM_MAX] = {0};
          bmcv_color_t rgb = {color[0], color[1], color[2]};

          ret = bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input_img, NULL);
          ret = bm_image_alloc_dev_mem(input_img, BMCV_HEAP1_ID);

          offset_list[0] = width * height;
          offset_list[1] = ALIGN(width, 2) * ALIGN(height, 2) >> 2;
          offset_list[2] = ALIGN(width, 2) * ALIGN(height, 2) >> 2;

          in_ptr[0] = input;
          in_ptr[1] = input + offset_list[0];
          in_ptr[2] = input + offset_list[0] + offset_list[1];

          ret = bm_image_copy_host_to_device(input_img, (void**)in_ptr);
          bmcv_image_draw_lines(handle, input_img, p1, p2, line_num, rgb, thickness);
          ret = bm_image_copy_device_to_host(input_img, (void**)in_ptr);

          bm_image_destroy(&input_img);

          for (int i = 0; i < IMAGE_CHN_NUM_MAX; ++i) {
              total_size += offset_list[i];
          }

          ret = writeBin(output_path, data_tpu, total_size);

          free(data_tpu);
          bm_dev_free(handle);
          return ret;
      }