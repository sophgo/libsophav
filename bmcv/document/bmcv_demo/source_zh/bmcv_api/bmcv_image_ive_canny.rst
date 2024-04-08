bmcv_ive_canny
------------------------------

| 【描述】

| 该 API 使用ive硬件资源的 ccl 功能, 计算 canny 边缘图。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_canny(
        bm_handle_t                  handle,
        bm_image                     input,
        bm_device_mem_t              output_edge,
        bmcv_ive_canny_hys_edge_ctrl   canny_hys_edge_attr);

| 【参数】

.. list-table:: bmcv_ive_canny 参数表
    :widths: 20 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input
      - 输入
      - 输入 bm_image 对象结构体，不能为空。
    * - \output_edge
      - 输出
      - 输出 bm_device_mem_t 对象结构体，不能为空。
    * - \canny_hys_edge_attr
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

| 【数据类型说明】

【说明】定义 Canny 边缘前半部分计算任务的控制参数 。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_canny_hys_edge_ctrl_s{
        bm_device_mem_t  st_mem;
        unsigned short u16_low_thr;
        unsigned short u16_high_thr;
        signed char as8_mask[25];
    } bmcv_ive_canny_hys_edge_ctrl;

.. list-table::
    :widths: 45 100

    * - **成员名称**
      - **描述**
    * - stmem
      - 辅助内存。
    * - u16_low_thr
      - 低阈值，取值范围：[0, 255]。
    * - u16_high_thr
      - 高阈值，取值范围: [u16LowThr, 255]。
    * - as8_mask
      - 用于计算梯度的参数模板。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 输入输出图像的 width 都需要16对齐。

2. canny_hys_edge_attr.st_mem 至少需要分配的内存大小:

   canny_hys_edge_attr.st_mem.size = input_stride * 3 * input.height。