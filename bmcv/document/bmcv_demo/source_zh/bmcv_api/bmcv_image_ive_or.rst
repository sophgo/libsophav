bmcv_ive_or
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建两张图像的相或操作。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_or(
        bm_handle_t          handle,
        bm_image             input1,
        bm_image             input2,
        bm_image             output);

| 【参数】

.. list-table:: bmcv_ive_or 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input1
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - \input2
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - \output
      - 输出
      - 输出 bm_image 对象结构体, 不能为空。


.. list-table::
    :widths: 22 40 64 42

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input1
      - GRAY

        GRAY 二值图
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - input2
      - GRAY

        GRAY 二值图
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input1
    * - output
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 Input1


| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

* 输入输出图像的 width 都需要16对齐。

* MOD_OR:
    .. math::

       \begin{aligned}
        & I_{\text{out}}(x, y) = I_{\text{src1}}(x, y) | I_{\text{src2}}(x, y)
      \end{aligned}

    其中，:math:`I_{\text{src1}}(x, y)` 对应 input1, :math:`I_{\text{src2}}(x, y)` 对应 input2, :math:`I_{\text{out}}(x, y)` 对应 output。

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
          int height = 1080, width = 1920;
          bm_image_format_ext src_fmt = FORMAT_GRAY, dst_fmt = FORMAT_GRAY;
          char *src1_name = "path/to/src1", *src2_name = "path/to/src2";
          char *dst_name = "path/to/dst";
          bm_handle_t handle = NULL;
          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }
          bm_image src1, src2, dst;
          int src_stride[4];
          int dst_stride[4];

          // calc ive image stride
          int data_size = 1;
          src_stride[0] = align_up(width, 16) * data_size;
          dst_stride[0] = align_up(width, 16) * data_size;



          // create bm image struct
          bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src1, src_stride);
          bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src2, src_stride);
          bm_image_create(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);

          // alloc bm image memory
          ret = bm_image_alloc_dev_mem(src1, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(src2, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

          // read image data from input files
          int byte_size;
          unsigned char *input_data;
          int image_byte_size[4] = {0};
          char *filename[] = {src1_name, src2_name};
          bm_image src_images[] = {src1, src2};
          for (int i = 0; i < 2; i++) {
              bm_image_get_byte_size(src_images[i], image_byte_size);
              byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
              input_data = (unsigned char *)malloc(byte_size);
              FILE *fp_src = fopen(filename[i], "rb");
              if (fread((void *)input_data, 1, byte_size, fp_src) < (unsigned int)byte_size) {
              printf("file size is less than required bytes%d\n", byte_size);
              };
              fclose(fp_src);
              void* in_ptr[4] = {(void *)input_data,
                                  (void *)((unsigned char*)input_data + image_byte_size[0]),
                                  (void *)((unsigned char*)input_data + image_byte_size[0] + image_byte_size[1]),
                                  (void *)((unsigned char*)input_data + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};
              bm_image_copy_host_to_device(src_images[i], in_ptr);
          }

          ret = bmcv_ive_or(handle, src1, src2, dst);


          unsigned char *ive_res = (unsigned char*) malloc(width * height * sizeof(unsigned char));
          memset(ive_res, 0, width * height * sizeof(unsigned char));

          ret = bm_image_copy_device_to_host(dst, (void**)&ive_res);

          FILE *fp = fopen(dst_name, "wb");
          fwrite((void *)ive_res, 1, width * height * sizeof(unsigned char), fp);
          fclose(fp);

          free(input_data);
          free(ive_res);

          bm_image_destroy(&src1);
          bm_image_destroy(&src2);
          bm_image_destroy(&dst);

          bm_dev_free(handle);
          return 0;
      }