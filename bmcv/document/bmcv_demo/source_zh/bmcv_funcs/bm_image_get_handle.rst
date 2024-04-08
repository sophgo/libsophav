bm_image_get_handle
--------------------

| 【描述】
| 该接口用于通过 bm_image 获取句柄 handle。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_handle_t bm_image_get_handle(bm_image* image);

| 【参数】

.. list-table:: bm_image_get_handle 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - image
      - 输入
      - 所要获取 handle 的目标 bm_image。

| 【返回值】

返回值即为目标 bm_image 的句柄 handle。