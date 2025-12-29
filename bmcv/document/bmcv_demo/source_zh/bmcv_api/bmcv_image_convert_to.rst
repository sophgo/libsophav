bmcv_image_convert_to
======================

该接口用于实现图像像素线性变化，具体数据关系可用如下公式表示：

.. math::
    y=kx+b


**接口形式：**

    .. code-block:: c

      bm_status_t bmcv_image_convert_to (
                  bm_handle_t handle,
                  int input_num,
                  bmcv_convert_to_attr convert_to_attr,
                  bm_image* input,
                  bm_image* output);


**参数说明：**

* bm_handle_t handle

  输入参数。bm_handle 句柄。

* int input_num

  输入参数。输入图片数，如果 input_num > 1, 那么多个输入图像必须是连续存储的（可以使用 bm_image_alloc_contiguous_mem 给多张图申请连续空间）。

* bmcv_convert_to_attr convert_to_attr

  输入参数。每张图片对应的配置参数。

* bm_image\* input

  输入参数。输入 bm_image。每个 bm_image 外部需要调用 bmcv_image_create 创建。image 内存可以使用 bm_image_alloc_dev_mem 或者 bm_image_copy_host_to_device 来开辟新的内存,或者使用 bmcv_image_attach 来 attach 已有的内存。

* bm_image\* output

  输出参数。输出 bm_image。每个 bm_image 外部需要调用 bmcv_image_create 创建。image 内存可以通过 bm_image_alloc_dev_mem 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。如果不主动分配将在 api 内部进行自行分配。


**数据类型说明：**

    .. code-block:: c

        struct bmcv_convert_to_attr_s{
              float alpha_0;
              float beta_0;
              float alpha_1;
              float beta_1;
              float alpha_2;
              float beta_2;
        } bmcv_convert_to_attr;


**参数说明：**

* float alpha_0

  第 0 个 channel 进行线性变换的系数

* float beta_0

  第 0 个 channel 进行线性变换的偏移

* float alpha_1

  第 1 个 channel 进行线性变换的系数

* float beta_1

  第 1 个 channel 进行线性变换的偏移

* float alpha_2

  第 2 个 channel 进行线性变换的系数

* float beta_2

 第 2 个 channel 进行线性变换的偏移


**返回值说明：**

* BM_SUCCESS: 成功

* 其他: 失败


