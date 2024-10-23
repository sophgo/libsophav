bmcv_istft
============

接口形式如下：

    .. code-block:: c

        bm_status_t bmcv_stft(
                    bm_handle_t handle,
                    float* XRHost, float* XIHost,
                    float* YRHost, float* YIHost,
                    int batch, int L,
                    bool realInput, int pad_mode,
                    int n_fft, int win_mode, int hop_len,
                    bool normalize);

**参数说明：**

* bm_handle_t handle

  输入参数。bm_handle 句柄。

* float\* XRHost

  输入参数。输入信号的实部地址，。空间大小为 (n_fft / 2 + 1) * (1 + L / (n_fft / 4))。

* float\* XIHost

  输入参数。输入信号的虚部地址，空间大小为 (n_fft / 2 + 1) * (1 + L / (n_fft / 4))。

* float\* YRHost

  输入参数。输出信号的实部地址，空间大小为 batch * L。

* float\* YIHost

  输入参数。输出信号的虚部地址，空间大小为 batch * L。

* int batch

  输入参数。batch数量。

* int L

  输入参数。每个batch的信号长度。

* bool realInput

  输入参数。输出的信号是否为实数， false 为复数， true 为实数。

* int pad_mode

  输入参数。信号填充模式, 0 为 constant, 1 为 reflect。

* int n_fft

  输入参数。每个 L 信号长度做 FFT 的长度

* int win_mode

  输入参数。信号加窗模式, 0 为 hanning窗, 1 为 hamming窗。

* int hop_len

  输入参数。信号做 FFT 的滑动距离，一般为 n_fft / 4 或者 n_fft / 2。

* bool normalize

  输入参数。输出结果是否要进行归一化。


**返回值说明:**

* BM_SUCCESS: 成功

* 其他: 失败


**注意事项：**

1. 本接口只处理一维信号的ISTFT变换。


**示例代码**

    .. code-block:: c

        int n_fft = 4096; // n_fft_length
        int hop_length = n_fft / 4; // hop_length
        int batch = 1;
        bool realInput = true;
        int pad_mode = 1;
        int win_mode = 0;
        int L= 4096;
        float* YRHost = (float*)malloc(L * batch * sizeof(float));
        float* YIHost = (float*)malloc(L * batch * sizeof(float));
        int num_frames = 1 + L / hop_length; // col
        int row_num = n_fft / 2 + 1; //row
        float* XRHost = (float*)malloc(batch * row_num * num_frames * sizeof(float));
        float* XIHost = (float*)malloc(batch * row_num * num_frames * sizeof(float));
        bm_status_t ret = BM_SUCCESS;
        bm_handle_t handle;
        bool norm = true;

        ret = bm_dev_request(&handle, 0);
        if (ret != BM_SUCCESS) {
            printf("Create bm handle failed. ret = %d\n", ret);
            return ret;
        }

        ret = bmcv_istft(handle, XRHost, XIHost, YRHost, YIHost, batch, L,
                        realInput, pad_mode, n_fft, win_mode, hop_length, norm);
        if (ret != BM_SUCCESS) {
            printf("bmcv_stft failed!\n");
            return ret;
        }

        free(XRHost);
        free(XIHost);
        free(YRHost);
        free(YIHost);
        bm_dev_free(handle);
