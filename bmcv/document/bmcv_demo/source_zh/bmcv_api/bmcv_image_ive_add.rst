bmcv_ive_add
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建两张图像的加权相加操作。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_add(
        bm_handle_t          handle,
        bm_image             input1,
        bm_image             input2,
        bm_image             output,
        bmcv_ive_add_attr    attr);

| 【参数】

.. list-table:: bmcv_ive_add 参数表
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
    * - \attr
      - 输入
      - 相加操作的需要的加权系数对应的结构体，不能为空。


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


【说明】定义两图像的加权加控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_add_attr_s {
        unsigned short param_x;
        unsigned short param_y;
    } bmcv_ive_add_attr;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - param_x
      - 加权加 "xA + yB" 中的权重x。
    * - param_y
      - 加权加 "xA + yB" 中的权重y。


| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

* 输入输出图像的 width 都需要16对齐。

* MOD_ADD:
    .. math::

       \begin{aligned}
        & I_{\text{out}}(i, j) = x * I_{1}(i, j) + y * I_{2}(i, j)
      \end{aligned}

    其中，:math:`I_{1}(i, j)` 对应 input1, :math:`I_{2}(i, j)` 对应 input2, :math:`x, y` 分别对应 bmcv_ive_add_attr 中的 param_x 与 param_y。

    若定点化前, :math:`x` 和 :math:`y` 满足 :math:`x + y > 1`, 则当前计算结果超过 8bit 取 低 8bit 作为最终结果。

