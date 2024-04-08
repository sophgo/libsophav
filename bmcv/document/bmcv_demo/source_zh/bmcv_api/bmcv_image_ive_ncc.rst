bmcv_ive_ncc
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建两相同分辨率灰度图像的归一化互相关系数计算任务。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_ncc(
    bm_handle_t       handle,
    bm_image          input1,
    bm_image          input2,
    bm_device_mem_t   output);

| 【参数】

.. list-table:: bmcv_ive_ncc 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - input1
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - input2
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - output
      - 输出
      - 输出 数据 对象结构体, 不能为空, 内存至少需要配置: sizeof(bmcv_ive_ncc_dst_mem_t)。

.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input1
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 32x32~1920x1080
    * - input2
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input1
    * - output
      - --
      - --
      - --

| 【数据类型说明】

【说明】定义LBP计算的比较模式。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct _bmcv_ive_ncc_dst_mem_s{
        unsigned long long u64_numerator;
        unsigned long long u64_quad_sum1;
        unsigned long long u64_quad_sum2;
        unsigned char u8_reserved[8];
    }bmcv_ive_ncc_dst_mem_t;

.. list-table::
    :widths: 60 100

    * - **成员名称**
      - **描述**
    * - u64_numerator
      - :math:`\sum_{i = 1}^{w} \sum_{j = 1}^{h} (I_{\text{src1}}(i, j) * I_{\text{src2}}(i, j))`
    * - u64_quad_sum1
      - :math:`\sum_{i = 1}^{w} \sum_{j = 1}^{h} I_{\text{src1}}^{2}(i, j)`
    * - u64_quad_sum2
      - :math:`\sum_{i = 1}^{w} \sum_{j = 1}^{h} I_{\text{src2}}^{2}(i, j)`
    * - u8_reserved
      - 保留字段。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

* 输入图像的 width 都需要16对齐。

* 计算公式如下：
   .. math::

       & NCC(I_{\text{src1}}, I_{\text{src2}}) =
         \frac{\sum_{i = 1}^{w} \sum_{j = 1}^{h} (I_{\text{src1}}(i, j) * I_{\text{src2}}(i, j))}
         {\sqrt{\sum_{i = 1}^{w} \sum_{j = 1}^{h} I_{\text{src1}}^{2}(i, j)}
          \sqrt{\sum_{i = 1}^{w} \sum_{j = 1}^{h} I_{\text{src2}}^{2}(i, j)}}