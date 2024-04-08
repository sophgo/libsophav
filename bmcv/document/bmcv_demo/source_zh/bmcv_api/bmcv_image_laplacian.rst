bmcv_image_laplacian
====================

梯度计算laplacian算子。


**接口形式：**

    .. code-block:: c

        bm_status_t bmcv_image_laplacian(
                    bm_handle_t handle,
                    bm_image input,
                    bm_image output,
                    unsigned int ksize);


**参数说明：**

* bm_handle_t handle

  输入参数。 bm_handle 句柄。

* bm_image input

  输入参数。输入图像的 bm_image，bm_image 需要外部调用 bmcv_image_create 创建。image 内存可以使用 bm_image_alloc_dev_mem 或者 bm_image_copy_host_to_device 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。

* bm_image output

  输出参数。输出 bm_image，bm_image 需要外部调用 bmcv_image_create 创建。image 内存可以通过 bm_image_alloc_dev_mem 来开辟新的内存，或者使用 bmcv_image_attach 来 attach 已有的内存。如果不主动分配将在 api 内部进行自行分配。

* int ksize = 1 / 3

  Laplacian核的大小，必须是1或3。


**返回值说明：**

* BM_SUCCESS: 成功

* 其他: 失败


**格式支持：**

1. 该接口目前支持以下 image_format:

+-----+------------------------+------------------------+
| num | input image_format     | output image_format    |
+=====+========================+========================+
| 1   | FORMAT_GRAY            | FORMAT_GRAY            |
+-----+------------------------+------------------------+

2. 目前支持以下 data_type:

+-----+--------------------------------+
| num | data_type                      |
+=====+================================+
| 1   | DATA_TYPE_EXT_1N_BYTE          |
+-----+--------------------------------+


**注意事项：**

1. 在调用该接口之前必须确保输入的 image 内存已经申请。

2. input output 的 data_type必须相同。

3. 目前支持图像宽高范围为2x2-4096x4096。



**示例代码**

    .. code-block:: c

        #define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

        int height = 1080;
        int width = 1920;
        unsigned int ksize = 3;
        bm_image_format_ext fmt = FORMAT_GRAY;
        bm_image_data_format_ext data_type = DATA_TYPE_EXT_1N_BYTE;
        bm_image input;
        bm_image output;
        bm_status_t ret = BM_SUCCESS;
        bm_handle_t handle;

        ret = bm_dev_request(&handle, 0);
        if (ret != BM_SUCCESS) {
            printf("bm_dev_request failed. ret = %d\n", ret);
            exit(-1);
        }

        ret = bm_image_create(handle, height, width, fmt, data_type, &input, NULL);
        if (ret != BM_SUCCESS) {
            printf("bm_image_createe failed. ret = %d\n", ret);
            bm_dev_free(handle);
            return ret;
        }
        ret = bm_image_alloc_dev_mem(input, 2);
        if (ret != BM_SUCCESS) {
            printf("bm_image_alloc_dev_mem failed. ret = %d\n", ret);
            bm_image_destroy(&input);
            bm_dev_free(handle);
            return ret;
        }
        ret = bm_image_create(handle, height, width, fmt, data_type, &output, NULL);
        if (ret != BM_SUCCESS) {
            printf("bm_image_create failed. ret = %d\n", ret);
            bm_image_destroy(&input);
            bm_dev_free(handle);
            return ret;
        }
        ret = bm_image_alloc_dev_mem(output, 2);
        if (ret != BM_SUCCESS) {
            printf("bm_image_alloc_dev_mem failed. ret = %d\n", ret);
            bm_image_destroy(&input);
            bm_image_destroy(&output);
            bm_dev_free(handle);
            return ret;
        }

        unsigned char* input_sys = (unsigned char*)malloc(width * height * sizeof(unsigned char));
        unsigned char* out_sys = (unsigned char*)malloc(width * height * sizeof(unsigned char));

        struct timespec tp;
        clock_gettime(NULL, &tp);
        srand(tp.tv_nsec);

        for (int i = 0; i < height * width; ++i) {
            input_sys[i] = rand() % 256;
        }

        ret = bm_image_copy_host_to_device(input, (void **)&input_sys);
        if (ret != BM_SUCCESS) {
            printf("bm_image_copy_host_to_device failed. ret = %d\n", ret);
            bm_image_destroy(&input);
            bm_image_destroy(&output);
            bm_dev_free(handle);
            return ret;
        }
        ret = bmcv_image_laplacian(handle, input, output, ksize);
        if (ret != BM_SUCCESS) {
            printf("bmcv_image_laplacian failed. ret = %d\n", ret);
            bm_image_destroy(&input);
            bm_image_destroy(&output);
            bm_dev_free(handle);
            return ret;
        }
        ret = bm_image_copy_device_to_host(output, (void **)&out_sys);
        if (ret != BM_SUCCESS) {
            printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
            bm_image_destroy(&input);
            bm_image_destroy(&output);
            bm_dev_free(handle);
            return ret;
        }

        bm_image_destroy(input);
        bm_image_destroy(output);
        bm_dev_free(handle);
        free(input_sys);
        free(out_sys);