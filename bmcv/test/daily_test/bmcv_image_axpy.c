#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "bmcv_api_ext_c.h"

#define N (10)
#define C 256 //(64 * 2 + (64 >> 1))
#define H 8
#define W 8
#define TENSOR_SIZE (N * C * H * W)

int main(){
    int trials = 1;
    int ret = 0;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return -1;
    }

    float *tensor_X = malloc(TENSOR_SIZE * sizeof(float));
    float *tensor_A = malloc(N * C * sizeof(float));
    float *tensor_Y = malloc(TENSOR_SIZE * sizeof(float));
    float *tensor_F = malloc(TENSOR_SIZE * sizeof(float));
    int idx_trial;

    for (idx_trial = 0; idx_trial < trials; idx_trial++) {
        for (int idx = 0; idx < TENSOR_SIZE; idx++) {
        tensor_X[idx] = (float)idx - 5.0f;
        tensor_Y[idx] = (float)idx/3.0f - 8.2f;  //y
        }

        for (int idx = 0; idx < N*C; idx++) {
        tensor_A[idx] = (float)idx * 1.5f + 1.0f;
        }

        ret = bmcv_image_axpy(handle,
                            bm_mem_from_system((void *)tensor_A),
                            bm_mem_from_system((void *)tensor_X),
                            bm_mem_from_system((void *)tensor_Y),
                            bm_mem_from_system((void *)tensor_F),
                            N, C, H, W);
        }
    free(tensor_X);
    free(tensor_A);
    free(tensor_Y);
    free(tensor_F);

    bm_dev_free(handle);
    return ret;
}
