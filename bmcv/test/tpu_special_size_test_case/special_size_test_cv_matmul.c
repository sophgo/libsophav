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

// static int parameters_check(struct Matpara para)
// {
//     int error = 0;
//     if (para.trans_B != 0 && para.trans_B != 1) {
//         printf("Unsupported value : trans_B = 0/1, 1 is transpose \n");
//         error = -1;
//     }
//     if (para.signA != 0 && para.signA != 1 && para.signB != 0 && para.signB != 1) {
//         printf("Unsupported value : 0 is unsigned/ 1 is signed \n");
//         error = -1;
//     }
//     if (para.result_type != 0 && para.result_type != 1 && para.result_type != 2) {
//         printf("Unsupported type : 0 is int8/1 is int16 /2 is float \n");
//         error = -1;
//     }

//     return error;
// }

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

static void local_transpose(void* src, int row_num, int col_num, int sign)
{
    int i, j;

    if (sign == 1) {
        signed char* dst = (signed char*)malloc(row_num * col_num * sizeof(signed char));
        for (i = 0; i < row_num; i++) {
            for (j = 0; j < col_num ; j++) {
                dst[j * row_num + i] = *((signed char*)src + i * col_num + j);;
            }
        }
        memcpy(src, dst, col_num * row_num * sizeof(signed char));
    } else {
        unsigned char* dst = (unsigned char*)malloc(row_num * col_num * sizeof(unsigned char));
        for (i = 0; i < row_num; i++) {
            for (j = 0; j < col_num ; j++) {
                dst[j * row_num + i] = *((unsigned char*)src + i * col_num + j);;
            }
        }
        memcpy(src, dst, col_num * row_num * sizeof(unsigned char));
    }
}

static int cpu_matrix_mul(bool trans_A, bool trans_B, int M, int N, int K, void* src_A, int lda,
                            void* src_B, int ldb, int* src_C, int signA, int signB)
{
    int* pC;
    int i, j, k;
    signed char* pA_signed;
    unsigned char* pA_unsigned;
    signed char* pB_signed;
    unsigned char* pB_unsigned;

    if (src_A == NULL || src_B == NULL || src_C == NULL) {
        printf("the cpu_matrix_mul param error!\n");
        return -1;
    }

    if (signA == 1) {
        pA_signed = (signed char*)malloc(M * K * sizeof(signed char));
        memcpy(pA_signed, (signed char*)src_A, M * K * sizeof(signed char));
    } else {
        pA_unsigned = (unsigned char*)malloc(M * K * sizeof(unsigned char));
        memcpy(pA_unsigned, (unsigned char*)src_A, M * K * sizeof(unsigned char));
    }

    if (signB == 1) {
        pB_signed = (signed char*)malloc(K * N * sizeof(signed char));
        memcpy(pB_signed, (signed char*)src_B, K * N * sizeof(signed char));
    } else {
        pB_unsigned = (unsigned char*)malloc(K * N * sizeof(unsigned char));
        memcpy(pB_unsigned, (unsigned char*)src_B, K * N * sizeof(unsigned char));
    }

    pC = (int*)malloc(M * N * sizeof(int));
    memset(pC, 0, M * N * sizeof(int));

    if (trans_A) {
        if (signA == 1) {
            local_transpose((void*)pA_signed, K, lda, signA);
        } else {
            local_transpose((void*)pA_unsigned, K, lda, signA);
        }
    }

    if (trans_B) {
        if (signB == 1) {
            local_transpose((void*)pB_signed, N, ldb, signB);
        } else {
            local_transpose((void*)pB_unsigned, N, ldb, signB);
        }
    }

    for (i = 0; i < M; i++) { /* matrix_multiply */
        for (j = 0; j < N; j++) {
            for (k = 0; k < K; k++) {
                if (signA == 1) {
                    if (signB == 1) {
                        pC[i * N + j] += pA_signed[i * K + k] * pB_signed[k * N + j];
                    } else {
                        pC[i * N + j] += pA_signed[i * K + k] * pB_unsigned[k * N + j];
                    }
                } else {
                    if (signB == 1) {
                        pC[i * N + j] += pA_unsigned[i * K + k] * pB_signed[k * N + j];
                    } else {
                        pC[i * N + j] += pA_unsigned[i * K + k] * pB_unsigned[k * N + j];
                    }
                }
            }
        }
    }

    memcpy(src_C, pC, M * N * sizeof(int));

    if (signA == 1) {
        free(pA_signed);
    } else {
        free(pA_unsigned);
    }

    if (signB == 1) {
        free(pB_signed);
    } else {
        free(pB_unsigned);
    }
    free(pC);

    return 0;
}

