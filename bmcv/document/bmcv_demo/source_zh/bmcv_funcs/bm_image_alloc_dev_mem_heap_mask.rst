bm_image_alloc_dev_mem_heap_mask
--------------------------------

| 【描述】
| 该 API 为 bm_image 对象申请内部管理内存，所申请 device memory 大小为各个 plane 所需 device memory 大小之和。plane_byte_size 计算方法在 bm_image_copy_host_to_device 中已经介绍，或者通过调用 bm_image_get_byte_size API 来确认。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bm_image_alloc_dev_mem_heap_mask(
            bm_image      image,
            int           heap_mask
    );

| 【参数】

.. list-table:: bm_image_alloc_dev_mem_heap_mask 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - image
      - 输入
      - 待申请 device memory 的 bm_image 对象。
    * - heap_mask
      - 输入
      - 选择一个或多个 heap id 的掩码，每一个 bit 表示
        一个 heap id 的是否有效, 1 表示可以在该 heap 分配
        ，0 则表示不可以在该 heap 上分配，最低位表示
        heap0，以此类推。比如 heap_mask=0x10 则表示指定在
        heap1 上分配空间，heap_mask=0x101 则表示指定在 heap0 或者 heap2 上分配空间。

| 【注意事项】

1. 如果 bm_image 对象未创建，则返回失败。

2. 如果 image format 为 FORMAT_COMPRESSED，且该 bm_image 在 create 时没有设置 stride 参数，则返回失败。

3. 如果 bm_image 对象已关联 device memory，则会直接返回成功。

4. 所申请 device memory 由内部管理，在 destroy 或者不再使用时释放。