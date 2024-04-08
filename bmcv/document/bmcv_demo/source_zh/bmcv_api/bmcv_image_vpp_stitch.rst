bmcv_image_vpp_stitch
------------------------------

| 【描述】

| 该 API 使用vpp硬件资源的 crop 功能，实现图像拼接的效果，对输入 image 可以一次完成 src crop + csc + resize + dst crop操作。dst image 中拼接的小图像数量不能超过256。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_vpp_stitch(
        bm_handle_t           handle,
        int                   input_num,
        bm_image*             input,
        bm_image              output,
        bmcv_rect_t*          dst_crop_rect,
        bmcv_rect_t*          src_crop_rect = NULL,
        bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR);

| 【参数】

.. list-table:: bmcv_image_vpp_stitch 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - input_num
      - 输入
      - 输入 bm_image 数量。
    * - \*input
      - 输入
      - 输入 bm_image 对象指针。
    * - output
      - 输出
      - 输出 bm_image 对象。
    * - \*dst_crop_rect
      - 输入
      - 在dst images上，各个目标小图的坐标和宽高信息。
    * - \*src_crop_rect
      - 输入
      - 在src image上，各个目标小图的坐标和宽高信息。
    * - algorithm
      - 输入
      - resize 算法选择，包括 BMCV_INTER_NEAREST、BMCV_INTER_LINEAR 和 BMCV_INTER_BICUBIC三种，默认情况下是双线性差值。

| 【数据类型说明】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_rect {
        int start_x;
        int start_y;
        int crop_w;
        int crop_h;
    } bmcv_rect_t;

start_x、start_y、crop_w、crop_h分别表示每个输出 bm_image 对象所对应的在输入图像上 crop 的参数，包括起始点x坐标、起始点y坐标、crop图像的宽度以及crop图像的高度。图像左上顶点作为坐标原点。如果不使用 crop 功能可填 NULL。

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

【注意】

1. 该 API 的src image不支持压缩格式的数据。

2. 该 API 所需要满足的格式以及部分要求与 bmcv_image_vpp_basic 一致。

3. 如果对src image做crop操作，一张src image只crop一个目标。