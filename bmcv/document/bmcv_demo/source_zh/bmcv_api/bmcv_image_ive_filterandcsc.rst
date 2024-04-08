bmcv_ive_filter_and_csc
------------------------------

| 【描述】

| 该 API 使用ive硬件资源,  创建 5x5 模板滤波和 YUV2RGB 色彩空间转换复合任务，通过一次创建完成两种功能。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_filter_and_csc(
        bm_handle_t             handle,
        bm_image                input,
        bm_image                output,
        bmcv_ive_filter_ctrl    filter_attr,
        csc_type_t              csc_type);

| 【参数】

.. list-table:: bmcv_ive_filter_and_csc 参数表
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
    * - \output
      - 输出
      - 输出 bm_image 对象结构体，不能为空，宽、高同 input。
    * - \filter_attr
      - 输入
      - 滤波控制参数结构体，不能为空。
    * - \csc_type
      - 输入
      - 色彩转换模式选择，不能为空。

.. list-table::
    :widths: 20 40 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - NV21

        NV61
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - output
      - RGB_PLANAR

        RGB_PACKAGE
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input

| 【数据类型说明】

【说明】定义 模板滤波加色彩空间转换符合功能控制信息。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_filter_ctrl_s{
        signed char as8_mask[25];
        unsigned char u8_norm;
    } bmcv_ive_filter_ctrl;

.. list-table::
    :widths: 58 100

    * - **成员名称**
      - **描述**
    * - as8_mask
      - 5x5 模板系数。
    * - u8_norm
      - 归一化参数，取值范围：[0， 13]。


【说明】定义色彩空间转换控制模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum csc_type {
        CSC_YCbCr2RGB_BT601 = 0,
        CSC_YPbPr2RGB_BT601,
        CSC_RGB2YCbCr_BT601,
        CSC_YCbCr2RGB_BT709,
        CSC_RGB2YCbCr_BT709,
        CSC_RGB2YPbPr_BT601,
        CSC_YPbPr2RGB_BT709,
        CSC_RGB2YPbPr_BT709,
        CSC_YCbCr2HSV_BT601,
        CSC_YCbCr2HSV_BT709,
        CSC_YCbCr2LAB_BT601,
        CSC_YCbCr2LAB_BT709,
        CSC_RGB2HSV,
        CSC_RGB2GRAY,
        CSC_FANCY_PbPr_BT601 = 100,
        CSC_FANCY_PbPr_BT709,
        CSC_USER_DEFINED_MATRIX = 1000,
        CSC_MAX_ENUM
    } csc_type_t;


| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 仅支持 YUV2RGB 的 4 种工作模式，具体参见 bmcv_ive_csc。

2. 输入输出图像的 width 都需要16对齐。