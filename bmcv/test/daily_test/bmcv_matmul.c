#include "bmcv_api_ext_c.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_misc.h"

static void assign_fix8b_matrix(void* mat, int size, int is_16bit)
{
    int i;

    for (i = 0; i < size; i++) {
        if (is_16bit) {
            *((signed short*)mat + i) = rand() % 65536 - 32768;
        } else {
            *((signed char*)mat + i) = rand() % 256 - 128;
        }
    }
}

int test_matmul()
{
    int M = 1 + rand() % 6;
    int K = 1 + rand() % 512;
    int N = 1 + rand() % 9216;
    // int trans_A = 0;  //whether to transpose
    int trans_B = 0;
    int A_sign = 0;  //unsigned or singned
    int B_sign = 0;
    int result_type = 0;  //0-int8 1-int16 2-flaot
    int right_shift_bit = 1;
    float alpha = 1;
    float beta = 0;
    int ret = 0;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);

    signed char* input_A;
    signed char* input_B;
    void* tpu_out;
    input_A = (signed char*)malloc(M * K * sizeof(signed char));
    input_B = (signed char*)malloc(K * N * sizeof(signed char));

    if (result_type == 0) {
        tpu_out = (signed char*)malloc(M * N * sizeof(signed char));
        memset(tpu_out, 0, M * N * sizeof(signed char));
    }

    assign_fix8b_matrix((void*)input_A, M * K, 0);
    assign_fix8b_matrix((void*)input_B, K * N, 0);

    ret = bmcv_matmul(handle, M, N, K, bm_mem_from_system((void*)input_A),
                    bm_mem_from_system((void*)input_B), bm_mem_from_system(tpu_out), A_sign,
                    B_sign, right_shift_bit, result_type, trans_B, alpha, beta);

    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        bm_dev_free(handle);
        return -1;
    }

    free(input_A);
    free(input_B);
    free(tpu_out);

    bm_dev_free(handle);
    return ret;
}

int test_matmul_u64()
{
    int M = 1 + rand() % 6;
    int K = 1 + rand() % 512;
    int N = 1 + rand() % 9216;
    // int trans_A = 0;  //whether to transpose
    int trans_B = 0;
    int A_sign = 0;  //unsigned or singned
    int B_sign = 0;
    int result_type = 0;  //0-int8 1-int16 2-flaot
    int right_shift_bit = 1;
    float alpha = 1;
    float beta = 0;
    int ret = 0;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);

    signed char* input_A;
    signed char* input_B;
    void* tpu_out;
    input_A = (signed char*)malloc(M * K * sizeof(signed char));
    input_B = (signed char*)malloc(K * N * sizeof(signed char));

    if (result_type == 0) {
        tpu_out = (signed char*)malloc(M * N * sizeof(signed char));
        memset(tpu_out, 0, M * N * sizeof(signed char));
    }

    assign_fix8b_matrix((void*)input_A, M * K, 0);
    assign_fix8b_matrix((void*)input_B, K * N, 0);

    ret = bmcv_matmul_u64(handle, M, N, K, bm_mem_from_system_u64((void*)input_A),
                    bm_mem_from_system_u64((void*)input_B), bm_mem_from_system_u64(tpu_out), A_sign,
                    B_sign, right_shift_bit, result_type, trans_B, alpha, beta);

    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        bm_dev_free(handle);
        return -1;
    }

    free(input_A);
    free(input_B);
    free(tpu_out);

    bm_dev_free(handle);
    return ret;
}

int main() {
    int result_normal = test_matmul();
    int result_over_4g = test_matmul_u64();
    
    if (result_normal == 0 && result_over_4g == 0) {
        return 0;
    } else {
        return 1;
    }
}