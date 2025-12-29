bmcv_image_warp_affine
=======================

该接口实现图像的仿射变换，可实现旋转、平移、缩放等操作。仿射变换是一种二维坐标 (:math:`x` , :math:`y`) 到二维坐标(:math:`x_0` , :math:`y_0`)的线性变换，该接口的实现是针对输出图像的每一个像素点找到在输入图像中对应的坐标，从而构成一幅新的图像，其数学表达式形式如下：

  .. math::

      \left\{
      \begin{array}{c}
      x_0=a_1x+b_1y+c_1 \\
      y_0=a_2x+b_2y+c_2 \\
      \end{array}
      \right.

对应的齐次坐标矩阵表示形式为：

  .. math::

      \left[\begin{matrix} x_0 \\ y_0 \\ 1 \end{matrix} \right]=\left[\begin{matrix} a_1&b_1&c_1 \\ a_2&b_2&c_2 \\ 0&0&1 \end{matrix} \right]\times \left[\begin{matrix} x \\ y \\ 1 \end{matrix} \right]


坐标变换矩阵是一个 6 点的矩阵，该矩阵是从输出图像坐标推导输入图像坐标的系数矩阵，可以通过输入输出图像上对应的 3 个点坐标来获取。在人脸检测中，通过获取人脸定位点来获取变换矩阵。


bmcv_affine_matrix 定义了一个坐标变换矩阵，其顺序为 float m[6] = {a1, b1, c1, a2, b2, c2}。而 bmcv_affine_image_matrix 定义了一张图片里面有几个变换矩阵，通常来说一张图片有多个人脸时，会对应多个变换矩阵。

    .. code-block:: c

        typedef struct bmcv_affine_matrix_s{
                float m[6];
        } bmcv_warp_matrix;

        typedef struct bmcv_affine_image_matrix_s{
                bmcv_affine_matrix *matrix;
                int matrix_num;
        } bmcv_affine_image_matrix;


**接口形式一:**

    .. code-block:: c

        bm_status_t bmcv_image_warp_affine(
                    bm_handle_t handle,
                    int image_num,
                    bmcv_affine_image_matrix matrix[4],
                    bm_image* input,
                    bm_image* output,
                    int use_bilinear = 0);

**接口形式二:**

    .. code-block:: c

        bm_status_t bmcv_image_warp_affine_similar_to_opencv(
                    bm_handle_t handle,
                    int image_num,
                    bmcv_affine_image_matrix matrix[4],
                    bm_image* input,
                    bm_image* output,
                    int use_bilinear = 0);

本接口是对齐opencv仿射变换的接口， 该矩阵是从输入图像坐标推导输出图像坐标的系数矩阵。


**参数说明**

* bm_handle_t handle

  输入参数。输入的 bm_handle 句柄。

* int image_num

  输入参数。输入图片数，最多支持 4。

* bmcv_affine_image_matrix matrix[4]

  输入参数。每张图片对应的变换矩阵数据结构，最多支持 4 张图片。

* bm_image\* input

  输入参数。输入 bm_image，对于 1N 模式，最多 4 个 bm_image，对于 4N 模式，最多一个 bm_image。

* bm_image\* output

  输出参数。输出 bm_image，外部需要调用 bmcv_image_create 创建，建议用户调用 bmcv_image_attach 来分配 device memory。如果用户不调用 attach，则内部分配 device memory。对于输出 bm_image，其数据类型和输入一致，即输入是 4N 模式，则输出也是 4N 模式,输入 1N 模式，输出也是 1N 模式。所需要的 bm_image 大小是所有图片的变换矩阵之和。比如输入 1 个 4N 模式的 bm_image，4 张图片的变换矩阵数目为【3,0,13,5】，则共有变换矩阵 3+0+13+5=21，由于输出是 4N 模式，则需要(21+4-1)/4=6 个 bm_image 的输出。

* int use_bilinear

  输入参数。是否使用 bilinear 进行插值，若为 0 则使用 nearest 插值，若为 1 则使用 bilinear 插值，默认使用 nearest 插值。选择 nearest 插值的性能会优于 bilinear，因此建议首选 nearest 插值，除非对精度有要求时可选择使用 bilinear 插值。


**返回值说明:**

* BM_SUCCESS: 成功

* 其他: 失败


**注意事项**

1. 该接口所支持的 image_format 包括：

   +-----+------------------------+
   | num | image_format           |
   +=====+========================+
   |  1  | FORMAT_BGR_PLANAR      |
   +-----+------------------------+
   |  2  | FORMAT_RGB_PLANAR      |
   +-----+------------------------+

2. 该接口所支持的 data_type 包括：

   +-----+------------------------+
   | num | data_type              |
   +=====+========================+
   |  1  | DATA_TYPE_EXT_1N_BYTE  |
   +-----+------------------------+
   |  2  | DATA_TYPE_EXT_4N_BYTE  |
   +-----+------------------------+

3. 该接口的输入以及输出 bm_image 均支持带有 stride。

4. 要求该接口输入 bm_image 的 width、height、image_format 以及 data_type 必须保持一致。

5. 要求该接口输出 bm_image 的 width、height、image_format、data_type 以及 stride 必须保持一致。


**示例代码**

    .. code-block:: c

      #include <stdlib.h>
      #include <stdint.h>
      #include <stdio.h>
      #include <string.h>
      #include <math.h>
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

      static unsigned char* image_read_2(
                            int            image_n,
                            int            image_c,
                            int            image_sh,
                            int            image_sw,
                            int            image_dh,
                            int            image_dw) {

          unsigned char* res          = (unsigned char*) malloc(image_n * image_c * image_sh * image_sw * sizeof(unsigned char));
          unsigned char* res_temp     = (unsigned char*) malloc(image_n * image_c * image_dh * image_dw * sizeof(unsigned char));
          unsigned char* res_temp_bak = (unsigned char*) malloc(image_n * image_c * image_dh * image_dw * sizeof(unsigned char));

          for (int i = 0; i < image_n * image_c * image_sh * image_sw; i++)
          {
              res[i] = i % 255;
          }

          if (image_dh <= image_sh && image_dw <= image_sw)
              return res;

          if (image_dh > image_sh){
              int pad_h_value = (image_dh - image_sh) / 2;
              for (int i = 0;i < pad_h_value * image_sw;i++)
                  res_temp[i] = 0;

              for (int i = pad_h_value * image_sw, j = 0; i < pad_h_value * image_sw + image_n * image_c * image_sh * image_sw;i++,j++)
                  res_temp[i] = res[j];

              for (int i = pad_h_value * image_sw + image_n * image_c * image_sh * image_sw;i <  pad_h_value * image_sw + image_n * image_c * image_sh * image_sw + pad_h_value * image_sw;i++)
                  res_temp[i] = 0;
          }

          if (image_dw > image_sw){
              int pad_w_value = (image_dw - image_sw) / 2;
              int j = 0;
              for (int i = 0;i < image_dh;i++){
                  for (;j < pad_w_value + i * image_dw;j++)
                      res_temp_bak[j] = 0;
                  for (;j < pad_w_value + image_sw + i * image_dw;j++)
                      res_temp_bak[j] = res_temp[j - pad_w_value - i * image_dw + i * image_sw];
                  for (;j < pad_w_value + pad_w_value + image_sw + i * image_dw;j++)
                      res_temp_bak[j] = 0;
              }
          }

          free(res);
          free(res_temp);
          return res_temp_bak;
      }


      int main() {
          int image_sh = 1080;
          int image_sw = 1920;
          int image_dh = 112;
          int image_dw = 112;
          int is_bilinear = 0;
          bm_handle_t handle;
          int dev_id = 0;
          bm_status_t ret = bm_dev_request(&handle, dev_id);

          int image_c = 3;
          int image_n = 1;
          int output_num = 1;

          unsigned char* src_data = image_read_2(image_n, image_c, image_sh, image_sw, image_dh, image_dw);
          float* trans_mat = (float*)malloc(output_num * 6 * sizeof(float));

          for (int i = 0; i < output_num; i++){
              trans_mat[0 + i * 6] = 3.84843f;
              trans_mat[1 + i * 6] = -0.0248411f;
              trans_mat[2 + i * 6] = 916.203f;
              trans_mat[3 + i * 6] = 0.0248411;
              trans_mat[4 + i * 6] = 3.84843f;
              trans_mat[5 + i * 6] = 55.9748f;
          }

          // int* map = (int*)malloc(output_num *image_dh *image_dw * 2 * sizeof(int));
          unsigned char* dst_image_tpu = (unsigned char*)malloc(output_num * image_c * image_dh * image_dw * sizeof(unsigned char));


          bmcv_affine_image_matrix matrix_image[4];
          bmcv_affine_matrix *     matrix = (bmcv_affine_matrix *)(trans_mat);
          for (int i = 0; i < image_n; i++) {
              matrix_image[i].matrix_num = 1;
              matrix_image[i].matrix     = matrix;
              matrix += 1;
          }

          bm_image                 src_img[4];
          bm_image_format_ext      image_format = FORMAT_BGR_PLANAR;
          bm_image_data_format_ext data_type    = DATA_TYPE_EXT_1N_BYTE;

          for (int i = 0; i < image_n; i++){
              bm_image_create(handle, image_sh, image_sw, image_format, data_type, src_img + i, NULL);
              int stride = 0;
              bm_image_get_stride(src_img[i], &stride);
              void *ptr = (void *)(src_data + 3 * stride * image_sh * i);
              bm_image_copy_host_to_device(src_img[i], (void **)(&ptr));
          }

          // create dst image.
          bm_image* dst_img = (bm_image*)malloc(image_n * sizeof(bm_image));

          for (int i = 0; i < image_n; i++) {
              bm_image_create(handle, image_dh, image_dw, image_format, data_type, dst_img + i, NULL);
          }
          ret = bmcv_image_warp_affine(handle, image_n, matrix_image, src_img, dst_img, is_bilinear);

          int size = 0;
          bm_image_get_byte_size(dst_img[0], &size);
          unsigned char* temp_out = (unsigned char*)malloc(output_num * size * sizeof(unsigned char));
          for (int i = 0; i < image_n; i++) {
              void *ptr = (void *)(temp_out + size * i);
              bm_image_copy_device_to_host(dst_img[i], (void **)&ptr);
          }
          memcpy(dst_image_tpu, temp_out, image_n * image_c * image_dh * image_dw);

          for (int i = 0; i < image_n; i++){
              bm_image_destroy(&src_img[i]);
          }
          char *dst_name = "path/to/dst";
          writeBin(dst_name, temp_out,  size);
          writeBin("path/to/src", src_data, image_sh * image_sw * 3);
          free(src_data);
          free(dst_img);
          free(temp_out);

          // free(map);
          free(dst_image_tpu);


          return ret;
      }


bmcv_image_warp_affine_padding
==============================

**接口说明**

* 所有的使用方式均和上述的 bmcv_image_warp_affine 相同，仅仅改变了接口名字，具体的 padding zero 的接口名字如下：

**接口形式一:**

    .. code-block:: c

        bm_status_t bmcv_image_warp_affine_padding(
            bm_handle_t handle,
            int image_num,
            bmcv_affine_image_matrix matrix[4],
            bm_image *input,
            bm_image *output,
            int use_bilinear);

**接口形式一:**

    .. code-block:: c

        bm_status_t bmcv_image_warp_affine_similar_to_opencv_padding(
            bm_handle_t handle,
            int image_num,
            bmcv_affine_image_matrix matrix[4],
            bm_image *input,
            bm_image *output,
            int use_bilinear);

**代码示例说明**

* 同 bmcv_image_warp_affine 接口使用方式相同，只需要将接口名字换成 bmcv_image_warp_affine_padding 或 bmcv_image_warp_affine_similar_to_opencv_padding 即可。