bm_image_copy_device_to_host
----------------------------

| 【描述】
| 该 API 将 device memory 中的图片数据拷贝到 host 端。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :name: a label for hyperlink (destroy)
    :force:

    bm_status_t bm_image_copy_device_to_host(
            bm_image image,
            void* buffers[]
    );

| 【参数】

.. list-table:: bm_image_copy_device_to_host 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - image
      - 输入
      - 待传输数据的 bm_image 对象。
    * - \* buffers[]
      - 输出
      - host 端指针，buffers 为指向不同 plane 数据的指针，每个 plane 需要传输的数据量可以通过 bm_image_get_byte_size API 获取。



| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

.. note::

    1. 如果 bm_image 未由 bm_image_create 创建，则返回失败。

    2. 如果 bm_image 没有与 device memory 相关联的话，将返回失败。

    3. 数据传输失败的话，API 将调用失败。

    4. 如果该函数成功返回,会将所关联的 device memory 中的数据拷贝到 host 端 buffers 中。