bm_image_write_to_bmp
-------------------------

| 【描述】
| 该接口用于将 bm_image 的数据保存成 bmp 格式存到文件中。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bm_image_write_to_bmp(bm_image image, const char *filename);

| 【参数】

.. list-table:: bm_image_write_to_bmp 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - image
      - 输入
      - 目标 bm_image。
    * - filename
      - 输入
      - 保存 bmp 文件的名称。

| 【返回值】

成功则返回 BM_SUCCESS。

| 【注意事项】

1. 该 api 要求输入 bm_image 对象关联 device memory，否则返回失败。

BMCV 还提供类似接口读入或写出 bm_image 的数据。如下：

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    void bm_read_bin(bm_image src, const char *input_name);
    void bm_read_compact_bin(bm_image src, const char *input_name);

    void bm_write_bin(bm_image dst, const char *output_name);
    void bm_write_compact_bin(bm_image dst, const char *output_name);

其中，bm_read_bin 和 bm_write_bin 可以读入和写出 raw 数据文件，读入写出数据按照每行间距为 bm_image 中各 plane 的 stride 来进行。(若 stride 大于默认 stride， 可能会读取或写出无效数据)

bm_read_compact_bin 和 bm_write_compact_bin 适合读入和写出紧凑排列的 raw 数据文件，读入写出数据按照每行间距为 bm_image 中各 plane 的默认 stride 来进行。

