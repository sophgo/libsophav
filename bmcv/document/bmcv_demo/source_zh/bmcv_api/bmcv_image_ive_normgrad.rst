bmcv_ive_normgrad
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建归一化梯度计算任务，梯度分量均归一化到 S8。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_normgrad(
        bm_handle_t             handle,
        bm_image  *             input,
        bm_image  *             output_h,
        bm_image  *             output_v,
        bm_image  *             output_hv,
        bmcv_ive_normgrad_ctrl    normgrad_attr);

| 【参数】

.. list-table:: bmcv_ive_normgrad 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \*input
      - 输入
      - 输入 bm_image 对象数据指针, 不能为空。
    * - \*output_h
      - 输出
      - 输出 bm_image 对象数据指针, 由模板直接滤波并归一到 s8 后得到的梯度分量图像 (H) 指针, 根据 normgrad_attr.en_mode, 若需要输出，则不能为空。
    * - \*output_v
      - 输出
      - 输出 bm_image 对象数据指针, 由模板直接滤波并归一到 s8 后得到的梯度分量图像 (V) 指针, 根据 normgrad_attr.en_mode, 若需要输出则不能为空。
    * - \*output_hv
      - 输出
      - 输出 bm_image 对象数据指针, 由模板和转置后的模板直接滤波, 并归一到 s8 后采用 package 格式存储的图像指针。根据 normgrad_attr.en_mode, 若需要输出则不为空。
    * - \normgrad_attr
      - 输入
      - 归一化梯度信息计算参数, 不能为空。

.. list-table::
    :widths: 21 18 72 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - output_h
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE_SIGNED
      - 64x64~1920x1080
    * - output_v
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE_SIGNED
      - 64x64~1920x1080
    * - output_hv
      - GRAY
      - DATA_TYPE_EXT_U16
      - 64x64~1920x1080


【数据类型说明】

【说明】定义归一化梯度信息计算任务输出控制枚举类型。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_normgrad_outmode_e{
        BM_IVE_NORM_GRAD_OUT_HOR_AND_VER = 0x0,
        BM_IVE_NORM_GRAD_OUT_HOR         = 0x1,
        BM_IVE_NORM_GRAD_OUT_VER         = 0x2,
        BM_IVE_NORM_GRAD_OUT_COMBINE     = 0x3,
    } bmcv_ive_normgrad_outmode;

.. list-table::
    :widths: 125 80

    * - **成员名称**
      - **描述**
    * - BM_IVE_NORM_GRAD_OUT_HOR_AND_VER
      - 同时输出梯度信息的H、V 分量图。
    * - BM_IVE_NORM_GRAD_OUT_HOR
      - 仅输出梯度信息的 H 分量图。
    * - BM_IVE_NORM_GRAD_OUT_VER
      - 仅输出梯度信息的 V 分量图。
    * - BM_IVE_NORM_GRAD_OUT_COMBINE
      - 输出梯度信息以 package 存储的 HV 图。

【说明】定义归一化梯度信息计算任务输出控制枚举类型。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_normgrad_ctrl_s{
        bmcv_ive_normgrad_outmode en_mode;
        signed char as8_mask[25];
        unsigned char u8_norm;
    } bmcv_ive_normgrad_ctrl;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - en_mode
      - 梯度信息输出控制模式。
    * - as8_mask
      - as8Mask 计算梯度需要的模板。
    * - u8_norm
      - 归一化参数, 取值范围: [1, 13]。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 控制参数中输出模式如下：
   - BM_IVE_NORM_GRAD_OUT_HOR_AND_VER 时, output_h 和 output_v 指针不能为空, 且要求跨度一致。

   - BM_IVE_NORM_GRAD_OUT_HOR 时, output_h 不能为空。

   - BM_IVE_NORM_GRAD_OUT_VER 时, output_v 不能为空。

   - BM_IVE_NORM_GRAD_OUT_COMBINE 时, output_hv 不能为空。

3. 计算公式如下：

  .. math::

    I_{\text{out}}(x, y) = \left\{ \sum_{-2 < j < 2} \sum_{-2 < i < 2} I(x+i, y+j)
                           \cdot \text{coef}(i, j) \right\} \gg \text{norm}(x)