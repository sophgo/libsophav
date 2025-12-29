bmcv_image_vpp_basic
--------------------

| 【描述】

| 该 API 可以实现对多张图片的 crop、color-space-convert、resize、padding 及其任意若干个功能的组合。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_vpp_basic(
        bm_handle_t           handle,
        int                   in_img_num,
        bm_image*             input,
        bm_image*             output,
        int*                  crop_num_vec = NULL,
        bmcv_rect_t*          crop_rect = NULL,
        bmcv_padding_atrr_t*  padding_attr = NULL,
        bmcv_resize_algorithm algorithm = BMCV_INTER_LINEAR,
        csc_type_t            csc_type = CSC_MAX_ENUM,
        csc_matrix_t*         matrix = NULL);

| 【参数】

.. list-table:: bmcv_image_vpp_basic 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - image_num
      - 输入
      - 输入 image 数量。
    * - \*input
      - 输入
      - 输入 bm_image 对象指针，其指向空间的长度由 in_img_num 决定。
    * - \* output
      - 输出
      - 输出 bm_image 对象指针，其指向空间的长度由 in_img_num 和 crop_num_vec 共同决定，即所有输入图片 crop 数量之和。
    * - \* crop_num_vec
      - 输入
      - 该指针指向对每张输入图片进行 crop 的数量，其指向空间的长度由 in_img_num 决定，如果不使用 crop 功能可填 NULL。
    * - \* crop_rect
      - 输入
      - 每个输出 bm_image 对象所对应的在输入图像上 crop 的参数。
    * - \* padding_attr
      - 输入
      - 所有 crop 的目标小图在 dst image 中的位置信息以及要 padding 的各通道像素值，若不使用 padding 功能则设置为 NULL。
    * - algorithm
      - 输入
      - resize 算法选择，包括 BMCV_INTER_NEAREST、BMCV_INTER_LINEAR 和 BMCV_INTER_BICUBIC三种，默认情况下是双线性差值。
    * - csc_type
      - 输入
      - color space convert 参数类型选择，填 CSC_MAX_ENUM 则使用默认值，默认为 CSC_YCbCr2RGB_BT601 或者 CSC_RGB2YCbCr_BT601。
    * - \*matrix
      - 输入
      - 如果 csc_type 选择 CSC_USER_DEFINED_MATRIX，则需要用该指针传入系数矩阵。

| 【数据类型说明】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum csc_type {
        CSC_YCbCr2RGB_BT601 = 0,
        CSC_YPbPr2RGB_BT601,
        CSC_RGB2YCbCr_BT601,
        CSC_YCbCr2RGB_BT709,
        CSC_RGB2YCbCr_BT709,
        CSC_RGB2YPbPr_BT601,
        CSC_YPbPr2RGB_BT709,
        CSC_RGB2YPbPr_BT709,
        CSC_FANCY_PbPr_BT601 = 100,
        CSC_FANCY_PbPr_BT709,
        CSC_USER_DEFINED_MATRIX = 1000,
        CSC_MAX_ENUM
    } csc_type_t;

可选的 csc_type 参数。

其中 CbCr 表示 Limit range，其应用 YUV 数据像素范围为[16, 235]，PbPr 表示 Full range，其应用 YUV 数据像素范围为[0, 255]。

BT601/BT709 代表相应的视频颜色空间标准。

FANCY 表示采用与 opencv 相同的处理方式计算上采样（如从 YUV420P 上采样到 RGB）。

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

start_x、start_y、crop_w、crop_h 分别表示每个输出 bm_image 对象所对应的在输入图像上 crop 的参数，包括起始点 x 坐标、起始点 y 坐标、crop 图像的宽度以及 crop 图像的高度。图像左上顶点作为坐标原点。如果不使用 crop 功能可填 NULL。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_padding_atrr_s {
        unsigned int  dst_crop_stx;
        unsigned int  dst_crop_sty;
        unsigned int  dst_crop_w;
        unsigned int  dst_crop_h;
        unsigned char padding_r;
        unsigned char padding_g;
        unsigned char padding_b;
        int           if_memset;
    } bmcv_padding_atrr_t;

