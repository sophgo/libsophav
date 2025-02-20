bmcv_ive_ord_stat_filter
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建 3x3 模板顺序统计量滤波任务，可进行 Median、 Max、 Min 滤波。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_ord_stat_filter(
    bm_handle_t               handle,
    bm_image                  input,
    bm_image                  output,
    bmcv_ive_ord_stat_filter_mode mode);

| 【参数】

.. list-table:: bmcv_ive_ord_stat_filter 参数表
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
    * - output
      - 输出
      - 输出 bm_image 对象结构体, 不能为空, 宽、高同 input。
    * - mode
      - 输入
      - 控制顺序统计量滤波的模式。

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
    * - output
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input1

| 【数据类型说明】

【说明】定义顺序统计量滤波模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    _bmcv_ord_stat_filter_mode_e{
        BM_IVE_ORD_STAT_FILTER_MEDIAN = 0x0,
        BM_IVE_ORD_STAT_FILTER_MAX    = 0x1,
        BM_IVE_ORD_STAT_FILTER_MIN    = 0x2,
    } bmcv_ive_ord_stat_filter_mode;

.. list-table::
    :widths: 120 80

    * - **成员名称**
      - **描述**
    * - BM_IVE_ORD_STAT_FILTER_MEDIAN
      - 中值滤波
    * - BM_IVE_ORD_STAT_FILTER_MAX
      - 最大值滤波, 等价于灰度图的膨胀。
    * - BM_IVE_ORD_STAT_FILTER_MIN
      - 最小值滤波, 等价于灰度图的腐蚀。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 可配置 3 种滤波模式, 参考 bmcv_ive_ord_stat_filter_mode 说明。

3. 计算公式如下：

- BM_IVE_ORD_STAT_FILTER_MEDIAN:

    .. math::
       \begin{aligned}
        & & I_{\text{out}}(x, y) = \text{median}\{\substack{-1 < i < 1 \\ -1 < j < 1} I(x+i, y+j)\}
      \end{aligned}

- BM_IVE_ORD_STAT_FILTER_MAX:

    .. math::

      \begin{aligned}
        & I_{\text{out}}(x, y) = \text{max}\{\substack{-1 < i < 1 \\ -1 < j < 1} I(x+i, y+j)\}
      \end{aligned}

- BM_IVE_ORD_STAT_FILTER_MIN:

    .. math::

      \begin{aligned}
       & I_{\text{out}}(x, y) = \text{min}\{\substack{-1 < i < 1 \\ -1 < j < 1} I(x+i, y+j)\} \\
      \end{aligned}

  其中, :math:`I(x, y)` 对应 input, :math:`I_{\text{out}(x, y)}` 对应 output。


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
        bmcv_ive_ord_stat_filter_mode ordStatFilterMode = BM_IVE_ORD_STAT_FILTER_MEDIAN;
        char *src_name = "path/to/src", *dst_name = "path/to/dst";

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

        ret = bmcv_ive_ord_stat_filter(handle, src, dst, ordStatFilterMode);

        unsigned char* ordStatFilter_res = malloc(width * height * sizeof(unsigned char));
        memset(ordStatFilter_res, 0, width * height * sizeof(unsigned char));

        ret = bm_image_copy_device_to_host(dst, (void **)&ordStatFilter_res);

        FILE *ive_result_fp = fopen(dst_name, "wb");
        fwrite((void *)ordStatFilter_res, 1, width * height, ive_result_fp);
        fclose(ive_result_fp);

        free(ordStatFilter_res);

        bm_image_destroy(&src);
        bm_image_destroy(&dst);

        bm_dev_free(handle);
        return 0;
      }