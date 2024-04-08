bmcv_image_pyramid_down
=======================

该接口实现图像高斯金字塔操作中的向下采样。


**接口形式：**

    .. code-block:: c

        bm_status_t bmcv_image_pyramid_down(
                    bm_handle_t handle,
                    bm_image input,
                    bm_image output);


**参数说明：**

* bm_handle_t handle

  输入参数。 bm_handle 句柄。

* bm_image input

  输入参数。输入图像 bm_image，bm_image 需要外部调用 bmcv_image_create 创建。bm_image 的内存可以使用 bm_image_alloc_dev_mem 或者bm_image_copy_host_to_device 来开辟新的内存，或者使用 bmcv_image_attach
  来 attach 已有的内存。

* bm_image output

  输出参数。输出图像 bm_image，bm_image 需要外部调用 bmcv_image_create 创建。bm_image的内存可以使用 bm_image_alloc_dev_mem 或者bm_image_copy_host_to_device 来开辟新的内存，或者使用 bmcv_image_attach
  来 attach 已有的内存。


**返回值说明：**

* BM_SUCCESS: 成功

* 其他: 失败


**格式支持：**

该接口目前支持以下 image_format 与 data_type:

+-----+------------------------+------------------------+
| num | image_format           | data_type              |
+=====+========================+========================+
| 1   | FORMAT_GRAY            | DATA_TYPE_EXT_1N_BYTE  |
+-----+------------------------+------------------------+


**注意事项：**

1. 目前支持图像的最小宽为3，最小高为3，最大宽为2043，最大高为4096。


**示例代码**

    .. code-block:: c

        int height = 1080;
        int width = 1920;
        int ow = height / 2;
        int oh = width / 2;
        int channel = 1;
        unsigned char* input_data = (unsigned char*)malloc(width * height * channel * sizeof(input_data));
        unsigned char* output = (unsigned char*)malloc(ow * oh * channel * sizeof(input_data));

        struct timespec tp;
        clock_gettime(NULL, &tp);
        srand(tp.tv_nsec);

        for (int i = 0; i < height * channel; i++) {
            for (int j = 0; j < width; j++) {
                input_data[i * width + j] = rand() % 256;
            }
        }

        bm_handle_t handle;
        bm_status_t ret = bm_dev_request(&handle, 0);
        if (ret != BM_SUCCESS) {
            printf("Create bm handle failed. ret = %d\n", ret);
            return -1;
        }

        bm_image_format_ext fmt = FORMAT_GRAY;
        bm_image img_i, img_o;
        ret = bm_image_create(handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, &img_i, NULL);
        if (ret != BM_SUCCESS) {
            printf("bm_image_createe failed. ret = %d\n", ret);
            bm_dev_free(handle);
            return ret;
        }
        ret = bm_image_create(handle, oh, ow, fmt, DATA_TYPE_EXT_1N_BYTE, &img_o, NULL);
        if (ret != BM_SUCCESS) {
            printf("bm_image_create failed. ret = %d\n", ret);
            bm_image_destroy(&img_i);
            bm_dev_free(handle);
            return ret;
        }
        ret = bm_image_alloc_dev_mem(img_i, 2);
        if (ret != BM_SUCCESS) {
            printf("bm_image_alloc_dev_mem failed. ret = %d\n", ret);
            bm_image_destroy(&img_i);
            bm_image_destroy(&img_o);
            bm_dev_free(handle);
            return ret;
        }
        ret = bm_image_alloc_dev_mem(img_o, 2);
        if (ret != BM_SUCCESS) {
            printf("bm_image_alloc_dev_mem failed. ret = %d\n", ret);
            bm_image_destroy(&img_i);
            bm_image_destroy(&img_o);
            bm_dev_free(handle);
            return ret;
        }

        ret = bm_image_copy_host_to_device(img_i, (void **)(&input));
        if (ret != BM_SUCCESS) {
            printf("bm_image_copy_host_to_device failed. ret = %d\n", ret);
            bm_image_destroy(&img_i);
            bm_image_destroy(&img_o);
            bm_dev_free(handle);
            return ret;
        }

        struct timeval t1, t2;
        gettimeofday_(&t1);
        ret = bmcv_image_pyramid_down(handle, img_i, img_o);
        gettimeofday_(&t2);
        printf("bmcv_image_pyramid_down TPU using time= %ld(us)\n", TIME_COST_US(t1, t2));
        if (ret != BM_SUCCESS) {
            printf("bmcv_image_pyramid_down failed. ret = %d\n", ret);
            bm_image_destroy(&img_i);
            bm_image_destroy(&img_o);
            bm_dev_free(handle);
            return ret;
        }
        ret = bm_image_copy_device_to_host(img_o, (void **)(&output));
        if (ret != BM_SUCCESS) {
            printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
            bm_image_destroy(&img_i);
            bm_image_destroy(&img_o);
            bm_dev_free(handle);
            return ret;
        }

        bm_image_destroy(&img_i);
        bm_image_destroy(&img_o);
        bm_dev_free(handle);
        free(input_data);
        free(output);