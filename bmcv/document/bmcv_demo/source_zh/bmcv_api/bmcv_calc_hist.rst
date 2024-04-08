bmcv_calc_hist
==================

直方图
_______


**接口形式：**

    .. code-block:: c

        bm_status_t bmcv_calc_hist(
                    bm_handle_t handle,
                    bm_device_mem_t input,
                    bm_device_mem_t output,
                    int C,
                    int H,
                    int W,
                    const int* channels,
                    int dims,
                    const int* histSizes,
                    const float* ranges,
                    int inputDtype);


**参数说明：**

* bm_handle_t handle

  输入参数。 bm_handle 句柄。

* bm_device_mem_t input

  输入参数。该device memory空间存储了输入数据，类型可以是float32或者uint8，由参数inputDtype决定。其大小为 C \* H \* W \* sizeof(Dtype)。

* bm_device_mem_t output

  输出参数。该device memory空间存储了输出结果，类型为float，其大小为histSizes[0] \* histSizes[1] \* …… \* histSizes[n] \* sizeof(float)。

* int C

  输入参数。输入数据的通道数量。

* int H

  输入参数。输入数据每个通道的高度。

* int W

  输入参数。输入数据每个通道的宽度。

* const int\* channels

  输入参数。需要计算直方图的 channel 列表，其长度为 dims，每个元素的值必须小于 C。

* int dims

  输入参数。输出的直方图维度，要求不大于 3。

* const int\* histSizes

  输入参数。对应每个 channel 统计直方图的份数，其长度为 dims。

* const float\* ranges

  输入参数。每个通道参与统计的范围，其长度为2 * dims。

* int inputDtype

  输入参数。输入数据的类型：0 表示 float，1 表示 uint8。


**返回值说明：**

* BM_SUCCESS: 成功

* 其他: 失败


**示例代码**

    .. code-block:: c

        int H = 1024;
        int W = 1024;
        int C = 3;
        int dim = 3;
        int channels[3] = {0, 1, 2};
        int histSizes[] = {32, 32, 32};
        float ranges[] = {0, 256, 0, 256, 0, 256};
        int totalHists = 1;

        for (int i = 0; i < dim; ++i) {
            totalHists *= histSizes[i];
        }

        float *inputHost = (float*)malloc(C * H * W * sizeof(float));
        float *outputHost = (float*)malloc(totalHists * sizeof(float));

        struct timespec tp;
        clock_gettime(NULL, &tp);
        srand(tp.tv_nsec);

        for (int i = 0; i < C; ++i) {
            for (int j = 0; j < H * W; ++j) {
                inputHost[i * H * W + j] = (float)(rand() % 256);
            }
        }

        bm_handle_t handle;
        bm_status_t ret = bm_dev_request(&handle, 0);
        if (ret != BM_SUCCESS) {
            printf("bm_dev_request failed. ret = %d\n", ret);
            exit(-1);
        }

        bm_device_mem_t input, output;
        ret = bm_malloc_device_byte(handle, &input, C * H * W * sizeof(float));
        if (ret != BM_SUCCESS) {
            printf("bm_malloc_device_byte failed. ret = %d\n", ret);
            exit(-1);
        }
        ret = bm_memcpy_s2d(handle, input, inputHost);
        if (ret != BM_SUCCESS) {
            printf("bm_memcpy_s2d failed. ret = %d\n", ret);
            exit(-1);
        }
        ret = bm_malloc_device_byte(handle, &output, totalHists * 4);
        if (ret != BM_SUCCESS) {
            printf("bm_malloc_device_byte failed. ret = %d\n", ret);
            exit(-1);
        }
        ret = bmcv_calc_hist(handle, input, output, C, H, W, channels, dim, histSizes, ranges, 0);
        if (ret != BM_SUCCESS) {
            printf("bmcv_calc_hist failed. ret = %d\n", ret);
            exit(-1);
        }
        ret = bm_memcpy_d2s(handle, outputHost, output);
        if (ret != BM_SUCCESS) {
            printf("bm_memcpy_d2s failed. ret = %d\n", ret);
            exit(-1);
        }

        bm_free_device(handle, input);
        bm_free_device(handle, output);
        bm_dev_free(handle);
        free(inputHost);
        free(outputHost);


带权重的直方图
_______________


**接口形式：**

    .. code-block:: c

        bm_status_t bmcv_calc_hist_with_weight(
                    bm_handle_t handle,
                    bm_device_mem_t input,
                    bm_device_mem_t output,
                    const float* weight,
                    int C,
                    int H,
                    int W,
                    const int* channels,
                    int dims,
                    const int* histSizes,
                    const float* ranges,
                    int inputDtype);


**参数说明：**

* bm_handle_t handle

  输入参数。 bm_handle 句柄。

* bm_device_mem_t input

  输入参数。该device memory空间存储了输入数据，其大小为 C \* H \* W \* sizeof(Dtype)。

* bm_device_mem_t output

  输出参数。该device memory空间存储了输出结果，类型为float，其大小为histSizes[0] \* histSizes[1] \* …… \* histSizes[n] \* sizeof(float)。

* const float\* weight

  输入参数。channel内部每个元素在统计直方图时的权重，其大小为H \* W \* sizeof(float)，如果所有值全为 1 则与普通直方图功能相同。

* int C

  输入参数。输入数据的通道数量。

* int H

 输入参数。输入数据每个通道的高度。

* int W

  输入参数。输入数据每个通道的宽度。

* const int\* channels

  输入参数。需要计算直方图的 channel 列表，其长度为 dims，每个元素的值必须小于 C。

* int dims

  输入参数。输出的直方图维度，要求不大于 3。

* const int\* histSizes

  输入参数。对应每个channel统计直方图的份数，其长度为 dims。

* const float\* ranges

  输入参数。每个通道参与统计的范围，其长度为2 \* dims。

* int inputDtype

  输入参数。输入数据的类型：0 表示float，1 表示uint8。


**返回值说明：**

* BM_SUCCESS: 成功

* 其他: 失败