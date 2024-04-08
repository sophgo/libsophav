
bmcv_ive_bersen
------------------------------

| 【描述】

| 该 API 使用ive硬件资源,  实现利用 Bernsen 二值法，对图像进行二值化求解，实现图像的二值化。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_bernsen(
        bm_handle_t           handle,
        bm_image              input,
        bm_image              output,
        bmcv_ive_bernsen_attr attr);

| 【参数】

.. list-table:: bmcv_ive_bersen 参数表
    :widths: 20 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \input
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - \output
      - 输出
      - 输出 bm_image 对象结构体, 不能为空。
    * - \attr
      - 输入
      - 控制参数结构体, 不能为空。

.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 16x16~1920x1080
    * - output
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input

| 【数据类型说明】

【说明】定义 Bernsen 阈值计算的不同方式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_ive_bernsen_mode_e{
        BM_IVE_BERNSEN_NORMAL = 0x0,
        BM_IVE_BERNSEN_THRESH = 0x1,
        BM_IVE_BERNSEN_PAPER = 0x2,
    } bmcv_ive_bernsen_mode;

【说明】定义 Bernsen 二值法的控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_bernsen_attr_s{
        bmcv_ive_bernsen_mode en_mode;
        unsigned char u8_win_size;
        unsigned char u8_thr;
        unsigned char u8_contrast_threshold;
    } bmcv_ive_bernsen_attr;

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 输入输出图像的 width 都需要16对齐。