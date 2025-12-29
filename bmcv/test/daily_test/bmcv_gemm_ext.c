#include "bmcv_api_ext_c.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define GEMM_INPUT_NUM 3
#define GEMM_OUTPUT_NUM 1


static int assign_values_to_matrix(float* matrix, int size)
{
    int i;

    if (matrix == NULL || size <= 0) {
        printf("the assign_values_to_matrix func error!\n");
        return -1;
    }

    for (i = 0; i < size; ++i) {
        matrix[i] = (rand() % 100) * 0.01f;
    }

    return 0;
}


int main()
{
    int M = 1 + rand() % 800;
    int N = 1 + rand() % 800;
    int K = 1 + rand() % 800;
    int rand_sign_a = (rand() % 2 == 0) ? 1 : -1;
    int rand_sign_b = (rand() % 2 == 0) ? 1 : -1;
    float alpha = rand_sign_a * (rand() % 100) * 0.05;
    float beta  = rand_sign_b * (rand() % 100) * 0.05;
    bool is_A_trans = rand() % 2;
    bool is_B_trans = rand () % 2;
    int ret = 0;
    bm_handle_t handle;

    if (is_A_trans) {
        is_B_trans = true;
    }

    ret = bm_dev_request(&handle, 0);
    if (ret) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return ret;
    }


    float* A = (float*)malloc(M * K * sizeof(float));
    float* B = (float*)malloc(N * K * sizeof(float));
    float* C = (float*)malloc(M * N * sizeof(float));
    float* tpu_C = (float*)malloc(M * N * sizeof(float));
    bm_image_data_format_ext in_dtype, out_dtype;

    ret = assign_values_to_matrix(A, M * K);
    ret = assign_values_to_matrix(B, N * K);
    ret = assign_values_to_matrix(C, M * N);
    memset(tpu_C, 0.f, sizeof(float) * M * N);

    in_dtype = DATA_TYPE_EXT_FLOAT32;
    out_dtype = DATA_TYPE_EXT_FLOAT32;
    memset(tpu_C, 0.f, sizeof(float) * M * N);

    if (in_dtype == DATA_TYPE_EXT_FP16 && is_A_trans && M > 64) {
        printf("Error! It only support M <= 64 when A is trans and input_dtype is FP16\n");
        return -1;
    }

    unsigned short* A_fp16 = (unsigned short*)malloc(M * K * sizeof(unsigned short));
    unsigned short* B_fp16 = (unsigned short*)malloc(N * K * sizeof(unsigned short));
    unsigned short* C_fp16 = (unsigned short*)malloc(M * N * sizeof(unsigned short));
    unsigned short* Y_fp16 = (unsigned short*)malloc(M * N * sizeof(unsigned short));
    bm_device_mem_t input_dev_buffer[GEMM_INPUT_NUM];
    bm_device_mem_t output_dev_buffer[GEMM_OUTPUT_NUM];

    ret = bm_malloc_device_byte(handle, &input_dev_buffer[0], M * K * sizeof(float));
    ret = bm_malloc_device_byte(handle, &input_dev_buffer[1], N * K * sizeof(float));
    ret = bm_malloc_device_byte(handle, &input_dev_buffer[2], M * N * sizeof(float));
    ret = bm_memcpy_s2d(handle, input_dev_buffer[0], (void*)A);
    ret = bm_memcpy_s2d(handle, input_dev_buffer[1], (void*)B);
    ret = bm_memcpy_s2d(handle, input_dev_buffer[2], (void*)C);

    ret = bm_malloc_device_byte(handle, &output_dev_buffer[0], M * N * sizeof(float));

    ret = bmcv_gemm_ext(handle, is_A_trans, is_B_trans, M, N, K, alpha, input_dev_buffer[0],
                        input_dev_buffer[1], beta, input_dev_buffer[2], output_dev_buffer[0],
                        in_dtype, out_dtype);

    ret = bm_memcpy_d2s(handle, (void*)tpu_C, output_dev_buffer[0]);

    free(A_fp16);
    free(B_fp16);
    free(C_fp16);
    free(Y_fp16);

    free(A);
    free(B);
    free(C);
    free(tpu_C);

    bm_dev_free(handle);
    return ret;
}
