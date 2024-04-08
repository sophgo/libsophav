bmcv_ive_csc
------------------------------

| 【描述】

| 该 API 使用ive硬件资源的 csc 功能, 创建色彩空间转换任务, 可实现YUV2RGB、YUV2HSV、RGB2YUV 的色彩空间转换。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_csc(
            bm_handle_t     handle,
            bm_image        input,
            bm_image        output,
            csc_type_t      csc_type);

| 【参数】

.. list-table:: bmcv_ive_csc 参数表
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
      - 输出 bm_image 对象结构体, 不能为空, 宽、高同 input。
    * - csc_type
      - 输入
      - csc 工作模式。

.. list-table::
    :widths: 20 56 26 32
    :align: center

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - FORMAT_NV21

        FORMAT_NV61

        FORMAT_RGB_PACKED

        FORMAT_RGB_PLANAR

        FORMAT_RGBP_SEPARATE

      - U8
      - 64x64~1920x1080
    * - output
      - FORMAT_NV21

        FORMAT_NV61

        FORMAT_RGB_PACKED

        FORMAT_RGB_PLANAR

        FORMAT_RGBP_SEPARATE

      - U8
      - 同 input

【补充】FORMAT_YUV444P可以转FORMAT_HSV_PLANAR，转换模式色彩空间转换控制模式选择CSC_YCbCr2RGB_BT601或者CSC_YCbCr2RGB_BT709。


| 【数据类型说明】

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
        CSC_FANCY_PbPr_BT601 = 100,
        CSC_FANCY_PbPr_BT709,
        CSC_USER_DEFINED_MATRIX = 1000,
        CSC_MAX_ENUM
    } csc_type_t;

.. list-table::
    :widths: 100 100

    * - **成员名称**
      - **描述**
    * - CSC_YCbCr2RGB_BT601
      - BT601 的 YUV2RGB 图片变换

    * - CSC_YPbPr2RGB_BT601
      - BT601 的 YUV2RGB 视频变换。

    * - CSC_RGB2YCbCr_BT601
      - BT601 的 RGB2YUV 图像变换。

    * - CSC_YCbCr2RGB_BT709
      - BT709 的 YUV2RGB 图像变换。

    * - CSC_RGB2YCbCr_BT709
      - BT709 的 RGB2YUV 图像变换。

    * - CSC_RGB2YPbPr_BT601
      - BT601 的 RGB2YUV 视频变换。

    * - CSC_YPbPr2RGB_BT709
      - BT709 的 YUV2RGB 视频变换。

    * - CSC_RGB2YPbPr_BT709
      - BT709 的 RGB2YUV 视频变换。


【注意】

* CSC_YPbPr2RGB_BT601 和 CSC_YPbPr2RGB_BT709 模式，输出满足 16≤R、 G、 B≤235。

* CSC_YCbCr2RGB_BT601 和 CSC_YCbCr2RGB_BT709 模式，输出满足 0≤R、 G、 B≤255。

* CSC_RGB2YPbPr_BT601 和 CSC_RGB2YPbPr_BT709 模式，输出满足 0≤Y、 U、 V≤ 255。

* CSC_RGB2YCbCr_BT601 和 CSC_RGB2YCbCr_BT709 模式。, 输出满足 0≤Y≤235, 0≤U、 V≤240。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 当输出数据为 FORMAT_RGB_PLANAR、 FORMAT_NV21、 FORMAT_NV61 类型时， 要求输出数据跨度一致。

3. 不同的模式其输出的取值范围不一样。
