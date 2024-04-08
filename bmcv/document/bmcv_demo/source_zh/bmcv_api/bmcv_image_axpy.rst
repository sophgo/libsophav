bmcv_axpy
=========

该接口实现 F = A * X + Y，其中 A 是常数，大小为 n * c ，F 、X 、Y 都是大小为n * c * h * w的矩阵。


**接口形式：**

    .. code-block:: c

        bm_status_t bmcv_image_axpy(
                    bm_handle_t handle,
                    bm_device_mem_t tensor_A,
                    bm_device_mem_t tensor_X,
                    bm_device_mem_t tensor_Y,
                    bm_device_mem_t tensor_F,
                    int input_n,
                    int input_c,
                    int input_h,
                    int input_w);


**参数说明：**

* bm_handle_t handle

  输入参数。 bm_handle 句柄。

* bm_device_mem_t tensor_A

  输入参数。存放常数 A 的设备内存地址。

* bm_device_mem_t tensor_X

  输入参数。存放矩阵X的设备内存地址。

* bm_device_mem_t tensor_Y

  输入参数。存放矩阵Y的设备内存地址。

* bm_device_mem_t tensor_F

  输出参数。存放结果矩阵F的设备内存地址。

* int input_n

  输入参数。n 维度大小。

* int input_c

  输入参数。c 维度大小。

* int input_h

  输入参数。h 维度大小。

* int input_w

  输入参数。w 维度大小。


**返回值说明：**

* BM_SUCCESS: 成功

* 其他: 失败


**示例代码**

    .. code-block:: c

        #define N 10
        #define C 256
        #define H 8
        #define W 8
        #define TENSOR_SIZE (N * C * H * W)
        #define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

        bm_handle_t handle;
        bm_status_t ret = BM_SUCCESS;

        ret = bm_dev_request(&handle, 0);
        if (ret != BM_SUCCESS) {
            printf("request dev failed\n");
            return BM_ERR_FAILURE;
        }

        float* tensor_X = (float*)malloc(TENSOR_SIZE * sizeof(float));
        float* tensor_A = (float*)malloc(N * C * sizeof(float));
        float* tensor_Y = (float*)malloc(TENSOR_SIZE * sizeof(float));
        float* tensor_F = (float*)malloc(TENSOR_SIZE * sizeof(float));

        for (int idx = 0; idx < TENSOR_SIZE; ++idx) {
            tensor_X[idx] = (float)idx - 5.0f;
            tensor_Y[idx] = (float)idx / 3.0f - 8.2f;
        }

        for (int idx = 0; idx < N * C; ++idx) {
            tensor_A[idx] = (float)idx * 1.5f + 1.0f;
        }

        struct timeval t1, t2;
        gettimeofday_(&t1);
        ret = bmcv_image_axpy(handle, bm_mem_from_system((void *)tensor_A), bm_mem_from_system((void *)tensor_X),
                              bm_mem_from_system((void *)tensor_Y), bm_mem_from_system((void *)tensor_F),
                              N, C, H, W);
        gettimeofday_(&t2);
        printf("bmcv_image_axpy TPU using time= %ld(us)\n", TIME_COST_US(t1, t2));
        if (ret != BM_SUCCESS) {
            printf("bmcv_image_axpy failed. ret = %d\n", ret);
            goto exit;
        }

        exit:
        free(tensor_A);
        free(tensor_X);
        free(tensor_Y);
        free(tensor_F);
        bm_dev_free(handle);