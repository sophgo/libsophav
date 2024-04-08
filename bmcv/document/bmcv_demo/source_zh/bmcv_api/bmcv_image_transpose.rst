bmcv_image_transpose
====================

该接口可以实现图片宽和高的转置。


**接口形式:**

    .. code-block:: c

        bm_status_t bmcv_image_transpose(
                    bm_handle_t handle,
                    bm_image input,
                    bm_image output);


**参数说明:**

* bm_handle_t handle

  输入参数。设备环境句柄，通过调用 bm_dev_request 获取。

* bm_image input

  输入参数。输入图像的 bm_image 结构体。

* bm_image output

  输出参数。输出图像的 bm_image 结构体。


**返回值说明:**

* BM_SUCCESS: 成功

* 其他: 失败


**示例代码**

    .. code-block:: c

        bm_handle_t handle;
        bm_status_t ret = BM_SUCCESS;
        int image_h = 1080;
        int image_w = 1920;

        ret = bm_dev_request(&handle, 0);
        if (ret != BM_SUCCESS) {
            printf("Create bm handle failed. ret = %d\n", ret);
            return -1;
        }

        bm_image src, dst;
        ret = bm_image_create(handle, image_h, image_w, FORMAT_RGB_PLANAR, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
        if (ret != BM_SUCCESS) {
            printf("bm_image_createe failed. ret = %d\n", ret);
            bm_dev_free(handle);
            return ret;
        }
        ret = bm_image_create(handle, image_w, image_h, FORMAT_RGB_PLANAR, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);
        if (ret != BM_SUCCESS) {
            printf("bm_image_createe failed. ret = %d\n", ret);
            bm_image_destroy(&src);
            bm_dev_free(handle);
            return ret;
        }
        ret = bm_image_alloc_dev_mem(src, 2);
        if (ret != BM_SUCCESS) {
            printf("bm_image_alloc_dev_mem failed. ret = %d\n", ret);
            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            bm_dev_free(handle);
            return ret;
        }
        ret = bm_image_alloc_dev_mem(dst, 2);
        if (ret != BM_SUCCESS) {
            printf("bm_image_alloc_dev_mem failed. ret = %d\n", ret);
            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            bm_dev_free(handle);
            return ret;
        }

        uint8_t* src_ptr = (uint8_t*)malloc(image_h * image_w * 3 * sizeof(uint8_t));
        uint8_t* dst_ptr = (uint8_t*)malloc(image_h * image_w * 3 * sizeof(uint8_t));

        struct timespec tp;
        clock_gettime(NULL, &tp);
        srand(tp.tv_nsec);

        for (int i = 0; i < image_h * image_w * 3; i++) {
            src_ptr[i] = rand() % 256;
        }

        ret = bm_image_copy_host_to_device(src, (void **)&src_ptr);
        if (ret != BM_SUCCESS) {
            printf("bm_image_copy_host_to_device failed. ret = %d\n", ret);
            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            bm_dev_free(handle);
            return ret;
        }

        ret = bmcv_image_transpose(handle, src, dst);
        if (ret != BM_SUCCESS) {
            printf("bmcv_image_transpose failed. ret = %d\n", ret);
            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            bm_dev_free(handle);
            return ret;
        }
        ret = bm_image_copy_device_to_host(dst, (void **)&dst_ptr);
        if (ret != BM_SUCCESS) {
            printf("bm_image_copy_device_to_host failed. ret = %d\n", ret);
            bm_image_destroy(&src);
            bm_image_destroy(&dst);
            bm_dev_free(handle);
            return ret;
        }

        bm_image_destroy(src);
        bm_image_destroy(dst);
        bm_dev_free(handle);
        free(src_ptr);
        free(dst_ptr);


**注意事项:**

1. 该 API 要求输入和输出的 bm_image 图像格式相同，支持以下格式：

+-----+-------------------------------+
| num | image_format                  |
+=====+===============================+
|  1  | FORMAT_RGB_PLANAR             |
+-----+-------------------------------+
|  2  | FORMAT_BGR_PLANAR             |
+-----+-------------------------------+
|  3  | FORMAT_GRAY                   |
+-----+-------------------------------+

2. 该 API 要求输入和输出的 bm_image 数据类型相同，支持以下类型：

+-----+-------------------------------+
| num | data_type                     |
+=====+===============================+
|  1  | DATA_TYPE_EXT_FLOAT32         |
+-----+-------------------------------+
|  2  | DATA_TYPE_EXT_1N_BYTE         |
+-----+-------------------------------+
|  3  | DATA_TYPE_EXT_4N_BYTE         |
+-----+-------------------------------+
|  4  | DATA_TYPE_EXT_1N_BYTE_SIGNED  |
+-----+-------------------------------+
|  5  | DATA_TYPE_EXT_4N_BYTE_SIGNED  |
+-----+-------------------------------+

3. 输出图像的 width 必须等于输入图像的 height，输出图像的 height 必须等于输入图像的 width ;

4. 输入图像支持带有 stride;

5. 输入输出 bm_image 结构必须提前创建，否则返回失败。

6. 输入 bm_image 必须 attach device memory，否则返回失败

7. 如果输出对象未 attach device memory，则会内部调用 bm_image_alloc_dev_mem 申请内部管理的 device memory，并将转置后的数据填充到 device memory 中。