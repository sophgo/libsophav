bmcv_ive_add
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建两张图像的加权相加操作。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_add(
        bm_handle_t          handle,
        bm_image             input1,
        bm_image             input2,
        bm_image             output,
        bmcv_ive_add_attr    attr);

| 【参数】

.. list-table:: bmcv_ive_add 参数表
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
    * - \attr
      - 输入
      - 相加操作的需要的加权系数对应的结构体，不能为空。


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


【说明】定义两图像的加权加控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_add_attr_s {
        unsigned short param_x;
        unsigned short param_y;
    } bmcv_ive_add_attr;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - param_x
      - 加权加 "xA + yB" 中的权重x。
    * - param_y
      - 加权加 "xA + yB" 中的权重y。


| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

* 输入输出图像的 width 都需要16对齐。

* MOD_ADD:
    .. math::

       \begin{aligned}
        & I_{\text{out}}(i, j) = x * I_{1}(i, j) + y * I_{2}(i, j)
      \end{aligned}

    其中，:math:`I_{1}(i, j)` 对应 input1, :math:`I_{2}(i, j)` 对应 input2, :math:`x, y` 分别对应 bmcv_ive_add_attr 中的 param_x 与 param_y。

    若定点化前, :math:`x` 和 :math:`y` 满足 :math:`x + y > 1`, 则当前计算结果超过 8bit 取 低 8bit 作为最终结果。


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
          unsigned short x = 19584, y = 45952; /* x + y = 65536 */
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


          bmcv_ive_add_attr add_attr;
          memset(&add_attr, 0, sizeof(bmcv_ive_add_attr));

          add_attr.param_x = x;
          add_attr.param_y = y;
          // calc ive image stride && create bm image struct
          int data_size = 1;
          src_stride[0] = align_up(width, 16) * data_size;
          dst_stride[0] = align_up(width, 16) * data_size;

          bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src1, src_stride);
          bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src2, src_stride);
          bm_image_create(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);

          ret = bm_image_alloc_dev_mem(src1, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(src2, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

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

          ret = bmcv_ive_add(handle, src1, src2, dst, add_attr);

          unsigned char *ive_add_res = (unsigned char*)malloc(width * height * sizeof(unsigned char));
          memset(ive_add_res, 0, width * height * sizeof(unsigned char));

          ret = bm_image_copy_device_to_host(dst, (void **)&ive_add_res);
          FILE *fp = fopen(dst_name, "wb");
          fwrite((void *)ive_add_res, 1, width * height * sizeof(unsigned char), fp);
          fclose(fp);

          free(input_data);
          free(ive_add_res);

          bm_image_destroy(&src1);
          bm_image_destroy(&src2);
          bm_image_destroy(&dst);


          bm_dev_free(handle);

          return 0;
      }