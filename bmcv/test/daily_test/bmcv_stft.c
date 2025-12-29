#include "bmcv_api_ext_c.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define ERR_MAX 1e-3
#define M_PI 3.14159265358979323846
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

enum bdry{
    zeros = 0,
    constant,
    even,
    odd,
    none,
};

enum win{
    HANN = 0,
    HAMM = 1,
};

int main() {
    struct timespec tp;
    clock_gettime(0, &tp);
    srand(tp.tv_nsec);
    bm_handle_t handle;
    int ret = 0;
    int batch = 1;

    int fs = 1024;
    int duration = 10;
    int window = HAMM;
    int nperseg = 1024;
    int noverlap = 512;
    int nfft = 1215;
    bool return_onesided = true;
    int boundary = even;
    bool padded = true;
    bool real_input = true;
    ret = (int)bm_dev_request(&handle, 0);
    if (ret) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return ret;
    }

    int xlen = fs * duration;
    float* XRHost = (float*)malloc(batch * xlen * sizeof(float));
    float* XIHost = (float*)malloc(batch * xlen * sizeof(float));

    // Generate signal
    memset(XRHost, 0, xlen * batch * sizeof(float));
    memset(XIHost, 0, xlen * batch * sizeof(float));

    int hop = nperseg - noverlap;
    int xlen_ = 0;
    if (boundary == zeros || boundary == constant || boundary == even || boundary == odd) {
        xlen_ = xlen + 2 * hop;
    } else {
        xlen_ = xlen;
    }
    int padded_len = 0;
    if (padded) {
        padded_len = (xlen_ - nperseg) % hop == 0 ? 0 : hop - (xlen_ - nperseg) % hop;
    }
    xlen_ += padded_len;

    int L = 1 + (xlen_ - nperseg) / hop;
    int row_num = return_onesided ? nfft / 2 + 1 : nfft;
    float* YRHost = (float*)malloc(batch * L * row_num * sizeof(float));
    float* YIHost = (float*)malloc(batch * L * row_num * sizeof(float));

    printf("the xlen = %d, the batch=%d, the L=%d, the nfft=%d, the realinput = %d, the boundary=%d, the window=%d\n",
            xlen, batch, L, nfft, real_input, boundary, window);

    float *f = (float*)malloc(nfft * sizeof(float));
    float *t = (float*)malloc(L * sizeof(float));

    struct timeval t1, t2;

    gettimeofday(&t1, NULL);
    ret = bmcv_stft(handle, XRHost, XIHost, batch, xlen, fs, window, nperseg, noverlap,
                    nfft, return_onesided, boundary, padded, real_input, YRHost, YIHost, f, t);
    if (ret != BM_SUCCESS) {
        printf("bmcv_stft failed!\n");
        return ret;
    }
    gettimeofday(&t2, NULL);
    printf("STFT TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    if (ret != 0) {
        printf("tpu_stft failed!\n");
    }

    printf("------Test STFT All Success!------\n");
    bm_dev_free(handle);
    free(f);
    free(t);
    free(XRHost);
    free(XIHost);
    free(YRHost);
    free(YIHost);

    return ret;
}