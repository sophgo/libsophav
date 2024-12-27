bmcv_ive_sub
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建两张图像的相减操作。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_sub(
        bm_handle_t          handle,
        bm_image             input1,
        bm_image             input2,
        bm_image             output,
        bmcv_ive_sub_attr    attr);

| 【参数】

.. list-table:: bmcv_ive_sub 参数表
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
      - 相减操作对应模式的结构体，不能为空。


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

【注意】 output 数据类型格式也有可能是 DATA_TYPE_EXT_1N_BYTE_SIGNED。

| 【数据类型说明】

【说明】定义两张图像相减输出格式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_sub_mode_e{
        IVE_SUB_ABS = 0x0,
        IVE_SUB_SHIFT = 0x1,
        IVE_SUB_BUTT
    } bmcv_ive_sub_mode;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - IVE_SUB_ABS
      - 取差的绝对值。
    * - IVE_SUB_SHIFT
      - 将结果右移一位输出, 保留符号位。

【说明】定义两图像相减控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_sub_attr_s {
        bmcv_ive_sub_mode en_mode;
    } bmcv_ive_sub_attr;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - en_mode
      - MOD_SUB 下 两图像相减模式。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

* 输入输出图像的 width 都需要16对齐。

* MOD_SUB:

     - IVE_SUB_ABS:
        .. math::

           \begin{aligned}
            & I_{\text{out}}(x, y) = abs(I_{\text{src1}}(x, y) - I_{\text{src2}}(x, y))
           \end{aligned}

     输出格式是 DATA_TYPE_EXT_1N_BYTE。

     - IVE_SUB_SHIFT:
        .. math::

           \begin{aligned}
            & I_{\text{out}}(x, y) = (I_{\text{src1}}(x, y) - I_{\text{src2}}(x, y)) \gg 1
           \end{aligned}

     输出格式是 DATA_TYPE_EXT_1N_BYTE_SIGNED。


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
          bmcv_ive_sub_mode sub_mode = IVE_SUB_ABS; /* IVE_SUB_MODE_ABS */
          bm_image_format_ext src_fmt = FORMAT_GRAY, dst_fmt = FORMAT_GRAY;
          char *src1_name = "path/to/src1", *src2_name = "path/to/src2";
          char *dst_name = "path/to/dst";
          bm_handle_t handle = NULL;
          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }

          bmcv_ive_sub_attr sub_attr;
          bm_image src1, src2, dst;
          int src_stride[4];
          int dst_stride[4];

          // set ive sub params
          memset(&sub_attr, 0, sizeof(bmcv_ive_sub_attr));
          sub_attr.en_mode = sub_mode;

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

          ret = bmcv_ive_sub(handle, src1, src2, dst, sub_attr);

          unsigned char* ive_sub_res = (unsigned char*)malloc(width * height * sizeof(unsigned char));
          memset(ive_sub_res, 0, width * height * sizeof(unsigned char));

          ret = bm_image_copy_device_to_host(dst, (void **)&ive_sub_res);

          FILE *fp = fopen(dst_name, "wb");
          fwrite((void *)ive_sub_res, 1, width * height * sizeof(unsigned char), fp);
          fclose(fp);

          free(input_data);
          free(ive_sub_res);

          bm_image_destroy(&src1);
          bm_image_destroy(&src2);
          bm_image_destroy(&dst);

          bm_dev_free(handle);
          return 0;
      }