bmcv_ive_thresh
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 支持 U8 数据到 U8 数据、S16 数据到 8bit 数据或 U16 数据到 U8 数据的阈值化任务。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_thresh(
        bm_handle_t          handle,
        bm_image             input,
        bm_image             output,
        bmcv_ive_thresh_mode thresh_mode,
        bmcv_ive_thresh_attr attr);

| 【参数】

.. list-table:: bmcv_ive_thresh 参数表
    :widths: 15 15 45

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - input
      - 输入
      - 输入 bm_image 结构体, 不能为空。
    * - output
      - 输出
      - 输出 bm_image 结构体, 由模板直接滤波得到的梯度分量图像 H 指针, 根据 sobel_attr.enOutCtrl, 若需要输出则不能为空, 宽、高同 input。
    * - thresh_mode
      - 输入
      - 控制阈值化计算模式。
    * - attr
      - 输入
      - 定义图像二值化控制信息。

| 【数据类型说明】

.. list-table::
    :widths: 20 18 72 28

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE

        DATA_TYPE_EXT_S16

        DATA_TYPE_EXT_U16
      - 64x64~1920x1080
    * - output
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE

        DATA_TYPE_EXT_1N_BYTE_SIGNED
      - 同 input


【说明】定义阈值化计算方式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_thresh_mode_e{
        // u8
        IVE_THRESH_BINARY = 0x0,
        IVE_THRESH_TRUNC = 0x1,
        IVE_THRESH_TO_MINAL = 0x2,
        IVE_THRESH_MIN_MID_MAX = 0x3,
        IVE_THRESH_ORI_MID_MAX = 0x4,
        IVE_THRESH_MIN_MID_ORI = 0x5,
        IVE_THRESH_MIN_ORI_MAX = 0x6,
        IVE_THRESH_ORI_MID_ORI = 0x7,

        // s16
        IVE_THRESH_S16_TO_S8_MIN_MID_MAX = 0x8,
        IVE_THRESH_S16_TO_S8_MIN_ORI_MAX = 0x9,
        IVE_THRESH_S16_TO_U8_MIN_MID_MAX = 0x10,
        IVE_THRESH_S16_TO_U8_MIN_ORI_MAX = 0x11,

        // u16
        IVE_THRESH_U16_TO_U8_MIN_MID_MAX = 0x12,
        IVE_THRESH_U16_TO_U8_MIN_ORI_MAX = 0x13,

      } bmcv_ive_thresh_mode;

1. MOD_U8 支持以下计算模式:

   * IVE_THRESH_BINARY:

        src_val ≤ low_thr, dst_val = min_val;

        src_val > low_thr, dst_val = max_val。

   * IVE_THRESH_TRUNC:

        src_val ≤ low_thr, dst_val = src_val;

        src_val > low_thr, dst_val = max_val。

   * IVE_THRESH_TO_MINAL:

        src_val ≤ low_thr, dst_val = min_val;

        src_val > low_thr, dst_val = src_val。

   * IVE_THRESH_MIN_MID_MAX:

        src_val ≤ low_thr, dst_val = min_val;

        low_thr < src_val ≤ high_thr, dst_val = mid_val;

        src_val > high_thr, dst_val = max_val。

   * IVE_THRESH_ORI_MID_MAX:

        src_val ≤ low_thr, dst_val = src_val;

        low_thr < src_val ≤ high_thr, dst_val = mid_val;

        src_val > high_thr, dst_val = max_val。

   * IVE_THRESH_MIN_MID_ORI:

        src_val ≤ low_thr, dst_val = min_val;

        low_thr < src_val ≤ high_thr, dst_val = mid_val;

        src_val > high_thr, dst_val = src_val。

   * IVE_THRESH_MIN_ORI_MAX:

        src_val ≤ low_thr, dst_val = min_val;

        low_thr < src_val ≤ high_thr, dst_val = src_val;

        src_val > high_thr, dst_val = max_val

   * IVE_THRESH_ORI_MID_ORI:

        src_val ≤ low_thr, dst_val = src_val;

        low_thr < src_val ≤ high_thr, dst_val = mid_val;

        src_val > high_thr, dst_val = src_val。

2. MOD_S16 支持以下计算模式:

   - IVE_THRESH_S16_TO_S8_MIN_MID_MAX:

        src_val ≤ low_thr, dst_val = min_val;

        low_thr < src_val ≤ high_thr, dst_val = mid_val;

        src_val > high_thr, dst_val = max_val。

   - IVE_THRESH_S16_TO_S8_MIN_ORI_MAX:

        src_val ≤ low_thr, dst_val = min_val;

        low_thr < src_val ≤ high_thr, dst_val = src_val;

        src_val > high_thr, dst_val = max_val。

   - IVE_THRESH_S16_TO_U8_MIN_MID_MAX:

        src_val ≤ low_thr, dst_val = min_val;

        low_thr < src_val ≤ high_thr, dst_val = mid_val;

        src_val > high_thr, dst_val = max_val。

   - IVE_THRESH_S16_TO_U8_MIN_ORI_MAX:

        src_val ≤ low_thr, dst_val = min_val;

        low_thr < src_val ≤ high_thr, dst_val = src_val;

        src_val > high_thr, dst_val = max_val。

