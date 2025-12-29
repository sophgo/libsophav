bm_image_is_attached
--------------------

| 【描述】
| 该接口用于判断目标 是否已经 attach 存储空间。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bool bm_image_is_attached(bm_image image);

| 【参数】

.. list-table:: bm_image_is_attached 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - image
      - 输入
      - 所要判断是否 attach 存储空间的目标 bm_image。

| 【返回值】

若目标 bm_image 已经 attach 存储空间则返回 true，否则返回 false。

| 【注意事项】

1. 一般情况而言，调用 bmcv api 要求输入 bm_image 对象关联 device memory，否则返回失败。而输出 bm_image 对象如果未关联 device memory，则会在内部调用 bm_image_alloc_dev_mem 函数，内部申请内存。

2. bm_image 调用 bm_image_alloc_dev_mem 所申请的内存都由内部自动管理，在调用 bm_image_destroy、 bm_image_detach 或者 bm_image_attach 其他 device memory 时自动释放，无需调用者管理。相反，如果 bm_image_attach 一块 device memory 时，表示这块 memory 将由调用者自己管理。无论是 bm_image_destroy、bm_image_detach，或者再调用 bm_image_attach 其他 device memory，均不会释放，需要调用者手动释放。

