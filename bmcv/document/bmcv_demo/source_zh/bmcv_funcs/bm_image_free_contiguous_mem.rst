bm_image_free_contiguous_mem
----------------------------

| 【描述】
| 释放通过 bm_image_alloc_contiguous_mem 所分配的多个 image 中的连续内存。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bm_image_free_contiguous_mem(
            int image_num,
            bm_image *images
    );

| 【参数】

.. list-table:: bm_image_free_contiguous_mem 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - image_num
      - 输入
      - 待释放内存的 image 个数。
    * - \*images
      - 输入
      - 待释放内存的 image 的指针。

| 【返回值】

函数调用成功时返回BM_SUCCESS。

| 【注意事项】

1. image_num 应该大于 0，否则将返回错误。

#. 所有待释放的 image 应该尺寸相同。

#. bm_image_alloc_contiguous_mem 与 bm_image_free_contiguous_mem 应成对使用。bm_image_free_contiguous_mem 所要释放的内存必须是通过 bm_image_alloc_contiguous_mem 所分配的。

#. 应先调用 bm_image_free_contiguous_mem,将 image 中内存释放，再调 bm_image_destroy 去 destory image。

