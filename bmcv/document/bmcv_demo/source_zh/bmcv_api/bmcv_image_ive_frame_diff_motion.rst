bmcv_ive_frame_diff_motion
---------------------------------

| 【描述】

| 该 API 使用ive硬件资源,  创建背景相减任务。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_frame_diff_motion(
        bm_handle_t                     handle,
        bm_image                        input1,
        bm_image                        input2,
        bm_image                        output,
        bmcv_ive_frame_diff_motion_attr attr);

| 【参数】

.. list-table:: bmcv_ive_frame_diff_motion 参数表
    :widths: 20 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input1
      - 输入
      - 输入 bm_image 对象结构体，不能为空。
    * - \input2
      - 输入
      - 输入 bm_image 对象结构体，不能为空。
    * - \output
      - 输出
      - 输出 bm_image 对象结构体，不能为空。
    * - \attr
      - 输入
      - 控制参数结构体，不能为空。

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
      - 同 input

| 【数据类型说明】

【说明】定义该算子控制信息。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_frame_diff_motion_attr_s{
        bmcv_ive_sub_mode sub_mode;
        bmcv_ive_thresh_mode thr_mode;
        unsigned char u8_thr_low;
        unsigned char u8_thr_high;
        unsigned char u8_thr_min_val;
        unsigned char u8_thr_mid_val;
        unsigned char u8_thr_max_val;
        unsigned char au8_erode_mask[25];
        unsigned char au8_dilate_mask[25];
    } bmcv_ive_frame_diff_motion_attr;

.. list-table::
    :widths: 40 100

    * - **成员名称**
      - **描述**
    * - sub_mode
      - 定义两张图像相减输出格式。
    * - thr_mode
      - 定义阈值化计算方式，仅支持 MOD_U8 模式，即 U8 数据到 U8 数据的阈值化。
    * - u8_thr_low
      - 低阈值，取值范围：[0, 255]。
    * - u8_thr_high
      - 表示高阈值， 0 ≤ u8ThrLow ≤ u8ThrHigh ≤ 255。
    * - u8_thr_min_val
      - 表示最小值，取值范围：[0, 255]。
    * - u8_thr_mid_val
      - 表示中间值，取值范围：[0, 255]。
    * - u8_thr_max_val
      - 表示最大值，取值范围：[0, 255]。
    * - au8_erode_mask[25]
      - 表示腐蚀的 5x5 模板系数，取值范围：0 或 255。
    * - au8_dilate_mask[25]
      - 表示膨胀的 5x5 模板系数，取值范围：0 或 255。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 输入输出图像的 width 都需要16对齐。