static void rightshift_int(const int *srcData, void *dstData, int shiftNum, int length, int is_16bit)
{
    int tmpData = 0;
    int half_data = 1 << (shiftNum - 1);
    const int maxDataforInt = is_16bit ? INT16_MAX : INT8_MAX;
    const int minDataforInt = is_16bit ? INT16_MIN : INT8_MIN;
    int i;
    int dst_data;

    half_data = shiftNum == 0 ? 0 : half_data;

    for (i = 0; i < length; i++) {

        tmpData = (srcData[i] + half_data) >> shiftNum; /* right shift */
        dst_data = 0; /*saturate to int8 */
        if (tmpData >= maxDataforInt) {
            dst_data = maxDataforInt;
        } else if (tmpData <= minDataforInt) {
            dst_data = minDataforInt;
        } else {
            dst_data = tmpData;
        }
        if (is_16bit) {
            *((signed short*)dstData + i) = (signed short)dst_data;
        } else {
            *((signed char*)dstData + i) = (signed char)dst_data;
        }
    }
}

static void rightshift_unint(const int *srcData, void *dstData, int shiftNum, int length, int is_16bit)
{
    int tmpData = 0;
    long half_data = 1 << (shiftNum - 1);
    const int maxDataforUnInt = is_16bit ? UINT16_MAX : UINT8_MAX;
    int i;
    int dst_data;

    half_data = shiftNum == 0 ? 0 : half_data;

    for (i = 0; i < length; i++) {
        tmpData = (srcData[i] + half_data) >> shiftNum; /* right shift */
        dst_data = 0; /*saturate to int8 */
        if (tmpData >= maxDataforUnInt) {
            dst_data = maxDataforUnInt;
        } else if (tmpData <= UINT_MIN) {
            dst_data = UINT_MIN;
        } else {
            dst_data = tmpData;
        }
        if (is_16bit) {
            *((unsigned short*)dstData + i) = (unsigned short)dst_data;
        } else {
            *((unsigned char*)dstData + i) = (unsigned char)dst_data;
        }
    }
}

static int cpu_matmul(signed char* src_A, signed char* src_B, void* output, struct Matpara para)
{
    int M = para.M;
    int K = para.K;
    int N = para.N;
    int trans_A = para.trans_A;
    int trans_B = para.trans_B;
    int A_sign = para.signA;
    int B_sign = para.signB;
    int right_shift_bit = para.right_shift_bit;
    int result_type = para.result_type;
    float alpha = para.alpha;
    float beta = para.beta;
    int* output_dst = (int*)malloc(M * N * sizeof(int));
    int i;
    int len;
    int ret = 0;

    if (src_A == NULL || src_B == NULL || output == NULL) {
        printf("the cpu_matmul param is error!\n");
        free(output_dst);
        return -1;
    }

    ret = cpu_matrix_mul(trans_A, trans_B, M, N, K, (void*)src_A, trans_A ? M:K, (void*)src_B, \
                        trans_B ? K:N, output_dst, A_sign, B_sign);
    if (ret) {
        printf("the cpy_matrix_mul failed!\n");
        free(output_dst);
        return ret;
    }

    if (result_type != 2) {
        len = M * N;
        if (A_sign == 1 || B_sign == 1) {
            rightshift_int(output_dst, output, right_shift_bit, len, result_type);
        } else {
            rightshift_unint(output_dst, output, right_shift_bit, len, result_type);
        }
    } else {
        float* outputC = (float*)output;
        for (i = 0; i < M * N; i++) {
            outputC[i] = alpha * output_dst[i] + beta;
        }
    }

    free(output_dst);

    return ret;
}

