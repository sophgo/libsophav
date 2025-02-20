#include "bmcv_api_ext_c.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int main() {
    int H = 1024;
    int W = 1024;
    int C = 3;
    int dim = 3;
    int data_type = 0; // 0: float; 1: uint8

    int channels[3] = {0, 1, 2};
    int histSizes[3] = {32, 32, 32};
    float ranges[6] = {0, 256, 0, 256, 0, 256};


    int totalHists = 1;
    int ret = 0;
    int i, j;
    int Num = 0;
    bm_handle_t handle;

    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return -1;
    }
    for (i = 0; i < dim; i++) {
        totalHists *= histSizes[i];
    }
    float* output_tpu = (float*)malloc(totalHists * sizeof(float));
    memset(output_tpu, 0, totalHists * sizeof(float));

    float* input_host = (float*)malloc(C * H * W * sizeof(float));

    // randomly initialize input_host
    for (i = 0; i < C; i++) {
        for (j = 0; j < H * W; j++) {
        Num = (int)ranges[2*C-1];
        input_host[i * H * W + j] = (float)(rand() % Num);
        }
    }

    bm_device_mem_t input, output;
    ret = bm_malloc_device_byte(handle, &output, totalHists * sizeof(float));
    ret = bm_malloc_device_byte(handle, &input, C * H * W * sizeof(float));
    ret = bm_memcpy_s2d(handle, input, input_host);
    ret = bmcv_calc_hist(handle, input, output, C, H, W, channels, dim, histSizes, ranges, data_type);

    ret = bm_memcpy_d2s(handle, output_tpu, output);
    if (ret != BM_SUCCESS) {
        printf("test calc hist failed. ret = %d\n", ret);
        bm_free_device(handle, input);
    }

    free(input_host);
    free(output_tpu);
    bm_dev_free(handle);
    return ret;
}