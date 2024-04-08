bmcv_ive_gmm
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建 GMM 背景建模任务，支持灰度图、 RGB_PACKAGE 图像的 GMM 背景建模， 高斯模型个数为 3 或者 5。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_gmm(
        bm_handle_t         handle,
        bm_image            input,
        bm_image            output_fg,
        bm_image            output_bg,
        bm_device_mem_t     output_model,
        bmcv_ive_gmm_ctrl   gmm_attr);

| 【参数】

.. list-table:: bmcv_ive_gmm 参数表
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
    * - \output_fg
      - 输出
      - 输出 bm_image 对象结构体, 表示前景图像, 不能为空, 宽、高同 input。
    * - \output_bg
      - 输出
      - 输出 bm_image 对象结构体, 表示背景图像, 不能为空, 宽、高同 input。
    * - \output_model
      - 输入、输出
      - bm_device_mem_t 对象结构体, 对应GMM 模型参数, 不能为空。
    * - gmm_attr
      - 输入
      - 控制信息结构体，不能为空。

.. list-table::
    :widths: 25 42 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY

        RGB_PACKED
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - output_fg
      - GRAY 二值图
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input
    * - output_bg
      - 同 input
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input

| 【数据类型说明】

【说明】定义 GMM 背景建模的控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_gmm_ctrl_s{
        unsigned int u22q10_noise_var;
        unsigned int u22q10_max_var;
        unsigned int u22q10_min_var;
        unsigned short u0q16_learn_rate;
        unsigned short u0q16_bg_ratio;
        unsigned short u8q8_var_thr;
        unsigned short u0q16_init_weight;
        unsigned char u8_model_num;
    } bmcv_ive_gmm_ctrl;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - u22q10_noise_var
      - 初始噪声方差。

        取值范围：[0x1, 0xFFFFFF]。
    * - u22q10_max_var
      - 模型方差的最大值。

        取值范围：[0x1, 0xFFFFFF]。
    * - u22q10_min_var
      - 模型方差的最小值。

        取值范围：[0x1, u22q10MaxVar]。
    * - u0q16_learn_rate
      - 学习速率。

        取值范围: [1, 65535]。
    * - u0q16_bg_ratio
      - 背景比例阈值。

        取值范围: [1, 65535]。
    * - u8q8_var_thr
      - 方差阈值。

        取值范围: [1, 65535]。
    * - u0q16_init_weight
      - 初始权重。

        取值范围: [1, 65535]。
    * - u8_model_num
      - 模型个数。

        取值范围: {3, 5}。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入输出图像的 width 都需要16对齐。

2. GMM 的实现方式参考了 OpenCV 中的 MOG 和 MOG2。

3. 源图像类型只能为 U8C1 或 U8C3_PACKAGE, 分别用于灰度图和 RGB 图的 GMM 背景建模。

4. 前景图像是二值图，类型只能为 U8C1; 背景图像与源图像类型一致。

5. 灰度图像 或 RGB 图像 GMM 采用 n 个(n=3 或 5)高斯模型。