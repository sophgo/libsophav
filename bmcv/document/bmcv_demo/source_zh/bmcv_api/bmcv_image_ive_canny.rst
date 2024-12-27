bmcv_ive_canny
------------------------------

| 【描述】

| 该 API 使用ive硬件资源的 ccl 功能, 计算 canny 边缘图。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_canny(
        bm_handle_t                  handle,
        bm_image                     input,
        bm_device_mem_t              output_edge,
        bmcv_ive_canny_hys_edge_ctrl   canny_hys_edge_attr);

| 【参数】

.. list-table:: bmcv_ive_canny 参数表
    :widths: 20 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input
      - 输入
      - 输入 bm_image 对象结构体，不能为空。
    * - \output_edge
      - 输出
      - 输出 bm_device_mem_t 对象结构体，不能为空。
    * - \canny_hys_edge_attr
      - 输入
      - 控制参数结构体，不能为空。

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

| 【数据类型说明】

【说明】定义 Canny 边缘前半部分计算任务的控制参数 。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_canny_hys_edge_ctrl_s{
        bm_device_mem_t  st_mem;
        unsigned short u16_low_thr;
        unsigned short u16_high_thr;
        signed char as8_mask[25];
    } bmcv_ive_canny_hys_edge_ctrl;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - stmem
      - 辅助内存。
    * - u16_low_thr
      - 低阈值，取值范围：[0, 255]。
    * - u16_high_thr
      - 高阈值，取值范围: [u16LowThr, 255]。
    * - as8_mask
      - 用于计算梯度的参数模板。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 输入输出图像的 width 都需要16对齐。

2. canny_hys_edge_attr.st_mem 至少需要分配的内存大小:

   canny_hys_edge_attr.st_mem.size = input_stride * 3 * input.height。



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
          int thrSize = 0; //0 -> 3x3; 1 -> 5x5
          int height = 1080, width = 1920;
          bm_image_format_ext fmt = FORMAT_GRAY;
          char *src_name = "path/to/src";
          char *dstEdge_name = "path/to/dst";
          bm_handle_t handle = NULL;

          signed char arr5by5[25] = { -1, -2, 0, 2, 1, -4, -8, 0, 8, 4, -6, -12, 0,
                          12, 6, -4, -8, 0, 8, 4, -1, -2, 0, 2, 1 };
          signed char arr3by3[25] = { 0, 0, 0, 0,  0, 0, -1, 0, 1, 0, 0, -2, 0,
                          2, 0, 0, -1, 0, 1, 0, 0, 0, 0, 0, 0 };
          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }
          bm_image src;
          bm_device_mem_t canny_stmem;
          int stride[4];

          int stmem_len = width * height * 4 * (sizeof(unsigned short) + sizeof(unsigned char));

          unsigned char *edge_res = malloc(width * height * sizeof(unsigned char));
          memset(edge_res, 0, width * height * sizeof(unsigned char));

          bmcv_ive_canny_hys_edge_ctrl cannyHysEdgeAttr;
          memset(&cannyHysEdgeAttr, 0, sizeof(bmcv_ive_canny_hys_edge_ctrl));
          cannyHysEdgeAttr.u16_low_thr = (thrSize == 0) ? 42 : 108;
          cannyHysEdgeAttr.u16_high_thr = 3 * cannyHysEdgeAttr.u16_low_thr;
          (thrSize == 0) ? memcpy(cannyHysEdgeAttr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char)) :
                          memcpy(cannyHysEdgeAttr.as8_mask, arr5by5, 5 * 5 * sizeof(signed char));

          // calc ive image stride && create bm image struct
          int data_size = 1;
          stride[0] = align_up(width, 16) * data_size;
          bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &src, stride);

          ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
          ret = bm_malloc_device_byte(handle, &canny_stmem, stmem_len);

          // bm_ive_read_bin(src, src_name);
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
          cannyHysEdgeAttr.st_mem = canny_stmem;

          ret = bmcv_ive_canny(handle, src, bm_mem_from_system((void *)edge_res), cannyHysEdgeAttr);

          FILE *edge_fp = fopen(dstEdge_name, "wb");
          fwrite((void *)edge_res, 1, width * height, edge_fp);
          fclose(edge_fp);

          free(input_data);
          free(edge_res);
          bm_image_destroy(&src);
          bm_free_device(handle, cannyHysEdgeAttr.st_mem);

          bm_dev_free(handle);
          return 0;
      }