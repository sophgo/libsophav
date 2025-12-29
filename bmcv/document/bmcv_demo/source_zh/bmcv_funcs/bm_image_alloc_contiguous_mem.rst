bm_image_alloc_contiguous_mem
-----------------------------

| 【描述】
| 为多个 image 分配连续的内存。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bm_image_alloc_contiguous_mem(
            int           image_num,
            bm_image      *images
    );

| 【参数】

.. list-table:: bm_image_alloc_contiguous_mem 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - image_num
      - 输入
      - 待分配内存的 image 个数。
    * - \*images
      - 输入
      - 待分配内存的 image 的指针。

| 【返回值】

函数调用成功时返回 BM_SUCCESS。

| 【注意事项】

1. image_num 应该大于 0,否则将返回错误。

#. 如传入的 image 已分配或者 attach 过内存，应先 detach 已有内存，否则将返回失败。

#. 所有待分配的 image 应该尺寸相同，否则将返回错误。

#. 当希望 destory 的 image 是通过调用本 api 所分配的内存时，应先调用 bm_image_free_contiguous_mem 将分配内存释放，再用 bm_image_destroy 来实现 destory image

#. bm_image_alloc_contiguous_mem 与 bm_image_free_contiguous_mem 应成对使用。