static int tpu_matmul(signed char* input_A, signed char* input_B, void* output, struct Matpara para, long int* tpu_time)
{
    bm_status_t ret = BM_SUCCESS;
    bm_handle_t handle;
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

    ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }

    gettimeofday(&t1, NULL);
    ret = bmcv_matmul_u64(handle, M, N, K, bm_mem_from_system_u64((void*)input_A),
                    bm_mem_from_system_u64((void*)input_B), bm_mem_from_system_u64(output), A_sign,
                    B_sign, right_shift_bit, result_type, trans_B, alpha, beta);
    gettimeofday(&t2, NULL);
    printf("Matmul TPU using time = %ld(us)\n", TIME_COST_US(t1, t2));
    *tpu_time += TIME_COST_US(t1, t2);

    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        bm_dev_free(handle);
        return -1;
    }

    bm_dev_free(handle);
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

static int test_matmul_random(struct Matpara para, bm_handle_t handle, long int* cpu_time, long int* tpu_time)
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
    *cpu_time += TIME_COST_US(t1, t2);

    ret = tpu_matmul(input_A, input_B, tpu_out, para, tpu_time);
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

// void* test_thread_matmul(void* args) {
//     cv_matmul_thread_arg_t* cv_matmul_thread_arg = (cv_matmul_thread_arg_t*)args;
//     int loop = cv_matmul_thread_arg->loop;
//     struct Matpara para = cv_matmul_thread_arg->para;
//     bm_handle_t handle = cv_matmul_thread_arg->handle;

//     for (int i = 0; i < loop; ++i) {
//         int ret = test_matmul_random(para, handle);
//         if (ret) {
//             printf("------Test Matmul Failed!------\n");
//             return (void*)-1;
//         }
//         printf("------Test Matmul Successed!------\n");
//     }
//     return (void*)0;
// }

// int main(int argc, char *args[])
// {
//     struct timespec tp;
//     clock_gettime(0, &tp);
//     int seed = tp.tv_nsec;
//     srand(seed);
//     int thread_num = 1;
//     struct Matpara para;
//     int loop = 1;
//     para.M = 1 + rand() % 6;
//     para.K = 1 + rand() % 512;
//     para.N = 1 + rand() % 9216;
//     para.trans_A = 0;  //whether to transpose
//     para.trans_B = 0;
//     para.signA = 0;  //unsigned or singned
//     para.signB = 0;
//     para.result_type = 0;  //0-int8 1-int16 2-flaot
//     para.right_shift_bit = 1;
//     para.alpha = 1;
//     para.beta = 0;
//     int check;
//     int ret = 0;
//     bm_handle_t handle;
//     ret = bm_dev_request(&handle, 0);
//     if (ret != BM_SUCCESS) {
//         printf("bm_dev_request failed. ret = %d\n", ret);
//         return -1;
//     }
//     if (argc == 2 && atoi(args[1]) == -1){
//         printf("usage: %d\n", argc);
//         printf("%s thread_num loop trans_B A_sign B_sign result_type M K N\n", args[0]);
//         printf("example:\n");
//         printf("%s \n", args[0]);
//         printf("%s 1 1 0 0 0 0 1 50 100\n", args[0]);
//         return 0;
//     }

//     if (argc > 1) thread_num = atoi(args[1]);
//     if (argc > 2) loop = atoi(args[2]);
//     if (argc > 3) para.trans_B = atoi(args[3]);
//     if (argc > 4) para.signA = atoi(args[4]);
//     if (argc > 5) para.signB = atoi(args[5]);
//     if (argc > 6) para.result_type = atoi(args[6]);
//     if (argc > 7) para.M = atoi(args[7]);
//     if (argc > 8) para.K = atoi(args[8]);
//     if (argc > 9) para.N = atoi(args[9]);

//     check = parameters_check(para);
//     if (check) {
//         printf("Parameters Failed! \n");
//         return check;
//     }
//     if (para.result_type == 2) {
//         para.alpha = rand() % 100 / 10.0;
//         para.beta = rand() % 100;
//         para.right_shift_bit = 0;
//     } else {
//         para.alpha = 1;
//         para.beta = 0;
//         para.right_shift_bit = rand() % 12 + 1;
//     }
//     printf("seed = %d\n", seed);
//     // test for multi-thread
//     pthread_t pid[thread_num];
//     cv_matmul_thread_arg_t cv_matmul_thread_arg[thread_num];
//     for (int i = 0; i < thread_num; i++) {
//         cv_matmul_thread_arg[i].loop = loop;
//         cv_matmul_thread_arg[i].para = para;
//         cv_matmul_thread_arg[i].handle = handle;
//         if (pthread_create(&pid[i], NULL, test_thread_matmul, &cv_matmul_thread_arg[i]) != 0) {
//             printf("create thread failed\n");
//             return -1;
//         }
//     }
//     for (int i = 0; i < thread_num; i++) {
//         ret = pthread_join(pid[i], NULL);
//         if (ret != 0) {
//             printf("Thread join failed\n");
//             exit(-1);
//         }
//     }
//     bm_dev_free(handle);
//     return ret;
// }

