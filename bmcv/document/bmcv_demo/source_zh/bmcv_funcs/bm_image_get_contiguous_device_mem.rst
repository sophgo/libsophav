bm_image_get_contiguous_device_mem
----------------------------------

| 【描述】
| 从多个内存连续的 image 中得到连续内存的 device memory 信息。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bm_image_get_contiguous_device_mem(
            int image_num,
            bm_image *images,
            bm_device_mem_t * mem
    );

| 【参数】

.. list-table:: bm_image_get_contiguous_device_mem 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - image_num
      - 输入
      - 待获取信息的 image 个数。
    * - \*images
      - 输入
      - 待获取信息的 image 指针。
    * - \*mem
      - 输出
      - 得到的连续内存的 device memory 信息。

| 【返回值】

函数调用成功时返回BM_SUCCESS。

| 【注意事项】

1. image_num 应该大于 0，否则将返回错误。

#. 所填入的 image 应该尺寸相同，否则将返回错误。

#. 所填入的 image 必须是内存连续的，否则返回错误。

#. 所填入的 image 内存必须是通过 bm_image_alloc_contiguous_mem 或者 bm_image_attach_contiguous_mem 获得。