bmcv_dpu_fgs_disp
------------------------------

| 【描述】

| 该 API 使用 DPU 硬件资源, 快速全局平滑算法FGS(Fast Global Smoothing)。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_dpu_fgs_disp(
        bm_handle_t handle,
        bm_image *guide_image,
        bm_image *smooth_image,
        bm_image *disp_image,
        bmcv_dpu_fgs_attrs *fgs_attr,
        bmcv_dpu_fgs_mode fgs_mode);

| 【参数】

.. list-table:: bmcv_dpu_fgs_disp 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \*guide_image
      - 输入
      - bm_image 类型，指导图指针, 不能为空。
    * - \*smooth_image
      - 输入
      - bm_image 类型，视差图指针, 不能为空，宽、高 同 guide_image。
    * - \*disp_image
      - 输出
      - bm_image 类型，平滑图, 不能为空, 宽、高 同 guide_image。
    * - \*fgs_attr
      - 输入
      - bmcv_dpu_fgs_attrs 类型，FGS 部分控制参数。
    * - fgs_mode
      - 输入
      - DPU FGS 输出模式。

.. list-table::
    :widths: 30 25 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - guide_image
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - smooth_image
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 guide_image
    * - disp_image
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE

        DATA_TYPE_EXT_U16
      - 同 guide_image

| 【数据类型说明】

【说明】定义 DPU FGS 输出模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_dpu_fgs_mode_{
        DPU_FGS_MUX0 = 7,
        DPU_FGS_MUX1 = 8,
    } bmcv_dpu_fgs_mode;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - DPU_FGS_MUX0
      - 使用FGS处理左图和右图，输出一张8bit视差图（也可用于图像的降噪，类似于引导滤波）。
    * - DPU_FGS_MUX1
      - 使用FGS处理左图和右图，输出一张16bit深度图。

【说明】定义 DPU FGS 深度单位。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_dpu_depth_unit_{
        BMCV_DPU_DEPTH_UNIT_DEFAULT,
        BMCV_DPU_DEPTH_UNIT_MM,
        BMCV_DPU_DEPTH_UNIT_CM,
        BMCV_DPU_DEPTH_UNIT_DM,
        BMCV_DPU_DEPTH_UNIT_M,
        BMCV_DPU_DEPTH_UNIT_BUTT
    } bmcv_dpu_depth_unit;

.. list-table::
    :widths: 100 100

    * - **成员名称**
      - **描述**
    * - BMCV_DPU_DEPTH_UNIT_DEFAULT
      - 默认深度单位，mm。
    * - BMCV_DPU_DEPTH_UNIT_MM
      - 深度单位是 mm。
    * - BMCV_DPU_DEPTH_UNIT_CM
      - 深度单位是 cm。
    * - BMCV_DPU_DEPTH_UNIT_DM
      - 深度单位是 dm。
    * - BMCV_DPU_DEPTH_UNIT_M
      - 深度单位是 m。
    * - BMCV_DPU_DEPTH_UNIT_BUTT
      - 枚举数组最大值，用于判断输入是否在范围内。

【说明】定义 DPU FGS 控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_dpu_fgs_attrs_{
        unsigned int         fgs_max_count;
        unsigned int         fgs_max_t;
        unsigned int         fxbase_line;
        bmcv_dpu_depth_unit  depth_unit_en;
    } bmcv_dpu_fgs_attrs;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - fgs_max_count
      - Fgs中转变为0的最大次数。
    * - fgs_max_t
      - Fgs的最大迭代次数（至少设置为2），由于 FGS 计算量较大，一般设置为 3。
    * - fxbase_line
      - 左右摄像头的基线值。
    * - depth_unit_en
      - 深度的度量单位，取值可参考 bmcv_dpu_depth_unit 说明。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 左右图像的 height 和 width 必须相同。

2. 左右图像的 width 要求 4 对齐，height 要求 2 对齐，输入图 stride 需要 16 对齐，输出图 stride 需要 32 对齐。

3. Fgs的最大迭代次数至少设置为 2。