bmcv_ive_dilate
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建二值图像 5x5 模板膨胀任务，将模板覆盖的区域中的最大像素值赋给参考点。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_dilate(
        bm_handle_t           handle,
        bm_image              input,
        bm_image              output,
        unsigned char         dilate_mask[25]);

| 【参数】

.. list-table:: bmcv_ive_dilate 参数表
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
      - 输出 bm_image 对象结构体，不能为空，宽、高同 input。
    * - \dilate_mask
      - 输入
      - 5x5 膨胀模板系数数组，不能为空，取值范围: 0 或 255。

| 【数据类型说明】

.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY 的二值图
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080
    * - output
      - GRAY 的二值图
      - DATA_TYPE_EXT_1N_BYTE
      - 同 input

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 输入输出图像的 width 都需要16对齐。

2. 模板系数只能为 0 或 255。

3. 接口可以处理灰度图的输入，对于处理的结果不可保证，所以不提倡，但是也可以尝试。

4. 模板样例

  .. raw:: latex

   \begin{align*}
   \begin{bmatrix}
      0 & 0 & 0 & 0 & 0 \\
      0 & 0 & 255 & 0 & 0 \\
      0 & 255 & 255 & 255 & 0 \\
      0 & 0 & 255 & 0 & 0 \\
      0 & 0 & 0 & 0 & 0 \\
   \end{bmatrix}
   \hspace{60pt} % 控制间隔
   \begin{bmatrix}
      0 & 0 & 255 & 0 & 0 \\
      0 & 0 & 255 & 0 & 0 \\
      255 & 255 & 255 & 255 & 255 \\
      0 & 0 & 255 & 0 & 0 \\
      0 & 0 & 255 & 0 & 0 \\
   \end{bmatrix}
   \end{align*}


  .. raw:: latex

   \begin{align*}
   \begin{bmatrix}
      0 & 0 & 0 & 0 & 0 \\
      0 & 255 & 255 & 255 & 0 \\
      0 & 255 & 255 & 255 & 0 \\
      0 & 255 & 255 & 255 & 0 \\
      0 & 0 & 0 & 0 & 0 \\
   \end{bmatrix}
   \hspace{60pt} % 控制间隔
   \begin{bmatrix}
      0 & 0 & 255 & 0 & 0 \\
      0 & 255 & 255 & 255 & 0 \\
      255 & 255 & 255 & 255 & 255 \\
      0 & 255 & 255 & 255 & 0 \\
      0 & 0 & 255 & 0 & 0 \\
   \end{bmatrix}
   \end{align*}

  .. raw:: latex

   \begin{align*}
   \begin{bmatrix}
      0 & 255 & 255 & 255 & 0 \\
      255 & 255 & 255 & 255 & 255 \\
      255 & 255 & 255 & 255 & 255 \\
      255 & 255 & 255 & 255 & 255 \\
      0 & 255 & 255 & 255 & 0 \\
   \end{bmatrix}
   \hspace{60pt} % 控制间隔
   \begin{bmatrix}
      255 & 255 & 255 & 255 & 255 \\
      255 & 255 & 255 & 255 & 255 \\
      255 & 255 & 255 & 255 & 255 \\
      255 & 255 & 255 & 255 & 255 \\
      255 & 255 & 255 & 255 & 255 \\
   \end{bmatrix}
   \end{align*}