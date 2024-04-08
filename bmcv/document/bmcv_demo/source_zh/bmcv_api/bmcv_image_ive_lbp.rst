bmcv_ive_lbp
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, lbp是描述图像局部特征的算子，邻域窗口内的像素值与中心像素值进行相减结果和阈值对比求和得到这一局部的中心像素值。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_lbp(
        bm_handle_t         handle,
        bm_image            input,
        bm_image            output,
        bmcv_ive_lbp_ctrl_attr    lbp_attr);

| 【参数】

.. list-table:: bmcv_ive_lbp 参数表
    :widths: 15 15 35

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
      - 输出 bm_image 对象结构体, 不能为空, 宽、高同 input。
    * - \lbp_attr
      - 输入
      - LBP 纹理计算控制参数结构体, 不能为空。

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
      - 64x64~1920x1080

| 【数据类型说明】

【说明】定义LBP计算的比较模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_lbp_cmp_mode_e{
        BM_IVE_LBP_CMP_MODE_NORMAL = 0x0,
        BM_IVE_LBP_CMP_MODE_ABS    = 0x1,
    } bmcv_lbp_cmp_mode;

.. list-table::
    :widths: 100 100

    * - **成员名称**
      - **描述**
    * - BM_IVE_LBP_CMP_MODE_NORMAL
      - LBP 简单比较模式。
    * - BM_IVE_LBP_CMP_MODE_ABS
      - LBP 绝对值比较模式。

【说明】定义 8 bit 数据联合体。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef union bmcv_ive_8bit_u{
        signed char  s8_val;
        unsigned char u8_val;
    } bmcv_ive_8bit;

.. list-table::
    :widths: 50 100

    * - **成员名称**
      - **描述**
    * - s8_val
      - 有符号的 8bit 值。
    * - u8_val
      - 无符号的 8bit 值。

【说明】定义 LBP 纹理计算控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_lbp_ctrl_attr_s{
        bmcv_lbp_cmp_mode en_mode;
        bmcv_ive_8bit     un8_bit_thr;
    } bmcv_ive_lbp_ctrl_attr;

.. list-table::
    :widths: 30 100

    * - **成员名称**
      - **描述**
    * - en_mode
      - LBP 比较模式。
    * - un8_bit_thr
      - LBP 比较阈值。

        BM_IVE_LBP_CMP_MODE_NORMAL：取值范围：[-128, 127];

        BM_IVE_LBP_CMP_MODE_ABS：取值范围：[0, 255]。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入输出图像的 width 都需要16对齐。

2. LBP 计算公式如图 1-3 所示:

   **图 1-3 LBP 计算公式示意图**

   .. image:: ./ive_images/lbp_image.png
      :align: center

* BM_IVE_LBP_CMP_MODE_NORMAL:
    .. math::

       \begin{aligned}
        & lbp(x, y) = \sum_{i = 0}^{7} ((I_{i} - I_{c}) \geq thr ) \ll (7 - i), \text{其中} thr \in [-128, 127];
      \end{aligned}

* BM_IVE_LBP_CMP_MODE_ABS:
    .. math::

      \begin{aligned}
        & lbp(x, y) = \sum_{i = 0}^{7} (abs(I_{i} - I_{c}) \geq thr ) \ll (7 - i), \text{其中} thr \in [0, 255]; \\
      \end{aligned}

    其中，:math:`I(x, y)` 对应 input， :math:`lbp(x, y)` 对应 output， :math:`thr` 对应 lbp\_attr.un8_bit_thr。