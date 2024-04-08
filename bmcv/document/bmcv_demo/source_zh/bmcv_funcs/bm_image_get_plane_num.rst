bm_image_get_plane_num
----------------------

| 【描述】
| 该接口用于获取目标 bm_image 的 plane 数量。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    int bm_image_get_plane_num(bm_image image);

| 【参数】

.. list-table:: bm_image_get_plane_num 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - image
      - 输入
      - 所要获取 plane 数量的目标 bm_image。

| 【返回值】

返回值即为目标 bm_image 的 plane 数量。