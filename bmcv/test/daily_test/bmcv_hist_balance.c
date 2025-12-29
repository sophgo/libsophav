#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "bmcv_api_ext_c.h"

int main()
{
    int height = rand() % 8192 + 1;
    int width = rand() % 8192 + 1;
    int ret = 0;
    bm_handle_t handle;
    int i;

    ret = bm_dev_request(&handle, 0);
    if (ret) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return ret;
    }

    int len = height * width;

    uint8_t* inputHost = (uint8_t*)malloc(len * sizeof(uint8_t));
    uint8_t* output_tpu = (uint8_t*)malloc(len * sizeof(uint8_t));

    for (i = 0; i < len; ++i) {
        inputHost[i] = (uint8_t)(rand() % 256);
    }

    bm_device_mem_t input, output;
    int H = height;
    int W = width;

    ret = bm_malloc_device_byte(handle, &output, H * W * sizeof(uint8_t));
    ret = bm_malloc_device_byte(handle, &input, H * W * sizeof(uint8_t));
    ret = bm_memcpy_s2d(handle, input, inputHost);

    ret = bmcv_hist_balance(handle, input, output, H, W);
    ret = bm_memcpy_d2s(handle, output_tpu, output);

    bm_free_device(handle, input);
    bm_free_device(handle, output);

    free(inputHost);
    free(output_tpu);

    bm_dev_free(handle);
    return ret;
}
