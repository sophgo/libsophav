bmcv_ive_hist
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建灰度图像，遍历图像像素值实现统计图像相同像素值出现的次数，构建直方图。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_hist(
        bm_handle_t      handle,
        bm_image         input,
        bm_device_mem_t  output);

| 【参数】

.. list-table:: bmcv_ive_hist 参数表
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
      - 输入 bm_device_mem_t 对象数据结构体, 不能为空, 内存至少配置 1024 字节，输出使用u32类型代表0-255每个数字的统计值。

.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入图像的 width 都需要16对齐。

2. 计算公式如下:

    .. math::

      I(x) = \sum_{i} \sum_{j} \begin{cases}
         1 & \text{if } I(i, j) = x \\
         0 & \text{otherwise}
      \end{cases} \quad \text{for } x = 0 \ldots 255


**示例代码**

    .. code-block:: c

      #include <stdio.h>
      #include <stdlib.h>
      #include <string.h>
      #include "bmcv_api_ext_c.h"
      #include <unistd.h>

      #define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

      int main(){
          int dev_id = 0;
          int height = 1080, width = 1920;
          bm_image_format_ext src_fmt = FORMAT_GRAY;
          char * src_name = "path/to/src", *dst_name = "path/to/dst";
          bm_handle_t handle = NULL;
          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }

          bm_image src;
          bm_device_mem_t dst;
          int src_stride[4];

          // calc ive image stride && create bm image struct
          int data_size = 1;
          src_stride[0] = align_up(width, 16) * data_size;
          bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
          ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);

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
          // create result
          ret = bm_malloc_device_byte(handle, &dst, 1024);
          ret = bmcv_ive_hist(handle, src, dst);

          unsigned char *dst_hist_result = malloc(dst.size);

          ret = bm_memcpy_d2s(handle, dst_hist_result, dst);

          FILE *hist_result_fp = fopen(dst_name, "wb");
          fwrite((void *)dst_hist_result, 1, dst.size, hist_result_fp);
          fclose(hist_result_fp);

          free(input_data);
          free(dst_hist_result);

          bm_image_destroy(&src);
          bm_free_device(handle, dst);

          bm_dev_free(handle);

          return 0;
      }