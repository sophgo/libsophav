bmcv_image_storage_convert
--------------------------

| 【描述】

| 该接口将源图像格式的对应的数据转换为目的图像的格式数据，并填充在目的图像关联的 device memory 中。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_storage_convert(
            bm_handle_t handle,
            int image_num,
            bm_image* input_image,
            bm_image* output_image
    );

| 【参数】

.. list-table:: bmcv_image_storage_convert 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - image_num
      - 输入
      - 输入/输出 image 数量。
    * - \*input
      - 输入
      - 输入 bm_image 对象指针。
    * - \* output
      - 输出
      - 输出 bm_image 对象指针。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

该接口的参数数据类型与注意事项与 bmcv_image_vpp_basic 接口相同。