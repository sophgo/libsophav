
bmcv_ive_bersen
------------------------------

| 【描述】

| 该 API 使用ive硬件资源,  实现利用 Bernsen 二值法，对图像进行二值化求解，实现图像的二值化。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_bernsen(
        bm_handle_t           handle,
        bm_image              input,
        bm_image              output,
        bmcv_ive_bernsen_attr attr);

| 【参数】

.. list-table:: bmcv_ive_bersen 参数表
    :widths: 20 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - \output
      - 输出
      - 输出 bm_image 对象结构体, 不能为空。
    * - \attr
      - 输入
      - 控制参数结构体, 不能为空。

.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 16x16~1920x1080
    * - output
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input

| 【数据类型说明】

【说明】定义 Bernsen 阈值计算的不同方式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_bernsen_mode_e{
        BM_IVE_BERNSEN_NORMAL = 0x0,
        BM_IVE_BERNSEN_THRESH = 0x1,
        BM_IVE_BERNSEN_PAPER = 0x2,
    } bmcv_ive_bernsen_mode;

【说明】定义 Bernsen 二值法的控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_bernsen_attr_s{
        bmcv_ive_bernsen_mode en_mode;
        unsigned char u8_win_size;
        unsigned char u8_thr;
        unsigned char u8_contrast_threshold;
    } bmcv_ive_bernsen_attr;

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 输入输出图像的 width 都需要16对齐。


**示例代码**

    .. code-block:: c

      #include <stdio.h>
      #include <stdlib.h>
      #include <string.h>
      #include "bmcv_api_ext_c.h"
      #include <unistd.h>

      #define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

      int main() {
          int dev_id = 0;
          bmcv_ive_bernsen_mode mode = BM_IVE_BERNSEN_NORMAL;
          int winsize = 5;
          int height = 1080, width = 1920;
          bm_image_format_ext fmt = FORMAT_GRAY;
          char *src_name = "path/to/src";
          char *dst_name = "path/to/dst";
          bm_handle_t handle = NULL;
          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }

          bm_image src, dst;
          int stride[4];

          // config setting
          bmcv_ive_bernsen_attr attr;
          memset(&attr, 0, sizeof(bmcv_ive_bernsen_attr));

          attr.en_mode = mode;
          attr.u8_thr = 128;
          attr.u8_contrast_threshold = 15;
          attr.u8_win_size = winsize;

          // calc ive image stride && create bm image struct
          int data_size = 1;
          stride[0] = align_up(width, 16) * data_size;

          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, stride);

          ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);
          // read image data from input files
          int image_byte_size[4] = {0};
          bm_image_get_byte_size(src, image_byte_size);
          int byte_size  = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
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

          ret = bmcv_ive_bernsen(handle, src, dst, attr);

          unsigned char *ive_add_res = (unsigned char*)malloc(width * height * sizeof(unsigned char));
          memset(ive_add_res, 0, width * height * sizeof(unsigned char));

          ret = bm_image_copy_device_to_host(dst, (void **)&ive_add_res);
          FILE *fp = fopen(dst_name, "wb");
          fwrite((void *)ive_add_res, 1, width * height * sizeof(unsigned char), fp);
          fclose(fp);


          bm_image_destroy(&src);
          bm_image_destroy(&dst);

          return 0;
      }