bmcv_ive_sad
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 计算两幅图像按照 4x4/8x8/16x16 分块的 16bit/8bit SAD 图像, 以及对 SAD 进行阈值化输出。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_sad(
          bm_handle_t                handle,
          bm_image *                 input,
          bm_image *                 output_sad,
          bm_image *                 output_thr,
          bmcv_ive_sad_attr *        sad_attr,
          bmcv_ive_sad_thresh_attr*  thresh_attr);

| 【参数】

.. list-table:: bmcv_ive_sad 参数表
    :widths: 15 15 60

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \*input
      - 输入
      - 输入 bm_image 对象数组，存放两个输入图像, 不能为空。
    * - \*output_sad
      - 输出
      - 输出 bm_image 对象数据指针, 输出 SAD 图像指针, 根据 sad_attr->en_out_ctrl, 若需要输出则不能为空。根据 sad_attr->en_mode, 对应 4x4、8x8、16x16 分块模式, 高、宽分别为 input1 的 1/4、1/8、1/16。
    * - \*output_thr
      - 输出
      - 输出 bm_image 对象数据指针, 输出 SAD 阈值化图像指针, 根据 sad_attr->en_out_ctrl, 若需要输出则不能为空。根据 sad_attr->en_mode, 对应 4x4、8x8、16x16 分块模式, 高、宽分别为 input1 的 1/4、1/8、1/16。
    * - \*sad_attr
      - 输入
      - SAD 控制参数指针结构体, 不能为空。
    * - \*thresh_attr
      - 输入
      - SAD thresh模式需要的阈值参数结构体指针，非thresh模式情况下，传入值但是不起作用。

.. list-table::
    :widths: 22 20 54 42

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input1
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - input2
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input1
    * - output_sad
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE

        DATA_TYPE_EXT_U16
      - 根据 sad_attr.en_mode,

        对应 4x4、8x8、16x16分块模式, 高 、宽分别是 input1 的 1/4、1/8、1/16。
    * - output_thr
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 根据 sad_attr.en_mode,

        对应 4x4、8x8、16x16分块模式, 高 、宽分别是 input1 的 1/4、1/8、1/16。

| 【数据类型说明】

【说明】定义 SAD 计算模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_sad_mode_e{
        BM_IVE_SAD_MODE_MB_4X4 = 0x0,
        BM_IVE_SAD_MODE_MB_8X8 = 0x1,
        BM_IVE_SAD_MODE_MB_16X16 = 0x2,
    } bmcv_ive_sad_mode;

.. list-table::
    :widths: 100 100

    * - **成员名称**
      - **描述**
    * - BM_IVE_SAD_MODE_MB_4X4
      - 按 4x4 像素块计算 SAD。
    * - BM_IVE_SAD_MODE_MB_8X8
      - 按 8x8 像素块计算 SAD。
    * - BM_IVE_SAD_MODE_MB_16X16
      - 按 16x16 像素块计算 SAD。

【说明】定义 SAD 输出控制模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_sad_out_ctrl_s{
        BM_IVE_SAD_OUT_BOTH = 0x0,
        BM_IVE_SAD_OUT_SAD  = 0x1,
        BM_IVE_SAD_OUT_THRESH  = 0x3,
    } bmcv_ive_sad_out_ctrl;

.. list-table::
    :widths: 100 100

    * - **成员名称**
      - **描述**
    * - BM_IVE_SAD_OUT_BOTH
      - SAD 图和阈值化图输出模式。
    * - BM_IVE_SAD_OUT_SAD
      - SAD 图输出模式。
    * - BM_IVE_SAD_OUT_THRESH
      - 阈值化输出模式。

【说明】定义 SAD 控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_sad_attr_s{
        bm_ive_sad_mode en_mode;
        bm_ive_sad_out_ctrl en_out_ctrl;
    } bmcv_ive_sad_attr;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - en_mode
      - SAD计算模式。
    * - en_out_ctrl
      - SAD 输出控制模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_sad_thresh_attr_s{
        unsigned short u16_thr;
        unsigned char  u8_min_val;
        unsigned char  u8_max_val;
    }bmcv_ive_sad_thresh_attr;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - u16_thr
      - 对计算的 SAD 图进行阈值化的阈值。取值范围依赖 enMode。
    * - u8_min_val
      - 阈值化不超过 u16Thr 时的取值。
    * - u8_max_val
      - 阈值化超过 u16Thr 时的取值。

【注意】

* BM_IVE_SAD_OUT_8BIT_BOTH：u16_thr 取值范围：[0, 255];

* BM_IVE_SAD_OUT_16BIT_BOTH 或者 BM_IVE_SAD_OUT_THRESH：u16_thr 取值范围：[0, 65535]。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 计算公式如下：

  .. math::

   \begin{aligned}
      & SAD_{\text{out}}(x, y) = \sum_{\substack{n \cdot x \leq i < n \cdot (x+1) \\ n \cdot y \leq j < n \cdot (j+1)}} \lvert I_{1}(x, y) - I_{2}(x, y) \rvert \\
      & THR_{\text{out}}(x, y) =
        \begin{cases}
          minVal & \ (SAD_{\text{out}}(x, y) \leq Thr) \\
          maxVal & \ (SAD_{\text{out}}(x, y) > Thr) \\
        \end{cases}
   \end{aligned}