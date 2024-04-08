bmcv_ive_integ
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建灰度图像的积分图计算任务，积分像素值等于灰度图的左上角与当前点所围成的矩形区域内所有像素点灰度值之和/平方和。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_integ(
        bm_handle_t        handle,
        bm_image           input,
        bm_device_mem_t    output,
        bmcv_ive_integ_ctrl_s  integ_attr);

| 【参数】

.. list-table:: bmcv_ive_integ 参数表
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
      - 输入 bm_device_mem_t 对象数据结构体, 宽、高同 input。
    * - integ_attr
      - 输入
      - 积分图计算控制参数结构体，不能为空。

.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 32x16~1920x1080

| 【数据类型说明】

【说明】定义积分图输出控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum _ive_integ_out_ctrl_e {
        IVE_INTEG_MODE_COMBINE = 0x0,
        IVE_INTEG_MODE_SUM = 0x1,
        IVE_INTEG_MODE_SQSUM = 0x2,
        IVE_INTEG_MODE_BUTT
    } ive_integ_out_ctrl_e;

.. list-table::
    :widths: 80 100

    * - **成员名称**
      - **描述**
    * - IVE_INTEG_MODE_COMBINE
      - 和、平方和积分图组合输出。
    * - IVE_INTEG_MODE_SUM
      - 仅和积分图输出。
    * - IVE_INTEG_MODE_SQSUM
      - 仅平方和积分图输出。

【说明】定义积分图计算控制参数

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_integ_ctrl_t{
        ive_integ_out_ctrl_e en_out_ctrl;
    } bmcv_ive_integ_ctrl_s;

.. list-table::
    :widths: 80 100

    * - **成员名称**
      - **描述**
    * - en_out_ctrl
      - 积分图输出控制参数。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入图像的 width 都需要16对齐。

2. IVE_INTEG_MODE_COMBINE, 组合输出模式, 输出图像数据类型必须为u64, 计算公式如下:

  .. math::

   \begin{aligned}
      & I_{\text{sum}}(x, y) = \sum_{i \geq 0}^{i \leq x} \sum_{j \geq 0}^{j \leq y} I(i, j) \\
      & I_{\text{sq}}(x, y)  = \sum_{i \geq 0}^{i \leq x} \sum_{j \geq 0}^{j \leq y} (I(i, j) \cdot I(i, j)) \\
      & I_{\text{out}}(x, y) = (I_{\text{sq}}(x, y) \ll 28 \mid I_{\text{sum}}(x, y) \& 0xFFFFFFF)
   \end{aligned}

3. IVE_INTEG_MODE_SUM, 仅和积分图输出模式, 输出图像数据类型必须为u32。

  .. math::

   \begin{aligned}
      & I_{\text{sum}}(x, y) = \sum_{i \geq 0}^{i \leq x} \sum_{j \geq 0}^{j \leq y} I(i, j) \\
      & I_{\text{out}}(x, y) = I_{\text{sum}}(x, y)
   \end{aligned}

4. IVE_INTEG_MODE_SQSUM, 仅平方和积分图输出模式, 输出图像数据类型必须为u32。

  .. math::

   \begin{aligned}
      & I_{\text{sq}}(x, y)  = \sum_{i \geq 0}^{i \leq x} \sum_{j \geq 0}^{j \leq y} (I(i, j) \cdot I(i, j)) \\
      & I_{\text{out}}(x, y) = I_{\text{sq}}(x, y)
   \end{aligned}


  其中，:math:`I(x, y)` 对应 input, :math:`I_{\text{sum}}` 对应 output。