1. 目标小图的左上角顶点相对于 dst image 原点（左上角）的 offset 信息：dst_crop_stx 和 dst_crop_sty；
#. 目标小图经resize后的宽高：dst_crop_w 和 dst_crop_h；
#. dst image 如果是RGB格式，各通道需要 padding 的像素值信息：padding_r、padding_g、padding_b，当 if_memset=1 时有效，如果是 GRAY 图像可以将三个值均设置为同一个值；
#. if_memset 表示要不要在该 api 内部对 dst image 按照各个通道的 padding 值做 memset。如果设置为 0 则用户需要在调用该 api 前，根据需要 padding 的像素值信息，调用 bmlib 中的 api 直接对 device memory 进行 memset 操作，如果用户对 padding 的值不关心，可以设置为 0 忽略该步骤。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct {
        short csc_coe00;
        short csc_coe01;
        short csc_coe02;
        unsigned char csc_add0;
        unsigned char csc_sub0;
        short csc_coe10;
        short csc_coe11;
        short csc_coe12;
        unsigned char csc_add1;
        unsigned char csc_sub1;
        short csc_coe20;
        short csc_coe21;
        short csc_coe22;
        unsigned char csc_add2;
        unsigned char csc_sub2;
    } csc_matrix_t;

自定义 csc_matrix 的系数。其中，矩阵变换关系如下：

