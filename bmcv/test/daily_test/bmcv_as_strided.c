#include "bmcv_api_ext_c.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    int input_row = 5;
    int input_col = 5;
    int output_row = 3;
    int output_col = 3;
    int row_stride = 1;
    int col_stride = 2;
    int input_size = input_row * input_col;
    int output_size = output_row * output_col;
    int ret = 0;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);

    float* input_data = (float*)malloc(input_size  * sizeof(float));
    float* tpu_output = (float*)malloc(output_size * sizeof(float));

    for (int i = 0; i < input_size; i++) {
        input_data[i] = (float)rand() / (float)RAND_MAX * 100;
    }

    memset(tpu_output, 0, output_size * sizeof(float));

    bm_device_mem_t input_dev_mem, output_dev_mem;
    ret = bm_malloc_device_byte(handle, &input_dev_mem, input_size * sizeof(float));
    ret = bm_malloc_device_byte(handle, &output_dev_mem, output_size * sizeof(float));

    ret = bm_memcpy_s2d(handle, input_dev_mem, input_data);

    ret = bmcv_as_strided(handle, input_dev_mem, output_dev_mem, input_row, \
                    input_col, output_row, output_col, row_stride, col_stride);

    ret = bm_memcpy_d2s(handle, tpu_output, output_dev_mem);

    bm_free_device(handle, input_dev_mem);
    bm_free_device(handle, output_dev_mem);

    free(input_data);
    free(tpu_output);
    bm_dev_free(handle);
    return ret;
}