3. MOD_U16 支持以下计算模式:

   - IVE_THRESH_U16_TO_U8_MIN_MID_MAX:

        src_val ≤ low_thr, dst_val = min_val;

        low_thr < src_val ≤ high_thr, dst_val = mid_val;

        src_val > high_thr, dst_val = max_val。

   - IVE_THRESH_U16_TO_U8_MIN_ORI_MAX:

        src_val ≤ low_thr, dst_val = min_val;

        low_thr < src_val ≤ high_thr, dst_val = src_val;

        src_val > high_thr, dst_val = max_val。

【说明】定义图像二值化控制信息。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_thresh_attr_s {
        int low_thr;
        int high_thr;
        int min_val;
        int mid_val;
        int max_val;
    } bmcv_ive_thresh_attr;

.. list-table::
    :widths: 30 100

    * - **成员名称**
      - **描述**
    * - low_thr
      - 低阈值。
    * - high_thr
      - 高阈值。
    * - min_val
      - 最小值。
    * - mid_val
      - 中间值。
    * - max_val
      - 最大值。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 可配置 3 种阈值化模式, 分别是 MOD_U8、MOD_U16、MOD_S16, 每种模式支持的阈值计算模式详见 bmcv_ive_thresh_mode。

3. 3 种阈值化模式的计算公式如下:

* MOD_U8

    (1) IVE_THRESH_BINARY:

    .. math::
       I_{\text{out}}(x, y) =
       \begin{cases}
        min\_val & \ (I(x, y) \leq low\_thr) \\
        max\_val & \ (I(x, y) > low\_thr) \\
      \end{cases}

    :math:`mid\_val 、 high\_thr` 无需赋值。

    (2) IVE_THRESH_TRUNC:

    .. math::
       I_{\text{out}}(x, y) =
       \begin{cases}
        I(x, y) & \ (I(x, y) \leq low\_thr) \\
        max\_val & \ (I(x, y) > low\_thr) \\
      \end{cases}

    :math:`min\_val、mid\_val、 high\_thr` 无需赋值。

    (3) IVE_THRESH_TO_MINAL:

    .. math::
       I_{\text{out}}(x, y) =
       \begin{cases}
        min\_val & \ (I(x, y) \leq low\_thr) \\
        I(x, y) & \ (I(x, y) > low\_thr) \\
      \end{cases}

    :math:`min\_val、max\_val、 high\_thr` 无需赋值。

    (4) IVE_THRESH_MIN_MID_MAX:

    .. math::
       I_{\text{out}}(x, y) =
       \begin{cases}
        min\_val & \ (I(x, y) \leq low\_thr) \\
        mid\_val & \ (low\_val < I(x, y) \leq high\_val) \\
        max\_val & \ (I(x, y) > high\_thr) \\
      \end{cases}

    (5) IVE_THRESH_ORI_MID_MAX:

    .. math::
       I_{\text{out}}(x, y) =
       \begin{cases}
        I(x, y) & \ (I(x, y) \leq low\_thr) \\
        mid\_val & \ (low\_val < I(x, y) \leq high\_val) \\
        max\_val & \ (I(x, y) > high\_thr) \\
      \end{cases}

    :math:`min\_val` 无需赋值。

    (6) IVE_THRESH_MIN_MID_ORI:

    .. math::
       I_{\text{out}}(x, y) =
       \begin{cases}
        min\_val & \ (I(x, y) \leq low\_thr) \\
        mid\_val & \ (low\_val < I(x, y) \leq high\_val) \\
        I(x, y) & \ (I(x, y) > high\_thr) \\
      \end{cases}

    :math:`max\_val` 无需赋值。

    (7) IVE_THRESH_MIN_ORI_MAX:

    .. math::
       I_{\text{out}}(x, y) =
       \begin{cases}
        min\_val & \ (I(x, y) \leq low\_thr) \\
        I(x, y) & \ (low\_val < I(x, y) \leq high\_val) \\
        max\_val & \ (I(x, y) > high\_thr) \\
      \end{cases}

    :math:`mid\_val` 无需赋值。

    (8) IVE_THRESH_ORI_MID_ORI:

    .. math::
       I_{\text{out}}(x, y) =
       \begin{cases}
        I(x, y) & \ (I(x, y) \leq low\_thr) \\
        mid\_val & \ (low\_val < I(x, y) \leq high\_val) \\
        I(x, y) & \ (I(x, y) > high\_thr) \\
      \end{cases}


    :math:`min\_val、max\_val` 无需赋值。

