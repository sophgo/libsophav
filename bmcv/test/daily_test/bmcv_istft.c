#include "bmcv_api_ext_c.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define M_PI 3.14159265358979323846

enum Pad_mode {
    CONSTANT = 0,
    REFLECT = 1,
};

enum Win_mode {
    HANN = 0,
    HAMM = 1,
};

typedef struct {
    float real;
    float imag;
} Complex;

int main()
{
    bm_handle_t handle;
    int ret = 0;
    int i;
    int L = 4096;
    bool real_input = true;
    int pad_mode = REFLECT;
    int win_mode = HANN;
    int n_fft = 4096;
    int hop_length = n_fft / 4;
    bool norm = true;

    ret = (int)bm_dev_request(&handle, 0);
    if (ret) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return ret;
    }

    int num_frames = 1 + L / hop_length;
    int row_num = n_fft / 2 + 1;

    float* input_R = (float*)malloc(row_num * num_frames * sizeof(float));
    float* input_I = (float*)malloc(row_num * num_frames * sizeof(float));
    float* YRHost_cpu = (float*)malloc(L * sizeof(float));
    float* YRHost_tpu = (float*)malloc(L * sizeof(float));
    float* YIHost_cpu = (float*)malloc(L * sizeof(float));
    float* YIHost_tpu = (float*)malloc(L * sizeof(float));

    memset(input_R, 0, row_num * num_frames * sizeof(float));
    memset(input_I, 0, row_num * num_frames * sizeof(float));

    for (i = 0; i < row_num * num_frames; i++) {
        input_R[i] = (float)rand() / RAND_MAX;
        input_I[i] = (float)rand() / RAND_MAX;
    }

    ret = bmcv_istft(handle, input_R, input_I, YRHost_tpu, YIHost_tpu, 1, L, real_input,
                    pad_mode, n_fft, win_mode, hop_length, norm);

    free(input_R);
    free(input_I);
    free(YRHost_cpu);
    free(YIHost_cpu);
    free(YRHost_tpu);
    free(YIHost_tpu);


    bm_dev_free(handle);
    return ret;
}