**示例代码**

    .. code-block:: c

      #include <stdlib.h>
      #include <stdint.h>
      #include <stdio.h>
      #include <string.h>
      #include <math.h>
      #include "bmcv_api_ext_c.h"

      typedef enum {
          UINT8_C1 = 0,
          UINT8_C3,
          INT8_C1,
          INT8_C3,
          FLOAT32_C1,
          FLOAT32_C3,
          MAX_COLOR_TYPE
      } cv_color_e;

      typedef enum {
          CONVERT_1N_TO_1N = 0
      } convert_storage_mode_e;

      typedef struct {
          int idx;
          int trials;
          bm_handle_t handle;
      } convert_to_thread_arg_t;

      typedef struct {
          float alpha;
          float beta;
      } convert_to_arg_t;


      typedef struct {
          int n;
          int c;
          int w;
          int h;
      } image_shape_t;

      #define BM1688_MAX_W (2048)
      #define BM1688_MAX_H (2048)
      #define MIN_W (16)
      #define MIN_H (16)

      #define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))

      #ifdef __linux__
      #define ALIGN(x, a) __ALIGN_MASK(x, (__typeof__(x))(a)-1)
      #else
      #define ALIGN(x, a) __ALIGN_MASK(x, (int)(a)-1)
      #endif

      typedef enum { MIN_TEST = 0, MAX_TEST, RAND_TEST, MAX_RAND_MODE } rand_mode_e;

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

      static bm_image_data_format_ext convert_to_type_translation(int cv_color, int data_type, int if_input) {
          bm_image_data_format_ext image_format = DATA_TYPE_EXT_1N_BYTE;
          if (((UINT8_C3 == cv_color) || (UINT8_C1 == cv_color)) &&
                    (CONVERT_1N_TO_1N == data_type)) {
              image_format = DATA_TYPE_EXT_1N_BYTE;
          } else if ((FLOAT32_C1 == cv_color) || (FLOAT32_C3 == cv_color)) {
              image_format = DATA_TYPE_EXT_FLOAT32;
          } else if (((INT8_C3 == cv_color) || (INT8_C1 == cv_color)) &&
                    (CONVERT_1N_TO_1N == data_type)) {
              image_format = DATA_TYPE_EXT_1N_BYTE_SIGNED;
          }

          return image_format;
      }


      int main(){
          int dev_id = 0;
          bm_handle_t handle = NULL;

          int image_n = 1;
          int image_c = 3;
          int image_w = 320;
          int image_h = 320;
          image_shape_t image_shape;

          convert_to_arg_t convert_to_arg[image_c], same_convert_to_arg[image_c];
          int convert_format = CONVERT_1N_TO_1N;

          bm_status_t ret = bm_dev_request(&handle, dev_id);

          image_shape.n = image_n;
          image_shape.c = image_c;
          image_shape.h = image_h;
          image_shape.w = image_w;
          memset(convert_to_arg, 0x0, sizeof(convert_to_arg));
          memset(same_convert_to_arg, 0x0, sizeof(same_convert_to_arg));
          float same_alpha = ((float)(rand() % 20)) / (float)10;
          float same_beta = ((float)(rand() % 20)) / (float)10 - 1;
          for(int i = 0; i < image_c; i++){
              convert_to_arg[i].alpha = ((float)(rand() % 20)) / (float)10;
              convert_to_arg[i].beta = ((float)(rand() % 20)) / (float)10 - 1;
              same_convert_to_arg[i].alpha = same_alpha;
              same_convert_to_arg[i].beta = same_beta;
          }

          int input_data_type = INT8_C3;
          int output_data_type = INT8_C3;

          int image_num      = image_shape.n;
          int image_channel  = image_shape.c;
          // int image_w        = image_shape.w;
          int image_w_stride = 0;
          // int image_h        = image_shape.h;
          int image_len      = 0;

          image_w_stride = image_w;
          image_len = image_num * image_channel * image_w * image_h;

          signed char *input = malloc(image_len * sizeof(signed char));
          signed char *bmcv_res = malloc(image_len * sizeof(signed char));

          memset(input, 0x0, image_len);
          memset(bmcv_res, 0x0, image_len);

          for(int i = 0; i < image_len; i++){
                  input[i] = rand() % 255;
          }

          bm_image_data_format_ext input_data_format_ext, output_data_format_ext;
          bmcv_convert_to_attr     convert_to_attr;
          convert_to_attr.alpha_0 = convert_to_arg[0].alpha;
          convert_to_attr.beta_0  = convert_to_arg[0].beta;
          convert_to_attr.alpha_1 = convert_to_arg[1].alpha;
          convert_to_attr.beta_1  = convert_to_arg[1].beta;
          convert_to_attr.alpha_2 = convert_to_arg[2].alpha;
          convert_to_attr.beta_2  = convert_to_arg[2].beta;

          input_data_format_ext = convert_to_type_translation(input_data_type, convert_format, 1);
          output_data_format_ext = convert_to_type_translation(output_data_type, convert_format, 0);

          bm_image input_images[image_num];
          bm_image output_images[32];

          int input_num = image_num;
          int output_num = image_num;

          for(int img_idx = 0; img_idx < input_num; img_idx++){
              bm_image_create(handle,
                              image_h,
                              image_w,
                              FORMAT_BGR_PLANAR,
                              input_data_format_ext,
                              &input_images[img_idx],
                              NULL);
          }

          for(int img_idx = 0; img_idx < input_num; img_idx++){
              int img_offset = image_w_stride * image_h * image_channel;
              signed char *input_img_data = input + img_offset * img_idx;
              bm_image_copy_host_to_device(input_images[img_idx], (void **)&input_img_data);
          }

          for(int img_idx = 0; img_idx < output_num; img_idx++){
              bm_image_create(handle,
                              image_h,
                              image_w,
                              FORMAT_BGR_PLANAR,
                              output_data_format_ext,
                              &output_images[img_idx],
                              NULL);
          }

          bm_image_alloc_contiguous_mem(output_num, output_images, BMCV_HEAP0_ID);

          bmcv_image_convert_to(handle, input_num, convert_to_attr, input_images, output_images);

          char filename[256];

          for(int img_idx = 0; img_idx < output_num; img_idx++){
              int img_offset = image_w * image_h * image_channel;
              signed char *res_img_data = bmcv_res + img_offset * img_idx;
              bm_image_copy_device_to_host(output_images[img_idx], (void **)&res_img_data);
              snprintf(filename, sizeof(filename), "path/to/dst_%d.bin", img_idx);
              writeBin(filename, bmcv_res, image_h * image_w);
          }

          writeBin("path/to/src", input, image_h * image_w);


          free(input);
          free(bmcv_res);

          for(int i = 0; i < input_num; i++){
              bm_image_destroy(&input_images[i]);
          }
          for(int i = 0; i < output_num; i++){
                  bm_image_destroy(&output_images[i]);
          }
          dm_dev_free(handle);

          return ret;
      }




**格式支持:**

1. 该接口支持下列 image_format 的转化：

* FORMAT_BGR_PLANAR ——> FORMAT_BGR_PLANAR
* FORMAT_RGB_PLANAR ——> FORMAT_RGB_PLANAR
* FORMAT_GRAY       ——> FORMAT_GRAY

2. 该接口支持下列情形data type之间的转换：

* DATA_TYPE_EXT_1N_BYTE        ——> DATA_TYPE_EXT_FLOAT32
* DATA_TYPE_EXT_1N_BYTE        ——> DATA_TYPE_EXT_1N_BYTE
* DATA_TYPE_EXT_1N_BYTE_SIGNED ——> DATA_TYPE_EXT_1N_BYTE_SIGNED
* DATA_TYPE_EXT_1N_BYTE        ——> DATA_TYPE_EXT_1N_BYTE_SIGNED
* DATA_TYPE_EXT_FLOAT32        ——> DATA_TYPE_EXT_FLOAT32


**注意事项：**

1. 在调用 bmcv_image_convert_to()之前必须确保输入的 image 内存已经申请。

2. 输入的各个 image 的宽、高以及 data_type、image_format 必须相同。

3. 输出的各个 image 的宽、高以及 data_type、image_format 必须相同。

4. 输入 image 宽、高必须等于输出 image 宽高。

5. image_num 必须大于 0。

6. 输出 image 的 stride 必须等于 width。

7. 输入 image 的 stride 必须大于等于 width。

8. 输入图片的最小尺寸为16 * 16，最大尺寸为4096 * 4096，当输入 data type 为 DATA_TYPE_EXT_1N_BYTE 时，最大尺寸可以支持8192 * 8192。

9. 输出图片的最小尺寸为16 * 16，最大尺寸8192 * 8192。