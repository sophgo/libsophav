bm_image_attach
---------------

| 【描述】
| 如果用户希望自己管理 device memory，或者 device memory 由外部组件 (VPU/VPP 等)产生，则可以调用以下 API 将这块 device memory 与 bm_image 相关联。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :name: a label for hyperlink (destroy)
    :force:

    bm_status_t bm_image_attach(
            bm_image image,
            bm_device_mem_t* device_memory
    );

| 【参数】

.. list-table:: bm_image_attach 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - image
      - 输入
      - 待关联的 bm_image 对象。
    * - \* device_memory
      - 输入
      - 填充 bm_image 所需的 device_memory，数量应由创建 bm_image 结构时 image_format 对应的 plane 数所决定。


| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

.. note::

    1. 如果 bm_image 未由 bm_image_create 创建，则返回失败。

    2. 该函数成功调用时，bm_image 对象将与传入的 device_memory 对象相关联。

    3. bm_image 不会对通过这种方式关联的 device_memory 进行管理，即在销毁的时候并不会释放对应的 device_memory，需要用户自行管理这块 device_memory。
