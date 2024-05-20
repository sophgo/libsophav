bmcv_image_flip
----------------

| 【描述】

| 该 API 可将输入图像进行水平翻转、垂直翻转或旋转180度的处理。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_flip(
          bm_handle_t          handle,
          bm_image             input,
          bm_image             output,
          bmcv_flip_mode       flip_mode)


| 【参数】

.. list-table:: bmcv_image_flip 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - input
      - 输入
      - 输入 bm_image 对象。
    * - output
      - 输出
      - 输出 bm_image 对象。
    * - flip_mode
      - 输入
      - flip模式，可选择水平翻转，垂直翻转和旋转180度。

| 【数据类型说明】

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

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 该 API 所需要满足的格式以及部分要求与 bmcv_image_vpp_basic 中的表格相同。

2. 输入输出的宽高（src.width, src.height, dst.widht, dst.height）限制在 8 ～ 8192 之间，缩放128倍。

3. 输入必须关联 device memory，否则返回失败。

| 【代码示例】

.. code-block:: c++

        bm_handle_t handle;
        int src_h = 1080, src_w = 1920, dst_w = 1920, dst_h = 1080, dev_id = 0;
        bm_image_format_ext src_fmt = FORMAT_YUV420P, dst_fmt = FORMAT_YUV420P;
        bmcv_flip_mode flip_mode = HORIZONTAL_FLIP;
        char *src_name = "1920x1080_yuv420.bin", *dst_name = "dst.bin";

        bm_dev_request(&handle, 0);

        bm_image_create(handle, src_h, src_w, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
        bm_image_create(handle, dst_h, dst_w, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);

        bm_image_alloc_dev_mem(src,BMCV_HEAP_ANY);
        bm_image_alloc_dev_mem(dst,BMCV_HEAP_ANY);

        bm_read_bin(src,src_name);

        bmcv_image_flip(handle, src, dst, flip_mode);

        bm_write_bin(dst, dst_name);

        bm_image_destroy(&src);
        bm_image_destroy(&dst);

        bm_dev_free(handle);