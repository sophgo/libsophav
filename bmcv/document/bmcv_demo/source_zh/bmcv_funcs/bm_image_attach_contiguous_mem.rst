bm_image_attach_contiguous_mem
------------------------------

| 【描述】
| 将一块连续内存 attach 到多个 image 中。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bm_image_attach_contiguous_mem(
            int image_num,
            bm_image * images,
            bm_device_mem_t dmem
    );

| 【参数】

.. list-table:: bm_image_attach_contiguous_mem 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - image_num
      - 输入
      - 待 attach 内存的 image 个数。
    * - \*images
      - 输入
      - 待 attach 内存的 image 的指针。
    * - dmem
      - 输入
      - 已分配好的 device memory 信息。

| 【返回值】

函数调用成功时返回BM_SUCCESS。

| 【注意事项】

1. image_num 应该大于 0，否则将返回错误。

#. 所有待 attach 的 image 应该尺寸相同，否则将返回错误。

