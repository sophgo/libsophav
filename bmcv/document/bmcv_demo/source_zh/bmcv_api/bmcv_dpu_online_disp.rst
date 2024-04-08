bmcv_dpu_online_disp
------------------------------

| 【描述】

| 该 API 使用 DPU 硬件资源, 实现半全局块匹配算法 SGBM(Semi-Global Block Maching) 跟 快速全局平滑算法FGS(Fast Global Smoothing)。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_dpu_online_disp(
        bm_handle_t handle,
        bm_image *left_image,
        bm_image *right_image,
        bm_image *disp_image,
        bmcv_dpu_sgbm_attrs *dpu_attr,
        bmcv_dpu_fgs_attrs *fgs_attr,
        bmcv_dpu_online_mode online_mode);

| 【参数】

.. list-table:: bmcv_dpu_online_disp 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \*left_image
      - 输入
      - bm_image 类型，指导图指针, 不能为空。
    * - \*right_image
      - 输入
      - bm_image 类型，平滑图指针, 不能为空，宽、高 同 left_image。
    * - \*disp_image
      - 输出
      - bm_image 类型，视差图, 不能为空, 宽、高 同 left_image。
    * - \*dpu_attr
      - 输入
      - bmcv_dpu_sgbm_attrs 类型，SGBM 部分控制参数。
    * - \*fgs_attr
      - 输入
      - bmcv_dpu_fgs_attrs 类型，FGS 部分控制参数。
    * - online_mode
      - 输入
      - DPU online（SGBM_TO_FGS）输出模式。

.. list-table::
    :widths: 30 25 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - left_image
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - right_image
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 left_image
    * - disp_image
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE

        DATA_TYPE_EXT_U16
      - 同 left_image

| 【数据类型说明】

【说明】定义 DPU FGS 输出模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_dpu_online_mode_{
        DPU_ONLINE_MUX0 = 4,
        DPU_ONLINE_MUX1 = 5,
        DPU_ONLINE_MUX2 = 6,
    } bmcv_dpu_online_mode;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - DPU_ONLINE_MUX0
      - 该模式下，使用FGS处理左图和右图，输出一张8bit视差图（也可用于图像的降噪，类似于引导滤波）。
    * - DPU_ONLINE_MUX1
      - 该模式下，使用SGBM、FGS处理左图和右图，输出一张16bit深度图。
    * - DPU_ONLINE_MUX2
      - 该模式下，单独使用SGBM处理左图和右图，输出一张16bit深度图。

【注意】bmcv_dpu_sgbm_attrs 及 bmcv_dpu_fgs_attrs 类型的说明，详见bmcv_dpu_fgs_disp 及 bmcv_dpu_sgbm_disp。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 左右图像的 height 和 width 必须相同。

1. 左右图像的 width 要求 4 对齐，height 要求 2 对齐，输入图 stride 需要 16 对齐，输出图 stride 需要 32对齐。