bm_image_detach
---------------

| 【描述】
| 该 API 用于将 device memory 与 bm\_image 解关联。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :name: a label for hyperlink (destroy)
    :force:

    bm_status_t bm_image_detach(bm_image image);

| 【参数】

.. list-table:: bm_image_detach 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - image
      - 输入
      - 待解关联的 bm_image 对象。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意事项】

1. 如果传入的 bm_image 对象未被创建，则返回失败。

2. 该函数成功返回时，会对 bm_image 对象关联的 device_memory 进行解关联，bm_image 对象将不再关联 device_memory。

3. 如果解关联的 device_memory 不是由 bm_image_attach 关联来的，而是由该 bm_image 对象通过 bm_image_alloc_dev_mem 系列函数申请的，该 API 则会释放这块 device memory。

4. 如果对象未关联任何 device memory，则直接返回成功。