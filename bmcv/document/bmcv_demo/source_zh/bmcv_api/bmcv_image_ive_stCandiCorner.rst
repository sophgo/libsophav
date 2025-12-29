bmcv_ive_stcandicorner
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 完成灰度图像 Shi-Tomasi-like 角点（两条边缘的交点）计算，角点在任意一个方向上做微小移动，都会引起该区域的梯度图的方向和幅值发生很大变化。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_stcandicorner(
    bm_handle_t                 handle,
    bm_image                    input,
    bm_image                    output,
    bmcv_ive_stcandicorner_attr   stcandicorner_attr);

| 【参数】

.. list-table:: bmcv_ive_stcandicorner 参数表
    :widths: 25 15 35

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
      - 输出 bm_image 对象数据结构体, 候选角点响应值图像指针, 不能为空, 宽、高同 input。
    * - \stcandicorner_attr
      - 输入
      - Shi-Tomas-like 候选角点计算控制参数结构体, 不能为空。

.. list-table::
    :widths: 25 18 62 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64～1920x1080
    * - output
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input

【数据类型说明】

【说明】定义 Shi-Tomas-like 候选角点计算控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_stcandicorner_attr_s{
        bm_device_mem_t st_mem;
        unsigned char   u0q8_quality_level;
    } bmcv_ive_stcandicorner_attr;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - st_mem
      - 内存配置大小见 bmcv_image_ive_stcandicorner 的注意。
    * - u0q8_quality_level
      - ShiTomasi 角点质量控制参数，角点响应值小于 u0q8_quality_level * 最大角点响应值 的点将直接被确认为非角点。

        取值范围: [1, 255], 参考取值: 25。

【说明】定义 Shi-Tomas-like 角点计算时最大角点响应值结构体。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_st_max_eig_s{
        unsigned short u16_max_eig;
        unsigned char  u8_reserved[14];
    } bmcv_ive_st_max_eig;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - u16_max_eig
      - 最大角点响应值。
    * - u8_reserved
      - 保留位。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 与 OpenCV 中 ShiTomas 角点计算原理类似。

3. stcandicorner_attr.st_mem 至少需要开辟的内存大小:

     stcandicorner_attr.st_mem.size = 4 * input_stride * input.height + sizoef(bm_ive_st_max_eig)。

4. 该任务完成后, 得到的并不是真正的角点, 还需要进行下一步计算。

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
          int u0q8QualityLevel = 25;
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

          bmcv_ive_stcandicorner_attr stCandiCorner_attr;
          memset(&stCandiCorner_attr, 0, sizeof(bmcv_ive_stcandicorner_attr));

          // calc ive image stride && create bm image struct
          int data_size = 1;
          stride[0] = align_up(width, 16) * data_size;
          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);
          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &dst, stride);

          ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

          int attr_len = 4 * height * stride[0] + sizeof(bmcv_ive_st_max_eig);
          ret = bm_malloc_device_byte(handle, &stCandiCorner_attr.st_mem, attr_len * sizeof(unsigned char));
          stCandiCorner_attr.u0q8_quality_level = u0q8QualityLevel;

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

          ret = bmcv_ive_stcandicorner(handle, src, dst, stCandiCorner_attr);

          unsigned char *ive_res = (unsigned char *)malloc(width * height * sizeof(unsigned char));
          memset(ive_res, 0, width * height * sizeof(unsigned char));

          ret = bm_image_copy_device_to_host(dst, (void**)&ive_res);

          FILE *ive_fp = fopen(dst_name, "wb");
          fwrite((void *)ive_res, 1, width * height, ive_fp);
          fclose(ive_fp);

          free(input_data);
          free(ive_res);

          bm_image_destroy(&src);
          bm_image_destroy(&dst);
          bm_free_device(handle, stCandiCorner_attr.st_mem);

          bm_dev_free(handle);
          return 0;
      }