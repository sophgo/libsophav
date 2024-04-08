bm_image_get_format_info
------------------------

| 【描述】
| 该接口用于获取 bm_image 的一些信息。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bm_image_get_format_info(
            bm_image *              src,
            bm_image_format_info_t *info
    );

| 【参数】

.. list-table:: bm_image_get_format_info 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - \*src
      - 输入
      - 所要获取信息的目标 bm_image。
    * - \*info
      - 输出
      - 保存所需信息的数据结构，返回给用户， 具体内容见下面的数据结构说明。

| 【返回值】

函数调用成功时返回BM_SUCCESS。

| 【数据类型说明】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bm_image_format_info {
            int                      plane_nb;
            bm_device_mem_t          plane_data[8];
            int                      stride[8];
            int                      width;
            int                      height;
            bm_image_format_ext      image_format;
            bm_image_data_format_ext data_type;
            bool                     default_stride;
    } bm_image_format_info_t;

* int plane_nb

  该 image 的 plane 数量

* bm_device_mem_t plane_data[8]

  各个 plane 的 device memory

* int stride[8];

  各个 plane 的 stride 值

* int width;

  图片的宽度

* int height;

  图片的高度

* bm_image_format_ext image_format;

  图片的格式

* bm_image_data_format_ext data_type;

  图片的存储数据类型

* bool default_stride;

  是否使用了默认的 stride