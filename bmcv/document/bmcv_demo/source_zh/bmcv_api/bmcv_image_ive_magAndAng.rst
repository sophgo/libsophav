bmcv_ive_mag_and_ang
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建 5x5 模板，计算灰度图的每个像素区域位置灰度值变化梯度的幅值和幅角任务。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_mag_and_ang(
        bm_handle_t            handle,
        bm_image  *             input,
        bm_image  *            mag_output,
        bm_image  *            ang_output,
        bmcv_ive_mag_and_ang_ctrl  attr);

| 【参数】

.. list-table:: bmcv_ive_magandang 参数表
    :widths: 20 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \*input
      - 输入
      - 输入 bm_image 对象指针, 不能为空。
    * - \*mag_output
      - 输出
      - 输出 bm_image 对象数据指针, 输出幅值图像, 不能为空, 宽、高同 input。
    * - \*ang_output
      - 输出
      - 输出 bm_image 对象数据指针, 输出幅角图像。根据 attr.en_out_ctrl, 需要输出则不能为空, 宽、高同 input。
    * - \attr
      - 输入
      - 控制信息结构体, 存放梯度幅值和幅角计算的控制信息，不能为空。


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
    * - mag_output
      - GRAY
      - DATA_TYPE_EXT_U16
      - 同 input
    * - ang_output
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input


| 【数据类型说明】

【说明】定义梯度幅值与角度计算的输出格式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_mag_and_ang_outctrl_e{
        BM_IVE_MAG_AND_ANG_OUT_MAG = 0x0,
        BM_IVE_MAG_AND_ANG_OUT_ALL = 0X1,
    } bmcv_ive_mag_and_ang_outctrl;

.. list-table::
    :widths: 100 100

    * - **成员名称**
      - **描述**
    * - BM_IVE_MAG_AND_ANG_OUT_MAG
      - 仅输出幅值。
    * - BM_IVE_MAG_AND_ANG_OUT_ALL
      - 同时输出幅值和角度值。

【说明】定义梯度幅值和幅角计算的控制信息。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_mag_and_ang_ctrl_s{
        bmcv_ive_mag_and_ang_outctrl     en_out_ctrl;
        unsigned short               u16_thr;
        signed char                  as8_mask[25];
    } bmcv_ive_mag_and_ang_ctrl;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - en_out_ctrl
      - 输出格式。
    * - u16_thr
      - 用于对幅值进行阈值化的阈值。
    * - as8_mask
      - 5x5 模板系数。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 可配置两种输出模式, 详细请参考 bm_ive_mag_and_ang_outctrl。

3. 配置 BM_IVE_MAG_AND_ANG_OUT_ALL 时， 要求 mag_output 与 ang_output 跨度一致。

4. 用户可以通过 attr.u16_thr 对幅值图进行阈值化操作(可用来实现EOH), 计算公式如下:

  .. math::

    \text{Mag}(x, y) =
    \begin{cases}
        \text{0} & \ (Mag(x, y) < u16_thr) \\
        \text{Mag(x, y)} & \ (Mag(x, y) \geq u16_thr) \\
    \end{cases}

  其中, :math:`Mag(x, y)` 对应 mag_output。