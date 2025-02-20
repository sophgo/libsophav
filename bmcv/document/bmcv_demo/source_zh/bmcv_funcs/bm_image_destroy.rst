bm_image_destroy
----------------

| 【描述】
| 销毁 bm_image 对象，避免内存泄漏，与 bm_image_create 成对使用。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :name: a label for hyperlink (destroy)
    :force:

    bm_status_t bm_image_destroy(
            bm_image* image
    );

| 【参数】

.. list-table:: bm_image_destroy 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - \*image
      - 输入
      - 待销毁的 bm_image 对象指针。

| 【返回值】

成功返回将销毁该 bm_image 对象。

注意，如果该对象的 device memory 是直接以该对象用 bm_image_alloc_dev_mem 系列函数申请的，则 bm_image_destroy 释放该内存空间。

如果该对象的 device memory 是使用 bm_image_attach 绑定而来的，则被视为由用户自己管理的内存空间，将不会被释放。