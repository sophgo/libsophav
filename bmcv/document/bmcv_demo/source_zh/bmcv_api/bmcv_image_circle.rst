bmcv_image_circle
-----------------

| 【描述】

| 在图像上绘制圆形。支持绘制空心圆（CIR_LINE）、实心圆（CIR_SHAPE）、和圆形相框（CIR_EMPTY）。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_circle(
        bm_handle_t          handle,
        bm_image             image,
        bmcv_point_t         center,
        int                  radius,
        bmcv_color_t         color,
        int                  line_width)


| 【参数】

.. list-table:: bmcv_image_circle 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - image
      - 输入/输出
      - 输入/输出 bm_image 对象。
    * - center
      - 输入
      - 圆心的位置(x, y)（以图片左上角为原点）。
    * - radius
      - 输入
      - 圆的半径。
    * - color
      - 输入
      - 所填充颜色的R/G/B值。
    * - line_width
      - 输入
      - 画圆的线宽，取值范围为[-2, 15], 当传入 -1 时表示绘制实心圆， 当传入 -2 时表示绘制相框。

| 【数据类型说明】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    typedef struct {
        int x;
        int y;
    } bmcv_point_t;

    typedef struct {
        unsigned char r;
        unsigned char g;
        unsigned char b;
    } bmcv_color_t;

    typedef enum {
        CIR_EMPTY = -2,
        CIR_SHAPE = -1,
    } bmcv_cir_mode;

1. x, y 代表坐标点的横纵坐标。
#. CIR_EMPTY 表示相框模式，即填充圆形外部区域，圆形内部不变；
#. CIR_SHAPE 表示绘制实心圆；

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 该 API 所需要满足的图像格式、尺寸等要求与 bmcv_image_vpp_basic 中的表格相同。

2. 输入必须关联 device memory，否则返回失败。

| 【代码示例】

.. code-block:: c++

    bm_handle_t handle;
    int src_w = 1920, src_h = 1080, dev_id = 0;
    bmcv_point_t center = {960, 540};
    bmcv_color_t color = {151, 255, 152};
    int radius = 250, line_width = 10;
    bm_image_format_ext src_fmt = FORMAT_YUV420P;
    char *src_name = "1920x1080_yuv420.bin", *dst_name = "dst.bin";

    bm_image src;
    bm_dev_request(&handle, 0);
    bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
    bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);

    bm_read_bin(src,src_name);
    bmcv_image_circle(handle, src, center, radius, color, line_width);
    bm_write_bin(src, dst_name);

    bm_image_destroy(&src);
    bm_dev_free(handle);