int main(int argc, char* args[])
{
    printf("log will be saved in special_size_test_cv_matmul.txt\n");
    FILE *original_stdout = stdout;
    FILE *file = freopen("special_size_test_cv_matmul.txt", "w", stdout);
    if (file == NULL) {
        fprintf(stderr,"无法打开文件\n");
        return -1;
    }
    struct timespec tp;
    clock_gettime(0, &tp);
    int seed = tp.tv_nsec;
    srand(seed);
    int loop = 0;
    int M_num[8] = {1, 8, 16, 32, 64, 96, 128, 256};
    int K_num[8] = {1, 8, 16, 64, 100, 256, 400, 512};
    int N_num[8] = {1, 64, 100, 256, 400, 512, 1024, 2048};
    bool trans_B_num[2] = {0, 1};
    int A_sign = 0;
    int B_sign = 1;
    int result_type_num[2] = {1, 2}; //int16  float32
    int rshift_num[3] = {0, 2, 4}; // int16 时的参数
    float alpha = 10.0f;
    float beta = -10.0f;
    bm_handle_t handle;
    int i, j, k, m;
    bm_status_t ret = bm_dev_request(&handle, 0);
    if (ret != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", ret);
        return -1;
    }
    struct Matpara para;
    para.alpha = alpha;
    para.beta = beta;
    para.signA = A_sign;
    para.signB = B_sign;
    para.trans_A = 0;

    for (i = 0; i < 2; ++i) {
        para.trans_B = trans_B_num[i];
        for (j = 0; j < 2; ++j) {
            para.result_type = result_type_num[j];
            if (j == 0) {
                for (k = 0; k < 3; ++k) {
                    para.right_shift_bit = rshift_num[k];
                    for (m = 0; m < 8; ++m) {
                        para.M = M_num[m];
                        para.N = N_num[m];
                        para.K = K_num[m];
                        printf("the M = %d, the K = %d, the N = %d, B_trans = %d, result_type = %d, right_shift_bit = %d\n", \
                                para.M, para.K, para.N, para.trans_B, para.result_type, para.right_shift_bit);
                        long int cputime = 0;
                        long int tputime = 0;
                        for(loop = 0; loop < 10; loop++) {
                            ret = test_matmul_random(para, handle, &cputime, &tputime);
                            if (ret) {
                                printf("------TEST CV_MATMUL FAILED------\n");
                                return -1;
                            }
                        }
                        printf("-----------average MATMUL CPU time = %ldus----------\n", cputime / 10);
                        printf("-----------average MATMUL TPU time = %ldus----------\n", tputime / 10);
                    }
                }
            } else if (j == 1) {
                for (m = 0; m < 8; ++m) {
                    para.M = M_num[m];
                    para.N = N_num[m];
                    para.K = K_num[m];
                    printf("the M = %d, the K = %d, the N = %d, B_trans = %d, result_type = %d, alpha = %f, beta = %f\n", \
                            para.M, para.K, para.N, para.trans_B, para.result_type, para.alpha, para.beta);
                    long int cputime = 0;
                    long int tputime = 0;
                    for(loop = 0; loop < 10; loop++) {
                        ret = test_matmul_random(para, handle, &cputime, &tputime);
                        if (ret) {
                            printf("------TEST CV_MATMUL FAILED------\n");
                            return -1;
                        }
                    }
                    printf("-----------average MATMUL CPU time = %ldus----------\n", cputime / 10);
                    printf("-----------average MATMUL TPU time = %ldus----------\n", tputime / 10);
                }
            }
        }
    }
    bm_dev_free(handle);
    fclose(stdout);
    stdout = original_stdout;
    return ret;
}