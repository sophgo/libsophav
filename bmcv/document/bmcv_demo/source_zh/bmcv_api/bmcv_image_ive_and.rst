bmcv_ive_and
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建两张图像的加权相与操作。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_and(
        bm_handle_t          handle,
        bm_image             input1,
        bm_image             input2,
        bm_image             output);

| 【参数】

.. list-table:: bmcv_ive_and 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input1
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - \input2
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - \output
      - 输出
      - 输出 bm_image 对象结构体, 不能为空。


.. list-table::
    :widths: 22 40 64 42

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input1
      - GRAY

        GRAY 二值图
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - input2
      - GRAY

        GRAY 二值图
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input1
    * - output
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 Input1



| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

* 输入输出图像的 width 都需要16对齐。

* MOD_AND:
    .. math::

       \begin{aligned}
        & I_{\text{out}}(x, y) = I_{\text{src1}}(x, y) \& I_{\text{src2}}(x, y)
      \end{aligned}

    其中，:math:`I_{\text{src1}}(x, y)` 对应 input1, :math:`I_{\text{src2}}(x, y)` 对应 input2, :math:`I_{\text{out}}(x, y)` 对应 output。
