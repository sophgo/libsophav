bmcv_image_gaussian_blur
------------------------------

**描述：**

该接口用于对图像进行高斯滤波操作。

**语法：**

.. code-block:: c
    :linenos:
    :lineno-start: 1
    :force:

    bm_status_t bmcv_image_gaussian_blur(
        bm_handle_t handle,
        bm_image input,
        bm_image output,
        int kw,
        int kh,
        float sigmaX,
        float sigmaY = 0);

**参数：**

.. list-table:: bmcv_image_gaussian_blur 参数表
    :widths: 15 15 35

    * - **参数名称**
      - **输入/输出**
      - **描述**
    * - handle
      - 输入
      - 设备环境句柄，通过调用bm_dev_request获取。
    * - input
      - 输入
      - 输入图像的bm_image，bm_image需要外部调用bmcv_image_create创建。image内存可以使用bm_image_alloc_dev_mem或者bm_image_copy_host_to_device来开辟新的内存，或者使用bmcv_image_attach来attach已有的内存。
    * - output
      - 输出
      - 输出图像的bm_image，bm_image需要外部调用bmcv_image_create创建。image内存可以通过bm_image_alloc_dev_mem来开辟新的内存，或者使用bmcv_image_attach来attach已有的内存。如果不主动分配将在api内部进行自行分配。
    * - kw
      - 输入
      - kernel宽的大小，目前支持数值为3。
    * - kh
      - 输入
      - kernel高的大小，目前支持数值为3。
    * - sigmaX
      - 输入
      - X方向上的高斯核标准差，取值范围为0-4.0。
    * - sigmaY = 0
      - 输入
      - Y方向上的高斯核标准差，取值范围为0-4.0。如果为0则表示与X方向上的高斯核标准差相同。

**返回值：**

该函数成功调用时, 返回BM_SUCCESS。

**格式支持：**

该接口目前支持以下图像格式:

+-----+------------------------+
| num | image_format           |
+=====+========================+
| 1   | FORMAT_BGR_PACKED      |
+-----+------------------------+
| 2   | FORMAT_BGR_PLANAR      |
+-----+------------------------+
| 3   | FORMAT_RGB_PACKED      |
+-----+------------------------+
| 4   | FORMAT_RGB_PLANAR      |
+-----+------------------------+
| 5   | FORMAT_RGBP_SEPARATE   |
+-----+------------------------+
| 6   | FORMAT_BGRP_SEPARATE   |
+-----+------------------------+
| 7   | FORMAT_GRAY            |
+-----+------------------------+
| 8   | FORMAT_YUV420P         |
+-----+------------------------+
| 9   | FORMAT_YUV422P         |
+-----+------------------------+
| 10  | FORMAT_YUV444P         |
+-----+------------------------+

该接口目前支持的数据格式：

+-----+------------------------+
| num | data_type              |
+=====+========================+
| 1   | DATA_TYPE_EXT_1N_BYTE  |
+-----+------------------------+

**注意事项：**

1. 在调用该接口之前必须确保输入图像的内存已经申请。

2. 输入输出图像的数据格式，图像格式必须相同。

3. 目前支持图像的最大宽为4096，最大高为8192。

**代码示例：**

.. code-block:: c
    :linenos:
    :lineno-start: 1
    :force:

    int format    = FORMAT_RGBP_SEPARATE;
    int channel   = 3;
    int width     = 1920;
    int height    = 1080;
    int dev_id    = 0;
    bm_handle_t handle;
    bm_status_t dev_ret = bm_dev_request(&handle, dev_id);
    unsigned char *input_data = (unsigned char*)malloc(width * height * channel);
    unsigned char *output_data = (unsigned char*)malloc(width * height * channel);
    for (int i = 0; i < channel * width * height; i++) {
        input_data[i] = rand() % 255;
    }
    bm_image input, output;
    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &input, NULL);
    bm_image_create(handle, height, width, (bm_image_format_ext)format, DATA_TYPE_EXT_1N_BYTE, &output, NULL);
    bm_image_alloc_dev_mem(input, 2);
    bm_image_alloc_dev_mem(output, 2);
    unsigned char *input_addr[3] = {input_data, input_data + width * height, input_data + width * height * 2};
    bm_image_copy_host_to_device(input, (void **)(input_addr));
    if (BM_SUCCESS != bmcv_image_gaussian_blur(handle, input, output, 3, 3, 0.1, 0.1)) {
        printf("bmcv gaussian blur error !!!\n");
        bm_image_destroy(&input);
        bm_image_destroy(&output);
        bm_dev_free(handle);
        exit(-1);
    }
    unsigned char *output_addr[3] = {output_data, output_data + width * height, output_data + width * height * 2};
    bm_image_copy_device_to_host(output, (void **)output_addr);
    bm_image_destroy(&input);
    bm_image_destroy(&output);
    free(input_data);
    free(output_data);
    bm_dev_free(handle);
