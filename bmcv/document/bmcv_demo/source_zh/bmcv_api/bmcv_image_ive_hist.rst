bmcv_ive_hist
------------------------------

| 【描述】

| 该 API 使用ive硬件资源, 创建灰度图像，遍历图像像素值实现统计图像相同像素值出现的次数，构建直方图。

| 【语法】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_ive_hist(
        bm_handle_t      handle,
        bm_image         input,
        bm_device_mem_t  output);

| 【参数】

.. list-table:: bmcv_ive_hist 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用 bm_dev_request 获取。
    * - input
      - 输入
      - 输入 bm_image 对象结构体, 不能为空。
    * - output
      - 输出
      - 输入 bm_device_mem_t 对象数据结构体, 不能为空, 内存至少配置 1024 字节，输出使用u32类型代表0-255每个数字的统计值。

.. list-table::
    :widths: 25 38 60 32

    * - **参数名称**
      - **图像格式**
      - **数据类型**
      - **分辨率**
    * - input
      - GRAY
      - DATA_TYPE_EXT_1N_BYTE
      - 64x64~1920x1080

| 【返回值】

该函数成功调用时, 返回 BM_SUCCESS。

| 【注意】

1. 输入图像的 width 都需要16对齐。

2. 计算公式如下:

    .. math::

      I(x) = \sum_{i} \sum_{j} \begin{cases}
         1 & \text{if } I(i, j) = x \\
         0 & \text{otherwise}
      \end{cases} \quad \text{for } x = 0 \ldots 255

| 【代码示例】

.. code-block:: c++
    :linenos:
    :lineno-start: 1
    :force:

    bm_image src;
    char * src_name = "./data/input_y.yuv";
    bm_device_mem_t dst;
    int src_stride[4];
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }
    bm_ive_image_calc_stride(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, src_stride);
    bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
    if (ret != BM_SUCCESS) {
        printf("bm_image_alloc_dev_mem_src. ret = %d\n", ret);
        exit(-1);
    }
    bm_ive_read_bin(src, src_name);

    // create result
    ret = bm_malloc_device_byte(handle, &dst, 1024);
    if (ret != BM_SUCCESS) {
        printf("bm_malloc_device_byte failed. ret = %d\n", ret);
        exit(-1);
    }
    ret = bmcv_ive_hist(handle, src, dst);
    if(ret != BM_SUCCESS){
        printf("bmcv_image_ive_hist failed. ret = %d\n", ret);
        exit(-1);
    }
    unsigned char *dst_hist_result = malloc(dst.size);

    ret = bm_memcpy_d2s(handle, dst_hist_result, dst);
    if (ret != BM_SUCCESS) {
        printf("bm_memcpy_d2s failed. ret = %d\n", ret);
        exit(-1);
    };
    FILE *hist_result_fp = fopen(dst_name, "wb");
    fwrite((void *)dst_hist_result, 1, dst.size, hist_result_fp);
    fclose(hist_result_fp);
    free(dst_hist_result);
    bm_image_destroy(&src);
    bm_free_device(handle, dst);
    bm_dev_free(handle);
