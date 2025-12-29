bmcv_image_csc_overlay
-------------------------

| **描述：**

| 该 API 可以实现对单张图片的 crop、color-space-convert、resize、padding、convert_to、flip、draw/fill rect、circle、overlay任意若干个功能的组合。

| **语法：**

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    DECL_EXPORT bm_status_t bmcv_image_csc_overlay(
        bm_handle_t             handle,
        int                     crop_num,
        bm_image                input,
        bm_image*               output,
        bmcv_rect_t*            crop_rect = NULL,
        bmcv_padding_attr_t*    padding_attr = NULL,
        bmcv_resize_algorithm   algorithm = BMCV_INTER_LINEAR,
        csc_type_t              csc_type = CSC_MAX_ENUM,
        bmcv_flip_mode          flip_mode = NO_FLIP,
        bmcv_convert_to_attr*   convert_to_attr = NULL,
        bmcv_overlay_attr*      overlay_attr = NULL,
        bmcv_draw_rect_attr*    draw_rect_attr = NULL,
        bmcv_fill_rect_attr*    fill_rect_attr = NULL,
        bmcv_circle_attr*       circle_attr = NULL);

| **参数：**

.. list-table:: bmcv_image_csc_overlay 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - crop_num
      - 输入
      - 对输入 bm_image 进行 crop 的数量。
    * - input
      - 输入
      - 输入 bm_image 对象。
    * - \* output
      - 输出
      - 输出 bm_image 对象指针，其指向空间的长度由 crop_num 决定。
    * - \* crop_rect
      - 输入
      - 输出 bm_image 对象所对应的在输入图像上 crop 的参数，其指向空间的长度由 crop_num 决定。若不使用 crop 功能则设置为 NULL。
    * - \* padding_attr
      - 输入
      - 所有 crop 的目标小图在 dst image 中的位置信息以及要 padding 的各通道像素值，其指向空间的长度由 crop_num 决定。若不使用 padding 功能则设置为 NULL。
    * - algorithm
      - 输入
      - resize 算法选择，包括 BMCV_INTER_NEAREST、BMCV_INTER_LINEAR 和 BMCV_INTER_BICUBIC三种，默认情况下是双线性差值。
    * - csc_type
      - 输入
      - color space convert 参数类型选择，填 CSC_MAX_ENUM 则使用默认值，默认为 CSC_YCbCr2RGB_BT601 或者 CSC_RGB2YCbCr_BT601。
    * - flip_mode
      - 输入
      - flip模式，可选择水平翻转，垂直翻转和旋转180度。
    * - \*convert_to_attr
      - 输入
      - 线性变换系数，其指向空间的长度为1。若不使用 convert_to 功能则设置为 NULL。当convert_to_attr参数非空时，输出格式仅支持FORMAT_RGB_PLANAR和FORMAT_BGR_PLANAR。
    * - \*overlay_attr
      - 输入
      - overlay功能参数，其指向空间的长度由 crop_num 决定。若不使用 overlay 功能则设置为 NULL。
    * - \*draw_rect_attr
      - 输入
      - 绘制长方形空心框功能参数，其指向空间的长度由 crop_num 决定。若不使用画框功能则设置为 NULL。
    * - \*fill_rect_attr
      - 输入
      - 绘制长方形实心框功能参数，其指向空间的长度由 crop_num 决定。若不使用填框功能则设置为 NULL。
    * - \*circle_attr
      - 输入
      - 绘制圆功能参数，其指向空间的长度由 crop_num 决定。若不使用画圆功能则设置为 NULL。

| **数据类型说明：**

除下述数据类型之外，未说明的数据类型请参阅 bmcv_image_csc_convert_to 接口文档。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef enum {
        NO_FLIP = 0,
        HORIZONTAL_FLIP = 1,
        VERTICAL_FLIP = 2,
        ROTATE_180 = 3,
    } bmcv_flip_mode;

NO_FLIP 表示不做处理，HORIZONTAL_FLIP 表示水平翻转，VERTICAL_FLIP 表示垂直翻转，ROTATE_180表示旋转180度。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_overlay_attr {
        char          overlay_num;
        bmcv_rect_t*  overlay_info;
        bm_image*     overlay_image;
    } bmcv_overlay_attr;

* overlay_num 描述需要叠图的数量，该接口最高支持在单张目的图上叠 16 张图。

* overlay_info 描述叠图位置信息指针，其指向空间的长度由 overlay_num 决定。

* overlay_image 描述需要叠图的 bm_image 指针，其指向空间的长度由 overlay_num 决定。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_draw_rect_attr {
        char          rect_num;
        bmcv_rect_t   draw_rect[4];
        unsigned int  color[4];
        short         line_width[4];
    } bmcv_draw_rect_attr;

* rect_num 描述需要画框的数量，该接口最高支持在单张目的图上画 4 个框。

* draw_rect 描述画框位置信息。

* color 描述画框的颜色信息，可由 ((r << 16) | (g << 8) | b) 表示。

* line_width 描述画框的线宽。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_fill_rect_attr {
        char          rect_num;
        bmcv_rect_t   fill_rect[4];
        unsigned int  color[4];
    } bmcv_fill_rect_attr;

* rect_num 描述需要填框的数量，该接口最高支持在单张目的图上填 4 个框。

* draw_rect 描述填框位置信息。

* color 描述填框的颜色信息，可由 ((r << 16) | (g << 8) | b) 表示。

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct bmcv_circle_attr {
        bmcv_point_t  center;
        short         radius;
        bmcv_color_t  color;
        signed char   line_width;
    } bmcv_circle_attr;

* 该接口最高支持在单张目的图上画 1 个圆。

* center 描述画圆中心位置信息。

* radius 描述画圆半径。

* color 描述画圆的颜色信息。

* line_width 描述画圆的线宽，取值为 [-2, 15]， -2 表示画圆形相框， -1 表示画实心圆。

| **返回值：**

该函数成功调用时, 返回 BM_SUCCESS。

**格式支持：**

本接口格式支持情况与 bmcv_image_csc_convert_to 接口相同，请参阅该接口文档。