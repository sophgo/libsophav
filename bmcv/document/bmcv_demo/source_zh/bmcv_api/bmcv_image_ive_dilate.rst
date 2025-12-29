bmcv_ive_dilate
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建二值图像 5x5 模板膨胀任务，将模板覆盖的区域中的最大像素值赋给参考点。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_dilate(
        bm_handle_t           handle,
        bm_image              input,
        bm_image              output,
        unsigned char         dilate_mask[25]);

| 【参数】

.. list-table:: bmcv_ive_dilate 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input
      - 输入
      - 输入 bm_image 对象结构体，不能为空。
    * - \output
      - 输出
      - 输出 bm_image 对象结构体，不能为空，宽、高同 input。
    * - \dilate_mask
      - 输入
      - 5x5 膨胀模板系数数组，不能为空，取值范围: 0 或 255。

| 【数据类型说明】

.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY 的二值图
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - output
      - GRAY 的二值图
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 模板系数只能为 0 或 255。

3. 接口可以处理灰度图的输入，对于处理的结果不可保证，所以不提倡，但是也可以尝试。

4. 模板样例

  .. raw:: latex

   \begin{align*}
   \begin{bmatrix}
      0 & 0 & 0 & 0 & 0 \\
      0 & 0 & 255 & 0 & 0 \\
      0 & 255 & 255 & 255 & 0 \\
      0 & 0 & 255 & 0 & 0 \\
      0 & 0 & 0 & 0 & 0 \\
   \end{bmatrix}
   \hspace{60pt} % 控制间隔
   \begin{bmatrix}
      0 & 0 & 255 & 0 & 0 \\
      0 & 0 & 255 & 0 & 0 \\
      255 & 255 & 255 & 255 & 255 \\
      0 & 0 & 255 & 0 & 0 \\
      0 & 0 & 255 & 0 & 0 \\
   \end{bmatrix}
   \end{align*}


  .. raw:: latex

   \begin{align*}
   \begin{bmatrix}
      0 & 0 & 0 & 0 & 0 \\
      0 & 255 & 255 & 255 & 0 \\
      0 & 255 & 255 & 255 & 0 \\
      0 & 255 & 255 & 255 & 0 \\
      0 & 0 & 0 & 0 & 0 \\
   \end{bmatrix}
   \hspace{60pt} % 控制间隔
   \begin{bmatrix}
      0 & 0 & 255 & 0 & 0 \\
      0 & 255 & 255 & 255 & 0 \\
      255 & 255 & 255 & 255 & 255 \\
      0 & 255 & 255 & 255 & 0 \\
      0 & 0 & 255 & 0 & 0 \\
   \end{bmatrix}
   \end{align*}

  .. raw:: latex

   \begin{align*}
   \begin{bmatrix}
      0 & 255 & 255 & 255 & 0 \\
      255 & 255 & 255 & 255 & 255 \\
      255 & 255 & 255 & 255 & 255 \\
      255 & 255 & 255 & 255 & 255 \\
      0 & 255 & 255 & 255 & 0 \\
   \end{bmatrix}
   \hspace{60pt} % 控制间隔
   \begin{bmatrix}
      255 & 255 & 255 & 255 & 255 \\
      255 & 255 & 255 & 255 & 255 \\
      255 & 255 & 255 & 255 & 255 \\
      255 & 255 & 255 & 255 & 255 \\
      255 & 255 & 255 & 255 & 255 \\
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
          int height = 1080, width = 1920;
          bm_image_format_ext fmt = FORMAT_GRAY;
          char *src_name = "path/to/src", *dst_name = "path/to/dst";

          unsigned char arr3by3[25] = { 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 255, 255,
                          255, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0 };

          bm_handle_t handle = NULL;
          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }

          bm_image src, dst;
          int stride[4];

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



          ret = bmcv_ive_dilate(handle, src, dst, arr3by3);

          unsigned char* ive_dilate_res = malloc(width * height * sizeof(unsigned char));
          memset(ive_dilate_res, 0, width * height * sizeof(unsigned char));

          ret = bm_image_copy_device_to_host(dst, (void **)&ive_dilate_res);

          FILE *ive_result_fp = fopen(dst_name, "wb");
          fwrite((void *)ive_dilate_res, 1, width * height, ive_result_fp);
          fclose(ive_result_fp);

          free(input_data);
          free(ive_dilate_res);

          bm_image_destroy(&src);
          bm_image_destroy(&dst);


          bm_dev_free(handle);
          return ret;
      }