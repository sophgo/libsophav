bmcv_ive_csc
------------------------------

| 【描述】

| 该 API 使用ive硬件资源的 csc 功能, 创建色彩空间转换任务, 可实现YUV2RGB、YUV2HSV、RGB2YUV 的色彩空间转换。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_csc(
            bm_handle_t     handle,
            bm_image        input,
            bm_image        output,
            csc_type_t      csc_type);

| 【参数】

.. list-table:: bmcv_ive_csc 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - input
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - output
      - 输出
      - 输出 bm_image 对象结构体, 不能为空, 宽、高同 input。
    * - csc_type
      - 输入
      - csc 工作模式。

.. list-table::
    :widths: 20 56 26 32
    :align: center

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - FORMAT_NV21

        FORMAT_NV61

        FORMAT_RGB_PACKED

        FORMAT_RGB_PLANAR

        FORMAT_RGBP_SEPARATE

      - U8
      - 64x64~1920x1080
    * - output
      - FORMAT_NV21

        FORMAT_NV61

        FORMAT_RGB_PACKED

        FORMAT_RGB_PLANAR

        FORMAT_RGBP_SEPARATE

      - U8
      - 同 input

【补充】FORMAT_YUV444P可以转FORMAT_HSV_PLANAR，转换模式色彩空间转换控制模式选择CSC_YCbCr2RGB_BT601或者CSC_YCbCr2RGB_BT709。


| 【数据类型说明】

【说明】定义色彩空间转换控制模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum csc_type {
        CSC_YCbCr2RGB_BT601 = 0,
        CSC_YPbPr2RGB_BT601,
        CSC_RGB2YCbCr_BT601,
        CSC_YCbCr2RGB_BT709,
        CSC_RGB2YCbCr_BT709,
        CSC_RGB2YPbPr_BT601,
        CSC_YPbPr2RGB_BT709,
        CSC_RGB2YPbPr_BT709,
        CSC_FANCY_PbPr_BT601 = 100,
        CSC_FANCY_PbPr_BT709,
        CSC_USER_DEFINED_MATRIX = 1000,
        CSC_MAX_ENUM
    } csc_type_t;

.. list-table::
    :widths: 100 100

    * - **成员名称**
      - **描述**
    * - CSC_YCbCr2RGB_BT601
      - BT601 的 YUV2RGB 图片变换

    * - CSC_YPbPr2RGB_BT601
      - BT601 的 YUV2RGB 视频变换。

    * - CSC_RGB2YCbCr_BT601
      - BT601 的 RGB2YUV 图像变换。

    * - CSC_YCbCr2RGB_BT709
      - BT709 的 YUV2RGB 图像变换。

    * - CSC_RGB2YCbCr_BT709
      - BT709 的 RGB2YUV 图像变换。

    * - CSC_RGB2YPbPr_BT601
      - BT601 的 RGB2YUV 视频变换。

    * - CSC_YPbPr2RGB_BT709
      - BT709 的 YUV2RGB 视频变换。

    * - CSC_RGB2YPbPr_BT709
      - BT709 的 RGB2YUV 视频变换。


【注意】

* CSC_YPbPr2RGB_BT601 和 CSC_YPbPr2RGB_BT709 模式，输出满足 16≤R、 G、 B≤235。

* CSC_YCbCr2RGB_BT601 和 CSC_YCbCr2RGB_BT709 模式，输出满足 0≤R、 G、 B≤255。

* CSC_RGB2YPbPr_BT601 和 CSC_RGB2YPbPr_BT709 模式，输出满足 0≤Y、 U、 V≤ 255。

* CSC_RGB2YCbCr_BT601 和 CSC_RGB2YCbCr_BT709 模式。, 输出满足 0≤Y≤235, 0≤U、 V≤240。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 当输出数据为 FORMAT_RGB_PLANAR、 FORMAT_NV21、 FORMAT_NV61 类型时， 要求输出数据跨度一致。

3. 不同的模式其输出的取值范围不一样。

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
          int height = 1080, width = 1920;
          csc_type_t csc_type = CSC_YCbCr2RGB_BT601;
          bm_image_format_ext src_fmt = 2; // FORMAT_NV21: 4; yuv444p : 2 FORMAT_RGB_PACKED: 10
          bm_image_format_ext dst_fmt = 10;
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

          // calc ive image stride && create bm image struct
          int data_size = 1;
          src_stride[0] = align_up(width, 16) * data_size;
          src_stride[1] = align_up(width, 16) * data_size;
          src_stride[2] = align_up(width, 16) * data_size;
          dst_stride[0] = align_up(width*3, 16) * data_size;

          bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
          bm_image_create(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);

          ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

          int image_byte_size[4] = {0};
          bm_image_get_byte_size(src, image_byte_size);

          int byte_size = width * height * 3;
          unsigned char *input_data = (unsigned char *)malloc(byte_size);
          FILE *fp_src = fopen(src_name, "rb");
          if (fread((void *)input_data, 1, byte_size, fp_src) < (unsigned int)byte_size) {
            printf("file size is less than required bytes%d\n", byte_size);
          };
          fclose(fp_src);
          void* in_ptr[3] = {(void *)input_data,
                            (void *)input_data + width * height,
                            (void *)input_data + width * height * 2};
          bm_image_copy_host_to_device(src, (void **)in_ptr);

          ret = bmcv_ive_csc(handle, src, dst, csc_type);

          bm_image_get_byte_size(dst, image_byte_size);

          byte_size = width * height * 3;
          unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
          memset(output_ptr, 0, sizeof(byte_size));
          void* out_ptr[1] = {(void*)output_ptr};
          ret = bm_image_copy_device_to_host(dst, (void **)out_ptr);

          FILE *fp_dst = fopen(dst_name, "wb");
          if (fwrite((void *)output_ptr, 1, byte_size, fp_dst) < (unsigned int)byte_size){
              printf("file size is less than %d required bytes\n", byte_size);
          };
          fclose(fp_dst);

          bm_image_destroy(&src);
          bm_image_destroy(&dst);
          free(input_data);
          free(output_ptr);

          bm_dev_free(handle);
          return 0;
      }