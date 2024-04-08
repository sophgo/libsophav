bmcv_ive_filter
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建 5x5 模板滤波任务, 通过配置不同的模板系数, 可以实现不同的滤波。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_filter(
        bm_handle_t            handle,
        bm_image               input,
        bm_image               output,
        bmcv_ive_filter_ctrl   filter_attr);

| 【参数】

.. list-table:: bmcv_ive_filter 参数表
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
      - 输出 bm_image 对象结构体，不能为空, 宽、高同 input。
    * - filter_attr
      - 输入
      - 控制信息结构体, 不能为空。

.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY

        NV21

        NV61
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - output
      - 同input
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input

| 【数据类型说明】

【说明】定义模板滤波控制信息。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_filter_ctrl_s{
        signed char as8_mask[25];
        unsigned char u8_norm;
    } bmcv_ive_filter_ctrl;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - as8_mask
      - 5x5 模板系数，外围系数设为0，可实现 3x3 模板滤波。
    * - u8_norm
      - 归一化参数，取值范围: [0, 13]。

【注意】

* 通过配置不同的模板系数可以达到不同的滤波效果。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 当输入数据 为 FORMAT_NV21、FORMAT_NV61 类型时, 要求输出数据跨度一致。

3. Filter 计算公式:

  .. math::

    I_{\text{out}}(x, y) = \left\{ \sum_{-2 < j < 2} \sum_{-2 < i < 2} I(x+i, y+j) \cdot \text{coef}(i, j) \right\} \gg \text{norm}(x)

4. 经典高斯模板如下， 其中 norm_u8 分别等于4、8、8:

  .. raw:: latex

   \begin{align*}
   \hspace{2em} % 控制间隔
   \begin{bmatrix}
      0 & 0 & 0 & 0 & 0 \\
      0 & 1 & 2 & 1 & 0 \\
      0 & 2 & 4 & 2 & 0 \\
      0 & 1 & 2 & 1 & 0 \\
      0 & 0 & 0 & 0 & 0 \\
   \end{bmatrix}
   \hspace{5em} % 控制间隔
   \begin{bmatrix}
      1 & 2 & 3 & 2 & 1 \\
      2 & 5 & 6 & 5 & 2 \\
      3 & 6 & 8 & 6 & 3 \\
      2 & 5 & 6 & 5 & 2 \\
      1 & 2 & 3 & 2 & 1 \\
   \end{bmatrix}
   \times 3
   \hspace{3.5em} % 控制间隔
   \begin{bmatrix}
      1 & 4 & 7 & 4 & 1 \\
      4 & 16 & 26 & 16 & 4 \\
      7 & 26 & 41 & 26 & 7 \\
      4 & 16 & 26 & 16 & 4 \\
      1 & 4 & 7 & 4 & 1 \\
   \end{bmatrix}
   \end{align*}