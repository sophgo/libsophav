bmcv_dpu_sgbm_disp
------------------------------

| 【描述】

| 该 API 使用 DPU 硬件资源, 实现半全局块匹配算法 SGBM(Semi-Global Block Maching)。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_dpu_sgbm_disp(
        bm_handle_t handle,
        bm_image *left_image,
        bm_image *right_image,
        bm_image *disp_image,
        bmcv_dpu_sgbm_attrs *dpu_attr,
        bmcv_dpu_sgbm_mode sgbm_mode);

| 【参数】

.. list-table:: bmcv_dpu_sgbm_disp 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - \*left_image
      - 输入
      - bm_image 类型，参考图指针, 不能为空。
    * - \*right_image
      - 输入
      - bm_image 类型，搜索图指针, 不能为空，宽、高 同 left_image。
    * - \*disp_image
      - 输出
      - bm_image 类型，视差图, 不能为空, 宽、高 同 left_image。
    * - \*dpu_attr
      - 输入
      - bmcv_dpu_sgbm_attrs 类型，SGBM 部分控制参数。
    * - sgbm_mode
      - 输入
      - DPU SGBM 输出模式。

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

【说明】定义 DPU SGBM 输出模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_dpu_sgbm_mode_{
        DPU_SGBM_MUX0 = 1,
        DPU_SGBM_MUX1 = 2,
        DPU_SGBM_MUX2 = 3,
    } bmcv_dpu_sgbm_mode;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - DPU_SGBM_MUX0
      - 使用SGBM处理左图和右图，输出一张没有经过后处理的8bit视差图。
    * - DPU_SGBM_MUX1
      - 使用SGBM处理左图和右图，输出一张经过后处理的16bit视差图。
    * - DPU_SGBM_MUX2
      - 使用SGBM处理左图和右图，输出一张经过后处理的8bit视差图。

【说明】定义 DPU SGBM 右图搜索范围。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_dpu_disp_range_{
        BMCV_DPU_DISP_RANGE_DEFAULT,
        BMCV_DPU_DISP_RANGE_16,
        BMCV_DPU_DISP_RANGE_32,
        BMCV_DPU_DISP_RANGE_48,
        BMCV_DPU_DISP_RANGE_64,
        BMCV_DPU_DISP_RANGE_80,
        BMCV_DPU_DISP_RANGE_96,
        BMCV_DPU_DISP_RANGE_112,
        BMCV_DPU_DISP_RANGE_128,
        BMCV_DPU_DISP_RANGE_BUTT
    } bmcv_dpu_disp_range;

.. list-table::
    :widths: 100 100

    * - **成员名称**
      - **描述**
    * - BMCV_DPU_DISP_RANGE_DEFAULT
      - 默认右图搜索范围为 16 pixel。
    * - BMCV_DPU_DISP_RANGE_16
      - 右图搜索范围为 16 pixel。
    * - BMCV_DPU_DISP_RANGE_32
      - 右图搜索范围为 32 pixel。
    * - BMCV_DPU_DISP_RANGE_48
      - 右图搜索范围为 48 pixel。
    * - BMCV_DPU_DISP_RANGE_64
      - 右图搜索范围为 64 pixel。
    * - BMCV_DPU_DISP_RANGE_80
      - 右图搜索范围为 80 pixel。
    * - BMCV_DPU_DISP_RANGE_96
      - 右图搜索范围为 96 pixel。
    * - BMCV_DPU_DISP_RANGE_112
      - 右图搜索范围为 112 pixel。
    * - BMCV_DPU_DISP_RANGE_128
      - 右图搜索范围为 128 pixel。
    * - BMCV_DPU_DISP_RANGE_BUTT
      - 枚举数组最大值，用于判断输入是否在范围内。

【说明】定义 DPU SGBM BoxFilter 模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_dpu_bfw_mode_{
        DPU_BFW_MODE_DEFAULT,
        DPU_BFW_MODE_1x1,
        DPU_BFW_MODE_3x3,
        DPU_BFW_MODE_5x5,
        DPU_BFW_MODE_7x7,
        DPU_BFW_MODE_BUTT
    } bmcv_dpu_bfw_mode;

