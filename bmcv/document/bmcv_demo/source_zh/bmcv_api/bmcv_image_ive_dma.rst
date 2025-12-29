bmcv_ive_dma
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建直接内存访问任务，支持快速拷贝、 间隔拷贝；可实现数据从一块内存快速拷贝到另一块内存，或者从一块内存有规律的拷贝一些数据到另一块内存。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_dma(
        bm_handle_t                      handle,
        bm_image                         input,
        bm_image                         output,
        bmcv_ive_dma_mode                dma_mode,
        bmcv_ive_interval_dma_attr *     attr);

| 【参数】

.. list-table:: bmcv_ive_dma 参数表
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
      - 输出 bm_image 对象结构体，不能为空。
    * - dma_mode
      - 输入
      - 定义 DMA copy操作模式。
    * - attr
      - 输入
      - DMA 在间隔拷贝模式下需要的控制参数结构体，直接拷贝可以为空。

【分辨率】：32x1 ~ 1920x1080

| 【数据类型说明】

【说明】定义 DMA 操作模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_dma_mode_e {
        IVE_DMA_DIRECT_COPY = 0x0,
        IVE_DMA_INTERVAL_COPY = 0x1
    } bmcv_ive_dma_mode;

.. list-table::
    :widths: 80 100

    * - **成员名称**
      - **描述**
    * - IVE_DMA_DIRECT_COPY
      - 直接快速拷贝模式。
    * - IVE_DMA_INTERVAL_COPY
      - 间隔拷贝模式。

【说明】定义 DMA 间隔拷贝需要的控制信息。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_interval_dma_attr_s {
        unsigned char hor_seg_size;
        unsigned char elem_size;
        unsigned char ver_seg_rows;
    } bmcv_ive_interval_dma_attr;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**

    * - hor_seg_size
      - 仅间隔拷贝模式使用, 水平方向将源图像一行分割的段大小。

        取值范围：{2, 3, 4, 8, 16}。
    * - elem_size
      - 仅间隔拷贝模式使用, 分割的每一段中前 elem_size 为有效拷贝字段。

        取值范围：[1, hor_seg_size - 1]。
    * - ver_seg_rows
      - 仅间隔拷贝模式使用, 将每 ver_seg_rows 行中第一行数据分割为 hor_seg_size 大小的段, 拷贝每段的前 elem_size 大小的字节。

        取值范围：:math:`[1,  min\{65535 / srcStride, srcHeight\}]` 。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 模式是指 IVE_DMA_DIRECT_COPY 和 IVE_DMA_INTERVAL_COPY 模式;

3. IVE_DMA_DIRECT_COPY : 快速拷贝模式, 可实现直接从大块内存中扣取小块内存，如图 1-1 所示，计算公式如下：

  .. math::

    I_{\text{out}}(x, y) = I(x, y) (0 \leq x \leq width, 0 \leq y \leq height)

  其中，:math:`I(x, y)` 对应 input， :math:`I_{\text{out}}(x, y)` 对应 output。

  **图 1-1 快速拷贝示意图**

  .. image:: ./ive_images/dma_direct_copy.jpg
     :align: center


4. IVE_DMA_INTERVAL_COPY : 间隔拷贝模式， 要求输入数据宽度为 hor_seg_size 的倍数; 间隔拷贝的方式, 即将每 ver_seg_rows 行中第一行数据分割成 hor_seg_size 大小的段, 拷贝每段的前 elem_size 大小的字节，如图 1-2 所示。

  **图 1-2 间隔拷贝示意图**

  .. image:: ./ive_images/dma_interval_copy.jpg
     :align: center

**示例代码**

    .. code-block:: c

      #include <stdio.h>
      #include <stdlib.h>
      #include <string.h>
      #include "bmcv_api_ext_c.h"
      #include <unistd.h>

      int main() {
          int dev_id = 0;
          int height = 1080, width = 1920;
          bm_image_format_ext src_fmt = FORMAT_GRAY, dst_fmt = FORMAT_GRAY;
          char *src_name = "path/to/src", *dst_name = "path/to/dst";
          bm_handle_t handle = NULL;
          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }
          bm_image src, dst;

          // create bm image struct
          bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
          bm_image_create(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

          // alloc bm image memory
          ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
          ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

          // read image data from input files
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

          ret = bmcv_ive_dma(handle, src, dst, 0, NULL);

          unsigned char *ive_res = (unsigned char*) malloc (width * height * sizeof(unsigned char));
          memset(ive_res, 0, width * height * sizeof(unsigned char));
          ret = bm_image_copy_device_to_host(dst, (void**)&ive_res);
          if(ret != BM_SUCCESS){
              printf("dst bm_image_copy_device_to_host is failed \n");
              exit(-1);
          }

          FILE *fp = fopen(dst_name, "wb");
          fwrite((void *)ive_res, 1, width * height * sizeof(unsigned char), fp);
          fclose(fp);

          free(input_data);
          free(ive_res);

          bm_image_destroy(&src);
          bm_image_destroy(&dst);

          bm_dev_free(handle);


          return ret;
      }
