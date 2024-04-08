bmcv_distance
=============

计算多维空间下多个点与特定一个点的欧式距离，前者坐标存放在连续的device memory中，而特定一个点的坐标通过参数传入。坐标值为float类型。


**接口形式：**

    .. code-block:: c

        bm_status_t bmcv_distance(
                    bm_handle_t handle,
                    bm_device_mem_t input,
                    bm_device_mem_t output,
                    int dim,
                    const float* pnt,
                    int len);


**参数说明：**

* bm_handle_t handle

  输入参数。bm_handle 句柄

* bm_device_mem_t input

  输入参数。存放len个点坐标的 device 空间。其大小为len*dim*sizeof(float)。

* bm_device_mem_t output

  输出参数。存放len个距离的 device 空间。其大小为len*sizeof(float)。

* int dim

  输入参数。空间维度大小。

* const float\* pnt

  输入参数。特定一个点的坐标，长度为dim。

* int len

  输入参数。待求坐标的数量。


**返回值说明:**

* BM_SUCCESS: 成功

* 其他: 失败


**示例代码**

    .. code-block:: c

        int L = 1024 * 1024;
        int dim = 3;
        float pnt[8] = {0};
        float* XHost = (float*)malloc(L * dim * sizeof(float));
        float *YHost = (float*)malloc(L * sizeof(float));

        struct timespec tp;
        clock_gettime(NULL, &tp);
        srand(tp.tv_nsec);

        for (int i = 0; i < dim; ++i) {
            pnt[i] = (rand() % 2 ? 1.f : -1.f) * (rand() % 100 + (rand() % 100) * 0.01);
        }

        for (int i = 0; i < L * dim; ++i) {
            XHost[i] = (rand() % 2 ? 1.f : -1.f) * (rand() % 100 + (rand() % 100) * 0.01);
        }

        bm_handle_t handle;
        bm_status_t ret = BM_SUCCESS;
        bm_device_mem_t XDev, YDev;

        ret = bm_dev_request(&handle, 0);
        if (ret != BM_SUCCESS) {
            printf("bm_dev_request failed. ret = %d\n", ret);
            exit(-1);
        }

        ret = bm_malloc_device_byte(handle, &XDev, L * dim * sizeof(float));
        if (ret != BM_SUCCESS) {
            printf("bm_malloc_device_byte failed. ret = %d\n", ret);
            exit(-1);
        }

        ret = bm_malloc_device_byte(handle, &YDev, L * sizeof(float));
        if (ret != BM_SUCCESS) {
            printf("bm_malloc_device_byte failed. ret = %d\n", ret);
            exit(-1);
        }

        ret = bm_memcpy_s2d(handle, XDev, XHost);
        if (ret != BM_SUCCESS) {
            printf("bm_memcpy_s2d failed. ret = %d\n", ret);
            exit(-1);
        }

        ret = bmcv_distance(handle, XDev, YDev, dim, pnt, L);
        if (ret != BM_SUCCESS) {
            printf("bmcv_distance failed. ret = %d\n", ret);
            exit(-1);
        }

        ret = bm_memcpy_d2s(handle, YHost, YDev);
        if (ret != BM_SUCCESS) {
            printf("bm_memcpy_d2s failed. ret = %d\n", ret);
            exit(-1);
        }

        free(XHost);
        free(YHost);
        bm_free_device(handle, XDev);
        bm_free_device(handle, YDev);
        bm_dev_free(handle);