.. math::

    \left\{
    \begin{array}{c}
    dst_0=(coe_{00} * (src_0-sub_0)+coe_{01} * (src_1-sub_1)+coe_{02} * (src_2-sub_2))>>10 + add_0 \\
    dst_1=(coe_{10} * (src_0-sub_0)+coe_{11} * (src_1-sub_1)+coe_{12} * (src_2-sub_2))>>10 + add_1 \\
    dst_2=(coe_{20} * (src_0-sub_0)+coe_{21} * (src_1-sub_1)+coe_{22} * (src_2-sub_2))>>10 + add_2 \\
    \end{array}
    \right.


| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【格式支持】

1. 支持的数据类型为：

+-----+------------------------+-------------------------------+
| num | input data_type        | output data_type              |
+=====+========================+===============================+
|  1  |                        | DATA_TYPE_EXT_FLOAT32         |
+-----+                        +-------------------------------+
|  2  |                        | DATA_TYPE_EXT_1N_BYTE         |
+-----+                        +-------------------------------+
|  3  | DATA_TYPE_EXT_1N_BYTE  | DATA_TYPE_EXT_1N_BYTE_SIGNED  |
+-----+                        +-------------------------------+
|  4  |                        | DATA_TYPE_EXT_FP16            |
+-----+                        +-------------------------------+
|  5  |                        | DATA_TYPE_EXT_BF16            |
+-----+------------------------+-------------------------------+


2. 输入支持色彩格式为：

+-----+-------------------------------+
| num | input image_format            |
+=====+===============================+
|  1  | FORMAT_YUV420P                |
+-----+-------------------------------+
|  2  | FORMAT_YUV422P                |
+-----+-------------------------------+
|  3  | FORMAT_YUV444P                |
+-----+-------------------------------+
|  4  | FORMAT_NV12                   |
+-----+-------------------------------+
|  5  | FORMAT_NV21                   |
+-----+-------------------------------+
|  6  | FORMAT_NV16                   |
+-----+-------------------------------+
|  7  | FORMAT_NV61                   |
+-----+-------------------------------+
|  8  | FORMAT_RGB_PLANAR             |
+-----+-------------------------------+
|  9  | FORMAT_BGR_PLANAR             |
+-----+-------------------------------+
|  10 | FORMAT_RGB_PACKED             |
+-----+-------------------------------+
|  11 | FORMAT_BGR_PACKED             |
+-----+-------------------------------+
|  12 | FORMAT_RGBP_SEPARATE          |
+-----+-------------------------------+
|  13 | FORMAT_BGRP_SEPARATE          |
+-----+-------------------------------+
|  14 | FORMAT_GRAY                   |
+-----+-------------------------------+
|  15 | FORMAT_COMPRESSED             |
+-----+-------------------------------+
|  16 | FORMAT_YUV444_PACKED          |
+-----+-------------------------------+
|  17 | FORMAT_YVU444_PACKED          |
+-----+-------------------------------+
|  18 | FORMAT_YUV422_YUYV            |
+-----+-------------------------------+
|  19 | FORMAT_YUV422_YVYU            |
+-----+-------------------------------+
|  20 | FORMAT_YUV422_UYVY            |
+-----+-------------------------------+
|  21 | FORMAT_YUV422_VYUY            |
+-----+-------------------------------+


3. 输出支持色彩格式为：

+-----+-------------------------------+
| num | input image_format            |
+=====+===============================+
|  1  | FORMAT_YUV420P                |
+-----+-------------------------------+
|  2  | FORMAT_YUV422P                |
+-----+-------------------------------+
|  3  | FORMAT_YUV444P                |
+-----+-------------------------------+
|  4  | FORMAT_NV12                   |
+-----+-------------------------------+
|  5  | FORMAT_NV21                   |
+-----+-------------------------------+
|  6  | FORMAT_NV16                   |
+-----+-------------------------------+
|  7  | FORMAT_NV61                   |
+-----+-------------------------------+
|  8  | FORMAT_RGB_PLANAR             |
+-----+-------------------------------+
|  9  | FORMAT_BGR_PLANAR             |
+-----+-------------------------------+
|  10 | FORMAT_RGB_PACKED             |
+-----+-------------------------------+
|  11 | FORMAT_BGR_PACKED             |
+-----+-------------------------------+
|  12 | FORMAT_RGBP_SEPARATE          |
+-----+-------------------------------+
|  13 | FORMAT_BGRP_SEPARATE          |
+-----+-------------------------------+
|  14 | FORMAT_GRAY                   |
+-----+-------------------------------+
|  15 | FORMAT_YUV422_YUYV            |
+-----+-------------------------------+
|  16 | FORMAT_YUV422_YVYU            |
+-----+-------------------------------+
|  17 | FORMAT_YUV422_UYVY            |
+-----+-------------------------------+
|  18 | FORMAT_YUV422_VYUY            |
+-----+-------------------------------+
|  19 | FORMAT_HSV_PLANAR             |
+-----+-------------------------------+

| 【注意】

1. 图片实际缩放倍数（（crop.width / output.width) 以及 (crop.height / output.height））限制在 1/128 ～ 128 之间。

#. 输入输出的宽高（src.width, src.height, dst.widht, dst.height）限制在 16 ～ 8192 之间。

#. 输入必须关联 device memory，否则返回失败。

#. 该接口支持 byte align 的数据输入。但当输入为 YUV420P / NV12 / NV21 格式时，使用 stride 32 byte align 的数据作为输入可使转换效率更高。

#. FORMAT_COMPRESSED 是 VPU 解码后内置的一种压缩格式，它包括4个部分：Y compressed table、Y compressed data、CbCr compressed table 以及 CbCr compressed data。请注意 bm_image 中这四部分存储的顺序与 FFMPEG 中 AVFrame 稍有不同，如果需要 attach AVFrame 中 device memory 数据到 bm_image 中时，对应关系如下，关于 AVFrame 详细内容请参考 VPU 的用户手册。

.. code-block:: c

    bm_device_mem_t src_plane_device[4];
    src_plane_device[0] = bm_mem_from_device((u64)avframe->data[6],
            avframe->linesize[6]);
    src_plane_device[1] = bm_mem_from_device((u64)avframe->data[4],
            avframe->linesize[4] * avframe->h);
    src_plane_device[2] = bm_mem_from_device((u64)avframe->data[7],
            avframe->linesize[7]);
    src_plane_device[3] = bm_mem_from_device((u64)avframe->data[5],
            avframe->linesize[4] * avframe->h / 2);

    bm_image_attach(*compressed_image, src_plane_device);