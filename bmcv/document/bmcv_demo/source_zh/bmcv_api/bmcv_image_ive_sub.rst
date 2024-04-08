bmcv_ive_sub
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建两张图像的相减操作。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_sub(
        bm_handle_t          handle,
        bm_image             input1,
        bm_image             input2,
        bm_image             output,
        bmcv_ive_sub_attr    attr);

| 【参数】

.. list-table:: bmcv_ive_sub 参数表
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
      - 相减操作对应模式的结构体，不能为空。


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

【注意】 output 数据类型格式也有可能是 DATA_TYPE_EXT_1N_BYTE_SIGNED。

| 【数据类型说明】

【说明】定义两张图像相减输出格式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_sub_mode_e{
        IVE_SUB_ABS = 0x0,
        IVE_SUB_SHIFT = 0x1,
        IVE_SUB_BUTT
    } bmcv_ive_sub_mode;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - IVE_SUB_ABS
      - 取差的绝对值。
    * - IVE_SUB_SHIFT
      - 将结果右移一位输出, 保留符号位。

【说明】定义两图像相减控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_sub_attr_s {
        bmcv_ive_sub_mode en_mode;
    } bmcv_ive_sub_attr;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - en_mode
      - MOD_SUB 下 两图像相减模式。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

* 输入输出图像的 width 都需要16对齐。

* MOD_SUB:

     - IVE_SUB_ABS:
        .. math::

           \begin{aligned}
            & I_{\text{out}}(x, y) = abs(I_{\text{src1}}(x, y) - I_{\text{src2}}(x, y))
           \end{aligned}

     输出格式是 DATA_TYPE_EXT_1N_BYTE。

     - IVE_SUB_SHIFT:
        .. math::

           \begin{aligned}
            & I_{\text{out}}(x, y) = (I_{\text{src1}}(x, y) - I_{\text{src2}}(x, y)) \gg 1
           \end{aligned}

     输出格式是 DATA_TYPE_EXT_1N_BYTE_SIGNED。
