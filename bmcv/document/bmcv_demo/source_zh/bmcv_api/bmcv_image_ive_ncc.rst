bmcv_ive_ncc
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建两相同分辨率灰度图像的归一化互相关系数计算任务。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_ncc(
    bm_handle_t       handle,
    bm_image          input1,
    bm_image          input2,
    bm_device_mem_t   output);

| 【参数】

.. list-table:: bmcv_ive_ncc 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - input1
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - input2
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - output
      - 输出
      - 输出 数据 对象结构体, 不能为空, 内存至少需要配置: sizeof(bmcv_ive_ncc_dst_mem_t)。

.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input1
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 32x32~1920x1080
    * - input2
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input1
    * - output
      - --
      - --
      - --

| 【数据类型说明】

【说明】定义LBP计算的比较模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct _bmcv_ive_ncc_dst_mem_s{
        unsigned long long u64_numerator;
        unsigned long long u64_quad_sum1;
        unsigned long long u64_quad_sum2;
        unsigned char u8_reserved[8];
    }bmcv_ive_ncc_dst_mem_t;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - u64_numerator
      - :math:`\sum_{i = 1}^{w} \sum_{j = 1}^{h} (I_{\text{src1}}(i, j) * I_{\text{src2}}(i, j))`
    * - u64_quad_sum1
      - :math:`\sum_{i = 1}^{w} \sum_{j = 1}^{h} I_{\text{src1}}^{2}(i, j)`
    * - u64_quad_sum2
      - :math:`\sum_{i = 1}^{w} \sum_{j = 1}^{h} I_{\text{src2}}^{2}(i, j)`
    * - u8_reserved
      - 保留字段。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

* 输入图像的 width 都需要16对齐。

* 计算公式如下：
   .. math::

       & NCC(I_{\text{src1}}, I_{\text{src2}}) =
         \frac{\sum_{i = 1}^{w} \sum_{j = 1}^{h} (I_{\text{src1}}(i, j) * I_{\text{src2}}(i, j))}
         {\sqrt{\sum_{i = 1}^{w} \sum_{j = 1}^{h} I_{\text{src1}}^{2}(i, j)}
          \sqrt{\sum_{i = 1}^{w} \sum_{j = 1}^{h} I_{\text{src2}}^{2}(i, j)}}


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
          int height = 288, width = 352;;
          bm_image_format_ext src_fmt = FORMAT_GRAY;
          char* src1_name = "path/to/src1", *src2_name = "path/to/src2";
          char* dst_name = "/path/to/dst";
          bm_handle_t handle = NULL;
          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }

          bm_image src1, src2;
          bm_device_mem_t dst;
          int src_stride[4];

          // calc ive image stride && create bm image struct
          int data_size = 1;
          src_stride[0] = align_up(width, 16) * data_size;

          bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src1, src_stride);
          bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src2, src_stride);
          ret = bm_image_alloc_dev_mem(src1, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(src2, BMCV_HEAP1_ID);

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

          int data_len = sizeof(bmcv_ive_ncc_dst_mem_t);

          ret = bm_malloc_device_byte(handle, &dst, data_len);

          ret = bmcv_ive_ncc(handle, src1, src2, dst);

          unsigned long long *ncc_result = malloc(data_len);
          ret = bm_memcpy_d2s(handle, ncc_result, dst);
          unsigned long long *numerator = ncc_result;
          unsigned long long *quadSum1 = ncc_result + 1;
          unsigned long long *quadSum2 = quadSum1 + 1;
          float fr = (float)((double)*numerator / (sqrt((double)*quadSum1) * sqrt((double)*quadSum2)));
          printf("bmcv ive NCC value is %f \n", fr);

          FILE *ncc_result_fp = fopen(dst_name, "wb");
          fwrite((void *)ncc_result, 1, data_len, ncc_result_fp);
          fclose(ncc_result_fp);

          free(input_data);
          free(ncc_result);

          bm_free_device(handle, dst);
          bm_dev_free(handle);
          return 0;
      }