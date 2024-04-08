bmcv_min_max
============

对于存储于device memory中连续空间的一组数据，该接口可以获取这组数据中的最大值和最小值。


**接口形式:**

    .. code-block:: c

        bm_status_t bmcv_min_max(
                    bm_handle_t handle,
                    bm_device_mem_t input,
                    float* minVal,
                    float* maxVal,
                    int len);


**参数说明：**

* bm_handle_t handle

  输入参数。bm_handle 句柄

* bm_device_mem_t input

  输入参数。输入数据的 device 地址。

* float\* minVal

  输出参数。运算后得到的最小值结果, 如果为 NULL 则不计算最小值。

* float\* maxVal

  输出参数。运算后得到的最大值结果，如果为 NULL 则不计算最大值。

* int len

  输入参数。输入数据的长度。


**返回值说明:**

* BM_SUCCESS: 成功

* 其他: 失败


**注意事项：**

1. 输入参数len支持范围为50～260144。



**示例代码**

    .. code-block:: c

        bm_handle_t handle;
        bm_status_t ret = BM_SUCCESS;
        int len = 1000;
        float max = 0;
        float min = 0;
        float* input = (float*)malloc(len * sizeof(float));

        ret = bm_dev_request(&handle, 0);
        if (ret != BM_SUCCESS) {
            printf("Create bm handle failed. ret = %d\n", ret);
            free(input);
            return -1;
        }

        struct timespec tp;
        clock_gettime(NULL, &tp);
        srand(tp.tv_nsec);

        for (int i = 0; i < len; i++) {
            input[i] = (float)(rand() % 1000) / 10.0;
        }

        bm_device_mem_t input_mem;
        ret = bm_malloc_device_byte(handle, input_mem, len * sizeof(float));
        if (ret != BM_SUCCESS) {
            printf("bm_malloc_device_byte failed. ret = %d\n", ret);
            exit(-1);
        }
        ret = bm_memcpy_s2d(handle, input_mem, input);
        if (ret != BM_SUCCESS) {
            printf("bm_memcpy_s2d failed. ret = %d\n", ret);
            exit(-1);
        }
        ret = bmcv_min_max(handle, input_mem, &min, &max, len);
        if (ret != BM_SUCCESS) {
            printf("bmcv_min_max failed. ret = %d\n", ret);
            exit(-1);
        }

        bm_free_device(handle, input_mem);
        bm_dev_free(handle);
        free(input);