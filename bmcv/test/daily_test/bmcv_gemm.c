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
    bool if_A_trans = rand() % 2;
    bool if_B_trans = rand () % 2;
    int ret = 0;
    bm_handle_t handle;

    if (if_A_trans) {
        if_B_trans = true;
    }

    ret = bm_dev_request(&handle, 0);
    if (ret) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return ret;
    }

    float* src_A = (float*)malloc(M * K * sizeof(float));
    float* src_B = (float*)malloc(N * K * sizeof(float));
    float* src_C = (float*)malloc(M * N * sizeof(float));
    int lda = if_A_trans ? M : K;
    int ldb = if_B_trans ? K : N;

    ret = assign_values_to_matrix(src_A, M * K);
    ret = assign_values_to_matrix(src_B, N * K);
    ret = assign_values_to_matrix(src_C, M * N);


    ret= bmcv_gemm(handle, if_A_trans, if_B_trans, M, N, K, alpha, bm_mem_from_system((void *)src_A),
                    lda, bm_mem_from_system((void *)src_B), ldb, beta,
                    bm_mem_from_system((void *)src_C), N);

    free(src_A);
    free(src_B);
    free(src_C);

    bm_dev_free(handle);
    return ret;
}
