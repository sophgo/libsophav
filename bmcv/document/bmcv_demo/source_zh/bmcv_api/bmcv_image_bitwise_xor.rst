bmcv_image_bitwise_xor
======================

两张大小相同的图片对应像素值进行按位异或操作。


**接口形式：**

    .. code-block:: c

        bm_status_t bmcv_image_bitwise_xor(
                    bm_handle_t handle,
                    bm_image input1,
                    bm_image input2,
                    bm_image output);


**参数说明：**

* bm_handle_t handle

  输入参数。bm_handle 设备环境句柄，通过调用 bm_dev_request 获取。

* input1

  输入参数。 输入第一张图像的 bm_image，bm_image 需要外部调用 bmcv_image_create 创建。image 内存可以使用 bm_image_alloc_dev_mem 或者 bm_image_copy_host_to_device 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。

* input2

  输入参数。 输入第二张图像的 bm_image，bm_image 需要外部调用 bmcv_image_create 创建。image 内存可以使用 bm_image_alloc_dev_mem 或者 bm_image_copy_host_to_device 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。

* output

  输出参数。输出 bm_image。image 内存可以通过 bm_image_alloc_dev_mem 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。如果不主动分配将在 api 内部进行自行分配。


**返回值说明:**

* BM_SUCCESS: 成功

* 其他: 失败


**格式支持:**

1. 输入和输出的数据类型

+-----+-------------------------------+
| num | data_type                     |
+=====+===============================+
|  1  | DATA_TYPE_EXT_1N_BYTE         |
+-----+-------------------------------+

2. 输入和输出的色彩格式

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
| 11  | FORMAT_NV12            |
+-----+------------------------+
| 12  | FORMAT_NV21            |
+-----+------------------------+
| 13  | FORMAT_NV16            |
+-----+------------------------+
| 14  | FORMAT_NV61            |
+-----+------------------------+
| 15  | FORMAT_NV24            |
+-----+------------------------+


**注意事项：**

1. 在调用 bmcv_image_bitwise_xor()之前必须确保输入的 image 内存已经申请。

2. 输入、输出的 data_type，image_format必须相同。


**示例代码**

    .. code-block:: c

        int channel = 3;
        int width = 1920;
        int height = 1080;
        int dev_id = 0;
        bm_handle_t handle;
        bm_status_t ret = BM_SUCCESS;
        unsigned char* src1_data = (unsigned char*)malloc(channel * width * height * sizeof(unsigned char));
        unsigned char* src2_data = (unsigned char*)malloc(channel * width * height * sizeof(unsigned char));
        unsigned char* res_data = (unsigned char*)malloc(channel * width * height * sizeof(unsigned char));

        struct timespec tp;
        clock_gettime(NULL, &tp);
        srand(tp.tv_nsec);

        for (int i = 0; i < channel * width * height; i++) {
            src1_data[i] = rand() % 256;
            src2_data[i] = rand() % 256;
        }

        ret = bm_dev_request(&handle, dev_id);
        if (ret != BM_SUCCESS) {
            printf("bm_dev_request failed. ret = %d\n", ret);
            exit(-1);
        }

        bm_image input1, input2, output;
        ret = bm_image_create(handle, height, width, FORMAT_RGB_PLANAR, DATA_TYPE_EXT_1N_BYTE, &input1, NULL);
        if (ret != BM_SUCCESS) {
            printf("bm_image_create failed. ret = %d\n", ret);
            exit(-1);
        }
        ret = bm_image_alloc_dev_mem(input1, 2);
        if (ret != BM_SUCCESS) {
            printf("bm_image_alloc_dev_mem failed. ret = %d\n", ret);
            exit(-1);
        }
        ret = bm_image_copy_host_to_device(input1, (void **)&src1_data);
        if (ret != BM_SUCCESS) {
            printf("bm_image_copy_host_to_device failed. ret = %d\n", ret);
            exit(-1);
        }
        ret = bm_image_create(handle, height, width, FORMAT_RGB_PLANAR, DATA_TYPE_EXT_1N_BYTE, &input2, NULL);
        if (ret != BM_SUCCESS) {
            printf("bm_image_create failed. ret = %d\n", ret);
            exit(-1);
        }
        ret = bm_image_alloc_dev_mem(input2, 2);
        if (ret != BM_SUCCESS) {
            printf("bm_image_alloc_dev_mem failed. ret = %d\n", ret);
            exit(-1);
        }
        ret = bm_image_copy_host_to_device(input2, (void **)&src2_data);
        if (ret != BM_SUCCESS) {
            printf("bm_image_copy_host_to_device failed. ret = %d\n", ret);
            exit(-1);
        }
        ret = bm_image_create(handle, height, width, FORMAT_RGB_PLANAR, DATA_TYPE_EXT_1N_BYTE, &output);
        if (ret != BM_SUCCESS) {
            printf("bm_image_create failed. ret = %d\n", ret);
            exit(-1);
        }
        ret = bm_image_alloc_dev_mem(output, 2);
        if (ret != BM_SUCCESS) {
            printf("bm_image_alloc_dev_mem failed. ret = %d\n", ret);
            exit(-1);
        }

        ret = bmcv_image_bitwise_xor(handle, input1, input2, output);
        if (ret != BM_SUCCESS) {
            printf("bmcv_image_bitwise_xor error!\n");
            bm_image_destroy(&input1);
            bm_image_destroy(&input2);
            bm_image_destroy(&output);
            bm_dev_free(handle);
            exit(-1);
        }

        ret = bm_image_copy_device_to_host(output, (void **)&res_data);
        if (ret != BM_SUCCESS) {
            printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
            exit(-1);
        }

        bm_image_destroy(&input1);
        bm_image_destroy(&input2);
        bm_image_destroy(&output);
        bm_dev_free(handle);
        free(src1_data);
        free(src2_data);
        free(res_data);