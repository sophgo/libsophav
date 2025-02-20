bmcv_ive_filter
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建 5x5 模板滤波任务, 通过配置不同的模板系数, 可以实现不同的滤波。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_filter(
        bm_handle_t            handle,
        bm_image               input,
        bm_image               output,
        bmcv_ive_filter_ctrl   filter_attr);

| 【参数】

.. list-table:: bmcv_ive_filter 参数表
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
      - 输出 bm_image 对象结构体，不能为空, 宽、高同 input。
    * - filter_attr
      - 输入
      - 控制信息结构体, 不能为空。

.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY

        NV21

        NV61
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - output
      - 同input
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input

| 【数据类型说明】

【说明】定义模板滤波控制信息。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_filter_ctrl_s{
        signed char as8_mask[25];
        unsigned char u8_norm;
    } bmcv_ive_filter_ctrl;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - as8_mask
      - 5x5 模板系数，外围系数设为0，可实现 3x3 模板滤波。
    * - u8_norm
      - 归一化参数，取值范围: [0, 13]。

【注意】

* 通过配置不同的模板系数可以达到不同的滤波效果。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 当输入数据 为 FORMAT_NV21、FORMAT_NV61 类型时, 要求输出数据跨度一致。

3. Filter 计算公式:

  .. math::

    I_{\text{out}}(x, y) = \left\{ \sum_{-2 < j < 2} \sum_{-2 < i < 2} I(x+i, y+j) \cdot \text{coef}(i, j) \right\} \gg \text{norm}(x)

4. 经典高斯模板如下， 其中 norm_u8 分别等于4、8、8:

  .. raw:: latex

   \begin{align*}
   \hspace{2em} % 控制间隔
   \begin{bmatrix}
      0 & 0 & 0 & 0 & 0 \\
      0 & 1 & 2 & 1 & 0 \\
      0 & 2 & 4 & 2 & 0 \\
      0 & 1 & 2 & 1 & 0 \\
      0 & 0 & 0 & 0 & 0 \\
   \end{bmatrix}
   \hspace{5em} % 控制间隔
   \begin{bmatrix}
      1 & 2 & 3 & 2 & 1 \\
      2 & 5 & 6 & 5 & 2 \\
      3 & 6 & 8 & 6 & 3 \\
      2 & 5 & 6 & 5 & 2 \\
      1 & 2 & 3 & 2 & 1 \\
   \end{bmatrix}
   \times 3
   \hspace{3.5em} % 控制间隔
   \begin{bmatrix}
      1 & 4 & 7 & 4 & 1 \\
      4 & 16 & 26 & 16 & 4 \\
      7 & 26 & 41 & 26 & 7 \\
      4 & 16 & 26 & 16 & 4 \\
      1 & 4 & 7 & 4 & 1 \\
   \end{bmatrix}
   \end{align*}


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
          unsigned char u8Norm = 4;
          int height = 1080, width = 1920;
          bm_image_format_ext fmt = FORMAT_GRAY; // 14 4 6
          char *src_name = "path/to/src";
          char *dst_name = "path/to/dst";
          bm_handle_t handle = NULL;

          signed char arr3by3[25] = { 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0, 2, 4,
                              2, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0 };

          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }
          bm_image src, dst;
          int stride[4];

          bmcv_ive_filter_ctrl filterAttr;
          memcpy(filterAttr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char));
          filterAttr.u8_norm = u8Norm;

          // calc ive image stride && create bm image struct
          int data_size = 1;
          stride[0] = align_up(width, 16) * data_size;

          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, stride);

          ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

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
          ret = bmcv_ive_filter(handle, src, dst, filterAttr);

          bm_image_get_byte_size(dst, image_byte_size);
          byte_size = image_byte_size[0] + image_byte_size[1] + image_byte_size[2] + image_byte_size[3];
          unsigned char* output_ptr = (unsigned char *)malloc(byte_size);
          memset(output_ptr, 0, sizeof(byte_size));

          void* out_ptr[4] = {(void*)output_ptr,
                              (void*)((char*)output_ptr + image_byte_size[0]),
                              (void*)((char*)output_ptr + image_byte_size[0] + image_byte_size[1]),
                              (void*)((char*)output_ptr + image_byte_size[0] + image_byte_size[1] + image_byte_size[2])};

          ret = bm_image_copy_device_to_host(dst, (void **)out_ptr);

          FILE *filter_fp = fopen(dst_name, "wb");
          fwrite((void *)output_ptr, 1, width * height, filter_fp);
          fclose(filter_fp);

          free(input_data);
          free(output_ptr);

          bm_image_destroy(&src);
          bm_image_destroy(&dst);

          bm_dev_free(handle);
          return 0;
      }