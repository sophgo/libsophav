bm_image_get_stride
-------------------

| 【描述】
| 该接口用于获取目标 bm_image 的 stride 信息。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bm_image_get_stride(
            bm_image image,
            int *stride
    );

| 【参数】

.. list-table:: bm_image_get_stride 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - image
      - 输入
      - 所要获取 stride 信息的目标 bm_image。
    * - \*stride
      - 输出
      - 存放各个 plane 的 stride 的指针。

| 【返回值】

函数调用成功时返回BM_SUCCESS。