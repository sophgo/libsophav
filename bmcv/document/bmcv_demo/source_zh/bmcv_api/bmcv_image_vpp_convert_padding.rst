bmcv_image_vpp_convert_padding
------------------------------

| 【描述】

| 该 API 使用vpp硬件资源，通过对 dst image 做 memset 操作，实现图像padding的效果。这个效果的实现是利用了vpp的dst crop的功能，通俗的讲是将一张小图填充到大图中。 可以从一张src image上crop多个目标图像，对于每一个目标小图，可以一次性完成csc+resize操作，然后根据其在大图中的offset信息，填充到大图中。 一次crop的数量不能超过256。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_vpp_convert_padding(
        bm_handle_t           handle,
        int                   output_num,
        bm_image              input,
        bm_image *            output,
        bmcv_padding_atrr_t * padding_attr,
        bmcv_rect_t *         crop_rect = NULL,
        bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR);
    );

| 【参数】

.. list-table:: bmcv_image_vpp_convert_padding 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - output_num
      - 输出
      - 输出 bm_image 数量，和src image的crop 数量相等，一个src crop 输出一个dst bm_image。
    * - input
      - 输入
      - 输入 bm_image 对象。
    * - \* output
      - 输出
      - 输出 bm_image 对象指针。
    * - \* padding_attr
      - 输入
      - src crop的目标小图在dst image中的位置信息以及要pdding的各通道像素值。
    * - \* crop_rect
      - 输入
      - 每个输出 bm_image 对象所对应的在输入图像上 crop 的参数。
    * - algorithm
      - 输入
      - resize 算法选择，包括 BMCV_INTER_NEAREST、BMCV_INTER_LINEAR 和 BMCV_INTER_BICUBIC三种，默认情况下是双线性差值。

| 【数据类型说明】

.. code-block:: c

    typedef struct bmcv_padding_atrr_s {
        unsigned int    dst_crop_stx;
        unsigned int    dst_crop_sty;
        unsigned int    dst_crop_w;
        unsigned int    dst_crop_h;
        unsigned char padding_r;
        unsigned char padding_g;
        unsigned char padding_b;
        int           if_memset;
    } bmcv_padding_atrr_t;


1. 目标小图的左上角顶点相对于 dst image 原点（左上角）的offset信息：dst_crop_stx 和 dst_crop_sty；
#. 目标小图经resize后的宽高：dst_crop_w 和 dst_crop_h；
#. dst image 如果是RGB格式，各通道需要padding的像素值信息：padding_r、padding_g、padding_b，当if_memset=1时有效，如果是GRAY图像可以将三个值均设置为同一个值；
#. if_memset表示要不要在该api内部对dst image 按照各个通道的padding值做memset，仅支持RGB和GRAY格式的图像。

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

1. 该 API 的dst image的格式仅支持:

+-----+-------------------------------+
| num | dst image_format              |
+=====+===============================+
|  1  | FORMAT_RGB_PLANAR             |
+-----+-------------------------------+
|  2  | FORMAT_BGR_PLANAR             |
+-----+-------------------------------+
|  3  | FORMAT_RGBP_SEPARATE          |
+-----+-------------------------------+
|  4  | FORMAT_BGRP_SEPARATE          |
+-----+-------------------------------+
|  5  | FORMAT_RGB_PACKED             |
+-----+-------------------------------+
|  6  | FORMAT_BGR_PACKED             |
+-----+-------------------------------+

2. 该 API 所需要满足的格式以及部分要求与 bmcv_image_vpp_basic 一致。