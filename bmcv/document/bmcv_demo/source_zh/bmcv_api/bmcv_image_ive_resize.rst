bmcv_ive_resize
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建图像缩放任务, 支持 双线性插值、区域插值 缩放, 支持多张 GRAY 与 RGB PLANAR 图像同时输入做一种类型的缩放。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_resize(
        bm_handle_t              handle,
        bm_image                 input,
        bm_image                 output,
        bmcv_resize_algorithm     resize_mode);

| 【参数】

.. list-table:: bmcv_ive_resize 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input
      - 输入
      - 输入 bm_image 对象结构体，不能为空。
    * - \output
      - 输出
      - 输出 bm_image 对象结构体，不能为空, 每张图像类型同input。
    * - resize_mode
      - 输入
      - 输入 bmcv_resize_algorithm 对象, 枚举控制信息。

.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY

        RGB_PLANAR
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - output
      - GRAY

        RGB_PLANAR
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080

| 【数据类型说明】

【说明】定义 Resize 模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_resize_algorithm_ {
        BMCV_INTER_NEAREST = 0,
        BMCV_INTER_LINEAR  = 1,
        BMCV_INTER_BICUBIC = 2,
        BMCV_INTER_AREA = 3,
    } bmcv_resize_algorithm;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - BMCV_INTER_NEAREST
      - 最近邻插值模式。
    * - IVE_RESIZE_AREA
      - 双线性插值缩放模式。
    * - IVE_RESIZE_LINEAR
      - 双三次插值模式。
    * - IVE_RESIZE_AREA
      - 区域插值缩放模式。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 支持 GRAY、RGB_PLANAR 混合图像数组输入, 但所有图像的缩放模式相同，同时模式只支持双线性插值和区域插值缩放模式。

2. 最大支持 16 倍缩放。

3. 输入输出图像的 width 都需要16对齐。