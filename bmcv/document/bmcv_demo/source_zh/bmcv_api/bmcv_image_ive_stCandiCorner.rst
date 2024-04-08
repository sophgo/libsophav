bmcv_ive_stcandicorner
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 完成灰度图像 Shi-Tomasi-like 角点（两条边缘的交点）计算，角点在任意一个方向上做微小移动，都会引起该区域的梯度图的方向和幅值发生很大变化。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_stcandicorner(
    bm_handle_t                 handle,
    bm_image                    input,
    bm_image                    output,
    bmcv_ive_stcandicorner_attr   stcandicorner_attr);

| 【参数】

.. list-table:: bmcv_ive_stcandicorner 参数表
    :widths: 25 15 35

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
      - 输出 bm_image 对象数据结构体, 候选角点响应值图像指针, 不能为空, 宽、高同 input。
    * - \stcandicorner_attr
      - 输入
      - Shi-Tomas-like 候选角点计算控制参数结构体, 不能为空。

.. list-table::
    :widths: 25 18 62 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64～1920x1080
    * - output
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input

【数据类型说明】

【说明】定义 Shi-Tomas-like 候选角点计算控制参数。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_stcandicorner_attr_s{
        bm_device_mem_t st_mem;
        unsigned char   u0q8_quality_level;
    } bmcv_ive_stcandicorner_attr;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - st_mem
      - 内存配置大小见 bmcv_image_ive_stcandicorner 的注意。
    * - u0q8_quality_level
      - ShiTomasi 角点质量控制参数，角点响应值小于 u0q8_quality_level * 最大角点响应值 的点将直接被确认为非角点。

        取值范围: [1, 255], 参考取值: 25。

【说明】定义 Shi-Tomas-like 角点计算时最大角点响应值结构体。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_ive_st_max_eig_s{
        unsigned short u16_max_eig;
        unsigned char  u8_reserved[14];
    } bmcv_ive_st_max_eig;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - u16_max_eig
      - 最大角点响应值。
    * - u8_reserved
      - 保留位。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 与 OpenCV 中 ShiTomas 角点计算原理类似。

3. stcandicorner_attr.st_mem 至少需要开辟的内存大小:

     stcandicorner_attr.st_mem.size = 4 * input_stride * input.height + sizoef(bm_ive_st_max_eig)。

4. 该任务完成后, 得到的并不是真正的角点, 还需要进行下一步计算。