* MOD_S16

    (1) IVE_THRESH_S16_TO_S8_MIN_MID_MAX:

    .. math::
       I_{\text{out}}(x, y) =
       \begin{cases}
        min\_val & \ (I(x, y) \leq low\_thr) \\
        mid\_val & \ (low\_val < I(x, y) \leq high\_val) \\
        max\_val & \ (I(x, y) > high\_thr) \\
      \end{cases}

    要求:
         :math:`-32768 \leq low\_thr \leq high\_thr \leq 32767`;

         :math:`-128 \leq min\_val、mid\_val、max\_val \leq 127`。

    (2) IVE_THRESH_S16_TO_S8_MIN_ORI_MAX:

    .. math::
       I_{\text{out}}(x, y) =
       \begin{cases}
        min\_val & \ (I(x, y) \leq low\_thr) \\
        I(x, y) & \ (low\_val < I(x, y) \leq high\_val) \\
        max\_val & \ (I(x, y) > high\_thr) \\
      \end{cases}

    要求:
         :math:`-129 \leq low\_thr \leq high\_thr \leq 127`;

         :math:`-128 \leq min\_val、max\_val \leq 127`。

    (3) IVE_THRESH_S16_TO_U8_MIN_MID_MAX:

    .. math::
       I_{\text{out}}(x, y) =
       \begin{cases}
        min\_val & \ (I(x, y) \leq low\_thr) \\
        mid\_val & \ (low\_val < I(x, y) \leq high\_val) \\
        max\_val & \ (I(x, y) > high\_thr) \\
      \end{cases}

    要求:
         :math:`-32768 \leq low\_thr \leq high\_thr \leq 32767`;

         :math:`0 \leq min\_val、mid\_val、max\_val \leq 255`。

    (4) IVE_THRESH_S16_TO_U8_MIN_ORI_MAX:

    .. math::
       I_{\text{out}}(x, y) =
       \begin{cases}
        min\_val & \ (I(x, y) \leq low\_thr) \\
        I(x, y) & \ (low\_val < I(x, y) \leq high\_val) \\
        max\_val & \ (I(x, y) > high\_thr) \\
      \end{cases}

    要求:
         :math:`-1 \leq low\_thr \leq high\_thr \leq 255`;

         :math:`0 \leq min\_val、max\_val \leq 255`。

* MOD_U16

    (1) IVE_THRESH_U16_TO_U8_MIN_MID_MAX:

    .. math::
       I_{\text{out}}(x, y) =
       \begin{cases}
        min\_val & \ (I(x, y) \leq low\_thr) \\
        mid\_val & \ (low\_val < I(x, y) \leq high\_val) \\
        max\_val & \ (I(x, y) > high\_thr) \\
      \end{cases}

    要求: :math:`0 \leq low\_thr \leq high\_thr \leq 65535`;

    (2) IVE_THRESH_U16_TO_U8_MIN_ORI_MAX:

    .. math::
       I_{\text{out}}(x, y) =
       \begin{cases}
        min\_val & \ (I(x, y) \leq low\_thr) \\
        I(x, y) & \ (low\_val < I(x, y) \leq high\_val) \\
        max\_val & \ (I(x, y) > high\_thr) \\
      \end{cases}

    要求: :math:`0 \leq low\_thr \leq high\_thr \leq 255`;


    其中, :math:`I(x, y)` 对应 input, :math:`I_{\text{out}}(x, y)` 对应 output, low_thr、 high_thr、 min_val、mid_val 和 max_val 分别对应 attr 的 low_thr、high_thr、min_val、mid_val、max_val;

    *  bmcv_ive_thresh_attr 中的 min_val、mid_val、max_val 并不需要满足变量命名含义中的大小关系.


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
          bmcv_ive_thresh_mode thresh_mode = IVE_THRESH_MIN_MID_MAX; /* IVE_THRESH_MODE_MIN_MID_MAX */
          int low_thr = 236, high_thr = 249, min_val = 166, mid_val = 219, max_val = 60;
          bm_image_format_ext src_fmt = FORMAT_GRAY, dst_fmt = FORMAT_GRAY;
          char *src_name = "path/to/src", *dst_name = "path/to/dst";

          bm_handle_t handle = NULL;
          int ret = (int)bm_dev_request(&handle, dev_id);
          if (ret != 0) {
              printf("Create bm handle failed. ret = %d\n", ret);
              exit(-1);
          }

          bmcv_ive_thresh_attr thresh_attr;
          bm_image src, dst;
          int src_stride[4];
          int dst_stride[4];

          // calc ive image stride
          int data_size = 1;
          src_stride[0] = align_up(width, 16) * data_size;
          dst_stride[0] = align_up(width, 16) * data_size;
          // create bm image struct
          bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
          bm_image_create(handle, height, width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);

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


          memset(&thresh_attr, 0, sizeof(bmcv_ive_thresh_attr));
          thresh_attr.low_thr = low_thr;
          thresh_attr.high_thr = high_thr;
          thresh_attr.min_val = min_val;
          thresh_attr.mid_val = mid_val;
          thresh_attr.max_val = max_val;

          ret = bmcv_ive_thresh(handle, src, dst, thresh_mode, thresh_attr);

          unsigned char *ive_res = (unsigned char*) malloc(width * height * sizeof(unsigned char));
          memset(ive_res, 0, width * height * sizeof(unsigned char));

          ret = bm_image_copy_device_to_host(dst, (void **)&ive_res);

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