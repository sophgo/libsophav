#include "bmcv_api_ext_c.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "test_misc.h"
#include <pthread.h>

#define BMDNN_FIX_POINT_ERR 1
#define BMDNN_FLOAT32_ERR 0.01
#define TIME_COST_US(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))
#define UINT_MIN 0

struct Matpara {
    int M;
    int K;
    int N;
    int trans_A;
    int trans_B;
    int signA;
    int signB;
    int right_shift_bit;
    int result_type;
    float alpha;
    float beta;
};

typedef struct {
    int loop;
    struct Matpara para;
    bm_handle_t handle;
} cv_matmul_thread_arg_t;

extern int cpu_matmul(signed char* src_A, signed char* src_B, void* output, struct Matpara para);

static int parameters_check(struct Matpara para)
{
    int error = 0;
    if (para.trans_B != 0 && para.trans_B != 1) {
        printf("Unsupported value : trans_B = 0/1, 1 is transpose \n");
        error = -1;
    }
    if (para.signA != 0 && para.signA != 1 && para.signB != 0 && para.signB != 1) {
        printf("Unsupported value : 0 is unsigned/ 1 is signed \n");
        error = -1;
    }
    if (para.result_type != 0 && para.result_type != 1 && para.result_type != 2) {
        printf("Unsupported type : 0 is int8/1 is int16 /2 is float \n");
        error = -1;
    }

    return error;
}

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

static int tpu_matmul(signed char* input_A, signed char* input_B, void* output, struct Matpara para, bm_handle_t handle)
{
    bm_status_t ret = BM_SUCCESS;
    int M = para.M;
    int K = para.K;
    int N = para.N;
    int trans_B = para.trans_B;
    int A_sign = para.signA;
    int B_sign = para.signB;
    int right_shift_bit = para.right_shift_bit;
    int result_type = para.result_type;
    float alpha = para.alpha;
    float beta = para.beta;
    struct timeval t1, t2;

    gettimeofday(&t1, NULL);
    ret = bmcv_matmul_u64(handle, M, N, K, bm_mem_from_system_u64((void*)input_A),
                    bm_mem_from_system_u64((void*)input_B), bm_mem_from_system_u64(output), A_sign,
                    B_sign, right_shift_bit, result_type, trans_B, alpha, beta);
    gettimeofday(&t2, NULL);
    printf("Matmul TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        bm_dev_free(handle);
        return -1;
    }

    return 0;
}

static int cmp_result(void* tpu_out, void* cpu_out, struct Matpara para)
{
    int ret = 0;
    int M = para.M;
    int N = para.N;
    int A_sign = para.signA;
    int B_sign = para.signB;
    int result_type = para.result_type;

    if (result_type == 1) {
        ret = array_cmp_fix16b(cpu_out, tpu_out, (A_sign || B_sign) ? 1 : 0, M * N, BMDNN_FIX_POINT_ERR);
    } else if (result_type == 2) {
        ret = array_cmp_float((float*)cpu_out, (float*)tpu_out, M * N, BMDNN_FLOAT32_ERR);
    } else {
        ret = array_cmp_fix8b(cpu_out, tpu_out, (A_sign || B_sign) ? 1 : 0, M * N, BMDNN_FIX_POINT_ERR);
    }

    return ret;
}

static int test_matmul_random(struct Matpara para, bm_handle_t handle)
{
    int M = para.M;
    int K = para.K;
    int N = para.N;
    int result_type = para.result_type;
    signed char* input_A;
    signed char* input_B;
    void* tpu_out;
    void* cpu_out;
    int ret = 0;
    struct timeval t1, t2;
    printf("M = %d, K = %d, N = %d, trans_B = %d, signA = %d, signB = %d, result_type = %d\n",
            M, K, N, para.trans_B, para.signA, para.signB, para.result_type);
    input_A = (signed char*)malloc(M * K * sizeof(signed char));
    input_B = (signed char*)malloc(K * N * sizeof(signed char));

    if (result_type) {
        tpu_out = (signed char*)malloc(M * N * 2 * result_type * sizeof (signed char)); /*int16 or f32*/
        cpu_out = (signed char*)malloc(M * N * 2 * result_type * sizeof (signed char)); /*int16 or f32*/
        memset(tpu_out, 0, M * N * 2 * result_type * sizeof (signed char));
        memset(cpu_out, 0, M * N * 2 * result_type * sizeof (signed char));
    } else {
        tpu_out = (signed char*)malloc(M * N * sizeof(signed char));
        cpu_out = (signed char*)malloc(M * N * sizeof(signed char));
        memset(tpu_out, 0, M * N * sizeof(signed char));
        memset(cpu_out, 0, M * N * sizeof(signed char));
    }

    assign_fix8b_matrix((void*)input_A, M * K, 0);
    assign_fix8b_matrix((void*)input_B, K * N, 0);

    gettimeofday(&t1, NULL);
    ret = cpu_matmul(input_A, input_B, cpu_out, para);
    if (ret) {
        printf("the cpu_matmul is failed!\n");
        goto exit;
    }
    gettimeofday(&t2, NULL);
    printf("Matmul CPU using time = %ld(us)\n", TIME_COST_US(t1, t2));

    ret = tpu_matmul(input_A, input_B, tpu_out, para, handle);
    if (ret) {
        printf("the tpu_matmul is failed!\n");
        goto exit;
    }
    ret = cmp_result(tpu_out, cpu_out, para);
    if (ret) {
        printf("the cpu & tpu cmp is failed!\n");
        goto exit;
    }
exit:
    free(input_A);
    free(input_B);
    free(tpu_out);
    free(cpu_out);
    return ret;
}