.. list-table::
    :widths: 80 100

    * - **成员名称**
      - **描述**
    * - DPU_BFW_MODE_DEFAULT
      - 默认BoxFilter窗口大小为 7x7。
    * - DPU_BFW_MODE_1x1
      - BoxFilter窗口大小为 1x1。
    * - DPU_BFW_MODE_3x3
      - BoxFilter窗口大小为 3x3。
    * - DPU_BFW_MODE_5x5
      - BoxFilter窗口大小为 5x5。
    * - DPU_BFW_MODE_7x7
      - BoxFilter窗口大小为 7x7。
    * - DPU_BFW_MODE_BUTT
      - 枚举数值的最大值，用于判断输入的枚举值是否在范围内。

【说明】定义 DPU SGBM DCC 代价聚合的模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum bmcv_dpu_dcc_dir_{
        BMCV_DPU_DCC_DIR_DEFAULT,
        BMCV_DPU_DCC_DIR_A12,
        BMCV_DPU_DCC_DIR_A13,
        BMCV_DPU_DCC_DIR_A14,
        BMCV_DPU_DCC_DIR_BUTT
    } bmcv_dpu_dcc_dir;

.. list-table::
    :widths: 80 100

    * - **成员名称**
      - **描述**
    * - BMCV_DPU_DCC_DIR_DEFAULT
      - DCC 默认代价聚合方向：A1+A2。
    * - BMCV_DPU_DCC_DIR_A12
      - DCC 代价聚合方向：A1+A2。
    * - BMCV_DPU_DCC_DIR_A13
      - DCC 代价聚合方向：A1+A3。
    * - BMCV_DPU_DCC_DIR_A14
      - DCC 代价聚合方向：A1+A4。
    * - DPU_BFW_MODE_BUTT
      - 枚举数值的最大值，用于判断输入的枚举值是否在范围内。

【说明】定义 DPU SGBM 控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_dpu_sgbm_attrs_{
        bmcv_dpu_bfw_mode    bfw_mode_en;
        bmcv_dpu_disp_range  disp_range_en;
        unsigned short       disp_start_pos;
        unsigned int         dpu_census_shift;
        unsigned int         dpu_rshift1;
        unsigned int         dpu_rshift2;
        bmcv_dpu_dcc_dir     dcc_dir_en;
        unsigned int         dpu_ca_p1;
        unsigned int         dpu_ca_p2;
        unsigned int         dpu_uniq_ratio;
        unsigned int         dpu_disp_shift;
    } bmcv_dpu_sgbm_attrs;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - bfw_mode_en
      - DPU SGBM BoxFilter窗口大小，取值可参考 bmcv_dpu_bfw_mode 的说明。
    * - disp_range_en
      - 右图搜索范围，取值可参考 bmcv_dpu_disp_range 的说明。
    * - disp_start_pos
      - 右图搜索的起始位置。
    * - dpu_census_shift
      - Census Transform 偏移量。
    * - dpu_rshift1
      - 原图的BTcost map的权值。
    * - dpu_rshift2
      - Census Transform的BTcost map的权值。
    * - dcc_dir_en
      - DCC 代价聚合方向，取值可参考 bmcv_dpu_dcc_dir 的说明。
    * - dpu_ca_p1
      - DCC 代价聚合 P1 惩罚因子。
    * - dpu_ca_p2
      - DCC 代价聚合 P2 惩罚因子。
    * - dpu_uniq_ratio
      - 唯一性检查因子，取值范围:[0, 100]。
    * - dpu_disp_shift
      - 视差偏移量。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 右图像的 width 必须大于或等于视差搜索的起始位置与视差搜索范围的和，即： :math:`right\_image->width \geq disp\_start\_pos + disp\_range\_en`。

2. 左右图像的 height 和 width 必须相同。

3. 左右图像的 width 要求 4 对齐，height 要求 2 对齐，stride 需要 16 对齐。