bmcv_ive_integ
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建灰度图像的积分图计算任务，积分像素值等于灰度图的左上角与当前点所围成的矩形区域内所有像素点灰度值之和/平方和。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_integ(
        bm_handle_t        handle,
        bm_image           input,
        bm_device_mem_t    output,
        bmcv_ive_integ_ctrl_s  integ_attr);

| 【参数】

.. list-table:: bmcv_ive_integ 参数表
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
      - 输入 bm_device_mem_t 对象数据结构体, 宽、高同 input。
    * - integ_attr
      - 输入
      - 积分图计算控制参数结构体，不能为空。

.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 32x16~1920x1080

| 【数据类型说明】

【说明】定义积分图输出控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum _ive_integ_out_ctrl_e {
        IVE_INTEG_MODE_COMBINE = 0x0,
        IVE_INTEG_MODE_SUM = 0x1,
        IVE_INTEG_MODE_SQSUM = 0x2,
        IVE_INTEG_MODE_BUTT
    } ive_integ_out_ctrl_e;

.. list-table::
    :widths: 80 100

    * - **成员名称**
      - **描述**
    * - IVE_INTEG_MODE_COMBINE
      - 和、平方和积分图组合输出。
    * - IVE_INTEG_MODE_SUM
      - 仅和积分图输出。
    * - IVE_INTEG_MODE_SQSUM
      - 仅平方和积分图输出。

【说明】定义积分图计算控制参数

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_integ_ctrl_t{
        ive_integ_out_ctrl_e en_out_ctrl;
    } bmcv_ive_integ_ctrl_s;

.. list-table::
    :widths: 80 100

    * - **成员名称**
      - **描述**
    * - en_out_ctrl
      - 积分图输出控制参数。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入图像的 width 都需要16对齐。

2. IVE_INTEG_MODE_COMBINE, 组合输出模式, 输出图像数据类型必须为u64, 计算公式如下:

  .. math::

   \begin{aligned}
      & I_{\text{sum}}(x, y) = \sum_{i \geq 0}^{i \leq x} \sum_{j \geq 0}^{j \leq y} I(i, j) \\
      & I_{\text{sq}}(x, y)  = \sum_{i \geq 0}^{i \leq x} \sum_{j \geq 0}^{j \leq y} (I(i, j) \cdot I(i, j)) \\
      & I_{\text{out}}(x, y) = (I_{\text{sq}}(x, y) \ll 28 \mid I_{\text{sum}}(x, y) \& 0xFFFFFFF)
   \end{aligned}

3. IVE_INTEG_MODE_SUM, 仅和积分图输出模式, 输出图像数据类型必须为u32。

  .. math::

   \begin{aligned}
      & I_{\text{sum}}(x, y) = \sum_{i \geq 0}^{i \leq x} \sum_{j \geq 0}^{j \leq y} I(i, j) \\
      & I_{\text{out}}(x, y) = I_{\text{sum}}(x, y)
   \end{aligned}

4. IVE_INTEG_MODE_SQSUM, 仅平方和积分图输出模式, 输出图像数据类型必须为u32。

  .. math::

   \begin{aligned}
      & I_{\text{sq}}(x, y)  = \sum_{i \geq 0}^{i \leq x} \sum_{j \geq 0}^{j \leq y} (I(i, j) \cdot I(i, j)) \\
      & I_{\text{out}}(x, y) = I_{\text{sq}}(x, y)
   \end{aligned}


  其中，:math:`I(x, y)` 对应 input, :math:`I_{\text{sum}}` 对应 output。


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
          ive_integ_out_ctrl_e integ_mode = 0;
          char* src_name = "path/to/src", *dst_name = "path/to/dst";
          bm_handle_t handle = NULL;
          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }

          bmcv_ive_integ_ctrl_s integ_attr;
          bm_image src;
          bm_device_mem_t dst;
          int src_stride[4];

          // calc ive image stride && create bm image struct
          int data_size = 1;
          src_stride[0] = align_up(width, 16) * data_size;

          bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
          ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
          if (ret != BM_SUCCESS) {
              printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
              exit(-1);
          }

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
          data_size = sizeof(unsigned int);

          ret = bm_malloc_device_byte(handle, &dst, height * width * data_size);
          integ_attr.en_out_ctrl = integ_mode;

          ret = bmcv_ive_integ(handle, src, dst, integ_attr);

          unsigned int *dst_intg_u32 =  malloc(width * height * data_size);
          ret = bm_memcpy_d2s(handle, dst_intg_u32, dst);
          FILE *intg_result_fp = fopen(dst_name, "wb");
          fwrite((void *)dst_intg_u32, data_size, width * height, intg_result_fp);
          fclose(intg_result_fp);

          free(dst_intg_u32);

          bm_image_destroy(&src);
          bm_free_device(handle, dst);

          bm_dev_free(handle);
          return 0;
      }