void* test_thread_matmul(void* args) {
    cv_matmul_thread_arg_t* cv_matmul_thread_arg = (cv_matmul_thread_arg_t*)args;
    int loop = cv_matmul_thread_arg->loop;
    struct Matpara para = cv_matmul_thread_arg->para;
    bm_handle_t handle = cv_matmul_thread_arg->handle;

    for (int i = 0; i < loop; ++i) {
        int ret = test_matmul_random(para, handle);
        if (ret) {
            printf("------Test Matmul Failed!------\n");
            exit(-1);
        }
        printf("------Test Matmul Successed!------\n");
    }
    return (void*)0;
}

int main(int argc, char *args[])
{
    struct timespec tp;
    clock_gettime(0, &tp);
    int seed = tp.tv_nsec;
    srand(seed);
    int thread_num = 1;
    struct Matpara para;
    int loop = 1;
    para.M = 1 + rand() % 6;
    para.K = 1 + rand() % 512;
    para.N = 1 + rand() % 9216;
    para.trans_A = 0;  //whether to transpose
    para.trans_B = 0;
    para.signA = 0;  //unsigned or singned
    para.signB = 0;
    para.result_type = 0;  //0-int8 1-int16 2-flaot
    para.right_shift_bit = 1;
    para.alpha = 1;
    para.beta = 0;
    int check;
    int ret = 0;
    bm_handle_t handle;
    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("bm_dev_request failed. ret = %d\n", ret);
        return -1;
    }
    if (argc == 2 && atoi(args[1]) == -1){
        printf("usage: %d\n", argc);
        printf("%s thread_num loop trans_B A_sign B_sign result_type M K N\n", args[0]);
        printf("example:\n");
        printf("%s \n", args[0]);
        printf("%s 1 1 0 0 0 0 1 50 100\n", args[0]);
        return 0;
    }

    if (argc > 1) thread_num = atoi(args[1]);
    if (argc > 2) loop = atoi(args[2]);
    if (argc > 3) para.trans_B = atoi(args[3]);
    if (argc > 4) para.signA = atoi(args[4]);
    if (argc > 5) para.signB = atoi(args[5]);
    if (argc > 6) para.result_type = atoi(args[6]);
    if (argc > 7) para.M = atoi(args[7]);
    if (argc > 8) para.K = atoi(args[8]);
    if (argc > 9) para.N = atoi(args[9]);

    check = parameters_check(para);
    if (check) {
        printf("Parameters Failed! \n");
        return check;
    }
    if (para.result_type == 2) {
        para.alpha = rand() % 100 / 10.0;
        para.beta = rand() % 100;
        para.right_shift_bit = 0;
    } else {
        para.alpha = 1;
        para.beta = 0;
        para.right_shift_bit = rand() % 12 + 1;
    }
    printf("seed = %d\n", seed);
    // test for multi-thread
    pthread_t pid[thread_num];
    cv_matmul_thread_arg_t cv_matmul_thread_arg[thread_num];
    for (int i = 0; i < thread_num; i++) {
        cv_matmul_thread_arg[i].loop = loop;
        cv_matmul_thread_arg[i].para = para;
        cv_matmul_thread_arg[i].handle = handle;
        if (pthread_create(&pid[i], NULL, test_thread_matmul, &cv_matmul_thread_arg[i]) != 0) {
            printf("create thread failed\n");
            return -1;
        }
    }
    for (int i = 0; i < thread_num; i++) {
        ret = pthread_join(pid[i], NULL);
        if (ret != 0) {
            printf("Thread join failed\n");
            exit(-1);
        }
    }
    bm_dev_free(handle);
    return ret;
}
