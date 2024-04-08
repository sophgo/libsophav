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